# Active SLAM 后端系统 - Protobuf + WebSocket 版本

## 📋 项目概述

这是一个基于 libhv 的 Active SLAM 后端系统，采用 **双端口策略**：

- **端口 9090 (TCP + Protobuf)**: 与机器人小车进行高频、低延迟的二进制数据通信
- **端口 8080 (WebSocket + Protobuf)**: 向浏览器实时推送数据，用于可视化监控

## 🏗️ 架构设计

```
┌─────────────────┐         TCP (Protobuf)          ┌─────────────────┐
│   机器人小车     │ ◄──────────────────────────────► │   C++ 后端       │
│   (Python/C++)  │         端口 9090               │   (libhv)       │
└─────────────────┘                                 └─────────────────┘
                                                            │
                                                            │ WebSocket
                                                            │ (Protobuf)
                                                            │ 端口 8080
                                                            ▼
                                                    ┌─────────────────┐
                                                    │   Web 前端       │
                                                    │   (Vue 3)       │
                                                    └─────────────────┘
```

## 🚀 快速开始

### 1. 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake protobuf-compiler libprotobuf-dev

# 或者使用编译脚本自动安装
chmod +x build_protobuf.sh
./build_protobuf.sh
```

### 2. 编译项目

```bash
# 方式 1: 使用编译脚本（推荐）
./build_protobuf.sh

# 方式 2: 手动编译
make -f Makefile.protobuf
```

### 3. 启动后端服务器

```bash
./robot_backend_protobuf
```

你会看到：

```
[INFO] Robot Backend Server Starting...
[INFO] WebSocket server started successfully
[INFO] TCP server started successfully
==============================================
Robot Backend Server Started Successfully!
TCP Server (Robot):      0.0.0.0:9090
WebSocket Server (Web):  0.0.0.0:8080
==============================================
```

### 4. 打开 Web 前端

在浏览器中打开：

```bash
file:///home/ubuntu/lzy/libhv/examples/httpd/web/index.html
```

或者使用简单的 HTTP 服务器：

```bash
cd web
python3 -m http.server 3000
# 然后访问 http://localhost:3000
```

### 5. 运行测试客户端

```bash
# 先编译 Python Protobuf
protoc --python_out=. proto/robot.proto

