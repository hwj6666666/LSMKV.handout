#pragma once

#include <set>
#include "kvstore_api.h"
#include "MemTable.h"
#include "VLog.h"
#include "SSTable.h"
#include "LevelSSTables.h"
using namespace std;

class KVStore : public KVStoreAPI
{
	// You can add your implementation here
private:
	uint64_t timestamp;
	MemTable<uint64_t, string> *memTable;
	VLOG::VLog *vLog;
	int *fileNum;
	string dir;
	string vlog;
	LevelSSTables *allLevelSSTables;

public:
	KVStore(const std::string &dir, const std::string &vlog);
	~KVStore();

	void put(uint64_t key, const std::string &s) override;
	std::string get(uint64_t key) override;
	bool del(uint64_t key) override;
	void reset() override;
	void scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list) override;
	void gc(uint64_t chunk_size) override;
	void compaction();
	int countLevel(int n);
	void memToDisk();
	void iniTimestamp();
};
