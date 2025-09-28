#include "connect_ssl.h"
#include <glog/logging.h>
#include "../common/common.h"
#include "../common/msg.h"
#include <fmt/format.h>
// #include "../proto/proto.h"
// #include "common/protobuf2json.h"
using namespace std;
using namespace google;

extern boost::asio::io_context g_ioc;

CConnectSSL::CConnectSSL(boost::asio::io_context &ioc, boost::asio::ip::tcp::socket&& _socket, boost::asio::ssl::context& context):
	CConnect(ioc),
	m_ssl_socket(move(_socket), context)
{
	LINFO << fmt::format("new connect m_id:%lld",m_id);
}

void CConnectSSL::Start()
{
	HandShack();
}


void CConnectSSL::DoRead()
{
	auto self = shared_from_this();

	m_ssl_socket.async_read_some(boost::asio::buffer(m_recv_buffer, RECV_BUFF_LEN),
		bind(&CConnect::ReadCb, self, placeholders::_1, placeholders::_2));
}

void CConnectSSL::DoSend()
{
	auto self = shared_from_this();

	boost::asio::async_write(m_ssl_socket, boost::asio::buffer(*m_list_send.front()),
		bind(&CConnect::SendCb, self, placeholders::_1, placeholders::_2));
}

void CConnectSSL::DoClose()
{
	LINFO << fmt::format("close ");

	if (m_ssl_socket.next_layer().is_open())
	{
		m_ssl_socket.next_layer().close();
	}
}

void CConnectSSL::HandShack()
{
	auto self = shared_from_this();
	m_ssl_socket.async_handshake(
		boost::asio::ssl::stream_base::server,
		[this, self](boost::system::error_code er)
	{
		if (er)
		{
			LINFO << fmt::format("er:%s", er.message().data());
			return;
		}

		DoRead();
	}
	);
}

