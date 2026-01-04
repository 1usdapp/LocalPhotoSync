#include "test_common.hpp"

namespace lps {

// ============================================================================
// TcpServer Tests
// ============================================================================
TEST(TcpServerTest, AcceptsConnection)
{
    init_crc32_table();
    TempDir tmp;
    boost::asio::io_context ioc;

    // Reserve a free port first
    boost::asio::ip::tcp::acceptor probe(ioc, {boost::asio::ip::tcp::v4(), 0});
    auto port = probe.local_endpoint().port();
    probe.close();

    ServerConfig cfg;
    cfg.tcp_port = std::to_string(port);
    cfg.root = tmp.path.string();

    CServiceContainer container;
    TcpServer server(ioc, cfg, container);
    server.start_accept();

    std::thread t([&]() { ioc.run(); });

    boost::asio::ip::tcp::socket client(ioc);
    client.connect({boost::asio::ip::address_v4::loopback(), port});

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ioc.stop();
    t.join();

    auto con_mgr = container.GetConMgr();
    std::string info = con_mgr->client_info();
    EXPECT_FALSE(info.empty());
}

// ============================================================================
// UdpBroadcaster Tests
// ============================================================================
TEST(UdpBroadcasterTest, BroadcastsServerInfo)
{
    init_crc32_table();
    boost::asio::io_context ioc;

    unsigned short port = 0;
    {
        boost::asio::ip::udp::socket probe(ioc, {boost::asio::ip::udp::v4(), 0});
        port = probe.local_endpoint().port();
    }

    ServerConfig cfg;
    cfg.broadcast = "127.0.0.1";
    cfg.udp_port = std::to_string(port);
    cfg.tcp_port = "5555";
    cfg.name = "udp-test";
    cfg.root = "/tmp/root";
    cfg.os = "linux";

    UdpBroadcaster broadcaster(ioc, cfg);

    boost::asio::ip::udp::socket receiver(ioc, {boost::asio::ip::udp::v4(), port});
    receiver.non_blocking(true);

    broadcaster.broadcast_server_info();

    std::vector<uint8_t> buf(512);
    boost::asio::ip::udp::endpoint sender;
    bool received = false;
    for (int i = 0; i < 50 && !received; ++i)
    {
        boost::system::error_code ec;
        auto n = receiver.receive_from(boost::asio::buffer(buf), sender, 0, ec);
        if (!ec && n > 0)
        {
            received = true;

            // Skip head, parse protobuf payload
            ASSERT_GE(n, static_cast<std::size_t>(kCurHeadLen));
            LocalPhotoSync::MsgPkg pkg;
            ASSERT_TRUE(pkg.ParseFromArray(buf.data() + kCurHeadLen, n - kCurHeadLen));
            EXPECT_EQ(pkg.csntyserverinfo().tcpport(), 5555);
            EXPECT_EQ(pkg.csntyserverinfo().name(), "udp-test");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(received);
}

}   // namespace lps
