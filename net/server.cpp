#include "server.h"
#include "connect.h"
#include "connect_ssl.h"
#include "../common/common.h"
#include "../config.h"
#include <glog/logging.h>
#include <fmt/format.h>

using namespace std;
using namespace google;

extern boost::asio::io_context g_ioc;

CServer::CServer(boost::asio::io_context &ioc, int port, int http_port, bool _ssl):
	m_ioc(ioc),
	m_accept(ioc,boost::asio::ip::tcp::endpoint( boost::asio::ip::make_address_v4("0.0.0.0") , port)),
	m_socket(ioc),
	m_is_ssl(_ssl),
	m_ssl_context(boost::asio::ssl::context::sslv23)
{
	if ( m_is_ssl)
	{
		m_ssl_context.set_options(boost::asio::ssl::context::default_workarounds
			| boost::asio::ssl::context::no_sslv2
			| boost::asio::ssl::context::single_dh_use);
		m_ssl_context.set_password_callback(std::bind(&CServer::get_password, this));
		m_ssl_context.use_certificate_chain_file(Config::GetInstance().GetSSLChain());
		m_ssl_context.use_private_key_file(Config::GetInstance().GetSSLPrivkey(), boost::asio::ssl::context::pem);
		m_ssl_context.use_tmp_dh_file(Config::GetInstance().GetSSLDhFile());
	}
}

void CServer::Start()
{
	StartTcp();
}

void CServer::StartTcp()
{
	m_accept.async_accept(m_socket,
		[this](boost::system::error_code er)
	{
		if (er)
		{
			LINFO << fmt::format("vps listen error :%s ", er.message().data());
			exit(0);
			return;
		}
		if ( m_is_ssl )
		{
			auto pconnect = shared_ptr<CConnect>( new CConnectSSL( m_ioc, move(m_socket) , m_ssl_context));
			pconnect->Start();
		}
		else
		{
			auto pconnect = make_shared<CConnect>(move(m_socket));
			pconnect->Start();
		}
		
		StartTcp();
	}
	);
}

