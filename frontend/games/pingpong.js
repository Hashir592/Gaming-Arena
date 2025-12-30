/**
 * Ping Pong Game
 * Classic two-player paddle game
 * Controls: Player 1 (W/S), Player 2 (Arrow keys or AI)
 * 
 * Multiplayer Architecture:
 * - Player 1 (Host): Calculates ball physics, sends ball/paddle/score data
 * - Player 2 (Client): Receives ball data, sends paddle data
 */

let pingPongGame = null;

function initPingPong(canvas, onGameEnd, isOpponentBot = false, matchId = null, isPlayer1 = true, playerId = null) {
    pingPongGame = new PingPongGame(canvas, onGameEnd, isOpponentBot, matchId, isPlayer1, playerId);
    pingPongGame.start();
}

class PingPongGame {
    constructor(canvas, onGameEnd, isOpponentBot = false, matchId = null, isPlayer1 = true, playerId = null) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.onGameEnd = onGameEnd;
        this.running = false;

        // Multiplayer settings
        this.isOpponentBot = isOpponentBot;
        this.matchId = matchId;
        this.isPlayer1 = isPlayer1; // true = Host, false = Client
        this.playerId = playerId;
        this.isMultiplayer = matchId !== null && !isOpponentBot;

        // Game settings
        this.winScore = 5;

        // Paddle settings
        this.paddleWidth = 15;
        this.paddleHeight = 100;
        this.paddleSpeed = 8;

        // Ball settings
        this.ballSize = 15;
        this.ballSpeed = 6;

        // Network smoothing
        this.lastRemoteUpdate = 0;
        this.remotePaddleY = 0;
        this.targetRemotePaddleY = 0;

        // AI settings
        this.aiReactionDelay = 0.15; // Slight delay to make AI beatable
        this.aiErrorMargin = 30; // Random error in targeting

        // Initialize game state
        this.reset();

        // Setup network callbacks if multiplayer
        if (this.isMultiplayer) {
            this.setupNetwork();
        }

