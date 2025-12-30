# System Architecture - Multiplayer Game System

## Overview

The Multiplayer Game System is a university DSA project demonstrating practical application of data structures in a game matchmaking context.

```
┌─────────────────────────────────────────────────────────────────┐
│                    FRONTEND (Browser)                            │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐             │
│  │  Ping Pong   │ │ Snake Battle │ │ Tank Battle  │  (Canvas)   │
│  └──────────────┘ └──────────────┘ └──────────────┘             │
│  ┌─────────────────────────────────────────────────┐            │
│  │           UI Layer (HTML/CSS/JS)                 │            │
│  │  - Game Selection   - Dashboard   - Leaderboard │            │
│  └─────────────────────────────────────────────────┘            │
│                          │ REST API                              │
└──────────────────────────┼──────────────────────────────────────┘
                           │
┌──────────────────────────┼──────────────────────────────────────┐
│                 C++ BACKEND (DSA Engine)                         │
│                          │                                       │
│  ┌───────────────────────┼───────────────────────────┐          │
│  │              API Layer (cpp-httplib)              │          │
│  │  - Player endpoints  - Matchmaking  - Leaderboard │          │
│  └───────────────────────┼───────────────────────────┘          │
│                          │                                       │
│  ┌───────────────────────┼───────────────────────────┐          │
│  │                   Services                         │          │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────────┐ │          │
│  │  │ Matchmaker │ │  Ranking   │ │    History     │ │          │
│  │  │            │ │  Service   │ │    Service     │ │          │
│  │  └────────────┘ └────────────┘ └────────────────┘ │          │
│  └───────────────────────┼───────────────────────────┘          │
│                          │                                       │
│  ┌───────────────────────┼───────────────────────────┐          │
│  │              Data Structures (Pure DSA)           │          │
│  │  ┌───────────┐ ┌───────────┐ ┌───────┐ ┌───────┐ │          │
│  │  │ HashTable │ │  AVLTree  │ │ Queue │ │ List  │ │          │
│  │  └───────────┘ └───────────┘ └───────┘ └───────┘ │          │
│  └───────────────────────────────────────────────────┘          │
└─────────────────────────────────────────────────────────────────┘
```

---

## Component Details

### Data Structures Layer (`/backend-cpp/ds/`)

| File | Class | Purpose |
|------|-------|---------|
| `HashTable.h` | `HashTable<K,V>` | Player storage, O(1) lookup |
| `AVLTree.h` | `AVLTree<T>` | Rankings, O(log n) matchmaking |
| `Queue.h` | `Queue<T>` | Matchmaking lobby (FIFO) |
| `LinkedList.h` | `LinkedList<T>` | Match history, collision chains |

### Models Layer (`/backend-cpp/models/`)

| File | Struct | Purpose |
|------|--------|---------|
| `Player.h` | `Player` | Player profile data |
| `Player.h` | `PlayerELO` | AVL tree node (ELO + ID) |
| `Match.h` | `Match` | Match record |

### Services Layer (`/backend-cpp/services/`)

| File | Class | Purpose |
|------|-------|---------|
| `RankingService.h` | `RankingService` | ELO updates, leaderboards |
| `HistoryService.h` | `HistoryService` | Match history storage |
| `Matchmaker.h` | `Matchmaker` | Queue + AVL matchmaking |

### API Layer (`/backend-cpp/main.cpp`)

Uses **cpp-httplib** for HTTP only. All DSA logic in services.

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/players` | POST | Register player |
| `/api/players/:id` | GET | Get profile |
| `/api/matchmaking/join` | POST | Join queue |
| `/api/matches/result` | POST | Submit result |
| `/api/leaderboard/:game` | GET | Get rankings |

---

## Data Flow

### Registration
```
User Input → API → HashTable.insert(player) → Return ID
```

### Matchmaking
```
1. Player joins     → Queue.enqueue(id)
2. Find opponent    → AVL.findClosest(elo)
3. Create match     → Return match to both players
```

### Match Completion
```
1. Submit result    → RankingService.updateRankings()
2. Calculate ELO    → AVL.remove(old) + AVL.insert(new)
3. Record history   → LinkedList.append(match)
```

---

## Technology Stack

| Layer | Technology |
|-------|------------|
| Backend | C++17 (Pure DSA) |
| HTTP Server | cpp-httplib (single header) |
| Frontend | HTML5, CSS3, JavaScript |
| Games | HTML5 Canvas |
| Communication | REST API (JSON) |

---

## File Structure

```
/backend-cpp/
  /ds/
    HashTable.h
    AVLTree.h
    Queue.h
    LinkedList.h
  /models/
    Player.h
    Match.h
  /services/
    Matchmaker.h
    RankingService.h
    HistoryService.h
  main.cpp
  httplib.h

/frontend/
  index.html
  /styles/
    main.css
  /js/
    api.js
    app.js
  /games/
    pingpong.js
    snake.js
    tank.js

/docs/
  DSA-Justification.md
  Architecture.md
```
