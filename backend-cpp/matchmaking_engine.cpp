/**
 * Matchmaking Engine - Stdin/Stdout JSON Interface
 * 
 * PURPOSE:
 * Standalone C++ process that handles matchmaking commands via stdin/stdout.
 * Designed to be spawned by Node.js WebSocket bridge.
 * 
 * COMMUNICATION:
 *   Input (stdin):  One JSON command per line
 *   Output (stdout): One JSON response per line
 * 
 * DSA PRESERVED:
 *   - AVLTree<PlayerELO>       : O(log n) closest-ELO matching
 *   - HashTable<int, Player>   : O(1) player storage
 *   - Queue<QueueEntry>        : O(1) FIFO matchmaking lobby
 *   - LinkedList<Match>        : O(1) match history
 * 
 * BUILD:
 *   g++ -std=c++11 -O2 -o engine matchmaking_engine.cpp
 * 
 * USAGE:
 *   ./engine           (reads from stdin, writes to stdout)
 */

#include "ds/HashTable.h"
#include "ds/AVLTree.h"
#include "ds/Queue.h"
#include "ds/LinkedList.h"
#include "models/Player.h"
#include "models/Match.h"
#include "services/RankingService.h"
#include "services/HistoryService.h"
#include "services/Matchmaker.h"

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

// ============== SIMPLE JSON PARSER ==============

/**
 * Extract a string value from JSON
 * Example: getJsonString("{\"name\":\"Ahmed\"}", "name") -> "Ahmed"
 */
std::string getJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";
    
    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) return "";
    
    size_t valueStart = colonPos + 1;
    while (valueStart < json.size() && (json[valueStart] == ' ' || json[valueStart] == '\t')) {
        valueStart++;
    }
    
    if (json[valueStart] == '"') {
        size_t valueEnd = json.find('"', valueStart + 1);
        if (valueEnd == std::string::npos) return "";
        return json.substr(valueStart + 1, valueEnd - valueStart - 1);
    }
    
    return "";
}

/**
 * Extract an integer value from JSON
 * Example: getJsonInt("{\"elo\":1200}", "elo") -> 1200
 */
int getJsonInt(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return 0;
    
    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) return 0;
    
    size_t valueStart = colonPos + 1;
    while (valueStart < json.size() && (json[valueStart] == ' ' || json[valueStart] == '\t')) {
        valueStart++;
    }
    
    size_t valueEnd = valueStart;
    while (valueEnd < json.size() && (json[valueEnd] >= '0' && json[valueEnd] <= '9')) {
        valueEnd++;
    }
    
    if (valueEnd > valueStart) {
        return std::stoi(json.substr(valueStart, valueEnd - valueStart));
    }
    
    return 0;
}

// ============== JSON OUTPUT HELPERS ==============

void outputJson(const std::string& json) {
    std::cout << json << std::endl;
    std::cout.flush();
}

void outputOk(const std::string& clientId, int playerId) {
    std::cout << "{\"type\":\"OK\",\"clientId\":\"" << clientId 
              << "\",\"playerId\":" << playerId << "}" << std::endl;
    std::cout.flush();
}

void outputQueued(const std::string& clientId, int position) {
    std::cout << "{\"type\":\"QUEUED\",\"clientId\":\"" << clientId 
              << "\",\"position\":" << position << "}" << std::endl;
    std::cout.flush();
}

void outputMatched(const std::string& clientId, int matchId, 
                   const std::string& opponent, int opponentElo, const std::string& game) {
    std::cout << "{\"type\":\"MATCHED\",\"clientId\":\"" << clientId 
              << "\",\"matchId\":" << matchId 
              << ",\"opponent\":\"" << opponent 
              << "\",\"opponentElo\":" << opponentElo 
              << ",\"game\":\"" << game << "\"}" << std::endl;
    std::cout.flush();
}

void outputStatus(const std::string& clientId, bool inQueue, bool inMatch, int matchId) {
    std::cout << "{\"type\":\"STATUS\",\"clientId\":\"" << clientId 
              << "\",\"inQueue\":" << (inQueue ? "true" : "false")
              << ",\"inMatch\":" << (inMatch ? "true" : "false")
              << ",\"matchId\":" << matchId << "}" << std::endl;
    std::cout.flush();
}

void outputLeaderboard(const std::string& clientId, const std::string& game,
                       int* playerIds, int* elos, const char** names, int count) {
    std::cout << "{\"type\":\"LEADERBOARD\",\"clientId\":\"" << clientId 
              << "\",\"game\":\"" << game << "\",\"players\":[";
    for (int i = 0; i < count; i++) {
        if (i > 0) std::cout << ",";
        std::cout << "{\"rank\":" << (i+1) 
                  << ",\"name\":\"" << names[i] 
                  << "\",\"elo\":" << elos[i] << "}";
    }
    std::cout << "]}" << std::endl;
    std::cout.flush();
}

void outputResult(const std::string& clientId, int newElo) {
    std::cout << "{\"type\":\"RESULT\",\"clientId\":\"" << clientId 
              << "\",\"newElo\":" << newElo << "}" << std::endl;
    std::cout.flush();
}

