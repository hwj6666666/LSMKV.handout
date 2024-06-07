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
        bool isLevel0 = false;
        SSTablePtrComp(bool isLevel0 = false) : isLevel0(isLevel0) {}
        bool operator()(const SSTable *lhs, const SSTable *rhs) const
        {
            if (isLevel0)
            {
                return lhs->getTimestamp() > rhs->getTimestamp();
            }
            else
            {
                return lhs->getMinestKey() < rhs->getMinestKey();
            }
        }
    };

    inline LevelSSTables(VLOG::VLog *vlog);
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

private:
    std::map<int, std::set<SSTable *, SSTablePtrComp>> levelSSTables;
    VLOG::VLog *vLog;
};

LevelSSTables::LevelSSTables(VLOG::VLog *vlog)
{
    this->vLog = vlog;
    levelSSTables[0] = std::set<SSTable *, SSTablePtrComp>(SSTablePtrComp(true));
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
        levelSSTables[0] = std::set<SSTable *, SSTablePtrComp>(SSTablePtrComp(1));

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
                        SSTable *ssTable = new SSTable(0);
                        ssTable->load(fullPath);
                        levelSSTables[num].insert(ssTable);
                    }
                }
            }
        }
    }
}