/**
 * WebSocket Bridge Server
 * 
 * PURPOSE:
 * Acts as a translator between browser WebSocket clients and the C++ matchmaking engine.
 * 
 * ARCHITECTURE:
 *   Browser --[WebSocket]--> Node.js --[stdin/stdout]--> C++ Engine
 * 
 * NO MATCHMAKING LOGIC HERE - just message routing!
 */

const WebSocket = require('ws');
const { spawn } = require('child_process');
const path = require('path');
const express = require('express');
const http = require('http');

// Configuration
const PORT = process.env.PORT || 3000;
const isWindows = process.platform === 'win32';
const engineName = isWindows ? 'engine.exe' : 'engine';
const ENGINE_PATH = process.env.ENGINE_PATH || path.join(__dirname, '..', 'backend-cpp', engineName);

// ============== EXPRESS SERVER (for static files) ==============

const app = express();
app.use(express.static(path.join(__dirname, 'public')));

// Health check endpoint
app.get('/health', (req, res) => {
    res.json({ status: 'ok', engine: engineProcess ? 'running' : 'stopped' });
});

const server = http.createServer(app);

// ============== WEBSOCKET SERVER ==============

const wss = new WebSocket.Server({ server });

// Client tracking: clientId -> WebSocket
const clients = new Map();
let clientCounter = 0;

// ============== C++ ENGINE PROCESS ==============

let engineProcess = null;
let engineBuffer = '';

function startEngine() {
    console.log('[Bridge] Starting C++ engine from:', ENGINE_PATH);

    engineProcess = spawn(ENGINE_PATH, [], {
        stdio: ['pipe', 'pipe', 'pipe']
    });

    engineProcess.on('error', (err) => {
        console.error('[Bridge] Engine failed to start:', err.message);
    });

    engineProcess.on('exit', (code) => {
        console.log('[Bridge] Engine exited with code:', code);
        // Restart after delay
        setTimeout(startEngine, 1000);
    });

    // Handle engine stdout (responses)
    engineProcess.stdout.on('data', (data) => {
        engineBuffer += data.toString();
        processEngineOutput();
    });

    // Handle engine stderr (logs)
    engineProcess.stderr.on('data', (data) => {
        console.log('[Engine]', data.toString().trim());
    });
}

function processEngineOutput() {
    // Process complete lines from engine
    const lines = engineBuffer.split('\n');

    // Keep incomplete last line in buffer
    engineBuffer = lines.pop() || '';

    for (const line of lines) {
        if (!line.trim()) continue;

        try {
            const response = JSON.parse(line);
            routeResponse(response);
        } catch (err) {
            console.error('[Bridge] Failed to parse engine response:', line);
        }
    }
}

function routeResponse(response) {
    const clientId = response.clientId;

    if (!clientId) {
        console.error('[Bridge] Response missing clientId:', response);
        return;
    }

    const ws = clients.get(clientId);
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(response));
    } else {
        console.log('[Bridge] Client not found for response:', clientId);
    }
}

function sendToEngine(message) {
    if (engineProcess && engineProcess.stdin.writable) {
        engineProcess.stdin.write(JSON.stringify(message) + '\n');
    } else {
        console.error('[Bridge] Engine not available');
    }
}

// ============== WEBSOCKET HANDLERS ==============

wss.on('connection', (ws) => {
    // Generate unique client ID
    const clientId = `ws-${++clientCounter}-${Date.now()}`;
    clients.set(clientId, ws);

    console.log('[Bridge] Client connected:', clientId);

    // Send client their ID
    ws.send(JSON.stringify({ type: 'CONNECTED', clientId }));

    // Handle messages from browser
    ws.on('message', (data) => {
        try {
            const message = JSON.parse(data.toString());

            // Inject clientId into message
            message.clientId = clientId;

            console.log('[Bridge] Received:', message.cmd, 'from', clientId);

            // Forward to C++ engine
            sendToEngine(message);

        } catch (err) {
            console.error('[Bridge] Invalid message from client:', err.message);
            ws.send(JSON.stringify({ type: 'ERROR', message: 'Invalid message format' }));
        }
    });

    // Handle disconnect
    ws.on('close', () => {
        console.log('[Bridge] Client disconnected:', clientId);

        // Notify engine about disconnect
        sendToEngine({ cmd: 'DISCONNECT', clientId });

        // Remove from tracking
        clients.delete(clientId);
    });

    ws.on('error', (err) => {
        console.error('[Bridge] WebSocket error for', clientId, ':', err.message);
    });
});

// ============== STARTUP ==============

// Start C++ engine
startEngine();

// Start HTTP/WebSocket server
server.listen(PORT, () => {
    console.log('=========================================');
    console.log('  Matchmaking WebSocket Bridge');
    console.log('=========================================');
    console.log(`  HTTP:      http://localhost:${PORT}`);
    console.log(`  WebSocket: ws://localhost:${PORT}`);
    console.log('=========================================');
});

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\n[Bridge] Shutting down...');

    if (engineProcess) {
        engineProcess.kill();
    }

    wss.clients.forEach(ws => ws.close());
    server.close();

    process.exit(0);
});
