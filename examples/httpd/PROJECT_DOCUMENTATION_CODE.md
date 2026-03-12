# libhv 机器人后端项目 - 代码详解与技术栈

## 📖 核心代码模块详解

### 模块 1: protocol.h - 协议定义

**职责**: 定义通信协议、数据结构、验证函数

**关键内容**:

1. **常量定义** (9个)
   - MAX_DEVICE_ID_LEN = 64
   - MAX_FIRMWARE_VERSION_LEN = 32
   - MAX_REQUEST_SIZE = 1MB
   - 范围限制: 地图分辨率(1-100), 电池(0-100), 信号(0-100)

2. **验证函数** (5个)
   - `isValidDeviceId()` - 正则: `^[a-zA-Z0-9_-]+$`
   - `isValidFirmwareVersion()` - 正则: `^[0-9]+(\.[0-9]+)*$`
   - `isValidMapResolution()` - 范围检查
   - `isValidBatteryPercent()` - 范围检查
   - `isValidSignalStrength()` - 范围检查

3. **数据结构** (3个)
   - `DeviceRegister` - 设备注册信息
   - `Telemetry` - 实时状态数据
   - `MapUpdate` - 地图更新块

**技术栈**: C++11 (std::string, std::regex, std::vector)

---

### 模块 2: device_manager.h/cpp - 设备管理器

**职责**: 管理所有连接的机器人设备，提供线程安全操作

**核心数据结构**:
```cpp
struct DeviceInfo {
    DeviceRegister reg;                    // 注册信息
    Telemetry latest_telem;                // 最新状态
    std::chrono::system_clock::time_point last_update;  // 更新时间
    bool online;                           // 在线状态
};

class DeviceManager {
private:
    std::map<std::string, DeviceInfo> devices_;  // 设备存储
    std::mutex devices_mutex_;                   // 线程安全锁
};
```

**核心函数** (6个):

1. `registerDevice()` - 注册新设备
   - 验证所有字段
   - 检查重复注册
   - 加锁存储

2. `updateTelemetry()` - 更新设备状态
   - 验证数据有效性
   - 更新时间戳
   - 标记在线

3. `listDevices()` - 获取设备列表
   - 遍历 map
   - 返回 device_id 列表

4. `getDeviceState()` - 查询单个设备
   - 查找设备
   - 复制状态数据

5. `deviceOffline()` - 标记离线
   - 设置 online = false

6. `cleanupOfflineDevices()` - 清理离线设备
   - 检查超时
   - 删除过期数据

**技术栈**: C++11 (std::map, std::mutex, std::lock_guard, std::chrono)

---

### 模块 3: robot_handler.cpp - HTTP 请求处理

**职责**: 处理 HTTP 请求，进行输入验证，调用业务逻辑

**核心函数** (5个):

1. `registerDevice()` - 处理注册请求
   - 检查请求体大小 (< 1MB)
   - 解析 JSON
   - 验证必需字段
   - 验证格式和范围
   - 调用 device_manager
   - 返回响应

2. `updateTelemetry()` - 处理状态上报
   - 检查请求体大小
   - 提取 device_id
   - 解析 JSON
   - 验证所有字段
   - 调用 device_manager
   - 返回响应

3. `listDevices()` - 处理列表查询
   - 调用 device_manager
   - 构建 JSON 数组
   - 返回响应

4. `getDeviceState()` - 处理单个设备查询
   - 提取 device_id
   - 验证格式
   - 调用 device_manager
   - 返回响应

5. `health()` - 健康检查
   - 返回状态和时间戳

**技术栈**: libhv HttpService, JSON 解析, 异常处理

---

### 模块 4: robot_router.cpp - 路由配置

**职责**: 配置 HTTP 路由映射

**路由表** (5个):
```
GET  /health                          → health()
POST /api/devices/register            → registerDevice()
POST /api/devices/:device_id/telemetry → updateTelemetry()
GET  /api/devices                     → listDevices()
GET  /api/devices/:device_id          → getDeviceState()
```

**技术栈**: libhv 路由系统

---

### 模块 5: httpd.cpp - 主程序

**职责**: 服务器初始化、配置加载、路由注册

**主要步骤** (11个):
1. 初始化主上下文
2. 解析命令行参数
3. 处理 -h, -v 等选项
4. 加载配置文件 (etc/httpd.conf)
5. 测试配置文件
6. 信号处理
7. 后台运行 (可选)
8. 创建 PID 文件
9. 注册路由 (原始 + 机器人)
10. 注册 HTTP 服务
11. 启动服务器

**技术栈**: libhv 核心库, INI 配置解析, 信号处理

---

## 🔄 完整执行流程详解

