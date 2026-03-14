// Protobuf 消息类型枚举（与 robot.proto 对应）
const MessageType = {
    MSG_UNKNOWN: 0,
    MSG_REGISTER: 1,
    MSG_TELEMETRY: 2,
    MSG_MAP_UPDATE: 3,
    MSG_FRONTIER_UPDATE: 4,
    MSG_YOLO_DETECTION: 5,
    MSG_CAMERA_FRAME: 6,
    MSG_ACK: 100,
    MSG_ERROR: 101
};

class RobotMonitor {
    constructor() {
        this.ws = null;
        this.devices = new Map();
        this.currentDevice = null;
        this.mapCanvas = document.getElementById('mapCanvas');
        this.mapCtx = this.mapCanvas.getContext('2d');
        this.trajectoryPoints = [];
        this.frontiers = [];
        
        this.initCanvas();
        this.connect();
    }
    
    initCanvas() {
        // 设置 Canvas 尺寸
        const container = this.mapCanvas.parentElement;
        this.mapCanvas.width = container.clientWidth;
        this.mapCanvas.height = container.clientHeight;
        
        // 初始化地图（灰色背景）
        this.mapCtx.fillStyle = '#1a1f2e';
        this.mapCtx.fillRect(0, 0, this.mapCanvas.width, this.mapCanvas.height);
        
        // 绘制网格
        this.drawGrid();
    }
    
    drawGrid() {
        const ctx = this.mapCtx;
        const gridSize = 50;
        
        ctx.strokeStyle = 'rgba(0, 255, 157, 0.1)';
        ctx.lineWidth = 1;
        
        // 垂直线
        for (let x = 0; x < this.mapCanvas.width; x += gridSize) {
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, this.mapCanvas.height);
            ctx.stroke();
        }
        
