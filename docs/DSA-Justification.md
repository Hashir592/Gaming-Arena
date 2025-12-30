# DSA Justification - Multiplayer Game System

## Overview
This document explains why each data structure was chosen for the Multiplayer Game System and how they fulfill academic DSA requirements.

---

## Data Structures Used

### 1. Hash Table (`HashTable<K, V>`)

**Purpose:** Store and retrieve Player profiles by ID.

**Implementation:**
- Template-based with separate chaining collision resolution
- Uses custom `LinkedList<T>` for collision chains
- Manual hash functions for `int` and `string` keys

**Why HashTable?**
- Player profile lookup must be fast (O(1) average)
- Profiles are accessed by unique PlayerID
- Frequent operations: registration, stat updates, matchmaking lookup

**Time Complexity:**
| Operation | Average | Worst |
|-----------|---------|-------|
| Insert    | O(1)    | O(n)  |
| Search    | O(1)    | O(n)  |
| Delete    | O(1)    | O(n)  |

**Alternative Considered:** AVL Tree (O(log n) for all ops) - rejected because we don't need ordered access to player profiles.

---

### 2. AVL Tree (`AVLTree<T>`)

**Purpose:** Rank-based matchmaking and leaderboard generation.

**Implementation:**
- Self-balancing binary search tree with height tracking
- Supports LL, RR, LR, RL rotations
- `findClosest(target)` for nearest-neighbor search
- In-order traversal for sorted leaderboard output

**Why AVL Tree?**
- Matchmaking requires finding the **closest ELO** to a target
- Leaderboard needs **sorted** player rankings
- Guaranteed O(log n) for all operations

**Critical Function: `findClosest()`**
```cpp
T* findClosest(const T& target) {
    // Traverse tree, tracking minimum absolute difference
    // Returns pointer to closest value
}
```

This is the heart of the matchmaking algorithm - it finds the opponent with the nearest ELO in logarithmic time.

**Time Complexity:**
| Operation      | Complexity |
|----------------|------------|
| Insert         | O(log n)   |
| Delete         | O(log n)   |
| Find Closest   | O(log n)   |
| Leaderboard    | O(n)       |

**Alternative Considered:** Sorted array with binary search - rejected because insertions/deletions are O(n).

---

### 3. Queue (`Queue<T>`)

**Purpose:** Matchmaking lobby - FIFO ordering ensures fair wait times.

**Implementation:**
- Linked-list based with front and rear pointers
- One queue per game type

**Why Queue?**
- Players should be matched in the order they joined
- FIFO guarantees fairness
- Constant time enqueue/dequeue

**Time Complexity:**
| Operation | Complexity |
|-----------|------------|
| Enqueue   | O(1)       |
| Dequeue   | O(1)       |
| isEmpty   | O(1)       |

**Alternative Considered:** Priority Queue by wait time - rejected because all players in queue have equal priority (FIFO is simpler and fair).

---

### 4. Linked List (`LinkedList<T>`)

**Purpose:** Match history per player, collision chains in HashTable.

**Implementation:**
- Singly-linked with head and tail pointers
- Iterator support for traversal
- `getLastN(n)` for recent match retrieval

**Why LinkedList?**
- Match history grows dynamically
- Only need recent matches (last N)
- Chronological append is O(1)

**Time Complexity:**
| Operation   | Complexity |
|-------------|------------|
| Append      | O(1)       |
| Get Last N  | O(n)       |
| Traverse    | O(n)       |

---

## Matchmaking Algorithm

**Flow:**
1. Player joins queue → `Queue.enqueue(playerID)`
2. Matchmaker dequeues player → `Queue.dequeue()`
3. Fetch player ELO → `HashTable.get(playerID)`
4. **Remove player from AVL** (prevent self-matching)
5. Find closest opponent → `AVLTree.findClosest(elo)`
6. Create match, remove opponent from queue
7. After match: update ELOs, reinsert into AVL

**Why This Approach?**
- O(1) queue operations for fairness
- O(log n) AVL search for skill-based matching
- No O(n) scans or heap-based global ranking

---

## Complexity Summary

| Component      | Operation          | Complexity |
|----------------|--------------------|------------|
| Player Lookup  | HashTable.get      | O(1) avg   |
| Join Queue     | Queue.enqueue      | O(1)       |
| Find Match     | AVL.findClosest    | O(log n)   |
| Update Rank    | AVL.remove+insert  | O(log n)   |
| Leaderboard    | AVL.inOrderTraverse| O(n)       |
| Match History  | LinkedList.append  | O(1)       |

---

## Academic Requirements Fulfilled

✅ All data structures are **hand-written** (no STL)  
✅ All structures use **C++ templates**  
✅ **Pointer-based** node implementations  
✅ Clear **time complexity** documentation  
✅ **Practical justification** for each structure  
✅ Separation of concerns (DSA logic ≠ UI logic)
