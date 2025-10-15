#pragma once

#include <atomic>
#include <boost/asio.hpp>

#include <memory>
#include <unordered_map>
#include <vector>
#include "connect.h"

class CServer
{
public:
    CServer() = default;

    void AddTcp(uint16_t uPort);

    void Start();
private:
    void DoAddTcp(std::shared_ptr<boost::asio::ip::tcp::acceptor> pAccept,
                  std::shared_ptr<boost::asio::ip::tcp::socket>   pSocket);

    int64_t GenID();

    std::vector<std::shared_ptr<boost::asio::ip::tcp::acceptor>> m_vecAccept;
    boost::asio::io_context m_ioc;

    std::unordered_map<int64_t,std::shared_ptr<CConnect>> m_mapConnect;
    std::atomic<int64_t> m_id;
};