        // 水平线
        for (let y = 0; y < this.mapCanvas.height; y += gridSize) {
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(this.mapCanvas.width, y);
            ctx.stroke();
        }
    }
    
    connect() {
        const wsUrl = 'ws://localhost:8080';
        console.log('Connecting to', wsUrl);
        
        this.ws = new WebSocket(wsUrl);
        this.ws.binaryType = 'arraybuffer';
        
        this.ws.onopen = () => {
            console.log('WebSocket connected');
            document.getElementById('connectionStatus').textContent = '已连接';
        };
        
        this.ws.onmessage = (event) => {
            this.handleMessage(event.data);
        };
        
        this.ws.onerror = (error) => {
            console.error('WebSocket error:', error);
            document.getElementById('connectionStatus').textContent = '连接错误';
        };
        
        this.ws.onclose = () => {
            console.log('WebSocket closed, reconnecting...');
            document.getElementById('connectionStatus').textContent = '连接断开';
            setTimeout(() => this.connect(), 3000);
        };
    }
    
    handleMessage(data) {
        // 解析 Protobuf 消息（简化版，实际需要 protobuf.js）
        // 这里我们假设后端也可以发送 JSON 格式
        try {
            if (data instanceof ArrayBuffer) {
                // 二进制 Protobuf 数据
                this.handleProtobufMessage(data);
            } else {
                // JSON 数据（用于调试）
                const msg = JSON.parse(data);
                this.handleJsonMessage(msg);
            }
        } catch (e) {
            console.error('Failed to parse message:', e);
        }
    }
    
    handleProtobufMessage(buffer) {
        // 这里需要使用 protobuf.js 来解析
        // 为了演示，我们先跳过
        console.log('Received protobuf message, size:', buffer.byteLength);
    }
    
    handleJsonMessage(msg) {
        console.log('Received message:', msg);
        
        switch (msg.type) {
            case MessageType.MSG_TELEMETRY:
                this.handleTelemetry(msg);
                break;
            case MessageType.MSG_MAP_UPDATE:
                this.handleMapUpdate(msg);
                break;
            case MessageType.MSG_FRONTIER_UPDATE:
                this.handleFrontierUpdate(msg);
                break;
            case MessageType.MSG_YOLO_DETECTION:
                this.handleYoloDetection(msg);
                break;
            case MessageType.MSG_CAMERA_FRAME:
                this.handleCameraFrame(msg);
                break;
        }
    }
    
    handleTelemetry(msg) {
        const telem = msg.telemetry;
        const deviceId = msg.device_id;
        
        // 更新设备列表
        if (!this.devices.has(deviceId)) {
            this.devices.set(deviceId, {
                id: deviceId,
                telemetry: telem
            });
            this.updateDeviceList();
        } else {
            this.devices.get(deviceId).telemetry = telem;
        }
        
        // 如果是当前选中的设备，更新显示
        if (!this.currentDevice || this.currentDevice === deviceId) {
            this.currentDevice = deviceId;
            this.updateTelemetryDisplay(telem);
            this.updateTrajectory(telem.x, telem.y);
        }
    }
    
    updateTelemetryDisplay(telem) {
        document.getElementById('posX').textContent = telem.x.toFixed(2) + ' m';
        document.getElementById('posY').textContent = telem.y.toFixed(2) + ' m';
        document.getElementById('yaw').textContent = (telem.yaw * 180 / Math.PI).toFixed(1) + '°';
        
        const velocity = Math.sqrt(telem.vx * telem.vx + telem.vy * telem.vy);
        document.getElementById('velocity').textContent = velocity.toFixed(2) + ' m/s';
        
        document.getElementById('battery').textContent = telem.battery_percent + '%';
        document.getElementById('signal').textContent = telem.signal_strength + '%';
        
        if (telem.coverage_rate !== undefined) {
            const coverage = (telem.coverage_rate * 100).toFixed(1);
            document.getElementById('coverageBar').style.width = coverage + '%';
            document.getElementById('coverageText').textContent = '覆盖率: ' + coverage + '%';
        }
    }
    
    updateTrajectory(x, y) {
        // 将世界坐标转换为画布坐标
        const scale = 20; // 20 pixels per meter
        const centerX = this.mapCanvas.width / 2;
        const centerY = this.mapCanvas.height / 2;
        
        const canvasX = centerX + x * scale;
        const canvasY = centerY - y * scale; // Y 轴翻转
        
        this.trajectoryPoints.push({ x: canvasX, y: canvasY });
        
        // 限制轨迹点数量
        if (this.trajectoryPoints.length > 500) {
            this.trajectoryPoints.shift();
        }
        
        this.redrawMap();
    }
    
    redrawMap() {
        // 清空画布
        this.mapCtx.fillStyle = '#1a1f2e';
        this.mapCtx.fillRect(0, 0, this.mapCanvas.width, this.mapCanvas.height);
        this.drawGrid();
        
        // 绘制轨迹
        if (this.trajectoryPoints.length > 1) {
            this.mapCtx.strokeStyle = '#00ff9d';
            this.mapCtx.lineWidth = 2;
            this.mapCtx.beginPath();
            this.mapCtx.moveTo(this.trajectoryPoints[0].x, this.trajectoryPoints[0].y);
            
            for (let i = 1; i < this.trajectoryPoints.length; i++) {
                this.mapCtx.lineTo(this.trajectoryPoints[i].x, this.trajectoryPoints[i].y);
            }
            this.mapCtx.stroke();
            
            // 绘制当前位置
            const last = this.trajectoryPoints[this.trajectoryPoints.length - 1];
            this.mapCtx.fillStyle = '#00ff9d';
            this.mapCtx.beginPath();
            this.mapCtx.arc(last.x, last.y, 5, 0, Math.PI * 2);
            this.mapCtx.fill();
        }
        
        // 绘制探索边界
        this.drawFrontiers();
    }
    
    handleMapUpdate(msg) {
        const mapData = msg.map_update;
        console.log('Map update:', mapData);
        
        // 这里需要根据实际的地图数据格式来绘制
        // 占用栅格地图的绘制
    }
    
    handleFrontierUpdate(msg) {
        const frontierData = msg.frontier_update;
        this.frontiers = frontierData.frontiers || [];
        
        console.log('Frontier update:', this.frontiers.length, 'frontiers');
        
        this.redrawMap();
        this.updateFrontierList();
    }
    
    drawFrontiers() {
        const scale = 20;
        const centerX = this.mapCanvas.width / 2;
        const centerY = this.mapCanvas.height / 2;
        
        this.frontiers.forEach(frontier => {
            const x = centerX + frontier.centroid_x * scale;
            const y = centerY - frontier.centroid_y * scale;
            
            // 绘制边界点
            this.mapCtx.fillStyle = frontier.is_selected ? '#ff0080' : '#ff6b00';
            this.mapCtx.beginPath();
            this.mapCtx.arc(x, y, frontier.is_selected ? 8 : 5, 0, Math.PI * 2);
            this.mapCtx.fill();
            
            // 绘制光晕
            this.mapCtx.strokeStyle = frontier.is_selected ? 'rgba(255, 0, 128, 0.5)' : 'rgba(255, 107, 0, 0.5)';
            this.mapCtx.lineWidth = 2;
            this.mapCtx.beginPath();
            this.mapCtx.arc(x, y, 12, 0, Math.PI * 2);
            this.mapCtx.stroke();
        });
    }
    
    updateFrontierList() {
        const container = document.getElementById('frontierList');
        
        if (this.frontiers.length === 0) {
            container.innerHTML = '<div class="no-data">暂无探索边界</div>';
            return;
        }
        
        container.innerHTML = this.frontiers.map(f => `
            <div class="metric">
                <span class="metric-label">边界 #${f.id}</span>
                <span class="metric-value">增益: ${f.gain.toFixed(2)}</span>
            </div>
        `).join('');
    }
    
    handleYoloDetection(msg) {
        const detection = msg.yolo_detection;
        console.log('YOLO detection:', detection.boxes.length, 'objects');
        
        // 这里可以在相机视图上叠加检测框
    }
    
    handleCameraFrame(msg) {
        const frame = msg.camera_frame;
        
        // 将 JPEG 数据转换为 Base64 并显示
        const base64 = btoa(String.fromCharCode(...new Uint8Array(frame.jpeg_data)));
        document.getElementById('cameraView').src = 'data:image/jpeg;base64,' + base64;
    }
    
    updateDeviceList() {
        const container = document.getElementById('deviceList');
        
        if (this.devices.size === 0) {
            container.innerHTML = '<div class="no-data">等待设备连接...</div>';
            return;
        }
        
        container.innerHTML = Array.from(this.devices.values()).map(device => `
            <div class="device-item ${device.id === this.currentDevice ? 'active' : ''}" 
                 onclick="monitor.selectDevice('${device.id}')">
                <div class="device-name">${device.id}</div>
                <div class="device-status">
                    电池: ${device.telemetry.battery_percent}% | 
                    信号: ${device.telemetry.signal_strength}%
                </div>
            </div>
        `).join('');
    }
    
    selectDevice(deviceId) {
        this.currentDevice = deviceId;
        this.trajectoryPoints = [];
        this.updateDeviceList();
    }
}

// 启动监控系统
const monitor = new RobotMonitor();
