#ifndef UNTITLED_SKIPLIST_H
#define UNTITLED_SKIPLIST_H
#include <vector>
#include <map>
#include <limits>
#include <cstdlib>

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

template<typename K, typename V>
Node<K, V>::Node(const K& key, const V& value, int level) : key(key), value(value), forward(level, nullptr) {}

template<typename K, typename V>
SkipList<K, V>::SkipList(int maxLevel) : maxLevel(maxLevel) {
    K k;
    V v;
    header = new Node<K, V>(k, v, maxLevel);
}

template<typename K, typename V>
SkipList<K, V>::~SkipList() {
    delete header;
}

template<typename K, typename V>
int SkipList<K, V>::randomLevel() {
    int level = 1;
    while ((rand() % 2) == 1 && level < maxLevel) {
        level++;
    }
    return level;
}

template<typename K, typename V>
void SkipList<K, V>::put(const K& key, const V& value) {
    std::vector<Node<K, V>*> update(maxLevel);
    Node<K, V>* x = header;
    for (int i = maxLevel - 1; i >= 0; i--) {
        while (x->forward[i] != nullptr && x->forward[i]->key < key) {
            x = x->forward[i];
        }
        update[i] = x;
    }
    int level = randomLevel();
    if (level > maxLevel) {
        for (int i = maxLevel; i < level; i++) {
            update[i] = header;
        }
        maxLevel = level;
    }
    x = new Node<K, V>(key, value, level);
    for (int i = 0; i < level; i++) {
        x->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = x;
    }
}

template<typename K, typename V>
void SkipList<K, V>::deleteKey(const K& key) {
    std::vector<Node<K, V>*> update(maxLevel);
    Node<K, V>* x = header;
    for (int i = maxLevel - 1; i >= 0; i--) {
        while (x->forward[i] != nullptr && x->forward[i]->key < key) {
            x = x->forward[i];
        }
        update[i] = x;
    }
    x = x->forward[0];
    if (x->key == key) {
        for (int i = 0; i < maxLevel; i++) {
            if (update[i]->forward[i] != x) {
                break;
            }
            update[i]->forward[i] = x->forward[i];
        }
        delete x;
        while (maxLevel > 1 && header->forward[maxLevel] == nullptr) {
            maxLevel--;
        }
    }
}

template<typename K, typename V>
V SkipList<K, V>::get(const K& key) {
    Node<K, V>* x = header;
    for (int i = maxLevel - 1; i >= 0; i--) {
        while (x->forward[i] != nullptr && x->forward[i]->key < key) {
            x = x->forward[i];
        }
    }
    x = x->forward[0];
    if (x->key == key) {
        return x->value;
    }
    return V();
}

template<typename K, typename V>
std::map<K, V> SkipList<K, V>::scan(const K& key1, const K& key2) {
    std::map<K, V> result;
    Node<K, V>* x = header;
    for (int i = maxLevel - 1; i >= 0; i--) {
        while (x->forward[i] != nullptr && x->forward[i]->key < key1) {
            x = x->forward[i];
        }
    }
    x = x->forward[0];
    while (x != nullptr && x->key <= key2) {
        result[x->key] = x->value;
        x = x->forward[0];
    }
    return result;
}

#endif //UNTITLED_SKIPLIST_H
