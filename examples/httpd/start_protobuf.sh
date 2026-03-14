#!/bin/bash

# 快速启动脚本

echo "🚀 Starting Active SLAM Backend..."

# 检查是否已编译
if [ ! -f "robot_backend_protobuf" ]; then
    echo "⚠️  Backend not built. Building now..."
    ./build_protobuf.sh
fi

# 启动后端
./robot_backend_protobuf
