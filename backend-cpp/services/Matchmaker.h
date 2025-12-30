#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#include "../ds/Queue.h"
#include "../ds/HashTable.h"
#include "../ds/AVLTree.h"
#include "../models/Player.h"
#include "../models/Match.h"
#include "RankingService.h"
#include "HistoryService.h"
#include <ctime>

/**
 * Matchmaker - Core matchmaking service using DSA
 * 
 * MATCHMAKING ALGORITHM (Closest-Rank based):
 * 1. Player selects a game and enters matchmaking queue
 * 2. Player ID enqueued in that game's queue
 * 3. Backend dequeues player
 * 4. Player profile fetched from HashTable
 * 5. AVL tree searched for closest ELO opponent
 * 6. Match is created between the two players
 * 7. Both players removed from queue
 * 8. Match ID returned to UI
 * 
 * DEMO MODE:
 * When queue size is 1 (only human), match Human vs Bot using AVL.findClosest()
 * 
 * Data Structures Used:
 *   - Queue<int>: FIFO matchmaking lobby per game
 *   - AVLTree<PlayerELO>: Rankings for O(log n) closest-match search
 *   - HashTable<int, Player>: Player profile storage
 *   - LinkedList<Match>: Match history storage
 */
class Matchmaker {
private:
    // One queue per game
    Queue<QueueEntry> pingpongQueue;
    Queue<QueueEntry> snakeQueue;
    Queue<QueueEntry> tankQueue;
    
    // Player storage and services
    HashTable<int, Player>* playerStorage;
    RankingService* rankingService;
    HistoryService* historyService;
    
    // Match tracking
    HashTable<int, Match> activeMatches;
    int nextMatchId;
    
    // Bot player IDs (per game)
    static const int MAX_BOTS_PER_GAME = 20;
    int pingpongBots[MAX_BOTS_PER_GAME];
    int snakeBots[MAX_BOTS_PER_GAME];
    int tankBots[MAX_BOTS_PER_GAME];
    int pingpongBotCount;
    int snakeBotCount;
    int tankBotCount;
    
    // Get queue for a specific game
    Queue<QueueEntry>* getQueueForGame(const char* gameName) {
        if (strcmp(gameName, "pingpong") == 0) return &pingpongQueue;
        if (strcmp(gameName, "snake") == 0) return &snakeQueue;
        if (strcmp(gameName, "tank") == 0) return &tankQueue;
        return nullptr;
    }
    
    // Get bot array for a specific game
    int* getBotsForGame(const char* gameName, int& count) {
        if (strcmp(gameName, "pingpong") == 0) { count = pingpongBotCount; return pingpongBots; }
        if (strcmp(gameName, "snake") == 0) { count = snakeBotCount; return snakeBots; }
        if (strcmp(gameName, "tank") == 0) { count = tankBotCount; return tankBots; }
        count = 0;
        return nullptr;
    }
    
    // Get current timestamp in milliseconds
    long long getCurrentTime() {
        return static_cast<long long>(time(nullptr));
    }

public:
    Matchmaker(HashTable<int, Player>* storage, RankingService* ranking, HistoryService* history)
        : playerStorage(storage), rankingService(ranking), 
          historyService(history), nextMatchId(1),
          pingpongBotCount(0), snakeBotCount(0), tankBotCount(0) {}
    
    /**
     * Register a bot for a specific game
     */
    void registerBot(int botId, const char* gameName) {
        if (strcmp(gameName, "pingpong") == 0 && pingpongBotCount < MAX_BOTS_PER_GAME) {
            pingpongBots[pingpongBotCount++] = botId;
        } else if (strcmp(gameName, "snake") == 0 && snakeBotCount < MAX_BOTS_PER_GAME) {
            snakeBots[snakeBotCount++] = botId;
        } else if (strcmp(gameName, "tank") == 0 && tankBotCount < MAX_BOTS_PER_GAME) {
            tankBots[tankBotCount++] = botId;
        }
    }
    
