#include "tcp_server_protobuf.h"
#include "hv/hlog.h"
#include "hv/htime.h"
#include <chrono>

RobotTcpServer::RobotTcpServer(DeviceManager* device_mgr, RobotWebSocketServer* ws_server)
    : device_manager_(device_mgr), ws_server_(ws_server) {
}

bool RobotTcpServer::start(int port) {
    tcp_server_.port = port;
    tcp_server_.setThreadNum(4);  // 4个工作线程
    
    // 设置回调
    tcp_server_.onConnection = [this](const SocketChannelPtr& channel) {
        onConnection(channel);
    };
    
    tcp_server_.onMessage = [this](const SocketChannelPtr& channel, Buffer* buf) {
        onMessage(channel, buf);
    };
    
    tcp_server_.onWriteComplete = [this](const SocketChannelPtr& channel, Buffer* buf) {
        onWriteComplete(channel, buf);
    };
    
    hlogi("Starting TCP server on port %d", port);
    tcp_server_.start();
    
    hlogi("TCP server started successfully");
    return true;
}

void RobotTcpServer::stop() {
    tcp_server_.stop();
    hlogi("TCP server stopped");
}

size_t RobotTcpServer::getConnectionCount() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return channel_to_device_.size();
}

void RobotTcpServer::onConnection(const SocketChannelPtr& channel) {
    std::string peeraddr = channel->peeraddr();
    
    if (channel->isConnected()) {
        hlogi("TCP client connected: %s, fd=%d", peeraddr.c_str(), channel->fd());
    } else {
        hlogi("TCP client disconnected: %s, fd=%d", peeraddr.c_str(), channel->fd());
        
        // 清理连接
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto it = channel_to_device_.find(channel->fd());
        if (it != channel_to_device_.end()) {
            std::string device_id = it->second;
            device_manager_->deviceOffline(device_id);
            channel_to_device_.erase(it);
            hlogi("Device %s marked as offline", device_id.c_str());
        }
    }
}

void RobotTcpServer::onMessage(const SocketChannelPtr& channel, Buffer* buf) {
    // 解析 Protobuf 消息
    robot::RobotMessage msg;
    
    if (!msg.ParseFromArray(buf->data(), buf->size())) {
        hloge("Failed to parse protobuf message from fd=%d", channel->fd());
        sendAck(channel, false, "Invalid protobuf message");
        return;
    }
    
    hlogd("Received message type=%d from device=%s", 
          msg.type(), msg.device_id().c_str());
    
    // 根据消息类型分发处理
    switch (msg.type()) {
        case robot::MSG_REGISTER:
            handleRegister(channel, msg);
            break;
        case robot::MSG_TELEMETRY:
            handleTelemetry(channel, msg);
            break;
        case robot::MSG_MAP_UPDATE:
            handleMapUpdate(channel, msg);
            break;
        case robot::MSG_FRONTIER_UPDATE:
            handleFrontierUpdate(channel, msg);
            break;
        case robot::MSG_YOLO_DETECTION:
            handleYoloDetection(channel, msg);
            break;
        case robot::MSG_CAMERA_FRAME:
            handleCameraFrame(channel, msg);
            break;
        default:
            hloge("Unknown message type: %d", msg.type());
            sendAck(channel, false, "Unknown message type");
            break;
    }
}

void RobotTcpServer::onWriteComplete(const SocketChannelPtr& channel, Buffer* buf) {
    // 可以在这里做流控
}

