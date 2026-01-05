#include "test_common.hpp"

namespace lps {

// ============================================================================
// Session integration tests
// ============================================================================
TEST(SessionTest, DeviceInfoCreatesDirAndDb)
{
    init_crc32_table();
    TempDir tmp;
    boost::asio::io_context ioc;
    SocketPair sockets(ioc);

    auto con_mgr = std::make_shared<CConMgr>();
    auto db_mgr = std::make_shared<SqliteDBManager>();
    ServerConfig cfg;
    cfg.root = tmp.path.string();

    auto session = std::make_shared<Session>(std::move(sockets.server), cfg, con_mgr, db_mgr);

    std::thread t([&]() { ioc.run(); });
    session->start();

    LocalPhotoSync::MsgPkg msg;
    auto* req = msg.mutable_csreqdeviceinfo();
    req->set_deviceid("dev123");
    req->set_path("device_dir");

    auto data = BuildPkg(msg, LocalPhotoSync::ID_CSReqDeviceInfo);
    boost::asio::write(sockets.client, boost::asio::buffer(data));

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    ioc.stop();
    t.join();

    fs::path expected_dir = tmp.path / "device_dir";
    EXPECT_TRUE(fs::exists(expected_dir));
    EXPECT_TRUE(fs::exists(expected_dir / "index.db"));

    SqliteCrcDB db((expected_dir / "index.db").string());
    EXPECT_TRUE(db.open());
    db.close();
}

TEST(SessionTest, SyncPhotoWritesFile)
{
    init_crc32_table();
    TempDir tmp;
    boost::asio::io_context ioc;
    SocketPair sockets(ioc);

    auto con_mgr = std::make_shared<CConMgr>();
    auto db_mgr = std::make_shared<SqliteDBManager>();
    ServerConfig cfg;
    cfg.root = tmp.path.string();

    auto session = std::make_shared<Session>(std::move(sockets.server), cfg, con_mgr, db_mgr);
    std::thread t([&]() { ioc.run(); });
    session->start();

    // 1) device info
    LocalPhotoSync::MsgPkg msg_dev;
    auto* req_dev = msg_dev.mutable_csreqdeviceinfo();
    req_dev->set_deviceid("dev123");
    req_dev->set_path("syncdir");
    auto dev_data = BuildPkg(msg_dev, LocalPhotoSync::ID_CSReqDeviceInfo);
    boost::asio::write(sockets.client, boost::asio::buffer(dev_data));

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // 2) sync photo single packet
    LocalPhotoSync::MsgPkg msg_sync;
    auto* req_sync = msg_sync.mutable_csreqsyncphoto();
    req_sync->set_filename("photo.bin");
    std::string payload = "hello world";
    req_sync->set_data(payload);
    req_sync->set_offset(0);
    req_sync->set_hasnextpkt(false);
    req_sync->set_crc32(lps::crc32(std::vector<uint8_t>(payload.begin(), payload.end())));
    req_sync->mutable_syncinfo()->set_photonum(1);
    req_sync->mutable_syncinfo()->set_curnum(1);

    auto sync_data = BuildPkg(msg_sync, LocalPhotoSync::ID_CSReqSyncPhoto);
    boost::asio::write(sockets.client, boost::asio::buffer(sync_data));

    // Give enough time for write
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ioc.stop();
    t.join();

    fs::path file_path = tmp.path / "syncdir" / "photo.bin";
    ASSERT_TRUE(fs::exists(file_path)) << "File should exist at: " << file_path;

    // Verify file content matches payload
    std::ifstream file(file_path, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    EXPECT_EQ(content, payload) << "File content should match sent payload";
    
    // Verify CRC is computed on the file
    uint32_t file_crc = lps::crc32_file(file_path.string());
    EXPECT_NE(file_crc, 0u) << "File CRC should be non-zero";
}

// ============================================================================
// HttpSession Tests
// ============================================================================
TEST(HttpSessionTest, GetClientsReturnsJson)
{
    init_crc32_table();
    TempDir tmp;
    boost::asio::io_context ioc;

    // First: create TCP session to register a client
    {
        SocketPair tcp_sockets(ioc);
        auto con_mgr = std::make_shared<CConMgr>();
        auto db_mgr = std::make_shared<SqliteDBManager>();
        ServerConfig cfg;
        cfg.root = tmp.path.string();

        auto session = std::make_shared<Session>(std::move(tcp_sockets.server), cfg, con_mgr, db_mgr);
        std::thread t([&]() { ioc.run_for(std::chrono::milliseconds(300)); });
        session->start();

        LocalPhotoSync::MsgPkg msg_dev;
        auto* req_dev = msg_dev.mutable_csreqdeviceinfo();
        req_dev->set_deviceid("dev_http");
        req_dev->set_path("httpdir");
        auto dev_data = BuildPkg(msg_dev, LocalPhotoSync::ID_CSReqDeviceInfo);
        boost::asio::write(tcp_sockets.client, boost::asio::buffer(dev_data));

        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        t.join();

        // Now test HTTP on the same con_mgr with registered client
        boost::asio::io_context http_ioc;
        SocketPair http_sockets(http_ioc);
        auto http_session = std::make_shared<HttpSession>(std::move(http_sockets.server), con_mgr);

        std::thread http_thread([&]() { http_ioc.run_for(std::chrono::milliseconds(200)); });
        http_session->run();

        std::string request = "GET /clients HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
        boost::asio::write(http_sockets.client, boost::asio::buffer(request));

        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        // Read response
        std::vector<char> resp_buf(4096);
        boost::system::error_code ec;
        auto n = http_sockets.client.read_some(boost::asio::buffer(resp_buf), ec);
        resp_buf.resize(n);

        http_thread.join();

        std::string response(resp_buf.begin(), resp_buf.end());
        EXPECT_NE(response.find("200 OK"), std::string::npos);
        EXPECT_NE(response.find("application/json"), std::string::npos);
        // Verify JSON was returned (dev_http may be in body)
        EXPECT_GT(response.size(), 100u);
    }
}

}   // namespace lps
