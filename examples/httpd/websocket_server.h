#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include "hv/WebSocketServer.h"
#include "hv/hlog.h"
#include "proto/robot.pb.h"
#include <set>
#include <mutex>
#include <memory>

using namespace hv;

// WebSocket 服务器，用于向浏览器推送数据
class RobotWebSocketServer {
public:
    RobotWebSocketServer();
    ~RobotWebSocketServer() = default;
    
    // 启动 WebSocket 服务器
    bool start(int port = 8080);
    
    // 停止服务器
    void stop();
    
    // 广播消息给所有连接的客户端
    void broadcast(const robot::RobotMessage& msg);
    void broadcastBinary(const std::string& data);
    void broadcastText(const std::string& text);
    
    // 获取连接数
    size_t getConnectionCount() const;
    
private:
    WebSocketServer ws_server_;
    WebSocketService ws_service_;
    std::set<WebSocketChannelPtr> clients_;
    mutable std::mutex clients_mutex_;
};

#endif // WEBSOCKET_SERVER_H
