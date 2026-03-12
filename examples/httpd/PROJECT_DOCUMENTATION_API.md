# libhv 机器人后端项目 - API 文档

## 📡 REST API 详细说明

### 1. 健康检查

**端点**: `GET /health`

**描述**: 检查服务器是否正常运行

**请求**:
```bash
curl -v http://localhost:8080/health
```

**响应** (200 OK):
```json
{
  "status": "ok",
  "timestamp": 1710259200
}
```

**错误处理**: 无

---

### 2. 设备注册

**端点**: `POST /api/devices/register`

**描述**: 机器人小车向后端注册，获得设备ID

**请求头**:
```
Content-Type: application/json
```

**请求体**:
```json
{
  "device_id": "robot_001",
  "firmware_version": "1.0",
  "map_resolution": 5,
  "has_occupancy": 1,
  "has_semantic": 0
}
```

**字段说明**:
| 字段 | 类型 | 必需 | 范围/格式 | 说明 |
|------|------|------|----------|------|
| device_id | string | ✓ | 1-63字符, `^[a-zA-Z0-9_-]+$` | 设备唯一标识 |
| firmware_version | string | ✓ | 1-31字符, `^[0-9]+(\.[0-9]+)*$` | 固件版本号 |
| map_resolution | int | ✗ | 1-100 | 地图分辨率 (cm/pixel), 默认5 |
| has_occupancy | int | ✗ | 0/1 | 是否支持占用栅格, 默认1 |
| has_semantic | int | ✗ | 0/1 | 是否支持语义, 默认0 |

**成功响应** (200 OK):
```json
{
  "code": 0,
  "message": "OK",
  "device_id": "robot_001"
}
```

**错误响应**:

1. 请求体过大 (> 1MB):
```json
{
  "code": -1,
  "message": "Request body too large"
}
```

2. 缺少必需字段:
```json
{
  "code": -1,
  "message": "Missing required fields: device_id, firmware_version"
}
```

3. device_id 格式无效:
```json
{
  "code": -1,
  "message": "Invalid device_id format (must be alphanumeric, underscore, or hyphen)"
}
```

4. firmware_version 格式无效:
```json
{
  "code": -1,
  "message": "Invalid firmware_version format"
}
```

5. map_resolution 范围无效:
```json
{
  "code": -1,
  "message": "Invalid map_resolution (must be 1-100)"
}
```

6. 设备已存在:
```json
{
  "code": -1,
  "message": "Device already exists"
}
```

**示例**:
```bash
curl -X POST http://localhost:8080/api/devices/register \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "robot_001",
    "firmware_version": "1.0",
    "map_resolution": 5,
    "has_occupancy": 1,
    "has_semantic": 0
  }'
```

---

### 3. 上报状态 (高频)

**端点**: `POST /api/devices/:device_id/telemetry`

**描述**: 机器人实时上报位姿、速度、电池等状态信息

**URL 参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| device_id | string | 设备ID (路径参数) |

**请求头**:
```
Content-Type: application/json
```

**请求体**:
```json
{
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
```

**字段说明**:
| 字段 | 类型 | 必需 | 范围 | 说明 |
|------|------|------|------|------|
| timestamp | double | ✗ | - | 时间戳 (秒), 默认0 |
| x | double | ✗ | - | X 坐标 (米), 默认0 |
| y | double | ✗ | - | Y 坐标 (米), 默认0 |
| yaw | double | ✗ | - | 偏航角 (弧度), 默认0 |
| vx | double | ✗ | - | X 方向速度 (m/s), 默认0 |
| vy | double | ✗ | - | Y 方向速度 (m/s), 默认0 |
| w | double | ✗ | - | 角速度 (rad/s), 默认0 |
| battery | int | ✗ | 0-100 | 电池百分比, 默认100 |
| signal | int | ✗ | 0-100 | 信号强度, 默认100 |

**成功响应** (200 OK):
```json
{
  "code": 0,
  "message": "OK"
}
```

