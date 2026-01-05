#ifndef OBJ_HELPER_HPP
#define OBJ_HELPER_HPP

#include "con_mgr.hpp"
#include "sqlite_db_manager.hpp"
#include "singleton.hpp"
#include <memory>

namespace lps {

class CServiceContainer
{
public:
    std::shared_ptr<CConMgr> GetConMgr();
    std::shared_ptr<SqliteDBManager> GetDBManager();


private:
    std::shared_ptr<CConMgr> con_mgr_;
    std::shared_ptr<SqliteDBManager> db_manager_;
};

using CCServiceContainerInstance = singleton<CServiceContainer>;

}   // namespace lps

#endif
