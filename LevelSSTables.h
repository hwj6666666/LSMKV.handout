#pragma once

#include "SSTable.h"
#include <map>
#include <set>
#include <string>
#include "VLog.h"

class LevelSSTables
{
public:
    struct SSTablePtrComp
    {
        // bool isLevel0 = false;
        // SSTablePtrComp() : isLevel0(isLevel0) {}
        bool operator()(const SSTable *lhs, const SSTable *rhs) const
        {
            // if (isLevel0)
            // {
            //     return lhs->getTimestamp() > rhs->getTimestamp();
            // }
            // else
            // {
            //     return lhs->getMinestKey() < rhs->getMinestKey();
            // }
            if(lhs->getTimestamp() == rhs->getTimestamp())
            {
                return lhs->getMinestKey() < rhs->getMinestKey();
            }
            else
            {
                return lhs->getTimestamp() > rhs->getTimestamp();
            }
        }
    };

    inline LevelSSTables(VLOG::VLog *vlog, int *fileNum);
    inline ~LevelSSTables();
    inline string getValueByKey(uint64_t key);
    inline void reset();
    inline void scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list);
    inline uint64_t getOffset(uint64_t key);
    inline void addSSTable(SSTable *ssTable)
    {
        levelSSTables[0].insert(ssTable);
    }
    inline void cacheSSTable();
    inline void compaction();

private:
    std::map<int, std::set<SSTable *, SSTablePtrComp>> levelSSTables;
    VLOG::VLog *vLog;
    int *fileNum;
};

LevelSSTables::LevelSSTables(VLOG::VLog *vlog, int *fileNum)
{
    this->vLog = vlog;
    this->fileNum = fileNum;
    // levelSSTables[0] = std::set<SSTable *, SSTablePtrComp>(SSTablePtrComp(true));
    cacheSSTable();
}
LevelSSTables::~LevelSSTables()
{
    for (auto &level : levelSSTables)
    {
        std::set<SSTable *, SSTablePtrComp> &ssTables = level.second;

        for (SSTable *ssTable : ssTables)
        {
            delete ssTable;
        }
    }
    levelSSTables.clear();
}
string LevelSSTables::getValueByKey(uint64_t key)
{
    for (auto &level : levelSSTables)
    {
        // int levelNumber = level.first;
        std::set<SSTable *, SSTablePtrComp> &ssTables = level.second;

        for (SSTable *ssTable : ssTables)
        {
            if (ssTable->checkKey(key))
            {
                Tuple tuple = ssTable->getTuple(key);

                if (tuple.vlen == 0)
                    return "";
                else
                {
                    return vLog->get(tuple.offset, tuple.vlen);
                }
            }
        }
    }
    return "";
}
void LevelSSTables::reset()
{
    for (auto &level : levelSSTables)
    {
        std::set<SSTable *, SSTablePtrComp> &ssTables = level.second;

        for (SSTable *ssTable : ssTables)
        {
            delete ssTable;
        }
    }
    levelSSTables.clear();
}

void LevelSSTables::scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list)
{

    for (auto &level : levelSSTables)
    {
        std::set<SSTable *, SSTablePtrComp> &ssTables = level.second;

        for (SSTable *ssTable : ssTables)
        {
            ssTable->scan(key1, key2, list, this->vLog);
        }
    }
}

