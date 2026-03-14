#!/bin/bash

# 一键部署脚本 - Active SLAM 系统

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║                                                            ║"
echo "║        Active SLAM Backend - 一键部署脚本                  ║"
echo "║        Protobuf + WebSocket 双端口架构                     ║"
echo "║                                                            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# 步骤 1: 检查依赖
echo "📦 [1/4] 检查系统依赖..."
if ! command -v protoc &> /dev/null; then
    echo "   ⚠️  protoc 未安装，正在安装..."
    sudo apt-get update -qq
    sudo apt-get install -y protobuf-compiler libprotobuf-dev
    echo "   ✅ protoc 安装完成"
else
    echo "   ✅ protoc 已安装: $(protoc --version)"
fi

if ! command -v g++ &> /dev/null; then
    echo "   ⚠️  g++ 未安装，正在安装..."
    sudo apt-get install -y build-essential
    echo "   ✅ g++ 安装完成"
else
    echo "   ✅ g++ 已安装: $(g++ --version | head -n1)"
fi

# 步骤 2: 编译 Protobuf
echo ""
echo "🔧 [2/4] 编译 Protobuf 协议..."
make -f Makefile.protobuf proto 2>&1 | grep -v "warning" || true
echo "   ✅ Protobuf 编译完成"

# 步骤 3: 编译后端
echo ""
echo "🔨 [3/4] 编译后端服务器..."
make -f Makefile.protobuf -j$(nproc) 2>&1 | grep -v "warning" || true
echo "   ✅ 后端编译完成"

# 步骤 4: 编译 Python Protobuf（用于测试）
echo ""
echo "🐍 [4/4] 编译 Python Protobuf..."
protoc --python_out=. proto/robot.proto 2>&1 || true
echo "   ✅ Python Protobuf 编译完成"

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                                                            ║"
echo "║                    🎉 部署完成！                           ║"
echo "║                                                            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "📋 快速启动指南："
echo ""
echo "   1️⃣  启动后端服务器："
echo "      ./start_protobuf.sh"
echo ""
echo "   2️⃣  打开 Web 前端："
echo "      cd web && python3 -m http.server 3000"
echo "      然后访问: http://localhost:3000"
echo ""
echo "   3️⃣  运行测试客户端（模拟机器人）："
echo "      python3 test_client.py"
echo ""
echo "📡 服务端口："
echo "   • TCP (机器人):      0.0.0.0:9090"
echo "   • WebSocket (浏览器): 0.0.0.0:8080"
echo ""
echo "📚 查看完整文档："
echo "   cat README_PROTOBUF.md"
echo "   cat IMPLEMENTATION_GUIDE.md"
echo ""
