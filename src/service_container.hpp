#ifndef OBJ_HELPER_HPP
#define OBJ_HELPER_HPP

#include "con_mgr.hpp"
#include "singleton.hpp"
#include <memory>

namespace lps {

class CServiceContainer
{
public:
    std::shared_ptr<CConMgr> GetConMgr();


private:
    std::shared_ptr<CConMgr> con_mgr_;
};

using CCServiceContainerInstance = singleton<CServiceContainer>;

}   // namespace lps

#endif
