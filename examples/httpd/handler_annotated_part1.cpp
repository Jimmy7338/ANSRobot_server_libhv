/*
 * handler.cpp - 请求处理器实现（带详细中文注释版本 - 第1部分）
 * 
 * 【什么是 Handler？】
 * Handler 是处理 HTTP 请求的核心逻辑。
 * 每个 Handler 函数接收请求，处理业务逻辑，返回响应。
 * 
 * 【处理器类型】
 * 1. headerHandler：处理请求头
 * 2. preprocessor：预处理请求
 * 3. middleware：中间件（如认证）
 * 4. processor：业务处理
 * 5. postprocessor：后处理响应
 * 6. errorHandler：错误处理
 */

#include "handler.h"

#include <thread>   // C++ 标准线程库
#include <chrono>   // C++ 时间库

#include "hbase.h"      // libhv 基础工具
#include "htime.h"      // libhv 时间工具
#include "hfile.h"      // libhv 文件操作
#include "hstring.h"    // libhv 字符串工具
#include "EventLoop.h"  // libhv 事件循环（提供定时器等功能）

// ============================================================================
// 1. 处理链函数
// ============================================================================

/**
 * 请求头处理器
 * 
 * 【执行时机】最先执行，在解析请求体之前
 * 【作用】
 * - 验证请求头
 * - HTTP 到 HTTPS 重定向
 * - 早期拦截不合法的请求
 * 
 * @param req 请求对象
 * @param resp 响应对象
 * @return HTTP_STATUS_NEXT 继续处理，其他值表示中断处理链
 */
int Handler::headerHandler(HttpRequest* req, HttpResponse* resp) {
    // 可以打印客户端地址（调试用）
    // printf("%s:%d\n", req->client_addr.ip.c_str(), req->client_addr.port);
    
#if REDIRECT_HTTP_TO_HTTPS
    // 如果启用了 HTTP 到 HTTPS 重定向
    if (req->scheme == "http") {
        // 构造 HTTPS URL
        std::string location = hv::asprintf("https://%s:%d%s", 
            req->host.c_str(), 8443, req->path.c_str());
        // 返回 301 永久重定向
        return resp->Redirect(location, HTTP_STATUS_MOVED_PERMANENTLY);
    }
#endif

    // 统一验证请求的 Content-Type（可选）
    // if (req->content_type != APPLICATION_JSON) {
    //     return response_status(resp, HTTP_STATUS_BAD_REQUEST);
    // }
    
    // 返回 HTTP_STATUS_NEXT 表示继续处理
    return HTTP_STATUS_NEXT;
}

/**
 * 预处理器
 * 
 * 【执行时机】在 headerHandler 之后，业务处理之前
 * 【作用】
 * - 解析请求体（JSON、表单等）
 * - 统一设置响应格式
 * - 请求日志记录
 * 
 * @param req 请求对象
 * @param resp 响应对象
 * @return HTTP_STATUS_NEXT 继续处理
 */
int Handler::preprocessor(HttpRequest* req, HttpResponse* resp) {
    // 可以打印完整的请求信息（调试用）
    // printf("%s\n", req->Dump(true, true).c_str());

    // 解析请求体
    // 【重要】这会根据 Content-Type 自动解析：
    // - application/json -> req->json
    // - application/x-www-form-urlencoded -> req->kv
    // - multipart/form-data -> req->form
    req->ParseBody();

    // 统一设置响应的 Content-Type 为 JSON（可选）
    resp->content_type = APPLICATION_JSON;

    return HTTP_STATUS_NEXT;
}

/**
 * 后处理器
 * 
 * 【执行时机】在业务处理之后，发送响应之前
 * 【作用】
 * - 统一格式化响应
 * - 添加响应头
 * - 响应日志记录
 * 
 * @param req 请求对象
 * @param resp 响应对象
 * @return 响应状态码
 */
int Handler::postprocessor(HttpRequest* req, HttpResponse* resp) {
    // 可以打印完整的响应信息（调试用）
    // printf("%s\n", resp->Dump(true, true).c_str());
    
    return resp->status_code;
}

/**
 * 错误处理器
 * 
 * 【执行时机】当发生错误时
 * 【作用】统一处理错误响应
 * 
 * @param ctx 请求上下文
 * @return 错误状态码
 */
int Handler::errorHandler(const HttpContextPtr& ctx) {
    int error_code = ctx->response->status_code;
    return response_status(ctx, error_code);
}

// ============================================================================
// 2. 中间件
// ============================================================================

