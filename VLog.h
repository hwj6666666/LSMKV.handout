#include <fstream>
#include <vector>
#include "MemTable.h"
#include "utils.h"
#include <iostream>
using namespace std;

struct Entry
{
    uint8_t magic;
    uint16_t checksum;
    uint64_t key;
    uint32_t vlen;
    std::string value;

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
    uint64_t tail, head;

public:
    inline VLog(const std::string &filename);
    inline ~VLog();
    inline uint64_t append(uint64_t key, const std::string &value);
    inline uint64_t getTail();
    inline std::string get(uint64_t offset, uint32_t vlen);
};

VLog::~VLog()
{
    if (file.is_open())
    {
        file.close();
    }
}

VLog::VLog(const std::string &filename)
    : filename(filename), tail(0), head(0)
{
    // Open the file in read-write mode. If the file does not exist, it will be created.
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

    // If the file does not exist, create it.
    if (!file.is_open())
    {
        file.clear();
        file.open(filename, std::ios::out | std::ios::binary);
    }

    // Move the write pointer to the end of the file
    file.seekp(0, std::ios::end);

    // Update the head
    head = file.tellp();
}

uint64_t VLog::getTail()
{
    //     off_t offset = utils::seek_data_block(filename);
    //     file.seekg(offset, std::ios::beg);
    //     uint16_t magic;
    //     while (file.read((char *)&magic, sizeof(magic)))
    //     {
    //         if (magic == 0xff)
    //         {
    //             uint16_t checksum;
    //             uint64_t key;
    //             uint64_t vlen;
    //             std::string value;
    //             file.seekg(-sizeof(magic), std::ios::cur);
    //             if (file.read((char *)&checksum, sizeof(checksum)) &&
    //                 file.read((char *)&key, sizeof(key)) &&
    //                 file.read((char *)&vlen, sizeof(vlen)))
    //             {
    //                 char *buffer = new char[vlen];
    //                 if (file.read(buffer, vlen))
    //                 {
    //                     value.assign(buffer, vlen);
    //                     delete[] buffer;
    //                     std::vector<uint8_t> data(value.begin(), value.end());
    //                     if (checksum == utils::crc16(data))
    //                     {
    //                         return file.tellg() - sizeof(magic) - sizeof(checksum) - sizeof(key) - sizeof(vlen) - vlen;
    //                     }
    //                 }
    //                 else
    //                 {
    //                     delete[] buffer;
    //                 }
    //             }
    //         }
    //     }
    //     return 0;
}

uint64_t VLog::append(uint64_t key, const std::string &value)
{
    Entry entry(key, value);

    file.write((char *)&entry.magic, sizeof(entry.magic));
    file.write((char *)&entry.checksum, sizeof(entry.checksum));
    file.write((char *)&key, sizeof(key));
    file.write((char *)&entry.vlen, sizeof(entry.vlen));
    file.write(value.c_str(), entry.vlen);

    file.flush();

    head = file.tellp();

    // Return the offset of the value field
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

    // Move the write pointer back to the head
    file.seekp(head);

    // Convert the vector to a string and return it
    return std::string(data.begin(), data.end());
}