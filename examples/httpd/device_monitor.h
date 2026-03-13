#ifndef DEVICE_MONITOR_H
#define DEVICE_MONITOR_H

#include "evpp/TimerThread.h"
#include "device_manager.h"
#include <map>
#include <mutex>
#include <chrono>
#include <string>
#include <vector>

class DeviceMonitor {
public:
    DeviceMonitor();
    ~DeviceMonitor();
    
    // 启动监控
    void start();
    
    // 停止监控
    void stop();
    
    // 注册设备
    void register_device(const std::string& device_id);
    
    // 注销设备
    void unregister_device(const std::string& device_id);
    
    // 更新设备活动时间
    void update_device_activity(const std::string& device_id);
    
    // 获取在线设备数
    size_t online_device_count() const;
    
    // 获取所有在线设备
    std::vector<std::string> get_online_devices() const;
    
private:
    void check_timeouts();
    
    hv::TimerThread timer_thread_;
    std::map<std::string, std::chrono::system_clock::time_point> 
        device_activity_;
    mutable std::mutex device_mutex_;
    hv::TimerID check_timer_id_;
    
    static constexpr int TIMEOUT_SECONDS = 30;
    static constexpr int CHECK_INTERVAL_MS = 5000;
};

#endif // DEVICE_MONITOR_H
