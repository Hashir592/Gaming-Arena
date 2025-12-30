/**
 * Snake Battle Game
 * Two-player competitive snake game
 * Controls: Player 1 (WASD), Player 2 (Arrow keys or AI)
 */

let snakeGame = null;

function initSnake(canvas, onGameEnd, isOpponentBot = false) {
    snakeGame = new SnakeGame(canvas, onGameEnd, isOpponentBot);
    snakeGame.start();
}

class SnakeGame {
    constructor(canvas, onGameEnd, isOpponentBot = false) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.onGameEnd = onGameEnd;
        this.running = false;
        this.isOpponentBot = isOpponentBot;

        // Grid settings
        this.gridSize = 20;
        this.gridWidth = Math.floor(canvas.width / this.gridSize);
        this.gridHeight = Math.floor(canvas.height / this.gridSize);

        // Game speed (ms per frame)
        this.gameSpeed = 100;
        this.lastUpdate = 0;

        // Initialize
        this.reset();
        this.setupInput();
    }

    reset() {
        // Snake 1 (left side, blue) - Human
        this.snake1 = {
            body: [
                { x: 5, y: Math.floor(this.gridHeight / 2) },
                { x: 4, y: Math.floor(this.gridHeight / 2) },
                { x: 3, y: Math.floor(this.gridHeight / 2) }
            ],
            direction: { x: 1, y: 0 },
            nextDirection: { x: 1, y: 0 },
            color: '#6366f1',
            alive: true
        };

        // Snake 2 (right side, orange for bot, purple for human)
        this.snake2 = {
            body: [
                { x: this.gridWidth - 6, y: Math.floor(this.gridHeight / 2) },
                { x: this.gridWidth - 5, y: Math.floor(this.gridHeight / 2) },
                { x: this.gridWidth - 4, y: Math.floor(this.gridHeight / 2) }
            ],
            direction: { x: -1, y: 0 },
            nextDirection: { x: -1, y: 0 },
            color: this.isOpponentBot ? '#f59e0b' : '#8b5cf6',
            alive: true
        };

        // Spawn initial food
        this.food = [];
        for (let i = 0; i < 3; i++) {
            this.spawnFood();
        }

        // Scores
        updateScore(this.snake1.body.length, this.snake2.body.length);
    }

    setupInput() {
        window.addEventListener('keydown', (e) => {
            // Don't intercept keys when typing in input fields
            if (document.activeElement.tagName === 'INPUT' ||
                document.activeElement.tagName === 'TEXTAREA') {
                return;
            }

            // Prevent scrolling
            if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'w', 'a', 's', 'd'].includes(e.key)) {
                e.preventDefault();
            }

            // Snake 1 controls (WASD) - Always human
            if (e.key === 'w' || e.key === 'W') {
                if (this.snake1.direction.y !== 1) this.snake1.nextDirection = { x: 0, y: -1 };
            } else if (e.key === 's' || e.key === 'S') {
                if (this.snake1.direction.y !== -1) this.snake1.nextDirection = { x: 0, y: 1 };
            } else if (e.key === 'a' || e.key === 'A') {
                if (this.snake1.direction.x !== 1) this.snake1.nextDirection = { x: -1, y: 0 };
            } else if (e.key === 'd' || e.key === 'D') {
                if (this.snake1.direction.x !== -1) this.snake1.nextDirection = { x: 1, y: 0 };
            }

            // Snake 2 controls (Arrow keys) - Only if not bot
            if (!this.isOpponentBot) {
                if (e.key === 'ArrowUp') {
                    if (this.snake2.direction.y !== 1) this.snake2.nextDirection = { x: 0, y: -1 };
                } else if (e.key === 'ArrowDown') {
                    if (this.snake2.direction.y !== -1) this.snake2.nextDirection = { x: 0, y: 1 };
                } else if (e.key === 'ArrowLeft') {
                    if (this.snake2.direction.x !== 1) this.snake2.nextDirection = { x: -1, y: 0 };
                } else if (e.key === 'ArrowRight') {
                    if (this.snake2.direction.x !== -1) this.snake2.nextDirection = { x: 1, y: 0 };
                }
            }
        });
    }

    spawnFood() {
        let x, y;
        do {
            x = Math.floor(Math.random() * this.gridWidth);
            y = Math.floor(Math.random() * this.gridHeight);
        } while (this.isOccupied(x, y));

        this.food.push({ x, y });
    }

    isOccupied(x, y) {
        // Check snake 1
        for (const seg of this.snake1.body) {
            if (seg.x === x && seg.y === y) return true;
        }
        // Check snake 2
        for (const seg of this.snake2.body) {
            if (seg.x === x && seg.y === y) return true;
        }
        // Check food
        for (const f of this.food) {
            if (f.x === x && f.y === y) return true;
        }
        return false;
    }

    start() {
        this.running = true;
        this.reset();
        this.lastUpdate = Date.now();
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

        const now = Date.now();
        if (now - this.lastUpdate >= this.gameSpeed) {
            this.update();
            this.lastUpdate = now;
        }

        this.render();
        window.currentGameLoop = requestAnimationFrame(() => this.gameLoop());
    }

    update() {
        // Update AI direction before movement
        if (this.isOpponentBot && this.snake2.alive) {
            this.updateBotAI();
        }

        // Update directions
        this.snake1.direction = { ...this.snake1.nextDirection };
        this.snake2.direction = { ...this.snake2.nextDirection };

        // Move snakes
        this.moveSnake(this.snake1);
        this.moveSnake(this.snake2);

        // Check collisions
        this.checkCollisions();

        // Check food
        this.checkFood(this.snake1);
        this.checkFood(this.snake2);

        // Update scores
        updateScore(this.snake1.body.length, this.snake2.body.length);

        // Check win condition
        this.checkWin();
    }

    /**
     * AI Logic for Bot Snake
     * - Finds nearest food
     * - Moves toward it
     * - Avoids self-collision and opponent
     */
    updateBotAI() {
        const head = this.snake2.body[0];

        // Find nearest food
        let nearestFood = null;
        let minDist = Infinity;
        for (const f of this.food) {
            const dist = Math.abs(f.x - head.x) + Math.abs(f.y - head.y);
            if (dist < minDist) {
                minDist = dist;
                nearestFood = f;
            }
        }

        if (!nearestFood) return;

        // Possible directions
        const directions = [
            { x: 0, y: -1 },  // Up
            { x: 0, y: 1 },   // Down
            { x: -1, y: 0 },  // Left
            { x: 1, y: 0 }    // Right
        ];

        // Filter out reverse direction (can't go backwards)
        const validDirections = directions.filter(d =>
            !(d.x === -this.snake2.direction.x && d.y === -this.snake2.direction.y)
        );

        // Score each direction
        let bestDir = this.snake2.direction;
        let bestScore = -Infinity;

        for (const dir of validDirections) {
            const newX = head.x + dir.x;
            const newY = head.y + dir.y;

            // Wrap around
            const wrappedX = (newX + this.gridWidth) % this.gridWidth;
            const wrappedY = (newY + this.gridHeight) % this.gridHeight;

            // Check for collision
            if (this.wouldCollide(wrappedX, wrappedY)) {
                continue; // Skip dangerous directions
            }

            // Score based on distance to food (lower is better, so negate)
            const distToFood = Math.abs(nearestFood.x - wrappedX) + Math.abs(nearestFood.y - wrappedY);
            let score = -distToFood;

            // Bonus for moving toward food
            if ((dir.x > 0 && nearestFood.x > head.x) ||
                (dir.x < 0 && nearestFood.x < head.x) ||
                (dir.y > 0 && nearestFood.y > head.y) ||
                (dir.y < 0 && nearestFood.y < head.y)) {
                score += 5;
            }

            // Small random factor to prevent predictability
            score += Math.random() * 2;

            if (score > bestScore) {
                bestScore = score;
                bestDir = dir;
            }
        }

        this.snake2.nextDirection = bestDir;
    }

    wouldCollide(x, y) {
        // Check self collision (skip first few segments as they'll move)
        for (let i = 0; i < this.snake2.body.length - 1; i++) {
            if (this.snake2.body[i].x === x && this.snake2.body[i].y === y) {
                return true;
            }
        }

        // Check opponent collision
        for (const seg of this.snake1.body) {
            if (seg.x === x && seg.y === y) {
                return true;
            }
        }

        return false;
    }

    moveSnake(snake) {
        if (!snake.alive) return;

        const head = snake.body[0];
        const newHead = {
            x: head.x + snake.direction.x,
            y: head.y + snake.direction.y
        };

        // Wrap around walls
        if (newHead.x < 0) newHead.x = this.gridWidth - 1;
        if (newHead.x >= this.gridWidth) newHead.x = 0;
        if (newHead.y < 0) newHead.y = this.gridHeight - 1;
        if (newHead.y >= this.gridHeight) newHead.y = 0;

        snake.body.unshift(newHead);
        snake.body.pop();
    }

    checkCollisions() {
        const head1 = this.snake1.body[0];
        const head2 = this.snake2.body[0];

        // Snake 1 self collision
        for (let i = 1; i < this.snake1.body.length; i++) {
            if (head1.x === this.snake1.body[i].x && head1.y === this.snake1.body[i].y) {
                this.snake1.alive = false;
            }
        }

        // Snake 2 self collision
        for (let i = 1; i < this.snake2.body.length; i++) {
            if (head2.x === this.snake2.body[i].x && head2.y === this.snake2.body[i].y) {
                this.snake2.alive = false;
            }
        }

        // Snake 1 hitting snake 2
        for (const seg of this.snake2.body) {
            if (head1.x === seg.x && head1.y === seg.y) {
                this.snake1.alive = false;
            }
        }

        // Snake 2 hitting snake 1
        for (const seg of this.snake1.body) {
            if (head2.x === seg.x && head2.y === seg.y) {
                this.snake2.alive = false;
            }
        }

        // Head-on collision
        if (head1.x === head2.x && head1.y === head2.y) {
            this.snake1.alive = false;
            this.snake2.alive = false;
        }
    }

    checkFood(snake) {
        if (!snake.alive) return;

        const head = snake.body[0];

        for (let i = this.food.length - 1; i >= 0; i--) {
            if (head.x === this.food[i].x && head.y === this.food[i].y) {
                // Eat food - grow snake
                snake.body.push({ ...snake.body[snake.body.length - 1] });
                this.food.splice(i, 1);
                this.spawnFood();
            }
        }
    }

    checkWin() {
        if (!this.snake1.alive && !this.snake2.alive) {
            // Draw - longest snake wins
            this.stop();
            this.onGameEnd(this.snake1.body.length >= this.snake2.body.length ? 1 : 2);
        } else if (!this.snake1.alive) {
            this.stop();
            this.onGameEnd(2);
        } else if (!this.snake2.alive) {
            this.stop();
            this.onGameEnd(1);
        }
    }

    render() {
        const ctx = this.ctx;

        // Clear canvas
        ctx.fillStyle = '#0a0a0f';
        ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

        // Draw grid
        ctx.strokeStyle = '#1a1a2a';
        ctx.lineWidth = 1;
        for (let x = 0; x <= this.gridWidth; x++) {
            ctx.beginPath();
            ctx.moveTo(x * this.gridSize, 0);
            ctx.lineTo(x * this.gridSize, this.canvas.height);
            ctx.stroke();
        }
        for (let y = 0; y <= this.gridHeight; y++) {
            ctx.beginPath();
            ctx.moveTo(0, y * this.gridSize);
            ctx.lineTo(this.canvas.width, y * this.gridSize);
            ctx.stroke();
        }

        // Draw food
        ctx.shadowBlur = 10;
        ctx.shadowColor = '#22c55e';
        ctx.fillStyle = '#22c55e';
        for (const f of this.food) {
            ctx.beginPath();
            ctx.arc(
                f.x * this.gridSize + this.gridSize / 2,
                f.y * this.gridSize + this.gridSize / 2,
                this.gridSize / 2 - 2,
                0, Math.PI * 2
            );
            ctx.fill();
        }

        // Draw snakes
        this.drawSnake(this.snake1);
        this.drawSnake(this.snake2);

        ctx.shadowBlur = 0;

        // Draw AI indicator if bot
        if (this.isOpponentBot) {
            ctx.font = '14px Rajdhani';
            ctx.fillStyle = '#f59e0b';
            ctx.textAlign = 'right';
            ctx.fillText('🤖 AI', this.canvas.width - 20, 30);
        }
    }

    drawSnake(snake) {
        const ctx = this.ctx;
        ctx.shadowColor = snake.color;
        ctx.shadowBlur = snake.alive ? 15 : 0;
        ctx.fillStyle = snake.alive ? snake.color : '#444';

        for (let i = 0; i < snake.body.length; i++) {
            const seg = snake.body[i];
            const size = i === 0 ? this.gridSize - 2 : this.gridSize - 4;
            const offset = i === 0 ? 1 : 2;

            ctx.fillRect(
                seg.x * this.gridSize + offset,
                seg.y * this.gridSize + offset,
                size,
                size
            );
        }
    }
}

window.initSnake = initSnake;