### 流程 1: 机器人注册 (详细步骤)

```
1. 机器人启动
   └─> 准备注册信息
       - device_id: "robot_001"
       - firmware_version: "1.0"
       - map_resolution: 5
       - has_occupancy: 1
       - has_semantic: 0

2. 发送 HTTP POST 请求
   POST /api/devices/register
   Content-Type: application/json
   Body: {...}

3. libhv 路由层
   └─> robot_router.cpp 匹配路由
       └─> 调用 RobotHandler::registerDevice()

4. HTTP 处理层 (robot_handler.cpp)
   ├─> 检查 ctx->body.size() < 1MB
   ├─> 解析 JSON: auto json = ctx->json()
   ├─> 验证字段存在: json.contains("device_id")
   ├─> 提取 device_id: json["device_id"].get<std::string>()
   ├─> 验证格式: isValidDeviceId(device_id_str)
   ├─> 提取 firmware_version
   ├─> 验证格式: isValidFirmwareVersion(fw_version_str)
   ├─> 构建 DeviceRegister 结构体
   ├─> 验证 map_resolution: isValidMapResolution()
   ├─> 调用 g_device_manager.registerDevice()
   └─> 生成 JSON 响应

5. 业务逻辑层 (device_manager.cpp)
   ├─> 验证 device_id 格式
   ├─> 验证 firmware_version 格式
   ├─> 验证 map_resolution 范围
   ├─> 加锁: std::lock_guard<std::mutex> lock(devices_mutex_)
   ├─> 检查重复: devices_.find(device_id) != devices_.end()
   ├─> 创建 DeviceInfo 对象
   ├─> 设置 online = true
   ├─> 设置 last_update = now()
   ├─> 存储: devices_[device_id] = dev_info
   ├─> 解锁 (自动)
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

### 流程 2: 机器人上报状态 (高频 10Hz)

```
1. 机器人实时循环
   └─> 每 100ms 执行一次
       - 获取当前位姿 (x, y, yaw)
       - 获取速度 (vx, vy, w)
       - 获取电池百分比
       - 获取信号强度

2. 发送 HTTP POST 请求
   POST /api/devices/robot_001/telemetry
   Content-Type: application/json
   Body: {
     "timestamp": 1710259200.5,
     "x": 1.5,
     "y": 2.3,
     "yaw": 0.5,
     "vx": 0.1,
     "vy": 0.05,
     "w": 0.02,
     "battery": 85,
     "signal": 95
   }

3. libhv 路由层
   └─> robot_router.cpp 匹配路由
       └─> 提取 :device_id 参数
       └─> 调用 RobotHandler::updateTelemetry()

4. HTTP 处理层 (robot_handler.cpp)
   ├─> 检查 ctx->body.size() < 1MB
   ├─> 提取 device_id: ctx->param("device_id")
   ├─> 验证格式: isValidDeviceId(device_id)
   ├─> 解析 JSON
   ├─> 提取所有字段 (timestamp, x, y, yaw, vx, vy, w)
   ├─> 提取 battery: json["battery"]
   ├─> 验证范围: isValidBatteryPercent(battery)
   ├─> 提取 signal: json["signal"]
   ├─> 验证范围: isValidSignalStrength(signal)
   ├─> 构建 Telemetry 结构体
   ├─> 验证完整性: telem.isValid()
   ├─> 调用 g_device_manager.updateTelemetry()
   └─> 生成响应

5. 业务逻辑层 (device_manager.cpp)
   ├─> 验证 device_id 格式
   ├─> 验证 Telemetry 数据: telem.isValid()
   ├─> 加锁: std::lock_guard<std::mutex> lock(devices_mutex_)
   ├─> 查找设备: devices_.find(device_id)
   ├─> 如果不存在:
   │   └─> 创建新 DeviceInfo
   ├─> 如果存在:
   │   ├─> 更新 latest_telem = telem
   │   ├─> 更新 last_update = now()
   │   └─> 设置 online = true
   ├─> 解锁 (自动)
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

## 🛡️ 安全机制详解

### 输入验证层

**验证函数** (protocol.h):

1. **isValidDeviceId()**
   ```cpp
   // 检查长度: 1-63 字符
   if (device_id.empty() || device_id.length() >= MAX_DEVICE_ID_LEN)
       return false;
   
   // 正则表达式验证
   static const std::regex valid_pattern("^[a-zA-Z0-9_-]+$");
   return std::regex_match(device_id, valid_pattern);
   ```

