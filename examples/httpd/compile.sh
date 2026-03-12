#!/bin/bash

# 简单的编译脚本 - 直接使用 libhv 的编译系统

cd /home/ubuntu/lzy/libhv/examples/httpd

# 编译所有源文件
echo "Compiling robot backend..."

# 获取 libhv 的编译标志
LIBHV_DIR="../../.."
CXXFLAGS="-std=c++11 -Wall -O2 -fPIC"
INCLUDES="-I. -I$LIBHV_DIR -I$LIBHV_DIR/base -I$LIBHV_DIR/event -I$LIBHV_DIR/http -I$LIBHV_DIR/cpputil -I$LIBHV_DIR/ssl -I$LIBHV_DIR/util"
LDFLAGS="-L$LIBHV_DIR/lib -lhv -lpthread"

# 编译源文件
g++ $CXXFLAGS $INCLUDES -c protocol.h 2>/dev/null || true
g++ $CXXFLAGS $INCLUDES -c device_manager.cpp -o device_manager.o
g++ $CXXFLAGS $INCLUDES -c robot_handler.cpp -o robot_handler.o
g++ $CXXFLAGS $INCLUDES -c robot_router.cpp -o robot_router.o
g++ $CXXFLAGS $INCLUDES -c handler.cpp -o handler.o
g++ $CXXFLAGS $INCLUDES -c router.cpp -o router.o
g++ $CXXFLAGS $INCLUDES -c httpd.cpp -o httpd.o

# 链接
g++ $CXXFLAGS -o httpd httpd.o handler.o router.o device_manager.o robot_handler.o robot_router.o $LDFLAGS

if [ -f httpd ]; then
    echo "✓ Compilation successful!"
    echo "Binary: $(pwd)/httpd"
else
    echo "✗ Compilation failed"
    exit 1
fi