    /**
     * Add player to matchmaking queue for a game
     * 
     * @param playerId Player joining queue
     * @param gameName Game to queue for
     * @return true if successfully queued
     */
    bool joinQueue(int playerId, const char* gameName) {
        Player* player = playerStorage->get(playerId);
        if (!player) return false;
        
        // Check if already in queue or match
        if (player->isInQueue || player->isInMatch) {
            return false;
        }
        
        Queue<QueueEntry>* queue = getQueueForGame(gameName);
        if (!queue) return false;
        
        // Add to queue
        QueueEntry entry(playerId, getCurrentTime());
        queue->enqueue(entry);
        
        // Update player state
        player->isInQueue = true;
        player->setPreferredGame(gameName);
        playerStorage->update(playerId, *player);
        
        // Add to ranking tree for this game
        rankingService->addPlayerToRanking(playerId, gameName);
        
        return true;
    }
    
    /**
     * Remove player from matchmaking queue
     * 
     * @param playerId Player leaving queue
     * @param gameName Game queue to leave
     * @return true if successfully removed
     */
    bool leaveQueue(int playerId, const char* gameName) {
        Player* player = playerStorage->get(playerId);
        if (!player || !player->isInQueue) return false;
        
        Queue<QueueEntry>* queue = getQueueForGame(gameName);
        if (!queue) return false;
        
        // Remove from queue
        QueueEntry entry(playerId, 0);
        if (queue->remove(entry)) {
            player->isInQueue = false;
            playerStorage->update(playerId, *player);
            
            // Remove from ranking tree
            rankingService->removePlayerFromRanking(playerId, player->elo, gameName);
            return true;
        }
        
        return false;
    }
    
    /**
     * Try to create a match - CORE MATCHMAKING ALGORITHM
     * 
     * This is the main matchmaking logic using AVL tree for closest-rank search.
     * 
     * DEMO MODE ENHANCEMENT:
     * - If queue size is 1 and it's a human, match with closest-ELO bot
     * - If queue size >= 2, try Human vs Human first, fallback to bot
     * 
     * @param gameName Game to match for
     * @return Match ID if match created, -1 otherwise
     */
    int tryCreateMatch(const char* gameName) {
        Queue<QueueEntry>* queue = getQueueForGame(gameName);
        if (!queue || queue->isEmpty()) return -1;
        
        // Get bot count for this game
        int botCount = 0;
        int* bots = getBotsForGame(gameName, botCount);
        
        // CASE A: Queue size == 1 (single human) -> Match with Bot
        if (queue->size() == 1) {
            return matchHumanWithBot(gameName);
        }
        
        // CASE B: Queue size >= 2 -> Try Human vs Human first
        QueueEntry entry1;
        if (!queue->dequeue(entry1)) return -1;
        
        Player* player1 = playerStorage->get(entry1.playerId);
        if (!player1) return -1;
        
        // Check if player1 is a bot - if so, skip and try to find humans
        if (player1->isBot) {
            // Re-queue the bot and try again
            queue->enqueue(entry1);
            return -1;
        }
        
        // CRITICAL: Temporarily remove player1 from AVL tree to avoid self-matching
        rankingService->removePlayerFromRanking(entry1.playerId, player1->elo, gameName);
        
        // Find closest HUMAN opponent using AVL tree
        int opponentId = findClosestHumanOpponent(entry1.playerId, gameName);
        
        if (opponentId == -1) {
            // No human opponent found - match with bot instead
            rankingService->addPlayerToRanking(entry1.playerId, gameName);
            
            // Find closest bot (pass human player ID for recent opponent check)
            int botOpponentId = findClosestBotOpponent(entry1.playerId, player1->elo, gameName);
            if (botOpponentId == -1) {
                // No bot available - re-queue player
                queue->enqueue(entry1);
                return -1;
            }
            
            // Create match with bot
            return createMatchBetween(entry1.playerId, botOpponentId, gameName);
        }
        
        // Get human opponent
        Player* player2 = playerStorage->get(opponentId);
        if (!player2) {
            rankingService->addPlayerToRanking(entry1.playerId, gameName);
            queue->enqueue(entry1);
            return -1;
        }
        
        // Remove opponent from queue and tree
        QueueEntry opponentEntry(opponentId, 0);
        queue->remove(opponentEntry);
        rankingService->removePlayerFromRanking(opponentId, player2->elo, gameName);
        
        // Create match
        return createMatchBetween(entry1.playerId, opponentId, gameName);
    }
    