        // Input handling
        this.keys = {};
        this.setupInput();
    }

    setupNetwork() {
        // Callback when we receive game state from opponent
        api.setGameCallbacks(
            (data) => this.onGameStateReceived(data),
            () => { console.log('Opponent Connected'); },
            () => { console.log('Opponent Left'); showToast('Opponent disconnected', 'error'); }
        );
    }

    onGameStateReceived(data) {
        if (data.type === 'SCORE_UPDATE') {
            this.paddle1.score = data.score1;
            this.paddle2.score = data.score2;
            updateScore(this.paddle1.score, this.paddle2.score);
            return;
        }

        // Handle game state (paddle/ball positions)
        if (this.isPlayer1) {
            // As Player 1 (Host), we only care about Player 2's paddle position
            if (data.p2y !== undefined) {
                this.targetRemotePaddleY = data.p2y;
                this.lastRemoteUpdate = Date.now();
            }
        } else {
            // As Player 2 (Client), we receive everything from Player 1
            if (data.p1y !== undefined) {
                this.targetRemotePaddleY = data.p1y;
            }

            // Sync ball position (client trusts host completely for ball)
            if (data.bx !== undefined && data.by !== undefined) {
                this.ball.x = data.bx;
                this.ball.y = data.by;
                this.ball.vx = data.bvx;
                this.ball.vy = data.bvy;
            }

            // Sync scores
            if (data.s1 !== undefined && data.s2 !== undefined) {
                if (this.paddle1.score !== data.s1 || this.paddle2.score !== data.s2) {
                    this.paddle1.score = data.s1;
                    this.paddle2.score = data.s2;
                    updateScore(this.paddle1.score, this.paddle2.score);
                }
            }
        }
    }

    sendState() {
        if (!this.running) return;

        if (this.isPlayer1) {
            // Host sends: P1 Paddle, Ball, Scores
            api.sendGameState({
                p1y: this.paddle1.y,
                bx: this.ball.x,
                by: this.ball.y,
                bvx: this.ball.vx,
                bvy: this.ball.vy,
                s1: this.paddle1.score,
                s2: this.paddle2.score
            });
        } else {
            // Client sends: P2 Paddle only
            api.sendGameState({
                p2y: this.paddle2.y
            });
        }
    }

    reset() {
        // Paddles
        this.paddle1 = {
            x: 30,
            y: this.canvas.height / 2 - this.paddleHeight / 2,
            score: 0
        };

        this.paddle2 = {
            x: this.canvas.width - 30 - this.paddleWidth,
            y: this.canvas.height / 2 - this.paddleHeight / 2,
            score: 0
        };

        // Initial remote positions
        this.remotePaddleY = this.isPlayer1 ? this.paddle2.y : this.paddle1.y;
        this.targetRemotePaddleY = this.remotePaddleY;

        // Ball
        this.resetBall();
    }

    resetBall() {
        // Only Host (Player 1) or Singleplayer determines ball reset
        if (this.isPlayer1 || !this.isMultiplayer) {
            this.ball = {
                x: this.canvas.width / 2,
                y: this.canvas.height / 2,
                vx: this.ballSpeed * (Math.random() > 0.5 ? 1 : -1),
                vy: this.ballSpeed * (Math.random() - 0.5)
            };
        } else {
            // Client initializes ball but waits for update
            this.ball = { x: this.canvas.width / 2, y: this.canvas.height / 2, vx: 0, vy: 0 };
        }
    }

    setupInput() {
        // Keyboard controls
        window.addEventListener('keydown', (e) => {
            if (document.activeElement.tagName === 'INPUT' ||
                document.activeElement.tagName === 'TEXTAREA') {
                return;
            }
            this.keys[e.key] = true;
            if (['ArrowUp', 'ArrowDown', 'w', 's'].includes(e.key)) {
                e.preventDefault();
            }
        });

        window.addEventListener('keyup', (e) => {
            this.keys[e.key] = false;
        });

        // Touch controls for mobile
        this.canvas.addEventListener('touchstart', (e) => {
            e.preventDefault();
            this.handleTouch(e);
        }, { passive: false });

        this.canvas.addEventListener('touchmove', (e) => {
            e.preventDefault();
            this.handleTouch(e);
        }, { passive: false });
    }

    handleTouch(e) {
        const touch = e.touches[0];
        const rect = this.canvas.getBoundingClientRect();
        const touchY = touch.clientY - rect.top;

        // Center paddle on touch
        const targetY = touchY - this.paddleHeight / 2;

        // Update local paddle position directly
        if (this.isPlayer1) {
            this.paddle1.y = targetY;
        } else {
            this.paddle2.y = targetY;
        }
    }

    start() {
        this.running = true;
        this.reset();
        this.gameLoop();

        // Network sync loop (30fps)
        if (this.isMultiplayer) {
            this.networkInterval = setInterval(() => this.sendState(), 33);
        }
    }

    stop() {
        this.running = false;
        if (window.currentGameLoop) {
            cancelAnimationFrame(window.currentGameLoop);
        }
        if (this.networkInterval) {
            clearInterval(this.networkInterval);
        }
    }

    gameLoop() {
        if (!this.running) return;

        // Auto-reconnect if needed
        if (this.isMultiplayer && !api.isConnected()) {
            if (this.matchId && this.playerId) {
                api.ensureConnection(this.matchId, this.playerId);
            }
        }

        this.update();
        this.render();

        window.currentGameLoop = requestAnimationFrame(() => this.gameLoop());
    }

    update() {
        // ... (existing update logic) ...
        // Handle Local Player Input
        if (this.isPlayer1) {
            // Player 1 controls Left Paddle (W/S or Arrows if preferred for P1)
            if (this.keys['w'] || this.keys['W'] || this.keys['ArrowUp']) {
                this.paddle1.y -= this.paddleSpeed;
            }
            if (this.keys['s'] || this.keys['S'] || this.keys['ArrowDown']) {
                this.paddle1.y += this.paddleSpeed;
            }
        } else {
            // Player 2 controls Right Paddle (Arrow keys or W/S)
            if (this.keys['ArrowUp'] || this.keys['w'] || this.keys['W']) {
                this.paddle2.y -= this.paddleSpeed;
            }
            if (this.keys['ArrowDown'] || this.keys['s'] || this.keys['S']) {
                this.paddle2.y += this.paddleSpeed;
            }
        }

        // Handle Remote Player Movement (Interpolation)
        if (this.isMultiplayer) {
            const lerpFactor = 0.2;
            this.remotePaddleY += (this.targetRemotePaddleY - this.remotePaddleY) * lerpFactor;

            if (this.isPlayer1) {
                this.paddle2.y = this.remotePaddleY;
            } else {
                this.paddle1.y = this.remotePaddleY;
            }
        } else if (this.isOpponentBot) {
            // Singleplayer Bot Logic
            this.updateBotAI();
        } else {
            // Local multiplayer (debug/testing) - P2 controls
            if (this.keys['ArrowUp']) this.paddle2.y -= this.paddleSpeed;
            if (this.keys['ArrowDown']) this.paddle2.y += this.paddleSpeed;
        }

        // Keep paddles in bounds
        this.paddle1.y = Math.max(0, Math.min(this.canvas.height - this.paddleHeight, this.paddle1.y));
        this.paddle2.y = Math.max(0, Math.min(this.canvas.height - this.paddleHeight, this.paddle2.y));

        // Physics: Run physics for everyone to ensure smooth movement (Client prediction)
        // Client will correct position when server update arrives
        this.updatePhysics();
    }

    updatePhysics() {
        // ... (existing physics logic same as before, no changes needed, just context) ...
        // Ball movement
        this.ball.x += this.ball.vx;
        this.ball.y += this.ball.vy;

        // Ball collision with top/bottom walls
        if (this.ball.y <= 0 || this.ball.y >= this.canvas.height - this.ballSize) {
            this.ball.vy *= -1;
            this.ball.y = Math.max(0, Math.min(this.canvas.height - this.ballSize, this.ball.y));
        }

        // Ball collision with paddles
        // Left paddle (Player 1)
        if (this.ball.x <= this.paddle1.x + this.paddleWidth &&
            this.ball.x + this.ballSize >= this.paddle1.x &&
            this.ball.y + this.ballSize >= this.paddle1.y &&
            this.ball.y <= this.paddle1.y + this.paddleHeight) {
            this.ball.vx = Math.abs(this.ball.vx) * 1.05; // Speed up slightly
            this.ball.x = this.paddle1.x + this.paddleWidth;
            // Add angle based on where ball hit paddle
            let hitPos = (this.ball.y + this.ballSize / 2 - this.paddle1.y) / this.paddleHeight;
            this.ball.vy = (hitPos - 0.5) * this.ballSpeed * 2;
        }

        // Right paddle (Player 2)
        if (this.ball.x + this.ballSize >= this.paddle2.x &&
            this.ball.x <= this.paddle2.x + this.paddleWidth &&
            this.ball.y + this.ballSize >= this.paddle2.y &&
            this.ball.y <= this.paddle2.y + this.paddleHeight) {
            this.ball.vx = -Math.abs(this.ball.vx) * 1.05;
            this.ball.x = this.paddle2.x - this.ballSize;
            let hitPos = (this.ball.y + this.ballSize / 2 - this.paddle2.y) / this.paddleHeight;
            this.ball.vy = (hitPos - 0.5) * this.ballSpeed * 2;
        }

        // Scoring
        if (this.ball.x < 0) {
            this.paddle2.score++;
            this.checkWin();
            this.resetBall();
            updateScore(this.paddle1.score, this.paddle2.score);
        } else if (this.ball.x > this.canvas.width) {
            this.paddle1.score++;
            this.checkWin();
            this.resetBall();
            updateScore(this.paddle1.score, this.paddle2.score);
        }
    }

    // ... (rest of methods: updateBotAI, checkWin) ...

    render() {
        const ctx = this.ctx;

        // Clear canvas
        ctx.fillStyle = '#0a0a0f';
        ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

        // Draw center line
        ctx.setLineDash([10, 10]);
        ctx.strokeStyle = '#333';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(this.canvas.width / 2, 0);
        ctx.lineTo(this.canvas.width / 2, this.canvas.height);
        ctx.stroke();
        ctx.setLineDash([]);

        // Draw paddles with glow effect
        ctx.shadowBlur = 20;
        ctx.shadowColor = '#6366f1';
        ctx.fillStyle = '#6366f1';

        // PADDLE 1 (Left)
        if (this.isPlayer1) {
            ctx.fillStyle = '#6366f1'; // Self is always purple
        } else {
            ctx.fillStyle = '#f43f5e'; // Enemy is red
            ctx.shadowColor = '#f43f5e';
        }
        ctx.fillRect(this.paddle1.x, this.paddle1.y, this.paddleWidth, this.paddleHeight);

        // PADDLE 2 (Right)
        if (this.isOpponentBot) {
            ctx.shadowColor = '#f59e0b';
            ctx.fillStyle = '#f59e0b';
        } else {
            if (!this.isPlayer1) {
                ctx.fillStyle = '#6366f1'; // Self is always purple
                ctx.shadowColor = '#6366f1';
            } else {
                ctx.fillStyle = '#f43f5e'; // Enemy is red
                ctx.shadowColor = '#f43f5e';
            }
        }
        ctx.fillRect(this.paddle2.x, this.paddle2.y, this.paddleWidth, this.paddleHeight);

        // Draw ball
        ctx.shadowColor = '#22c55e';
        ctx.fillStyle = '#22c55e';
        ctx.beginPath();
        ctx.arc(this.ball.x + this.ballSize / 2, this.ball.y + this.ballSize / 2,
            this.ballSize / 2, 0, Math.PI * 2);
        ctx.fill();

        ctx.shadowBlur = 0;

        // Draw scores
        ctx.font = '48px Orbitron';
        ctx.fillStyle = '#444';
        ctx.textAlign = 'center';
        ctx.fillText(this.paddle1.score, this.canvas.width / 4, 60);
        ctx.fillText(this.paddle2.score, this.canvas.width * 3 / 4, 60);

        // Draw Indicators
        ctx.font = '14px Rajdhani';
        ctx.textAlign = 'center';

        if (this.isOpponentBot) {
            ctx.fillStyle = '#f59e0b';
            ctx.textAlign = 'right';
            ctx.fillText('🤖 AI', this.canvas.width - 20, 30);
        } else if (this.isMultiplayer) {
            // Check real connection status
            if (api.isConnected()) {
                ctx.fillStyle = '#22c55e';
                ctx.fillText('🟢 ONLINE', this.canvas.width / 2, 30);
            } else {
                ctx.fillStyle = '#ef4444';
                ctx.fillText('🔴 RECONNECTING...', this.canvas.width / 2, 30);
            }

            // Draw "YOU" indicator
            ctx.fillStyle = '#fff';
            if (this.isPlayer1) {
                ctx.fillText('YOU', this.paddle1.x + this.paddleWidth / 2, this.paddle1.y - 10);
            } else {
                ctx.fillText('YOU', this.paddle2.x + this.paddleWidth / 2, this.paddle2.y - 10);
            }
        }
    }
}

window.initPingPong = initPingPong;
