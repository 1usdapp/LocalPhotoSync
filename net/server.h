#ifndef SERVER_VPS_H
#define SERVER_VPS_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

class CServer
{
public:
    CServer(boost::asio::io_context& ioc, int port);

    void Start();

private:
    void StartTcp();

    void StartHttp();

    boost::asio::ip::tcp::acceptor m_accept;
    boost::asio::ip::tcp::socket   m_socket;

    boost::asio::io_context& m_ioc;
};





#endif