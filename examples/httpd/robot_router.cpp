#include "robot_router.h"
#include "robot_handler.h"

void RobotRouter::Register(hv::HttpService& router) {
    // ========================================================================
    // 健康检查
    // ========================================================================
    // curl -v http://localhost:8080/health
    router.GET("/health", RobotHandler::health);
    
    // ========================================================================
    // 设备管理 API
    // ========================================================================
    
    // 设备注册
    // curl -X POST http://localhost:8080/api/devices/register \
    //   -H "Content-Type: application/json" \
    //   -d '{"device_id":"robot_001","firmware_version":"1.0","map_resolution":5,"has_occupancy":1,"has_semantic":0}'
    router.POST("/api/devices/register", RobotHandler::registerDevice);
    
    // 上报状态
    // curl -X POST http://localhost:8080/api/devices/robot_001/telemetry \
    //   -H "Content-Type: application/json" \
    //   -d '{"timestamp":1234567890,"x":1.5,"y":2.3,"yaw":0.5,"battery":85,"signal":95}'
    router.POST("/api/devices/:device_id/telemetry", RobotHandler::updateTelemetry);
    
    // 查询设备列表
    // curl -v http://localhost:8080/api/devices
    router.GET("/api/devices", RobotHandler::listDevices);
    
    // 查询单个设备状态
    // curl -v http://localhost:8080/api/devices/robot_001
    router.GET("/api/devices/:device_id", RobotHandler::getDeviceState);
}
