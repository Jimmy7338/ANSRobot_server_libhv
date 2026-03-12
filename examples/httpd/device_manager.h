#ifndef ROBOT_DEVICE_MANAGER_H
#define ROBOT_DEVICE_MANAGER_H

#include "protocol.h"
#include <map>
#include <mutex>
#include <chrono>
#include <memory>

// ============================================================================
// 设备信息结构
// ============================================================================

struct DeviceInfo {
    DeviceRegister reg;                                    // 注册信息
    Telemetry latest_telem;                               // 最新状态
    std::chrono::system_clock::time_point last_update;    // 最后更新时间
    bool online;                                          // 是否在线
    
    DeviceInfo() : online(false) {
        last_update = std::chrono::system_clock::now();
    }
};

// ============================================================================
// 设备管理器
// ============================================================================

class DeviceManager {
public:
    DeviceManager() = default;
    ~DeviceManager() = default;
    
    // 注册设备
    bool registerDevice(const std::string& device_id, const DeviceRegister& info);
    
    // 更新设备状态
    void updateTelemetry(const std::string& device_id, const Telemetry& telem);
    
    // 获取设备列表
    std::vector<std::string> listDevices();
    
    // 获取设备最新状态
    bool getDeviceState(const std::string& device_id, Telemetry& out);
    
    // 获取设备注册信息
    bool getDeviceInfo(const std::string& device_id, DeviceInfo& out);
    
    // 获取所有设备信息
    std::vector<DeviceInfo> getAllDevices();
    
    // 设备离线
    void deviceOffline(const std::string& device_id);
    
    // 清理离线设备（可选）
    void cleanupOfflineDevices(int timeout_seconds = 300);
    
private:
    std::map<std::string, DeviceInfo> devices_;  // device_id -> info
    std::mutex devices_mutex_;                   // 保护 devices_
};

#endif // ROBOT_DEVICE_MANAGER_H
