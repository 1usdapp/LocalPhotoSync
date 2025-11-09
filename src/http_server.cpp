#include "http_server.hpp"
#include "http_session.hpp"
#include <boost/bind/bind.hpp>
#include <iostream>


namespace lps {
HttpServer::HttpServer(boost::asio::io_context& io_context, const ServerConfig& config)
    : ioc_(io_context),
      acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),
                                                           std::stoi(config.http_port))),
      config_(config)
{
    std::cout << "Http Server listening on port " << config.http_port << std::endl;
}

void HttpServer::start_accept()
{
    do_accept();
}

void HttpServer::do_accept()
{
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec)
            {
                std::cout << "New http client connected from "
                          << socket.remote_endpoint().address().to_string() << ":"
                          << socket.remote_endpoint().port() << std::endl;

                // 创建新的会话
                auto session = std::make_shared<HttpSession>(std::move(socket));
                session->run();
            }
            else
            {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }

            // 继续接受下一个连接
            do_accept();
        });
}



}   // namespace lps