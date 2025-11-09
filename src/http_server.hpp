#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include "config.hpp"
#include "http_session.hpp"
#include <algorithm>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>


namespace lps {


class HttpServer : public std::enable_shared_from_this<HttpServer>
{
public:
    HttpServer(boost::asio::io_context& io_context, const ServerConfig& config);

    void start_accept();

private:
    void do_accept();

private:
    boost::asio::io_context&           ioc_;
    boost::asio::ip::tcp::acceptor     acceptor_;
    std::shared_ptr<std::string const> doc_root_;
    ServerConfig                       config_;
};

}   // namespace lps



#endif