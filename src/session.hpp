#ifndef SESSION_HPP
#define SESSION_HPP

#include "../proto/cmd.h"
#include "../proto/csmsg.pb.h"
#include "../proto/msgid.pb.h"
#include "config.hpp"
#include "crc.hpp"
#include "sqlite_db.hpp"
#include <boost/asio.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace lps {
class Session : public std::enable_shared_from_this<Session>
{
public:
    class ITCPConEvent
    {
    public:
        virtual ~ITCPConEvent(){}
        virtual uint32_t on_get_conid() = 0;
        virtual int on_connected(uint32_t id, std::weak_ptr<Session> psession) = 0;
        virtual int on_closed(uint32_t id) = 0;
        virtual int on_sync_info_update(uint32_t id,
                                        const LocalPhotoSync::SyncInfo& stSyncInfo) = 0;
    };

private:
    boost::asio::ip::tcp::socket socket_;
    ServerConfig config_;
    SqliteCrcDB db_;
    std::string device_id_;
    std::string save_path_;
    std::string full_save_path_;
    std::vector<uint8_t> buffer_;
    std::vector<uint8_t> head_buffer_;   // 用于存储包头(20字节)
    PkgHead pkg_head_;                   // 解析后的包头
    bool device_info_received_;
    std::unordered_map<std::string, std::shared_ptr<std::fstream>> map_path_file_;
    std::string address_;

    std::shared_ptr<ITCPConEvent> pevent_;
    uint32_t id_{0};

public:
    Session(boost::asio::ip::tcp::socket socket, const ServerConfig& config,
            std::shared_ptr<ITCPConEvent> event);
    ~Session();

    void start();

    void close_stream();
    
    const std::string& address();
private:
    void do_start();
    void close();
    void handle_read_head(const boost::system::error_code& error, size_t bytes_transferred);
    void handle_read_data(const boost::system::error_code& error, size_t bytes_transferred);
    void send_response(const std::vector<uint8_t>& response_data);

    void handle_device_info_request(const LocalPhotoSync::CSReqDeviceInfo& stMsg);
    void handle_sync_photo_request(const LocalPhotoSync::CSReqSyncPhoto& stMsg);
    void process_file_data(const std::string& filename, uint64_t offset,
                           const std::vector<uint8_t>& data, bool has_next_pkt);
    bool create_directories(const std::string& path);
    std::string get_full_path(const std::string& filename);
    bool check_file_crc(const std::string& filename, uint32_t expected_crc);
    bool update_file_crc(const std::string& filename, uint32_t crc32);
    void send_device_info_response();
    void send_sync_photo_response(int32_t result_id);

    std::shared_ptr<std::fstream> make_or_get_file_handle(const std::string& file_path,
                                                          bool new_file);
};
}   // namespace lps

#endif   // SESSION_HPP