#ifndef SERVER_VPS_H
#define SERVER_VPS_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

class CServer
{
public:
	CServer(boost::asio::io_context &ioc,int port,int http_port, bool _ssl);

	void Start();

private:
	void StartTcp();

	void StartHttp();

	std::string get_password() const
	{
		return "test";
	}

	boost::asio::ip::tcp::acceptor m_accept;
	boost::asio::ip::tcp::socket m_socket;

	boost::asio::ssl::context m_ssl_context;
	boost::asio::io_context &m_ioc;

	bool m_is_ssl{ false };
};





#endif