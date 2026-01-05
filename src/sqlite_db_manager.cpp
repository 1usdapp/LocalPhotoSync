#include "sqlite_db_manager.hpp"

namespace lps {

SqliteDBManager::SqliteDBManager()
{
}

SqliteDBManager::~SqliteDBManager()
{
    close_all();
}

bool SqliteDBManager::acquire_db(const std::string& db_path)
{
    // 检查是否已缓存
    auto it = db_cache_.find(db_path);
    if (it != db_cache_.end()) {
        // DB已缓存，递增引用计数
        ref_counts_[db_path]++;
        return true;
    }

    // 首次访问，创建新DB实例
    auto db = std::make_shared<SqliteCrcDB>(db_path);
    if (!db->open()) {
        // 打开失败，不缓存
        return false;
    }

    // 缓存并初始化引用计数
    db_cache_[db_path] = db;
    ref_counts_[db_path] = 1;
    return true;
}

void SqliteDBManager::release_db(const std::string& db_path)
{
    auto it = db_cache_.find(db_path);
    if (it == db_cache_.end()) {
        // DB未缓存，直接返回（错误情况但不处理）
        return;
    }

    // 递减引用计数
    int& count = ref_counts_[db_path];
    count--;

    // 如果计数达到0，关闭并从缓存删除
    if (count <= 0) {
        it->second->close();
        db_cache_.erase(it);
        ref_counts_.erase(db_path);
    }
}

void SqliteDBManager::close_all()
{
    for (auto& pair : db_cache_) {
        pair.second->close();
    }
    db_cache_.clear();
    ref_counts_.clear();
}

bool SqliteDBManager::get_file_crc(const std::string& db_path, const std::string& filename,
                                    uint32_t& crc32)
{
    auto it = db_cache_.find(db_path);
    if (it == db_cache_.end()) {
        // DB未缓存，返回失败
        return false;
    }
    return it->second->get_file_crc(filename, crc32);
}

bool SqliteDBManager::set_file_crc(const std::string& db_path, const std::string& filename,
                                    uint32_t crc32)
{
    auto it = db_cache_.find(db_path);
    if (it == db_cache_.end()) {
        // DB未缓存，返回失败
        return false;
    }
    return it->second->set_file_crc(filename, crc32);
}

bool SqliteDBManager::delete_file(const std::string& db_path, const std::string& filename)
{
    auto it = db_cache_.find(db_path);
    if (it == db_cache_.end()) {
        // DB未缓存，返回失败
        return false;
    }
    return it->second->delete_file(filename);
}

}   // namespace lps