void outputError(const std::string& clientId, const std::string& message) {
    std::cout << "{\"type\":\"ERROR\",\"clientId\":\"" << clientId 
              << "\",\"message\":\"" << message << "\"}" << std::endl;
    std::cout.flush();
}

void outputLog(const std::string& message) {
    std::cerr << "[Engine] " << message << std::endl;
}

// ============== MATCHMAKING SERVICES ==============

class MatchmakingEngine {
private:
    HashTable<int, Player> playerStorage;
    RankingService rankingService;
    HistoryService historyService;
    Matchmaker matchmaker;
    
    // Client ID -> Player ID mapping
    HashTable<int, int> clientToPlayer;  // hash of clientId -> playerId
    
    int nextPlayerId;
    static const int BOT_ID_START = 1000;
    
    int hashClientId(const std::string& clientId) {
        int hash = 0;
        for (size_t i = 0; i < clientId.size(); i++) {
            hash = hash * 31 + clientId[i];
        }
        return hash < 0 ? -hash : hash;
    }
    
public:
    MatchmakingEngine() 
        : rankingService(&playerStorage),
          matchmaker(&playerStorage, &rankingService, &historyService),
          nextPlayerId(1) {}
    
    void initializeBots() {
        srand(static_cast<unsigned>(time(NULL)));
        
        const char* games[] = {"pingpong", "snake", "tank"};
        const int BOTS_PER_GAME = 5;
        
        int botId = BOT_ID_START;
        
        for (int g = 0; g < 3; g++) {
            const char* game = games[g];
            
            for (int i = 0; i < BOTS_PER_GAME; i++) {
                int elo = 800 + (rand() % 801);
                
                char botName[50];
                snprintf(botName, sizeof(botName), "BOT_%d", botId - BOT_ID_START + 1);
                
                Player bot(botId, botName, elo, true);
                bot.setPreferredGame(game);
                playerStorage.insert(botId, bot);
                
                matchmaker.registerBot(botId, game);
                rankingService.addPlayerToRanking(botId, game);
                
                outputLog(std::string("Created ") + botName + " (ELO: " + std::to_string(elo) + ") for " + game);
                botId++;
            }
        }
        
        nextPlayerId = botId + 1;
        outputLog("Total bots created: " + std::to_string(botId - BOT_ID_START));
    }
    
    // ========== COMMAND HANDLERS ==========
    
    void handleJoin(const std::string& clientId, const std::string& username, int elo) {
        // Check if client already has a player
        int clientHash = hashClientId(clientId);
        int* existingId = clientToPlayer.get(clientHash);
        if (existingId) {
            // Return existing player
            Player* p = playerStorage.get(*existingId);
            if (p) {
                outputOk(clientId, *existingId);
                return;
            }
        }
        
        // Check if username exists
        int existingKeys[2000];
        size_t keyCount;
        playerStorage.getAllKeys(existingKeys, keyCount);
        
        for (size_t i = 0; i < keyCount; i++) {
            Player* existing = playerStorage.get(existingKeys[i]);
            if (existing && strcmp(existing->username, username.c_str()) == 0) {
                clientToPlayer.insert(clientHash, existing->id);
                outputOk(clientId, existing->id);
                return;
            }
        }
        
        // Create new player
        int playerId = nextPlayerId++;
        Player player(playerId, username.c_str(), elo, false);
        playerStorage.insert(playerId, player);
        clientToPlayer.insert(clientHash, playerId);
        
        outputLog("Player joined: " + username + " (ID: " + std::to_string(playerId) + ")");
        outputOk(clientId, playerId);
    }
    
    void handleQueue(const std::string& clientId, int playerId, const std::string& game) {
        Player* player = playerStorage.get(playerId);
        if (!player) {
            outputError(clientId, "Player not found");
            return;
        }
        
        if (player->isInQueue) {
            outputError(clientId, "Already in queue");
            return;
        }
        
        if (player->isInMatch) {
            outputError(clientId, "Already in match");
            return;
        }
        
        if (!matchmaker.joinQueue(playerId, game.c_str())) {
            outputError(clientId, "Failed to join queue");
            return;
        }
        
        // Refresh player pointer (may have changed)
        player = playerStorage.get(playerId);
        
        int position = static_cast<int>(matchmaker.getQueueSize(game.c_str()));
        outputLog("Player " + std::to_string(playerId) + " queued for " + game + " (position: " + std::to_string(position) + ")");
        
        // Try to create a match immediately
        Match match;
        int matchId = matchmaker.tryCreateMatch(game.c_str());
        
        if (matchId != -1) {
            Match* m = matchmaker.getMatch(matchId);
            if (m) {
                int opponentId = (m->player1Id == playerId) ? m->player2Id : m->player1Id;
                Player* opponent = playerStorage.get(opponentId);
                
                if (opponent) {
                    outputLog("Match created: " + std::to_string(matchId) + " - " + 
                              std::string(player->username) + " vs " + std::string(opponent->username));
                    outputMatched(clientId, matchId, opponent->username, opponent->elo, game);
                    return;
                }
            }
        }
        
        // No immediate match - still queued
        outputQueued(clientId, position);
    }
    