/**
 * 身份认证中间件
 * 
 * 【什么是中间件？】
 * 中间件是在业务处理前执行的通用逻辑，如：
 * - 身份认证：检查用户是否登录
 * - 权限验证：检查用户是否有权限
 * - 日志记录：记录请求信息
 * - 限流：防止恶意请求
 * 
 * 【认证流程】
 * 1. 客户端登录成功后，服务器返回 token
 * 2. 客户端后续请求在 Authorization 头中携带 token
 * 3. 服务器验证 token 是否有效
 * 
 * @param req 请求对象
 * @param resp 响应对象
 * @return HTTP_STATUS_NEXT 验证通过，HTTP_STATUS_UNAUTHORIZED 验证失败
 */
int Handler::Authorization(HttpRequest* req, HttpResponse* resp) {
    // 登录接口不需要认证
    if (strcmp(req->path.c_str(), "/login") == 0) {
        return HTTP_STATUS_NEXT;
    }
    
    // 从请求头获取 token
    std::string token = req->GetHeader("Authorization");
    
    if (token.empty()) {
        // 缺少 Authorization 头
        response_status(resp, 10011, "Miss Authorization header!");
        return HTTP_STATUS_UNAUTHORIZED;  // 返回 401 未授权
    }
    else if (strcmp(token.c_str(), "abcdefg") != 0) {
        // token 验证失败
        // 【注意】实际项目中应该：
        // 1. 使用 JWT（JSON Web Token）
        // 2. 验证 token 签名
        // 3. 检查 token 是否过期
        // 4. 从 token 中提取用户信息
        response_status(resp, 10012, "Authorization failed!");
        return HTTP_STATUS_UNAUTHORIZED;
    }
    
    // 验证通过，继续处理
    return HTTP_STATUS_NEXT;
}

// ============================================================================
// 3. 延迟响应示例
// ============================================================================

/**
 * 同步延迟（阻塞方式）
 * 
 * 【同步 vs 异步】
 * - 同步：函数执行期间会阻塞线程，直到完成
 * - 异步：函数立即返回，稍后通过回调完成
 * 
 * 【缺点】
 * 这种方式会占用工作线程，降低并发能力
 * 如果有 100 个并发请求，每个延迟 1 秒，需要 100 个线程
 * 
 * @param req 请求对象
 * @param writer 响应写入器
 * @return 200 成功
 */
int Handler::sleep(const HttpRequestPtr& req, const HttpResponseWriterPtr& writer) {
    // 写入响应头：当前线程 ID
    writer->WriteHeader("X-Response-tid", hv_gettid());
    
    // 记录开始时间
    unsigned long long start_ms = gettimeofday_ms();
    writer->response->Set("start_ms", start_ms);
    
    // 从 URL 参数获取延迟时间（默认 1000 毫秒）
    std::string strTime = req->GetParam("t", "1000");
    if (!strTime.empty()) {
        int ms = atoi(strTime.c_str());
        if (ms > 0) {
            // 阻塞延迟（占用线程）
            hv_delay(ms);
        }
    }
    
    // 记录结束时间
    unsigned long long end_ms = gettimeofday_ms();
    writer->response->Set("end_ms", end_ms);
    writer->response->Set("cost_ms", end_ms - start_ms);
    
    // 返回响应
    response_status(writer, 0, "OK");
    return 200;
}

/**
 * 异步延迟（非阻塞方式）
 * 
 * 【优点】
 * 使用定时器实现延迟，不占用线程
 * 100 个并发请求只需要少量线程
 * 
 * 【事件循环】
 * libhv 使用事件循环处理异步操作：
 * 1. 注册定时器
 * 2. 继续处理其他请求
 * 3. 定时器到期时，执行回调函数
 * 
 * @param ctx 请求上下文
 * @return HTTP_STATUS_UNFINISHED 表示异步处理，稍后发送响应
 */
int Handler::setTimeout(const HttpContextPtr& ctx) {
    // 记录开始时间
    unsigned long long start_ms = gettimeofday_ms();
    ctx->set("start_ms", start_ms);
    
    // 从 URL 参数获取延迟时间
    std::string strTime = ctx->param("t", "1000");
    if (!strTime.empty()) {
        int ms = atoi(strTime.c_str());
        if (ms > 0) {
            // 设置定时器（非阻塞）
            // Lambda 表达式：[捕获列表](参数列表){ 函数体 }
            hv::setTimeout(ms, [ctx, start_ms](hv::TimerID timerID){
                // 定时器到期后执行
                unsigned long long end_ms = gettimeofday_ms();
                ctx->set("end_ms", end_ms);
                ctx->set("cost_ms", end_ms - start_ms);
                response_status(ctx, 0, "OK");
            });
        }
    }
    
    // 返回 HTTP_STATUS_UNFINISHED 表示异步处理
    // 框架不会立即发送响应，等待回调函数调用 ctx->send()
    return HTTP_STATUS_UNFINISHED;
}

// ============================================================================
// 4. URL 参数解析
// ============================================================================

