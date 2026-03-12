/*
 * router.cpp - 路由注册模块（带详细中文注释版本）
 * 
 * 【什么是路由？】
 * 路由就是 URL 路径到处理函数的映射关系。
 * 例如：当用户访问 http://localhost:8080/login 时，
 * 服务器会调用对应的登录处理函数。
 * 
 * 【HTTP 方法】
 * - GET：获取资源（查询数据）
 * - POST：创建资源（提交数据）
 * - PUT：更新资源（完整更新）
 * - PATCH：更新资源（部分更新）
 * - DELETE：删除资源
 * 
 * 【RESTful API】
 * 一种 API 设计风格，使用 HTTP 方法和 URL 表达操作：
 * GET    /users      - 获取用户列表
 * GET    /users/123  - 获取 ID 为 123 的用户
 * POST   /users      - 创建新用户
 * PUT    /users/123  - 更新用户 123
 * DELETE /users/123  - 删除用户 123
 */

#include "router.h"

#include "handler.h"
#include "hthread.h"    // 线程相关函数（如获取线程 ID）
#include "hasync.h"     // 异步操作支持
#include "requests.h"   // HTTP 客户端请求库

/**
 * 注册所有路由
 * 
 * @param router HTTP 服务对象的引用
 * 
 * 【处理链】
 * 每个请求会按以下顺序经过多个处理器：
 * headerHandler -> preprocessor -> middleware -> processor -> postprocessor
 * 
 * - headerHandler：处理请求头（如重定向 HTTP 到 HTTPS）
 * - preprocessor：预处理（如解析请求体）
 * - middleware：中间件（如身份认证）
 * - processor：实际的业务处理函数
 * - postprocessor：后处理（如统一格式化响应）
 */
