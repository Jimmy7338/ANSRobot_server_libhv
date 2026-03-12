/*
 * handler.cpp - 请求处理器实现（带详细中文注释版本 - 第2部分）
 * 
 * 这部分包含：
 * - 通用测试接口
 * - RESTful API 示例
 * - 用户登录
 * - 文件上传下载
 * - SSE 服务器推送
 */

// ============================================================================
// 6. 通用测试接口
// ============================================================================

/**
 * 通用测试接口
 * 
 * 【功能】自动识别请求格式并解析参数
 * 支持三种格式：
 * 1. application/x-www-form-urlencoded
 * 2. application/json
 * 3. multipart/form-data
 * 
 * @param ctx 请求上下文
 * @return 200 成功
 */
int Handler::test(const HttpContextPtr& ctx) {
    // 设置响应的 Content-Type 与请求相同
    ctx->setContentType(ctx->type());
    
    // 自动解析并返回各种类型的参数
    ctx->set("bool", ctx->get<bool>("bool"));      // 布尔值
    ctx->set("int", ctx->get<int>("int"));         // 整数
    ctx->set("float", ctx->get<float>("float"));   // 浮点数
    ctx->set("string", ctx->get("string"));        // 字符串
    
    response_status(ctx, 0, "OK");
    return 200;
}

// ============================================================================
// 7. RESTful API 示例
// ============================================================================

/**
 * RESTful API 处理器
 * 
 * 【什么是 RESTful？】
 * REST（Representational State Transfer）是一种 API 设计风格
 * 
 * 【路径参数】
 * 路径中的 :参数名 会被提取为参数
 * 例如：/group/:group_name/user/:user_id
 * 访问：/group/developers/user/123
 * 提取：group_name=developers, user_id=123
 * 
 * 【RESTful 设计原则】
 * 1. 使用名词表示资源（user, group）
 * 2. 使用 HTTP 方法表示操作（GET, POST, PUT, DELETE）
 * 3. 使用路径参数标识具体资源
 * 
 * @param ctx 请求上下文
 * @return 200 成功
 */
int Handler::restful(const HttpContextPtr& ctx) {
    // 从路径中提取参数
    std::string group_name = ctx->param("group_name");
    std::string user_id = ctx->param("user_id");
    
    // 将参数添加到响应
    ctx->set("group_name", group_name);
    ctx->set("user_id", user_id);
    
    // 实际项目中应该：
    // 1. 验证参数有效性
    // 2. 从数据库查询/删除数据
    // 3. 返回操作结果
    
    response_status(ctx, 0, "OK");
    return 200;
}

// ============================================================================
// 8. 用户登录
// ============================================================================

/**
 * 用户登录接口
 * 
 * 【登录流程】
 * 1. 客户端发送用户名和密码
 * 2. 服务器验证用户名和密码
 * 3. 验证成功，返回 token
 * 4. 客户端保存 token，后续请求携带 token
 * 
 * 【安全建议】
 * 1. 使用 HTTPS 传输密码
 * 2. 密码应该加密存储（bcrypt, scrypt）
 * 3. 使用 JWT 生成 token
 * 4. 设置 token 过期时间
 * 5. 实现刷新 token 机制
 * 
 * @param ctx 请求上下文
 * @return 200 成功，400 失败
 */
int Handler::login(const HttpContextPtr& ctx) {
    // 从请求中获取用户名和密码
    std::string username = ctx->get("username");
    std::string password = ctx->get("password");
    
    // 验证参数
    if (username.empty() || password.empty()) {
        response_status(ctx, 10001, "Miss username or password");
        return HTTP_STATUS_BAD_REQUEST;
    }
    else if (strcmp(username.c_str(), "admin") != 0) {
        // 用户名不存在
        // 【注意】实际项目中应该查询数据库
        response_status(ctx, 10002, "Username not exist");
        return HTTP_STATUS_BAD_REQUEST;
    }
    else if (strcmp(password.c_str(), "123456") != 0) {
        // 密码错误
        // 【注意】实际项目中应该：
        // 1. 从数据库获取加密后的密码
        // 2. 使用相同算法加密输入的密码
        // 3. 比较两个加密后的密码
        response_status(ctx, 10003, "Password wrong");
        return HTTP_STATUS_BAD_REQUEST;
    }
    else {
        // 登录成功，返回 token
        // 【注意】实际项目中应该：
        // 1. 生成 JWT token
        // 2. 在 token 中包含用户 ID、过期时间等信息
        // 3. 使用密钥签名 token
        ctx->set("token", "abcdefg");
        response_status(ctx, 0, "OK");
        return HTTP_STATUS_OK;
    }
}

