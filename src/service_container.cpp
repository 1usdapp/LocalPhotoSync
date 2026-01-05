#include "service_container.hpp"
#include "con_mgr.hpp"
#include <memory>


namespace lps {

std::shared_ptr<CConMgr> CServiceContainer::GetConMgr()
{
    if (!con_mgr_)
    {
        con_mgr_ = std::make_shared<CConMgr>();
    }
    return con_mgr_;
}

std::shared_ptr<SqliteDBManager> CServiceContainer::GetDBManager()
{
    if (!db_manager_)
    {
        db_manager_ = std::make_shared<SqliteDBManager>();
    }
    return db_manager_;
}

}   // namespace lps