#ifndef CODEPROJECTTEST_SSTABLE_H
#define CODEPROJECTTEST_SSTABLE_H

#include <cstdint>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <string>
#include <filesystem>
#include "BloomFilter.h"
#include "MemTable.h"
#include "VLog.h"

struct Header
{
    uint64_t timestamp;
    uint64_t kv_count;
    uint64_t min_key;
    uint64_t max_key;
};

struct Tuple
{
    uint64_t key;
    uint64_t offset;
    uint32_t vlen;
};

class SSTable
{
private:
    Header header;
    BloomFilter bloomFilter;
    std::map<uint64_t, Tuple> tuples;
    string relatedFile;
    int *fileNum;

public:
    inline SSTable(uint64_t timestamp, int *fileNum);
    inline void addTuple(uint64_t key, uint64_t offset, uint32_t vlen);
    inline void flush(int level);
    inline uint64_t getMinestKey() const { return header.min_key; }
    inline uint64_t getMaxestKey() const { return header.max_key; }
    inline uint64_t getTimestamp() const { return header.timestamp; }
    inline uint64_t getKvCount() const { return header.kv_count; }
    inline bool checkKey(uint64_t key);
    inline Tuple getTuple(uint64_t key);
    inline void scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list, VLOG::VLog *vLog);
    inline void load(const std::string &filename);
    inline void deletedRelatedFile();
    inline void merge(SSTable *ssTable);
    inline const std::map<uint64_t, Tuple> &getTuples() const { return tuples; }
    inline void eraseTrash();
};

SSTable::SSTable(uint64_t timestamp, int *fileNum)
{
    this->fileNum = fileNum;
    header.timestamp = timestamp;
    header.kv_count = 0;
    header.min_key = std::numeric_limits<uint64_t>::max();
    header.max_key = std::numeric_limits<uint64_t>::min();
}

void SSTable::addTuple(uint64_t key, uint64_t offset, uint32_t vlen)
{
    if (key < header.min_key)
        header.min_key = key;
    if (key > header.max_key)
        header.max_key = key;

    header.kv_count++;

    bloomFilter.insert(key);

    Tuple tuple;
    tuple.key = key;
    tuple.offset = offset;
    tuple.vlen = vlen;

    tuples[key] = tuple;
}

Tuple SSTable::getTuple(uint64_t key)
{
    auto it = tuples.find(key);
    if (it != tuples.end())
    {
        return it->second;
    }
    else
        cout << "Key" << key << "is checked but no found in SSTable" << endl;
}

bool SSTable::checkKey(uint64_t key)
{
    if (!bloomFilter.query(key))
        return false;

    if (tuples.find(key) != tuples.end())
        return true;
    else
        return false;
}
void SSTable::scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list, VLOG::VLog *vLog)
{
    auto it = tuples.lower_bound(key1);
    while (it != tuples.end() && it->first <= key2)
    {

        bool found = false;
        for (const auto &pair : list)
        {
            if (pair.first == it->first)
            {
                found = true;
                break;
            }
        }

        if (!found && it->second.vlen != 0)

            list.push_back(std::make_pair(it->first, vLog->get(it->second.offset, it->second.vlen)));
        ++it;
    }
}

void SSTable::flush(int level)
{
    if (header.kv_count == 0)
        return;

    std::string level_dir = "./data/level-" + std::to_string(level);

    if (!utils::dirExists(level_dir))
    {
        utils::_mkdir(level_dir);
    }

    std::string filename = level_dir + "/" + std::to_string(*fileNum) + ".sst";
    (*fileNum) += 1;
    relatedFile = filename;

    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    file.write(reinterpret_cast<char *>(&header), sizeof(header));
    bloomFilter.serialize(file);

    for (const auto &pair : tuples)
    {
        const Tuple &tuple = pair.second;

        file.write(reinterpret_cast<const char *>(&tuple), sizeof(tuple));
    }

    if (!file)
    {
        throw std::runtime_error("Failed to write to file: " + filename);
    }

    file.close();
}

void SSTable::load(const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    file.read(reinterpret_cast<char *>(&header), sizeof(header));
    bloomFilter.deserialize(file);

    tuples.clear();
    for (uint64_t i = 0; i < header.kv_count; ++i)
    {
        Tuple tuple;
        file.read(reinterpret_cast<char *>(&tuple), sizeof(tuple));
        tuples[tuple.key] = tuple;
    }

    if (!file)
    {
        throw std::runtime_error("Failed to read from file: " + filename);
    }

    file.close();
}
void SSTable::deletedRelatedFile()
{
    std::filesystem::remove(relatedFile);
}
void SSTable::eraseTrash()
{
    for (auto it = tuples.begin(); it != tuples.end();)
    {
        if (it->second.vlen == 0)
        {
            it = tuples.erase(it);
            header.kv_count--;
        }
        else
        {
            ++it;
        }
    }
}

void SSTable::merge(SSTable *ssTable)
{
    for (const auto &pair : ssTable->getTuples())
    {
        const Tuple &tuple = pair.second;
        if (tuples.find(pair.first) == tuples.end())
        {
            addTuple(pair.first, tuple.offset, tuple.vlen);
            header.kv_count++;
            if (pair.first < header.min_key)
                header.min_key = pair.first;
            if (pair.first > header.max_key)
                header.max_key = pair.first;
        }
        else
        {
            if (header.timestamp < ssTable->getTimestamp())
            {
                cout << "MERGE AND ENTER" << endl;
                tuples[pair.first] = pair.second;
            }
        }
    }

    if (ssTable->getTimestamp() > header.timestamp)
        header.timestamp = ssTable->getTimestamp();
}

#endif // CODEPROJECTTEST_SSTABLE_H
