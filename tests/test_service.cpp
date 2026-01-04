#include "test_common.hpp"

namespace lps {

// ============================================================================
// CConMgr Tests
// ============================================================================
class CConMgrTest : public ::testing::Test
{
protected:
    CConMgr con_mgr_;
};

TEST_F(CConMgrTest, OnGetConId)
{
    uint32_t id1 = con_mgr_.on_get_conid();
    uint32_t id2 = con_mgr_.on_get_conid();
    uint32_t id3 = con_mgr_.on_get_conid();

    // 每次获取的ID应该递增
    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
}

TEST_F(CConMgrTest, OnGetConIdUniqueness)
{
    // 连续获取多个ID，验证唯一性
    std::set<uint32_t> ids;
    for (int i = 0; i < 100; ++i)
    {
        uint32_t id = con_mgr_.on_get_conid();
        EXPECT_EQ(ids.count(id), 0) << "ID " << id << " already exists";
        ids.insert(id);
    }
}

TEST_F(CConMgrTest, ClientInfoReturnsJson)
{
    std::string info = con_mgr_.client_info();
    // Should return valid JSON even when empty
    EXPECT_FALSE(info.empty());
}

TEST_F(CConMgrTest, ClientInfo)
{
    std::string info = con_mgr_.client_info();
    // client_info应该返回一个非空字符串或有效的JSON
    EXPECT_FALSE(info.empty());
}

// ============================================================================
// CServiceContainer Tests
// ============================================================================
class CServiceContainerTest : public ::testing::Test
{
protected:
    CServiceContainer service_container_;
};

TEST_F(CServiceContainerTest, GetConMgr)
{
    auto con_mgr = service_container_.GetConMgr();
    EXPECT_NE(con_mgr, nullptr);
}

TEST_F(CServiceContainerTest, GetConMgrSameInstance)
{
    auto con_mgr1 = service_container_.GetConMgr();
    auto con_mgr2 = service_container_.GetConMgr();
    // 应该返回相同的实例
    EXPECT_EQ(con_mgr1, con_mgr2);
}

TEST_F(CServiceContainerTest, GetConMgrFunctionality)
{
    auto con_mgr = service_container_.GetConMgr();
    EXPECT_NE(con_mgr, nullptr);

    // 验证返回的CConMgr可以使用
    uint32_t id = con_mgr->on_get_conid();
    EXPECT_NE(id, 0);
}

}   // namespace lps
