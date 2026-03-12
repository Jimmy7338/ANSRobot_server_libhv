#ifndef ROBOT_HANDLER_H
#define ROBOT_HANDLER_H

#include "HttpService.h"
#include "device_manager.h"

class RobotHandler {
public:
    // 设备注册
    static int registerDevice(const HttpContextPtr& ctx);
    
    // 上报状态
    static int updateTelemetry(const HttpContextPtr& ctx);
    
    // 查询设备列表
    static int listDevices(const HttpContextPtr& ctx);
    
    // 查询单个设备状态
    static int getDeviceState(const HttpContextPtr& ctx);
    
    // 健康检查
    static int health(const HttpContextPtr& ctx);
};

#endif // ROBOT_HANDLER_H
