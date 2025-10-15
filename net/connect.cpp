#include "connect.h"
#include "common.h"
#include "msg.h"


#include <glog/logging.h>
#include <string>




CConnect::CConnect(boost::asio::ip::tcp::socket&& _socket)
    : m_socket(std::move(_socket)), m_need_close(false), m_id(GetID())
{
    std::string m_str_ip = m_socket.remote_endpoint().address().to_string();
    LINFO << std::format("new connect ip:{}	m_id:{}", m_str_ip.data(), m_id);
}

CConnect::CConnect(boost::asio::io_context& ioc) : m_socket(ioc), m_id(GetID()) {}

CConnect::~CConnect()
{
    LINFO << std::format("id:{} close", m_id);
}

void CConnect::Start()
{
    if (m_pEvent)
    {
        m_pEvent->OnConnected(m_id, shared_from_this());
    }
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
        if (m_pEvent)
        {
            m_pEvent->OnClosed(m_id);
        }
    }
}

void CConnect::DoRead()
{
    auto self = shared_from_this();

    m_socket.async_read_some(
        boost::asio::buffer(m_recv_buffer, RECV_BUFF_LEN),
        std::bind(&CConnect::ReadCb, self, std::placeholders::_1, std::placeholders::_2));
}

void CConnect::ReadCb(boost::system::error_code er, size_t length)
{
    if (er)
    {
        NotifyClose();
        LINFO << std::format("id:{}  close er:{} \t", m_id, er.message().data());
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

    boost::asio::async_write(
        m_socket,
        boost::asio::buffer(*m_list_send.front()),
        bind(&CConnect::SendCb, self, std::placeholders::_1, std::placeholders::_2));
}

void CConnect::NotifyClose()
{
    if (m_need_close)
    {
        return;
    }
    if (m_pEvent)
    {
        m_pEvent->OnClosed(m_id);
    }
}


void CConnect::HandlePacket()
{
    if (m_recv_data.size() < sizeof(PkgHead))
    {
        return;
    }

    PkgHead* phead = (PkgHead*)m_recv_data.data();

    if (phead->PackageLen > RECV_BUFF_LEN)
    {
        LINFO << std::format("packet length:{} ", phead->PackageLen);
        Close();
        return;
    }

    if (m_recv_data.size() < (phead->PackageLen + sizeof(PkgHead)))
    {
        return;
    }

    MsgInfo msg;
    msg.conid = m_id;
    msg.head  = *phead;
    msg.pstr =
        std::make_shared<std::string>(m_recv_data.data() + sizeof(PkgHead), phead->PackageLen);

    m_pEvent->OnHandlePacket(m_id, shared_from_this());

    m_recv_data.erase(0, phead->PackageLen + sizeof(PkgHead));

    HandlePacket();
}
