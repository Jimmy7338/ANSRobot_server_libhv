#!/bin/bash

# Active SLAM 后端编译脚本

set -e

echo "=========================================="
echo "Active SLAM Backend - Build Script"
echo "=========================================="

# 检查依赖
echo "Checking dependencies..."

if ! command -v protoc &> /dev/null; then
    echo "❌ protoc not found. Installing..."
    sudo apt-get update
    sudo apt-get install -y protobuf-compiler libprotobuf-dev
fi

if ! command -v g++ &> /dev/null; then
    echo "❌ g++ not found. Installing..."
    sudo apt-get install -y build-essential
fi

echo "✅ All dependencies installed"

# 编译 libhv（如果需要）
if [ ! -f "../../lib/libhv.a" ] && [ ! -f "../../lib/libhv.so" ]; then
    echo ""
    echo "Building libhv..."
    cd ../..
    mkdir -p build
    cd build
    cmake ..
    make -j$(nproc)
    cd -
    echo "✅ libhv built successfully"
fi

# 编译 Protobuf
echo ""
echo "Compiling Protobuf definitions..."
make -f Makefile.protobuf proto
echo "✅ Protobuf compiled"

# 编译后端
echo ""
echo "Building backend server..."
make -f Makefile.protobuf -j$(nproc)
echo "✅ Backend built successfully"

echo ""
echo "=========================================="
echo "Build complete!"
echo "=========================================="
echo ""
echo "To run the server:"
echo "  ./robot_backend_protobuf"
echo ""
echo "Server will listen on:"
echo "  - TCP (Robot):      0.0.0.0:9090"
echo "  - WebSocket (Web):  0.0.0.0:8080"
echo ""
echo "Open web interface:"
echo "  file://$(pwd)/web/index.html"
echo ""
