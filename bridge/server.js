/**
 * Game Arena Web Server
 * 
 * Serves the ORIGINAL frontend and proxies API calls to C++ backend
 * Also handles WebSocket connections for real-time multiplayer sync.
 * 
 * Architecture:
 *   Browser → Node.js (port 3000) → C++ Backend (port 8080)
 *                ↓
 *         Serves frontend/index.html
 */

const express = require('express');
const { spawn, execSync } = require('child_process');
const path = require('path');
const http = require('http');
const fs = require('fs');
const WebSocket = require('ws');

const PORT = process.env.PORT || 3000;
const CPP_PORT = 8080;
const isWindows = process.platform === 'win32';
// On Linux (Docker), the executable is 'server', on Windows it's 'server.exe'
const serverExe = isWindows ? 'server.exe' : 'server';

console.log('[Server] Platform:', process.platform);
console.log('[Server] Using executable:', serverExe);

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// ============== WEBSOCKET GAME RELAY ==============

// Map to store match rooms: string (matchId) -> Set<WebSocket>
const matchRooms = new Map();

wss.on('connection', (ws) => {
    console.log('[WS] Client connected');
    let currentMatchId = null;

    ws.on('message', (message) => {
        try {
            const data = JSON.parse(message);

            if (data.type === 'JOIN_MATCH') {
                const matchId = data.matchId.toString();
                currentMatchId = matchId;

                if (!matchRooms.has(matchId)) {
                    matchRooms.set(matchId, new Set());
                }

                const room = matchRooms.get(matchId);
                room.add(ws);
                console.log(`[WS] Client joined match ${matchId} (Total: ${room.size})`);

                // Notify others in room
                room.forEach(client => {
                    if (client !== ws && client.readyState === WebSocket.OPEN) {
                        client.send(JSON.stringify({ type: 'PLAYER_JOINED' }));
                    }
                });
            }
            else if (data.type === 'GAME_STATE' || data.type === 'SCORE_UPDATE') {
                if (currentMatchId && matchRooms.has(currentMatchId)) {
                    // Relay to OTHER players in the same match
                    const room = matchRooms.get(currentMatchId);
                    room.forEach(client => {
                        if (client !== ws && client.readyState === WebSocket.OPEN) {
                            client.send(message.toString());
                        }
                    });
                }
            }
        } catch (err) {
            console.error('[WS] Error:', err);
        }
    });

    ws.on('close', () => {
        if (currentMatchId && matchRooms.has(currentMatchId)) {
            const room = matchRooms.get(currentMatchId);
            room.delete(ws);

            if (room.size === 0) {
                matchRooms.delete(currentMatchId);
            } else {
                // Notify remaining player
                room.forEach(client => {
                    if (client.readyState === WebSocket.OPEN) {
                        client.send(JSON.stringify({ type: 'PLAYER_LEFT' }));
                    }
                });
            }
            console.log(`[WS] Client left match ${currentMatchId}`);
        }
    });
});

// ============== START C++ BACKEND ==============

let cppProcess = null;
let cppStarted = false;

function startCppBackend() {
    const cppPath = path.join(__dirname, '..', 'backend-cpp', serverExe);
    console.log('[Server] Starting C++ backend from:', cppPath);

    // Check if file exists
    if (!fs.existsSync(cppPath)) {
        console.error('[Server] ERROR: C++ executable not found at:', cppPath);
        console.log('[Server] Directory contents:', fs.readdirSync(path.join(__dirname, '..', 'backend-cpp')));
        return;
    }

    // On Linux, make sure it's executable
    if (!isWindows) {
        try {
            execSync(`chmod +x "${cppPath}"`);
            console.log('[Server] Made executable with chmod +x');
        } catch (e) {
            console.error('[Server] chmod failed:', e.message);
        }
    }

    console.log('[Server] Spawning C++ process...');

    cppProcess = spawn(cppPath, [], {
        cwd: path.join(__dirname, '..', 'backend-cpp'),
        stdio: ['ignore', 'pipe', 'pipe']
    });

    cppProcess.stdout.on('data', (data) => {
        console.log('[C++ Backend]', data.toString().trim());
        cppStarted = true;
    });

    cppProcess.stderr.on('data', (data) => {
        console.error('[C++ Backend Error]', data.toString().trim());
    });

    cppProcess.on('error', (err) => {
        console.error('[Server] Failed to start C++ backend:', err.message);
    });

    cppProcess.on('exit', (code, signal) => {
        console.log('[Server] C++ backend exited with code:', code, 'signal:', signal);
        cppStarted = false;
        // Restart after delay
        setTimeout(startCppBackend, 3000);
    });

    // Check if process started
    setTimeout(() => {
        if (cppProcess && cppProcess.pid) {
            console.log('[Server] C++ process PID:', cppProcess.pid);
        } else {
            console.error('[Server] C++ process failed to start');
        }
    }, 1000);
}

// ============== PROXY API CALLS TO C++ ==============

app.use('/api', (req, res) => {
    const options = {
        hostname: '127.0.0.1',  // Use IPv4 explicitly, not 'localhost'
        port: CPP_PORT,
        path: '/api' + req.url,
        method: req.method,
        headers: {
            'Content-Type': 'application/json'
        }
    };

    const proxyReq = http.request(options, (proxyRes) => {
        res.writeHead(proxyRes.statusCode, proxyRes.headers);
        proxyRes.pipe(res);
    });

    proxyReq.on('error', (err) => {
        console.error('[Proxy Error]', err.message);
        res.status(503).json({ error: 'Backend unavailable', started: cppStarted });
    });

    if (req.method === 'POST' || req.method === 'PUT') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
            proxyReq.write(body);
            proxyReq.end();
        });
    } else {
        proxyReq.end();
    }
});

// ============== SERVE ORIGINAL FRONTEND ==============

app.use(express.static(path.join(__dirname, '..', 'frontend')));

// Fallback to index.html
app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, '..', 'frontend', 'index.html'));
});

// ============== START SERVER ==============

// Start C++ backend first
startCppBackend();

// Wait a bit for C++ to start, then start web server
setTimeout(() => {
    // Note: Use 'server.listen' instead of 'app.listen' to include WebSocket upgrades
    server.listen(PORT, '0.0.0.0', () => {
        console.log('=========================================');
        console.log('  🎮 GAME ARENA - Online');
        console.log('=========================================');
        console.log(`  Website: http://0.0.0.0:${PORT}`);
        console.log('  C++ API: http://127.0.0.1:' + CPP_PORT);
        console.log('=========================================');
    });
}, 3000);

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\n[Server] Shutting down...');
    if (cppProcess) cppProcess.kill();
    process.exit(0);
});
