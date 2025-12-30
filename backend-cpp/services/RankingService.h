#ifndef RANKING_SERVICE_H
#define RANKING_SERVICE_H

#include "../ds/AVLTree.h"
#include "../ds/HashTable.h"
#include "../models/Player.h"
#include <cmath>

/**
 * RankingService - Manages player rankings per game
 * 
 * Uses AVL trees for O(log n) ranking operations:
 *   - Insert/update player rankings
 *   - Generate leaderboards via in-order traversal
 *   - Find closest-ranked player for matchmaking
 * 
 * ELO calculation based on standard K-factor formula.
 */
class RankingService {
private:
    // One AVL tree per game for rankings
    AVLTree<PlayerELO> pingpongRankings;
    AVLTree<PlayerELO> snakeRankings;
    AVLTree<PlayerELO> tankRankings;
    
    // Reference to player storage
    HashTable<int, Player>* playerStorage;
    
    // K-factor for ELO calculation
    static const int K_FACTOR = 32;
    
    // Get the appropriate tree for a game
    AVLTree<PlayerELO>* getTreeForGame(const char* gameName) {
        if (strcmp(gameName, "pingpong") == 0) return &pingpongRankings;
        if (strcmp(gameName, "snake") == 0) return &snakeRankings;
        if (strcmp(gameName, "tank") == 0) return &tankRankings;
        return nullptr;
    }
    
    // Calculate expected score (probability of winning)
    float calculateExpectedScore(int playerElo, int opponentElo) {
        return 1.0f / (1.0f + pow(10.0f, (opponentElo - playerElo) / 400.0f));
    }
    
    // Calculate new ELO after a match
    int calculateNewElo(int currentElo, float expectedScore, float actualScore) {
        return currentElo + static_cast<int>(K_FACTOR * (actualScore - expectedScore));
    }

public:
    RankingService(HashTable<int, Player>* storage) : playerStorage(storage) {}
    
    /**
     * Add player to a game's ranking tree
     */
    void addPlayerToRanking(int playerId, const char* gameName) {
        Player* player = playerStorage->get(playerId);
        if (!player) return;
        
        AVLTree<PlayerELO>* tree = getTreeForGame(gameName);
        if (!tree) return;
        
        PlayerELO entry(player->elo, playerId);
        tree->insert(entry);
    }
    
    /**
     * Remove player from a game's ranking tree
     */
    void removePlayerFromRanking(int playerId, int elo, const char* gameName) {
        AVLTree<PlayerELO>* tree = getTreeForGame(gameName);
        if (!tree) return;
        
        PlayerELO entry(elo, playerId);
        tree->remove(entry);
    }
    
    /**
     * Update rankings after a match
     * 
     * @param winnerId ID of the winning player
     * @param loserId ID of the losing player
     * @param gameName Name of the game
     */
    void updateRankings(int winnerId, int loserId, const char* gameName) {
        Player* winner = playerStorage->get(winnerId);
        Player* loser = playerStorage->get(loserId);
        
        if (!winner || !loser) return;
        
        AVLTree<PlayerELO>* tree = getTreeForGame(gameName);
        if (!tree) return;
        
        // Store old ELOs for removal
        int winnerOldElo = winner->elo;
        int loserOldElo = loser->elo;
        
        // Remove old entries from AVL tree
        PlayerELO winnerOld(winnerOldElo, winnerId);
        PlayerELO loserOld(loserOldElo, loserId);
        tree->remove(winnerOld);
        tree->remove(loserOld);
        
        // Calculate new ELOs
        float winnerExpected = calculateExpectedScore(winnerOldElo, loserOldElo);
        float loserExpected = calculateExpectedScore(loserOldElo, winnerOldElo);
        
        winner->elo = calculateNewElo(winnerOldElo, winnerExpected, 1.0f);
        loser->elo = calculateNewElo(loserOldElo, loserExpected, 0.0f);
        
        // Update win/loss counts
        winner->wins++;
        loser->losses++;
        
        // Reinsert with new ELOs
        PlayerELO winnerNew(winner->elo, winnerId);
        PlayerELO loserNew(loser->elo, loserId);
        tree->insert(winnerNew);
        tree->insert(loserNew);
        
        // Update player storage
        playerStorage->update(winnerId, *winner);
        playerStorage->update(loserId, *loser);
    }
    
    /**
     * Get leaderboard for a game
     * 
     * Uses reverse in-order traversal to get players sorted by ELO descending.
     * 
     * @param gameName Name of the game
     * @param outPlayers Array to store player IDs
     * @param outElos Array to store player ELOs
     * @param maxCount Maximum number of entries to return
     * @return Actual number of entries returned
     */
    int getLeaderboard(const char* gameName, int* outPlayerIds, int* outElos, int maxCount) {
        AVLTree<PlayerELO>* tree = getTreeForGame(gameName);
        if (!tree) return 0;
        
        struct LeaderboardData {
            int* playerIds;
            int* elos;
            int count;
            int maxCount;
        };
        
        LeaderboardData data = {outPlayerIds, outElos, 0, maxCount};
        
        tree->reverseInOrderTraversal([&data](const PlayerELO& entry) {
            if (data.count < data.maxCount) {
                data.playerIds[data.count] = entry.playerId;
                data.elos[data.count] = entry.elo;
                data.count++;
            }
        });
        
        return data.count;
    }
    
    /**
     * Find closest-ranked player for matchmaking
     * 
     * CRITICAL: This is the core matchmaking algorithm.
     * Uses AVL tree's findClosest for O(log n) performance.
     * 
     * @param playerId Player looking for a match
     * @param gameName Game to match for
     * @return ID of closest-ranked opponent, or -1 if none found
     */
    int findClosestOpponent(int playerId, const char* gameName) {
        Player* player = playerStorage->get(playerId);
        if (!player) return -1;
        
        AVLTree<PlayerELO>* tree = getTreeForGame(gameName);
        if (!tree || tree->size() < 2) return -1;
        
        PlayerELO target(player->elo, playerId);
        PlayerELO* closest = tree->findClosestExcluding(target, target);
        
        return closest ? closest->playerId : -1;
    }
    
    /**
     * Get ranking tree size for a game
     */
    size_t getRankingCount(const char* gameName) {
        AVLTree<PlayerELO>* tree = getTreeForGame(gameName);
        return tree ? tree->size() : 0;
    }
};

#endif // RANKING_SERVICE_H
