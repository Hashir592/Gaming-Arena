/**
 * Multiplayer Game System - C++ Backend (Compatible Version)
 * 
 * Uses simple HTTP implementation for older MinGW compilers.
 * For modern compilers, use main.cpp with cpp-httplib instead.
 */

#include "simple_http.h"
#include "ds/HashTable.h"
#include "ds/AVLTree.h"
#include "ds/Queue.h"
#include "ds/LinkedList.h"
#include "models/Player.h"
#include "models/Match.h"
#include "services/RankingService.h"
#include "services/HistoryService.h"
#include "services/Matchmaker.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
#include <ctime>

// Global data storage
HashTable<int, Player> playerStorage;
RankingService rankingService(&playerStorage);
HistoryService historyService;
Matchmaker matchmaker(&playerStorage, &rankingService, &historyService);
int nextPlayerId = 1;

// Bot ID range (1000+)
const int BOT_ID_START = 1000;

/**
 * Initialize bot players at server startup
 * Creates 5 bots per game with randomized ELO (800-1600)
 */
void initializeBots() {
    srand(static_cast<unsigned>(time(nullptr)));
    
    const char* games[] = {"pingpong", "snake", "tank"};
    const int BOTS_PER_GAME = 5;
    
    int botId = BOT_ID_START;
    
    for (int g = 0; g < 3; g++) {
        const char* game = games[g];
        
        for (int i = 0; i < BOTS_PER_GAME; i++) {
            // Generate random ELO between 800-1600
            int elo = 800 + (rand() % 801);
            
            // Create bot name like "BOT_1", "BOT_2", etc.
            char botName[50];
            snprintf(botName, sizeof(botName), "BOT_%d", botId - BOT_ID_START + 1);
            
            // Create bot player (isBot = true)
            Player bot(botId, botName, elo, true);
            bot.setPreferredGame(game);
            playerStorage.insert(botId, bot);
            
            // Register bot with matchmaker for this game
            matchmaker.registerBot(botId, game);
            
            // Add to ranking tree for this game
            rankingService.addPlayerToRanking(botId, game);
            
            printf("  Created %s (ELO: %d) for %s\n", botName, elo, game);
            botId++;
        }
    }
    
    // Ensure human player IDs start after bots
    nextPlayerId = botId + 1;
    
    printf("\nTotal bots created: %d\n\n", botId - BOT_ID_START);
}

// Simple JSON helpers
std::string jsonString(const char* key, const char* value) {
    return "\"" + std::string(key) + "\":\"" + std::string(value) + "\"";
}

std::string jsonInt(const char* key, int value) {
    return "\"" + std::string(key) + "\":" + std::to_string(value);
}

std::string jsonBool(const char* key, bool value) {
    return "\"" + std::string(key) + "\":" + (value ? "true" : "false");
}

std::string jsonFloat(const char* key, float value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", value);
    return "\"" + std::string(key) + "\":" + std::string(buf);
}

// Parse simple JSON
std::string getJsonValue(const std::string& json, const std::string& key) {
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
    } else {
        size_t valueEnd = valueStart;
        while (valueEnd < json.size() && json[valueEnd] != ',' && json[valueEnd] != '}') {
            valueEnd++;
        }
        std::string val = json.substr(valueStart, valueEnd - valueStart);
        while (!val.empty() && (val.back() == ' ' || val.back() == '\t' || val.back() == '\n')) {
            val.pop_back();
        }
        return val;
    }
}

