#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <cstddef>

/**
 * LinkedList<T> - A templated singly linked list implementation
 * 
 * Purpose: Store match history per player, used as collision chain in HashTable
 * Time Complexity:
 *   - prepend(): O(1)
 *   - append(): O(1) with tail pointer
 *   - remove(): O(n)
 *   - search(): O(n)
 *   - getLastN(): O(n)
 * 
 */

template <typename T>
class LinkedList {
private:
    struct Node {
        T data;
        Node* next;
        
        Node(const T& value) : data(value), next(nullptr) {}
    };
    
    Node* head;
    Node* tail;
    size_t listSize;

public:
    // Iterator for traversal
    class Iterator {
    private:
        Node* current;
    public:
        Iterator(Node* node) : current(node) {}
        
        T& operator*() { return current->data; }
        T* operator->() { return &current->data; }
        
        Iterator& operator++() {
            if (current) current = current->next;
            return *this;
        }
        
        bool operator!=(const Iterator& other) const {
            return current != other.current;
        }
        
        bool operator==(const Iterator& other) const {
            return current == other.current;
        }
    };

    // Constructor
    LinkedList() : head(nullptr), tail(nullptr), listSize(0) {}
    
    // Destructor - clean up all nodes
    ~LinkedList() {
        clear();
    }
    
    // Copy constructor
    LinkedList(const LinkedList& other) : head(nullptr), tail(nullptr), listSize(0) {
        Node* current = other.head;
        while (current) {
            append(current->data);
            current = current->next;
        }
    }
    
    // Copy assignment operator
    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            clear();
            Node* current = other.head;
            while (current) {
                append(current->data);
                current = current->next;
            }
        }
        return *this;
    }
    
    // Add element at the beginning - O(1)
    void prepend(const T& value) {
        Node* newNode = new Node(value);
        newNode->next = head;
        head = newNode;
        if (!tail) {
            tail = head;
        }
        listSize++;
    }
    
    // Add element at the end - O(1)
    void append(const T& value) {
        Node* newNode = new Node(value);
        if (!tail) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
        listSize++;
    }
    
    // Remove first occurrence of value - O(n)
    bool remove(const T& value) {
        if (!head) return false;
        
        // Special case: removing head
        if (head->data == value) {
            Node* toDelete = head;
            head = head->next;
            if (!head) {
                tail = nullptr;
            }
            delete toDelete;
            listSize--;
            return true;
        }
        
        // Search for the node
        Node* current = head;
        while (current->next && !(current->next->data == value)) {
            current = current->next;
        }
        
        if (current->next) {
            Node* toDelete = current->next;
            current->next = toDelete->next;
            if (toDelete == tail) {
                tail = current;
            }
            delete toDelete;
            listSize--;
            return true;
        }
        
        return false;
    }
    
    // Search for a value - O(n)
    T* find(const T& value) {
        Node* current = head;
        while (current) {
            if (current->data == value) {
                return &current->data;
            }
            current = current->next;
        }
        return nullptr;
    }
    
    // Get last N elements - O(n)
    // Returns a new LinkedList with at most n elements from the end
    LinkedList<T> getLastN(size_t n) const {
        LinkedList<T> result;
        if (!head || n == 0) return result;
        
        // Count total elements
        size_t total = listSize;
        size_t skip = (total > n) ? (total - n) : 0;
        
        // Skip first 'skip' elements
        Node* current = head;
        for (size_t i = 0; i < skip && current; i++) {
            current = current->next;
        }
        
        // Add remaining elements
        while (current) {
            result.append(current->data);
            current = current->next;
        }
        
        return result;
    }
    
    // Clear all elements - O(n)
    void clear() {
        while (head) {
            Node* toDelete = head;
            head = head->next;
            delete toDelete;
        }
        tail = nullptr;
        listSize = 0;
    }
    
    // Check if empty - O(1)
    bool isEmpty() const {
        return head == nullptr;
    }
    
    // Get size - O(1)
    size_t size() const {
        return listSize;
    }
    
    // Get first element - O(1)
    T* front() {
        return head ? &head->data : nullptr;
    }
    
    // Get last element - O(1)
    T* back() {
        return tail ? &tail->data : nullptr;
    }
    
    // Iterator methods
    Iterator begin() { return Iterator(head); }
    Iterator end() { return Iterator(nullptr); }
    
    // Const versions
    const T* front() const {
        return head ? &head->data : nullptr;
    }
    
    const T* back() const {
        return tail ? &tail->data : nullptr;
    }
};

#endif // LINKEDLIST_H
