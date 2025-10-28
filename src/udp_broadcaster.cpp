#include "udp_broadcaster.hpp"
#include "framing.hpp"
#include <boost/bind/bind.hpp>
#include <iostream>

namespace lps {
UdpBroadcaster::UdpBroadcaster(boost::asio::io_context& io_context, const ServerConfig& config)
    : io_context_(io_context), socket_(io_context), config_(config)
{
    // 创建UDP socket
    socket_.open(boost::asio::ip::udp::v4());
    socket_.set_option(boost::asio::socket_base::broadcast(true));

    setup_broadcast_endpoint();
    create_server_info_message();
}

void UdpBroadcaster::setup_broadcast_endpoint()
{
    boost::asio::ip::address broadcast_addr = boost::asio::ip::make_address(config_.broadcast);
    unsigned short           port           = std::stoi(config_.udp_port);
    broadcast_endpoint_                     = boost::asio::ip::udp::endpoint(broadcast_addr, port);
}

void UdpBroadcaster::create_server_info_message()
{
    server_info_msg_ = std::make_shared<LocalPhotoSync::MsgPkg>();
    server_info_msg_->set_resultid(0);
    server_info_msg_->set_serialid(0);

    auto* server_info = server_info_msg_->mutable_csntyserverinfo();
    server_info->set_tcpport(std::stoi(config_.tcp_port));
    server_info->set_name(config_.name);
    server_info->set_rootpath(config_.root);
    server_info->set_os(config_.os);
}

void UdpBroadcaster::start_broadcast()
{
    do_broadcast();
}

void UdpBroadcaster::do_broadcast()
{
    broadcast_server_info();

    // 创建定时器，每秒广播一次
    auto timer = std::make_shared<boost::asio::steady_timer>(io_context_, std::chrono::seconds(1));
    timer->async_wait([this, timer](const boost::system::error_code& error) {
        if (!error)
        {
            do_broadcast();
        }
    });
}

void UdpBroadcaster::broadcast_server_info()
{
    try
    {
        // 序列化protobuf消息
        std::string serialized_data;
        server_info_msg_->SerializeToString(&serialized_data);

        // 编码长度前缀
        std::vector<uint8_t> length_prefix = lps::encode_length(serialized_data.size());

        // 组合完整消息
        std::vector<uint8_t> full_message;
        full_message.insert(full_message.end(), length_prefix.begin(), length_prefix.end());
        full_message.insert(full_message.end(), serialized_data.begin(), serialized_data.end());

        // 发送广播
        socket_.send_to(boost::asio::buffer(full_message), broadcast_endpoint_);

        std::cout << "Broadcast server info: " << config_.name << " (TCP:" << config_.tcp_port
                  << ")" << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << "Broadcast error: " << e.what() << std::endl;
    }
}
}   // namespace lps