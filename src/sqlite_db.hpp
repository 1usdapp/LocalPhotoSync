#ifndef SQLITE_DB_HPP
#define SQLITE_DB_HPP

#include <cstdint>
#include <sqlite3.h>
#include <string>

namespace lps {
class SqliteCrcDB
{
private:
    sqlite3* db_;
    std::string db_path_;

public:
    SqliteCrcDB(const std::string& db_path);
    ~SqliteCrcDB();

    // 打开数据库
    bool open();

    // 关闭数据库
    void close();

    // 检查文件是否存在并获取CRC32
    bool get_file_crc(const std::string& filename, uint32_t& crc32);

    // 设置文件CRC32
    bool set_file_crc(const std::string& filename, uint32_t crc32);

    // 删除文件记录
    bool delete_file(const std::string& filename);

    // 创建表
    bool create_table();
};
}   // namespace lps

#endif   // SQLITE_DB_HPP