#ifndef MATCH_H
#define MATCH_H

#include <cstring>
#include <ctime>

/**
 * Match - Represents a completed or ongoing match
 * 
 * Stored in LinkedList<Match> for player match history
 */
struct Match {
    int matchId;
    int player1Id;
    int player2Id;
    char gameName[20];  // "pingpong", "snake", "tank"
    int winnerId;       // 0 if match not finished
    char timestamp[30];
    bool isCompleted;
    
    // Default constructor
    Match() : matchId(0), player1Id(0), player2Id(0), winnerId(0), isCompleted(false) {
        gameName[0] = '\0';
        timestamp[0] = '\0';
    }
    
    // Parameterized constructor
    Match(int id, int p1, int p2, const char* game) 
        : matchId(id), player1Id(p1), player2Id(p2), winnerId(0), isCompleted(false) {
        strncpy(gameName, game, 19);
        gameName[19] = '\0';
        setCurrentTimestamp();
    }
    
    // Set current timestamp
    void setCurrentTimestamp() {
        time_t now = time(nullptr);
        struct tm* localTime = localtime(&now);
        strftime(timestamp, 30, "%Y-%m-%d %H:%M:%S", localTime);
    }
    
    // Set winner and complete the match
    void complete(int winner) {
        winnerId = winner;
        isCompleted = true;
    }
    
    // Get the opponent ID for a given player
    int getOpponentId(int playerId) const {
        if (playerId == player1Id) return player2Id;
        if (playerId == player2Id) return player1Id;
        return 0;
    }
    
    // Check if player won
    bool didPlayerWin(int playerId) const {
        return winnerId == playerId;
    }
    
    // Comparison for LinkedList operations
    bool operator==(const Match& other) const {
        return matchId == other.matchId;
    }
};

/**
 * MatchHistoryEntry - Simplified version for player history display
 */
struct MatchHistoryEntry {
    int matchId;
    int opponentId;
    char gameName[20];
    bool won;
    char timestamp[30];
    
    MatchHistoryEntry() : matchId(0), opponentId(0), won(false) {
        gameName[0] = '\0';
        timestamp[0] = '\0';
    }
    
    MatchHistoryEntry(const Match& match, int forPlayerId) {
        matchId = match.matchId;
        opponentId = match.getOpponentId(forPlayerId);
        strncpy(gameName, match.gameName, 19);
        gameName[19] = '\0';
        won = match.didPlayerWin(forPlayerId);
        strncpy(timestamp, match.timestamp, 29);
        timestamp[29] = '\0';
    }
    
    bool operator==(const MatchHistoryEntry& other) const {
        return matchId == other.matchId;
    }
};

#endif // MATCH_H
