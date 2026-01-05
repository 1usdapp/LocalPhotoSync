#include "session.hpp"
#include "../proto/cmd.h"
#include <boost/bind/bind.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>

namespace lps {
Session::Session(boost::asio::ip::tcp::socket socket, const ServerConfig& config,
                 std::shared_ptr<ITCPConEvent> event,
                 std::shared_ptr<SqliteDBManager> db_manager)
    : socket_(std::move(socket)), config_(config), db_manager_(db_manager), db_path_(""),
      buffer_(65536), head_buffer_(kCurHeadLen), device_info_received_(false), pevent_(event)
{   // 64KB buffer，包头缓冲区为20字节
    address_ = std::format(
        "{}:{}", socket_.remote_endpoint().address().to_string(), socket_.remote_endpoint().port());
}

Session::~Session()
{
    try
    {
        // 释放DB引用计数
        if (!db_path_.empty() && db_manager_)
        {
            db_manager_->release_db(db_path_);
        }

        if (socket_.is_open())
        {
            socket_.close();
        }
    }
    catch (...)
    {}
}

void Session::start()
{

    if (pevent_)
    {
        id_ = pevent_->on_get_conid();
        pevent_->on_connected(id_, shared_from_this());
    }

    do_start();
}

void Session::do_start()
{
    // 开始读取包头（20字节）
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(head_buffer_),
                            [this, self](boost::system::error_code ec, std::size_t length) {
                                handle_read_head(ec, length);
                            });
}

void Session::close_stream()
{
    if (socket_.is_open())
    {
        socket_.close();
    }
}

const std::string& Session::address()
{
    return address_;
}

void Session::close()
{
    if (pevent_)
    {
        pevent_->on_closed(id_);
    }
}

void Session::handle_read_head(const boost::system::error_code& error, size_t bytes_transferred)
{
    (void)bytes_transferred;   // 参数未使用，避免警告

    if (error)
    {
        close();
        std::cerr << "Read head error: " << error.message() << std::endl;
        return;
    }

    // 解析包头（自动处理网络字节序转换）
    if (!parse_pkg_head(head_buffer_.data(), head_buffer_.size(), pkg_head_))
    {
        close();
        std::cerr << "Failed to parse package head" << std::endl;
        return;
    }

    // 验证包头
    if (pkg_head_.HeadLen != kCurHeadLen)
    {
        close();
        std::cerr << "Invalid head length: " << static_cast<int>(pkg_head_.HeadLen) << std::endl;
        return;
    }

    // 计算消息体长度（总长度 - 包头长度）
    if (pkg_head_.PackageLen < kCurHeadLen)
    {
        close();
        std::cerr << "Invalid package length: " << pkg_head_.PackageLen << std::endl;
        return;
    }

    uint32_t msg_length = pkg_head_.PackageLen - kCurHeadLen;

    if (msg_length > 100 * 1024 * 1024)
    {   // 最大100MB
        close();
        std::cerr << "Message too large: " << msg_length << std::endl;
        return;
    }

    // 如果消息体为空，直接处理
    if (msg_length == 0)
    {
        handle_read_data(error, 0);
        return;
    }

    // 调整buffer大小
    if (buffer_.size() < msg_length)
    {
        buffer_.resize(msg_length);
    }

    // 读取消息体
    auto self(shared_from_this());
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(buffer_.data(), msg_length),
        [this, self, msg_length](boost::system::error_code ec, std::size_t length) {
            if (!ec && length == msg_length)
            {
                handle_read_data(ec, length);
            }
            else
            {
                close();
                std::cout << "Read data error: " << ec.message() << std::endl;
            }
        });
}

void Session::handle_read_data(const boost::system::error_code& error, size_t bytes_transferred)
{
    if (error)
    {
        close();
        std::cout << "Read data error: " << error.message() << std::endl;
        return;
    }

    try
    {
        // 解析protobuf消息
        LocalPhotoSync::MsgPkg msg_pkg;
        if (!msg_pkg.ParseFromArray((const void*)buffer_.data(), bytes_transferred))
        {
            close();
            std::cerr << "Failed to parse MsgPkg" << std::endl;
            return;
        }

        // 根据消息类型处理
        if (msg_pkg.has_csreqdeviceinfo())
        {
            handle_device_info_request(msg_pkg.csreqdeviceinfo());
        }
        else if (msg_pkg.has_csreqsyncphoto())
        {
            handle_sync_photo_request(msg_pkg.csreqsyncphoto());
        }
        else
        {
            close();
            std::cerr << "Unknown message type" << std::endl;
            return;
        }
    }
    catch (std::exception& e)
    {
        close();
        std::cerr << "Handle data exception: " << e.what() << std::endl;
        return;
    }

    // 继续读取下一个消息
    do_start();
}

