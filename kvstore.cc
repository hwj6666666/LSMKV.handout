#include "kvstore.h"
#include <string>

#include <iostream> // Add this line
KVStore::KVStore(const std::string &dir, const std::string &vlog) : KVStoreAPI(dir, vlog)
{
	MemTable = new SkipList<uint64_t, string>(32);
	std::cout << "KVStore constructed" << std::endl;
}

KVStore::~KVStore()
{
	delete MemTable;
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s)
{
	// std::cout << "PUT" << key << endl;
	MemTable->put(key, s);
}
/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key)
{
	// std::cout << "GET" << key << endl;
	return MemTable->get(key);
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key)
{
	// std::cout<<"DEL"<<key<<endl;
	if (MemTable->isExist(key))
	{
		MemTable->deleteKey(key);
		return true;
	}
	return false;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset()
{
	MemTable->reset();
}

/**
 * Return a list including all the key-value pair between key1 and key2.
 * keys in the list should be in an ascending order.
 * An empty string indicates not found.
 */
void KVStore::scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list)
{
	map<uint64_t, string> result = MemTable->scan(key1, key2);

	for (auto it = result.begin(); it != result.end(); ++it)
	{
		list.push_back(*it);
	}
}

/**
 * This reclaims space from vLog by moving valid value and discarding invalid value.
 * chunk_size is the size in byte you should AT LEAST recycle.
 */
void KVStore::gc(uint64_t chunk_size)
{
}