void RobotTcpServer::handleRegister(const SocketChannelPtr& channel, 
                                    const robot::RobotMessage& msg) {
    if (!msg.has_register_msg()) {
        sendAck(channel, false, "Missing register payload");
        return;
    }
    
    const auto& reg = msg.register_msg();
    std::string device_id = msg.device_id();
    
    // 转换为内部格式
    DeviceRegister internal_reg;
    snprintf(internal_reg.device_id, sizeof(internal_reg.device_id), 
             "%s", device_id.c_str());
    snprintf(internal_reg.firmware_version, sizeof(internal_reg.firmware_version), 
             "%s", reg.firmware_version().c_str());
    internal_reg.map_resolution = reg.map_resolution();
    internal_reg.has_occupancy = reg.has_occupancy() ? 1 : 0;
    internal_reg.has_semantic = reg.has_semantic() ? 1 : 0;
    
    // 注册设备
    bool success = device_manager_->registerDevice(device_id, internal_reg);
    
    if (success) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        channel_to_device_[channel->fd()] = device_id;
        hlogi("Device %s registered successfully", device_id.c_str());
    }
    
    sendAck(channel, success, success ? "Registered" : "Already exists");
}

void RobotTcpServer::handleTelemetry(const SocketChannelPtr& channel, 
                                     const robot::RobotMessage& msg) {
    if (!msg.has_telemetry()) {
        return;
    }
    
    const auto& telem_pb = msg.telemetry();
    std::string device_id = msg.device_id();
    
    // 转换为内部格式
    Telemetry telem;
    telem.timestamp = telem_pb.timestamp();
    telem.x = telem_pb.x();
    telem.y = telem_pb.y();
    telem.yaw = telem_pb.yaw();
    telem.vx = telem_pb.vx();
    telem.vy = telem_pb.vy();
    telem.w = telem_pb.w();
    telem.battery_percent = telem_pb.battery_percent();
    telem.signal_strength = telem_pb.signal_strength();
    
    // 更新设备状态
    device_manager_->updateTelemetry(device_id, telem);
    
    // 转发到 WebSocket 客户端
    if (ws_server_) {
        ws_server_->broadcast(msg);
    }
}

void RobotTcpServer::handleMapUpdate(const SocketChannelPtr& channel, 
                                     const robot::RobotMessage& msg) {
    if (!msg.has_map_update()) {
        return;
    }
    
    hlogi("Received map update from device %s", msg.device_id().c_str());
    
    // 转发到 WebSocket 客户端
    if (ws_server_) {
        ws_server_->broadcast(msg);
    }
}

void RobotTcpServer::handleFrontierUpdate(const SocketChannelPtr& channel, 
                                          const robot::RobotMessage& msg) {
    if (!msg.has_frontier_update()) {
        return;
    }
    
    hlogi("Received frontier update from device %s, frontiers=%d", 
          msg.device_id().c_str(), msg.frontier_update().frontiers_size());
    
    // 转发到 WebSocket 客户端
    if (ws_server_) {
        ws_server_->broadcast(msg);
    }
}

void RobotTcpServer::handleYoloDetection(const SocketChannelPtr& channel, 
                                         const robot::RobotMessage& msg) {
    if (!msg.has_yolo_detection()) {
        return;
    }
    
    hlogd("Received YOLO detection from device %s, boxes=%d", 
          msg.device_id().c_str(), msg.yolo_detection().boxes_size());
    
    // 转发到 WebSocket 客户端
    if (ws_server_) {
        ws_server_->broadcast(msg);
    }
}

void RobotTcpServer::handleCameraFrame(const SocketChannelPtr& channel, 
                                       const robot::RobotMessage& msg) {
    if (!msg.has_camera_frame()) {
        return;
    }
    
    hlogd("Received camera frame from device %s, size=%d bytes", 
          msg.device_id().c_str(), msg.camera_frame().jpeg_data().size());
    
    // 转发到 WebSocket 客户端（可能需要降频）
    if (ws_server_) {
        ws_server_->broadcast(msg);
    }
}

void RobotTcpServer::sendAck(const SocketChannelPtr& channel, 
                             bool success, const std::string& message) {
    robot::RobotMessage response;
    response.set_type(robot::MSG_ACK);
    response.set_timestamp(time(NULL));
    
    auto* ack = response.mutable_ack();
    ack->set_success(success);
    ack->set_message(message);
    
    std::string data;
    if (response.SerializeToString(&data)) {
        channel->write(data);
    }
}
