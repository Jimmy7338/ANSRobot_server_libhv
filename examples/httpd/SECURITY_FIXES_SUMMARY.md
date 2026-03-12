# 安全修复快速参考

## 修复清单

### ✅ 已完成的修复

#### 1. protocol.h - 添加验证常量和函数
- ✅ 添加了字段长度限制常量
- ✅ 添加了数值范围限制常量
- ✅ 实现了 `isValidDeviceId()` 函数 - 使用正则表达式验证格式
- ✅ 实现了 `isValidFirmwareVersion()` 函数 - 验证版本号格式
- ✅ 实现了 `isValidMapResolution()` 函数 - 检查范围
- ✅ 实现了 `isValidBatteryPercent()` 函数 - 检查范围
- ✅ 实现了 `isValidSignalStrength()` 函数 - 检查范围
- ✅ 在 `Telemetry` 结构中添加了 `isValid()` 方法

#### 2. robot_handler.cpp - 修复所有处理函数
- ✅ `registerDevice()` - 添加了完整的输入验证
  - 检查请求体大小
  - 验证必需字段存在
  - 验证 device_id 格式
  - 验证 firmware_version 格式
  - 验证 map_resolution 范围
  - 使用 `snprintf` 替代 `strncpy`

- ✅ `updateTelemetry()` - 添加了完整的输入验证
  - 检查请求体大小
  - 验证 device_id 格式
  - 验证 battery_percent 范围
  - 验证 signal_strength 范围
  - 调用 `telem.isValid()` 进行完整验证

- ✅ `getDeviceState()` - 添加了 device_id 验证
  - 验证 device_id 格式

#### 3. device_manager.cpp - 添加验证层
- ✅ `registerDevice()` - 添加了参数验证
  - 验证 device_id 格式
  - 验证 firmware_version 格式
  - 验证 map_resolution 范围

- ✅ `updateTelemetry()` - 添加了参数验证
  - 验证 device_id 格式
  - 调用 `telem.isValid()` 进行完整验证

- ✅ `getDeviceState()` - 添加了 device_id 验证
- ✅ `getDeviceInfo()` - 添加了 device_id 验证
- ✅ `deviceOffline()` - 添加了 device_id 验证

---

## 漏洞修复对比

### 漏洞 #1: 缓冲区溢出

**修复前**:
```cpp
strncpy(reg.device_id, json["device_id"].dump().c_str(), sizeof(reg.device_id) - 1);
```

**修复后**:
```cpp
std::string device_id_str = json["device_id"].is_string() ? 
                            json["device_id"].get<std::string>() : 
                            json["device_id"].dump();

// 移除 JSON 引号
if (!device_id_str.empty() && device_id_str.front() == '"' && device_id_str.back() == '"') {
    device_id_str = device_id_str.substr(1, device_id_str.length() - 2);
}

// 验证格式
if (!isValidDeviceId(device_id_str)) {
    // 返回错误
}

// 安全复制
snprintf(reg.device_id, sizeof(reg.device_id), "%s", device_id_str.c_str());
```

---

### 漏洞 #2: 缺少输入验证

**修复前**:
```cpp
auto json = ctx->json();
DeviceRegister reg;
strncpy(reg.device_id, json["device_id"].dump().c_str(), sizeof(reg.device_id) - 1);
// 直接使用，无验证
```

**修复后**:
```cpp
// 检查请求体大小
if (ctx->body.size() > MAX_REQUEST_SIZE) {
    ctx->set("code", -1);
    ctx->set("message", "Request body too large");
    return ctx->send();
}

// 验证必需字段
if (!json.contains("device_id") || !json.contains("firmware_version")) {
    ctx->set("code", -1);
    ctx->set("message", "Missing required fields");
    return ctx->send();
}

// 验证字段值
if (!isValidDeviceId(device_id_str)) {
    ctx->set("code", -1);
    ctx->set("message", "Invalid device_id format");
    return ctx->send();
}

if (!isValidMapResolution(reg.map_resolution)) {
    ctx->set("code", -1);
    ctx->set("message", "Invalid map_resolution");
    return ctx->send();
}
```

---

### 漏洞 #3: 设备ID注入

**修复前**:
```cpp
std::string device_id = ctx->param("device_id");
g_device_manager.updateTelemetry(device_id, telem);  // 无验证
```

**修复后**:
```cpp
std::string device_id = ctx->param("device_id");
if (!isValidDeviceId(device_id)) {
    ctx->set("code", -1);
    ctx->set("message", "Invalid device_id format");
    return ctx->send();
}
g_device_manager.updateTelemetry(device_id, telem);
```

---

### 漏洞 #4: 数据类型溢出

**修复前**:
```cpp
telem.battery_percent = json["battery"].is_null() ? 100 : (int)json["battery"];
// 可能是 -1, 150, 等无效值
```