2. **isValidFirmwareVersion()**
   ```cpp
   // 检查长度: 1-31 字符
   if (version.empty() || version.length() >= MAX_FIRMWARE_VERSION_LEN)
       return false;
   
   // 正则表达式验证 (版本号格式)
   static const std::regex valid_pattern("^[0-9]+(\\.[0-9]+)*$");
   return std::regex_match(version, valid_pattern);
   ```

3. **isValidMapResolution()**
   ```cpp
   return resolution >= MIN_MAP_RESOLUTION && 
          resolution <= MAX_MAP_RESOLUTION;
   ```

4. **isValidBatteryPercent()**
   ```cpp
   return percent >= MIN_BATTERY_PERCENT && 
          percent <= MAX_BATTERY_PERCENT;
   ```

5. **isValidSignalStrength()**
   ```cpp
   return strength >= MIN_SIGNAL_STRENGTH && 
          strength <= MAX_SIGNAL_STRENGTH;
   ```

### 防护措施

| 防护 | 实现 | 位置 |
|------|------|------|
| 缓冲区溢出 | snprintf + 字符串验证 | robot_handler.cpp |
| 输入注入 | 正则表达式验证 | protocol.h |
| 数据溢出 | 范围检查 | protocol.h |
| DoS 攻击 | 请求大小限制 (1MB) | robot_handler.cpp |
| 线程安全 | std::mutex 保护 | device_manager.cpp |
| 异常处理 | try-catch 块 | robot_handler.cpp |

---

## 📊 技术栈总结

### 编程语言
- **C++11** - 核心语言
- **JSON** - 数据交换格式

### 核心库
- **libhv** - HTTP 服务框架
  - HttpService - HTTP 服务
  - HttpContext - HTTP 上下文
  - 路由系统 - URL 路由

### C++ 标准库
- **std::map** - 设备存储
- **std::mutex** - 线程同步
- **std::lock_guard** - RAII 锁管理
- **std::chrono** - 时间管理
- **std::string** - 字符串处理
- **std::regex** - 正则表达式验证
- **std::vector** - 动态数组

### 系统库
- **pthread** - 多线程支持
- **signal.h** - 信号处理
- **unistd.h** - POSIX API

### 编译工具
- **g++** - C++ 编译器
- **Makefile** - 构建系统
- **C++11 标准** - 编译标志

### 配置管理
- **INI 格式** - 配置文件格式
- **IniParser** - 配置解析

### 日志系统
- **hlog** - libhv 日志库
  - hlogi() - 信息日志
  - hlogw() - 警告日志
  - hloge() - 错误日志
  - hlogd() - 调试日志

---

## 📈 代码统计

| 指标 | 数值 |
|------|------|
| 总代码行数 | ~1500 行 |
| 核心模块 | 5 个 |
| 验证函数 | 5 个 |
| HTTP 处理函数 | 5 个 |
| 数据结构 | 3 个 |
| 常量定义 | 9 个 |
| 路由端点 | 5 个 |
| 线程安全点 | 6 个 |

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
port = 8080                    # HTTP 端口
worker_threads = 4            # 工作线程数
worker_connections = 1000     # 最大连接数

[log]
logfile = logs/robot_backend.log  # 日志文件
loglevel = INFO                   # 日志级别
```

---

## 📝 关键设计决策

### 1. 内存存储 vs 数据库
- **选择**: 内存存储 (std::map)
- **原因**: 
  - 实时性要求高
  - 数据量相对较小
  - 简化部署

### 2. 线程安全
- **选择**: std::mutex + std::lock_guard
- **原因**:
  - RAII 自动释放锁
  - 异常安全
  - 性能足够

### 3. 输入验证
- **选择**: 多层验证 (格式 + 范围)
- **原因**:
  - 防止注入攻击
  - 防止缓冲区溢出
  - 防止数据污染

### 4. 错误处理
- **选择**: 统一的 JSON 错误响应
- **原因**:
  - 客户端易于处理
  - 错误信息清晰
  - 便于调试

---

## 🔗 模块间交互

```
HTTP 请求
    ↓
libhv 路由层 (robot_router.cpp)
    ↓
HTTP 处理层 (robot_handler.cpp)
    ├─> 请求解析
    ├─> 输入验证 (protocol.h)
    └─> 业务逻辑调用
    ↓
业务逻辑层 (device_manager.cpp)
    ├─> 数据验证
    ├─> 线程安全 (std::mutex)
    └─> 数据存储 (std::map)
    ↓
HTTP 响应
```

---

## 📚 相关文档

- `PROJECT_DOCUMENTATION.md` - 项目架构和执行流程
- `PROJECT_DOCUMENTATION_API.md` - REST API 详细说明
- `SECURITY_AUDIT.md` - 安全审查报告
- `SECURITY_FIXES_SUMMARY.md` - 安全修复总结

