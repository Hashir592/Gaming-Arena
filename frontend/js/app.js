/**
 * Main Application Module
 * Handles UI state, navigation, and game flow
 */

// Application State
const appState = {
    currentPlayer: null,
    currentMatch: null,
    currentGame: null,
    isInQueue: false,
    pollInterval: null,
    isOpponentBot: false  // Track if opponent is a bot for AI control
};

// ==================== Initialization ====================

document.addEventListener('DOMContentLoaded', async () => {
    // Check if backend is available
    const isOnline = await api.healthCheck();
    if (!isOnline) {
        showToast('Backend server not running. Start the C++ server first.', 'error');
    }

    // Setup navigation
    setupNavigation();

    // Load saved player or show auth modal
    const savedPlayer = localStorage.getItem('playerId');
    if (savedPlayer) {
        await loadPlayer(parseInt(savedPlayer));
    }

    // Start queue status polling
    startQueuePolling();
});

// ==================== Navigation ====================

function setupNavigation() {
    document.querySelectorAll('.nav-link').forEach(link => {
        link.addEventListener('click', (e) => {
            e.preventDefault();
            const view = link.dataset.view;
            switchView(view);

            // Update active state
            document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));
            link.classList.add('active');
        });
    });
}

function switchView(viewName) {
    document.querySelectorAll('.view').forEach(view => view.classList.remove('active'));
    const targetView = document.getElementById(`${viewName}View`);
    if (targetView) {
        targetView.classList.add('active');

        // Load data for specific views
        if (viewName === 'dashboard' && appState.currentPlayer) {
            loadDashboard();
        } else if (viewName === 'leaderboard') {
            loadLeaderboard('pingpong');
        }
    }
}

// ==================== Player Management ====================

async function registerPlayer() {
    const nameInput = document.getElementById('playerNameInput');
    const username = nameInput.value.trim();

    if (!username) {
        showToast('Please enter a username', 'error');
        return;
    }

    try {
        const player = await api.registerPlayer(username);
        appState.currentPlayer = player;
        localStorage.setItem('playerId', player.id);

        updateUserDisplay();
        hideModal('authModal');
        showToast(`Welcome, ${player.username}!`, 'success');
    } catch (error) {
        showToast('Failed to register. Is the server running?', 'error');
    }
}

async function loadPlayer(playerId) {
    try {
        const player = await api.getPlayer(playerId);
        if (player) {
            appState.currentPlayer = player;
            updateUserDisplay();
            hideModal('authModal');
        }
    } catch (error) {
        localStorage.removeItem('playerId');
    }
}

function updateUserDisplay() {
    if (appState.currentPlayer) {
        document.getElementById('userName').textContent = appState.currentPlayer.username;
        document.getElementById('userElo').textContent = appState.currentPlayer.elo;
        // Show logout button when logged in
        document.getElementById('logoutBtn').style.display = 'inline-block';
    } else {
        document.getElementById('logoutBtn').style.display = 'none';
    }
}

// ==================== Matchmaking ====================

async function joinMatchmaking(game) {
    if (!appState.currentPlayer) {
        showToast('Please register first', 'error');
        return;
    }

    if (appState.isInQueue) {
        showToast('Already in queue', 'error');
        return;
    }

    showLoading('Finding Match...');
    appState.currentGame = game;

    try {
        const result = await api.joinMatchmaking(appState.currentPlayer.id, game);

        if (result.matched) {
            // Match found immediately
            hideLoading();
            appState.currentMatch = await api.getMatch(result.matchId);
            showMatchFoundModal();
        } else if (result.queued) {
            // Waiting in queue
            appState.isInQueue = true;
            startMatchPolling(game);
        } else {
            hideLoading();
            showToast(result.error || 'Failed to join queue', 'error');
        }
    } catch (error) {
        hideLoading();
        showToast('Connection error. Is the server running?', 'error');
    }
}

