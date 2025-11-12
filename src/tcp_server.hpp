#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include "config.hpp"
#include "service_container.hpp"
#include "session.hpp"
#include <boost/asio.hpp>
#include <memory>

namespace lps {
class TcpServer
{
private:
    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    ServerConfig config_;

public:
    TcpServer(boost::asio::io_context& io_context, const ServerConfig& config,
              CServiceContainer& sc);
    void start_accept();


private:
    void do_accept();
    CServiceContainer& service_container_;
};
}   // namespace lps

#endif   // TCP_SERVER_HPP