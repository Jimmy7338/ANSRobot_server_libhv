# Active SLAM Backend System

基于 libhv 的高性能机器人后端系统 - Protobuf + WebSocket 双端口架构

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Protobuf](https://img.shields.io/badge/Protobuf-3.20-green.svg)](https://developers.google.com/protocol-buffers)

## 🎯 项目简介

这是一个完整的 Active SLAM 后端系统，采用现代化的双端口架构设计。系统使用 **Protobuf** 进行高效的二进制通信，通过 **TCP** 与机器人进行低延迟交互，通过 **WebSocket** 向浏览器实时推送数据，实现了完整的监控可视化。

### 核心特性

- 🚀 **高性能通信** - Protobuf 比 JSON 快 3-10 倍
- 🔌 **双端口架构** - TCP (9090) + WebSocket (8080) 隔离设计
- 🎨 **现代化界面** - 赛博朋克风格实时监控
- 📊 **Canvas 渲染** - 高性能地图可视化
- 🤖 **完整功能** - 地图、轨迹、探索边界、YOLOv8、相机流
- 📹 **视频流支持** - MJPEG + YOLOv8 检测框叠加

## 🏗️ 系统架构

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
                                                    │   (HTML5)       │
                                                    └─────────────────┘
```

## 🚀 快速开始

### 环境要求

- Ubuntu 20.04+ / Debian 11+
- GCC 9.0+ (支持 C++17)
- Protocol Buffers 3.0+
- Python 3.6+ (用于测试客户端)

### 安装依赖

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake protobuf-compiler libprotobuf-dev
```

### 编译项目

```bash
cd examples/httpd

# 一键部署
./deploy.sh

# 或手动编译
make -f Makefile.protobuf
```

### 启动系统

```bash
# 1. 启动后端服务器
./start_protobuf.sh

# 2. 新终端：启动 Web 前端
cd web && python3 -m http.server 3000

# 3. 新终端：运行测试客户端
python3 test_client.py
```

### 访问界面

在浏览器中打开：`http://localhost:3000`

## 📡 消息协议

### 消息类型

| 类型 | 频率 | 用途 |
|------|------|------|
| MSG_REGISTER | 一次 | 设备注册 |
| MSG_TELEMETRY | 10-50Hz | 实时状态（位姿、速度、电池） |
| MSG_MAP_UPDATE | 1-5Hz | 地图增量更新 |
| MSG_FRONTIER_UPDATE | 0.2-1Hz | 探索边界 |
| MSG_YOLO_DETECTION | 5-10Hz | 目标检测 |
| MSG_CAMERA_FRAME | 10-30Hz | 视频流 |

### Protobuf 定义

```protobuf
message Telemetry {
    double timestamp = 1;
    double x = 2;                   // 位置 x (m)
    double y = 3;                   // 位置 y (m)
    double yaw = 4;                 // 航向角 (rad)
    double vx = 5;                  // 线速度 x (m/s)
    double vy = 6;                  // 线速度 y (m/s)
    double w = 7;                   // 角速度 (rad/s)
    int32 battery_percent = 8;      // 电池百分比
    int32 signal_strength = 9;      // 信号强度
    double coverage_rate = 12;      // 覆盖率
}
```

## 🎨 前端界面

### 设计风格

- **配色方案**: 深色背景 + 荧光绿 (#00ff9d)
- **字体**: JetBrains Mono 编程字体
- **动画效果**: 脉冲、渐变、光晕
- **布局**: 3 列响应式网格

### 功能模块

1. **设备列表**（左侧）- 多设备管理和切换
2. **地图画布**（中央）- Canvas 实时渲染轨迹和边界
3. **实时状态**（右上）- 位置、速度、电池、覆盖率
4. **相机视图**（中下）- MJPEG 视频流 + YOLOv8 检测框
5. **探索边界**（右下）- 边界列表和信息增益

## 📊 性能指标

| 指标 | 目标值 | 说明 |
|------|--------|------|
| TCP 延迟 | < 5ms | 机器人通信延迟 |
| WebSocket 延迟 | < 20ms | 浏览器推送延迟 |
| 并发设备 | 100+ | 同时连接的设备数 |
| 状态更新频率 | 10-50 Hz | 位姿数据更新频率 |
| 地图更新频率 | 1-5 Hz | 地图数据更新频率 |

## 📁 项目结构

```
examples/httpd/
├── proto/
│   └── robot.proto              # Protobuf 协议定义
├── web/
│   ├── index.html               # Web 前端界面
│   └── app.js                   # 前端逻辑
├── tcp_server_protobuf.h/cpp    # TCP 服务器
├── websocket_server.h/cpp       # WebSocket 服务器
├── main_protobuf.cpp            # 主程序
├── device_manager.h/cpp         # 设备管理器
├── Makefile.protobuf            # 编译配置
├── deploy.sh                    # 部署脚本
├── test_client.py               # 测试客户端
└── README_PROTOBUF.md           # 技术文档
```

## 🔧 开发指南

### 添加新消息类型

1. 在 `proto/robot.proto` 中定义消息
2. 重新编译：`make -f Makefile.protobuf proto`
3. 在 `tcp_server_protobuf.cpp` 中添加处理函数
4. 在 `web/app.js` 中添加前端处理逻辑

### 性能优化

- **地图压缩**: 使用 zlib/lz4 压缩占用栅格数据
- **降频策略**: 对高频消息（如视频流）进行降采样
- **批量处理**: 合并多个小消息减少网络开销

## 📚 文档

- [README_PROTOBUF.md](examples/httpd/README_PROTOBUF.md) - 完整技术文档
- [IMPLEMENTATION_GUIDE.md](examples/httpd/IMPLEMENTATION_GUIDE.md) - 实施指南
- [ARCHITECTURE_IMPROVEMENT.md](examples/httpd/ARCHITECTURE_IMPROVEMENT.md) - 架构设计

## 🐛 故障排查

### 编译失败

```bash
# 安装依赖
sudo apt-get install -y protobuf-compiler libprotobuf-dev

# 清理重新编译
make -f Makefile.protobuf clean
make -f Makefile.protobuf
```

### WebSocket 连接失败

```bash
# 检查端口
netstat -tuln | grep 8080

# 检查防火墙
sudo ufw allow 8080
```

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 许可证

MIT License

## 🔗 相关链接

- [libhv](https://github.com/ithewei/libhv) - 高性能网络库
- [Protocol Buffers](https://developers.google.com/protocol-buffers) - 数据序列化
- [WebSocket API](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket) - WebSocket 文档

## 📧 联系方式

- GitHub: [@Jimmy7338](https://github.com/Jimmy7338)
- Repository: [ANSRobot_server_libhv](https://github.com/Jimmy7338/ANSRobot_server_libhv)

---

**⭐ 如果这个项目对你有帮助，请给个 Star！**
