#ifndef HISTORY_SERVICE_H
#define HISTORY_SERVICE_H

#include "../ds/HashTable.h"
#include "../ds/LinkedList.h"
#include "../models/Match.h"
#include "../models/Player.h"

/**
 * HistoryService - Manages match history for all players
 * 
 * Uses HashTable<int, LinkedList<Match>> for O(1) access to player histories.
 * Each player has a LinkedList of their matches for chronological storage.
 * 
 * Operations:
 *   - Add match to history: O(1)
 *   - Get last N matches: O(n)
 */
class HistoryService {
private:
    // Maps playerID -> LinkedList of their matches
    HashTable<int, LinkedList<Match>> playerHistories;
    
public:
    HistoryService() {}
    
    /**
     * Record a match for both players
     */
    void recordMatch(const Match& match) {
        // Add to player 1's history
        LinkedList<Match>* p1History = playerHistories.get(match.player1Id);
        if (!p1History) {
            playerHistories.insert(match.player1Id, LinkedList<Match>());
            p1History = playerHistories.get(match.player1Id);
        }
        p1History->append(match);
        
        // Add to player 2's history
        LinkedList<Match>* p2History = playerHistories.get(match.player2Id);
        if (!p2History) {
            playerHistories.insert(match.player2Id, LinkedList<Match>());
            p2History = playerHistories.get(match.player2Id);
        }
        p2History->append(match);
    }
    
    /**
     * Get a player's match history
     * 
     * @param playerId Player to get history for
     * @return Pointer to their match list, or nullptr if none
     */
    LinkedList<Match>* getPlayerHistory(int playerId) {
        return playerHistories.get(playerId);
    }
    
    /**
     * Get last N matches for a player
     * 
     * @param playerId Player to get history for
     * @param n Number of recent matches to retrieve
     * @param outMatches Array to store matches (caller provides)
     * @param outCount Number of matches retrieved
     */
    void getLastNMatches(int playerId, int n, Match* outMatches, int& outCount) {
        outCount = 0;
        LinkedList<Match>* history = playerHistories.get(playerId);
        if (!history) return;
        
        LinkedList<Match> lastN = history->getLastN(n);
        for (auto it = lastN.begin(); it != lastN.end(); ++it) {
            outMatches[outCount++] = *it;
        }
    }
    
    /**
     * Get match count for a player
     */
    int getMatchCount(int playerId) {
        LinkedList<Match>* history = playerHistories.get(playerId);
        return history ? static_cast<int>(history->size()) : 0;
    }
    
    /**
     * Clear a player's history
     */
    void clearPlayerHistory(int playerId) {
        LinkedList<Match>* history = playerHistories.get(playerId);
        if (history) {
            history->clear();
        }
    }
};

#endif // HISTORY_SERVICE_H
