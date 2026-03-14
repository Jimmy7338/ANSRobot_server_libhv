#!/bin/bash
# Active SLAM 演示脚本

echo "🚀 Active SLAM 系统演示"
echo ""

# 检查编译
if [ ! -f "robot_backend_protobuf" ]; then
    echo "⚠️  未编译，正在部署..."
    ./deploy.sh
fi

# 启动后端
echo "启动后端..."
./robot_backend_protobuf &
BACKEND_PID=$!
sleep 2

# 启动前端
echo "启动前端..."
cd web && python3 -m http.server 3000 &
FRONTEND_PID=$!
cd ..
sleep 1

echo ""
echo "✅ 系统已启动！"
echo ""
echo "📱 打开浏览器访问: http://localhost:3000"
echo ""
read -p "按 Enter 启动测试客户端..."

# 运行测试
python3 test_client.py

# 清理
echo ""
echo "清理中..."
kill $BACKEND_PID $FRONTEND_PID 2>/dev/null
echo "完成！"
