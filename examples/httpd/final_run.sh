#!/bin/bash

# 最终的启动脚本 - 包含所有必要的环境设置

LIBHV_DIR="/home/ubuntu/lzy/libhv"
HTTPD_DIR="$LIBHV_DIR/examples/httpd"

export LD_LIBRARY_PATH="$LIBHV_DIR/lib:$LD_LIBRARY_PATH"

case "${1:-help}" in
    build)
        echo "Building libhv..."
        cd "$LIBHV_DIR"
        make libhv
        
        echo ""
        echo "Building robot backend..."
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
        
        if curl -s http://localhost:8080/health > /dev/null 2>&1; then
            echo "✓ Backend started successfully"
            echo ""
            echo "Test commands:"
            echo "  curl http://localhost:8080/health"
            echo "  python /home/ubuntu/lzy/ANS/Neural-SLAM/test_backend_communication.py"
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
        echo ""
        echo "Quick start:"
        echo "  $0 build"
        echo "  $0 start"
        echo "  $0 test"
        ;;
esac
