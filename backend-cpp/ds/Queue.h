#ifndef QUEUE_H
#define QUEUE_H

#include <cstddef>

/**
 * Queue<T> - A templated FIFO queue implementation using linked nodes
 * 
 * Purpose: Matchmaking lobby queue - one per game
 * Behavior: First-In-First-Out (FIFO)
 * Time Complexity:
 *   - enqueue(): O(1)
 *   - dequeue(): O(1)
 *   - front(): O(1)
 *   - isEmpty(): O(1)
 *   - size(): O(1)
 * 
 * No STL dependencies - pure pointer-based implementation
 */

template <typename T>
class Queue {
private:
    struct Node {
        T data;
        Node* next;
        
        Node(const T& value) : data(value), next(nullptr) {}
    };
    
    Node* frontNode;
    Node* rearNode;
    size_t queueSize;

public:
    // Constructor
    Queue() : frontNode(nullptr), rearNode(nullptr), queueSize(0) {}
    
    // Destructor - clean up all nodes
    ~Queue() {
        clear();
    }
    
    // Copy constructor
    Queue(const Queue& other) : frontNode(nullptr), rearNode(nullptr), queueSize(0) {
        Node* current = other.frontNode;
        while (current) {
            enqueue(current->data);
            current = current->next;
        }
    }
    
    // Copy assignment operator
    Queue& operator=(const Queue& other) {
        if (this != &other) {
            clear();
            Node* current = other.frontNode;
            while (current) {
                enqueue(current->data);
                current = current->next;
            }
        }
        return *this;
    }
    
    // Add element to rear - O(1)
    void enqueue(const T& value) {
        Node* newNode = new Node(value);
        
        if (isEmpty()) {
            frontNode = rearNode = newNode;
        } else {
            rearNode->next = newNode;
            rearNode = newNode;
        }
        queueSize++;
    }
    
    // Remove and return front element - O(1)
    // Returns true if successful, false if queue is empty
    bool dequeue(T& outValue) {
        if (isEmpty()) {
            return false;
        }
        
        Node* toDelete = frontNode;
        outValue = frontNode->data;
        frontNode = frontNode->next;
        
        if (!frontNode) {
            rearNode = nullptr;
        }
        
        delete toDelete;
        queueSize--;
        return true;
    }
    
    // Peek at front element without removing - O(1)
    T* front() {
        return frontNode ? &frontNode->data : nullptr;
    }
    
    const T* front() const {
        return frontNode ? &frontNode->data : nullptr;
    }
    
    // Peek at rear element - O(1)
    T* rear() {
        return rearNode ? &rearNode->data : nullptr;
    }
    
    const T* rear() const {
        return rearNode ? &rearNode->data : nullptr;
    }
    
    // Check if queue is empty - O(1)
    bool isEmpty() const {
        return frontNode == nullptr;
    }
    
    // Get queue size - O(1)
    size_t size() const {
        return queueSize;
    }
    
    // Clear all elements - O(n)
    void clear() {
        while (frontNode) {
            Node* toDelete = frontNode;
            frontNode = frontNode->next;
            delete toDelete;
        }
        rearNode = nullptr;
        queueSize = 0;
    }
    
    // Check if an element exists in the queue - O(n)
    bool contains(const T& value) const {
        Node* current = frontNode;
        while (current) {
            if (current->data == value) {
                return true;
            }
            current = current->next;
        }
        return false;
    }
    
    // Remove a specific element from queue (useful for canceling matchmaking) - O(n)
    bool remove(const T& value) {
        if (isEmpty()) return false;
        
        // Special case: removing front
        if (frontNode->data == value) {
            Node* toDelete = frontNode;
            frontNode = frontNode->next;
            if (!frontNode) {
                rearNode = nullptr;
            }
            delete toDelete;
            queueSize--;
            return true;
        }
        
        // Search for the node
        Node* current = frontNode;
        while (current->next && !(current->next->data == value)) {
            current = current->next;
        }
        
        if (current->next) {
            Node* toDelete = current->next;
            current->next = toDelete->next;
            if (toDelete == rearNode) {
                rearNode = current;
            }
            delete toDelete;
            queueSize--;
            return true;
        }
        
        return false;
    }
};

#endif // QUEUE_H
