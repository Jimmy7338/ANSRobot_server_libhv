#include "device_monitor.h"
#include "hlog.h"

DeviceMonitor::DeviceMonitor() : check_timer_id_(hv::INVALID_TIMER_ID) {
}

DeviceMonitor::~DeviceMonitor() {
    stop();
}

void DeviceMonitor::start() {
    // 使用 TimerThread 的 setInterval 定期检查超时
    check_timer_id_ = timer_thread_.setInterval(
        CHECK_INTERVAL_MS,
        [this]() {
            check_timeouts();
        }
    );
    
    hlogi("DeviceMonitor started with %dms check interval", 
          CHECK_INTERVAL_MS);
}

void DeviceMonitor::stop() {
    if (check_timer_id_ != hv::INVALID_TIMER_ID) {
        timer_thread_.killTimer(check_timer_id_);
        check_timer_id_ = hv::INVALID_TIMER_ID;
    }
    hlogi("DeviceMonitor stopped");
}

void DeviceMonitor::register_device(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(device_mutex_);
    device_activity_[device_id] = std::chrono::system_clock::now();
    hlogi("Device %s registered in monitor", device_id.c_str());
}

void DeviceMonitor::unregister_device(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(device_mutex_);
    device_activity_.erase(device_id);
    hlogi("Device %s unregistered from monitor", device_id.c_str());
}

void DeviceMonitor::update_device_activity(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(device_mutex_);
    auto it = device_activity_.find(device_id);
    if (it != device_activity_.end()) {
        it->second = std::chrono::system_clock::now();
    }
}

void DeviceMonitor::check_timeouts() {
    std::lock_guard<std::mutex> lock(device_mutex_);
    auto now = std::chrono::system_clock::now();
    
    std::vector<std::string> timeout_devices;
    
    for (const auto& [device_id, last_activity] : device_activity_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - last_activity
        ).count();
        
        if (elapsed > TIMEOUT_SECONDS) {
            timeout_devices.push_back(device_id);
        }
    }
    
    // 标记超时设备为离线
    extern DeviceManager g_device_manager;
    for (const auto& device_id : timeout_devices) {
        g_device_manager.deviceOffline(device_id);
        device_activity_.erase(device_id);
        hlogi("Device %s timeout detected and marked offline", 
              device_id.c_str());
    }
}

size_t DeviceMonitor::online_device_count() const {
    std::lock_guard<std::mutex> lock(device_mutex_);
    return device_activity_.size();
}

std::vector<std::string> DeviceMonitor::get_online_devices() const {
    std::lock_guard<std::mutex> lock(device_mutex_);
    std::vector<std::string> result;
    for (const auto& [device_id, _] : device_activity_) {
        result.push_back(device_id);
    }
    return result;
}
