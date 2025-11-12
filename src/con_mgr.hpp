#ifndef CON_MGR_HPP
#define CON_MGR_HPP

#include "../proto/http.pb.h"
#include "session.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace lps {

class CConMgr : public Session::ITCPConEvent
{
public:
    // ITCPConEvent
    virtual uint32_t on_get_conid();
    virtual int on_connected(uint32_t id, std::weak_ptr<Session> psession);
    virtual int on_closed(uint32_t id);
    virtual int on_sync_info_update(uint32_t id, const LocalPhotoSync::SyncInfo& stSyncInfo);


    std::string client_info();
private:


    std::atomic<uint32_t> id_{0};
    std::unordered_map<uint32_t, std::weak_ptr<Session>> map_connection;
    LocalPhotoSync::HttpGetClient cache_clients_info;
};
}   // namespace lps
#endif