    /**
     * Match a human player with the closest-ELO bot (DEMO MODE)
     */
    int matchHumanWithBot(const char* gameName) {
        Queue<QueueEntry>* queue = getQueueForGame(gameName);
        if (!queue || queue->isEmpty()) return -1;
        
        // Dequeue the human player
        QueueEntry entry;
        if (!queue->dequeue(entry)) return -1;
        
        Player* human = playerStorage->get(entry.playerId);
        if (!human) return -1;
        
        // Bots should never be in queue, but check just in case
        if (human->isBot) {
            queue->enqueue(entry);
            return -1;
        }
        
        // Remove human from ranking tree temporarily
        rankingService->removePlayerFromRanking(entry.playerId, human->elo, gameName);
        
        // Find closest bot (pass human player ID for recent opponent check)
        int botId = findClosestBotOpponent(entry.playerId, human->elo, gameName);
        if (botId == -1) {
            // No bot available - re-add human to queue
            rankingService->addPlayerToRanking(entry.playerId, gameName);
            queue->enqueue(entry);
            return -1;
        }
        
        // Create match
        return createMatchBetween(entry.playerId, botId, gameName);
    }
    
    /**
     * Find the closest ELO human opponent (excludes bots)
     */
    int findClosestHumanOpponent(int playerId, const char* gameName) {
        int opponentId = rankingService->findClosestOpponent(playerId, gameName);
        if (opponentId == -1) return -1;
        
        Player* opponent = playerStorage->get(opponentId);
        if (!opponent) return -1;
        
        // Only return human opponents
        if (!opponent->isBot && opponent->isInQueue) {
            return opponentId;
        }
        return -1;
    }
    
    /**
     * Find the closest ELO bot opponent
     * 
     * ENHANCED: Skips bots that were recently matched with this player
     * to ensure opponent rotation and fair matchmaking.
     * 
     * Selection criteria (in order):
     * 1. Bot must not be in a match
     * 2. Bot must not be in player's recent opponent list
     * 3. Among eligible bots, select the closest ELO
     * 4. If all bots are recent, fallback to absolute closest (deadlock prevention)
     */
    int findClosestBotOpponent(int humanPlayerId, int targetElo, const char* gameName) {
        int botCount = 0;
        int* bots = getBotsForGame(gameName, botCount);
        if (!bots || botCount == 0) return -1;
        
        Player* human = playerStorage->get(humanPlayerId);
        if (!human) return -1;
        
        int bestBotId = -1;
        int bestEloDiff = 999999;
        
        int fallbackBotId = -1;  // Absolute closest for deadlock prevention
        int fallbackEloDiff = 999999;
        
        for (int i = 0; i < botCount; i++) {
            int botId = bots[i];
            Player* bot = playerStorage->get(botId);
            if (!bot || bot->isInMatch) continue;
            
            int eloDiff = bot->elo - targetElo;
            if (eloDiff < 0) eloDiff = -eloDiff;
            
            // Track absolute closest for fallback
            if (eloDiff < fallbackEloDiff) {
                fallbackEloDiff = eloDiff;
                fallbackBotId = botId;
            }
            
            // Skip if recently matched (opponent rotation)
            if (human->wasRecentOpponent(botId)) {
                continue;  // Don't match with same bot again
            }
            
            // Find best among eligible bots
            if (eloDiff < bestEloDiff) {
                bestEloDiff = eloDiff;
                bestBotId = botId;
            }
        }
        
        // If no eligible bot found (all recently matched), use fallback
        if (bestBotId == -1) {
            printf("[Matchmaker] All bots recently matched with player %d - using fallback\n", humanPlayerId);
            bestBotId = fallbackBotId;
        }
        
        return bestBotId;
    }
    
