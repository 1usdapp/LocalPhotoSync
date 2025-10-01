#include "net_mgr.h"
#include "common/macros.h"
#include "server.h"
#include <glog/logging.h>




void NetMgr::Start()
{
	LINFO << "start server";

	try
	{
		for (auto pServer:m_listServer) 
		{
			pServer->Start();
		}
		m_ioc.run();

		LINFO << "normal end";
	}
	catch (const std::exception& e)
	{
		LERROR << std::format("exception end :{}", e.what());
	}
}

bool NetMgr::AddListenPort(uint16_t uPort)
{
	auto pServer = std::make_shared<CServer>(m_ioc , uPort);
	m_listServer.push_back(pServer);
	return true;
}

void NetMgr::Send(int64_t conid, std::shared_ptr<std::string> pstring)
{
	if (m_map_client.find(conid) == m_map_client.end() )
	{
		LERROR << fmt::format("not find conid:{}", conid);
		return;
	}
	
	m_map_client[conid]->SendData(pstring);
}


void NetMgr::AddClient(int64_t conid, std::shared_ptr<CConnect> pclient)
{
	LINFO << fmt::format("add conid:{}", conid);
	m_map_client[conid] = pclient;
}

void NetMgr::DelClient(int64_t conid)
{
	LINFO << fmt::format("del conid:{}", conid);
	m_map_client.erase(conid);

	g_vps_mgr.DelConid(conid);
}

void NetMgr::CloseClient(int64_t conid)
{
	LINFO << fmt::format(" conid:{}", conid);
	if ( m_map_client.find(conid) == m_map_client.end() )
	{
		LERROR << fmt::format(" not find conid:{}", conid);
		return;
	}
	m_map_client[conid]->Close();
}