    void handleLeave(const std::string& clientId, int playerId) {
        Player* player = playerStorage.get(playerId);
        if (!player) {
            outputError(clientId, "Player not found");
            return;
        }
        
        if (!player->isInQueue) {
            outputError(clientId, "Not in queue");
            return;
        }
        
        // Leave all queues
        const char* games[] = {"pingpong", "snake", "tank"};
        bool left = false;
        for (int i = 0; i < 3; i++) {
            if (matchmaker.leaveQueue(playerId, games[i])) {
                left = true;
                break;
            }
        }
        
        if (left) {
            outputLog("Player " + std::to_string(playerId) + " left queue");
            std::cout << "{\"type\":\"OK\",\"clientId\":\"" << clientId << "\"}" << std::endl;
            std::cout.flush();
        } else {
            outputError(clientId, "Failed to leave queue");
        }
    }
    
    void handleStatus(const std::string& clientId, int playerId) {
        Player* player = playerStorage.get(playerId);
        if (!player) {
            outputError(clientId, "Player not found");
            return;
        }
        
        int activeMatchId = matchmaker.getPlayerActiveMatch(playerId);
        outputStatus(clientId, player->isInQueue, player->isInMatch, activeMatchId);
    }
    
    void handleResult(const std::string& clientId, int matchId, int winnerId) {
        if (!matchmaker.submitMatchResult(matchId, winnerId)) {
            outputError(clientId, "Failed to submit result");
            return;
        }
        
        Player* winner = playerStorage.get(winnerId);
        int newElo = winner ? winner->elo : 0;
        
        outputLog("Match " + std::to_string(matchId) + " result: Winner ID " + std::to_string(winnerId));
        outputResult(clientId, newElo);
    }
    
    void handleLeaderboard(const std::string& clientId, const std::string& game) {
        int playerIds[20], elos[20];
        const char* names[20];
        
        int count = rankingService.getLeaderboard(game.c_str(), playerIds, elos, 20);
        
        for (int i = 0; i < count; i++) {
            Player* p = playerStorage.get(playerIds[i]);
            names[i] = p ? p->username : "Unknown";
        }
        
        outputLeaderboard(clientId, game, playerIds, elos, names, count);
    }
    
    void handleDisconnect(const std::string& clientId) {
        int clientHash = hashClientId(clientId);
        int* playerId = clientToPlayer.get(clientHash);
        
        if (playerId) {
            // Leave all queues
            const char* games[] = {"pingpong", "snake", "tank"};
            for (int i = 0; i < 3; i++) {
                matchmaker.leaveQueue(*playerId, games[i]);
            }
            
            Player* p = playerStorage.get(*playerId);
            if (p) {
                p->isInQueue = false;
                playerStorage.update(*playerId, *p);
            }
            
            outputLog("Client disconnected: " + clientId + " (player: " + std::to_string(*playerId) + ")");
        }
    }
};

// ============== MAIN LOOP ==============

int main() {
    // Disable buffering for real-time communication
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    
    outputLog("Matchmaking Engine starting...");
    
    MatchmakingEngine engine;
    engine.initializeBots();
    
    outputLog("Ready - listening for commands on stdin");
    
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        // Parse command
        std::string cmd = getJsonString(line, "cmd");
        std::string clientId = getJsonString(line, "clientId");
        
        if (cmd.empty() || clientId.empty()) {
            outputError("unknown", "Invalid command format");
            continue;
        }
        
        // Route to handler
        if (cmd == "JOIN") {
            std::string username = getJsonString(line, "username");
            int elo = getJsonInt(line, "elo");
            if (elo == 0) elo = 1000;
            engine.handleJoin(clientId, username, elo);
        }
        else if (cmd == "QUEUE") {
            int playerId = getJsonInt(line, "playerId");
            std::string game = getJsonString(line, "game");
            engine.handleQueue(clientId, playerId, game);
        }
        else if (cmd == "LEAVE") {
            int playerId = getJsonInt(line, "playerId");
            engine.handleLeave(clientId, playerId);
        }
        else if (cmd == "STATUS") {
            int playerId = getJsonInt(line, "playerId");
            engine.handleStatus(clientId, playerId);
        }
        else if (cmd == "RESULT") {
            int matchId = getJsonInt(line, "matchId");
            int winnerId = getJsonInt(line, "winnerId");
            engine.handleResult(clientId, matchId, winnerId);
        }
        else if (cmd == "LEADERBOARD") {
            std::string game = getJsonString(line, "game");
            engine.handleLeaderboard(clientId, game);
        }
        else if (cmd == "DISCONNECT") {
            engine.handleDisconnect(clientId);
        }
        else {
            outputError(clientId, "Unknown command: " + cmd);
        }
    }
    
    outputLog("Engine shutting down");
    return 0;
}
