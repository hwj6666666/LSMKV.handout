#include <fstream>
#include <vector>
#include "MemTable.h"
#include "utils.h"
#include <iostream>
using namespace std;
#ifndef VLOG_H
#define VLOG_H

namespace VLOG
{

    struct Entry
    {
        uint8_t magic;
        uint16_t checksum;
        uint64_t key;
        uint32_t vlen;
        std::string value;

        Entry() : magic(0), checksum(0), key(0), vlen(0) {}

        Entry(uint64_t key, const std::string &value)
            : magic(0xff), key(key), vlen(value.size()), value(value)
        {
            std::vector<uint8_t> data;

            for (int i = 0; i < sizeof(key); ++i)
                data.push_back((key >> (i * 8)) & 0xFF);

            for (int i = 0; i < sizeof(vlen); ++i)
                data.push_back((vlen >> (i * 8)) & 0xFF);

            for (char c : value)
                data.push_back(c);

            checksum = utils::crc16(data);
        }
    };

    class VLog
    {
    private:
        std::string filename;
        std::fstream file;
        // uint64_t tail, head;

    public:
        inline VLog(const std::string &filename);
        inline ~VLog();
        inline uint64_t append(uint64_t key, const std::string &value);
        inline uint64_t getTail();
        inline std::string get(uint64_t offset, uint32_t vlen);
        inline map<uint64_t, pair<uint64_t, string>> getKAndOffset(uint64_t chunk_size);
    };

    VLog::~VLog()
    {
        if (file.is_open())
        {
            file.close();
        }
    }

    VLog::VLog(const std::string &filename)
        : filename(filename)
    {
    // Open the file in read-write mode. If the file does not exist, it will be created.
    boo:
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

        // If the file does not exist, create it.
        if (!file.is_open())
        {
            file.clear();
            file.open(filename, std::ios::out | std::ios::binary);
            file.close();

            goto boo;
        }
    }

    uint64_t VLog::append(uint64_t key, const std::string &value)
    {
        file.seekp(0, std::ios::end);

        Entry entry(key, value);

        file.write((char *)&entry.magic, sizeof(entry.magic));
        file.write((char *)&entry.checksum, sizeof(entry.checksum));
        file.write((char *)&key, sizeof(key));
        file.write((char *)&entry.vlen, sizeof(entry.vlen));
        file.write(value.c_str(), entry.vlen);

        file.flush();

        // Return the offset of the value field
        uint64_t head = file.tellp();
        return head - entry.vlen;
    }

    std::string VLog::get(uint64_t offset, uint32_t vlen)
    {
        // Close and reopen the file to ensure that we read the latest data
        file.close();
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

        // Move the read pointer to the start of the value field
        file.seekg(offset);

        // Create a vector to store the data
        std::vector<char> data(vlen);

        // Read the data from the file
        file.read(data.data(), vlen);

        // Convert the vector to a string and return it
        return std::string(data.begin(), data.end());
    }

    std::map<uint64_t, std::pair<uint64_t, std::string>> VLog::getKAndOffset(uint64_t chunk_size)
    {
        std::map<uint64_t, std::pair<uint64_t, std::string>> result;
        uint64_t totalSize = 0;

        file.seekp(0, std::ios::end);
        uint64_t head = file.tellp();
        uint64_t tail = getTail();
        uint64_t fileSize = head - tail; // 获取文件大小

        while (totalSize < chunk_size && tail < head)
        {
            file.seekg(tail);

            Entry entry;

            if (!file.read((char *)&entry.magic, sizeof(entry.magic)) || entry.magic != 0xff) // 检查是否到达文件末尾或者文件格式错误
                break;

            if (!file.read((char *)&entry.checksum, sizeof(entry.checksum)) ||
                !file.read((char *)&entry.key, sizeof(entry.key)) ||
                !file.read((char *)&entry.vlen, sizeof(entry.vlen)))
                break;

            std::vector<char> value(entry.vlen);
            if (!file.read(value.data(), entry.vlen))
                break;

            entry.value.assign(value.begin(), value.end());

            std::vector<uint8_t> data;
            for (int i = 0; i < sizeof(entry.key); ++i)
                data.push_back((entry.key >> (i * 8)) & 0xFF);
            for (int i = 0; i < sizeof(entry.vlen); ++i)
                data.push_back((entry.vlen >> (i * 8)) & 0xFF);
            for (char c : entry.value)
                data.push_back(c);

            if (entry.checksum != utils::crc16(data))
            {
                cerr << "Checksum mismatch at offset: " << tail << endl;
                break;
            }

            uint64_t offset = tail + sizeof(entry.magic) + sizeof(entry.checksum) + sizeof(entry.key) + sizeof(entry.vlen);
            result[entry.key] = std::make_pair(offset, entry.value);

            uint64_t entrySize = sizeof(entry.magic) + sizeof(entry.checksum) + sizeof(entry.key) + sizeof(entry.vlen) + entry.vlen;
            totalSize += entrySize;

            // 打洞
            if (utils::de_alloc_file(filename, tail, entrySize) != 0)
            {
                cerr << "Failed to de-allocate file space at offset: " << tail << endl;
                break;
            }

            // 更新 tail，但不要超过文件末尾
            tail = std::min(tail + entrySize, fileSize);
        }

        return result;
    }

    uint64_t VLog::getTail()
    {
        off_t offset = utils::seek_data_block(filename);
        if (offset < 0)
        {
            std::cerr << "Failed to seek data block in file: " << filename << std::endl;
            return 0;
        }

        std::fstream tempFile(filename, std::ios::in | std::ios::binary);
        if (!tempFile.is_open())
        {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return 0;
        }

        tempFile.seekg(offset);

        uint8_t magic;
        while (tempFile.read((char *)&magic, sizeof(magic)))
        {
            if (magic == 0xff) // 找到 vLog entry 的开始
            {
                Entry entry(0, "");
                tempFile.read((char *)&entry.checksum, sizeof(entry.checksum));
                tempFile.read((char *)&entry.key, sizeof(entry.key));
                tempFile.read((char *)&entry.vlen, sizeof(entry.vlen));

                std::vector<char> value(entry.vlen);
                tempFile.read(value.data(), entry.vlen);
                entry.value.assign(value.begin(), value.end());

                std::vector<uint8_t> data;
                for (int i = 0; i < sizeof(entry.key); ++i)
                    data.push_back((entry.key >> (i * 8)) & 0xFF);
                for (int i = 0; i < sizeof(entry.vlen); ++i)
                    data.push_back((entry.vlen >> (i * 8)) & 0xFF);
                for (char c : entry.value)
                    data.push_back(c);

                if (entry.checksum == utils::crc16(data)) // 校验通过
                {
                    tempFile.close();
                    return offset;
                }
            }

            offset++;
            tempFile.seekg(offset);
        }

        tempFile.close();
        return 0;
    }
}

#endif