#include "sqlite_db.hpp"
#include <iostream>
#include <sys/stat.h>
#ifdef _WIN32
    #include <io.h>
    #include <direct.h>
#else
    #include <unistd.h>
#endif

namespace lps {
SqliteCrcDB::SqliteCrcDB(const std::string& db_path) : db_(nullptr), db_path_(db_path) {}

SqliteCrcDB::~SqliteCrcDB()
{
    close();
}

bool SqliteCrcDB::open()
{
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }
    return create_table();
}

void SqliteCrcDB::close()
{
    if (db_)
    {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool SqliteCrcDB::get_file_crc(const std::string& filename, uint32_t& crc32)
{
    if (!db_)
        return false;

    sqlite3_stmt* stmt;
    std::string sql = "SELECT crc32 FROM photos WHERE filename = ?";

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        crc32 = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return true;
    }

    sqlite3_finalize(stmt);
    return false;
}

bool SqliteCrcDB::set_file_crc(const std::string& filename, uint32_t crc32)
{
    if (!db_)
        return false;

    sqlite3_stmt* stmt;
    std::string sql = "INSERT OR REPLACE INTO photos (filename, crc32) VALUES (?, ?)";

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, crc32);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool SqliteCrcDB::delete_file(const std::string& filename)
{
    if (!db_)
        return false;

    sqlite3_stmt* stmt;
    std::string sql = "DELETE FROM photos WHERE filename = ?";

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool SqliteCrcDB::create_table()
{
    if (!db_)
        return false;

    const char* sql = "CREATE TABLE IF NOT EXISTS photos ("
                      "filename TEXT PRIMARY KEY, "
                      "crc32 INTEGER)";

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}
}   // namespace lps