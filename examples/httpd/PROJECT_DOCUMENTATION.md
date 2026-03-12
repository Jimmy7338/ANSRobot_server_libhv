# libhv 机器人后端项目 - 完整文档

## 📋 项目概述

本项目是基于开源项目 **libhv** 的 HTTP 服务框架，为 Neural-SLAM 机器人小车开发的后端服务系统。

**项目名称**: Robot Backend Service (基于 libhv)  
**开发语言**: C++11  
**框架**: libhv (高性能 HTTP 服务库)  
**功能**: 机器人设备管理、状态上报、实时监控  
**部署方式**: 独立 HTTP 服务器

---

## 🏗️ 项目架构

### 整体架构图

```
┌─────────────────────────────────────────────────────────────┐
│                    机器人小车 (客户端)                        │
│  - Neural-SLAM 导航系统                                      │
│  - 实时位姿估计                                              │
│  - 地图构建                                                  │
└────────────────────┬────────────────────────────────────────┘
                     │ HTTP/JSON
                     │ (REST API)
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              libhv HTTP 服务器 (后端)                        │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  HTTP 路由层 (robot_router.cpp)                      │   │
│  │  - /api/devices/register                             │   │
│  │  - /api/devices/:device_id/telemetry                 │   │
│  │  - /api/devices                                      │   │
│  │  - /api/devices/:device_id                           │   │
│  └──────────────────────────────────────────────────────┘   │
│                     ▼                                        │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  HTTP 处理层 (robot_handler.cpp)                     │   │
│  │  - 请求解析                                          │   │
│  │  - 输入验证                                          │   │
│  │  - 业务逻辑处理                                      │   │
│  │  - 响应生成                                          │   │
│  └──────────────────────────────────────────────────────┘   │
│                     ▼                                        │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  设备管理层 (device_manager.cpp)                     │   │
│  │  - 设备注册                                          │   │
│  │  - 状态更新                                          │   │
│  │  - 设备查询                                          │   │
│  │  - 离线管理                                          │   │
│  └──────────────────────────────────────────────────────┘   │
│                     ▼                                        │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  数据存储层 (内存存储)                               │   │
│  │  - std::map<device_id, DeviceInfo>                   │   │
│  │  - 线程安全 (std::mutex)                             │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 核心文件说明

### 1. protocol.h - 协议定义
**职责**: 定义通信协议、数据结构、验证函数

**关键内容**:
- `DeviceRegister` - 设备注册信息结构
- `Telemetry` - 实时状态数据结构
- `MapUpdate` - 地图更新数据结构
- 5个验证函数 (输入安全)
- 9个常量定义 (范围限制)

**技术栈**: C++ 结构体、正则表达式验证

---

### 2. device_manager.h/cpp - 设备管理器
**职责**: 管理所有连接的机器人设备

**核心功能**:
- `registerDevice()` - 注册新设备
- `updateTelemetry()` - 更新设备状态
- `listDevices()` - 获取设备列表
- `getDeviceState()` - 查询单个设备状态
- `deviceOffline()` - 标记设备离线
- `cleanupOfflineDevices()` - 清理离线设备

**数据结构**:
```cpp
struct DeviceInfo {
    DeviceRegister reg;              // 注册信息
    Telemetry latest_telem;          // 最新状态
    std::chrono::system_clock::time_point last_update;  // 更新时间
    bool online;                     // 在线状态
};
```

**技术栈**: C++11 (std::map, std::mutex, std::chrono)

---

### 3. robot_handler.h/cpp - HTTP 请求处理
**职责**: 处理 HTTP 请求，调用业务逻辑

**核心函数**:
- `registerDevice()` - 处理设备注册请求
- `updateTelemetry()` - 处理状态上报请求
- `listDevices()` - 处理设备列表查询
- `getDeviceState()` - 处理单个设备查询
- `health()` - 健康检查

**技术栈**: libhv HttpService, JSON 解析, 输入验证

---

### 4. robot_router.h/cpp - 路由配置
**职责**: 配置 HTTP 路由映射

**路由表**:
```
GET  /health                          → health()
POST /api/devices/register            → registerDevice()
POST /api/devices/:device_id/telemetry → updateTelemetry()
GET  /api/devices                     → listDevices()
GET  /api/devices/:device_id          → getDeviceState()
```

**技术栈**: libhv 路由系统

---

### 5. httpd.cpp - 主程序
**职责**: 服务器初始化、配置加载、路由注册

**主要步骤**:
1. 解析命令行参数
2. 加载配置文件 (etc/httpd.conf)
3. 配置日志系统
4. 配置多进程/多线程
5. 注册路由 (原始 + 机器人)
6. 启动 HTTP 服务器

**技术栈**: libhv 核心库, INI 配置解析

---

## 🔄 完整执行流程

### 流程 1: 机器人注册

```
1. 机器人启动
   └─> 准备注册信息 (device_id, firmware_version, map_resolution 等)

