# Visual Data Flows

This document visualizes how data structures interact during the core Matchmaking process.

## 1. Matchmaking Data Flow

This diagram shows the exact journey of a player from joining the lobby to finding a match.

```mermaid
graph TD
    %% Nodes
    User([User: SOHAIB])
    
    subgraph "Phase 1: Registration"
        HT[("HashTable<br>(ID -> Player Profile)")]
    end

    subgraph "Phase 2: Queueing (FIFO)"
        Q[("Queue<int><br>(Matchmaking Lobby)")]
    end

    subgraph "Phase 3: Matching (Logic)"
        MM{"Matchmaker<br>System"}
        AVL[("AVL Tree<br>(Rankings)")]
    end
    
    subgraph "Phase 4: Game"
        Game([Active Match])
    end

    %% Styles
    style HT fill:#e1f5fe,stroke:#01579b
    style Q fill:#fff3e0,stroke:#e65100
    style AVL fill:#e8f5e9,stroke:#1b5e20
    style MM fill:#f3e5f5,stroke:#4a148c

    %% Flows
    User -- "1. Register/Login" --> HT
    HT -- "2. Join Game" --> Q
    
    Q -- "3. Dequeue (First In)" --> MM
    
    MM -- "4. Look up Elo" --> HT
    MM -- "5. Remove Self" --> AVL
    note1[("Prevent Self-Match")] --- AVL
    
    MM -- "6. findClosest(Elo)" --> AVL
    AVL -- "7. Return Opponent ID" --> MM
    
    MM -- "8. Create Match" --> Game
```

## 2. Data Structure Interaction Details

### Storage (HashTable)
*   **Structure:** Array of Linked Lists (Chaining).
*   **Role:** The "Database" of the system.
*   **Operation:** `get(PlayerID)` is O(1).

### The Lobby (Queue)
*   **Structure:** Linked List with Head/Tail pointers.
*   **Role:** Waiting room.
*   **Operation:** `enqueue()` at Tail, `dequeue()` at Head. O(1).

### The Matcher (AVL Tree)
*   **Structure:** Balanced Binary Search Tree.
*   **Role:** The "Brain". Organizes players by Skill (ELO).
*   **Key Logic:** `findClosest(TargetElo)`
    *   Traverses tree (Left/Right) based on value.
    *   Keeps track of the node with the **smallest absolute difference** (`abs(NodeElo - TargetElo)`).
    *   **Time:** O(log n).

## 3. Bot Matching Logic

When a Human plays against a Bot, the flow is slightly different:

```mermaid
sequenceDiagram
    participant H as Human
    participant Q as Queue
    participant MM as Matchmaker
    participant AVL as AVL Tree
    participant Bot as Bot Pool

    H->>Q: Join Queue
    Q->>MM: Dequeue Human
    
    MM->>Q: Check Queue Size
    alt Queue Size == 0 (No other humans)
        MM->>AVL: findClosestBot(HumanElo)
        AVL->>MM: Return Best Bot ID
        MM->>Bot: Checks: Not Recently Played?
        Bot-->>MM: Valid Bot
        MM->>H: Create Match (Human vs Bot)
    else Queue Size > 0 (Human waiting)
        MM->>Q: Dequeue Opponent
        MM->>H: Create Match (Human vs Human)
    end
```
