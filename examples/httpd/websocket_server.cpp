#include "websocket_server.h"
#include "hv/hlog.h"
#include "hv/htime.h"
#include <google/protobuf/util/json_util.h>

RobotWebSocketServer::RobotWebSocketServer() {
    // 设置 WebSocket 回调
    ws_service_.onopen = [this](const WebSocketChannelPtr& channel, const HttpRequestPtr& req) {
        std::string client_ip = channel->peeraddr();
        hlogi("WebSocket client connected: %s", client_ip.c_str());
        
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            clients_.insert(channel);
        }
        
        // 发送欢迎消息
        robot::RobotMessage welcome;
        welcome.set_type(robot::MSG_ACK);
        
        // 使用当前时间戳
        welcome.set_timestamp(time(NULL));
        
        auto* ack = welcome.mutable_ack();
        ack->set_success(true);
        ack->set_message("Connected to robot backend");
        
        std::string data;
        if (welcome.SerializeToString(&data)) {
            channel->send(data, WS_OPCODE_BINARY);
        }
        
        hlogi("Total WebSocket connections: %zu", getConnectionCount());
    };
    
    ws_service_.onmessage = [this](const WebSocketChannelPtr& channel, const std::string& msg) {
        hlogd("Received message from client: %s", msg.c_str());
        // 这里可以处理来自前端的控制命令
    };
    
    ws_service_.onclose = [this](const WebSocketChannelPtr& channel) {
        std::string client_ip = channel->peeraddr();
        hlogi("WebSocket client disconnected: %s", client_ip.c_str());
        
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            clients_.erase(channel);
        }
        
        hlogi("Total WebSocket connections: %zu", getConnectionCount());
    };
}

bool RobotWebSocketServer::start(int port) {
    ws_server_.port = port;
    ws_server_.registerWebSocketService(&ws_service_);
    
    hlogi("Starting WebSocket server on port %d", port);
    ws_server_.start();
    
    hlogi("WebSocket server started successfully");
    return true;
}

void RobotWebSocketServer::stop() {
    ws_server_.stop();
    hlogi("WebSocket server stopped");
}

void RobotWebSocketServer::broadcast(const robot::RobotMessage& msg) {
    // 序列化为二进制
    std::string binary_data;
    if (!msg.SerializeToString(&binary_data)) {
        hloge("Failed to serialize protobuf message");
        return;
    }
    
    broadcastBinary(binary_data);
}

void RobotWebSocketServer::broadcastBinary(const std::string& data) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    for (const auto& client : clients_) {
        if (client && client->isConnected()) {
            client->send(data, WS_OPCODE_BINARY);
        }
    }
}

void RobotWebSocketServer::broadcastText(const std::string& text) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    for (const auto& client : clients_) {
        if (client && client->isConnected()) {
            client->send(text, WS_OPCODE_TEXT);
        }
    }
}

size_t RobotWebSocketServer::getConnectionCount() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size();
}
