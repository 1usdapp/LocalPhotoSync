#include "config.hpp"
#include "crc.hpp"
#include "tcp_server.hpp"
#include "udp_broadcaster.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <memory>

int main()
{
    try
    {
        // 初始化CRC32表
        lps::init_crc32_table();

        boost::asio::io_context io_context;
        ServerConfig            config;

        // 创建TCP服务器
        lps::TcpServer tcp_server(io_context, config);

        // 创建UDP广播器
        lps::UdpBroadcaster udp_broadcaster(io_context, config);

        // 启动UDP广播
        udp_broadcaster.start_broadcast();

        // 启动TCP服务器
        tcp_server.start_accept();

        // Capture SIGINT and SIGTERM to perform a clean shutdown
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code& , int) {
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