void Router::Register(hv::HttpService& router) {
    
    // ========================================================================
    // 1. 注册处理链
    // ========================================================================
    
    // 请求头处理器：最先执行，可以做重定向、验证等
    router.headerHandler = Handler::headerHandler;
    
    // 预处理器：解析请求体（JSON、表单等）
    router.preprocessor = Handler::preprocessor;
    
    // 后处理器：在响应发送前执行，可以统一处理响应格式
    router.postprocessor = Handler::postprocessor;
    
    // 错误处理器：处理错误响应（可选）
    // router.errorHandler = Handler::errorHandler;
    
    // 大文件处理器：处理大文件下载（可选）
    // router.largeFileHandler = Handler::sendLargeFile;

    // ========================================================================
    // 2. 注册中间件（可选）
    // ========================================================================
    
    // 中间件：在所有路由处理前执行
    // 例如：身份认证中间件，检查用户是否登录
    // router.Use(Handler::Authorization);

    // ========================================================================
    // 3. 基础路由示例
    // ========================================================================
    
    // ------------------------------------------------------------------------
    // 3.1 简单的字符串响应
    // ------------------------------------------------------------------------
    // 路由：GET /ping
    // 功能：健康检查接口，返回 "pong"
    // 测试：curl -v http://localhost:8080/ping
    router.GET("/ping", [](HttpRequest* req, HttpResponse* resp) {
        return resp->String("pong");
    });

    // ------------------------------------------------------------------------
    // 3.2 返回原始数据
    // ------------------------------------------------------------------------
    // 路由：GET /data
    // 功能：返回二进制数据
    // 测试：curl -v http://localhost:8080/data
    router.GET("/data", [](HttpRequest* req, HttpResponse* resp) {
        static char data[] = "0123456789";
        // resp->Data(数据指针, 数据长度, 是否复制数据)
        return resp->Data(data, 10 /*, false */);
    });

    // ------------------------------------------------------------------------
    // 3.3 返回静态文件
    // ------------------------------------------------------------------------
    // 路由：GET /html/index.html
    // 功能：返回 HTML 文件
    // 测试：curl -v http://localhost:8080/html/index.html
    router.GET("/html/index.html", [](HttpRequest* req, HttpResponse* resp) {
        return resp->File("html/index.html");
    });

    // ------------------------------------------------------------------------
    // 3.4 返回所有已注册的路由
    // ------------------------------------------------------------------------
    // 路由：GET /paths
    // 功能：查看服务器支持的所有路由
    // 测试：curl -v http://localhost:8080/paths
    router.GET("/paths", [&router](HttpRequest* req, HttpResponse* resp) {
        return resp->Json(router.Paths());
    });

    // ========================================================================
    // 4. 使用 HttpContext 的路由（推荐方式）
    // ========================================================================
    
    // 【什么是 HttpContext？】
    // HttpContext 是一个上下文对象，封装了请求和响应，提供更方便的 API
    
    // ------------------------------------------------------------------------
    // 4.1 查看服务配置
    // ------------------------------------------------------------------------
    // 路由：GET /service
    // 功能：返回服务器配置信息
    // 测试：curl -v http://localhost:8080/service
    router.GET("/service", [](const HttpContextPtr& ctx) {
        ctx->setContentType("application/json");
        ctx->set("base_url", ctx->service->base_url);
        ctx->set("document_root", ctx->service->document_root);
        ctx->set("home_page", ctx->service->home_page);
        ctx->set("error_page", ctx->service->error_page);
        ctx->set("index_of", ctx->service->index_of);
        return 200;  // HTTP 状态码 200 表示成功
    });

    // ------------------------------------------------------------------------
    // 4.2 获取请求信息
    // ------------------------------------------------------------------------
    // 路由：GET /get?env=1
    // 功能：返回请求的详细信息（IP、URL、参数、请求头等）
    // 测试：curl -v "http://localhost:8080/get?name=admin&age=18"
    router.GET("/get", [](const HttpContextPtr& ctx) {
        hv::Json resp;
        resp["origin"] = ctx->ip();          // 客户端 IP 地址
        resp["url"] = ctx->url();            // 完整 URL
        resp["args"] = ctx->params();        // URL 参数（?后面的部分）
        resp["headers"] = ctx->headers();    // 所有请求头
        return ctx->send(resp.dump(2));      // 格式化 JSON 并发送
    });

    // ------------------------------------------------------------------------
    // 4.3 回显请求体
    // ------------------------------------------------------------------------
    // 路由：POST /echo
    // 功能：原样返回客户端发送的数据
    // 测试：curl -v http://localhost:8080/echo -d "hello,world!"
    router.POST("/echo", [](const HttpContextPtr& ctx) {
        return ctx->send(ctx->body(), ctx->type());
    });

    // ========================================================================
    // 5. 通配符路由
    // ========================================================================
    
    // 路由：GET /wildcard*
    // 功能：匹配所有以 /wildcard 开头的路径
    // 测试：curl -v http://localhost:8080/wildcard/any/path
    router.GET("/wildcard*", [](HttpRequest* req, HttpResponse* resp) {
        std::string str = req->path + " match /wildcard*";
        return resp->String(str);
    });

    // ========================================================================
    // 6. 异步响应
    // ========================================================================
    
    // 【什么是异步响应？】
    // 同步：处理函数必须立即返回响应
    // 异步：处理函数可以稍后再发送响应，适合耗时操作
    
    // ------------------------------------------------------------------------
    // 6.1 手动控制响应
    // ------------------------------------------------------------------------
    // 路由：GET /async
    // 功能：演示异步响应的写法
    // 测试：curl -v http://localhost:8080/async
    router.GET("/async", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer) {
        writer->Begin();  // 开始写响应
        writer->WriteHeader("X-Response-tid", hv_gettid());  // 写入自定义响应头
        writer->WriteHeader("Content-Type", "text/plain");   // 设置内容类型
        writer->WriteBody("This is an async response.\n");   // 写入响应体
        writer->End();    // 结束响应
    });

    // ------------------------------------------------------------------------
    // 6.2 异步代理请求
    // ------------------------------------------------------------------------
    // 路由：GET /www.*
    // 功能：动态代理，访问 /www.example.com 会请求 http://example.com
    // 测试：curl -v http://localhost:8080/www.baidu.com
    router.GET("/www.*", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer) {
        // 创建一个新的 HTTP 请求
        auto req2 = std::make_shared<HttpRequest>();
        req2->url = req->path.substr(1);  // 去掉开头的 /
        
        // 异步发送请求
        requests::async(req2, [writer](const HttpResponsePtr& resp2){
            writer->Begin();
            if (resp2 == NULL) {
                // 请求失败，返回 404
                writer->WriteStatus(HTTP_STATUS_NOT_FOUND);
                writer->WriteHeader("Content-Type", "text/html");
                writer->WriteBody("<center><h1>404 Not Found</h1></center>");
            } else {
                // 请求成功，转发响应
                writer->WriteResponse(resp2.get());
            }
            writer->End();
        });
    });

    // ========================================================================
    // 7. 延迟响应（演示同步和异步的区别）
    // ========================================================================
    
    // ------------------------------------------------------------------------
    // 7.1 同步延迟（阻塞）
    // ------------------------------------------------------------------------
    // 路由：GET /sleep?t=1000
    // 功能：阻塞指定毫秒后返回（会占用线程）
    // 测试：curl -v "http://localhost:8080/sleep?t=2000"
    router.GET("/sleep", Handler::sleep);

    // ------------------------------------------------------------------------
    // 7.2 异步延迟（非阻塞）
    // ------------------------------------------------------------------------
    // 路由：GET /setTimeout?t=1000
    // 功能：使用定时器延迟响应（不占用线程）
    // 测试：curl -v "http://localhost:8080/setTimeout?t=2000"
    router.GET("/setTimeout", Handler::setTimeout);

    // ========================================================================
    // 8. URL 参数解析
    // ========================================================================
    
    // 路由：GET /query?page_no=1&page_size=10
    // 功能：解析 URL 查询参数
    // 测试：curl -v "http://localhost:8080/query?page_no=1&page_size=10"
    router.GET("/query", Handler::query);

    // ========================================================================
    // 9. 不同的请求体格式
    // ========================================================================
    
    // 【HTTP 请求体格式】
    // 1. application/x-www-form-urlencoded：表单格式（key1=value1&key2=value2）
    // 2. application/json：JSON 格式
    // 3. multipart/form-data：文件上传格式
    
    // ------------------------------------------------------------------------
    // 9.1 表单格式（application/x-www-form-urlencoded）
    // ------------------------------------------------------------------------
    // 路由：POST /kv
    // 功能：处理表单数据
    // 测试：curl -v http://localhost:8080/kv -H "content-type:application/x-www-form-urlencoded" -d 'user=admin&pswd=123456'
    router.POST("/kv", Handler::kv);

    // ------------------------------------------------------------------------
    // 9.2 JSON 格式（application/json）
    // ------------------------------------------------------------------------
    // 路由：POST /json
    // 功能：处理 JSON 数据
    // 测试：curl -v http://localhost:8080/json -H "Content-Type:application/json" -d '{"user":"admin","pswd":"123456"}'
    router.POST("/json", Handler::json);

    // ------------------------------------------------------------------------
    // 9.3 文件上传格式（multipart/form-data）
    // ------------------------------------------------------------------------
    // 路由：POST /form
    // 功能：处理文件上传
    // 测试：./bin/curl -v http://localhost:8080/form -F 'user=admin' -F 'pswd=123456'
    router.POST("/form", Handler::form);

    // ------------------------------------------------------------------------
    // 9.4 通用测试接口（支持多种格式）
    // ------------------------------------------------------------------------
    // 路由：POST /test
    // 功能：自动识别请求体格式并解析
    // 测试1：curl -v http://localhost:8080/test -H "Content-Type:application/x-www-form-urlencoded" -d 'bool=1&int=123&float=3.14&string=hello'
    // 测试2：curl -v http://localhost:8080/test -H "Content-Type:application/json" -d '{"bool":true,"int":123,"float":3.14,"string":"hello"}'
    // 测试3：./bin/curl -v http://localhost:8080/test -F 'bool=1' -F 'int=123' -F 'float=3.14' -F 'string=hello'
    router.POST("/test", Handler::test);

    // ========================================================================
    // 10. gRPC 支持
    // ========================================================================
    
    // 路由：POST /grpc
    // 功能：处理 gRPC 请求（需要 HTTP/2）
    // 测试：./bin/curl -v --http2 http://localhost:8080/grpc -H "content-type:application/grpc" -d 'protobuf'
    router.POST("/grpc", Handler::grpc);

    // ========================================================================
    // 11. RESTful API 示例
    // ========================================================================
    
    // 【RESTful 路径参数】
    // 使用 :参数名 或 {参数名} 定义路径参数
    // 例如：/group/:group_name/user/:user_id
    // 访问：/group/test/user/123
    // 可以获取：group_name=test, user_id=123
    
    // 路由：DELETE /group/:group_name/user/:user_id
    // 功能：删除指定组的指定用户
    // 测试：curl -v -X DELETE http://localhost:8080/group/test/user/123
    router.Delete("/group/:group_name/user/:user_id", Handler::restful);
    // 也可以使用花括号语法：
    // router.Delete("/group/{group_name}/user/{user_id}", Handler::restful);

    // ========================================================================
    // 12. 用户认证
    // ========================================================================
    
    // 路由：POST /login
    // 功能：用户登录
    // 测试：curl -v http://localhost:8080/login -H "Content-Type:application/json" -d '{"username":"admin","password":"123456"}'
    router.POST("/login", Handler::login);

    // ========================================================================
    // 13. 文件上传
    // ========================================================================
    
    // ------------------------------------------------------------------------
    // 13.1 普通文件上传
    // ------------------------------------------------------------------------
    // 路由：POST /upload
    // 功能：上传文件
    // 测试1：curl -v http://localhost:8080/upload?filename=test.txt -d '@LICENSE'
    // 测试2：curl -v http://localhost:8080/upload -F 'file=@LICENSE'
    router.POST("/upload", Handler::upload);
    
    // ------------------------------------------------------------------------
    // 13.2 大文件上传（流式处理）
    // ------------------------------------------------------------------------
    // 路由：POST /upload/{filename}
    // 功能：上传大文件，边接收边写入磁盘
    // 测试：curl -v http://localhost:8080/upload/README.md -d '@README.md'
    router.POST("/upload/{filename}", Handler::recvLargeFile);

    // ========================================================================
    // 14. SSE（Server-Sent Events）服务器推送
    // ========================================================================
    
    // 【什么是 SSE？】
    // SSE 是一种服务器向客户端推送数据的技术
    // 与 WebSocket 不同，SSE 是单向的（服务器 -> 客户端）
    // 适用场景：实时通知、进度更新、股票行情等
    
    // 路由：GET /sse
    // 功能：每秒推送一次当前时间
    // 测试：curl -v http://localhost:8080/sse
    // 或在浏览器中打开 html/EventSource.html
    router.GET("/sse", Handler::sse);
}

