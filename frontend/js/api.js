/**
 * API Communication Module
 * Handles all communication with the C++ backend and WebSocket Bridge
 */

// Use relative path so it works both locally and when deployed
const API_BASE = '/api';

// WebSocket connection
let gameSocket = null;
let onGameStateCallback = null;
let onPlayerJoinedCallback = null;
let onPlayerLeftCallback = null;

const api = {
    // ==================== WebSocket Methods ====================

    connectGameSocket(matchId, playerId) {
        // Close existing connection
        if (gameSocket) {
            gameSocket.close();
        }

        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}`;

        console.log('Connecting to WebSocket:', wsUrl);
        gameSocket = new WebSocket(wsUrl);

        gameSocket.onopen = () => {
            console.log('WebSocket Connected');
            // Join the specific match room
            this.send({
                type: 'JOIN_MATCH',
                matchId: matchId,
                playerId: playerId
            });
        };

        gameSocket.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);

                if (data.type === 'GAME_STATE' || data.type === 'SCORE_UPDATE') {
                    if (onGameStateCallback) onGameStateCallback(data);
                } else if (data.type === 'PLAYER_JOINED') {
                    if (onPlayerJoinedCallback) onPlayerJoinedCallback();
                } else if (data.type === 'PLAYER_LEFT') {
                    if (onPlayerLeftCallback) onPlayerLeftCallback();
                }
            } catch (err) {
                console.error('Error parsing WS message:', err);
            }
        };

        gameSocket.onclose = () => {
            console.log('WebSocket Disconnected');
        };

        gameSocket.onerror = (err) => {
            console.error('WebSocket Error:', err);
        };
    },

    setGameCallbacks(onGameState, onPlayerJoined, onPlayerLeft) {
        onGameStateCallback = onGameState;
        onPlayerJoinedCallback = onPlayerJoined;
        onPlayerLeftCallback = onPlayerLeft;
    },

    sendGameState(data) {
        if (gameSocket && gameSocket.readyState === WebSocket.OPEN) {
            this.send({
                type: 'GAME_STATE',
                ...data
            });
        }
    },

    sendScoreUpdate(score1, score2) {
        if (gameSocket && gameSocket.readyState === WebSocket.OPEN) {
            this.send({
                type: 'SCORE_UPDATE',
                score1,
                score2
            });
        }
    },

    send(data) {
        if (gameSocket && gameSocket.readyState === WebSocket.OPEN) {
            gameSocket.send(JSON.stringify(data));
        }
    },

    disconnectSocket() {
        if (gameSocket) {
            gameSocket.close();
            gameSocket = null;
        }
        onGameStateCallback = null;
        onPlayerJoinedCallback = null;
        onPlayerLeftCallback = null;
    },

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
