/**
 * Tank Battle Game
 * Two-player top-down tank game
 * Controls: Player 1 (WASD + Space), Player 2 (Arrow keys + Enter or AI)
 */

let tankGame = null;

function initTank(canvas, onGameEnd, isOpponentBot = false) {
    tankGame = new TankGame(canvas, onGameEnd, isOpponentBot);
    tankGame.start();
}

class TankGame {
    constructor(canvas, onGameEnd, isOpponentBot = false) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.onGameEnd = onGameEnd;
        this.running = false;
        this.isOpponentBot = isOpponentBot;

        // Game settings
        this.tankSize = 30;
        this.tankSpeed = 3;
        this.rotationSpeed = 0.05;
        this.bulletSpeed = 8;
        this.bulletSize = 6;
        this.maxHealth = 3;
        this.shootCooldown = 500; // ms

        // AI settings
        this.aiShootCooldown = 800; // Slower than human
        this.aiLastDecision = 0;
        this.aiDecisionInterval = 200; // Update AI every 200ms
        this.aiTargetAngle = 0;

        // Initialize
        this.reset();
        this.setupInput();
    }

    reset() {
        // Tank 1 (left side, blue) - Human
        this.tank1 = {
            x: 100,
            y: this.canvas.height / 2,
            angle: 0,
            health: this.maxHealth,
            lastShot: 0,
            color: '#6366f1'
        };

        // Tank 2 (right side, orange for bot, purple for human)
        this.tank2 = {
            x: this.canvas.width - 100,
            y: this.canvas.height / 2,
            angle: Math.PI,
            health: this.maxHealth,
            lastShot: 0,
            color: this.isOpponentBot ? '#f59e0b' : '#8b5cf6'
        };

        // Bullets
        this.bullets = [];

        // Obstacles
        this.obstacles = this.generateObstacles();

        // Input state
        this.keys = {};

        // AI state
        this.aiState = 'hunting'; // 'hunting', 'dodging', 'circling'
        this.aiMoveDir = 1;

        updateScore(this.tank1.health, this.tank2.health);
    }

    generateObstacles() {
        const obstacles = [];
        const centerX = this.canvas.width / 2;
        const centerY = this.canvas.height / 2;

        // Center obstacle
        obstacles.push({ x: centerX - 40, y: centerY - 40, width: 80, height: 80 });

        // Corner obstacles
        obstacles.push({ x: 150, y: 100, width: 60, height: 40 });
        obstacles.push({ x: this.canvas.width - 210, y: 100, width: 60, height: 40 });
        obstacles.push({ x: 150, y: this.canvas.height - 140, width: 60, height: 40 });
        obstacles.push({ x: this.canvas.width - 210, y: this.canvas.height - 140, width: 60, height: 40 });

        return obstacles;
    }

    setupInput() {
        window.addEventListener('keydown', (e) => {
            // Don't intercept keys when typing in input fields
            if (document.activeElement.tagName === 'INPUT' ||
                document.activeElement.tagName === 'TEXTAREA') {
                return;
            }

            this.keys[e.key] = true;

            if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', ' ', 'Enter'].includes(e.key)) {
                e.preventDefault();
            }

            // Shooting for player 1
            if (e.key === ' ') {
                this.shoot(this.tank1);
            }

            // Shooting for player 2 (only if not bot)
            if (e.key === 'Enter' && !this.isOpponentBot) {
                this.shoot(this.tank2);
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
        // Tank 1 controls (WASD) - Always human
        if (this.keys['w'] || this.keys['W']) {
            this.moveTank(this.tank1, 1);
        }
        if (this.keys['s'] || this.keys['S']) {
            this.moveTank(this.tank1, -1);
        }
        if (this.keys['a'] || this.keys['A']) {
            this.tank1.angle -= this.rotationSpeed;
        }
        if (this.keys['d'] || this.keys['D']) {
            this.tank1.angle += this.rotationSpeed;
        }

        // Tank 2: AI or keyboard
        if (this.isOpponentBot) {
            this.updateBotAI();
        } else {
            // Human player 2 (Arrow keys)
            if (this.keys['ArrowUp']) {
                this.moveTank(this.tank2, 1);
            }
            if (this.keys['ArrowDown']) {
                this.moveTank(this.tank2, -1);
            }
            if (this.keys['ArrowLeft']) {
                this.tank2.angle -= this.rotationSpeed;
            }
            if (this.keys['ArrowRight']) {
                this.tank2.angle += this.rotationSpeed;
            }
        }

        // Update bullets
        this.updateBullets();

        // Update score display
        updateScore(this.tank1.health, this.tank2.health);

        // Check win
        this.checkWin();
    }

    /**
     * AI Logic for Bot Tank
     * - Rotates to aim at player
     * - Moves strategically
     * - Fires when aligned
     * - Attempts to dodge bullets
     */
    updateBotAI() {
        const now = Date.now();

        // Calculate angle to player
        const dx = this.tank1.x - this.tank2.x;
        const dy = this.tank1.y - this.tank2.y;
        const targetAngle = Math.atan2(dy, dx);
        const distToPlayer = Math.hypot(dx, dy);

        // Normalize angles for comparison
        let angleDiff = targetAngle - this.tank2.angle;
        while (angleDiff > Math.PI) angleDiff -= Math.PI * 2;
        while (angleDiff < -Math.PI) angleDiff += Math.PI * 2;

        // Rotate toward player (with slight imperfection)
        const rotateSpeed = this.rotationSpeed * 0.8;
        if (Math.abs(angleDiff) > 0.1) {
            if (angleDiff > 0) {
                this.tank2.angle += rotateSpeed;
            } else {
                this.tank2.angle -= rotateSpeed;
            }
        }

        // Check for incoming bullets and try to dodge
        let dodging = false;
        for (const bullet of this.bullets) {
            if (bullet.owner === 1) {
                const bulletDist = Math.hypot(bullet.x - this.tank2.x, bullet.y - this.tank2.y);
                if (bulletDist < 100) {
                    dodging = true;
                    // Move perpendicular to bullet direction
                    const perpAngle = Math.atan2(bullet.vy, bullet.vx) + Math.PI / 2;
                    this.tank2.x += Math.cos(perpAngle) * this.tankSpeed * 0.8;
                    this.tank2.y += Math.sin(perpAngle) * this.tankSpeed * 0.8;
                    break;
                }
            }
        }

        // Movement logic
        if (!dodging) {
            // Decision making every interval
            if (now - this.aiLastDecision > this.aiDecisionInterval) {
                this.aiLastDecision = now;

                // Random movement pattern
                if (Math.random() < 0.1) {
                    this.aiMoveDir = this.aiMoveDir === 1 ? -1 : 1;
                }

                // Change state occasionally
                if (Math.random() < 0.05) {
                    const states = ['hunting', 'circling', 'retreating'];
                    this.aiState = states[Math.floor(Math.random() * states.length)];
                }
            }

            // Execute movement based on state
            switch (this.aiState) {
                case 'hunting':
                    // Move toward player if far
                    if (distToPlayer > 200) {
                        this.moveTank(this.tank2, 0.7);
                    } else if (distToPlayer < 100) {
                        this.moveTank(this.tank2, -0.5);
                    }
                    break;

                case 'circling':
                    // Circle around player
                    const circleAngle = this.tank2.angle + Math.PI / 2 * this.aiMoveDir;
                    const newX = this.tank2.x + Math.cos(circleAngle) * this.tankSpeed * 0.6;
                    const newY = this.tank2.y + Math.sin(circleAngle) * this.tankSpeed * 0.6;
                    if (this.isValidPosition(newX, newY)) {
                        this.tank2.x = newX;
                        this.tank2.y = newY;
                    }
                    break;

                case 'retreating':
                    if (distToPlayer < 250) {
                        this.moveTank(this.tank2, -0.8);
                    } else {
                        this.aiState = 'hunting';
                    }
                    break;
            }
        }

        // Keep in bounds
        this.tank2.x = Math.max(this.tankSize, Math.min(this.canvas.width - this.tankSize, this.tank2.x));
        this.tank2.y = Math.max(this.tankSize, Math.min(this.canvas.height - this.tankSize, this.tank2.y));

        // Shoot when roughly aligned with player
        if (Math.abs(angleDiff) < 0.3 && now - this.tank2.lastShot > this.aiShootCooldown) {
            // Add slight randomness to shooting
            if (Math.random() < 0.7) {
                this.shoot(this.tank2);
            }
        }
    }

    isValidPosition(x, y) {
        if (x < this.tankSize / 2 || x > this.canvas.width - this.tankSize / 2) return false;
        if (y < this.tankSize / 2 || y > this.canvas.height - this.tankSize / 2) return false;

        for (const obs of this.obstacles) {
            if (this.circleRectCollision(x, y, this.tankSize / 2, obs)) {
                return false;
            }
        }
        return true;
    }

    moveTank(tank, direction) {
        const newX = tank.x + Math.cos(tank.angle) * this.tankSpeed * direction;
        const newY = tank.y + Math.sin(tank.angle) * this.tankSpeed * direction;

        // Check bounds
        if (newX < this.tankSize / 2 || newX > this.canvas.width - this.tankSize / 2) return;
        if (newY < this.tankSize / 2 || newY > this.canvas.height - this.tankSize / 2) return;

        // Check obstacle collision
        for (const obs of this.obstacles) {
            if (this.circleRectCollision(newX, newY, this.tankSize / 2, obs)) {
                return;
            }
        }

        // Check tank-tank collision
        const otherTank = tank === this.tank1 ? this.tank2 : this.tank1;
        const dist = Math.hypot(newX - otherTank.x, newY - otherTank.y);
        if (dist < this.tankSize) return;

        tank.x = newX;
        tank.y = newY;
    }

    shoot(tank) {
        const now = Date.now();
        if (now - tank.lastShot < this.shootCooldown) return;

        tank.lastShot = now;

        const bullet = {
            x: tank.x + Math.cos(tank.angle) * this.tankSize / 2,
            y: tank.y + Math.sin(tank.angle) * this.tankSize / 2,
            vx: Math.cos(tank.angle) * this.bulletSpeed,
            vy: Math.sin(tank.angle) * this.bulletSpeed,
            owner: tank === this.tank1 ? 1 : 2
        };

        this.bullets.push(bullet);
    }

    updateBullets() {
        for (let i = this.bullets.length - 1; i >= 0; i--) {
            const bullet = this.bullets[i];

            bullet.x += bullet.vx;
            bullet.y += bullet.vy;

            // Remove if out of bounds
            if (bullet.x < 0 || bullet.x > this.canvas.width ||
                bullet.y < 0 || bullet.y > this.canvas.height) {
                this.bullets.splice(i, 1);
                continue;
            }

            // Check obstacle collision
            let hitObstacle = false;
            for (const obs of this.obstacles) {
                if (this.circleRectCollision(bullet.x, bullet.y, this.bulletSize / 2, obs)) {
                    this.bullets.splice(i, 1);
                    hitObstacle = true;
                    break;
                }
            }
            if (hitObstacle) continue;

            // Check tank collision
            const targetTank = bullet.owner === 1 ? this.tank2 : this.tank1;
            const dist = Math.hypot(bullet.x - targetTank.x, bullet.y - targetTank.y);
            if (dist < this.tankSize / 2 + this.bulletSize / 2) {
                targetTank.health--;
                this.bullets.splice(i, 1);
            }
        }
    }

    circleRectCollision(cx, cy, radius, rect) {
        const closestX = Math.max(rect.x, Math.min(cx, rect.x + rect.width));
        const closestY = Math.max(rect.y, Math.min(cy, rect.y + rect.height));
        const distX = cx - closestX;
        const distY = cy - closestY;
        return (distX * distX + distY * distY) < (radius * radius);
    }

    checkWin() {
        if (this.tank1.health <= 0) {
            this.stop();
            this.onGameEnd(2);
        } else if (this.tank2.health <= 0) {
            this.stop();
            this.onGameEnd(1);
        }
    }

    render() {
        const ctx = this.ctx;

        // Clear canvas
        ctx.fillStyle = '#0a0a0f';
        ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

        // Draw grid pattern
        ctx.strokeStyle = '#1a1a2a';
        ctx.lineWidth = 1;
        const gridSize = 40;
        for (let x = 0; x <= this.canvas.width; x += gridSize) {
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, this.canvas.height);
            ctx.stroke();
        }
        for (let y = 0; y <= this.canvas.height; y += gridSize) {
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(this.canvas.width, y);
            ctx.stroke();
        }

        // Draw obstacles
        ctx.fillStyle = '#333';
        ctx.strokeStyle = '#555';
        ctx.lineWidth = 2;
        for (const obs of this.obstacles) {
            ctx.fillRect(obs.x, obs.y, obs.width, obs.height);
            ctx.strokeRect(obs.x, obs.y, obs.width, obs.height);
        }

        // Draw tanks
        this.drawTank(this.tank1);
        this.drawTank(this.tank2);

        // Draw bullets
        ctx.shadowBlur = 10;
        ctx.shadowColor = '#22c55e';
        ctx.fillStyle = '#22c55e';
        for (const bullet of this.bullets) {
            ctx.beginPath();
            ctx.arc(bullet.x, bullet.y, this.bulletSize / 2, 0, Math.PI * 2);
            ctx.fill();
        }
        ctx.shadowBlur = 0;

        // Draw health bars
        this.drawHealthBar(30, 20, this.tank1.health, this.tank1.color, 'P1');
        this.drawHealthBar(this.canvas.width - 180, 20, this.tank2.health, this.tank2.color, this.isOpponentBot ? '🤖 AI' : 'P2');
    }

    drawTank(tank) {
        const ctx = this.ctx;

        ctx.save();
        ctx.translate(tank.x, tank.y);
        ctx.rotate(tank.angle);

        // Tank body
        ctx.shadowBlur = 15;
        ctx.shadowColor = tank.color;
        ctx.fillStyle = tank.color;
        ctx.fillRect(-this.tankSize / 2, -this.tankSize / 2.5, this.tankSize, this.tankSize / 1.25);

        // Tank turret
        ctx.fillStyle = tank.health > 0 ? '#fff' : '#666';
        ctx.fillRect(0, -this.tankSize / 8, this.tankSize / 2 + 5, this.tankSize / 4);

        // Tank outline
        ctx.shadowBlur = 0;
        ctx.strokeStyle = '#fff';
        ctx.lineWidth = 2;
        ctx.strokeRect(-this.tankSize / 2, -this.tankSize / 2.5, this.tankSize, this.tankSize / 1.25);

        ctx.restore();
    }

    drawHealthBar(x, y, health, color, label) {
        const ctx = this.ctx;
        const barWidth = 150;
        const barHeight = 20;

        // Label
        ctx.font = '14px Orbitron';
        ctx.fillStyle = '#888';
        ctx.textAlign = 'left';
        ctx.fillText(label, x, y - 5);

        // Background
        ctx.fillStyle = '#222';
        ctx.fillRect(x, y, barWidth, barHeight);

        // Health
        ctx.fillStyle = color;
        ctx.fillRect(x, y, barWidth * (health / this.maxHealth), barHeight);

        // Border
        ctx.strokeStyle = '#444';
        ctx.lineWidth = 2;
        ctx.strokeRect(x, y, barWidth, barHeight);

        // Health text
        ctx.fillStyle = '#fff';
        ctx.textAlign = 'center';
        ctx.fillText(`${health}/${this.maxHealth}`, x + barWidth / 2, y + 15);
    }
}

window.initTank = initTank;
