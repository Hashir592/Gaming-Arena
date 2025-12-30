# System Architecture Framework

## High-Level System Design

This diagram represents the **complete architectural framework** of the Multiplayer Game System, showing how the Frontend interacts with the specialized C++ Backend Layers.

```mermaid
graph TB
    %% ==========================================
    %% PRESENTATION LAYER (The User Interface)
    %% ==========================================
    subgraph "Presentation Layer (Frontend)"
        UI[User Interface<br>(HTML5 / CSS3)]
        JS[Game Logic<br>(JavaScript / Canvas)]
        API_C[API Client<br>(fetch / async-await)]
    end

    %% ==========================================
    %% APPLICATION LAYER (The Gateway)
    %% ==========================================
    subgraph "Application Layer (C++ Server)"
        Server[HTTP Server<br>(simple_http.h)]
        Router[Request Router]
        Endpoints[API Endpoints<br>(/api/players, /join, /result)]
    end

    %% ==========================================
    %% DOMAIN LAYER (Business Logic)
    %% ==========================================
    subgraph "Domain Layer (Services)"
        MM_S[Matchmaker Service<br>(Core Logic)]
        RS_S[Ranking Service<br>(ELO Calc)]
        HS_S[History Service<br>(Records)]
    end

    %% ==========================================
    %% DATA LAYER (Custom Memory Structures)
    %% ==========================================
    subgraph "Data Layer (The DSA Core)"
        direction TB
        
        HT_Data[("HashTable<br>(O(1) Storage)")]
        AVL_Data[("AVL Tree<br>(O(log n) Ranking)")]
        Q_Data[("Queue<br>(FIFO Lobby)")]
        LL_Data[("LinkedList<br>(History Chains)")]
    end

    %% ==========================================
    %% CONNECTIONS
    %% ==========================================
    
    %% Basic Flow
    UI --> JS
    JS --> API_C
    API_C -- "JSON / HTTP" --> Server
    Server --> Router
    Router --> Endpoints
    
    %% Service Connections
    Endpoints --> MM_S
    Endpoints --> RS_S
    Endpoints --> HS_S
    
    %% Data Connections
    MM_S -- "Read/Write Parameters" --> HT_Data
    MM_S -- "Enqueue Players" --> Q_Data
    MM_S -- "Find Opponent" --> AVL_Data
    
    RS_S -- "Update ELO" --> AVL_Data
    HS_S -- "Append Match" --> LL_Data
    
    %% Styling
    style UI fill:#eceff1,stroke:#455a64
    style Server fill:#e3f2fd,stroke:#1565c0
    style MM_S fill:#f3e5f5,stroke:#7b1fa2
    style HT_Data fill:#e8f5e9,stroke:#2e7d32
    style AVL_Data fill:#e8f5e9,stroke:#2e7d32
    style Q_Data fill:#e8f5e9,stroke:#2e7d32
    style LL_Data fill:#e8f5e9,stroke:#2e7d32
```

## Description of Layers

### 1. Presentation Layer (Frontend)
The "Face" of the application.
*   **Technologies:** HTML, CSS, JavaScript.
*   **Responsibility:** Renders the game, captures inputs, and *polls* the backend for updates.

### 2. Application Layer (Server)
The "Gateway".
*   **Technologies:** C++ (`server_compat.cpp`), Custom HTTP.
*   **Responsibility:** Listens on port 8080, parses raw HTTP strings, and routes requests to the correct function. It acts as the bridge between JSON (Frontend) and Structs (Backend).

### 3. Domain Layer (Services)
The "Brain".
*   **Technologies:** C++ Classes (`Matchmaker`, `RankingService`).
*   **Responsibility:** Contains the game rules.
    *   *Matchmaker:* Decides who plays whom.
    *   *RankingService:* Calculates ELO math ($R_a = R_a + K(S_a - E_a)$).
    *   *HistoryService:* Manages past records.

### 4. Data Layer (The DSA Core)
The "Memory".
*   **Technologies:** Custom Templates (`HashTable`, `AVLTree`, `Queue`, `LinkedList`).
*   **Responsibility:** Storing data efficiently. This is where the academic requirement is fulfilled. We do **not** use a database (like SQL) or STL (Standard Template Library); we manage memory manually using these structures.
