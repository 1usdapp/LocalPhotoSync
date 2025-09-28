#include "net_mgr.h"
#include "../common/macros.h"
#include "../config.h"
#include "../global.h"
#include <fmt/format.h>
#include <glog/logging.h>
using namespace std;
using namespace google;

NetMgr::NetMgr() :
	m_server(m_ioc,Config::GetInstance().GetPort(), Config::GetInstance().GetHttpPort(), Config::GetInstance().GetSSLEnable())
{

}

void NetMgr::Start()
{
	LINFO << "start server";

	try
	{
		m_server.Start();
		m_ioc.run();

		LINFO << "normal end";
	}
	catch (const std::exception& e)
	{
		LERROR << fmt::format("exception end :{}", e.what());
	}
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