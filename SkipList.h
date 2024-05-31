//
// Created by 86183 on 2024/4/9.
//

#ifndef UNTITLED_SKIPLIST_H
#define UNTITLED_SKIPLIST_H

#include <vector>
#include <map>

template<typename K, typename V>
class Node {
public:
    K key;
    V value;
    std::vector<Node<K, V>*> forward;

    Node(const K& key, const V& value, int level);
};

template<typename K, typename V>
class SkipList {
public:
    SkipList(int maxLevel);
    ~SkipList();
    void put(const K& key, const V& value);
    void deleteKey(const K& key);
    V get(const K& key);
    std::map<K, V> scan(const K& key1, const K& key2);

private:
    Node<K, V>* header;
    int maxLevel;
    int randomLevel();
};


#endif //UNTITLED_SKIPLIST_H
