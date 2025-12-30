#ifndef AVLTREE_H
#define AVLTREE_H

#include <cstddef>

/**
 * AVLTree<T> - A templated self-balancing AVL tree implementation
 * 
 * Purpose: Ranking system per game - enables O(log n) matchmaking
 * Key Features:
 *   - Self-balancing with LL, RR, LR, RL rotations
 *   - findClosest(target) - CRITICAL for rank-based matchmaking
 *   - In-order traversal for leaderboard generation
 * 
 * Time Complexity:
 *   - insert(): O(log n)
 *   - remove(): O(log n)
 *   - search(): O(log n)
 *   - findClosest(): O(log n)
 *   - inOrderTraversal(): O(n)
 * 
 * No STL dependencies - pure pointer-based implementation
 */

template <typename T>
class AVLTree {
private:
    struct Node {
        T data;
        Node* left;
        Node* right;
        int height;
        
        Node(const T& value) 
            : data(value), left(nullptr), right(nullptr), height(1) {}
    };
    
    Node* root;
    size_t nodeCount;
    
    // Get height of a node (nullptr safe)
    int getHeight(Node* node) const {
        return node ? node->height : 0;
    }
    
    // Get balance factor of a node
    int getBalance(Node* node) const {
        return node ? getHeight(node->left) - getHeight(node->right) : 0;
    }
    
    // Update height of a node based on children
    void updateHeight(Node* node) {
        if (node) {
            int leftHeight = getHeight(node->left);
            int rightHeight = getHeight(node->right);
            node->height = 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
        }
    }
    
    // Right rotation (LL case)
    Node* rotateRight(Node* y) {
        Node* x = y->left;
        Node* T2 = x->right;
        
        // Perform rotation
        x->right = y;
        y->left = T2;
        
        // Update heights
        updateHeight(y);
        updateHeight(x);
        
        return x;
    }
    
    // Left rotation (RR case)
    Node* rotateLeft(Node* x) {
        Node* y = x->right;
        Node* T2 = y->left;
        
        // Perform rotation
        y->left = x;
        x->right = T2;
        
        // Update heights
        updateHeight(x);
        updateHeight(y);
        
        return y;
    }
    
    // Balance a node after insertion/deletion
    Node* balance(Node* node) {
        if (!node) return nullptr;
        
        updateHeight(node);
        int balanceFactor = getBalance(node);
        
        // Left heavy
        if (balanceFactor > 1) {
            // Left-Right case
            if (getBalance(node->left) < 0) {
                node->left = rotateLeft(node->left);
            }
            // Left-Left case
            return rotateRight(node);
        }
        
        // Right heavy
        if (balanceFactor < -1) {
            // Right-Left case
            if (getBalance(node->right) > 0) {
                node->right = rotateRight(node->right);
            }
            // Right-Right case
            return rotateLeft(node);
        }
        
        return node;
    }
    
    // Find minimum node in subtree
    Node* findMin(Node* node) const {
        while (node && node->left) {
            node = node->left;
        }
        return node;
    }
    
    // Find maximum node in subtree
    Node* findMax(Node* node) const {
        while (node && node->right) {
            node = node->right;
        }
        return node;
    }
    
    // Recursive insert
    Node* insertNode(Node* node, const T& value) {
        if (!node) {
            nodeCount++;
            return new Node(value);
        }
        
        if (value < node->data) {
            node->left = insertNode(node->left, value);
        } else if (node->data < value) {
            node->right = insertNode(node->right, value);
        } else {
            // Duplicate value - don't insert
            return node;
        }
        
        return balance(node);
    }
    
    // Recursive remove
    Node* removeNode(Node* node, const T& value) {
        if (!node) return nullptr;
        
        if (value < node->data) {
            node->left = removeNode(node->left, value);
        } else if (node->data < value) {
            node->right = removeNode(node->right, value);
        } else {
            // Found node to delete
            if (!node->left || !node->right) {
                // One or no children
                Node* temp = node->left ? node->left : node->right;
                delete node;
                nodeCount--;
                return temp;
            } else {
                // Two children: get inorder successor
                Node* successor = findMin(node->right);
                node->data = successor->data;
                node->right = removeNode(node->right, successor->data);
            }
        }
        
        return balance(node);
    }
    
    // Recursive search
    Node* searchNode(Node* node, const T& value) const {
        if (!node) return nullptr;
        
        if (value < node->data) {
            return searchNode(node->left, value);
        } else if (node->data < value) {
            return searchNode(node->right, value);
        }
        return node;
    }
    
    // Recursive cleanup
    void destroyTree(Node* node) {
        if (node) {
            destroyTree(node->left);
            destroyTree(node->right);
            delete node;
        }
    }
    
    // Recursive copy
    Node* copyTree(Node* node) {
        if (!node) return nullptr;
        
        Node* newNode = new Node(node->data);
        newNode->height = node->height;
        newNode->left = copyTree(node->left);
        newNode->right = copyTree(node->right);
        return newNode;
    }
    
    // Find closest value - tracks predecessor and successor
    void findClosestHelper(Node* node, const T& target, T*& closest, int& minDiff) const {
        if (!node) return;
        
        // Calculate difference (assumes T has operator- for numeric types)
        int diff = node->data - target;
        if (diff < 0) diff = -diff;  // Absolute value
        
        if (closest == nullptr || diff < minDiff) {
            minDiff = diff;
            closest = const_cast<T*>(&node->data);
        } else if (diff == minDiff) {
            // Tie-breaker: prefer lower value (optional behavior)
            if (node->data < *closest) {
                closest = const_cast<T*>(&node->data);
            }
        }
        
        // Navigate tree based on target
        if (target < node->data) {
            findClosestHelper(node->left, target, closest, minDiff);
        } else if (node->data < target) {
            findClosestHelper(node->right, target, closest, minDiff);
        }
        // If equal, we already have the closest
    }

public:
    // Constructor
    AVLTree() : root(nullptr), nodeCount(0) {}
    