**修复后**:
```cpp
int battery = json["battery"].is_null() ? 100 : (int)json["battery"];
if (!isValidBatteryPercent(battery)) {
    ctx->set("code", -1);
    ctx->set("message", "Invalid battery_percent (must be 0-100)");
    return ctx->send();
}
telem.battery_percent = battery;

// 在 Telemetry 中添加验证
bool Telemetry::isValid() const {
    return isValidBatteryPercent(battery_percent) && 
           isValidSignalStrength(signal_strength);
}
```

---

### 漏洞 #5: 缺少请求大小限制

**修复前**:
```cpp
auto json = ctx->json();
// 可能处理超大请求体
```

**修复后**:
```cpp
if (ctx->body.size() > MAX_REQUEST_SIZE) {
    ctx->set("code", -1);
    ctx->set("message", "Request body too large");
    return ctx->send();
}
auto json = ctx->json();
```

---

## 验证函数说明

### isValidDeviceId()
- **用途**: 验证设备ID格式
- **规则**: 只允许字母、数字、下划线、连字符
- **长度**: 1-63 字符
- **示例**:
  - ✅ `robot_001`, `device-123`, `ROBOT_A1`
  - ❌ `robot@001`, `device#123`, ``, `robot 001`

### isValidFirmwareVersion()
- **用途**: 验证固件版本格式
- **规则**: 版本号格式 (如 1.0, 1.2.3)
- **长度**: 1-31 字符
- **示例**:
  - ✅ `1.0`, `2.1.3`, `10.20.30`
  - ❌ `v1.0`, `1.0.0.0.0`, `1.a`, ``

### isValidMapResolution()
- **用途**: 验证地图分辨率
- **范围**: 1-100
- **示例**:
  - ✅ `5`, `10`, `50`
  - ❌ `0`, `101`, `-1`

### isValidBatteryPercent()
- **用途**: 验证电池百分比
- **范围**: 0-100
- **示例**:
  - ✅ `0`, `50`, `100`
  - ❌ `-1`, `101`, `150`

### isValidSignalStrength()
- **用途**: 验证信号强度
- **范围**: 0-100
- **示例**:
  - ✅ `0`, `50`, `100`
  - ❌ `-1`, `101`, `150`

---

## 测试用例

### 有效请求
```bash
# 注册设备
curl -X POST http://localhost:8080/api/devices/register \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "robot_001",
    "firmware_version": "1.0",
    "map_resolution": 5,
    "has_occupancy": 1,
    "has_semantic": 0
  }'

# 更新遥测数据
curl -X POST http://localhost:8080/api/devices/robot_001/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "timestamp": 1234567890,
    "x": 1.5,
    "y": 2.3,
    "yaw": 0.5,
    "vx": 0.1,
    "vy": 0.2,
    "w": 0.05,
    "battery": 85,
    "signal": 95
  }'
```

### 无效请求 (应被拒绝)
```bash
# 缺少必需字段
curl -X POST http://localhost:8080/api/devices/register \
  -H "Content-Type: application/json" \
  -d '{"device_id": "robot_001"}'

# 无效的 device_id 格式
curl -X POST http://localhost:8080/api/devices/register \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "robot@001",
    "firmware_version": "1.0"
  }'

# 无效的 map_resolution
curl -X POST http://localhost:8080/api/devices/register \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "robot_001",
    "firmware_version": "1.0",
    "map_resolution": 150
  }'

# 无效的 battery_percent
curl -X POST http://localhost:8080/api/devices/robot_001/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "timestamp": 1234567890,
    "x": 1.5,
    "y": 2.3,
    "battery": 150
  }'

# 超大请求体
curl -X POST http://localhost:8080/api/devices/register \
  -H "Content-Type: application/json" \
  -d '{"device_id":"robot_001","firmware_version":"1.0","data":"'$(python3 -c "print(\"x\"*2000000)")'}'
```

---

## 编译和测试

### 编译
```bash
cd /home/ubuntu/lzy/libhv/examples/httpd
make clean
make
```

### 运行
```bash
./httpd -c etc/httpd.conf
```

### 测试
```bash
# 健康检查
curl http://localhost:8080/health

# 注册设备
curl -X POST http://localhost:8080/api/devices/register \
  -H "Content-Type: application/json" \
  -d '{"device_id":"robot_001","firmware_version":"1.0"}'

# 查询设备列表
curl http://localhost:8080/api/devices

# 查询设备状态
curl http://localhost:8080/api/devices/robot_001
```

---

## 后续建议

### 立即实施
1. ✅ 已完成的输入验证
2. ✅ 已完成的缓冲区溢出修复
3. ✅ 已完成的数据类型检查

### 短期 (1-2 周)
1. ⚠️ 实现认证机制 (JWT 或 API Key)
2. ⚠️ 添加请求日志审计
3. ⚠️ 实现速率限制

### 中期 (1-2 月)
1. ⚠️ 启用 HTTPS/TLS
2. ⚠️ 添加单元测试
3. ⚠️ 添加集成测试

### 长期 (持续)
1. ⚠️ 定期安全审查
2. ⚠️ 依赖库更新
3. ⚠️ 渗透测试

---

**修复完成日期**: 2026-03-12  
**修复状态**: ✅ 所有关键漏洞已修复
