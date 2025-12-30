#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "LinkedList.h"
#include <cstddef>
#include <cstring>

/**
 * HashTable<K, V> - A templated hash table with separate chaining
 * 
 * Purpose: Store Player & Bot profiles for O(1) average lookup
 * Key Types: int (PlayerID) or string (hashed manually)
 * Collision Handling: Custom LinkedList (separate chaining)
 * 
 * Time Complexity:
 *   - insert(): O(1) average, O(n) worst
 *   - get(): O(1) average, O(n) worst
 *   - update(): O(1) average, O(n) worst
 *   - remove(): O(1) average, O(n) worst
 *   - contains(): O(1) average, O(n) worst
 * 
 */

// Hash function specializations
template <typename K>
struct HashFunc {
    size_t operator()(const K& key, size_t tableSize) const;
};

// Hash function for int keys
template <>
struct HashFunc<int> {
    size_t operator()(const int& key, size_t tableSize) const {
        // Simple modulo hash for integers
        size_t hash = static_cast<size_t>(key >= 0 ? key : -key);
        return hash % tableSize;
    }
};

// Hash function for C-string keys (char*)
template <>
struct HashFunc<const char*> {
    size_t operator()(const char* const& key, size_t tableSize) const {
        // djb2 hash algorithm
        size_t hash = 5381;
        int c;
        const char* str = key;
        while ((c = *str++)) {
            hash = ((hash << 5) + hash) + c; // hash * 33 + c
        }
        return hash % tableSize;
    }
};

// Key-Value pair structure
template <typename K, typename V>
struct KeyValuePair {
    K key;
    V value;
    
    KeyValuePair() {}
    KeyValuePair(const K& k, const V& v) : key(k), value(v) {}
    
    bool operator==(const KeyValuePair& other) const {
        return key == other.key;
    }
};

// Specialization for const char* key comparison
template <typename V>
struct KeyValuePair<const char*, V> {
    const char* key;
    V value;
    
    KeyValuePair() : key(nullptr) {}
    KeyValuePair(const char* k, const V& v) : key(k), value(v) {}
    
    bool operator==(const KeyValuePair& other) const {
        if (key == nullptr && other.key == nullptr) return true;
        if (key == nullptr || other.key == nullptr) return false;
        return strcmp(key, other.key) == 0;
    }
};

template <typename K, typename V, typename Hash = HashFunc<K>>
class HashTable {
private:
    static const size_t DEFAULT_SIZE = 101;  // Prime number for better distribution
    static constexpr float LOAD_FACTOR_THRESHOLD = 0.75f;
    
    LinkedList<KeyValuePair<K, V>>* buckets;
    size_t tableSize;
    size_t elementCount;
    Hash hashFunc;
    
    // Compute hash index for a key
    size_t getIndex(const K& key) const {
        return hashFunc(key, tableSize);
    }
    
    // Resize and rehash when load factor exceeded
    void rehash() {
        size_t oldSize = tableSize;
        LinkedList<KeyValuePair<K, V>>* oldBuckets = buckets;
        
        // Double the size, find next prime
        tableSize = oldSize * 2 + 1;
        buckets = new LinkedList<KeyValuePair<K, V>>[tableSize];
        elementCount = 0;
        
        // Reinsert all elements
        for (size_t i = 0; i < oldSize; i++) {
            for (auto it = oldBuckets[i].begin(); it != oldBuckets[i].end(); ++it) {
                insert((*it).key, (*it).value);
            }
        }
        
        delete[] oldBuckets;
    }

public:
    // Constructor
    HashTable(size_t size = DEFAULT_SIZE) 
        : tableSize(size), elementCount(0) {
        buckets = new LinkedList<KeyValuePair<K, V>>[tableSize];
    }
    
    // Destructor
    ~HashTable() {
        delete[] buckets;
    }
    
    // Copy constructor
    HashTable(const HashTable& other) 
        : tableSize(other.tableSize), elementCount(0) {
        buckets = new LinkedList<KeyValuePair<K, V>>[tableSize];
        for (size_t i = 0; i < tableSize; i++) {
            buckets[i] = other.buckets[i];
        }
        elementCount = other.elementCount;
    }
    
    // Copy assignment operator
    HashTable& operator=(const HashTable& other) {
        if (this != &other) {
            delete[] buckets;
            tableSize = other.tableSize;
            buckets = new LinkedList<KeyValuePair<K, V>>[tableSize];
            for (size_t i = 0; i < tableSize; i++) {
                buckets[i] = other.buckets[i];
            }
            elementCount = other.elementCount;
        }
        return *this;
    }
    
    // Insert a key-value pair - O(1) average
    void insert(const K& key, const V& value) {
        // Check if key already exists
        V* existing = get(key);
        if (existing) {
            *existing = value;  // Update existing
            return;
        }
        
        // Check load factor
        float loadFactor = static_cast<float>(elementCount + 1) / tableSize;
        if (loadFactor > LOAD_FACTOR_THRESHOLD) {
            rehash();
        }
        
        // Insert new pair
        size_t index = getIndex(key);
        buckets[index].append(KeyValuePair<K, V>(key, value));
        elementCount++;
    }
    
    // Get value by key - O(1) average
    // Returns pointer to value or nullptr if not found
    V* get(const K& key) {
        size_t index = getIndex(key);
        KeyValuePair<K, V> searchPair(key, V());
        
        for (auto it = buckets[index].begin(); it != buckets[index].end(); ++it) {
            if ((*it) == searchPair) {
                return &((*it).value);
            }
        }
        return nullptr;
    }
    
    const V* get(const K& key) const {
        size_t index = getIndex(key);
        KeyValuePair<K, V> searchPair(key, V());
        
        // Need non-const access for iteration, create workaround
        LinkedList<KeyValuePair<K, V>>& bucket = 
            const_cast<LinkedList<KeyValuePair<K, V>>&>(buckets[index]);
        
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if ((*it) == searchPair) {
                return &((*it).value);
            }
        }
        return nullptr;
    }
    
    // Update value for existing key - O(1) average
    bool update(const K& key, const V& newValue) {
        V* existing = get(key);
        if (existing) {
            *existing = newValue;
            return true;
        }
        return false;
    }
    
    // Remove a key-value pair - O(1) average
    bool remove(const K& key) {
        size_t index = getIndex(key);
        KeyValuePair<K, V> searchPair(key, V());
        
        if (buckets[index].remove(searchPair)) {
            elementCount--;
            return true;
        }
        return false;
    }
    
    // Check if key exists - O(1) average
    bool contains(const K& key) const {
        return get(key) != nullptr;
    }
    
    // Get number of elements
    size_t size() const {
        return elementCount;
    }
    
    // Check if empty
    bool isEmpty() const {
        return elementCount == 0;
    }
    
    // Clear all elements
    void clear() {
        for (size_t i = 0; i < tableSize; i++) {
            buckets[i].clear();
        }
        elementCount = 0;
    }
    
    // Get all keys (useful for iteration)
    void getAllKeys(K* outKeys, size_t& outCount) const {
        outCount = 0;
        for (size_t i = 0; i < tableSize; i++) {
            LinkedList<KeyValuePair<K, V>>& bucket = 
                const_cast<LinkedList<KeyValuePair<K, V>>&>(buckets[i]);
            for (auto it = bucket.begin(); it != bucket.end(); ++it) {
                outKeys[outCount++] = (*it).key;
            }
        }
    }
};

#endif // HASHTABLE_H
