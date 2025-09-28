#ifndef NET_MGR_H
#define NET_MGR_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <boost/asio.hpp>
#include "server.h"
#include "connect.h"

class NetMgr
{
public:
	NetMgr();

	void Start();


	//io send

	void Send(int64_t conid, std::shared_ptr<std::string> pstring);


	//io conid for client
	void AddClient(int64_t conid, std::shared_ptr<CConnect> pclient);

	void DelClient(int64_t conid);

	// io conid for server

	void CloseClient(int64_t conid);


private:

	std::unordered_map<int64_t, std::shared_ptr<CConnect>> m_map_client;

	CServer m_server;

	boost::asio::io_context m_ioc;


};

#endif