    /**
     * Create a match between two players (human or bot)
     * 
     * ENHANCED: Records opponent in recent history for rotation
     */
    int createMatchBetween(int player1Id, int player2Id, const char* gameName) {
        Player* player1 = playerStorage->get(player1Id);
        Player* player2 = playerStorage->get(player2Id);
        
        if (!player1 || !player2) return -1;
        
        // Record recent opponents for matchmaking rotation
        // Only track for human players (bots don't need rotation tracking)
        if (!player1->isBot) {
            player1->addRecentOpponent(player2Id);
            printf("[Matchmaker] Player %s matched with %s (ELO diff: %d)\n", 
                   player1->username, player2->username, 
                   player1->elo > player2->elo ? player1->elo - player2->elo : player2->elo - player1->elo);
        }
        if (!player2->isBot) {
            player2->addRecentOpponent(player1Id);
        }
        
        // Create match
        Match match(nextMatchId++, player1Id, player2Id, gameName);
        activeMatches.insert(match.matchId, match);
        
        // Update player states
        player1->isInQueue = false;
        player1->isInMatch = true;
        playerStorage->update(player1Id, *player1);
        
        player2->isInQueue = false;
        player2->isInMatch = true;
        playerStorage->update(player2Id, *player2);
        
        return match.matchId;
    }
    
    /**
     * Process matchmaking for a specific game
     * 
     * Should be called periodically to create matches.
     * 
     * @param gameName Game to process
     * @return Number of matches created
     */
    int processMatchmaking(const char* gameName) {
        int matchesCreated = 0;
        
        // Try to create matches while queue has 2+ players
        while (getQueueSize(gameName) >= 2) {
            int matchId = tryCreateMatch(gameName);
            if (matchId == -1) break;
            matchesCreated++;
        }
        
        return matchesCreated;
    }
    
    /**
     * Submit match result
     * 
     * @param matchId ID of the completed match
     * @param winnerId ID of the winning player
     * @return true if result recorded successfully
     */
    bool submitMatchResult(int matchId, int winnerId) {
        Match* match = activeMatches.get(matchId);
        if (!match || match->isCompleted) return false;
        
        // Validate winner is part of the match
        if (winnerId != match->player1Id && winnerId != match->player2Id) {
            return false;
        }
        
        // Complete the match
        int loserId = (winnerId == match->player1Id) ? match->player2Id : match->player1Id;
        match->complete(winnerId);
        
        // Update rankings (this handles ELO calculation)
        rankingService->updateRankings(winnerId, loserId, match->gameName);
        
        // Record to history
        historyService->recordMatch(*match);
        
        // Update player states
        Player* winner = playerStorage->get(winnerId);
        Player* loser = playerStorage->get(loserId);
        
        if (winner) {
            winner->isInMatch = false;
            playerStorage->update(winnerId, *winner);
        }
        
        if (loser) {
            loser->isInMatch = false;
            playerStorage->update(loserId, *loser);
        }
        
        // Re-add players to ranking trees for future matchmaking
        rankingService->addPlayerToRanking(winnerId, match->gameName);
        rankingService->addPlayerToRanking(loserId, match->gameName);
        
        return true;
    }
    
    /**
     * Get match by ID
     */
    Match* getMatch(int matchId) {
        return activeMatches.get(matchId);
    }
    
    /**
     * Get queue size for a game
     */
    size_t getQueueSize(const char* gameName) {
        Queue<QueueEntry>* queue = getQueueForGame(gameName);
        return queue ? queue->size() : 0;
    }
    
    /**
     * Check if player is in queue
     */
    bool isPlayerInQueue(int playerId) {
        Player* player = playerStorage->get(playerId);
        return player && player->isInQueue;
    }
    
    /**
     * Check if player is in active match
     */
    bool isPlayerInMatch(int playerId) {
        Player* player = playerStorage->get(playerId);
        return player && player->isInMatch;
    }
    
    /**
     * Get active match for a player
     */
    int getPlayerActiveMatch(int playerId) {
        // Search through active matches
        // Note: This is O(n) - could be optimized with another hash table
        int keys[1000];
        size_t keyCount;
        activeMatches.getAllKeys(keys, keyCount);
        
        for (size_t i = 0; i < keyCount; i++) {
            Match* match = activeMatches.get(keys[i]);
            if (match && !match->isCompleted) {
                if (match->player1Id == playerId || match->player2Id == playerId) {
                    return match->matchId;
                }
            }
        }
        return -1;
    }
};

#endif // MATCHMAKER_H
