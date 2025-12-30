#ifndef PLAYER_H
#define PLAYER_H

#include <cstring>
#include <cstdio>

/**
 * Player - Represents a player profile in the system
 * 
 * Stored in HashTable<int, Player> with PlayerID as key
 * Also used in AVL tree for ranking (via PlayerELO wrapper)
 */
struct Player {
    int id;
    char username[50];
    int elo;
    int wins;
    int losses;
    char preferredGame[20];  // "pingpong", "snake", "tank"
    bool isInQueue;
    bool isInMatch;
    bool isBot;  // Flag to identify bot players
    
    // Recent opponent tracking for matchmaking rotation
    static const int MAX_RECENT_OPPONENTS = 3;
    int recentOpponents[MAX_RECENT_OPPONENTS];
    int recentOpponentCount;
    
    // Default constructor
    Player() : id(0), elo(1000), wins(0), losses(0), isInQueue(false), isInMatch(false), isBot(false), recentOpponentCount(0) {
        username[0] = '\0';
        preferredGame[0] = '\0';
        for (int i = 0; i < MAX_RECENT_OPPONENTS; i++) {
            recentOpponents[i] = -1;
        }
    }
    
    // Parameterized constructor
    Player(int playerId, const char* name, int startingElo = 1000, bool bot = false) 
        : id(playerId), elo(startingElo), wins(0), losses(0), 
          isInQueue(false), isInMatch(false), isBot(bot), recentOpponentCount(0) {
        strncpy(username, name, 49);
        username[49] = '\0';
        preferredGame[0] = '\0';
        for (int i = 0; i < MAX_RECENT_OPPONENTS; i++) {
            recentOpponents[i] = -1;
        }
    }
    
    // Add an opponent to recent history (circular buffer)
    void addRecentOpponent(int opponentId) {
        // Shift existing entries
        for (int i = MAX_RECENT_OPPONENTS - 1; i > 0; i--) {
            recentOpponents[i] = recentOpponents[i - 1];
        }
        recentOpponents[0] = opponentId;
        if (recentOpponentCount < MAX_RECENT_OPPONENTS) {
            recentOpponentCount++;
        }
    }
    
    // Check if opponent was recently matched
    bool wasRecentOpponent(int opponentId) const {
        for (int i = 0; i < recentOpponentCount; i++) {
            if (recentOpponents[i] == opponentId) {
                return true;
            }
        }
        return false;
    }
    
    // Get total matches played
    int getTotalMatches() const {
        return wins + losses;
    }
    
    // Get win rate as percentage
    float getWinRate() const {
        int total = getTotalMatches();
        return total > 0 ? (static_cast<float>(wins) / total) * 100.0f : 0.0f;
    }
    
    // Set preferred game
    void setPreferredGame(const char* game) {
        strncpy(preferredGame, game, 19);
        preferredGame[19] = '\0';
    }
    
    // Comparison operators for hashing
    bool operator==(const Player& other) const {
        return id == other.id;
    }
    
    bool operator!=(const Player& other) const {
        return id != other.id;
    }
};

/**
 * PlayerELO - Wrapper for AVL tree storage
 * 
 * Combines ELO and PlayerID for proper ordering in the ranking tree.
 * Primary sort: ELO (descending for leaderboard)
 * Secondary sort: PlayerID (for tie-breaking)
 */
struct PlayerELO {
    int elo;
    int playerId;
    
    PlayerELO() : elo(0), playerId(0) {}
    PlayerELO(int e, int id) : elo(e), playerId(id) {}
    
    // For AVL tree comparison - sort by ELO, then by ID
    bool operator<(const PlayerELO& other) const {
        if (elo != other.elo) {
            return elo < other.elo;
        }
        return playerId < other.playerId;
    }
    
    bool operator==(const PlayerELO& other) const {
        return elo == other.elo && playerId == other.playerId;
    }
    
    // For findClosest - difference calculation
    int operator-(const PlayerELO& other) const {
        return elo - other.elo;
    }
};

/**
 * QueueEntry - Entry in matchmaking queue
 */
struct QueueEntry {
    int playerId;
    long long joinTime;  // Timestamp when joined queue
    
    QueueEntry() : playerId(0), joinTime(0) {}
    QueueEntry(int id, long long time) : playerId(id), joinTime(time) {}
    
    bool operator==(const QueueEntry& other) const {
        return playerId == other.playerId;
    }
};

#endif // PLAYER_H
