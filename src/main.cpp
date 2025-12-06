#include "config.hpp"
#include "crc.hpp"
#include "http_server.hpp"
#include "http_session.hpp"
#include "service_container.hpp"
#include "tcp_server.hpp"
#include "udp_broadcaster.hpp"
#include <boost/asio.hpp>
#include <gflags/gflags.h>
#include <iostream>
#include <memory>



DEFINE_string(tcp_port, "9176", "TCP服务器端口");
DEFINE_string(udp_port, "9176", "UDP服务器端口");
DEFINE_string(http_port, "9175", "HTTP服务器端口");
DEFINE_string(root, "./data", "图片根目录");


int main(int argc, char* argv[])
{
    // 解析命令行参数
    using namespace GFLAGS_NAMESPACE;
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    try
    {
        ServerConfig config;
        config.tcp_port = FLAGS_tcp_port;
        config.udp_port = FLAGS_udp_port;
        config.http_port = FLAGS_http_port;
        config.root = FLAGS_root;

        // 初始化CRC32表
        lps::init_crc32_table();

        boost::asio::io_context io_context;


        // 创建TCP服务器
        lps::TcpServer tcp_server(io_context, config, lps::CCServiceContainerInstance::instance());

        // 创建UDP广播器
        lps::UdpBroadcaster udp_broadcaster(io_context, config);

        // 创建http 服务器
        lps::HttpServer http_server(
            io_context, config, lps::CCServiceContainerInstance::instance());

        // 启动UDP广播
        udp_broadcaster.start_broadcast();

        // 启动TCP服务器
        tcp_server.start_accept();

        // 启动http server
        http_server.start_accept();

        // Capture SIGINT and SIGTERM to perform a clean shutdown
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code&, int) {
            // Stop the `io_context`. This will cause `run()`
            // to return immediately, eventually destroying the
            // `io_context` and all of the sockets in it.
            io_context.stop();
        });

        // 运行io_context
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}