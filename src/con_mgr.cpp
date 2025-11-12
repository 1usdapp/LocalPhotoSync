#include "con_mgr.hpp"
#include "google/protobuf/util/json_util.h"

namespace lps {

uint32_t CConMgr::on_get_conid()
{
    ++id_;
    while (map_connection.count(id_) > 0)
    {
        id_++;
    }
    return id_;
}

int CConMgr::on_connected(uint32_t id, std::weak_ptr<Session> psession)
{
    auto it = map_connection.find(id);
    if (it != map_connection.end())
    {
        if (!psession.expired())
        {
            psession.lock()->close_stream();
        }
        std::cerr << "on_connected double id:" << id << std::endl;
        return 0;
    }

    map_connection[id] = psession;
    auto session = psession.lock();
    auto& stClientInfo = (*cache_clients_info.mutable_data())[id];
    stClientInfo.set_address(session->address());
    stClientInfo.set_id(id);
    return 0;
}

int CConMgr::on_closed(uint32_t id)
{
    map_connection.erase(id);
    cache_clients_info.mutable_data()->erase(id);
    return 0;
}

int CConMgr::on_sync_info_update(uint32_t id, const LocalPhotoSync::SyncInfo& stSyncInfo)
{
    auto it = map_connection.find(id);
    if (it == map_connection.end())
    {
        std::cerr << "on_connected double id:" << id << std::endl;
        return 0;
    }

    auto& stClientInfo = (*cache_clients_info.mutable_data())[id];
    stClientInfo.set_photonum(stSyncInfo.photonum());
    stClientInfo.set_curnum(stSyncInfo.curnum());
    return 0;
}

std::string CConMgr::client_info()
{
    // 转换为 JSON
    std::string json_string;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;                   // 添加缩进和换行
    options.preserve_proto_field_names = true;       // 保持原始字段名（默认转为驼峰）
    options.always_print_enums_as_ints = true;       // 枚举是否总是输出为整数
    options.always_print_primitive_fields = true;   // 是否打印默认值

    auto status =
        google::protobuf::util::MessageToJsonString(cache_clients_info, &json_string, options);

    if (status.ok())
    {
        std::cout << "JSON Output:\n" << json_string << std::endl;
    }
    else
    {
        std::cerr << "Error converting to JSON: " << status.ToString() << std::endl;
    }

    return json_string;
}

}   // namespace lps