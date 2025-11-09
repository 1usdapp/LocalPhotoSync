#ifndef HTTP_SESSION_HPP
#define HTTP_SESSION_HPP

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
class HttpSession : public std::enable_shared_from_this<HttpSession>
{
public:
    HttpSession(boost::asio::ip::tcp::socket&& socket);

    void run();

    void do_read();

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);

    void do_close();

    void process_request();

    void create_response();

    void write_response();

    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);

private:
    boost::beast::tcp_stream                                     stream_;
    boost::beast::flat_buffer                                    buffer_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    // The response message.
    boost::beast::http::response<boost::beast::http::dynamic_body> response_;
};
}   // namespace lps

#endif