2. 发送 HTTP POST 请求
   POST /api/devices/register
   Content-Type: application/json
   {
     "device_id": "robot_001",
     "firmware_version": "1.0",
     "map_resolution": 5,
     "has_occupancy": 1,
     "has_semantic": 0
   }

3. 后端接收请求
   └─> robot_router.cpp 路由到 RobotHandler::registerDevice()

4. HTTP 处理层 (robot_handler.cpp)
   ├─> 检查请求体大小 (< 1MB)
   ├─> 解析 JSON 对象
   ├─> 验证必需字段存在
   ├─> 验证 device_id 格式 (正则: ^[a-zA-Z0-9_-]+$)
   ├─> 验证 firmware_version 格式 (正则: ^[0-9]+(\.[0-9]+)*$)
   ├─> 验证 map_resolution 范围 (1-100)
   └─> 构建 DeviceRegister 结构体

5. 业务逻辑层 (device_manager.cpp)
   ├─> 再次验证所有字段
   ├─> 检查设备是否已存在
   ├─> 创建 DeviceInfo 对象
   ├─> 加锁 (std::mutex)
   ├─> 存储到 std::map<device_id, DeviceInfo>
   ├─> 解锁
   └─> 记录日志

6. 生成响应
   {
     "code": 0,
     "message": "OK",
     "device_id": "robot_001"
   }

7. 返回给机器人
   └─> HTTP 200 OK
```

---

### 流程 2: 机器人上报状态 (高频)

```
1. 机器人实时循环 (例如 10Hz)
   └─> 获取当前位姿、速度、电池等信息

2. 发送 HTTP POST 请求
   POST /api/devices/robot_001/telemetry
   Content-Type: application/json
   {
     "timestamp": 1234567890.5,
     "x": 1.5,
     "y": 2.3,
     "yaw": 0.5,
     "vx": 0.1,
     "vy": 0.05,
     "w": 0.02,
     "battery": 85,
     "signal": 95
   }

3. 后端接收请求
   └─> robot_router.cpp 路由到 RobotHandler::updateTelemetry()

4. HTTP 处理层 (robot_handler.cpp)
   ├─> 检查请求体大小 (< 1MB)
   ├─> 从 URL 参数提取 device_id
   ├─> 验证 device_id 格式
   ├─> 解析 JSON 对象
   ├─> 提取所有字段 (timestamp, x, y, yaw, vx, vy, w, battery, signal)
   ├─> 验证 battery 范围 (0-100)
   ├─> 验证 signal 范围 (0-100)
   └─> 构建 Telemetry 结构体

5. 业务逻辑层 (device_manager.cpp)
   ├─> 验证 device_id 格式
   ├─> 验证 Telemetry 数据有效性
   ├─> 加锁 (std::mutex)
   ├─> 查找设备
   ├─> 更新 latest_telem
   ├─> 更新 last_update 时间戳
   ├─> 标记 online = true
   ├─> 解锁
   └─> 记录日志

6. 生成响应
   {
     "code": 0,
     "message": "OK"
   }

7. 返回给机器人
   └─> HTTP 200 OK
```

---

### 流程 3: 查询设备列表

```
1. 前端/监控系统发送请求
   GET /api/devices

2. 后端接收请求
   └─> robot_router.cpp 路由到 RobotHandler::listDevices()

