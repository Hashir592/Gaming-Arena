# Dockerfile for Game Arena
# Runs the original frontend + C++ backend

FROM node:18-slim

# Install g++ for C++ compilation
RUN apt-get update && apt-get install -y g++ && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy and build C++ server (the original HTTP server)
COPY backend-cpp/ ./backend-cpp/
RUN cd backend-cpp && g++ -std=c++11 -O2 -pthread -o server server_compat.cpp

# Copy Node.js bridge
COPY bridge/package*.json ./bridge/
RUN cd bridge && npm install --production

COPY bridge/ ./bridge/

# Copy original frontend
COPY frontend/ ./frontend/

# Port
ENV PORT=3000
EXPOSE 3000

# Start Node.js (which spawns C++ backend)
WORKDIR /app/bridge
CMD ["node", "server.js"]
