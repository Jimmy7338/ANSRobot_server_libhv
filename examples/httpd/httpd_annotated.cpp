/*
 * httpd.cpp - HTTP/HTTPS 服务器主程序（带详细中文注释版本）
 * 
 * 【项目概述】
 * 这是一个完整的生产级 HTTP 服务器实现，展示了 libhv 的核心功能：
 * - HTTP/HTTPS 双协议支持
 * - 多进程/多线程并发处理
 * - 配置文件解析
 * - 路由管理
 * - 静态文件服务
 * - 代理功能（正向代理和反向代理）
 * 
 * 【后端基础知识】
 * 1. HTTP 服务器：接收客户端（浏览器/APP）的请求，返回响应
 * 2. 端口：服务器监听的网络地址，如 8080（HTTP）、8443（HTTPS）
 * 3. 路由：URL 路径到处理函数的映射，如 /login -> 登录处理函数
 * 4. 中间件：在请求处理前后执行的通用逻辑（如认证、日志）
 */

#include "hv.h"           // libhv 核心头文件
#include "hssl.h"         // SSL/TLS 支持（HTTPS 加密通信）
#include "hmain.h"        // 主程序框架（提供命令行参数解析、信号处理等）
#include "iniparser.h"    // INI 配置文件解析器

#include "HttpServer.h"   // HTTP 服务器类
#include "hasync.h"       // 异步操作支持

#include "router.h"       // 路由注册模块

// ============================================================================
// 全局变量定义
// ============================================================================

// 全局 HTTP 服务器对象 - 负责监听端口、管理连接、处理请求
// 这是整个服务器的核心对象
hv::HttpServer  g_http_server;

// 全局 HTTP 服务配置对象 - 存储服务配置（路由、静态文件路径、代理规则等）
hv::HttpService g_http_service;

// ============================================================================
// 函数声明
// ============================================================================

static void print_version();  // 打印版本信息
static void print_help();     // 打印帮助信息
static int  parse_confile(const char* confile);  // 解析配置文件

// ============================================================================
// 命令行参数定义
// ============================================================================

// 定义支持的命令行参数（长选项格式）
// 例如：./httpd -h 或 ./httpd --help
static const option_t long_options[] = {
    {'h', "help",       NO_ARGUMENT,        "Print this information"},           // 显示帮助
    {'v', "version",    NO_ARGUMENT,        "Print version"},                    // 显示版本
    {'c', "confile",    REQUIRED_ARGUMENT,  "Set configure file, default etc/{program}.conf"},  // 指定配置文件
    {'t', "test",       NO_ARGUMENT,        "Test configure file and exit"},     // 测试配置文件
    {'s', "signal",     REQUIRED_ARGUMENT,  "send signal to process, signal=[start,stop,restart,status,reload]"},  // 发送信号
    {'d', "daemon",     NO_ARGUMENT,        "Daemonize"},                        // 后台运行
    {'p', "port",       REQUIRED_ARGUMENT,  "Set listen port"}                   // 指定监听端口
};

// ============================================================================
// 工具函数实现
// ============================================================================

/**
 * 打印版本信息
 * 用法：./httpd -v
 */
void print_version() {
    printf("%s version %s\n", g_main_ctx.program_name, hv_compile_version());
}

/**
 * 打印帮助信息
 * 用法：./httpd -h
 * 显示所有可用的命令行参数
 */
void print_help() {
    char detail_options[1024] = {0};
    dump_opt_long(long_options, ARRAY_SIZE(long_options), detail_options, sizeof(detail_options));
    printf("%s\n", detail_options);
}

/**
 * 解析配置文件
 * 
 * @param confile 配置文件路径（通常是 etc/httpd.conf）
 * @return 0 表示成功，非 0 表示失败
 * 
 * 【配置文件说明】
 * INI 格式的配置文件，包含以下配置项：
 * - 日志配置：日志文件路径、日志级别、日志大小等
 * - 进程配置：工作进程数、工作线程数、最大连接数
 * - 端口配置：HTTP 端口、HTTPS 端口
 * - 服务配置：静态文件根目录、首页、错误页面等
 * - SSL 配置：证书文件、私钥文件
 * - 代理配置：正向代理、反向代理规则
 */
