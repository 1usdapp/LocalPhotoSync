#include "server.h"
#include "common/common.h"
#include "config.h"
#include "connect.h"
#include <format>
#include <glog/logging.h>

extern boost::asio::io_context g_ioc;

CServer::CServer(boost::asio::io_context& ioc, int port)
    : m_ioc(ioc), m_accept(ioc, boost::asio::ip::tcp::endpoint(
                                    boost::asio::ip::make_address_v4("0.0.0.0"), port)),
      m_socket(ioc)
{}

void CServer::Start()
{
    StartTcp();
}

void CServer::StartTcp()
{
    m_accept.async_accept(m_socket, [this](boost::system::error_code er) {
        if (er)
        {
            LINFO << std::format("vps listen error :{} ", er.message().data());
            exit(0);
            return;
        }

        auto pconnect = std::make_shared<CConnect>(std::move(m_socket));
        pconnect->Start();


        StartTcp();
    });
}
