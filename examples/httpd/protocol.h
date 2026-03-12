#ifndef ROBOT_PROTOCOL_H
#define ROBOT_PROTOCOL_H

#include <string>
#include <vector>
#include <cstring>
#include <regex>

// ============================================================================
// 常量定义
// ============================================================================

// 字段长度限制
constexpr int MAX_DEVICE_ID_LEN = 64;
constexpr int MAX_FIRMWARE_VERSION_LEN = 32;
constexpr int MAX_REQUEST_SIZE = 1024 * 1024;  // 1MB

// 数值范围限制
constexpr int MIN_MAP_RESOLUTION = 1;
constexpr int MAX_MAP_RESOLUTION = 100;
constexpr int MIN_BATTERY_PERCENT = 0;
constexpr int MAX_BATTERY_PERCENT = 100;
constexpr int MIN_SIGNAL_STRENGTH = 0;
constexpr int MAX_SIGNAL_STRENGTH = 100;

// ============================================================================
// 消息类型定义
// ============================================================================

enum MessageType {
    MSG_REGISTER = 1,      // 车注册
    MSG_TELEMETRY = 2,     // 状态上报（高频）
    MSG_MAP_UPDATE = 3,    // 地图更新（中频）
    MSG_ACK = 100,         // 确认
};

// ============================================================================
// 验证函数
// ============================================================================

inline bool isValidDeviceId(const std::string& device_id) {
    if (device_id.empty() || device_id.length() >= MAX_DEVICE_ID_LEN) {
        return false;
    }
    // 只允许字母、数字、下划线、连字符
    static const std::regex valid_pattern("^[a-zA-Z0-9_-]+$");
    return std::regex_match(device_id, valid_pattern);
}

inline bool isValidFirmwareVersion(const std::string& version) {
    if (version.empty() || version.length() >= MAX_FIRMWARE_VERSION_LEN) {
        return false;
    }
    // 允许版本号格式如 1.0, 1.2.3 等
    static const std::regex valid_pattern("^[0-9]+(\\.[0-9]+)*$");
    return std::regex_match(version, valid_pattern);
}

inline bool isValidMapResolution(int resolution) {
    return resolution >= MIN_MAP_RESOLUTION && resolution <= MAX_MAP_RESOLUTION;
}

inline bool isValidBatteryPercent(int percent) {
    return percent >= MIN_BATTERY_PERCENT && percent <= MAX_BATTERY_PERCENT;
}

inline bool isValidSignalStrength(int strength) {
    return strength >= MIN_SIGNAL_STRENGTH && strength <= MAX_SIGNAL_STRENGTH;
}

// ============================================================================
// 数据结构定义
// ============================================================================

// 车的注册信息
struct DeviceRegister {
    char device_id[MAX_DEVICE_ID_LEN];           // 设备ID
    char firmware_version[MAX_FIRMWARE_VERSION_LEN];    // 固件版本
    int map_resolution;           // 地图分辨率（cm/pixel）
    int has_occupancy;            // 是否支持占用栅格
    int has_semantic;             // 是否支持语义
    
    DeviceRegister() {
        memset(device_id, 0, sizeof(device_id));
        memset(firmware_version, 0, sizeof(firmware_version));
        map_resolution = 5;
        has_occupancy = 1;
        has_semantic = 0;
    }
};

// 车的实时状态
struct Telemetry {
    double timestamp;      // 时间戳
    double x, y, yaw;      // 位姿 (m, m, rad)
    double vx, vy, w;      // 速度 (m/s, m/s, rad/s)
    int battery_percent;   // 电池百分比
    int signal_strength;   // 信号强度 (0-100)
    
    Telemetry() {
        timestamp = 0;
        x = y = yaw = 0;
        vx = vy = w = 0;
        battery_percent = 100;
        signal_strength = 100;
    }
    
    // 验证数据有效性
    bool isValid() const {
        return isValidBatteryPercent(battery_percent) && 
               isValidSignalStrength(signal_strength);
    }
};

// 地图块（增量）
struct MapUpdate {
    double timestamp;                    // 时间戳
    int block_x, block_y;               // 块坐标
    int width, height;                  // 块大小 (pixels)
    int data_size;                      // 数据大小
    std::vector<uint8_t> occupancy_data; // 压缩的占用栅格
    
    MapUpdate() {
        timestamp = 0;
        block_x = block_y = 0;
        width = height = 0;
        data_size = 0;
    }
};

// ============================================================================
// 序列化/反序列化辅助函数
// ============================================================================

inline std::string serializeTelemetry(const Telemetry& telem) {
    std::string result;
    result.resize(sizeof(Telemetry));
    memcpy((void*)result.data(), &telem, sizeof(Telemetry));
    return result;
}

inline bool deserializeTelemetry(const std::string& data, Telemetry& telem) {
    if (data.size() != sizeof(Telemetry)) return false;
    memcpy(&telem, data.data(), sizeof(Telemetry));
    return true;
}

#endif // ROBOT_PROTOCOL_H
