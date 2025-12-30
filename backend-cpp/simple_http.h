/**
 * Simple HTTP Server - Compatible with MinGW GCC 6.x
 * 
 * This is a minimal HTTP implementation for the DSA project.
 * Works with older compilers that don't support modern cpp-httplib.
 */

#ifndef SIMPLE_HTTP_H
#define SIMPLE_HTTP_H

#ifdef _WIN32
    #define _WIN32_WINNT 0x0501
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <cstring>
#include <cstdio>

namespace http {

struct Request {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> params;
    
    // URL path parameter (e.g., /api/players/123 -> matches[1] = "123")
    std::string matches[10];
};

struct Response {
    int status = 200;
    std::string body;
    std::string content_type = "application/json";
    
    void set_content(const std::string& content, const std::string& type) {
        body = content;
        content_type = type;
    }
};

typedef std::function<void(const Request&, Response&)> Handler;

struct Route {
    std::string method;
    std::string pattern;
    Handler handler;
    bool is_regex;
};

class Server {
private:
    SOCKET server_socket;
    std::vector<Route> routes;
    bool running;
    
    bool match_route(const std::string& pattern, const std::string& path, Request& req) {
        // Simple pattern matching for paths like /api/players/:id
        if (pattern.find("(") != std::string::npos) {
            // Regex-like pattern: /api/players/(\d+)
            size_t paren_start = pattern.find("(");
            std::string prefix = pattern.substr(0, paren_start);
            
            if (path.substr(0, prefix.length()) == prefix) {
                std::string rest = path.substr(prefix.length());
                // Find end of captured group
                size_t end = rest.find('/');
                if (end == std::string::npos) end = rest.length();
                req.matches[1] = rest.substr(0, end);
                return true;
            }
            return false;
        }
        return pattern == path;
    }
    
    void parse_request(const char* buffer, Request& req) {
        std::string request(buffer);
        
        // Parse first line: GET /path HTTP/1.1
        size_t method_end = request.find(' ');
        req.method = request.substr(0, method_end);
        
        size_t path_start = method_end + 1;
        size_t path_end = request.find(' ', path_start);
        req.path = request.substr(path_start, path_end - path_start);
        
        // Parse body (after double newline)
        size_t body_start = request.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            req.body = request.substr(body_start + 4);
        }
    }
    
    std::string build_response(const Response& res) {
        std::string response;
        response += "HTTP/1.1 " + std::to_string(res.status) + " OK\r\n";
        response += "Content-Type: " + res.content_type + "\r\n";
        response += "Content-Length: " + std::to_string(res.body.length()) + "\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
        response += "Access-Control-Allow-Headers: Content-Type\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += res.body;
        return response;
    }
    
    void handle_client(SOCKET client_socket) {
        char buffer[8192] = {0};
        int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes > 0) {
            Request req;
            Response res;
            
            parse_request(buffer, req);
            
            // Handle CORS preflight
            if (req.method == "OPTIONS") {
                res.status = 204;
                res.body = "";
            } else {
                // Find matching route
                bool found = false;
                for (const auto& route : routes) {
                    if (route.method == req.method && match_route(route.pattern, req.path, req)) {
                        route.handler(req, res);
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    res.status = 404;
                    res.body = "{\"error\":\"Not found\"}";
                }
            }
            
            std::string response = build_response(res);
            send(client_socket, response.c_str(), response.length(), 0);
        }
        
        closesocket(client_socket);
    }

public:
    Server() : server_socket(INVALID_SOCKET), running(false) {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    }
    
    ~Server() {
        if (server_socket != INVALID_SOCKET) {
            closesocket(server_socket);
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    void Get(const std::string& pattern, Handler handler) {
        routes.push_back({"GET", pattern, handler, pattern.find("(") != std::string::npos});
    }
    
    void Post(const std::string& pattern, Handler handler) {
        routes.push_back({"POST", pattern, handler, pattern.find("(") != std::string::npos});
    }
    
    void Options(const std::string& pattern, Handler handler) {
        routes.push_back({"OPTIONS", pattern, handler, false});
    }
    
    bool listen(const char* host, int port) {
        server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_socket == INVALID_SOCKET) {
            printf("Failed to create socket\n");
            return false;
        }
        
        // Allow address reuse
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            printf("Failed to bind to port %d\n", port);
            return false;
        }
        
        if (::listen(server_socket, 10) == SOCKET_ERROR) {
            printf("Failed to listen\n");
            return false;
        }
        
        running = true;
        
        while (running) {
            struct sockaddr_in client_addr;
            int client_len = sizeof(client_addr);
            
            SOCKET client = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (client != INVALID_SOCKET) {
                handle_client(client);
            }
        }
        
        return true;
    }
    
    void stop() {
        running = false;
    }
};

} // namespace http

#endif // SIMPLE_HTTP_H