int main() {
    http::Server svr;
    
    // ==================== PLAYER ENDPOINTS ====================
    
    svr.Post("/api/players", [](const http::Request& req, http::Response& res) {
        std::string username = getJsonValue(req.body, "username");
        std::string eloStr = getJsonValue(req.body, "elo");
        
        if (username.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"Username required\"}", "application/json");
            return;
        }
        
        // Check if username already exists (iterate through all players)
        int existingKeys[2000];
        size_t keyCount;
        playerStorage.getAllKeys(existingKeys, keyCount);
        
        for (size_t i = 0; i < keyCount; i++) {
            Player* existing = playerStorage.get(existingKeys[i]);
            if (existing && strcmp(existing->username, username.c_str()) == 0) {
                // Username already taken - return the existing player instead
                std::string response = "{" +
                    jsonInt("id", existing->id) + "," +
                    jsonString("username", existing->username) + "," +
                    jsonInt("elo", existing->elo) + "," +
                    jsonInt("wins", existing->wins) + "," +
                    jsonInt("losses", existing->losses) + "," +
                    jsonBool("isBot", existing->isBot) + "," +
                    jsonString("message", "Welcome back!") +
                "}";
                res.set_content(response, "application/json");
                printf("[Server] Player '%s' logged back in (ID: %d)\n", existing->username, existing->id);
                return;
            }
        }
        
        // Username is available - create new player
        int elo = eloStr.empty() ? 1000 : std::stoi(eloStr);
        int playerId = nextPlayerId++;
        
        Player player(playerId, username.c_str(), elo);
        playerStorage.insert(playerId, player);
        
        printf("[Server] New player '%s' registered (ID: %d)\n", username.c_str(), playerId);
        
        std::string response = "{" +
            jsonInt("id", playerId) + "," +
            jsonString("username", player.username) + "," +
            jsonInt("elo", player.elo) + "," +
            jsonInt("wins", 0) + "," +
            jsonInt("losses", 0) +
        "}";
        
        res.set_content(response, "application/json");
    });
    
    svr.Get("/api/players/(\\d+)", [](const http::Request& req, http::Response& res) {
        int playerId = std::stoi(req.matches[1]);
        Player* player = playerStorage.get(playerId);
        
        if (!player) {
            res.status = 404;
            res.set_content("{\"error\":\"Player not found\"}", "application/json");
            return;
        }
        
        std::string response = "{" +
            jsonInt("id", player->id) + "," +
            jsonString("username", player->username) + "," +
            jsonInt("elo", player->elo) + "," +
            jsonInt("wins", player->wins) + "," +
            jsonInt("losses", player->losses) + "," +
            jsonFloat("winRate", player->getWinRate()) + "," +
            jsonBool("isInQueue", player->isInQueue) + "," +
            jsonBool("isInMatch", player->isInMatch) + "," +
            jsonBool("isBot", player->isBot) +
        "}";
        
        res.set_content(response, "application/json");
    });
    
    // ==================== MATCHMAKING ENDPOINTS ====================
    
    svr.Post("/api/matchmaking/join", [](const http::Request& req, http::Response& res) {
        std::string playerIdStr = getJsonValue(req.body, "playerId");
        std::string gameName = getJsonValue(req.body, "game");
        
        if (playerIdStr.empty() || gameName.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"playerId and game required\"}", "application/json");
            return;
        }
        
        int playerId = std::stoi(playerIdStr);
        
        // Fix: Force reset stale player state if they try to join again
        Player* player = playerStorage.get(playerId);
        if (player) {
            if (player->isInQueue) {
                printf("[Server] Resetting stale queue state for player %d\n", playerId);
                matchmaker.leaveQueue(playerId, gameName.c_str());
                player->isInQueue = false;
                playerStorage.update(playerId, *player);
            }
            
            if (player->isInMatch) {
                printf("[Server] Force-ending stale match for player %d\n", playerId);
                // Find and end the stale match to free up the opponent (bot/human)
                int activeMatchId = matchmaker.getPlayerActiveMatch(playerId);
                if (activeMatchId != -1) {
                    // Give win to this player to close it out simply
                    matchmaker.submitMatchResult(activeMatchId, playerId);
                }
                player->isInMatch = false;
                playerStorage.update(playerId, *player);
            }
        }

        if (matchmaker.joinQueue(playerId, gameName.c_str())) {
            int matchId = matchmaker.tryCreateMatch(gameName.c_str());
            
            if (matchId != -1) {
                Match* match = matchmaker.getMatch(matchId);
                std::string response = "{" +
                    jsonBool("queued", false) + "," +
                    jsonBool("matched", true) + "," +
                    jsonInt("matchId", matchId) + "," +
                    jsonInt("player1Id", match->player1Id) + "," +
                    jsonInt("player2Id", match->player2Id) + "," +
                    jsonString("game", match->gameName) +
                "}";
                res.set_content(response, "application/json");
            } else {
                std::string response = "{" +
                    jsonBool("queued", true) + "," +
                    jsonBool("matched", false) + "," +
                    jsonInt("queuePosition", static_cast<int>(matchmaker.getQueueSize(gameName.c_str()))) +
                "}";
                res.set_content(response, "application/json");
            }
        } else {
            res.status = 400;
            res.set_content("{\"error\":\"Failed to join queue\"}", "application/json");
        }
    });
    
    svr.Post("/api/matchmaking/leave", [](const http::Request& req, http::Response& res) {
        std::string playerIdStr = getJsonValue(req.body, "playerId");
        std::string gameName = getJsonValue(req.body, "game");
        
        if (playerIdStr.empty() || gameName.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"playerId and game required\"}", "application/json");
            return;
        }
        
        int playerId = std::stoi(playerIdStr);
        
        if (matchmaker.leaveQueue(playerId, gameName.c_str())) {
            res.set_content("{\"success\":true}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"error\":\"Not in queue\"}", "application/json");
        }
    });
    
    svr.Get("/api/matchmaking/status/(\\d+)", [](const http::Request& req, http::Response& res) {
        int playerId = std::stoi(req.matches[1]);
        Player* player = playerStorage.get(playerId);
        
        if (!player) {
            res.status = 404;
            res.set_content("{\"error\":\"Player not found\"}", "application/json");
            return;
        }
        
        // Try to create a match if player is in queue (handles bot timeout)
        if (player->isInQueue) {
            // Try matching for all games since we don't track which game they queued for
            matchmaker.tryCreateMatch("pingpong");
            matchmaker.tryCreateMatch("snake");
            matchmaker.tryCreateMatch("tank");
        }
        
        int activeMatchId = matchmaker.getPlayerActiveMatch(playerId);
        
        std::string response = "{" +
            jsonBool("isInQueue", player->isInQueue) + "," +
            jsonBool("isInMatch", player->isInMatch) + "," +
            jsonInt("activeMatchId", activeMatchId) +
        "}";
        
        res.set_content(response, "application/json");
    });
    
    // ==================== MATCH ENDPOINTS ====================
    
    svr.Get("/api/matches/(\\d+)", [](const http::Request& req, http::Response& res) {
        int matchId = std::stoi(req.matches[1]);
        Match* match = matchmaker.getMatch(matchId);
        
        if (!match) {
            res.status = 404;
            res.set_content("{\"error\":\"Match not found\"}", "application/json");
            return;
        }
        
        Player* p1 = playerStorage.get(match->player1Id);
        Player* p2 = playerStorage.get(match->player2Id);
        
        std::string response = "{" +
            jsonInt("matchId", match->matchId) + "," +
            jsonInt("player1Id", match->player1Id) + "," +
            jsonString("player1Name", p1 ? p1->username : "Unknown") + "," +
            jsonInt("player2Id", match->player2Id) + "," +
            jsonString("player2Name", p2 ? p2->username : "Unknown") + "," +
            jsonString("game", match->gameName) + "," +
            jsonBool("isCompleted", match->isCompleted) + "," +
            jsonInt("winnerId", match->winnerId) +
        "}";
        
        res.set_content(response, "application/json");
    });
    
    svr.Post("/api/matches/result", [](const http::Request& req, http::Response& res) {
        std::string matchIdStr = getJsonValue(req.body, "matchId");
        std::string winnerIdStr = getJsonValue(req.body, "winnerId");
        
        if (matchIdStr.empty() || winnerIdStr.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"matchId and winnerId required\"}", "application/json");
            return;
        }
        
        int matchId = std::stoi(matchIdStr);
        int winnerId = std::stoi(winnerIdStr);
        
        if (matchmaker.submitMatchResult(matchId, winnerId)) {
            Match* match = matchmaker.getMatch(matchId);
            Player* winner = playerStorage.get(winnerId);
            int loserId = (winnerId == match->player1Id) ? match->player2Id : match->player1Id;
            Player* loser = playerStorage.get(loserId);
            
            std::string response = "{" +
                jsonBool("success", true) + "," +
                jsonInt("winnerNewElo", winner ? winner->elo : 0) + "," +
                jsonInt("loserNewElo", loser ? loser->elo : 0) +
            "}";
            res.set_content(response, "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"error\":\"Failed to submit result\"}", "application/json");
        }
    });
    
    // ==================== LEADERBOARD ENDPOINTS ====================
    
    svr.Get("/api/leaderboard/(\\w+)", [](const http::Request& req, http::Response& res) {
        std::string gameName = req.matches[1];
        
        int playerIds[100];
        int elos[100];
        int count = rankingService.getLeaderboard(gameName.c_str(), playerIds, elos, 100);
        
        std::string response = "{\"game\":\"" + gameName + "\",\"leaderboard\":[";
        
        for (int i = 0; i < count; i++) {
            Player* player = playerStorage.get(playerIds[i]);
            if (player) {
                if (i > 0) response += ",";
                response += "{" +
                    jsonInt("rank", i + 1) + "," +
                    jsonInt("playerId", player->id) + "," +
                    jsonString("username", player->username) + "," +
                    jsonInt("elo", player->elo) + "," +
                    jsonInt("wins", player->wins) + "," +
                    jsonInt("losses", player->losses) +
                "}";
            }
        }
        
        response += "]}";
        res.set_content(response, "application/json");
    });
    
    // ==================== HISTORY ENDPOINTS ====================
    
    svr.Get("/api/history/(\\d+)", [](const http::Request& req, http::Response& res) {
        int playerId = std::stoi(req.matches[1]);
        
        Match matches[50];
        int count;
        historyService.getLastNMatches(playerId, 50, matches, count);
        
        std::string response = "{\"playerId\":" + std::to_string(playerId) + ",\"matches\":[";
        
        for (int i = 0; i < count; i++) {
            int opponentId = matches[i].getOpponentId(playerId);
            Player* opponent = playerStorage.get(opponentId);
            bool won = matches[i].didPlayerWin(playerId);
            
            if (i > 0) response += ",";
            response += "{" +
                jsonInt("matchId", matches[i].matchId) + "," +
                jsonInt("opponentId", opponentId) + "," +
                jsonString("opponentName", opponent ? opponent->username : "Unknown") + "," +
                jsonString("game", matches[i].gameName) + "," +
                jsonBool("won", won) +
            "}";
        }
        
        response += "]}";
        res.set_content(response, "application/json");
    });
    
    // ==================== UTILITY ENDPOINTS ====================
    
    svr.Get("/api/queues", [](const http::Request&, http::Response& res) {
        std::string response = "{" +
            jsonInt("pingpong", static_cast<int>(matchmaker.getQueueSize("pingpong"))) + "," +
            jsonInt("snake", static_cast<int>(matchmaker.getQueueSize("snake"))) + "," +
            jsonInt("tank", static_cast<int>(matchmaker.getQueueSize("tank"))) +
        "}";
        res.set_content(response, "application/json");
    });
    
    svr.Get("/api/health", [](const http::Request&, http::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });
    
    // Logout endpoint - removes player from queue and clears session
    svr.Post("/api/logout", [](const http::Request& req, http::Response& res) {
        std::string playerIdStr = getJsonValue(req.body, "playerId");
        
        if (playerIdStr.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"playerId required\"}", "application/json");
            return;
        }
        
        int playerId = std::stoi(playerIdStr);
        Player* player = playerStorage.get(playerId);
        
        if (!player) {
            res.status = 404;
            res.set_content("{\"error\":\"Player not found\"}", "application/json");
            return;
        }
        
        // Leave any queues
        const char* games[] = {"pingpong", "snake", "tank"};
        for (int i = 0; i < 3; i++) {
            matchmaker.leaveQueue(playerId, games[i]);
        }
        
        // Update player state
        player->isInQueue = false;
        playerStorage.update(playerId, *player);
        
        res.set_content("{\"success\":true}", "application/json");
    });
    
    printf("======================================\n");
    printf("  Multiplayer Game System Backend\n");
    printf("======================================\n");
    printf("\nInitializing bot players...\n");
    initializeBots();
    printf("Server starting on http://localhost:8080\n");
    printf("Press Ctrl+C to stop\n\n");
    
    svr.listen("0.0.0.0", 8080);
    
    return 0;
}
