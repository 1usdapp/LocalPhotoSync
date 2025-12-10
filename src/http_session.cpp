#include "http_session.hpp"
#include "../proto/http.pb.h"
#include "service_container.hpp"
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <functional>


namespace lps {

HttpSession::HttpSession(boost::asio::ip::tcp::socket&& socket, std::shared_ptr<CConMgr> con_mgr)
    : stream_(std::move(socket)), con_mgr_(con_mgr)
{
    //
}


void HttpSession::run()
{
    do_read();
}

void HttpSession::do_read()
{
    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.
    req_ = {};

    // Set the timeout.
    stream_.expires_after(std::chrono::seconds(30));

    // Read a request
    boost::beast::http::async_read(
        stream_,
        buffer_,
        req_,
        boost::beast::bind_front_handler(&HttpSession::on_read, shared_from_this()));
}

void HttpSession::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == boost::beast::http::error::end_of_stream)
        return do_close();

    if (ec)
    {
        std::cerr << ec.what() << ec.message() << std::endl;
        return;
    }


    // Send the response
    process_request();
}


void HttpSession::do_close()
{
    // Send a TCP shutdown
    boost::beast::error_code ec;
    stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
}


void HttpSession::process_request()
{
    // 处理 OPTIONS 预检请求（Pre-flight）
    if (req_.method() == boost::beast::http::verb::options) {
        response_.result(boost::beast::http::status::ok);
        response_.set(boost::beast::http::field::access_control_allow_origin, "*");
        response_.set(boost::beast::http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
        response_.set(boost::beast::http::field::access_control_allow_headers, "Content-Type, Authorization");
        response_.set(boost::beast::http::field::content_length, "0");
        write_response();
        return;
    }

    // 在所有响应中添加 CORS 头
    response_.set(boost::beast::http::field::access_control_allow_origin, "*");
    // 如果需要支持凭据，使用具体 origin 并加 Credentials 头


    response_.version(req_.version());
    response_.keep_alive(true);
    response_.result(boost::beast::http::status::ok);
    response_.set(boost::beast::http::field::server, "Beast");

    switch (req_.method())
    {
    case boost::beast::http::verb::get:
    {
        response_.set(boost::beast::http::field::content_type, "application/json");
        handle_get_method();
        break;
    }
    case boost::beast::http::verb::post:
    {
        handle_post_method();
        break;
    }
    default: response_.body() = R"({"status":-1})"; break;
    }

    write_response();
}

void HttpSession::create_response() {}

void HttpSession::write_response()
{
    auto self = shared_from_this();

    response_.content_length(response_.body().size());

    boost::beast::http::async_write(
        stream_, response_, [self](boost::beast::error_code ec, std::size_t bytes_transferred) {
            self->on_write(ec, bytes_transferred);
        });
}

void HttpSession::on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        std::cerr << ec.what() << ec.message() << std::endl;
        return;
    }


    /* if (!keep_alive)
    {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        return do_close();
    } */

    // Read another request
    do_read();
}

void HttpSession::handle_get_method()
{
    if (req_.target() == "/clients")
    {
        handle_get_clients();
    }
}

void HttpSession::handle_get_clients()
{
    if (con_mgr_)
    {
        response_.body() = con_mgr_->client_info();
    }
}

void HttpSession::handle_post_method()
{
    //
}

}   // namespace lps