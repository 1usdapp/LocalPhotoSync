#include "test_common.hpp"
#include "../src/sqlite_db_manager.hpp"
#include <filesystem>

namespace lps {

// ============================================================================
// SqliteDBManager Tests
// ============================================================================
class SqliteDBManagerTest : public ::testing::Test
{
protected:
    std::string db_path_1_;
    std::string db_path_2_;
    std::unique_ptr<SqliteDBManager> manager_;

    void SetUp() override
    {
        db_path_1_ = "/tmp/test_manager_1.db";
        db_path_2_ = "/tmp/test_manager_2.db";

        // 删除旧的测试数据库
        std::filesystem::remove(db_path_1_);
        std::filesystem::remove(db_path_2_);

        // 创建Manager实例
        manager_ = std::make_unique<SqliteDBManager>();
    }

    void TearDown() override
    {
        // Manager析构会关闭所有DB
        manager_.reset();

        // 清理测试数据库
        std::filesystem::remove(db_path_1_);
        std::filesystem::remove(db_path_2_);
    }
};

// 测试1：Manager初始化时缓存为空
TEST_F(SqliteDBManagerTest, ManagerInitializesWithEmptyCache)
{
    // 无法直接测试私有成员，但可以通过尝试访问不存在的DB来验证
    uint32_t crc = 0;
    EXPECT_FALSE(manager_->get_file_crc(db_path_1_, "file.txt", crc));
}

// 测试2：相同目录的首次调用返回同一DB实例
TEST_F(SqliteDBManagerTest, SameDirectoryReturnsSameInstance)
{
    // 第一次acquire
    EXPECT_TRUE(manager_->acquire_db(db_path_1_));

    // 设置一个文件CRC
    EXPECT_TRUE(manager_->set_file_crc(db_path_1_, "photo1.jpg", 0xAAAA));

    // 第二次acquire（不同session）
    EXPECT_TRUE(manager_->acquire_db(db_path_1_));

    // 应该能读到之前设置的CRC（说明是同一个DB实例）
    uint32_t crc = 0;
    EXPECT_TRUE(manager_->get_file_crc(db_path_1_, "photo1.jpg", crc));
    EXPECT_EQ(crc, 0xAAAA);

    // 清理（两个acquire对应两个release）
    manager_->release_db(db_path_1_);
    manager_->release_db(db_path_1_);
}

// 测试3：不同目录有不同的DB实例
TEST_F(SqliteDBManagerTest, DifferentDirectoriesHaveDifferentInstances)
{
    // 分别获取两个不同目录的DB
    EXPECT_TRUE(manager_->acquire_db(db_path_1_));
    EXPECT_TRUE(manager_->acquire_db(db_path_2_));

    // 在第一个DB设置CRC
    EXPECT_TRUE(manager_->set_file_crc(db_path_1_, "photo.jpg", 0xAAAA));

    // 在第二个DB查询（不应该存在）
    uint32_t crc = 0;
    EXPECT_FALSE(manager_->get_file_crc(db_path_2_, "photo.jpg", crc));

    // 在第二个DB设置不同的CRC
    EXPECT_TRUE(manager_->set_file_crc(db_path_2_, "photo.jpg", 0xBBBB));

    // 验证第一个DB的CRC未变
    crc = 0;
    EXPECT_TRUE(manager_->get_file_crc(db_path_1_, "photo.jpg", crc));
    EXPECT_EQ(crc, 0xAAAA);

    // 清理
    manager_->release_db(db_path_1_);
    manager_->release_db(db_path_2_);
}

// 测试4：引用计数在无Session时关闭DB
TEST_F(SqliteDBManagerTest, RefCountClosesDBWhenZero)
{
    // 第一个Session获取DB
    EXPECT_TRUE(manager_->acquire_db(db_path_1_));
    EXPECT_TRUE(manager_->set_file_crc(db_path_1_, "photo.jpg", 0xAAAA));

    // 第二个Session获取同一DB
    EXPECT_TRUE(manager_->acquire_db(db_path_1_));

    // 第一个Session释放
    manager_->release_db(db_path_1_);

    // DB仍应该被缓存（因为第二个Session还持有）
    uint32_t crc = 0;
    EXPECT_TRUE(manager_->get_file_crc(db_path_1_, "photo.jpg", crc));
    EXPECT_EQ(crc, 0xAAAA);

    // 第二个Session释放
    manager_->release_db(db_path_1_);

    // 现在DB应该被关闭，无法访问
    EXPECT_FALSE(manager_->get_file_crc(db_path_1_, "photo.jpg", crc));
}

// 测试5：同一目录的多个操作被序列化
TEST_F(SqliteDBManagerTest, MultipleOperationsOnSameDB)
{
    EXPECT_TRUE(manager_->acquire_db(db_path_1_));

    // 连续多个写入
    EXPECT_TRUE(manager_->set_file_crc(db_path_1_, "photo1.jpg", 0x1111));
    EXPECT_TRUE(manager_->set_file_crc(db_path_1_, "photo2.jpg", 0x2222));
    EXPECT_TRUE(manager_->set_file_crc(db_path_1_, "photo3.jpg", 0x3333));

    // 验证所有写入成功
    uint32_t crc = 0;
    EXPECT_TRUE(manager_->get_file_crc(db_path_1_, "photo1.jpg", crc));
    EXPECT_EQ(crc, 0x1111);

    EXPECT_TRUE(manager_->get_file_crc(db_path_1_, "photo2.jpg", crc));
    EXPECT_EQ(crc, 0x2222);

    EXPECT_TRUE(manager_->get_file_crc(db_path_1_, "photo3.jpg", crc));
    EXPECT_EQ(crc, 0x3333);

    // 删除文件
    EXPECT_TRUE(manager_->delete_file(db_path_1_, "photo2.jpg"));

    // 验证删除成功
    crc = 0;
    EXPECT_FALSE(manager_->get_file_crc(db_path_1_, "photo2.jpg", crc));

    manager_->release_db(db_path_1_);
}

// 测试6：close_all正确关闭所有DB
TEST_F(SqliteDBManagerTest, CloseAllClosesAllDatabases)
{
    EXPECT_TRUE(manager_->acquire_db(db_path_1_));
    EXPECT_TRUE(manager_->acquire_db(db_path_2_));

    EXPECT_TRUE(manager_->set_file_crc(db_path_1_, "photo.jpg", 0xAAAA));
    EXPECT_TRUE(manager_->set_file_crc(db_path_2_, "photo.jpg", 0xBBBB));

    // 关闭所有DB
    manager_->close_all();

    // 两个DB都应该无法访问
    uint32_t crc = 0;
    EXPECT_FALSE(manager_->get_file_crc(db_path_1_, "photo.jpg", crc));
    EXPECT_FALSE(manager_->get_file_crc(db_path_2_, "photo.jpg", crc));
}

}   // namespace lps
