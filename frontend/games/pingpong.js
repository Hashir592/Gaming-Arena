/**
 * Ping Pong Game
 * Classic two-player paddle game
 * Controls: Player 1 (W/S), Player 2 (Arrow keys or AI)
 */

let pingPongGame = null;

function initPingPong(canvas, onGameEnd, isOpponentBot = false) {
    pingPongGame = new PingPongGame(canvas, onGameEnd, isOpponentBot);
    pingPongGame.start();
}

class PingPongGame {
    constructor(canvas, onGameEnd, isOpponentBot = false) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.onGameEnd = onGameEnd;
        this.running = false;
        this.isOpponentBot = isOpponentBot;

        // Game settings
        this.winScore = 5;

        // Paddle settings
        this.paddleWidth = 15;
        this.paddleHeight = 100;
        this.paddleSpeed = 8;

        // Ball settings
        this.ballSize = 15;
        this.ballSpeed = 6;

        // AI settings
        this.aiReactionDelay = 0.15; // Slight delay to make AI beatable
        this.aiErrorMargin = 30; // Random error in targeting

        // Initialize game state
        this.reset();

        // Input handling
        this.keys = {};
        this.setupInput();
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

        // Ball
        this.resetBall();
    }

    resetBall() {
        this.ball = {
            x: this.canvas.width / 2,
            y: this.canvas.height / 2,
            vx: this.ballSpeed * (Math.random() > 0.5 ? 1 : -1),
            vy: this.ballSpeed * (Math.random() - 0.5)
        };
    }

    setupInput() {
        window.addEventListener('keydown', (e) => {
            // Don't intercept keys when typing in input fields
            if (document.activeElement.tagName === 'INPUT' ||
                document.activeElement.tagName === 'TEXTAREA') {
                return;
            }

            this.keys[e.key] = true;
            // Prevent scrolling
            if (['ArrowUp', 'ArrowDown', 'w', 's'].includes(e.key)) {
                e.preventDefault();
            }
        });

        window.addEventListener('keyup', (e) => {
            this.keys[e.key] = false;
        });
    }

    start() {
        this.running = true;
        this.reset();
        this.gameLoop();
    }

    stop() {
        this.running = false;
        if (window.currentGameLoop) {
            cancelAnimationFrame(window.currentGameLoop);
        }
    }

    gameLoop() {
        if (!this.running) return;

        this.update();
        this.render();

        window.currentGameLoop = requestAnimationFrame(() => this.gameLoop());
    }

    update() {
        // Player 1 input (W/S) - Always human controlled
        if (this.keys['w'] || this.keys['W']) {
            this.paddle1.y -= this.paddleSpeed;
        }
        if (this.keys['s'] || this.keys['S']) {
            this.paddle1.y += this.paddleSpeed;
        }

        // Player 2: AI or keyboard
        if (this.isOpponentBot) {
            this.updateBotAI();
        } else {
            // Human player 2 (Arrow keys)
            if (this.keys['ArrowUp']) {
                this.paddle2.y -= this.paddleSpeed;
            }
            if (this.keys['ArrowDown']) {
                this.paddle2.y += this.paddleSpeed;
            }
        }

        // Keep paddles in bounds
        this.paddle1.y = Math.max(0, Math.min(this.canvas.height - this.paddleHeight, this.paddle1.y));
        this.paddle2.y = Math.max(0, Math.min(this.canvas.height - this.paddleHeight, this.paddle2.y));

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

    /**
     * AI Logic for Bot Paddle
     * - Tracks ball Y position
     * - Moves toward predicted ball position
     * - Has slight reaction delay and error margin to be beatable
     */
    updateBotAI() {
        // Only react when ball is moving toward bot
        if (this.ball.vx > 0) {
            // Predict where ball will be when it reaches paddle
            const timeToReach = (this.paddle2.x - this.ball.x) / this.ball.vx;
            let predictedY = this.ball.y + this.ball.vy * timeToReach * this.aiReactionDelay;

            // Add some error to make AI beatable
            predictedY += (Math.random() - 0.5) * this.aiErrorMargin;

            // Target center of paddle to predicted ball position
            const paddleCenter = this.paddle2.y + this.paddleHeight / 2;
            const targetY = predictedY + this.ballSize / 2;

            // Move toward target with speed limit
            const diff = targetY - paddleCenter;
            const moveSpeed = this.paddleSpeed * 0.85; // Slightly slower than player

            if (Math.abs(diff) > 5) {
                if (diff > 0) {
                    this.paddle2.y += moveSpeed;
                } else {
                    this.paddle2.y -= moveSpeed;
                }
            }
        } else {
            // Ball moving away - slowly return to center
            const paddleCenter = this.paddle2.y + this.paddleHeight / 2;
            const canvasCenter = this.canvas.height / 2;
            const diff = canvasCenter - paddleCenter;

            if (Math.abs(diff) > 10) {
                if (diff > 0) {
                    this.paddle2.y += this.paddleSpeed * 0.3;
                } else {
                    this.paddle2.y -= this.paddleSpeed * 0.3;
                }
            }
        }
    }

    checkWin() {
        if (this.paddle1.score >= this.winScore) {
            this.stop();
            this.onGameEnd(1);
        } else if (this.paddle2.score >= this.winScore) {
            this.stop();
            this.onGameEnd(2);
        }
    }

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
        ctx.fillRect(this.paddle1.x, this.paddle1.y, this.paddleWidth, this.paddleHeight);

        // Bot paddle has different color (orange glow)
        if (this.isOpponentBot) {
            ctx.shadowColor = '#f59e0b';
            ctx.fillStyle = '#f59e0b';
        } else {
            ctx.shadowColor = '#8b5cf6';
            ctx.fillStyle = '#8b5cf6';
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

        // Draw AI indicator if bot
        if (this.isOpponentBot) {
            ctx.font = '14px Rajdhani';
            ctx.fillStyle = '#f59e0b';
            ctx.textAlign = 'right';
            ctx.fillText('🤖 AI', this.canvas.width - 20, 30);
        }
    }
}

window.initPingPong = initPingPong;