void Session::handle_device_info_request(const LocalPhotoSync::CSReqDeviceInfo& stMsg)
{
    std::cout << "handle_device_info_request begin" << std::endl;


    const auto& req = stMsg;
    device_id_ = req.deviceid();
    save_path_ = req.path();

    // remove begin char /
    while (!save_path_.empty() && *save_path_.begin() == '/')
    {
        save_path_.erase(save_path_.cbegin());
    }

    // 构建完整保存路径
    full_save_path_ = (std::filesystem::path(config_.root) / save_path_).string();

    // 创建目录
    if (!create_directories(full_save_path_))
    {
        std::cerr << "Failed to create directory: " << full_save_path_ << std::endl;
        send_device_info_response();
        return;
    }

    // 打开或创建数据库
    db_path_ = full_save_path_ + "/index.db";
    if (!db_manager_->acquire_db(db_path_))
    {
        std::cerr << "Failed to acquire database: " << db_path_ << std::endl;
        send_device_info_response();
        return;
    }

    device_info_received_ = true;
    std::cout << "Device registered: " << device_id_ << ", Path: " << full_save_path_ << std::endl;

    send_device_info_response();
}

void Session::handle_sync_photo_request(const LocalPhotoSync::CSReqSyncPhoto& stMsg)
{
    std::cout << "handle_sync_photo_request begin" << std::endl;
    if (!device_info_received_)
    {
        std::cerr << "handle_sync_photo_request device_info_received_ = false" << std::endl;
        send_sync_photo_response(-1);
        return;
    }

    const auto& req = stMsg;
    std::string filename = req.filename();
    uint64_t offset = req.offset();
    // uint64_t    size         = req.size();
    bool has_next_pkt = req.hasnextpkt();
    uint32_t client_crc = req.crc32();

    // 将protobuf bytes转为vector
    std::vector<uint8_t> data(req.data().begin(), req.data().end());

    // 获取完整文件路径
    std::string file_path = get_full_path(filename);

    // 如果是单包且完整文件，检查是否需要写入
    if (offset == 0)
    {
        uint32_t existing_crc = 0;
        if (db_manager_->get_file_crc(db_path_, filename, existing_crc) &&
            std::filesystem::exists(file_path))
        {
            if (existing_crc == client_crc)
            {
                // CRC一致，跳过写入
                std::cout << "File " << filename << " already exists with same CRC, skipping"
                          << std::endl;
                send_sync_photo_response(0);
                return;
            }
        }
    }



    std::cout << "begin to write File :" << file_path << "  offset:" << offset << std::endl;

    try
    {
        // 打开文件进行写入
        std::shared_ptr<std::fstream> pfile = make_or_get_file_handle(file_path, offset == 0);

        if (!pfile->is_open())
        {
            std::cerr << "Failed to open file: " << file_path << std::endl;
            send_sync_photo_response(-3);
            return;
        }

        // 定位到指定偏移
        pfile->seekp(offset);

        // 写入数据
        pfile->write(reinterpret_cast<const char*>(data.data()), data.size());


        // 如果是最后一片，计算整文件CRC并更新数据库
        if (!has_next_pkt)
        {
            uint32_t file_crc = lps::crc32_file(file_path);
            if (!db_manager_->set_file_crc(db_path_, filename, file_crc))
            {
                std::cerr << "Failed to update file CRC in database" << std::endl;
            }
            std::cout << "File completed: " << filename << ", CRC32: 0x" << std::hex << file_crc
                      << std::dec << std::endl;

            pfile->close();
            map_path_file_.erase(file_path);
        }

        send_sync_photo_response(0);
    }
    catch (std::exception& e)
    {
        std::cout << "File write exception: " << e.what() << std::endl;
        send_sync_photo_response(-4);
    }

    if (pevent_)
    {
        pevent_->on_sync_info_update(id_, req.syncinfo());
    }
}

