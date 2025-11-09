#include "http_session.hpp"
#include <functional>


namespace lps {

HttpSession::HttpSession(boost::asio::ip::tcp::socket&& socket) : stream_(std::move(socket))
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
    response_.version(req_.version());
    response_.keep_alive(false);

    switch (req_.method())
    {
    case boost::beast::http::verb::get:
        response_.result(boost::beast::http::status::ok);
        response_.set(boost::beast::http::field::server, "Beast");
        create_response();
        break;

    default:
        // We return responses indicating an error if
        // we do not recognize the request method.
        response_.result(boost::beast::http::status::bad_request);
        response_.set(boost::beast::http::field::content_type, "text/plain");
        boost::beast::ostream(response_.body())
            << "Invalid request-method '" << std::string(req_.method_string()) << "'";
        break;
    }

    write_response();
}

void HttpSession::create_response()
{
    if (req_.target() == "/count")
    {
        response_.set(boost::beast::http::field::content_type, "text/html");
        boost::beast::ostream(response_.body()) << "<html>\n"
                                                << "<head><title>Request count</title></head>\n"
                                                << "<body>\n"
                                                << "<h1>Request count</h1>\n"
                                                << "<p>There have been "
                                                << " requests so far.</p>\n"
                                                << "</body>\n"
                                                << "</html>\n";
    }
    else if (req_.target() == "/time")
    {
        response_.set(boost::beast::http::field::content_type, "text/html");
        boost::beast::ostream(response_.body()) << "<html>\n"
                                                << "<head><title>Current time</title></head>\n"
                                                << "<body>\n"
                                                << "<h1>Current time</h1>\n"
                                                << "<p>The current time is "
                                                << " seconds since the epoch.</p>\n"
                                                << "</body>\n"
                                                << "</html>\n";
    }
    else
    {
        response_.result(boost::beast::http::status::not_found);
        response_.set(boost::beast::http::field::content_type, "text/plain");
        boost::beast::ostream(response_.body()) << "File not found\r\n";
    }
}

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

}   // namespace lps