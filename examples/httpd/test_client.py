#!/usr/bin/env python3
"""
Active SLAM 机器人客户端示例
使用 Protobuf 与后端通信
"""

import socket
import time
import math
import sys
import os

# 添加 proto 路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'proto'))

try:
    import robot_pb2
except ImportError:
    print("❌ 请先编译 Protobuf:")
    print("   protoc --python_out=. proto/robot.proto")
    sys.exit(1)


class RobotClient:
    def __init__(self, host='localhost', port=9090):
        self.host = host
        self.port = port
        self.sock = None
        self.device_id = "robot_001"
        
    def connect(self):
        """连接到后端服务器"""
        print(f"Connecting to {self.host}:{self.port}...")
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))
        print("✅ Connected")
        
    def send_message(self, msg):
        """发送 Protobuf 消息"""
        data = msg.SerializeToString()
        self.sock.sendall(data)
        
    def register(self):
        """注册设备"""
        print(f"Registering device: {self.device_id}")
        
        msg = robot_pb2.RobotMessage()
        msg.type = robot_pb2.MSG_REGISTER
        msg.device_id = self.device_id
        msg.timestamp = time.time()
        
        reg = msg.register_msg
        reg.device_id = self.device_id
        reg.firmware_version = "1.0.0"
        reg.map_resolution = 5
        reg.has_occupancy = True
        reg.has_semantic = False
        reg.has_camera = True
        reg.has_yolo = True
        
        self.send_message(msg)
        print("✅ Registration sent")
        
    def send_telemetry(self, x, y, yaw, vx=0.0, vy=0.0, w=0.0):
        """发送状态数据"""
        msg = robot_pb2.RobotMessage()
        msg.type = robot_pb2.MSG_TELEMETRY
        msg.device_id = self.device_id
        msg.timestamp = time.time()
        
        telem = msg.telemetry
        telem.timestamp = time.time()
        telem.x = x
        telem.y = y
        telem.yaw = yaw
        telem.vx = vx
        telem.vy = vy
        telem.w = w
        telem.battery_percent = 85
        telem.signal_strength = 95
        telem.explored_area = 50.0
        telem.total_area = 100.0
        telem.coverage_rate = 0.5
        
        self.send_message(msg)
        
    def send_map_update(self, block_x, block_y, width, height):
        """发送地图更新"""
        msg = robot_pb2.RobotMessage()
        msg.type = robot_pb2.MSG_MAP_UPDATE
        msg.device_id = self.device_id
        msg.timestamp = time.time()
        
        map_update = msg.map_update
        map_update.timestamp = time.time()
        map_update.block_x = block_x
        map_update.block_y = block_y
        map_update.width = width
        map_update.height = height
        
        # 生成模拟的占用栅格数据
        map_update.occupancy_data = bytes([127] * (width * height))
        map_update.is_compressed = False
        
        self.send_message(msg)
        print(f"📍 Map update sent: block ({block_x}, {block_y})")
        
    def send_frontier_update(self, frontiers):
        """发送探索边界"""
        msg = robot_pb2.RobotMessage()
        msg.type = robot_pb2.MSG_FRONTIER_UPDATE
        msg.device_id = self.device_id
        msg.timestamp = time.time()
        
        frontier_update = msg.frontier_update
        frontier_update.timestamp = time.time()
        
        for i, (x, y, gain) in enumerate(frontiers):
            f = frontier_update.frontiers.add()
            f.id = i
            f.centroid_x = x
            f.centroid_y = y
            f.size = 100
            f.gain = gain
            f.is_selected = (i == 0)
        
        frontier_update.selected_frontier_id = 0
        
        self.send_message(msg)
        print(f"🎯 Frontier update sent: {len(frontiers)} frontiers")
        
    def simulate_circular_motion(self, duration=60):
        """模拟圆周运动"""
        print(f"\n🤖 Starting simulation for {duration} seconds...")
        print("Press Ctrl+C to stop\n")
        
        radius = 5.0  # 5米半径
        angular_velocity = 0.2  # rad/s
        
        start_time = time.time()
        last_map_update = 0
        last_frontier_update = 0
        
        try:
            while time.time() - start_time < duration:
                t = time.time() - start_time
                
                # 计算位置
                angle = angular_velocity * t
                x = radius * math.cos(angle)
                y = radius * math.sin(angle)
                yaw = angle + math.pi / 2
                
                # 计算速度
                vx = -radius * angular_velocity * math.sin(angle)
                vy = radius * angular_velocity * math.cos(angle)
                w = angular_velocity
                
                # 发送状态（高频 10Hz）
                self.send_telemetry(x, y, yaw, vx, vy, w)
                print(f"📡 Telemetry: x={x:.2f}, y={y:.2f}, yaw={yaw:.2f}")
                
                # 发送地图更新（中频 1Hz）
                if t - last_map_update > 1.0:
                    block_x = int(x / 2)
                    block_y = int(y / 2)
                    self.send_map_update(block_x, block_y, 50, 50)
                    last_map_update = t
                
                # 发送探索边界（低频 0.2Hz）
                if t - last_frontier_update > 5.0:
                    frontiers = [
                        (x + 3, y + 2, 0.8),
                        (x - 2, y + 3, 0.6),
                        (x + 1, y - 3, 0.4),
                    ]
                    self.send_frontier_update(frontiers)
                    last_frontier_update = t
                
                time.sleep(0.1)  # 10Hz
                
        except KeyboardInterrupt:
            print("\n\n⏹️  Simulation stopped by user")
            
    def close(self):
        """关闭连接"""
        if self.sock:
            self.sock.close()
            print("Connection closed")


def main():
    client = RobotClient()
    
    try:
        client.connect()
        client.register()
        time.sleep(1)
        client.simulate_circular_motion(duration=300)  # 5分钟
        
    except Exception as e:
        print(f"❌ Error: {e}")
        
    finally:
        client.close()


if __name__ == '__main__':
    main()
