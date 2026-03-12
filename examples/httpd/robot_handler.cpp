#include "robot_handler.h"
#include "hlog.h"
#include <ctime>

// 全局设备管理器实例
extern DeviceManager g_device_manager;

int RobotHandler::health(const HttpContextPtr& ctx) {
    ctx->setContentType("application/json");
    ctx->set("status", "ok");
    ctx->set("timestamp", (long long)std::time(nullptr));
    return ctx->send();
}

int RobotHandler::registerDevice(const HttpContextPtr& ctx) {
    try {
        // 检查请求体大小
        if (ctx->body.size() > MAX_REQUEST_SIZE) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Request body too large");
            return ctx->send();
        }
        
        // 获取 JSON 对象
        auto json = ctx->json();
        
        // 验证必需字段存在
        if (!json.contains("device_id") || !json.contains("firmware_version")) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Missing required fields: device_id, firmware_version");
            return ctx->send();
        }
        
        // 提取并验证 device_id
        std::string device_id_str = json["device_id"].is_string() ? 
                                    json["device_id"].get<std::string>() : 
                                    json["device_id"].dump();
        
        // 移除 JSON 引号（如果有）
        if (!device_id_str.empty() && device_id_str.front() == '"' && device_id_str.back() == '"') {
            device_id_str = device_id_str.substr(1, device_id_str.length() - 2);
        }
        
        if (!isValidDeviceId(device_id_str)) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid device_id format (must be alphanumeric, underscore, or hyphen)");
            return ctx->send();
        }
        
        // 提取并验证 firmware_version
        std::string fw_version_str = json["firmware_version"].is_string() ? 
                                     json["firmware_version"].get<std::string>() : 
                                     json["firmware_version"].dump();
        
        if (!fw_version_str.empty() && fw_version_str.front() == '"' && fw_version_str.back() == '"') {
            fw_version_str = fw_version_str.substr(1, fw_version_str.length() - 2);
        }
        
        if (!isValidFirmwareVersion(fw_version_str)) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid firmware_version format");
            return ctx->send();
        }
        
        DeviceRegister reg;
        
        // 安全复制字符串（使用 snprintf 替代 strncpy）
        snprintf(reg.device_id, sizeof(reg.device_id), "%s", device_id_str.c_str());
        snprintf(reg.firmware_version, sizeof(reg.firmware_version), "%s", fw_version_str.c_str());
        
        // 验证并设置 map_resolution
        reg.map_resolution = json["map_resolution"].is_null() ? 5 : (int)json["map_resolution"];
        if (!isValidMapResolution(reg.map_resolution)) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid map_resolution (must be 1-100)");
            return ctx->send();
        }
        
        reg.has_occupancy = json["has_occupancy"].is_null() ? 1 : (int)json["has_occupancy"];
        reg.has_semantic = json["has_semantic"].is_null() ? 0 : (int)json["has_semantic"];
        
        // 调用设备管理器
        bool ok = g_device_manager.registerDevice(reg.device_id, reg);
        
        ctx->setContentType("application/json");
        ctx->set("code", ok ? 0 : -1);
        ctx->set("message", ok ? "OK" : "Device already exists");
        ctx->set("device_id", reg.device_id);
        
        return ctx->send();
    } catch (const std::exception& e) {
        ctx->setContentType("application/json");
        ctx->set("code", -1);
        ctx->set("message", std::string("Error: ") + e.what());
        return ctx->send();
    }
}

int RobotHandler::updateTelemetry(const HttpContextPtr& ctx) {
    try {
        // 检查请求体大小
        if (ctx->body.size() > MAX_REQUEST_SIZE) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Request body too large");
            return ctx->send();
        }
        
        // 获取并验证 device_id
        std::string device_id = ctx->param("device_id");
        if (!isValidDeviceId(device_id)) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid device_id format");
            return ctx->send();
        }
        
        // 获取 JSON 对象
        auto json = ctx->json();
        
        Telemetry telem;
        telem.timestamp = json["timestamp"].is_null() ? 0.0 : (double)json["timestamp"];
        telem.x = json["x"].is_null() ? 0.0 : (double)json["x"];
        telem.y = json["y"].is_null() ? 0.0 : (double)json["y"];
        telem.yaw = json["yaw"].is_null() ? 0.0 : (double)json["yaw"];
        telem.vx = json["vx"].is_null() ? 0.0 : (double)json["vx"];
        telem.vy = json["vy"].is_null() ? 0.0 : (double)json["vy"];
        telem.w = json["w"].is_null() ? 0.0 : (double)json["w"];
        
        // 验证并设置 battery_percent
        int battery = json["battery"].is_null() ? 100 : (int)json["battery"];
        if (!isValidBatteryPercent(battery)) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid battery_percent (must be 0-100)");
            return ctx->send();
        }
        telem.battery_percent = battery;
        
        // 验证并设置 signal_strength
        int signal = json["signal"].is_null() ? 100 : (int)json["signal"];
        if (!isValidSignalStrength(signal)) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid signal_strength (must be 0-100)");
            return ctx->send();
        }
        telem.signal_strength = signal;
        
        // 验证整个 telemetry 对象
        if (!telem.isValid()) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid telemetry data");
            return ctx->send();
        }
        
        // 更新设备状态
        g_device_manager.updateTelemetry(device_id, telem);
        
        ctx->setContentType("application/json");
        ctx->set("code", 0);
        ctx->set("message", "OK");
        
        return ctx->send();
    } catch (const std::exception& e) {
        ctx->setContentType("application/json");
        ctx->set("code", -1);
        ctx->set("message", std::string("Error: ") + e.what());
        return ctx->send();
    }
}

int RobotHandler::listDevices(const HttpContextPtr& ctx) {
    try {
        auto devices = g_device_manager.listDevices();
        
        ctx->setContentType("application/json");
        ctx->set("code", 0);
        
        // 构建设备列表 JSON 数组
        hv::Json devices_json;
        for (const auto& device_id : devices) {
            devices_json.push_back(device_id);
        }
        ctx->set("devices", devices_json);
        ctx->set("count", (int)devices.size());
        
        return ctx->send();
    } catch (const std::exception& e) {
        ctx->setContentType("application/json");
        ctx->set("code", -1);
        ctx->set("message", std::string("Error: ") + e.what());
        return ctx->send();
    }
}

int RobotHandler::getDeviceState(const HttpContextPtr& ctx) {
    try {
        // 获取并验证 device_id
        std::string device_id = ctx->param("device_id");
        if (!isValidDeviceId(device_id)) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid device_id format");
            return ctx->send();
        }
        
        Telemetry telem;
        bool ok = g_device_manager.getDeviceState(device_id, telem);
        
        ctx->setContentType("application/json");
        
        if (ok) {
            ctx->set("code", 0);
            ctx->set("data/device_id", device_id);
            ctx->set("data/timestamp", telem.timestamp);
            ctx->set("data/x", telem.x);
            ctx->set("data/y", telem.y);
            ctx->set("data/yaw", telem.yaw);
            ctx->set("data/vx", telem.vx);
            ctx->set("data/vy", telem.vy);
            ctx->set("data/w", telem.w);
            ctx->set("data/battery", telem.battery_percent);
            ctx->set("data/signal", telem.signal_strength);
        } else {
            ctx->set("code", -1);
            ctx->set("message", "Device not found");
        }
        
        return ctx->send();
    } catch (const std::exception& e) {
        ctx->setContentType("application/json");
        ctx->set("code", -1);
        ctx->set("message", std::string("Error: ") + e.what());
        return ctx->send();
    }
}
