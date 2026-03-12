# libhv 本地部署指南

## 项目信息
- **项目地址**: https://github.com/ithewei/libhv
- **本地路径**: `/home/ubuntu/libhv`
- **编译目录**: `/home/ubuntu/libhv/build`

## 编译状态
✅ 编译成功完成

## 生成的文件

### 库文件 (位于 `/home/ubuntu/libhv/build/lib/`)
- `libhv.so` - 动态链接库 (3.9M)
- `libhv_static.a` - 静态链接库 (9.8M)

### 示例程序 (位于 `/home/ubuntu/libhv/build/bin/`)
- `httpd` - HTTP 服务器
- `http_server_test` - HTTP 服务器测试
- `http_client_test` - HTTP 客户端测试
- `websocket_server_test` - WebSocket 服务器测试
- `websocket_client_test` - WebSocket 客户端测试
- `tcp_echo_server` - TCP Echo 服务器
- `tcp_client_test` - TCP 客户端测试
- `curl` - curl 工具
- `wget` - wget 工具
- 以及更多示例...

## 快速测试

### 1. 启动 HTTP 服务器
```bash
cd /home/ubuntu/libhv/build/bin
./http_server_test 8080
```

### 2. 测试 HTTP 服务器
```bash
curl http://localhost:8080/
```

### 3. 启动完整的 httpd 服务器
```bash
cd /home/ubuntu/libhv/build/bin
./httpd
```

## 使用 libhv 开发

### 编译你的程序
```bash
# 使用动态库
gcc your_program.c -I/home/ubuntu/libhv/build/include -L/home/ubuntu/libhv/build/lib -lhv -o your_program

# 使用静态库
gcc your_program.c -I/home/ubuntu/libhv/build/include /home/ubuntu/libhv/build/lib/libhv_static.a -lpthread -o your_program
```

### C++ 程序
```bash
g++ your_program.cpp -I/home/ubuntu/libhv/build/include -L/home/ubuntu/libhv/build/lib -lhv -lpthread -o your_program
```

## 示例代码位置
- C 示例: `/home/ubuntu/libhv/examples/`
- HTTP 服务器示例: `/home/ubuntu/libhv/examples/httpd/`
- WebSocket 示例: `/home/ubuntu/libhv/examples/websocket_*.cpp`
- MQTT 示例: `/home/ubuntu/libhv/examples/mqtt/`

## 文档资源
- 中文 README: `/home/ubuntu/libhv/README-CN.md`
- 编译说明: `/home/ubuntu/libhv/BUILD.md`
- 在线教程: https://hewei.blog.csdn.net/article/details/113733758

## 系统安装（需要 sudo 权限）
如果需要安装到系统目录，运行：
```bash
cd /home/ubuntu/libhv/build
sudo cmake --install .
```

这将把库文件安装到 `/usr/local/lib`，头文件安装到 `/usr/local/include/hv/`

## 测试结果
✅ HTTP 服务器测试成功
- 服务器版本: libhv/1.3.4
- 测试端口: 8080
- 响应正常

## 下一步
1. 查看示例代码学习使用方法
2. 阅读中文文档了解 API
3. 根据需求开发自己的网络应用

祝使用愉快！