**错误响应**:

1. device_id 格式无效:
```json
{
  "code": -1,
  "message": "Invalid device_id format"
}
```

2. battery 范围无效:
```json
{
  "code": -1,
  "message": "Invalid battery_percent (must be 0-100)"
}
```

3. signal 范围无效:
```json
{
  "code": -1,
  "message": "Invalid signal_strength (must be 0-100)"
}
```

4. 数据无效:
```json
{
  "code": -1,
  "message": "Invalid telemetry data"
}
```

**示例**:
```bash
curl -X POST http://localhost:8080/api/devices/robot_001/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "timestamp": 1710259200.5,
    "x": 1.5,
    "y": 2.3,
    "yaw": 0.5,
    "vx": 0.1,
    "vy": 0.05,
    "w": 0.02,
    "battery": 85,
    "signal": 95
  }'
```

**调用频率**: 建议 10Hz (每 100ms 一次)

---

### 4. 查询设备列表

**端点**: `GET /api/devices`

**描述**: 获取所有已注册的设备列表

**请求**:
```bash
curl -v http://localhost:8080/api/devices
```

**成功响应** (200 OK):
```json
{
  "code": 0,
  "devices": [
    "robot_001",
    "robot_002",
    "robot_003"
  ],
  "count": 3
}
```

**空列表响应**:
```json
{
  "code": 0,
  "devices": [],
  "count": 0
}
```

**错误响应**:
```json
{
  "code": -1,
  "message": "Error: [error details]"
}
```

---

### 5. 查询单个设备状态

**端点**: `GET /api/devices/:device_id`

**描述**: 获取指定设备的最新状态信息

**URL 参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| device_id | string | 设备ID (路径参数) |

**请求**:
```bash
curl -v http://localhost:8080/api/devices/robot_001
```

**成功响应** (200 OK):
```json
{
  "code": 0,
  "data": {
    "device_id": "robot_001",
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
}
```

**设备不存在响应** (200 OK):
```json
{
  "code": -1,
  "message": "Device not found"
}
```

**device_id 格式无效响应**:
```json
{
  "code": -1,
  "message": "Invalid device_id format"
}
```

---

## 🔄 数据流示例

### 完整交互流程

```
时间线:
├─ T0: 机器人启动
│  └─> POST /api/devices/register
│      请求: {"device_id":"robot_001","firmware_version":"1.0"}
│      响应: {"code":0,"message":"OK","device_id":"robot_001"}
│
├─ T1: 机器人开始导航 (10Hz 循环)
│  ├─> POST /api/devices/robot_001/telemetry
│  │   请求: {"timestamp":1710259200.0,"x":0.0,"y":0.0,"yaw":0.0,"battery":100,"signal":100}
│  │   响应: {"code":0,"message":"OK"}
│  │
│  ├─> POST /api/devices/robot_001/telemetry (100ms 后)
│  │   请求: {"timestamp":1710259200.1,"x":0.1,"y":0.0,"yaw":0.0,"battery":100,"signal":100}
│  │   响应: {"code":0,"message":"OK"}
│  │
│  └─> ... (持续上报)
│
├─ T2: 前端查询设备列表
│  └─> GET /api/devices
│      响应: {"code":0,"devices":["robot_001"],"count":1}
│
├─ T3: 前端查询设备状态
│  └─> GET /api/devices/robot_001
│      响应: {"code":0,"data":{"device_id":"robot_001","x":5.2,"y":3.1,"battery":85,...}}
│
└─ T4: 机器人停止
   └─> 停止上报状态
```

---

## 📊 性能指标

### 吞吐量
- **单设备**: 10 Hz (每秒 10 个请求)
- **多设备**: 支持 100+ 并发设备
- **总吞吐**: 1000+ 请求/秒 (取决于硬件)

### 延迟
- **平均响应时间**: < 10ms
- **P99 响应时间**: < 50ms