void Session::send_device_info_response()
{
    LocalPhotoSync::MsgPkg response;
    response.set_resultid(device_info_received_ ? 0 : -1);
    response.set_serialid(0);

    auto* res = response.mutable_csresdeviceinfo();
    (void)res;   // 避免未使用警告

    std::string serialized;
    response.SerializeToString(&serialized);

    // 构建包头
    PkgHead head;
    head.PackageLen = kCurHeadLen + serialized.size();
    head.HeadLen = kCurHeadLen;
    head.Version = 1;
    head.CMDID = LocalPhotoSync::ID_CSResDeviceInfo;   // 根据实际协议设置CMDID
    head.Reserve = 0;
    head.Reserve2 = 0;

    // 编码包头为网络字节序
    std::vector<uint8_t> head_buffer(kCurHeadLen);
    if (!encode_pkg_head(head, head_buffer.data(), head_buffer.size()))
    {
        std::cerr << "Failed to encode package head" << std::endl;
        return;
    }

    // 组装完整响应（包头 + 消息体）
    std::vector<uint8_t> full_response;
    full_response.insert(full_response.end(), head_buffer.begin(), head_buffer.end());
    full_response.insert(full_response.end(), serialized.begin(), serialized.end());

    send_response(full_response);
}

void Session::send_sync_photo_response(int32_t result_id)
{
    LocalPhotoSync::MsgPkg response;
    response.set_resultid(result_id);
    response.set_serialid(0);

    auto* res = response.mutable_csressyncphoto();
    (void)res;   // 避免未使用警告

    std::string serialized;
    response.SerializeToString(&serialized);

    // 构建包头
    PkgHead head;
    head.PackageLen = kCurHeadLen + serialized.size();
    head.HeadLen = kCurHeadLen;
    head.Version = 1;
    head.CMDID = LocalPhotoSync::ID_CSResSyncPhoto;   // 根据实际协议设置CMDID
    head.Reserve = 0;
    head.Reserve2 = 0;

    // 编码包头为网络字节序
    std::vector<uint8_t> head_buffer(kCurHeadLen);
    if (!encode_pkg_head(head, head_buffer.data(), head_buffer.size()))
    {
        std::cerr << "Failed to encode package head" << std::endl;
        return;
    }

    // 组装完整响应（包头 + 消息体）
    std::vector<uint8_t> full_response;
    full_response.insert(full_response.end(), head_buffer.begin(), head_buffer.end());
    full_response.insert(full_response.end(), serialized.begin(), serialized.end());

    send_response(full_response);
}

void Session::send_response(const std::vector<uint8_t>& response_data)
{
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
                             boost::asio::buffer(response_data),
                             [self](boost::system::error_code ec, std::size_t /*length*/) {
                                 if (ec)
                                 {
                                     std::cout << "Write error: " << ec.message() << std::endl;
                                 }
                             });
}

bool Session::create_directories(const std::string& path)
{
    try
    {
        std::filesystem::create_directories(path);
        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << "Create directory error: " << e.what() << std::endl;
        return false;
    }
}

std::string Session::get_full_path(const std::string& filename)
{
    std::filesystem::path p(full_save_path_);
    p /= filename;
    return p.string();
}

bool Session::check_file_crc(const std::string& filename, uint32_t expected_crc)
{
    uint32_t file_crc = lps::crc32_file(get_full_path(filename));
    return file_crc == expected_crc;
}

bool Session::update_file_crc(const std::string& filename, uint32_t crc32)
{
    return db_manager_->set_file_crc(db_path_, filename, crc32);
}

std::shared_ptr<std::fstream> Session::make_or_get_file_handle(const std::string& file_path,
                                                               bool new_file)
{
    // 打开文件进行写入
    std::shared_ptr<std::fstream> pfile;
    auto it = map_path_file_.find(file_path);
    if (it != map_path_file_.end())
    {
        pfile = it->second;
    }
    else
    {
        pfile = std::make_shared<std::fstream>();
        map_path_file_[file_path] = pfile;
        if (new_file)
        {
            // 新文件或覆盖
            pfile->open(file_path, std::ios::binary | std::ios::out | std::ios::trunc);
        }
        else
        {
            // 追加写入
            pfile->open(file_path, std::ios::binary | std::ios::in | std::ios::out);
        }
    }
    return pfile;
}
}   // namespace lps