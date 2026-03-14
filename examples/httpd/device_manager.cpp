#include "device_manager.h"
#include "hv/hlog.h"
#include "protocol.h"

bool DeviceManager::registerDevice(const std::string& device_id, const DeviceRegister& info) {
    // 验证 device_id
    if (!isValidDeviceId(device_id)) {
        hloge("Invalid device_id format: %s", device_id.c_str());
        return false;
    }
    
    // 验证 firmware_version
    if (!isValidFirmwareVersion(info.firmware_version)) {
        hloge("Invalid firmware_version: %s", info.firmware_version);
        return false;
    }
    
    // 验证 map_resolution
    if (!isValidMapResolution(info.map_resolution)) {
        hloge("Invalid map_resolution: %d", info.map_resolution);
        return false;
    }
    
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    // 检查设备是否已存在
    if (devices_.find(device_id) != devices_.end()) {
        hlogi("Device %s already registered", device_id.c_str());
        return false;
    }
    
    // 创建新设备
    DeviceInfo dev_info;
    dev_info.reg = info;
    dev_info.online = true;
    dev_info.last_update = std::chrono::system_clock::now();
    
    devices_[device_id] = dev_info;
    
    hlogi("Device %s registered: firmware=%s, map_res=%d, occupancy=%d, semantic=%d",
          device_id.c_str(),
          info.firmware_version,
          info.map_resolution,
          info.has_occupancy,
          info.has_semantic);
    
    return true;
}

void DeviceManager::updateTelemetry(const std::string& device_id, const Telemetry& telem) {
    // 验证 device_id
    if (!isValidDeviceId(device_id)) {
        hloge("Invalid device_id format: %s", device_id.c_str());
        return;
    }
    
    // 验证 telemetry 数据
    if (!telem.isValid()) {
        hloge("Invalid telemetry data for device %s", device_id.c_str());
        return;
    }
    
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = devices_.find(device_id);
    if (it == devices_.end()) {
        hlogw("Device %s not found, creating new entry", device_id.c_str());
        DeviceInfo dev_info;
        dev_info.online = true;
        dev_info.latest_telem = telem;
        dev_info.last_update = std::chrono::system_clock::now();
        devices_[device_id] = dev_info;
    } else {
        it->second.latest_telem = telem;
        it->second.last_update = std::chrono::system_clock::now();
        it->second.online = true;
    }
    
    hlogd("Device %s telemetry: x=%.2f y=%.2f yaw=%.2f battery=%d",
          device_id.c_str(),
          telem.x, telem.y, telem.yaw,
          telem.battery_percent);
}

std::vector<std::string> DeviceManager::listDevices() {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    std::vector<std::string> result;
    for (const auto& [device_id, info] : devices_) {
        result.push_back(device_id);
    }
    return result;
}

bool DeviceManager::getDeviceState(const std::string& device_id, Telemetry& out) {
    // 验证 device_id
    if (!isValidDeviceId(device_id)) {
        hloge("Invalid device_id format: %s", device_id.c_str());
        return false;
    }
    
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = devices_.find(device_id);
    if (it == devices_.end()) {
        return false;
    }
    
    out = it->second.latest_telem;
    return true;
}

bool DeviceManager::getDeviceInfo(const std::string& device_id, DeviceInfo& out) {
    // 验证 device_id
    if (!isValidDeviceId(device_id)) {
        hloge("Invalid device_id format: %s", device_id.c_str());
        return false;
    }
    
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = devices_.find(device_id);
    if (it == devices_.end()) {
        return false;
    }
    
    out = it->second;
    return true;
}

std::vector<DeviceInfo> DeviceManager::getAllDevices() {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    std::vector<DeviceInfo> result;
    for (const auto& [device_id, info] : devices_) {
        result.push_back(info);
    }
    return result;
}

void DeviceManager::deviceOffline(const std::string& device_id) {
    // 验证 device_id
    if (!isValidDeviceId(device_id)) {
        hloge("Invalid device_id format: %s", device_id.c_str());
        return;
    }
    
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = devices_.find(device_id);
    if (it != devices_.end()) {
        it->second.online = false;
        hlogi("Device %s marked offline", device_id.c_str());
    }
}

void DeviceManager::cleanupOfflineDevices(int timeout_seconds) {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto timeout_duration = std::chrono::seconds(timeout_seconds);
    
    std::vector<std::string> to_remove;
    for (const auto& [device_id, info] : devices_) {
        if (!info.online) {
            auto elapsed = now - info.last_update;
            if (elapsed > timeout_duration) {
                to_remove.push_back(device_id);
            }
        }
    }
    
    for (const auto& device_id : to_remove) {
        devices_.erase(device_id);
        hlogi("Removed offline device %s", device_id.c_str());
    }
}