function startMatchPolling(game) {
    if (appState.pollInterval) {
        clearInterval(appState.pollInterval);
    }

    appState.pollInterval = setInterval(async () => {
        try {
            const status = await api.getMatchmakingStatus(appState.currentPlayer.id);

            if (status.isInMatch && status.activeMatchId > 0) {
                // Match found!
                clearInterval(appState.pollInterval);
                appState.isInQueue = false;
                hideLoading();

                appState.currentMatch = await api.getMatch(status.activeMatchId);
                showMatchFoundModal();
            }
        } catch (error) {
            console.error('Polling error:', error);
        }
    }, 1000);

    // Timeout after 30 seconds
    setTimeout(() => {
        if (appState.isInQueue) {
            clearInterval(appState.pollInterval);
            appState.isInQueue = false;
            hideLoading();
            api.leaveMatchmaking(appState.currentPlayer.id, game);
            showToast('No match found. Try again later.', 'error');
        }
    }, 30000);
}

async function showMatchFoundModal() {
    if (!appState.currentMatch) return;

    const match = appState.currentMatch;
    const isPlayer1 = match.player1Id === appState.currentPlayer.id;

    // Determine opponent
    const opponentId = isPlayer1 ? match.player2Id : match.player1Id;
    const opponentName = isPlayer1 ? match.player2Name : match.player1Name;

    // Check if opponent is a bot
    const opponent = await api.getPlayer(opponentId);
    const isOpponentBot = opponent && opponent.isBot;

    // Store bot status for game initialization
    appState.isOpponentBot = isOpponentBot;

    // Update player 1 display (always the human in this view)
    document.getElementById('matchPlayer1').textContent = appState.currentPlayer.username;
    document.getElementById('matchPlayer1Elo').textContent = appState.currentPlayer.elo;

    // Update player 2 display (opponent)
    document.getElementById('matchPlayer2').textContent = opponentName;
    document.getElementById('matchPlayer2Elo').textContent = opponent ? opponent.elo : '???';

    // Show/hide bot badge
    const botBadge = document.getElementById('opponentBotBadge');
    if (botBadge) {
        if (isOpponentBot) {
            botBadge.classList.remove('hidden');
        } else {
            botBadge.classList.add('hidden');
        }
    }

    // Connect to Game Socket IMMEDIATELY when match is found
    // This gives time for connection to establish during countdown
    if (!isOpponentBot) {
        api.connectGameSocket(match.matchId, appState.currentPlayer.id);
    }

    showModal('matchModal');

    // Auto-start countdown
    const matchBtn = document.getElementById('startMatchBtn');
    if (matchBtn) matchBtn.style.display = 'none'; // Hide manual button if present (removed in HTML)

    const countdownEl = document.createElement('div');
    countdownEl.id = 'matchCountdown';
    countdownEl.className = 'match-countdown';
    countdownEl.style.fontSize = '48px';
    countdownEl.style.fontWeight = 'bold';
    countdownEl.style.color = '#22c55e';
    countdownEl.style.margin = '20px 0';
    countdownEl.textContent = 'Starting in 3...';

    const modalContent = document.querySelector('#matchModal .modal-content');
    const existing = document.getElementById('matchCountdown');
    if (existing) existing.remove();
    modalContent.appendChild(countdownEl);

    let count = 3;
    const interval = setInterval(() => {
        count--;
        if (count > 0) {
            countdownEl.textContent = `Starting in ${count}...`;
        } else {
            clearInterval(interval);
            countdownEl.textContent = 'GO!';
            setTimeout(() => {
                startGame();
            }, 500);
        }
    }, 1000);
}

// ==================== Game Management ====================

