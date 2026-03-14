# Active SLAM 系统 - 完整实施指南

## 🎯 项目概述

本项目将原有的 JSON 架构升级为 **Protobuf + WebSocket** 双端口架构，实现：

1. **高性能通信**：使用 Protobuf 二进制协议，比 JSON 快 3-10 倍
2. **双端口策略**：TCP (9090) 用于机器人，WebSocket (8080) 用于浏览器
3. **现代化前端**：赛博朋克风格的实时监控界面
4. **完整功能**：支持地图、轨迹、探索边界、YOLOv8 检测、相机流

---

## 📁 项目文件清单

### 后端 C++ 文件

```
httpd/
├── proto/
│   └── robot.proto                    # ✅ Protobuf 协议定义
│
├── tcp_server_protobuf.h              # ✅ TCP 服务器（端口 9090）
├── tcp_server_protobuf.cpp            # ✅ TCP 服务器实现
├── websocket_server.h                 # ✅ WebSocket 服务器（端口 8080）
├── websocket_server.cpp               # ✅ WebSocket 服务器实现
├── main_protobuf.cpp                  # ✅ 主程序入口
│
├── device_manager.h                   # ✅ 设备管理器（已存在）
├── device_manager.cpp                 # ✅ 设备管理器实现（已存在）
│
├── Makefile.protobuf                  # ✅ 编译配置
├── build_protobuf.sh                  # ✅ 自动编译脚本
├── start_protobuf.sh                  # ✅ 快速启动脚本
└── README_PROTOBUF.md                 # ✅ 完整文档
```

### 前端文件

```
web/
├── index.html                         # ✅ 主界面（赛博朋克风格）
└── app.js                             # ✅ WebSocket 客户端逻辑
```

### 测试文件

```
├── test_client.py                     # ✅ Python 测试客户端
```

---

## 🚀 快速开始（3 步）

### 步骤 1: 编译后端

```bash
cd /home/ubuntu/lzy/libhv/examples/httpd

# 给脚本添加执行权限
chmod +x build_protobuf.sh start_protobuf.sh test_client.py

# 编译（会自动安装依赖）
./build_protobuf.sh
```

### 步骤 2: 启动后端服务器

```bash
./start_protobuf.sh
```

你会看到：

```
==============================================
Robot Backend Server Started Successfully!
TCP Server (Robot):      0.0.0.0:9090
WebSocket Server (Web):  0.0.0.0:8080
==============================================
```

### 步骤 3: 打开前端界面

**方式 1: 直接打开文件**

```bash
# 在浏览器中打开
firefox web/index.html
# 或
google-chrome web/index.html
```

**方式 2: 使用 HTTP 服务器（推荐）**

```bash
cd web
python3 -m http.server 3000
```

然后访问：`http://localhost:3000`

---

## 🧪 测试系统

### 运行测试客户端

```bash
# 先编译 Python Protobuf
protoc --python_out=. proto/robot.proto

# 运行测试客户端（模拟机器人）
python3 test_client.py
```

测试客户端会：
- 连接到后端 (TCP 9090)
- 注册设备 `robot_001`
- 模拟圆周运动
- 发送实时状态（10Hz）
- 发送地图更新（1Hz）
- 发送探索边界（0.2Hz）

你会在前端看到：
- ✅ 实时轨迹绘制
- ✅ 位置、速度更新
- ✅ 探索边界标记
- ✅ 覆盖率进度条

---

## 🎨 前端界面特性

### 设计风格