// ============================================================================
// 9. 文件上传
// ============================================================================

/**
 * 普通文件上传
 * 
 * 【两种上传方式】
 * 1. multipart/form-data：表单上传，适合小文件
 * 2. 直接上传文件内容：适合大文件
 * 
 * @param ctx 请求上下文
 * @return 200 成功，其他值表示失败
 */
int Handler::upload(const HttpContextPtr& ctx) {
    int status_code = 200;
    std::string save_path = "html/uploads/";  // 保存目录
    
    if (ctx->is(MULTIPART_FORM_DATA)) {
        // 方式1：multipart/form-data 格式
        // 从表单字段 "file" 中保存文件
        status_code = ctx->request->SaveFormFile("file", save_path.c_str());
    } else {
        // 方式2：直接上传文件内容
        // 从 URL 参数获取文件名
        std::string filename = ctx->param("filename", "unnamed.txt");
        std::string filepath = save_path + filename;
        status_code = ctx->request->SaveFile(filepath.c_str());
    }
    
    return response_status(ctx, status_code);
}

/**
 * 大文件上传（流式处理）
 * 
 * 【为什么需要流式处理？】
 * 普通上传会将整个文件读入内存，大文件会导致内存不足
 * 流式处理边接收边写入磁盘，内存占用小
 * 
 * 【HTTP 解析状态】
 * - HP_HEADERS_COMPLETE：请求头解析完成
 * - HP_BODY：接收到部分请求体
 * - HP_MESSAGE_COMPLETE：请求完成
 * - HP_ERROR：发生错误
 * 
 * @param ctx 请求上下文
 * @param state HTTP 解析状态
 * @param data 数据指针
 * @param size 数据大小
 * @return HTTP_STATUS_UNFINISHED 继续接收，其他值表示完成或错误
 */
int Handler::recvLargeFile(const HttpContextPtr& ctx, http_parser_state state, const char* data, size_t size) {
    // printf("recvLargeFile state=%d\n", (int)state);
    int status_code = HTTP_STATUS_UNFINISHED;
    
    // 使用 userdata 存储文件对象
    HFile* file = (HFile*)ctx->userdata;
    
    switch (state) {
    case HP_HEADERS_COMPLETE:
        {
            // 请求头解析完成，准备接收文件
            if (ctx->is(MULTIPART_FORM_DATA)) {
                // 不支持 multipart/form-data（需要使用 multipart_parser）
                ctx->close();
                return HTTP_STATUS_BAD_REQUEST;
            }
            
            // 构造文件路径
            std::string save_path = "html/uploads/";
            std::string filename = ctx->param("filename", "unnamed.txt");
            std::string filepath = save_path + filename;
            
            // 打开文件准备写入
            file = new HFile;
            if (file->open(filepath.c_str(), "wb") != 0) {
                ctx->close();
                return HTTP_STATUS_INTERNAL_SERVER_ERROR;
            }
            ctx->userdata = file;
        }
        break;
        
    case HP_BODY:
        {
            // 接收到部分数据，写入文件
            if (file && data && size) {
                if (file->write(data, size) != size) {
                    // 写入失败
                    ctx->close();
                    return HTTP_STATUS_INTERNAL_SERVER_ERROR;
                }
            }
        }
        break;
        
    case HP_MESSAGE_COMPLETE:
        {
            // 接收完成
            status_code = HTTP_STATUS_OK;
            ctx->setContentType(APPLICATION_JSON);
            response_status(ctx, status_code);
            
            // 关闭文件
            if (file) {
                delete file;
                ctx->userdata = NULL;
            }
        }
        break;
        
    case HP_ERROR:
        {
            // 发生错误，删除文件
            if (file) {
                file->remove();
                delete file;
                ctx->userdata = NULL;
            }
        }
        break;
        
    default:
        break;
    }
    
    return status_code;
}

