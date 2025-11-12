#ifndef UDP_BROADCASTER_HPP
#define UDP_BROADCASTER_HPP

#include "../proto/csmsg.pb.h"
#include "config.hpp"
#include <boost/asio.hpp>
#include <memory>

namespace lps {
class UdpBroadcaster
{
private:
    boost::asio::io_context& io_context_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint broadcast_endpoint_;
    ServerConfig config_;
    std::shared_ptr<LocalPhotoSync::MsgPkg> server_info_msg_;

public:
    UdpBroadcaster(boost::asio::io_context& io_context, const ServerConfig& config);
    void start_broadcast();
    void broadcast_server_info();

private:
    void do_broadcast();
    void setup_broadcast_endpoint();
    void create_server_info_message();
};
}   // namespace lps

#endif   // UDP_BROADCASTER_HPP