# Dockerfile for Render.com Deployment
#
# Builds C++ engine and runs Node.js WebSocket bridge
# 
# Build: docker build -t matchmaking .
# Run:   docker run -p 3000:3000 matchmaking

FROM node:18-slim

# Install g++ for C++ compilation
RUN apt-get update && apt-get install -y g++ && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy and build C++ engine
COPY backend-cpp/ ./backend-cpp/
RUN cd backend-cpp && g++ -std=c++11 -O2 -o engine matchmaking_engine.cpp

# Copy Node.js bridge
COPY bridge/ ./bridge/

# Install Node dependencies
WORKDIR /app/bridge
RUN npm install --production

# Set engine path for Node.js
ENV ENGINE_PATH=/app/backend-cpp/engine
ENV PORT=3000

# Expose port
EXPOSE 3000

# Start Node.js server (which spawns C++ engine)
CMD ["node", "server.js"]