# 运行测试客户端
python3 test_client.py
```

## 📦 文件结构

```
httpd/
├── proto/
│   └── robot.proto              # Protobuf 协议定义
├── web/
│   ├── index.html               # Web 前端界面
│   └── app.js                   # 前端逻辑
├── tcp_server_protobuf.h        # TCP 服务器头文件
├── tcp_server_protobuf.cpp      # TCP 服务器实现
├── websocket_server.h           # WebSocket 服务器头文件
├── websocket_server.cpp         # WebSocket 服务器实现
├── main_protobuf.cpp            # 主程序
├── device_manager.h             # 设备管理器
├── device_manager.cpp           # 设备管理器实现
├── Makefile.protobuf            # 编译配置
├── build_protobuf.sh            # 编译脚本
├── test_client.py               # Python 测试客户端
└── README_PROTOBUF.md           # 本文档
```

## 📡 Protobuf 消息类型

### 1. 设备注册 (MSG_REGISTER)

```protobuf
message DeviceRegister {
    string device_id = 1;
    string firmware_version = 2;
    int32 map_resolution = 3;
    bool has_occupancy = 4;
    bool has_semantic = 5;
    bool has_camera = 6;
    bool has_yolo = 7;
}
```

### 2. 实时状态 (MSG_TELEMETRY)

高频发送（10-50Hz），包含位姿、速度、电池等信息。

```protobuf
message Telemetry {
    double timestamp = 1;
    double x = 2;
    double y = 3;
    double yaw = 4;
    double vx = 5;
    double vy = 6;
    double w = 7;
    int32 battery_percent = 8;
    int32 signal_strength = 9;
    double coverage_rate = 12;
}
```

### 3. 地图更新 (MSG_MAP_UPDATE)

增量式地图数据，支持压缩。

```protobuf
message MapUpdate {
    double timestamp = 1;
    int32 block_x = 2;
    int32 block_y = 3;
    int32 width = 4;
    int32 height = 5;
    bytes occupancy_data = 6;
    bool is_compressed = 8;
}
```

### 4. 探索边界 (MSG_FRONTIER_UPDATE)

Active SLAM 的核心，标记未探索区域的边界。

```protobuf
message FrontierUpdate {
    double timestamp = 1;
    repeated Frontier frontiers = 2;
    int32 selected_frontier_id = 3;
}
```

### 5. YOLOv8 检测 (MSG_YOLO_DETECTION)

语义识别结果。

```protobuf
message YoloDetection {
    double timestamp = 1;
    int32 frame_id = 2;
    repeated BoundingBox boxes = 3;
}
```

### 6. 相机帧 (MSG_CAMERA_FRAME)

MJPEG 视频流。

```protobuf
message CameraFrame {
    double timestamp = 1;
    int32 frame_id = 2;
    bytes jpeg_data = 5;
}
```

## 🎨 Web 前端特性

### 设计风格

- **赛博朋克风格**：深色背景 + 荧光绿主题色
- **字体**：JetBrains Mono 等编程字体
- **动画**：脉冲效果、渐变、光晕

### 核心功能

1. **实时地图渲染** (Canvas)
   - 栅格地图显示
   - 机器人轨迹绘制
   - 探索边界高亮

2. **状态监控面板**
   - 位置、速度、航向
   - 电池、信号强度
   - 覆盖率进度条

3. **相机视图**
   - MJPEG 视频流
   - YOLOv8 检测框叠加

4. **设备管理**
   - 多设备列表
   - 设备切换
   - 在线状态

## 🔧 开发指南

### 添加新的消息类型

1. 在 `proto/robot.proto` 中定义消息
2. 重新编译 Protobuf：`make -f Makefile.protobuf proto`
3. 在 `tcp_server_protobuf.cpp` 中添加处理函数
4. 在 `web/app.js` 中添加前端处理逻辑

### 性能优化建议

1. **高频消息**（Telemetry）：直接转发，不做复杂处理
2. **中频消息**（Map Update）：可以做降采样或压缩
3. **低频消息**（Frontier）：可以做缓存和聚合
4. **视频流**：考虑使用 WebRTC 或降低帧率

### 调试技巧

```bash
# 查看 Protobuf 消息（需要 protoc）
protoc --decode=robot.RobotMessage proto/robot.proto < message.bin

# 监控网络流量
tcpdump -i lo -X port 9090

# 查看 WebSocket 连接
netstat -an | grep 8080
```

## 📊 性能指标

| 指标 | 目标值 |
|------|--------|
| TCP 延迟 | < 5ms |
| WebSocket 延迟 | < 20ms |
| 并发设备数 | 100+ |
| 地图更新频率 | 1-5 Hz |
| 状态更新频率 | 10-50 Hz |

## 🔒 安全建议

1. **生产环境**：添加 TLS/SSL 加密
2. **认证**：实现设备认证机制
3. **限流**：防止恶意客户端
4. **输入验证**：严格验证 Protobuf 消息

## 🐛 常见问题

### Q: 编译失败，找不到 protobuf

```bash
sudo apt-get install -y protobuf-compiler libprotobuf-dev
```

### Q: WebSocket 连接失败

检查防火墙设置，确保端口 8080 开放。

### Q: 前端无法显示地图

打开浏览器控制台，查看 JavaScript 错误。确保 WebSocket 连接成功。

### Q: 如何支持更多设备？

修改 `tcp_server_protobuf.cpp` 中的线程池大小：

```cpp
tcp_server_.setThreadNum(8);  // 增加到 8 个线程
```

## 📚 参考资料

- [libhv 文档](https://github.com/ithewei/libhv)
- [Protocol Buffers](https://developers.google.com/protocol-buffers)
- [WebSocket RFC](https://tools.ietf.org/html/rfc6455)
- [Active SLAM 论文](https://arxiv.org/abs/...)

## 📝 TODO

- [ ] 添加 Protobuf.js 支持（前端直接解析二进制）
- [ ] 实现地图压缩（zlib/lz4）
- [ ] 添加录制/回放功能
- [ ] 实现多用户权限管理
- [ ] 添加性能监控面板
- [ ] 支持 WebRTC 视频流

## 📄 许可证

MIT License

---

**作者**: Active SLAM Team  
**更新时间**: 2026-03-14
