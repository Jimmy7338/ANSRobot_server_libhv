#include "robot_handler.h"
#include "hlog.h"
#include "evpp/EventLoopThreadPool.h"
#include "device_monitor.h"
#include <ctime>

// 全局对象
extern DeviceManager g_device_manager;
extern hv::EventLoopThreadPool* g_loop_thread_pool;
extern DeviceMonitor* g_device_monitor;

int RobotHandler::health(const HttpContextPtr& ctx) {
    ctx->setContentType("application/json");
    ctx->set("status", "ok");
    ctx->set("timestamp", (long long)std::time(nullptr));
    return ctx->send();
}

int RobotHandler::registerDevice(const HttpContextPtr& ctx) {
    try {
        // 步骤 1: 快速验证 (< 1ms)
        if (ctx->body.size() > MAX_REQUEST_SIZE) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Request body too large");
            return ctx->send();
        }
        
        auto json = ctx->json();
        
        if (!json.contains("device_id") || !json.contains("firmware_version")) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Missing required fields: device_id, firmware_version");
            return ctx->send();
        }
        
        std::string device_id_str = json["device_id"].is_string() ? 
                                    json["device_id"].get<std::string>() : 
                                    json["device_id"].dump();
        
        if (!device_id_str.empty() && device_id_str.front() == '"' && device_id_str.back() == '"') {
            device_id_str = device_id_str.substr(1, device_id_str.length() - 2);
        }
        
        if (!isValidDeviceId(device_id_str)) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid device_id format (must be alphanumeric, underscore, or hyphen)");
            return ctx->send();
        }
        
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
        
        // 步骤 2: 提交到 EventLoop 处理 (异步)
        if (g_loop_thread_pool) {
            auto loop = g_loop_thread_pool->nextLoop();
            
            if (loop) {
                // 在 EventLoop 中执行任务
                loop->runInLoop([device_id_str, fw_version_str, json, ctx]() {
                    DeviceRegister reg;
                    snprintf(reg.device_id, sizeof(reg.device_id), "%s", device_id_str.c_str());
                    snprintf(reg.firmware_version, sizeof(reg.firmware_version), "%s", fw_version_str.c_str());
                    
                    reg.map_resolution = json["map_resolution"].is_null() ? 5 : (int)json["map_resolution"];
                    if (!isValidMapResolution(reg.map_resolution)) {
                        ctx->setContentType("application/json");
                        ctx->set("code", -1);
                        ctx->set("message", "Invalid map_resolution (must be 1-100)");
                        ctx->send();
                        return;
                    }
                    
                    reg.has_occupancy = json["has_occupancy"].is_null() ? 1 : (int)json["has_occupancy"];
                    reg.has_semantic = json["has_semantic"].is_null() ? 0 : (int)json["has_semantic"];
                    
                    bool ok = g_device_manager.registerDevice(device_id_str, reg);
                    
                    if (ok && g_device_monitor) {
                        g_device_monitor->register_device(device_id_str);
                    }
                    
                    ctx->setContentType("application/json");
                    ctx->set("code", ok ? 0 : -1);
                    ctx->set("message", ok ? "OK" : "Device already exists");
                    ctx->set("device_id", device_id_str);
                    ctx->send();
                });
            }
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        ctx->setContentType("application/json");
        ctx->set("code", -1);
        ctx->set("message", std::string("Error: ") + e.what());
        return ctx->send();
    }
}

int RobotHandler::updateTelemetry(const HttpContextPtr& ctx) {
    try {
        // 步骤 1: 快速验证 (< 1ms)
        if (ctx->body.size() > MAX_REQUEST_SIZE) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Request body too large");
            return ctx->send();
        }
        
        std::string device_id = ctx->param("device_id");
        if (!isValidDeviceId(device_id)) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid device_id format");
            return ctx->send();
        }
        
        auto json = ctx->json();
        
        Telemetry telem;
        telem.timestamp = json["timestamp"].is_null() ? 0.0 : (double)json["timestamp"];
        telem.x = json["x"].is_null() ? 0.0 : (double)json["x"];
        telem.y = json["y"].is_null() ? 0.0 : (double)json["y"];
        telem.yaw = json["yaw"].is_null() ? 0.0 : (double)json["yaw"];
        telem.vx = json["vx"].is_null() ? 0.0 : (double)json["vx"];
        telem.vy = json["vy"].is_null() ? 0.0 : (double)json["vy"];
        telem.w = json["w"].is_null() ? 0.0 : (double)json["w"];
        
        int battery = json["battery"].is_null() ? 100 : (int)json["battery"];
        if (!isValidBatteryPercent(battery)) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid battery_percent (must be 0-100)");
            return ctx->send();
        }
        telem.battery_percent = battery;
        
        int signal = json["signal"].is_null() ? 100 : (int)json["signal"];
        if (!isValidSignalStrength(signal)) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid signal_strength (must be 0-100)");
            return ctx->send();
        }
        telem.signal_strength = signal;
        
        if (!telem.isValid()) {
            ctx->setContentType("application/json");
            ctx->set("code", -1);
            ctx->set("message", "Invalid telemetry data");
            return ctx->send();
        }
        
        // 步骤 2: 提交到 EventLoop 处理 (异步)
        if (g_loop_thread_pool) {
            auto loop = g_loop_thread_pool->nextLoop();
            
            if (loop) {
                loop->runInLoop([device_id, telem]() {
                    g_device_manager.updateTelemetry(device_id, telem);
                    
                    if (g_device_monitor) {
                        g_device_monitor->update_device_activity(device_id);
                    }
                    
                    hlogd("Telemetry updated for device %s in EventLoop", 
                          device_id.c_str());
                });
            }
        }
        
        // 步骤 3: 立即返回响应
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
