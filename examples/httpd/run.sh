#!/bin/bash

# 简化的启动脚本

LIBHV_DIR="/home/ubuntu/lzy/libhv"
HTTPD_DIR="$LIBHV_DIR/examples/httpd"

case "${1:-help}" in
    build)
        echo "Building libhv and robot backend..."
        cd "$LIBHV_DIR"
        if [ ! -f "config.mk" ]; then
            ./configure
        fi
        make
        cd "$HTTPD_DIR"
        make -f Makefile.robot
        echo "✓ Build completed"
        ;;
    
    start)
        echo "Starting backend on port 8080..."
        cd "$HTTPD_DIR"
        mkdir -p logs
        ./httpd -c etc/httpd.conf -p 8080 &
        sleep 2
        if curl -s http://localhost:8080/health > /dev/null; then
            echo "✓ Backend started successfully"
            echo "Test: curl http://localhost:8080/health"
        else
            echo "✗ Backend failed to start"
        fi
        ;;
    
    stop)
        echo "Stopping backend..."
        pkill -f "httpd.*8080" || true
        echo "✓ Backend stopped"
        ;;
    
    test)
        echo "Testing backend..."
        cd /home/ubuntu/lzy/ANS/Neural-SLAM
        python test_backend_communication.py
        ;;
    
    logs)
        tail -f "$HTTPD_DIR/logs/robot_backend.log"
        ;;
    
    *)
        echo "Usage: $0 {build|start|stop|test|logs}"
        echo ""
        echo "Commands:"
        echo "  build  - Build libhv and robot backend"
        echo "  start  - Start the backend server"
        echo "  stop   - Stop the backend server"
        echo "  test   - Run tests"
        echo "  logs   - View backend logs"
        ;;
esac
