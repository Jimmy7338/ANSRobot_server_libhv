#!/bin/bash

# 简单的编译脚本 - 用于编译 libhv 和机器人后端

set -e

LIBHV_DIR="/home/ubuntu/lzy/libhv"
HTTPD_DIR="$LIBHV_DIR/examples/httpd"

echo "=========================================="
echo "Building libhv and Robot Backend"
echo "=========================================="

# 1. 配置 libhv
echo "Step 1: Configuring libhv..."
cd "$LIBHV_DIR"
if [ ! -f "config.mk" ]; then
    ./configure
fi

# 2. 编译 libhv
echo ""
echo "Step 2: Building libhv..."
make

# 3. 编译 httpd
echo ""
echo "Step 3: Building httpd with robot backend..."
cd "$HTTPD_DIR"
make -f Makefile.robot

echo ""
echo "=========================================="
echo "✓ Build completed successfully!"
echo "=========================================="
echo ""
echo "Next steps:"
echo "1. Start the backend:"
echo "   cd $HTTPD_DIR"
echo "   ./httpd -c etc/httpd.conf -p 8080"
echo ""
echo "2. Test the connection:"
echo "   curl http://localhost:8080/health"
echo ""
