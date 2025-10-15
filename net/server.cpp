#include "server.h"
#include "common.h"

#include "connect.h"
#include <format>
#include <glog/logging.h>
#include <memory>

extern boost::asio::io_context g_ioc;



void CServer::AddTcp(uint16_t uPort)
{
    auto pAccept = std::make_shared<boost::asio::ip::tcp::acceptor>(
        m_ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address_v4("0.0.0.0"), uPort));

    auto pSocket = std::make_shared<boost::asio::ip::tcp::socket>(m_ioc);

    m_vecAccept.push_back(pAccept);

    DoAddTcp(pAccept, pSocket);
}


void CServer::Start()
{
    LINFO << "start server";

    try
    {
        m_ioc.run();

        LINFO << "normal end";
    }
    catch (const std::exception& e)
    {
        LERROR << std::format("exception end :{}", e.what());
    }
}

void CServer::DoAddTcp(std::shared_ptr<boost::asio::ip::tcp::acceptor> pAccept,
                       std::shared_ptr<boost::asio::ip::tcp::socket>   pSocket)
{
    pAccept->async_accept(*pSocket, [pAccept, pSocket, this](boost::system::error_code er) {
        if (er)
        {
            LINFO << std::format("vps listen error :{} ", er.message().data());
            exit(0);
            return;
        }

        auto pconnect = std::make_shared<CConnect>(std::move(*pSocket));
        pconnect->Start();


        DoAddTcp(pAccept, pSocket);
    });
}

int64_t CServer::GenID()
{
    m_id++;
    if (m_mapConnect.count(m_id) > 0)
    {
        return GenID();
    }
    return m_id;
}