- **配色方案**：深色背景 + 荧光绿 (#00ff9d)
- **字体**：JetBrains Mono（编程字体）
- **动画效果**：脉冲、渐变、光晕
- **布局**：3 列网格，响应式设计

### 功能模块

#### 1. 地图画布（中央）
- Canvas 实时渲染
- 网格背景
- 机器人轨迹（绿色曲线）
- 当前位置（绿色圆点）
- 探索边界（红色/橙色高亮）

#### 2. 设备列表（左侧）
- 多设备支持
- 在线状态
- 电池/信号显示
- 点击切换设备

#### 3. 实时状态（右上）
- 位置 (x, y)
- 航向角 (yaw)
- 速度 (v)
- 电池百分比
- 信号强度
- 覆盖率进度条

#### 4. 相机视图（中下）
- MJPEG 视频流
- YOLOv8 检测框叠加

#### 5. 探索边界（右下）
- 边界列表
- 信息增益显示

---

## 📡 通信协议详解

### Protobuf 消息格式

所有消息都包装在 `RobotMessage` 中：

```protobuf
message RobotMessage {
    MessageType type = 1;        // 消息类型
    string device_id = 2;        // 设备ID
    double timestamp = 3;        // 时间戳
    
    oneof payload {              // 具体消息内容
        DeviceRegister register_msg = 10;
        Telemetry telemetry = 12;
        MapUpdate map_update = 13;
        FrontierUpdate frontier_update = 14;
        YoloDetection yolo_detection = 15;
        CameraFrame camera_frame = 16;
        // ...
    }
}
```

### 消息类型

| 类型 | 频率 | 用途 |
|------|------|------|
| MSG_REGISTER | 一次 | 设备注册 |
| MSG_TELEMETRY | 10-50Hz | 实时状态 |
| MSG_MAP_UPDATE | 1-5Hz | 地图增量更新 |
| MSG_FRONTIER_UPDATE | 0.2-1Hz | 探索边界 |
| MSG_YOLO_DETECTION | 5-10Hz | 目标检测 |
| MSG_CAMERA_FRAME | 10-30Hz | 视频流 |

---

## 🔧 开发指南

### 添加新消息类型

**步骤 1**: 在 `proto/robot.proto` 中定义

```protobuf
message NewMessage {
    string field1 = 1;
    int32 field2 = 2;
}

// 添加到 RobotMessage
message RobotMessage {
    oneof payload {
        // ...
        NewMessage new_msg = 19;
    }
}

// 添加消息类型
enum MessageType {
    // ...
    MSG_NEW = 7;
}
```

**步骤 2**: 重新编译 Protobuf

```bash
make -f Makefile.protobuf proto
```

**步骤 3**: 在 `tcp_server_protobuf.cpp` 中处理

```cpp
void RobotTcpServer::onMessage(...) {
    switch (msg.type()) {
        // ...
        case robot::MSG_NEW:
            handleNewMessage(channel, msg);
            break;
    }
}

void RobotTcpServer::handleNewMessage(...) {
    // 处理逻辑
    ws_server_->broadcast(msg);  // 转发到前端
}
```

**步骤 4**: 在 `web/app.js` 中显示

```javascript
handleJsonMessage(msg) {
    switch (msg.type) {
        // ...
        case MessageType.MSG_NEW:
            this.handleNewMessage(msg);
            break;
    }
}
```

### 性能优化技巧

#### 1. 地图数据压缩

```cpp
#include <zlib.h>

// 压缩占用栅格
std::string compressMapData(const std::vector<uint8_t>& data) {
    uLongf compressed_size = compressBound(data.size());
    std::string compressed(compressed_size, 0);
    
    compress((Bytef*)compressed.data(), &compressed_size,
             (const Bytef*)data.data(), data.size());
    
    compressed.resize(compressed_size);
    return compressed;
}
```

#### 2. WebSocket 降频

```cpp
// 只转发部分帧
static int frame_counter = 0;
if (msg.type() == robot::MSG_CAMERA_FRAME) {
    if (++frame_counter % 3 == 0) {  // 降到 1/3
        ws_server_->broadcast(msg);
    }
}
```

#### 3. 前端节流

```javascript
// 使用 requestAnimationFrame
let needsRedraw = false;

function updateMap() {
    needsRedraw = true;
}

function render() {
    if (needsRedraw) {
        redrawMap();
        needsRedraw = false;
    }
    requestAnimationFrame(render);
}
```

---

## 📊 架构对比

### 改造前（JSON + HTTP）

```
机器人 → HTTP POST (JSON) → 后端 → 轮询 → 前端
```

**缺点**：
- ❌ JSON 解析慢
- ❌ HTTP 开销大
- ❌ 轮询延迟高
- ❌ 无法推送

### 改造后（Protobuf + WebSocket）

```
机器人 → TCP (Protobuf) → 后端 → WebSocket (Protobuf) → 前端
```

**优点**：
- ✅ Protobuf 快 3-10 倍
- ✅ TCP 长连接，低延迟
- ✅ WebSocket 实时推送
- ✅ 双端口隔离

---

## 🎯 下一步优化

### 短期（1-2 周）

- [ ] 集成 protobuf.js（前端直接解析二进制）
- [ ] 实现地图压缩（zlib）
- [ ] 添加设备认证
- [ ] 优化 Canvas 渲染性能

### 中期（1 个月）

- [ ] 支持多用户（权限管理）
- [ ] 添加录制/回放功能
- [ ] 实现路径规划可视化
- [ ] 添加性能监控面板

### 长期（3 个月）

- [ ] 迁移到 WebRTC（视频流）
- [ ] 支持 3D 地图（Three.js）
- [ ] 云端部署（Docker + K8s）
- [ ] 移动端适配

---

## 🐛 故障排查

### 问题 1: 编译失败

```bash
# 检查依赖
protoc --version
g++ --version

# 重新安装
sudo apt-get install -y protobuf-compiler libprotobuf-dev build-essential
```

### 问题 2: WebSocket 连接失败

```bash
# 检查端口
netstat -tuln | grep 8080

# 检查防火墙
sudo ufw status
sudo ufw allow 8080
```

### 问题 3: 前端无法显示

打开浏览器控制台（F12），查看错误信息。常见原因：
- WebSocket URL 错误（检查 `app.js` 中的 `wsUrl`）
- CORS 问题（使用 HTTP 服务器而非 file://）

### 问题 4: 测试客户端无法连接

```bash
# 检查后端是否运行
ps aux | grep robot_backend

# 检查端口
telnet localhost 9090
```

---

## 📚 参考资料

- [libhv GitHub](https://github.com/ithewei/libhv)
- [Protocol Buffers 文档](https://developers.google.com/protocol-buffers)
- [WebSocket API](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket)
- [Canvas API](https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API)

---

## 🎉 总结

你现在拥有一个完整的 Active SLAM 后端系统：

✅ **高性能通信**：Protobuf 二进制协议  
✅ **双端口架构**：TCP (机器人) + WebSocket (浏览器)  
✅ **现代化前端**：赛博朋克风格实时监控  
✅ **完整功能**：地图、轨迹、探索边界、YOLOv8、相机流  
✅ **易于扩展**：模块化设计，清晰的接口  

**立即开始**：

```bash
cd /home/ubuntu/lzy/libhv/examples/httpd
./build_protobuf.sh
./start_protobuf.sh
```

然后打开 `web/index.html`，享受实时监控的乐趣！🚀