int parse_confile(const char* confile) {
    // 创建 INI 解析器对象
    IniParser ini;
    
    // 从文件加载配置
    int ret = ini.LoadFromFile(confile);
    if (ret != 0) {
        printf("Load confile [%s] failed: %d\n", confile, ret);
        exit(-40);
    }

    // ------------------------------------------------------------------------
    // 1. 日志配置
    // ------------------------------------------------------------------------
    
    // 读取日志文件路径配置
    std::string str = ini.GetValue("logfile");
    if (!str.empty()) {
        strncpy(g_main_ctx.logfile, str.c_str(), sizeof(g_main_ctx.logfile));
    }
    hlog_set_file(g_main_ctx.logfile);  // 设置日志文件
    
    // 读取日志级别（VERBOSE, DEBUG, INFO, WARN, ERROR, FATAL, SILENT）
    str = ini.GetValue("loglevel");
    if (!str.empty()) {
        hlog_set_level_by_str(str.c_str());
    }
    
    // 读取单个日志文件最大大小（如 64M）
    str = ini.GetValue("log_filesize");
    if (!str.empty()) {
        hlog_set_max_filesize_by_str(str.c_str());
    }
    
    // 读取日志保留天数
    str = ini.GetValue("log_remain_days");
    if (!str.empty()) {
        hlog_set_remain_days(atoi(str.c_str()));
    }
    
    // 读取是否启用日志同步写入（fsync）
    str = ini.GetValue("log_fsync");
    if (!str.empty()) {
        logger_enable_fsync(hlog, hv_getboolean(str.c_str()));
    }
    
    // 记录启动日志
    hlogi("%s version: %s", g_main_ctx.program_name, hv_compile_version());
    hlog_fsync();

    // ------------------------------------------------------------------------
    // 2. 多进程配置
    // ------------------------------------------------------------------------
    
    // worker_processes：工作进程数
    // 【概念】多进程模式可以充分利用多核 CPU，提高并发处理能力
    // - 0：单进程模式
    // - auto：自动设置为 CPU 核心数
    // - N：指定进程数
    int worker_processes = 0;
#ifdef DEBUG
    // 调试模式下禁用多进程（方便调试）
    worker_processes = 0;
#else
    str = ini.GetValue("worker_processes");
    if (str.size() != 0) {
        if (strcmp(str.c_str(), "auto") == 0) {
            worker_processes = get_ncpu();  // 获取 CPU 核心数
            hlogd("worker_processes=ncpu=%d", worker_processes);
        }
        else {
            worker_processes = atoi(str.c_str());
        }
    }
#endif
    g_http_server.worker_processes = LIMIT(0, worker_processes, MAXNUM_WORKER_PROCESSES);
    
    // ------------------------------------------------------------------------
    // 3. 多线程配置
    // ------------------------------------------------------------------------
    
    // worker_threads：每个进程的工作线程数
    // 【概念】每个进程内部可以有多个线程处理请求
    // - 0：单线程模式
    // - auto：自动设置为 CPU 核心数
    // - N：指定线程数
    int worker_threads = 0;
    str = ini.GetValue("worker_threads");
    if (str.size() != 0) {
        if (strcmp(str.c_str(), "auto") == 0) {
            worker_threads = get_ncpu();
            hlogd("worker_threads=ncpu=%d", worker_threads);
        }
        else {
            worker_threads = atoi(str.c_str());
        }
    }
    g_http_server.worker_threads = LIMIT(0, worker_threads, 64);

    // ------------------------------------------------------------------------
    // 4. 连接数配置
    // ------------------------------------------------------------------------
    
    // worker_connections：每个工作进程的最大连接数
    // 【概念】限制单个进程能同时处理的连接数，防止资源耗尽
    str = ini.GetValue("worker_connections");
    if (str.size() != 0) {
        g_http_server.worker_connections = atoi(str.c_str());
    }

    // ------------------------------------------------------------------------
    // 5. 端口配置
    // ------------------------------------------------------------------------
    
    // HTTP 端口配置（默认 8080）
    int port = 0;
    const char* szPort = get_arg("p");  // 优先使用命令行参数
    if (szPort) {
        port = atoi(szPort);
    }
    if (port == 0) {
        port = ini.Get<int>("port");
    }
    if (port == 0) {
        port = ini.Get<int>("http_port");
    }
    g_http_server.port = port;
    
    // HTTPS 端口配置（默认 8443）
    if (HV_WITH_SSL) {
        g_http_server.https_port = ini.Get<int>("https_port");
    }
    
    // 至少需要配置一个端口
    if (g_http_server.port == 0 && g_http_server.https_port == 0) {
        printf("Please config listen port!\n");
        exit(-10);
    }

    // ------------------------------------------------------------------------
    // 6. HTTP 服务配置
    // ------------------------------------------------------------------------
    
    // base_url：API 基础路径（如 /api/v1）
    str = ini.GetValue("base_url");
    if (str.size() != 0) {
        g_http_service.base_url = str;
    }
    
    // document_root：静态文件根目录（如 html/）
    // 【概念】存放 HTML、CSS、JS、图片等静态资源的目录
    str = ini.GetValue("document_root");
    if (str.size() != 0) {
        g_http_service.document_root = str;
    }
    
    // home_page：首页文件名（如 index.html）
    str = ini.GetValue("home_page");
    if (str.size() != 0) {
        g_http_service.home_page = str;
    }
    
    // error_page：错误页面
    str = ini.GetValue("error_page");
    if (str.size() != 0) {
        g_http_service.error_page = str;
    }
    
    // index_of：目录浏览路径
    str = ini.GetValue("index_of");
    if (str.size() != 0) {
        g_http_service.index_of = str;
    }
    
    // keepalive_timeout：HTTP 长连接超时时间（毫秒）
    // 【概念】HTTP 1.1 支持长连接，一个 TCP 连接可以发送多个请求
    str = ini.GetValue("keepalive_timeout");
    if (str.size() != 0) {
        g_http_service.keepalive_timeout = atoi(str.c_str());
    }
    
    // limit_rate：下载限速（KB/s）
    str = ini.GetValue("limit_rate");
    if (str.size() != 0) {
        g_http_service.limit_rate = atoi(str.c_str());
    }
    
    // access_log：是否启用访问日志
    str = ini.GetValue("access_log");
    if (str.size() != 0) {
        g_http_service.enable_access_log = hv_getboolean(str.c_str());
    }
    
    // cors：是否启用跨域资源共享（CORS）
    // 【概念】允许浏览器从不同域名访问 API
    if (ini.Get<bool>("cors")) {
        g_http_service.AllowCORS();
    }
    
    // ------------------------------------------------------------------------
    // 7. SSL/TLS 配置（HTTPS）
    // ------------------------------------------------------------------------
    
    if (g_http_server.https_port > 0) {
        // 读取证书文件路径
        std::string crt_file = ini.GetValue("ssl_certificate");      // 证书文件
        std::string key_file = ini.GetValue("ssl_privatekey");       // 私钥文件
        std::string ca_file = ini.GetValue("ssl_ca_certificate");    // CA 证书文件
        
        hlogi("SSL backend is %s", hssl_backend());
        
        // 配置 SSL 参数
        hssl_ctx_opt_t param;
        memset(&param, 0, sizeof(param));
        param.crt_file = crt_file.c_str();
        param.key_file = key_file.c_str();
        param.ca_file = ca_file.c_str();
        param.endpoint = HSSL_SERVER;  // 服务器端
        
        // 创建 SSL 上下文
        if (g_http_server.newSslCtx(&param) != 0) {
#ifdef OS_WIN
            if (strcmp(hssl_backend(), "schannel") == 0) {
                hlogw("schannel needs pkcs12 formatted certificate file.");
                g_http_server.https_port = 0;
            }
#else
            hloge("SSL certificate verify failed!");
            exit(0);
#endif
        }
        else {
            hlogi("SSL certificate verify ok!");
        }
    }
    
    // ------------------------------------------------------------------------
    // 8. 代理配置
    // ------------------------------------------------------------------------
    
    // 【概念】代理服务器
    // - 正向代理：客户端通过代理访问外部服务器（如翻墙）
    // - 反向代理：客户端访问代理，代理转发到后端服务器（如负载均衡）
    
    auto proxy_keys = ini.GetKeys("proxy");
    for (const auto& proxy_key : proxy_keys) {
        str = ini.GetValue(proxy_key, "proxy");
        if (str.empty()) continue;
        
        if (proxy_key[0] == '/') {
            // 反向代理配置
            // 例如：/httpbin/ => http://httpbin.org/
            // 客户端访问 /httpbin/get 会被转发到 http://httpbin.org/get
            const std::string& path = proxy_key;
            std::string proxy_url = hv::ltrim(str, "> ");
            hlogi("reverse_proxy %s => %s", path.c_str(), proxy_url.c_str());
            g_http_service.Proxy(path.c_str(), proxy_url.c_str());
        }
        else if (strcmp(proxy_key.c_str(), "proxy_connect_timeout") == 0) {
            // 代理连接超时时间
            g_http_service.proxy_connect_timeout = atoi(str.c_str());
        }
        else if (strcmp(proxy_key.c_str(), "proxy_read_timeout") == 0) {
            // 代理读取超时时间
            g_http_service.proxy_read_timeout = atoi(str.c_str());
        }
        else if (strcmp(proxy_key.c_str(), "proxy_write_timeout") == 0) {
            // 代理写入超时时间
            g_http_service.proxy_write_timeout = atoi(str.c_str());
        }
        else if (strcmp(proxy_key.c_str(), "forward_proxy") == 0) {
            // 启用正向代理
            hlogi("forward_proxy = %s", str.c_str());
            if (hv_getboolean(str.c_str())) {
                g_http_service.EnableForwardProxy();
            }
        }
        else if (strcmp(proxy_key.c_str(), "trust_proxies") == 0) {
            // 信任的代理列表
            auto trust_proxies = hv::split(str, ';');
            for (auto trust_proxy : trust_proxies) {
                trust_proxy = hv::trim(trust_proxy);
                if (trust_proxy.empty()) continue;
                hlogi("trust_proxy %s", trust_proxy.c_str());
                g_http_service.AddTrustProxy(trust_proxy.c_str());
            }
        }
        else if (strcmp(proxy_key.c_str(), "no_proxies") == 0) {
            // 不使用代理的列表
            auto no_proxies = hv::split(str, ';');
            for (auto no_proxy : no_proxies) {
                no_proxy = hv::trim(no_proxy);
                if (no_proxy.empty()) continue;
                hlogi("no_proxy %s", no_proxy.c_str());
                g_http_service.AddNoProxy(no_proxy.c_str());
            }
        }
    }

    hlogi("parse_confile('%s') OK", confile);
    return 0;
}

