# Multiplayer Matchmaking Arena

A browser-accessible matchmaking system powered by a C++ DSA engine.

## 🌐 Live Demo

**Open in browser:** `https://your-app.onrender.com`

Multiple users can join simultaneously and get matched in real-time!

---

## 🏗️ Architecture

```
Browser  ─────WebSocket────▶  Node.js Bridge  ────stdin/stdout────▶  C++ Engine
(HTML/JS)                     (server.js)                           (matchmaking_engine.cpp)
    │                              │                                       │
    │  Send: JOIN, QUEUE           │  Spawn & manage                       │  AVL Tree
    │  Recv: MATCHED, QUEUED       │  Route messages                       │  Hash Table
    │                              │                                       │  Queue
    ▼                              ▼                                       ▼
```

**Key Points:**
- All matchmaking logic runs in **C++** (preserved DSA)
- Node.js only routes messages (no game logic)
- Browser connects via WebSocket
- Single container deployment on Render.com

---

## 🚀 Quick Start (Local)

### 1. Build C++ Engine

```bash
cd backend-cpp
g++ -std=c++11 -O2 -o engine matchmaking_engine.cpp
```

### 2. Install Node.js Dependencies

```bash
cd bridge
npm install
```

### 3. Start Server

```bash
node server.js
```

### 4. Open Browser

Go to: **http://localhost:3000**

---

## ☁️ Deploy to Render.com

### Option 1: Blueprint (Recommended)

1. Push code to GitHub
2. Go to [render.com](https://render.com) → New → Blueprint
3. Connect your repository
4. Render auto-detects `render.yaml` and deploys

### Option 2: Manual

1. Go to Render → New → Web Service
2. Connect GitHub repo
3. Settings:
   - **Runtime:** Docker
   - **Dockerfile Path:** `./Dockerfile`
4. Deploy

Your public URL will be: `https://matchmaking-arena.onrender.com`

---

## 📁 Project Structure

```
/backend-cpp/
  matchmaking_engine.cpp    # C++ stdin/stdout server
  /ds/                      # Data structures (AVL, Hash, Queue, List)
  /models/                  # Player, Match models
  /services/                # Matchmaker, Ranking, History

/bridge/
  server.js                 # Node.js WebSocket bridge
  package.json              # Node dependencies
  /public/
    index.html              # Browser UI

Dockerfile                  # Container build
render.yaml                 # Render.com config
```

---

## 📡 Protocol

### Browser → Server (WebSocket JSON)

```json
{"cmd":"JOIN","username":"Ahmed","elo":1200}
{"cmd":"QUEUE","playerId":1,"game":"pingpong"}
{"cmd":"LEAVE","playerId":1}
{"cmd":"STATUS","playerId":1}
```

### Server → Browser (WebSocket JSON)

```json
{"type":"CONNECTED","clientId":"ws-1-1234567890"}
{"type":"OK","playerId":1}
{"type":"QUEUED","position":2}
{"type":"MATCHED","matchId":5,"opponent":"BOT_3","opponentElo":1150}
```

---

## 📊 DSA Features

| Data Structure | Purpose | Complexity |
|---------------|---------|------------|
| **AVL Tree** | ELO-based closest opponent matching | O(log n) |
| **Hash Table** | Player storage by ID | O(1) average |
| **Queue** | FIFO matchmaking lobby per game | O(1) |
| **LinkedList** | Match history, hash collision chains | O(1) append |

---

## 🎮 Games

- 🏓 Ping Pong
- 🐍 Snake Battle  
- 🎯 Tank Wars

---

## 📝 License

Academic project for educational purposes.