// ============================================================================
// 10. 大文件下载
// ============================================================================

/**
 * 大文件下载（流式传输）
 * 
 * 【为什么需要流式传输？】
 * 普通下载会将整个文件读入内存，大文件会导致内存不足
 * 流式传输分块读取和发送，内存占用小
 * 
 * 【限速下载】
 * 可以限制下载速度，防止占用过多带宽
 * 
 * 【实现方式】
 * 使用独立线程处理下载，不阻塞主线程
 * 
 * @param ctx 请求上下文
 * @return HTTP_STATUS_UNFINISHED 异步处理
 */
int Handler::sendLargeFile(const HttpContextPtr& ctx) {
    // 创建独立线程处理下载
    // .detach() 表示线程独立运行，不需要 join
    std::thread([ctx](){
        ctx->writer->Begin();
        
        // 构造文件路径
        std::string filepath = ctx->service->document_root + ctx->request->Path();
        HFile file;
        
        // 打开文件
        if (file.open(filepath.c_str(), "rb") != 0) {
            // 文件不存在，返回 404
            ctx->writer->WriteStatus(HTTP_STATUS_NOT_FOUND);
            ctx->writer->WriteHeader("Content-Type", "text/html");
            ctx->writer->WriteBody("<center><h1>404 Not Found</h1></center>");
            ctx->writer->End();
            return;
        }
        
        // 根据文件扩展名确定 Content-Type
        http_content_type content_type = CONTENT_TYPE_NONE;
        const char* suffix = hv_suffixname(filepath.c_str());
        if (suffix) {
            content_type = http_content_type_enum_by_suffix(suffix);
        }
        if (content_type == CONTENT_TYPE_NONE || content_type == CONTENT_TYPE_UNDEFINED) {
            content_type = APPLICATION_OCTET_STREAM;  // 默认为二进制流
        }
        
        // 写入响应头
        size_t filesize = file.size();
        ctx->writer->WriteHeader("Content-Type", http_content_type_str(content_type));
        
#if USE_TRANSFER_ENCODING_CHUNKED
        // 使用分块传输编码
        ctx->writer->WriteHeader("Transfer-Encoding", "chunked");
#else
        // 使用 Content-Length
        ctx->writer->WriteHeader("Content-Length", filesize);
#endif
        ctx->writer->EndHeaders();

        // 分块读取和发送文件
        char* buf = NULL;
        int len = 40960; // 每次读取 40KB
        SAFE_ALLOC(buf, len);
        
        size_t total_readbytes = 0;  // 已读取字节数
        int last_progress = 0;        // 上次进度
        
        // 计算限速参数
        int sleep_ms_per_send = 0;
        if (ctx->service->limit_rate <= 0) {
            // 不限速
        } else {
            // 根据限速计算每次发送后的延迟时间
            // limit_rate 单位是 KB/s
            sleep_ms_per_send = len * 1000 / 1024 / ctx->service->limit_rate;
        }
        if (sleep_ms_per_send == 0) sleep_ms_per_send = 1;
        int sleep_ms = sleep_ms_per_send;
        
        // 记录开始时间
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time;
        
        // 循环读取和发送
        while (total_readbytes < filesize) {
            // 检查连接是否断开
            if (!ctx->writer->isConnected()) {
                break;
            }
            
            // 检查上次写入是否完成
            if (!ctx->writer->isWriteComplete()) {
                hv_delay(1);
                continue;
            }
            
            // 读取文件
            size_t readbytes = file.read(buf, len);
            if (readbytes <= 0) {
                // 读取错误
                ctx->writer->close();
                break;
            }
            
            // 发送数据
            int nwrite = ctx->writer->WriteBody(buf, readbytes);
            if (nwrite < 0) {
                // 连接断开
                break;
            }
            
            total_readbytes += readbytes;
            
            // 计算进度
            int cur_progress = total_readbytes * 100 / filesize;
            if (cur_progress > last_progress) {
                // 可以打印进度（调试用）
                // printf("<< %s progress: %ld/%ld = %d%%\n",
                //     ctx->request->path.c_str(), (long)total_readbytes, (long)filesize, (int)cur_progress);
                last_progress = cur_progress;
            }
            
            // 限速延迟
            end_time += std::chrono::milliseconds(sleep_ms);
            std::this_thread::sleep_until(end_time);
        }
        
        ctx->writer->End();
        SAFE_FREE(buf);
        
        // 可以打印总耗时（调试用）
        // auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        // printf("<< %s taked %ds\n", ctx->request->path.c_str(), (int)elapsed_time.count());
    }).detach();
    
    return HTTP_STATUS_UNFINISHED;
}

