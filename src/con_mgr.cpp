#include "con_mgr.hpp"
#include "google/protobuf/util/json_util.h"
#include <type_traits>
namespace lps {

// SFINAE 检测器
template<typename T, typename = void>
struct has_always_print_primitive_fields : std::false_type {};

template<typename T>
struct has_always_print_primitive_fields<T, 
    std::void_t<decltype(std::declval<T>().always_print_primitive_fields)>> 
    : std::true_type {};

template<typename T, typename = void>
struct has_always_print_fields_with_no_presence : std::false_type {};

template<typename T>
struct has_always_print_fields_with_no_presence<T,
    std::void_t<decltype(std::declval<T>().always_print_fields_with_no_presence)>>
    : std::true_type {};

// 使用函数重载而不是 if constexpr
template<typename OptionsType>
void SetOptionalField(OptionsType& options, std::true_type /* has_always_print_primitive_fields */) {
    options.always_print_primitive_fields = true;
}

template<typename OptionsType>
void SetOptionalField(OptionsType& options, std::false_type /* has_always_print_primitive_fields */) {
    // 不设置该字段
    (void)options;
}

template<typename OptionsType>
void SetOptionalField2(OptionsType& options, std::true_type /* has_always_print_fields_with_no_presence */) {
    options.always_print_fields_with_no_presence = true;
}

template<typename OptionsType>
void SetOptionalField2(OptionsType& options, std::false_type /* has_always_print_fields_with_no_presence */) {
    // 不设置该字段
    (void)options;
}

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

    // 使用 SFINAE 函数重载
    SetOptionalField(options, has_always_print_primitive_fields<google::protobuf::util::JsonPrintOptions>{});
    SetOptionalField2(options, has_always_print_fields_with_no_presence<google::protobuf::util::JsonPrintOptions>{});

    auto status =
        google::protobuf::util::MessageToJsonString(cache_clients_info, &json_string, options);

    if (status.ok())
    {
        // std::cout << "JSON Output:\n" << json_string << std::endl;
    }
    else
    {
        std::cerr << "Error converting to JSON: " << status.ToString() << std::endl;
    }

    return json_string;
}

}   // namespace lps