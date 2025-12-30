/**
 * API Communication Module
 * Handles all communication with the C++ backend
 */

// Use relative path so it works both locally and when deployed
const API_BASE = '/api';

const api = {
    // ==================== Player Endpoints ====================

    async registerPlayer(username, elo = 1000) {
        const response = await fetch(`${API_BASE}/players`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, elo })
        });
        return response.json();
    },

    async getPlayer(playerId) {
        const response = await fetch(`${API_BASE}/players/${playerId}`);
        if (!response.ok) return null;
        return response.json();
    },

    // ==================== Matchmaking Endpoints ====================

    async joinMatchmaking(playerId, game) {
        const response = await fetch(`${API_BASE}/matchmaking/join`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ playerId: playerId.toString(), game })
        });
        return response.json();
    },

    async leaveMatchmaking(playerId, game) {
        const response = await fetch(`${API_BASE}/matchmaking/leave`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ playerId: playerId.toString(), game })
        });
        return response.json();
    },

    async getMatchmakingStatus(playerId) {
        const response = await fetch(`${API_BASE}/matchmaking/status/${playerId}`);
        return response.json();
    },

    async processMatchmaking(game) {
        const response = await fetch(`${API_BASE}/matchmaking/process`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ game })
        });
        return response.json();
    },

    // ==================== Match Endpoints ====================

    async getMatch(matchId) {
        const response = await fetch(`${API_BASE}/matches/${matchId}`);
        if (!response.ok) return null;
        return response.json();
    },

    async submitMatchResult(matchId, winnerId) {
        const response = await fetch(`${API_BASE}/matches/result`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                matchId: matchId.toString(),
                winnerId: winnerId.toString()
            })
        });
        return response.json();
    },

    // ==================== Leaderboard Endpoints ====================

    async getLeaderboard(game) {
        const response = await fetch(`${API_BASE}/leaderboard/${game}`);
        return response.json();
    },

    // ==================== History Endpoints ====================

    async getHistory(playerId) {
        const response = await fetch(`${API_BASE}/history/${playerId}`);
        return response.json();
    },

    // ==================== Utility Endpoints ====================

    async getQueueStatus() {
        const response = await fetch(`${API_BASE}/queues`);
        return response.json();
    },

    async healthCheck() {
        try {
            const response = await fetch(`${API_BASE}/health`);
            return response.ok;
        } catch {
            return false;
        }
    },

    async logout(playerId) {
        const response = await fetch(`${API_BASE}/logout`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ playerId: playerId.toString() })
        });
        return response.json();
    }
};

// Export for use in other modules
window.api = api;