/**
 * 重载配置文件的回调函数
 * 当收到 reload 信号时会调用此函数
 * 用法：./httpd -s reload
 */
static void on_reload(void* userdata) {
    hlogi("reload confile [%s]", g_main_ctx.confile);
    parse_confile(g_main_ctx.confile);
}

// ============================================================================
// 主函数
// ============================================================================

/**
 * 程序入口
 * 
 * 【执行流程】
 * 1. 初始化主程序上下文
 * 2. 解析命令行参数
 * 3. 处理特殊参数（help、version、test）
 * 4. 解析配置文件
 * 5. 初始化信号处理
 * 6. 注册路由
 * 7. 启动 HTTP 服务器（进入事件循环）
 */
int main(int argc, char** argv) {
    // ------------------------------------------------------------------------
    // 1. 初始化主程序上下文
    // ------------------------------------------------------------------------
    main_ctx_init(argc, argv);
    
    // ------------------------------------------------------------------------
    // 2. 解析命令行参数
    // ------------------------------------------------------------------------
    int ret = parse_opt_long(argc, argv, long_options, ARRAY_SIZE(long_options));
    if (ret != 0) {
        print_help();
        exit(ret);
    }

    // ------------------------------------------------------------------------
    // 3. 处理特殊参数
    // ------------------------------------------------------------------------
    
    // 显示帮助信息
    if (get_arg("h")) {
        print_help();
        exit(0);
    }

    // 显示版本信息
    if (get_arg("v")) {
        print_version();
        exit(0);
    }

    // ------------------------------------------------------------------------
    // 4. 解析配置文件
    // ------------------------------------------------------------------------
    const char* confile = get_arg("c");
    if (confile) {
        strncpy(g_main_ctx.confile, confile, sizeof(g_main_ctx.confile));
    }
    parse_confile(g_main_ctx.confile);

    // 测试配置文件（不启动服务器）
    if (get_arg("t")) {
        printf("Test confile [%s] OK!\n", g_main_ctx.confile);
        exit(0);
    }

    // ------------------------------------------------------------------------
    // 5. 信号处理
    // ------------------------------------------------------------------------
    // 【概念】信号是进程间通信的一种方式
    // 常用信号：start（启动）、stop（停止）、restart（重启）、reload（重载配置）
    signal_init(on_reload);
    const char* signal = get_arg("s");
    if (signal) {
        signal_handle(signal);
    }

#ifdef OS_UNIX
    // ------------------------------------------------------------------------
    // 6. 守护进程模式（后台运行）
    // ------------------------------------------------------------------------
    if (get_arg("d")) {
        // nochdir：不改变工作目录
        // noclose：不关闭标准输入输出
        int ret = daemon(1, 1);
        if (ret != 0) {
            printf("daemon error: %d\n", ret);
            exit(-10);
        }
    }
#endif

    // ------------------------------------------------------------------------
    // 7. 创建 PID 文件
    // ------------------------------------------------------------------------
    // 【概念】PID 文件记录进程 ID，用于进程管理（停止、重启等）
    create_pidfile();

    // ------------------------------------------------------------------------
    // 8. 注册路由和启动服务器
    // ------------------------------------------------------------------------
    
    // 注册所有路由（在 router.cpp 中定义）
    Router::Register(g_http_service);
    
    // 将服务配置注册到服务器
    g_http_server.registerHttpService(&g_http_service);

    // 可选：配置工作进程的启动和停止回调
#if 0
    std::atomic_flag init_flag = ATOMIC_FLAG_INIT;
    g_http_server.onWorkerStart = [&init_flag](){
        if (!init_flag.test_and_set()) {
            hv::async::startup();
        }
    };
    g_http_server.onWorkerStop = [&init_flag](){
        if (init_flag.test_and_set()) {
            hv::async::cleanup();
        }
    };
#endif

    // 启动 HTTP 服务器（阻塞运行，进入事件循环）
    // 【概念】事件循环：不断监听网络事件（新连接、数据到达等），并调用相应的处理函数
    g_http_server.run();
    
    return ret;
}

