# ANSRobot Server - libhv 版本

🤖 基于 libhv 的高性能机器人后端系统

## ✨ 特性

- ⚡ **Protobuf + WebSocket** 双端口架构
- 🚀 **高性能通信** - 比 JSON 快 3-10 倍
- 🎨 **实时监控界面** - 赛博朋克风格
- 📊 **Canvas 地图渲染** - 轨迹、边界可视化
- 🤖 **Active SLAM** - 完整的探索功能

## 🚀 快速开始

```bash
cd examples/httpd

# 一键部署
./deploy.sh

# 启动后端
./start_protobuf.sh

# 启动前端（新终端）
cd web && python3 -m http.server 3000

# 运行测试（新终端）
python3 test_client.py
```

访问：http://localhost:3000

## 📡 架构

```
机器人 → TCP:9090 (Protobuf) → C++ 后端 → WebSocket:8080 → 浏览器
```

## 📚 文档

- [完整文档](examples/httpd/README_PROTOBUF.md)
- [实施指南](examples/httpd/IMPLEMENTATION_GUIDE.md)

## 📄 许可证

MIT License