    // Destructor
    ~AVLTree() {
        destroyTree(root);
    }
    
    // Copy constructor
    AVLTree(const AVLTree& other) : root(nullptr), nodeCount(0) {
        root = copyTree(other.root);
        nodeCount = other.nodeCount;
    }
    
    // Copy assignment operator
    AVLTree& operator=(const AVLTree& other) {
        if (this != &other) {
            destroyTree(root);
            root = copyTree(other.root);
            nodeCount = other.nodeCount;
        }
        return *this;
    }
    
    // Insert a value - O(log n)
    void insert(const T& value) {
        root = insertNode(root, value);
    }
    
    // Remove a value - O(log n)
    bool remove(const T& value) {
        size_t oldCount = nodeCount;
        root = removeNode(root, value);
        return nodeCount < oldCount;
    }
    
    // Search for a value - O(log n)
    T* search(const T& value) {
        Node* node = searchNode(root, value);
        return node ? &node->data : nullptr;
    }
    
    const T* search(const T& value) const {
        Node* node = searchNode(root, value);
        return node ? &node->data : nullptr;
    }
    
    // Check if tree contains value - O(log n)
    bool contains(const T& value) const {
        return searchNode(root, value) != nullptr;
    }
    
    /**
     * findClosest - CRITICAL for matchmaking
     * 
     * Finds the value in the tree closest to the target.
     * Uses absolute difference comparison.
     * 
     * @param target The value to find the closest match for
     * @return Pointer to closest value, or nullptr if tree is empty
     * 
     * Time Complexity: O(log n)
     */
    T* findClosest(const T& target) {
        if (!root) return nullptr;
        
        T* closest = nullptr;
        int minDiff = 0;
        findClosestHelper(root, target, closest, minDiff);
        return closest;
    }
    
    const T* findClosest(const T& target) const {
        if (!root) return nullptr;
        
        T* closest = nullptr;
        int minDiff = 0;
        findClosestHelper(root, target, closest, minDiff);
        return closest;
    }
    
    /**
     * findClosestExcluding - For matchmaking (avoid self-matching)
     * 
     * Finds the closest value that is NOT equal to excluded value.
     */
    T* findClosestExcluding(const T& target, const T& excluded) {
        if (!root) return nullptr;
        
        T* best = nullptr;
        int bestDiff = -1;
        
        findClosestExcludingHelper(root, target, excluded, best, bestDiff);
        return best;
    }
    
private:
    void findClosestExcludingHelper(Node* node, const T& target, const T& excluded, 
                                     T*& best, int& bestDiff) const {
        if (!node) return;
        
        // Skip if this is the excluded value
        bool isExcluded = !(node->data < excluded) && !(excluded < node->data);
        
        if (!isExcluded) {
            int diff = node->data - target;
            if (diff < 0) diff = -diff;
            
            if (bestDiff < 0 || diff < bestDiff) {
                bestDiff = diff;
                best = const_cast<T*>(&node->data);
            }
        }
        
        // Continue searching both subtrees for potentially closer matches
        if (target < node->data) {
            findClosestExcludingHelper(node->left, target, excluded, best, bestDiff);
            // Also check right subtree if needed
            if (bestDiff < 0 || node->right) {
                findClosestExcludingHelper(node->right, target, excluded, best, bestDiff);
            }
        } else {
            findClosestExcludingHelper(node->right, target, excluded, best, bestDiff);
            // Also check left subtree if needed
            if (bestDiff < 0 || node->left) {
                findClosestExcludingHelper(node->left, target, excluded, best, bestDiff);
            }
        }
    }
    
public:
    /**
     * inOrderTraversal - For leaderboard generation
     * 
     * Traverses tree in-order (ascending) and calls callback for each element.
     * 
     * @param callback Function to call for each element
     * @param userData Optional user data to pass to callback
     * 
     * Time Complexity: O(n)
     */
    template <typename Callback>
    void inOrderTraversal(Callback callback) const {
        inOrderHelper(root, callback);
    }
    
private:
    template <typename Callback>
    void inOrderHelper(Node* node, Callback callback) const {
        if (!node) return;
        inOrderHelper(node->left, callback);
        callback(node->data);
        inOrderHelper(node->right, callback);
    }
    
public:
    // Reverse in-order traversal (descending) for leaderboard
    template <typename Callback>
    void reverseInOrderTraversal(Callback callback) const {
        reverseInOrderHelper(root, callback);
    }
    
private:
    template <typename Callback>
    void reverseInOrderHelper(Node* node, Callback callback) const {
        if (!node) return;
        reverseInOrderHelper(node->right, callback);
        callback(node->data);
        reverseInOrderHelper(node->left, callback);
    }
    
public:
    // Get size - O(1)
    size_t size() const {
        return nodeCount;
    }
    
    // Check if empty - O(1)
    bool isEmpty() const {
        return root == nullptr;
    }
    
    // Clear tree - O(n)
    void clear() {
        destroyTree(root);
        root = nullptr;
        nodeCount = 0;
    }
    
    // Get minimum value - O(log n)
    T* getMin() {
        Node* minNode = findMin(root);
        return minNode ? &minNode->data : nullptr;
    }
    
    // Get maximum value - O(log n)
    T* getMax() {
        Node* maxNode = findMax(root);
        return maxNode ? &maxNode->data : nullptr;
    }
    
    // Get tree height - O(1)
    int height() const {
        return getHeight(root);
    }
};

#endif // AVLTREE_H
