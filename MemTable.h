#ifndef UNTITLED_SKIPLIST_H
#define UNTITLED_SKIPLIST_H

#include <vector>
#include <map>
#include <list>
#include <cstdlib>
#include <limits>
#include "parameters.h"

template <typename K, typename V>
class Node
{
public:
    K key;
    V value;
    std::vector<Node<K, V> *> forward;
    Node(const K &key, const V &value, int level);
};

template <typename K, typename V>
Node<K, V>::Node(const K &key, const V &value, int level) : key(key), value(value), forward(level, nullptr) {}

template <typename K, typename V>
class MemTable
{
public:
    MemTable(int maxLevel);
    MemTable();
    ~MemTable();
    void put(const K &key, const V &value);
    void deleteKey(const K &key);
    V get(const K &key);
    void scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list);
    std::map<K, V> getAll();
    bool isExist(const K &key);
    void reset();
    int size() { return nodeCount; }
    int randomLevel();

private:
    Node<K, V> *header;
    int maxLevel;
    int nodeCount;
};

template <typename K, typename V>
MemTable<K, V>::MemTable(int maxLevel) : maxLevel(maxLevel), nodeCount(0)
{
    K k = K();  // 使用默认构造函数初始化K
    V v = V();  // 使用默认构造函数初始化V
    header = new Node<K, V>(k, v, maxLevel);
}

template <typename K, typename V>
MemTable<K, V>::MemTable() : maxLevel(MAXLEVEL), nodeCount(0)
{
    K k = K();  // 使用默认构造函数初始化K
    V v = V();  // 使用默认构造函数初始化V
    header = new Node<K, V>(k, v, maxLevel);
}

template <typename K, typename V>
MemTable<K, V>::~MemTable()
{
    Node<K, V> *current = header;
    while (current != nullptr)
    {
        Node<K, V> *next = current->forward[0];
        delete current;
        current = next;
    }
}

template <typename K, typename V>
int MemTable<K, V>::randomLevel()
{
    int level = 1;
    while ((rand() % 2) == 1 && level < maxLevel)
    {
        level++;
    }
    return level;
}

template <typename K, typename V>
void MemTable<K, V>::put(const K &key, const V &value)
{
    std::vector<Node<K, V> *> update(maxLevel);
    Node<K, V> *x = header;
    for (int i = maxLevel - 1; i >= 0; i--)
    {
        while (x->forward[i] != nullptr && x->forward[i]->key < key)
        {
            x = x->forward[i];
        }
        update[i] = x;
    }
    x = x->forward[0];
    if (x != nullptr && x->key == key)
    {
        x->value = value;  // 更新已有节点的值
    }
    else
    {
        int level = randomLevel();
        if (level > maxLevel)
        {
            for (int i = maxLevel; i < level; i++)
            {
                update.push_back(header);
            }
            maxLevel = level;
        }
        x = new Node<K, V>(key, value, level);
        for (int i = 0; i < level; i++)
        {
            x->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = x;
        }
        nodeCount++;
    }
}

template <typename K, typename V>
void MemTable<K, V>::deleteKey(const K &key)
{
    std::vector<Node<K, V> *> update(maxLevel);
    Node<K, V> *x = header;
    for (int i = maxLevel - 1; i >= 0; i--)
    {
        while (x->forward[i] != nullptr && x->forward[i]->key < key)
        {
            x = x->forward[i];
        }
        update[i] = x;
    }
    x = x->forward[0];
    if (x != nullptr && x->key == key)
    {
        for (int i = 0; i < maxLevel; i++)
        {
            if (update[i]->forward[i] != x)
            {
                break;
            }
            update[i]->forward[i] = x->forward[i];
        }
        delete x;
        nodeCount--;

        while (maxLevel > 1 && header->forward[maxLevel - 1] == nullptr)
        {
            maxLevel--;
        }
    }
}

template <typename K, typename V>
V MemTable<K, V>::get(const K &key)
{
    Node<K, V> *x = header;
    for (int i = maxLevel - 1; i >= 0; i--)
    {
        while (x->forward[i] != nullptr && x->forward[i]->key < key)
        {
            x = x->forward[i];
        }
    }
    x = x->forward[0];
    if (x != nullptr && x->key == key)
    {
        return x->value;
    }
    return V();  // 返回默认值
}

template <typename K, typename V>
void MemTable<K, V>::scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list)
{
    if (key1 > key2)
    {
        return;  // 无效范围
    }

    Node<K, V> *x = header;
    for (int i = maxLevel - 1; i >= 0; i--)
    {
        while (x->forward[i] != nullptr && x->forward[i]->key < key1)
        {
            x = x->forward[i];
        }
    }
    x = x->forward[0];
    while (x != nullptr && x->key <= key2)
    {
        list.push_back(std::make_pair(x->key, x->value));
        x = x->forward[0];
    }
}

template <typename K, typename V>
bool MemTable<K, V>::isExist(const K &key)
{
    Node<K, V> *x = header;
    for (int i = maxLevel - 1; i >= 0; i--)
    {
        while (x->forward[i] != nullptr && x->forward[i]->key < key)
        {
            x = x->forward[i];
        }
    }
    x = x->forward[0];
    return x != nullptr && x->key == key;
}

template <typename K, typename V>
void MemTable<K, V>::reset()
{
    Node<K, V> *current = header->forward[0];
    while (current != nullptr)
    {
        Node<K, V> *temp = current;
        current = current->forward[0];
        delete temp;
    }
    for (int i = 0; i < maxLevel; i++)
    {
        header->forward[i] = nullptr;
    }
    maxLevel = MAXLEVEL;  // 重置到初始最大层级
    nodeCount = 0;
}

template <typename K, typename V>
std::map<K, V> MemTable<K, V>::getAll()
{
    std::map<K, V> result;
    Node<K, V> *node = header->forward[0];
    while (node != nullptr)
    {
        result[node->key] = node->value;
        node = node->forward[0];
    }
    return result;
}

#endif // UNTITLED_SKIPLIST_H
