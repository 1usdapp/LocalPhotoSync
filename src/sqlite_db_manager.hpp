#ifndef SQLITE_DB_MANAGER_HPP
#define SQLITE_DB_MANAGER_HPP

#include "sqlite_db.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace lps {

/**
 * @brief SqliteDBManager - 管理所有SQLite数据库连接
 * 
 * 职责：
 * - 为每个唯一目录路径缓存一个SqliteCrcDB实例
 * - 实现引用计数，当无Session引用时关闭DB
 * - 提供便利方法访问DB操作（get_file_crc, set_file_crc等）
 * 
 * 设计理由：单线程Boost.Asio事件循环自然序列化操作，无需互斥锁。
 */
class SqliteDBManager
{
public:
    SqliteDBManager();
    ~SqliteDBManager();

    /**
     * @brief 获取指定路径的数据库连接
     * 首次调用时延迟加载并缓存DB；后续调用复用缓存
     * @param db_path 数据库文件路径
     * @return 成功返回true，失败返回false
     */
    bool acquire_db(const std::string& db_path);

    /**
     * @brief 释放指定路径的数据库连接
     * 递减引用计数；计数为0时关闭DB并从缓存删除
     * @param db_path 数据库文件路径
     */
    void release_db(const std::string& db_path);

    /**
     * @brief 关闭所有缓存的数据库连接
     * 通常在Manager析构或服务器关闭时调用
     */
    void close_all();

    // ========== 便利方法 ==========
    // 这些方法委托给对应目录的缓存DB实例

    /**
     * @brief 从指定DB获取文件CRC32
     */
    bool get_file_crc(const std::string& db_path, const std::string& filename,
                      uint32_t& crc32);

    /**
     * @brief 在指定DB中设置文件CRC32
     */
    bool set_file_crc(const std::string& db_path, const std::string& filename,
                      uint32_t crc32);

    /**
     * @brief 从指定DB删除文件记录
     */
    bool delete_file(const std::string& db_path, const std::string& filename);

private:
    // 缓存映射：db_path -> SqliteCrcDB实例
    std::map<std::string, std::shared_ptr<SqliteCrcDB>> db_cache_;

    // 引用计数映射：db_path -> 引用计数
    std::map<std::string, int> ref_counts_;
};

}   // namespace lps

#endif   // SQLITE_DB_MANAGER_HPP