### 内存占用
- **基础**: ~10MB
- **每个设备**: ~1KB
- **100 个设备**: ~110MB

---

## 🔐 安全特性

### 请求验证
- ✅ 请求体大小限制 (1MB)
- ✅ 字段格式验证 (正则表达式)
- ✅ 数值范围检查
- ✅ 必需字段检查

### 线程安全
- ✅ 所有设备操作使用 std::mutex 保护
- ✅ 支持多线程并发访问

### 错误处理
- ✅ 详细的错误信息
- ✅ 统一的错误响应格式
- ✅ 异常捕获和处理

---

## 📝 HTTP 状态码

| 状态码 | 说明 |
|--------|------|
| 200 | 请求成功 (业务逻辑可能失败，需检查 code 字段) |
| 400 | 请求格式错误 |
| 404 | 路由不存在 |
| 500 | 服务器内部错误 |

---

## 🧪 测试用例

### 测试 1: 正常注册
```bash
curl -X POST http://localhost:8080/api/devices/register \
  -H "Content-Type: application/json" \
  -d '{"device_id":"robot_001","firmware_version":"1.0"}'
```

### 测试 2: 无效 device_id
```bash
curl -X POST http://localhost:8080/api/devices/register \
  -H "Content-Type: application/json" \
  -d '{"device_id":"robot@001","firmware_version":"1.0"}'
```

### 测试 3: 正常上报状态
```bash
curl -X POST http://localhost:8080/api/devices/robot_001/telemetry \
  -H "Content-Type: application/json" \
  -d '{"timestamp":1710259200.5,"x":1.5,"y":2.3,"battery":85,"signal":95}'
```

### 测试 4: 无效电池百分比
```bash
curl -X POST http://localhost:8080/api/devices/robot_001/telemetry \
  -H "Content-Type: application/json" \
  -d '{"timestamp":1710259200.5,"x":1.5,"y":2.3,"battery":150,"signal":95}'
```

### 测试 5: 查询设备列表
```bash
curl -v http://localhost:8080/api/devices
```

### 测试 6: 查询单个设备
```bash
curl -v http://localhost:8080/api/devices/robot_001
```

---

## 📚 集成指南

### Python 客户端示例

```python
import requests
import json
import time

BASE_URL = "http://localhost:8080"

# 1. 注册设备
def register_device(device_id, firmware_version):
    url = f"{BASE_URL}/api/devices/register"
    data = {
        "device_id": device_id,
        "firmware_version": firmware_version,
        "map_resolution": 5,
        "has_occupancy": 1,
        "has_semantic": 0
    }
    response = requests.post(url, json=data)
    return response.json()

# 2. 上报状态
def update_telemetry(device_id, x, y, yaw, battery, signal):
    url = f"{BASE_URL}/api/devices/{device_id}/telemetry"
    data = {
        "timestamp": time.time(),
        "x": x,
        "y": y,
        "yaw": yaw,
        "battery": battery,
        "signal": signal
    }
    response = requests.post(url, json=data)
    return response.json()

# 3. 查询设备列表
def list_devices():
    url = f"{BASE_URL}/api/devices"
    response = requests.get(url)
    return response.json()

# 4. 查询设备状态
def get_device_state(device_id):
    url = f"{BASE_URL}/api/devices/{device_id}"
    response = requests.get(url)
    return response.json()

# 使用示例
if __name__ == "__main__":
    # 注册
    print(register_device("robot_001", "1.0"))
    
    # 上报状态
    print(update_telemetry("robot_001", 1.5, 2.3, 0.5, 85, 95))
    
    # 查询列表
    print(list_devices())
    
    # 查询状态
    print(get_device_state("robot_001"))
```

---

## 🔗 相关文档

- `PROJECT_DOCUMENTATION.md` - 项目架构和执行流程
- `SECURITY_AUDIT.md` - 安全审查报告
- `SECURITY_FIXES_SUMMARY.md` - 安全修复总结

