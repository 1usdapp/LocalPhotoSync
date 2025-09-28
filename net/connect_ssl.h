#ifndef	CONNECT_SSL_H
#define CONNECT_SSL_H

#include "connect.h"
#include <boost/asio/ssl.hpp>



class CConnectSSL :
	public CConnect
{
public:
	CConnectSSL(boost::asio::io_context &ioc, boost::asio::ip::tcp::socket&& _socket, boost::asio::ssl::context& context);

	virtual void Start() override;

protected:
	virtual void DoRead() override;

	virtual void DoSend() override;

	virtual void DoClose();

	void HandShack();

private:

	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_ssl_socket;
};




#endif