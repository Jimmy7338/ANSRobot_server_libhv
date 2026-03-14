#ifndef TCP_SERVER_PROTOBUF_H
#define TCP_SERVER_PROTOBUF_H

#include "hv/TcpServer.h"
#include "hv/hlog.h"
#include "proto/robot.pb.h"
#include "device_manager.h"
#include "websocket_server.h"
#include <map>
#include <mutex>

using namespace hv;

// TCP 服务器，用于与小车进行高频 Protobuf 通信
class RobotTcpServer {
public:
    RobotTcpServer(DeviceManager* device_mgr, RobotWebSocketServer* ws_server);
    ~RobotTcpServer() = default;
    
    // 启动 TCP 服务器
    bool start(int port = 9090);
    
    // 停止服务器
    void stop();
    
    // 获取连接数
    size_t getConnectionCount() const;
    
private:
    void onConnection(const SocketChannelPtr& channel);
    void onMessage(const SocketChannelPtr& channel, Buffer* buf);
    void onWriteComplete(const SocketChannelPtr& channel, Buffer* buf);
    
    // 处理不同类型的消息
    void handleRegister(const SocketChannelPtr& channel, const robot::RobotMessage& msg);
    void handleTelemetry(const SocketChannelPtr& channel, const robot::RobotMessage& msg);
    void handleMapUpdate(const SocketChannelPtr& channel, const robot::RobotMessage& msg);
    void handleFrontierUpdate(const SocketChannelPtr& channel, const robot::RobotMessage& msg);
    void handleYoloDetection(const SocketChannelPtr& channel, const robot::RobotMessage& msg);
    void handleCameraFrame(const SocketChannelPtr& channel, const robot::RobotMessage& msg);
    
    // 发送响应
    void sendAck(const SocketChannelPtr& channel, bool success, const std::string& message);
    
    TcpServer tcp_server_;
    DeviceManager* device_manager_;
    RobotWebSocketServer* ws_server_;
    
    // 连接管理
    std::map<int, std::string> channel_to_device_;  // channel_id -> device_id
    mutable std::mutex connections_mutex_;
};

#endif // TCP_SERVER_PROTOBUF_H
