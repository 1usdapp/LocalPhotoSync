#ifndef TEST_COMMON_HPP
#define TEST_COMMON_HPP

#include <gtest/gtest.h>
#include <cmd.h>
#include <con_mgr.hpp>
#include <config.hpp>
#include <crc.hpp>
#include <csmsg.pb.h>
#include <framing.hpp>
#include <http_server.hpp>
#include <http_session.hpp>
#include <msgid.pb.h>
#include <service_container.hpp>
#include <session.hpp>
#include <singleton.hpp>
#include <sqlite_db.hpp>
#include <tcp_server.hpp>
#include <udp_broadcaster.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <set>
#include <thread>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace lps {
// Common test helper: TempDir
struct TempDir
{
    fs::path path;
    TempDir()
    {
        path = fs::temp_directory_path() / fs::path("lps_test_" + std::to_string(::getpid()) + "_" 
               + std::to_string(::time(nullptr)) + "_" + std::to_string(rand()));
        fs::create_directories(path);
    }
    ~TempDir() { std::error_code ec; fs::remove_all(path, ec); }
};

// Common test helper: SocketPair
struct SocketPair
{
    boost::asio::ip::tcp::socket server;
    boost::asio::ip::tcp::socket client;
    SocketPair(boost::asio::io_context& ioc)
        : server(ioc), client(ioc)
    {
        boost::asio::ip::tcp::acceptor acc(ioc, {boost::asio::ip::tcp::v4(), 0});
        auto port = acc.local_endpoint().port();
        client.connect({boost::asio::ip::address_v4::loopback(), port});
        acc.accept(server);
    }
};

// Common test helper: BuildPkg
inline std::vector<uint8_t> BuildPkg(const LocalPhotoSync::MsgPkg& msg, uint32_t cmd_id)
{
    std::string serialized;
    msg.SerializeToString(&serialized);

    PkgHead head{};
    head.PackageLen = kCurHeadLen + serialized.size();
    head.HeadLen = kCurHeadLen;
    head.Version = 1;
    head.CMDID = cmd_id;
    head.Reserve = 0;
    head.Reserve2 = 0;

    std::vector<uint8_t> head_buf(kCurHeadLen);
    if (!encode_pkg_head(head, head_buf.data(), head_buf.size())) {
        throw std::runtime_error("Failed to encode package head");
    }

    std::vector<uint8_t> full;
    full.insert(full.end(), head_buf.begin(), head_buf.end());
    full.insert(full.end(), serialized.begin(), serialized.end());
    return full;
}

}   // namespace lps

#endif   // TEST_COMMON_HPP