function startGame() {
    hideModal('matchModal');

    // Switch to game view
    document.querySelectorAll('.view').forEach(v => v.classList.remove('active'));
    document.getElementById('gameView').classList.add('active');

    // Set game title
    const gameNames = {
        'pingpong': '🏓 Ping Pong',
        'snake': '🐍 Snake Battle',
        'tank': '🚓 Tank Battle'
    };
    document.getElementById('currentGameTitle').textContent =
        gameNames[appState.currentGame] || 'Game';

    // Initialize game based on type
    const canvas = document.getElementById('gameCanvas');
    const isOpponentBot = appState.isOpponentBot;

    // Multiplayer settings
    let matchId = null;
    let isPlayer1 = true;
    let playerId = appState.currentPlayer ? appState.currentPlayer.id : null;

    if (appState.currentMatch && !isOpponentBot) {
        matchId = appState.currentMatch.matchId;
        isPlayer1 = appState.currentMatch.player1Id === appState.currentPlayer.id;

        // Socket connection handled in showMatchFoundModal
    }

    console.log(`Starting Game: Bot=${isOpponentBot}, Match=${matchId}, P1=${isPlayer1}`);

    switch (appState.currentGame) {
        case 'pingpong':
            initPingPong(canvas, onGameEnd, isOpponentBot, matchId, isPlayer1, playerId);
            break;
        case 'snake':
            initSnake(canvas, onGameEnd, isOpponentBot); // Snake needs sync later
            break;
        case 'tank':
            initTank(canvas, onGameEnd, isOpponentBot); // Tank needs sync later
            break;
    }
}

async function onGameEnd(winner) {
    // Disconnect socket when game ends
    api.disconnectSocket();

    // Winner: 1 = player 1, 2 = player 2
    const match = appState.currentMatch;
    const winnerId = winner === 1 ? match.player1Id : match.player2Id;
    const playerWon = winnerId === appState.currentPlayer.id;

    try {
        const result = await api.submitMatchResult(match.matchId, winnerId);

        // Update local player data
        const oldElo = appState.currentPlayer.elo;
        const newPlayer = await api.getPlayer(appState.currentPlayer.id);
        appState.currentPlayer = newPlayer;
        updateUserDisplay();

        // Show result modal
        const eloChange = newPlayer.elo - oldElo;
        showResultModal(playerWon, eloChange);
    } catch (error) {
        showToast('Failed to submit result', 'error');
    }
}

function showResultModal(won, eloChange) {
    document.getElementById('resultTitle').textContent = won ? '🏆 Victory!' : '💔 Defeat';

    const eloChangeEl = document.getElementById('eloChange');
    eloChangeEl.textContent = (eloChange >= 0 ? '+' : '') + eloChange + ' ELO';
    eloChangeEl.className = 'elo-change ' + (eloChange >= 0 ? 'positive' : 'negative');

    showModal('resultModal');
}

function closeResultModal() {
    hideModal('resultModal');
    appState.currentMatch = null;
    appState.currentGame = null;
    switchView('games');

    // Update nav
    document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));
    document.querySelector('[data-view="games"]').classList.add('active');
}

function exitGame() {
    // Stop any running game
    if (window.currentGameLoop) {
        cancelAnimationFrame(window.currentGameLoop);
    }

    // Stop network sync
    if (pingPongGame) {
        pingPongGame.stop();
    }

    api.disconnectSocket();

    closeResultModal();
}

// ==================== Dashboard ====================

async function loadDashboard() {
    if (!appState.currentPlayer) return;

    // Refresh player data
    const player = await api.getPlayer(appState.currentPlayer.id);
    if (player) {
        appState.currentPlayer = player;
        updateUserDisplay();

        document.getElementById('dashElo').textContent = player.elo;
        document.getElementById('dashWins').textContent = player.wins;
        document.getElementById('dashLosses').textContent = player.losses;
        document.getElementById('dashWinRate').textContent = player.winRate.toFixed(1) + '%';
    }

    // Load match history
    const history = await api.getHistory(appState.currentPlayer.id);
    renderMatchHistory(history.matches || []);
}

function renderMatchHistory(matches) {
    const container = document.getElementById('matchHistory');

    if (!matches || matches.length === 0) {
        container.innerHTML = '<div class="no-matches">No matches played yet</div>';
        return;
    }

    const gameIcons = {
        'pingpong': '🏓',
        'snake': '🐍',
        'tank': '🚓'
    };

    container.innerHTML = matches.map(match => `
        <div class="history-item ${match.won ? 'win' : 'loss'}">
            <span class="history-game">${gameIcons[match.game] || '🎮'}</span>
            <span class="history-opponent">vs ${match.opponentName}</span>
            <span class="history-result ${match.won ? 'win' : 'loss'}">${match.won ? 'WIN' : 'LOSS'}</span>
        </div>
    `).join('');
}

