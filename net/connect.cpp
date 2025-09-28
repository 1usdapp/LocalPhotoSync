#include "connect.h"
#include "../common/common.h"
#include "../common/msg.h"
#include "../global.h"
#include <fmt/format.h>
#include <string>
#include <glog/logging.h>
using namespace std;
using namespace google;

CConnect::CConnect(boost::asio::ip::tcp::socket&& _socket) :
	m_socket(move(_socket)),
	m_need_close(false),
	m_id(GetID())
{
	string m_str_ip = m_socket.remote_endpoint().address().to_string();
	LINFO << fmt::format("new connect ip:{}	m_id:{}", m_str_ip.data(), m_id);
}

CConnect::CConnect(boost::asio::io_context &ioc) :
	m_socket(ioc),
	m_id(GetID())
{

}

CConnect::~CConnect()
{
	LINFO << fmt::format("id:{} close", m_id);
}

void CConnect::Start()
{
	g_network.AddClient(m_id, shared_from_this());
	DoRead();
}

void CConnect::SendData(std::shared_ptr<std::string> pdata)
{
	auto self = shared_from_this();


	if (m_list_send.empty())
	{
		m_list_send.push_back(pdata);
		DoSend();
		return;
	}

	m_list_send.push_back(pdata);
}

void CConnect::Close()
{
	m_need_close = true;

	if (m_list_send.empty())
	{
		m_socket.close();
	}

}

void CConnect::DoRead()
{
	auto self = shared_from_this();

	m_socket.async_read_some(boost::asio::buffer(m_recv_buffer, RECV_BUFF_LEN),
		bind(&CConnect::ReadCb, self, placeholders::_1, placeholders::_2));
}

void CConnect::ReadCb(boost::system::error_code er, size_t length)
{
	if (er)
	{
		NotifyClose();
		LINFO << fmt::format("id:{}  close er:{} \t", m_id, er.message().data());
		return;
	}

	m_recv_data.append(m_recv_buffer, length);

	HandlePacket();

	DoRead();
}

void CConnect::SendCb(boost::system::error_code er, size_t length)
{
	if (er)
	{
		return;
	}

	m_list_send.pop_front();
	if (m_list_send.empty())
	{
		if (m_need_close)
		{
			m_socket.close();
		}
		return;
	}

	DoSend();
}


void CConnect::DoSend()
{
	auto self = shared_from_this();

	boost::asio::async_write(m_socket, boost::asio::buffer(*m_list_send.front()),
		bind(&CConnect::SendCb , self , placeholders::_1 , placeholders::_2));
}

void CConnect::NotifyClose()
{
	if ( m_need_close )
	{
		return;
	}
	g_network.DelClient(m_id);
}


void CConnect::HandlePacket()
{
	if (m_recv_data.size() < sizeof(MSG_HEAD))
	{
		return;
	}

	MSG_HEAD *phead = (MSG_HEAD *)m_recv_data.data();

	if ( phead->length > 10240 )
	{
		LINFO << fmt::format("packet length:{} ", phead->length);
		Close();
		return;
	}

	if (m_recv_data.size() < (phead->length + sizeof(MSG_HEAD)))
	{
		return;
	}

	MsgInfo msg;
	msg.conid = m_id;
	msg.head = *phead;
	msg.pstr = make_shared<string>(m_recv_data.data() + sizeof(MSG_HEAD), phead->length);

	g_work_mgr.PostMsg(msg);

	m_recv_data.erase(0, phead->length + sizeof(MSG_HEAD));

	HandlePacket();
}