/**
 * 解析 URL 查询参数
 * 
 * 【URL 格式】
 * scheme:[//[user[:password]@]host[:port]][/path][?query][#fragment]
 * 例如：http://localhost:8080/query?page_no=1&page_size=10
 * 
 * 【参数解析】
 * ?query 部分会被解析为 key-value 对
 * page_no=1 -> {"page_no": "1"}
 * page_size=10 -> {"page_size": "10"}
 * 
 * @param ctx 请求上下文
 * @return 200 成功
 */
int Handler::query(const HttpContextPtr& ctx) {
    // 遍历所有 URL 参数
    for (auto& param : ctx->params()) {
        // 将参数添加到响应中
        ctx->set(param.first.c_str(), param.second);
    }
    response_status(ctx, 0, "OK");
    return 200;
}

// ============================================================================
// 5. 不同请求体格式处理
// ============================================================================

/**
 * 处理表单格式（application/x-www-form-urlencoded）
 * 
 * 【格式】key1=value1&key2=value2
 * 【使用场景】传统的 HTML 表单提交
 * 
 * @param req 请求对象
 * @param resp 响应对象
 * @return 200 成功，400 格式错误
 */
int Handler::kv(HttpRequest* req, HttpResponse* resp) {
    // 验证 Content-Type
    if (req->content_type != APPLICATION_URLENCODED) {
        return response_status(resp, HTTP_STATUS_BAD_REQUEST);
    }
    
    // 设置响应格式
    resp->content_type = APPLICATION_URLENCODED;
    
    // 获取请求中的键值对
    resp->kv = req->GetUrlEncoded();
    
    // 添加一些示例数据到响应
    resp->SetUrlEncoded("int", 123);
    resp->SetUrlEncoded("float", 3.14);
    resp->SetUrlEncoded("string", "hello");
    
    return 200;
}

/**
 * 处理 JSON 格式（application/json）
 * 
 * 【格式】{"key1": "value1", "key2": "value2"}
 * 【使用场景】现代 Web API 的标准格式
 * 【优点】
 * - 支持嵌套结构
 * - 支持数组
 * - 类型明确（字符串、数字、布尔值等）
 * 
 * @param req 请求对象
 * @param resp 响应对象
 * @return 200 成功，400 格式错误
 */
int Handler::json(HttpRequest* req, HttpResponse* resp) {
    // 验证 Content-Type
    if (req->content_type != APPLICATION_JSON) {
        return response_status(resp, HTTP_STATUS_BAD_REQUEST);
    }
    
    // 设置响应格式
    resp->content_type = APPLICATION_JSON;
    
    // 获取请求中的 JSON 对象
    resp->json = req->GetJson();
    
    // 添加一些示例数据到响应
    resp->json["int"] = 123;
    resp->json["float"] = 3.14;
    resp->json["string"] = "hello";
    
    return 200;
}

/**
 * 处理文件上传格式（multipart/form-data）
 * 
 * 【格式】多部分表单数据，可以同时传输文本和文件
 * 【使用场景】文件上传
 * 
 * @param req 请求对象
 * @param resp 响应对象
 * @return 200 成功，400 格式错误
 */
int Handler::form(HttpRequest* req, HttpResponse* resp) {
    // 验证 Content-Type
    if (req->content_type != MULTIPART_FORM_DATA) {
        return response_status(resp, HTTP_STATUS_BAD_REQUEST);
    }
    
    // 设置响应格式
    resp->content_type = MULTIPART_FORM_DATA;
    
    // 获取表单数据
    resp->form = req->GetForm();
    
    // 添加一些示例数据到响应
    resp->SetFormData("int", 123);
    resp->SetFormData("float", 3.14);
    resp->SetFormData("string", "hello");
    // 也可以添加文件
    // resp->SetFormFile("file", "test.jpg");
    
    return 200;
}

/**
 * 处理 gRPC 请求
 * 
 * 【什么是 gRPC？】
 * gRPC 是 Google 开发的高性能 RPC 框架
 * - 使用 Protocol Buffers 序列化数据
 * - 基于 HTTP/2
 * - 支持双向流
 * 
 * @param req 请求对象
 * @param resp 响应对象
 * @return 200 成功，400 格式错误
 */
int Handler::grpc(HttpRequest* req, HttpResponse* resp) {
    // 验证 Content-Type
    if (req->content_type != APPLICATION_GRPC) {
        return response_status(resp, HTTP_STATUS_BAD_REQUEST);
    }
    
    // 实际项目中需要：
    // 1. 解析 protobuf 数据
    // ParseFromString(req->body);
    
    // 2. 处理业务逻辑
    
    // 3. 序列化响应
    // resp->content_type = APPLICATION_GRPC;
    // resp->body = SerializeAsString(xxx);
    
    response_status(resp, 0, "OK");
    return 200;
}

// 文件太长，继续在 handler_annotated_part2.cpp 中...