// ==================== Leaderboard ====================

async function loadLeaderboard(game) {
    // Update active tab
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.classList.toggle('active', btn.dataset.game === game);
    });

    const container = document.getElementById('leaderboardBody');
    container.innerHTML = '<div class="no-players">Loading...</div>';

    try {
        const data = await api.getLeaderboard(game);
        renderLeaderboard(data.leaderboard || []);
    } catch (error) {
        container.innerHTML = '<div class="no-players">Failed to load leaderboard</div>';
    }
}

function renderLeaderboard(players) {
    const container = document.getElementById('leaderboardBody');

    if (!players || players.length === 0) {
        container.innerHTML = '<div class="no-players">No players ranked yet</div>';
        return;
    }

    container.innerHTML = players.map((player, index) => {
        const rank = index + 1;
        let rankClass = 'rank-other';
        if (rank === 1) rankClass = 'rank-1';
        else if (rank === 2) rankClass = 'rank-2';
        else if (rank === 3) rankClass = 'rank-3';

        return `
            <div class="leaderboard-row">
                <div class="col-rank">
                    <span class="rank-badge ${rankClass}">${rank}</span>
                </div>
                <div class="col-player">
                    <div class="player-info">
                        <span class="player-name">${player.username}</span>
                    </div>
                </div>
                <div class="col-elo">
                    <span class="player-elo">${player.elo}</span>
                </div>
                <div class="col-record">
                    <span class="player-record">${player.wins}W / ${player.losses}L</span>
                </div>
            </div>
        `;
    }).join('');
}

// ==================== Queue Status Polling ====================

function startQueuePolling() {
    setInterval(async () => {
        try {
            const status = await api.getQueueStatus();
            document.getElementById('pingpongQueue').textContent = status.pingpong || 0;
            document.getElementById('snakeQueue').textContent = status.snake || 0;
            document.getElementById('tankQueue').textContent = status.tank || 0;
        } catch (error) {
            // Silent fail for queue status
        }
    }, 3000);
}

// ==================== UI Helpers ====================

function showModal(modalId) {
    document.getElementById(modalId).classList.remove('hidden');
}

function hideModal(modalId) {
    document.getElementById(modalId).classList.add('hidden');
}

function showLoading(text = 'Loading...') {
    document.querySelector('.loader-text').textContent = text;
    document.getElementById('loadingOverlay').classList.remove('hidden');
}

function hideLoading() {
    document.getElementById('loadingOverlay').classList.add('hidden');
}

function showToast(message, type = 'success') {
    const toast = document.getElementById('toast');
    toast.textContent = message;
    toast.className = `toast ${type}`;

    setTimeout(() => {
        toast.classList.add('hidden');
    }, 3000);
}

// Update score display (called by games)
function updateScore(player1, player2) {
    document.getElementById('player1Score').textContent = player1;
    document.getElementById('player2Score').textContent = player2;
}

// Make functions globally available
window.joinMatchmaking = joinMatchmaking;
window.loadLeaderboard = loadLeaderboard;
window.startGame = startGame;
window.exitGame = exitGame;
window.registerPlayer = registerPlayer;
window.closeResultModal = closeResultModal;
window.updateScore = updateScore;

// ==================== Logout ====================

async function logoutPlayer() {
    if (!appState.currentPlayer) return;

    try {
        await api.logout(appState.currentPlayer.id);
    } catch (error) {
        console.error('Logout error:', error);
    }

    // Clear local state
    appState.currentPlayer = null;
    appState.currentMatch = null;
    appState.currentGame = null;
    appState.isInQueue = false;

    // Clear localStorage
    localStorage.removeItem('playerId');

    // Reset UI
    document.getElementById('userName').textContent = 'Guest';
    document.getElementById('userElo').textContent = '1000';
    document.getElementById('logoutBtn').style.display = 'none';

    // Switch to games view and show auth modal
    switchView('games');
    showModal('authModal');

    showToast('Logged out successfully', 'success');
}

window.logoutPlayer = logoutPlayer;
