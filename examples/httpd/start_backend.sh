#!/bin/bash

# Robot Backend Quick Start Script
# 用于快速启动后端服务器和测试

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIBHV_DIR="$SCRIPT_DIR/../.."
HTTPD_DIR="$LIBHV_DIR/examples/httpd"
NEURAL_SLAM_DIR="$SCRIPT_DIR/../ANS/Neural-SLAM"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ============================================================================
# 函数定义
# ============================================================================

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

# ============================================================================
# 编译后端
# ============================================================================

build_backend() {
    print_header "Building Robot Backend"
    
    cd "$HTTPD_DIR"
    
    print_info "Cleaning old build..."
    make clean > /dev/null 2>&1 || true
    
    print_info "Compiling..."
    if make > /dev/null 2>&1; then
        print_success "Backend compiled successfully"
        return 0
    else
        print_error "Backend compilation failed"
        print_info "Running make again with verbose output:"
        make
        return 1
    fi
}

# ============================================================================
# 启动后端
# ============================================================================

start_backend() {
    print_header "Starting Robot Backend"
    
    cd "$HTTPD_DIR"
    
    # 检查端口是否被占用
    if lsof -i :8080 > /dev/null 2>&1; then
        print_warning "Port 8080 is already in use"
        print_info "Killing existing process..."
        lsof -i :8080 | grep -v COMMAND | awk '{print $2}' | xargs kill -9 2>/dev/null || true
        sleep 1
    fi
    
    # 创建日志目录
    mkdir -p logs
    
    print_info "Starting httpd on port 8080..."
    ./httpd -c etc/httpd.conf -p 8080 &
    BACKEND_PID=$!
    
    sleep 1
    
    # 检查后端是否启动成功
    if kill -0 $BACKEND_PID 2>/dev/null; then
        print_success "Backend started (PID: $BACKEND_PID)"
        echo $BACKEND_PID > /tmp/robot_backend.pid
        return 0
    else
        print_error "Failed to start backend"
        return 1
    fi
}

# ============================================================================
# 测试后端
# ============================================================================

test_backend() {
    print_header "Testing Backend Connection"
    
    # 等待后端启动
    sleep 2
    
    # 测试健康检查
    print_info "Testing health check..."
    if curl -s http://localhost:8080/health > /dev/null; then
        print_success "Backend is healthy"
    else
        print_error "Backend health check failed"
        return 1
    fi
    
    # 运行完整测试
    print_info "Running full test suite..."
    cd "$NEURAL_SLAM_DIR"
    
    if python test_backend_communication.py --backend-url http://localhost:8080; then
        print_success "All tests passed"
        return 0
    else
        print_error "Some tests failed"
        return 1
    fi
}

# ============================================================================
# 停止后端
# ============================================================================

stop_backend() {
    print_header "Stopping Robot Backend"
    
    if [ -f /tmp/robot_backend.pid ]; then
        PID=$(cat /tmp/robot_backend.pid)
        if kill -0 $PID 2>/dev/null; then
            print_info "Killing process $PID..."
            kill $PID
            sleep 1
            print_success "Backend stopped"
            rm /tmp/robot_backend.pid
        fi
    else
        print_warning "Backend PID file not found"
        # 尝试杀死所有 httpd 进程
        pkill -f "httpd.*8080" || true
        print_success "Killed all httpd processes"
    fi
}

# ============================================================================
# 显示状态
# ============================================================================

show_status() {
    print_header "Backend Status"
    
    if curl -s http://localhost:8080/health > /dev/null 2>&1; then
        print_success "Backend is running"
        
        # 获取设备列表
        DEVICES=$(curl -s http://localhost:8080/api/devices | grep -o '"devices":\[[^]]*\]')
        print_info "Connected devices: $DEVICES"
    else
        print_error "Backend is not running"
    fi
}

# ============================================================================
# 显示日志
# ============================================================================

show_logs() {
    print_header "Backend Logs"
    
    if [ -f "$HTTPD_DIR/logs/robot_backend.log" ]; then
        tail -f "$HTTPD_DIR/logs/robot_backend.log"
    else
        print_error "Log file not found"
    fi
}

# ============================================================================
# 主函数
# ============================================================================

main() {
    case "${1:-help}" in
        build)
            build_backend
            ;;
        start)
            build_backend && start_backend && test_backend
            ;;
        stop)
            stop_backend
            ;;
        restart)
            stop_backend
            sleep 1
            start_backend && test_backend
            ;;
        test)
            test_backend
            ;;
        status)
            show_status
            ;;
        logs)
            show_logs
            ;;
        clean)
            cd "$HTTPD_DIR"
            make clean
            print_success "Cleaned"
            ;;
        help|*)
            print_header "Robot Backend Quick Start"
            echo ""
            echo "Usage: $0 [command]"
            echo ""
            echo "Commands:"
            echo "  build       - Build the backend"
            echo "  start       - Build and start the backend"
            echo "  stop        - Stop the backend"
            echo "  restart     - Restart the backend"
            echo "  test        - Run tests"
            echo "  status      - Show backend status"
            echo "  logs        - Show backend logs (tail -f)"
            echo "  clean       - Clean build files"
            echo "  help        - Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0 start       # Start backend and run tests"
            echo "  $0 status      # Check if backend is running"
            echo "  $0 logs        # View backend logs"
            echo ""
            ;;
    esac
}

# ============================================================================
# 执行主函数
# ============================================================================

main "$@"
