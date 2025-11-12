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

}   // namespace lps