uint64_t LevelSSTables::getOffset(uint64_t key)
{
    uint64_t tempOff = 0;

    bool isDeleted = true;

    for (auto &level : levelSSTables)
    {
        std::set<SSTable *, SSTablePtrComp> &ssTables = level.second;

        for (SSTable *ssTable : ssTables)
        {
            if (ssTable->checkKey(key))
            {

                Tuple tuple = ssTable->getTuple(key);
                if (tuple.vlen == 0)
                {
                    if (isDeleted)
                        return 0;
                    else
                        return tempOff;
                }
                else
                {
                    isDeleted = false;

                    if (tuple.offset > tempOff)
                        tempOff = tuple.offset;
                }
            }
        }
    }
    return tempOff;
}
void LevelSSTables::cacheSSTable()
{
    std::string dir = "./data";

    // Check if the path exists and is a directory
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
    {
        return;
    }

    for (const auto &entry : std::filesystem::directory_iterator(dir))
    {
        // levelSSTables[0] = std::set<SSTable *, SSTablePtrComp>(SSTablePtrComp(1));

        if (entry.is_directory())
        {
            std::string levelDir = entry.path().filename().string();
            // Check if the directory name starts with "level-"
            if (levelDir.find("level-") == 0)
            {
                // Print the level number
                string levelStr = levelDir.substr(6);
                int num = stoi(levelStr);

                for (const auto &file : std::filesystem::directory_iterator(entry.path()))
                {
                    if (file.is_regular_file())
                    {
                        std::string filename = file.path().filename().string();
                        // Combine the directory and the filename to form the full path
                        std::string fullPath = dir + "/" + levelDir + "/" + filename;
                        SSTable *ssTable = new SSTable(0, fileNum);
                        ssTable->load(fullPath);
                        levelSSTables[num].insert(ssTable);
                    }
                }
            }
        }
    }
}
void LevelSSTables::compaction()
{

    SSTable *tempSSTable = new SSTable(0, fileNum);

    for (auto &level : levelSSTables)
    {
        int levelNumber = level.first;
        std::set<SSTable *, SSTablePtrComp> &ssTables = level.second;

        if (levelNumber == 0)
        {
            if (ssTables.size() <= (1 << (levelNumber + 1)))
            {
                return;
            }
            else
            {
                for (auto it = ssTables.begin(); it != ssTables.end();)
                {
                    SSTable *ssTable = *it;
                    tempSSTable->merge(ssTable);
                    ssTable->deletedRelatedFile();
                    it = ssTables.erase(it);
                    delete ssTable;
                }
            }
        }
        else
        {
            if (tempSSTable->getKvCount() > 0)
            {
                uint64_t min = tempSSTable->getMinestKey();
                uint64_t max = tempSSTable->getMaxestKey();

                for (auto it = ssTables.begin(); it != ssTables.end();)
                {
                    SSTable *ssTable = *it;
                    if ((ssTable->getMaxestKey() >= min)|| ssTable->getMinestKey() <= max)
                    {
                        tempSSTable->merge(ssTable);
                        ssTable->deletedRelatedFile();
                        it = ssTables.erase(it);
                        delete ssTable;
                    }
                    else
                    {
                        ++it;
                    }
                }

                uint64_t timestamp = tempSSTable->getTimestamp();

                SSTable *resultSSTable = new SSTable(timestamp, fileNum);

                for (const auto &tuplePair : tempSSTable->getTuples())
                {
                    Tuple tuple = tuplePair.second;
                    resultSSTable->addTuple(tuple.key, tuple.offset, tuple.vlen);
                    if (resultSSTable->getKvCount() >= MAXMEMBER)
                    {
                        resultSSTable->flush(levelNumber);
                        ssTables.insert(resultSSTable);
                        resultSSTable = new SSTable(timestamp, fileNum);
                    }
                }

                if (resultSSTable->getKvCount() > 0)
                {
                    resultSSTable->flush(levelNumber);
                    ssTables.insert(resultSSTable);
                }

                tempSSTable = new SSTable(0, fileNum);

                if (ssTables.size() <= (1 << (levelNumber + 1)))
                {
                    // for (SSTable *ssTable : ssTables)
                    {
                        // ssTable->eraseTrash();
                    }
                    // return;
                }
                else
                {
                    auto it = ssTables.begin();
                    std::advance(it, 1 << (levelNumber + 1));

                    // 截取超出部分
                    std::set<SSTable *> mergeSet(it, ssTables.end());
                    ssTables.erase(it, ssTables.end());

                    // 对 mergeSet 中的元素进行合并操作
                    for (SSTable *ssTable : mergeSet)
                    {
                        tempSSTable->merge(ssTable);
                        ssTable->deletedRelatedFile();
                        delete ssTable;
                    }
                }
            }
        }
    }

    uint64_t timestamp = tempSSTable->getTimestamp();
    int maxLevel = levelSSTables.rbegin()->first;

    if (tempSSTable->getKvCount() > 0)
    {
        SSTable *resultSSTable = new SSTable(timestamp, fileNum);
        for (const auto &tuplePair : tempSSTable->getTuples())
        {
            Tuple tuple = tuplePair.second;
            resultSSTable->addTuple(tuple.key, tuple.offset, tuple.vlen);
            if (resultSSTable->getKvCount() >= MAXMEMBER)
            {
                resultSSTable->flush(maxLevel + 1);
                levelSSTables[maxLevel + 1].insert(resultSSTable);
                resultSSTable = new SSTable(timestamp, fileNum);
            }
        }

        if (resultSSTable->getKvCount() > 0)
        {
            resultSSTable->flush(maxLevel + 1);
            levelSSTables[maxLevel + 1].insert(resultSSTable);
        }
    }
}