#ifndef CODEPROJECTTEST_BLOOMFILTER_H
#define CODEPROJECTTEST_BLOOMFILTER_H
#include "MurmurHash3.h"
#include <vector>
#include <cstring>

class BloomFilter {
private:
    std::vector<bool> bitArray;
    static constexpr size_t bitArraySize = 8192 * 8;

public:
    BloomFilter() : bitArray(bitArraySize, false) {}

    void insert(const void* key, int len) {
        uint32_t hash[4];
        MurmurHash3_x64_128(key, len, 1, hash);
        for (int i = 0; i < 4; i++) {
            size_t pos = hash[i] % bitArraySize;
            bitArray[pos] = true;
        }
    }

    bool query(const void* key, int len) {
        uint32_t hash[4];
        MurmurHash3_x64_128(key, len, 1, hash);
        for (int i = 0; i < 4; i++) {
            size_t pos = hash[i] % bitArraySize;
            if (!bitArray[pos]) {
                return false;
            }
        }
        return true;
    }

    void insert(uint64_t key) {
        insert(&key, sizeof(key));
    }

    bool query(uint64_t key) {
        return query(&key, sizeof(key));
    }
};
#endif //CODEPROJECTTEST_BLOOMFILTER_H
