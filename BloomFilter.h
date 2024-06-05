#ifndef CODEPROJECTTEST_BLOOMFILTER_H
#define CODEPROJECTTEST_BLOOMFILTER_H
#include "MurmurHash3.h"
#include <vector>
#include <cstring>

class BloomFilter
{
private:
    std::vector<bool> bitArray;
    static constexpr size_t bitArraySize = 8192 * 8;

public:
    BloomFilter() : bitArray(bitArraySize, false) {}

    void insert(const void *key, int len)
    {
        uint32_t hash[4];
        MurmurHash3_x64_128(key, len, 1, hash);
        for (int i = 0; i < 4; i++)
        {
            size_t pos = hash[i] % bitArraySize;
            bitArray[pos] = true;
        }
    }

    bool query(const void *key, int len)
    {
        uint32_t hash[4];
        MurmurHash3_x64_128(key, len, 1, hash);
        for (int i = 0; i < 4; i++)
        {
            size_t pos = hash[i] % bitArraySize;
            if (!bitArray[pos])
            {
                return false;
            }
        }
        return true;
    }

    void insert(uint64_t key)
    {
        insert(&key, sizeof(key));
    }

    bool query(uint64_t key)
    {
        return query(&key, sizeof(key));
    }

    void serialize(std::ofstream &file) const
    {
        for (size_t i = 0; i < bitArraySize; i += 8)
        {
            uint8_t byte = 0;
            for (size_t j = 0; j < 8 && (i + j) < bitArraySize; ++j)
            {
                if (bitArray[i + j])
                {
                    byte |= (1 << j);
                }
            }
            file.write(reinterpret_cast<const char *>(&byte), sizeof(byte));
        }
    }

    void deserialize(std::ifstream &file)
    {
        for (size_t i = 0; i < bitArraySize; i += 8)
        {
            uint8_t byte = 0;
            file.read(reinterpret_cast<char *>(&byte), sizeof(byte));
            for (size_t j = 0; j < 8 && (i + j) < bitArraySize; ++j)
            {
                bitArray[i + j] = (byte & (1 << j)) != 0;
            }
        }
    }

};
#endif // CODEPROJECTTEST_BLOOMFILTER_H