// ============================================================================
// 11. SSE（Server-Sent Events）服务器推送
// ============================================================================

/**
 * SSE 服务器推送
 * 
 * 【什么是 SSE？】
 * SSE（Server-Sent Events）是一种服务器向客户端推送数据的技术
 * 
 * 【SSE vs WebSocket】
 * - SSE：单向通信（服务器 -> 客户端），基于 HTTP
 * - WebSocket：双向通信，需要升级协议
 * 
 * 【使用场景】
 * - 实时通知（新消息、新订单）
 * - 进度更新（文件上传进度）
 * - 实时数据（股票行情、传感器数据）
 * 
 * 【客户端代码】
 * ```javascript
 * const eventSource = new EventSource('/sse');
 * eventSource.onmessage = function(event) {
 *     console.log('收到消息:', event.data);
 * };
 * ```
 * 
 * @param ctx 请求上下文
 * @return HTTP_STATUS_UNFINISHED 异步处理
 */
int Handler::sse(const HttpContextPtr& ctx) {
    // 设置定时器，每秒推送一次当前时间
    hv::setInterval(1000, [ctx](hv::TimerID timerID) {
        if (ctx->writer->isConnected()) {
            // 连接正常，推送数据
            char szTime[DATETIME_FMT_BUFLEN] = {0};
            datetime_t now = datetime_now();
            datetime_fmt(&now, szTime);
            
            // 发送 SSE 事件
            ctx->writer->SSEvent(szTime);
        } else {
            // 连接断开，停止定时器
            hv::killTimer(timerID);
        }
    });
    
    return HTTP_STATUS_UNFINISHED;
}

// ============================================================================
// 总结
// ============================================================================

/*
 * 【后端开发核心概念总结】
 * 
 * 1. HTTP 协议
 *    - 请求方法：GET, POST, PUT, DELETE
 *    - 状态码：200 成功, 400 客户端错误, 500 服务器错误
 *    - 请求头/响应头：Content-Type, Authorization 等
 * 
 * 2. 请求体格式
 *    - application/x-www-form-urlencoded：表单
 *    - application/json：JSON
 *    - multipart/form-data：文件上传
 * 
 * 3. 同步 vs 异步
 *    - 同步：阻塞线程，简单但并发能力差
 *    - 异步：非阻塞，复杂但并发能力强
 * 
 * 4. RESTful API
 *    - 使用 HTTP 方法表示操作
 *    - 使用 URL 表示资源
 *    - 无状态设计
 * 
 * 5. 认证授权
 *    - 登录获取 token
 *    - 后续请求携带 token
 *    - 服务器验证 token
 * 
 * 6. 文件处理
 *    - 小文件：一次性读写
 *    - 大文件：流式处理
 * 
 * 7. 实时通信
 *    - SSE：服务器推送
 *    - WebSocket：双向通信
 * 
 * 【学习建议】
 * 1. 先理解 HTTP 协议基础
 * 2. 学习常见的请求响应格式
 * 3. 掌握同步和异步的区别
 * 4. 实践 RESTful API 设计
 * 5. 了解认证授权机制
 * 6. 学习数据库操作（MySQL, Redis）
 * 7. 掌握常见的设计模式
 */