3. HTTP 处理层 (robot_handler.cpp)
   ├─> 调用 device_manager.listDevices()
   └─> 构建 JSON 数组

4. 业务逻辑层 (device_manager.cpp)
   ├─> 加锁 (std::mutex)
   ├─> 遍历 std::map
   ├─> 收集所有 device_id
   ├─> 解锁
   └─> 返回列表

5. 生成响应
   {
     "code": 0,
     "devices": ["robot_001", "robot_002", "robot_003"],
     "count": 3
   }

6. 返回给前端
   └─> HTTP 200 OK
```

---

### 流程 4: 查询单个设备状态

```
1. 前端/监控系统发送请求
   GET /api/devices/robot_001

2. 后端接收请求
   └─> robot_router.cpp 路由到 RobotHandler::getDeviceState()

3. HTTP 处理层 (robot_handler.cpp)
   ├─> 从 URL 参数提取 device_id
   ├─> 验证 device_id 格式
   ├─> 调用 device_manager.getDeviceState()
   └─> 构建响应 JSON

4. 业务逻辑层 (device_manager.cpp)
   ├─> 验证 device_id 格式
   ├─> 加锁 (std::mutex)
   ├─> 查找设备
   ├─> 复制 Telemetry 数据
   ├─> 解锁
   └─> 返回数据

5. 生成响应
   {
     "code": 0,
     "data": {
       "device_id": "robot_001",
       "timestamp": 1234567890.5,
       "x": 1.5,
       "y": 2.3,
       "yaw": 0.5,
       "vx": 0.1,
       "vy": 0.05,
       "w": 0.02,
       "battery": 85,
       "signal": 95
     }
   }

6. 返回给前端
   └─> HTTP 200 OK
```

---

## 🛡️ 安全机制

### 输入验证层

**验证函数** (protocol.h):
1. `isValidDeviceId()` - 设备ID格式验证
   - 长度: 1-63 字符
   - 格式: 只允许字母、数字、下划线、连字符
   - 正则: `^[a-zA-Z0-9_-]+$`

2. `isValidFirmwareVersion()` - 固件版本验证
   - 长度: 1-31 字符
   - 格式: 版本号格式 (如 1.0, 1.2.3)
   - 正则: `^[0-9]+(\.[0-9]+)*$`

3. `isValidMapResolution()` - 地图分辨率验证
   - 范围: 1-100 cm/pixel

4. `isValidBatteryPercent()` - 电池百分比验证
   - 范围: 0-100%

5. `isValidSignalStrength()` - 信号强度验证
   - 范围: 0-100

### 防护措施

| 防护 | 实现 | 位置 |
|------|------|------|
| 缓冲区溢出 | snprintf + 字符串验证 | robot_handler.cpp |
| 输入注入 | 正则表达式验证 | protocol.h |
| 数据溢出 | 范围检查 | protocol.h |
| DoS 攻击 | 请求大小限制 (1MB) | robot_handler.cpp |
| 线程安全 | std::mutex 保护 | device_manager.cpp |

---

## 📊 技术栈总结

| 层级 | 技术 | 文件 |
|------|------|------|
| 协议层 | C++ 结构体、正则表达式 | protocol.h |
| 业务逻辑 | C++11 STL (map, mutex, chrono) | device_manager.cpp |
| HTTP 处理 | libhv HttpService、JSON | robot_handler.cpp |
| 路由 | libhv 路由系统 | robot_router.cpp |
| 服务器 | libhv HTTP 服务器 | httpd.cpp |
| 配置 | INI 文件格式 | etc/httpd.conf |
| 编译 | Makefile、g++ C++11 | Makefile.robot |

---

## 🚀 部署和运行

### 编译
```bash
cd /home/ubuntu/lzy/libhv/examples/httpd
make -f Makefile.robot clean
make -f Makefile.robot
```

### 运行
```bash
./httpd -c etc/httpd.conf
```

### 配置文件 (etc/httpd.conf)
```ini
[server]
port = 8080
worker_threads = 4
worker_connections = 1000

[log]
logfile = logs/robot_backend.log
loglevel = INFO
```

---

## 📝 API 文档

详见下一个文档文件: `PROJECT_DOCUMENTATION_API.md`

