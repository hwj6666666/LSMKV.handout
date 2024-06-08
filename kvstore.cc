#include "kvstore.h"
#include <string>

#include <iostream> // Add this line
#include "parameters.h"
#include "SSTable.h"
KVStore::KVStore(const std::string &dir, const std::string &vlog) : KVStoreAPI(dir, vlog)
{
	if (!utils::dirExists(dir))
	{
		utils::_mkdir(dir);
	}

	vLog = new VLOG::VLog(vlog);


	this->dir = dir;
	this->vlog = vlog;

	memTable = new MemTable<uint64_t, string>();

	iniTimestamp();
	fileNum = new int(timestamp);

	allLevelSSTables = new LevelSSTables(vLog, fileNum);
}

KVStore::~KVStore()
{
	memToDisk();
	delete memTable;
	allLevelSSTables->~LevelSSTables();
	delete allLevelSSTables;
	delete fileNum;
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s)
{
	memTable->put(key, s);

	if (memTable->size() >= MAXMEMBER)
	{
		memToDisk();
	}
}
/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key)
{
	string result = memTable->get(key);

	if (result == "~DELETED~")
	{
		return "";
	}
	else if (result != "")
		return result;
	else
	{
		return allLevelSSTables->getValueByKey(key);
	}
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key)
{
	if (get(key) == "")
		return false;
	else
	{
		put(key, "~DELETED~");
		return true;
	}
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset()
{
	timestamp = 1;

	std::string dir = "./data"; // Replace with your directory path

	// Check if the path exists and is a directory
	if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir))
	{
		// Remove the directory and all its contents
		std::filesystem::remove_all(dir);
	}

	// After removing, you may want to recreate the directory
	std::filesystem::create_directories(dir);

	memTable->reset();

	vLog = new VLOG::VLog(vlog);

	allLevelSSTables->~LevelSSTables();
	allLevelSSTables = new LevelSSTables(vLog, fileNum);
	*fileNum = 1;
}

/**
 * Return a list including all the key-value pair between key1 and key2.
 * keys in the list should be in an ascending order.
 * An empty string indicates not found.
 */
void KVStore::scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list)
{
	memTable->scan(key1, key2, list);

	allLevelSSTables->scan(key1, key2, list);

	list.sort();
}

/**
 * This reclaims space from vLog by moving valid value and discarding invalid value.
 * chunk_size is the size in byte you should AT LEAST recycle.
 */
void KVStore::gc(uint64_t chunk_size)
{
	map<uint64_t, pair<uint64_t, string>> result = vLog->getKAndOffset(chunk_size);

	for (const auto &pair : result)
	{
		uint64_t key = pair.first;
		uint64_t offset = pair.second.first;
		string value = pair.second.second;

		if (memTable->isExist(key))
		{
		}

		else if (allLevelSSTables->getOffset(key) == offset)
		{
			memTable->put(key, value);
		}
	}

	if (memTable->size() > 0)
		memToDisk();
}

void KVStore::compaction()
{
	allLevelSSTables->compaction();
	return;
}

int KVStore::countLevel(int n)
{
	std::string dir = "./data/level-" + std::to_string(n);

	// Check if the path exists and is a directory
	if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
	{
		return 0;
	}

	int fileCount = 0;

	for (const auto &entry : std::filesystem::directory_iterator(dir))
	{
		if (entry.is_regular_file())
		{
			fileCount++;
		}
	}

	return fileCount;
}

void KVStore::memToDisk()
{

	map<uint64_t, string> result = memTable->getAll();

	if (result.size() == 0)
		return;

	memTable->reset();

	SSTable *ssTable = new SSTable(timestamp, fileNum);
	timestamp++;

	for (const auto &pair : result)
	{
		uint64_t key = pair.first;
		std::string value = pair.second;

		if (value == "~DELETED~")
		{
			ssTable->addTuple(key, 0, 0);
		}
		else
		{
			uint64_t offset = vLog->append(key, value);
			ssTable->addTuple(key, offset, value.length());
		}
	}

	ssTable->flush(0);
	allLevelSSTables->addSSTable(ssTable);

	if (countLevel(0) > 2)
	{
		compaction();
	}
}

void KVStore::iniTimestamp()
{
	string basePath = "./data";
	if (!utils::dirExists(basePath))
	{
		timestamp = 1;
		return;
	}

	std::vector<std::string> levelDirs;
	if (utils::scanDir(basePath, levelDirs) <= 0)
	{
		timestamp = 1;
		return;
	}

	int maxFileNum = 1;
	for (const auto &levelDir : levelDirs)
	{
		if (levelDir.find("level-") != 0)
			continue; // Skip directories not starting with "level-"

		string path = basePath + "/" + levelDir;
		std::vector<std::string> files;
		if (utils::scanDir(path, files) <= 0)
			continue; // Skip empty directories

		for (const auto &file : files)
		{
			size_t pos = file.find(".sst");
			if (pos != std::string::npos)
			{
				try
				{
					int num = std::stoi(file.substr(0, pos));
					maxFileNum = std::max(maxFileNum, num);
				}
				catch (const std::invalid_argument &)
				{
					// Ignore files that do not start with a number
				}
			}
		}
	}

	timestamp = maxFileNum + 1;
}