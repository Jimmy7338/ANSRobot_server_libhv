#include "hv/hlog.h"
#include "device_manager.h"
#include "tcp_server_protobuf.h"
#include "websocket_server.h"
#include <signal.h>
#include <thread>

// 全局对象
DeviceManager g_device_manager;
RobotWebSocketServer g_ws_server;
RobotTcpServer* g_tcp_server = nullptr;

static void signal_handler(int signo) {
    hlogi("Received signal %d, shutting down...", signo);
    if (g_tcp_server) {
        g_tcp_server->stop();
    }
    g_ws_server.stop();
    exit(0);
}

int main(int argc, char** argv) {
    // 初始化日志
    hlog_set_level(LOG_LEVEL_INFO);
    hlogi("Robot Backend Server Starting...");
    
    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 启动 WebSocket 服务器（端口 8080 - 浏览器展示面）
    if (!g_ws_server.start(8080)) {
        hloge("Failed to start WebSocket server");
        return -1;
    }
    
    // 启动 TCP 服务器（端口 9090 - 小车控制面）
    g_tcp_server = new RobotTcpServer(&g_device_manager, &g_ws_server);
    if (!g_tcp_server->start(9090)) {
        hloge("Failed to start TCP server");
        return -1;
    }
    
    hlogi("==============================================");
    hlogi("Robot Backend Server Started Successfully!");
    hlogi("TCP Server (Robot):      0.0.0.0:9090");
    hlogi("WebSocket Server (Web):  0.0.0.0:8080");
    hlogi("==============================================");
    
    // 主线程保持运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        // 定期打印统计信息
        hlogi("Status - TCP connections: %zu, WebSocket connections: %zu, Devices: %zu",
              g_tcp_server->getConnectionCount(),
              g_ws_server.getConnectionCount(),
              g_device_manager.listDevices().size());
    }
    
    return 0;
}
