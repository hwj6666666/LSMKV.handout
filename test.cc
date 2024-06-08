#include "kvstore.cc"
#include "SSTable.h"
#include <chrono>

#define MB 1024 * 1024
#define KB 1024

int main()
{
    KVStore store("./data", "./data/vlog");
    store.reset();
    for (int i = 0; i < 1000; i++)
        store.put(i, string(1 * KB, 'a'));

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++)
        store.get(i);

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_seconds = end - start;

    cout << "Total time: " << elapsed_seconds.count() << endl;
    store.reset();
    // for (int i = 0; i < 1000; i++)
    //     store.put(i, string(2 * KB, 'a'));

    // auto start = std::chrono::high_resolution_clock::now();
    // for (int i = 0; i < 1000; i++)
    //     store.del(i);

    // auto end = std::chrono::high_resolution_clock::now();

    // std::chrono::duration<double> elapsed_seconds = end - start;

    // cout << "Total time: " << elapsed_seconds.count() << endl;
    // store.reset();
    // for (int i = 0; i < 1000; i++)
    //     store.put(i, string(4 * KB, 'a'));

    // auto start = std::chrono::high_resolution_clock::now();
    // for (int i = 0; i < 1000; i++)
    //     store.del(i);

    // auto end = std::chrono::high_resolution_clock::now();

    // std::chrono::duration<double> elapsed_seconds = end - start;

    // cout << "Total time: " << elapsed_seconds.count() << endl;
    // store.reset();
    // for (int i = 0; i < 1000; i++)
    //     store.put(i, string(8 * KB, 'a'));

    // auto start = std::chrono::high_resolution_clock::now();
    // for (int i = 0; i < 1000; i++)
    //     store.del(i);

    // auto end = std::chrono::high_resolution_clock::now();

    // std::chrono::duration<double> elapsed_seconds = end - start;

    // cout << "Total time: " << elapsed_seconds.count() << endl;
    // store.reset();
    // for (int i = 0; i < 1000; i++)
    //     store.put(i, string(16 * KB, 'a'));

    // auto start = std::chrono::high_resolution_clock::now();
    // for (int i = 0; i < 1000; i++)
    //     store.del(i);

    // auto end = std::chrono::high_resolution_clock::now();

    // std::chrono::duration<double> elapsed_seconds = end - start;

    // cout << "Total time: " << elapsed_seconds.count() << endl;
    store.reset();
    cout << "hello, world";
}