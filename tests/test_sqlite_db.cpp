#include "test_common.hpp"

namespace lps {

// ============================================================================
// SqliteCrcDB Tests
// ============================================================================
class SqliteCrcDBTest : public ::testing::Test
{
protected:
    std::string db_file_;

    void SetUp() override
    {
        db_file_ = "/tmp/test_crc.db";
        // 删除旧的测试数据库
        std::remove(db_file_.c_str());
    }

    void TearDown() override
    {
        // 清理测试数据库
        std::remove(db_file_.c_str());
    }
};

TEST_F(SqliteCrcDBTest, OpenDatabase)
{
    SqliteCrcDB db(db_file_);
    EXPECT_TRUE(db.open());
    db.close();
}

TEST_F(SqliteCrcDBTest, CreateTable)
{
    SqliteCrcDB db(db_file_);
    db.open();
    // 表应该被创建
    EXPECT_TRUE(db.open());
    db.close();
}

TEST_F(SqliteCrcDBTest, CloseDisallowsOperations)
{
    SqliteCrcDB db(db_file_);
    db.open();
    db.close();

    uint32_t crc_value = 0;
    EXPECT_FALSE(db.get_file_crc("file", crc_value));
    EXPECT_FALSE(db.set_file_crc("file", 1));
}

TEST_F(SqliteCrcDBTest, OpenInvalidPath)
{
    // likely not writable
    SqliteCrcDB db("/root/forbidden/index.db");
    EXPECT_FALSE(db.open());
}

TEST_F(SqliteCrcDBTest, SetAndGetFileCrc)
{
    SqliteCrcDB db(db_file_);
    db.open();

    std::string filename = "test_photo.jpg";
    uint32_t crc_value = 0x12345678;

    // 设置CRC32
    EXPECT_TRUE(db.set_file_crc(filename, crc_value));

    // 获取CRC32
    uint32_t retrieved_crc = 0;
    EXPECT_TRUE(db.get_file_crc(filename, retrieved_crc));
    EXPECT_EQ(retrieved_crc, crc_value);

    db.close();
}

TEST_F(SqliteCrcDBTest, GetNonexistentFile)
{
    SqliteCrcDB db(db_file_);
    db.open();

    std::string filename = "nonexistent_file.jpg";
    uint32_t crc_value = 0;

    // 获取不存在的文件应该返回false
    EXPECT_FALSE(db.get_file_crc(filename, crc_value));

    db.close();
}

TEST_F(SqliteCrcDBTest, UpdateFileCrc)
{
    SqliteCrcDB db(db_file_);
    db.open();

    std::string filename = "update_test.jpg";
    uint32_t crc_value1 = 0x11111111;
    uint32_t crc_value2 = 0x22222222;

    // 设置初始CRC32
    EXPECT_TRUE(db.set_file_crc(filename, crc_value1));

    uint32_t retrieved_crc = 0;
    EXPECT_TRUE(db.get_file_crc(filename, retrieved_crc));
    EXPECT_EQ(retrieved_crc, crc_value1);

    // 更新CRC32
    EXPECT_TRUE(db.set_file_crc(filename, crc_value2));

    retrieved_crc = 0;
    EXPECT_TRUE(db.get_file_crc(filename, retrieved_crc));
    EXPECT_EQ(retrieved_crc, crc_value2);

    db.close();
}

TEST_F(SqliteCrcDBTest, DeleteNonexistentFile)
{
    SqliteCrcDB db(db_file_);
    db.open();
    EXPECT_TRUE(db.delete_file("missing.jpg"));
}

TEST_F(SqliteCrcDBTest, DeleteFile)
{
    SqliteCrcDB db(db_file_);
    db.open();

    std::string filename = "delete_test.jpg";
    uint32_t crc_value = 0xAAAAAAAA;

    // 设置CRC32
    EXPECT_TRUE(db.set_file_crc(filename, crc_value));

    // 验证存在
    uint32_t retrieved_crc = 0;
    EXPECT_TRUE(db.get_file_crc(filename, retrieved_crc));
    EXPECT_EQ(retrieved_crc, crc_value);

    // 删除文件
    EXPECT_TRUE(db.delete_file(filename));

    // 验证已删除
    retrieved_crc = 0;
    EXPECT_FALSE(db.get_file_crc(filename, retrieved_crc));

    db.close();
}

TEST_F(SqliteCrcDBTest, MultipleFiles)
{
    SqliteCrcDB db(db_file_);
    db.open();

    // 添加多个文件
    std::vector<std::pair<std::string, uint32_t>> files = {
        {"photo1.jpg", 0x11111111},
        {"photo2.jpg", 0x22222222},
        {"photo3.jpg", 0x33333333},
        {"photo4.jpg", 0x44444444},
    };

    for (const auto& [filename, crc] : files)
    {
        EXPECT_TRUE(db.set_file_crc(filename, crc));
    }

    // 验证所有文件
    for (const auto& [filename, expected_crc] : files)
    {
        uint32_t retrieved_crc = 0;
        EXPECT_TRUE(db.get_file_crc(filename, retrieved_crc));
        EXPECT_EQ(retrieved_crc, expected_crc);
    }

    db.close();
}

TEST_F(SqliteCrcDBTest, ClosedDatabaseOperations)
{
    SqliteCrcDB db(db_file_);
    db.open();
    db.close();

    // 在关闭的数据库上操作应该失败
    std::string filename = "test.jpg";
    uint32_t crc_value = 0x12345678;
    EXPECT_FALSE(db.set_file_crc(filename, crc_value));

    uint32_t retrieved_crc = 0;
    EXPECT_FALSE(db.get_file_crc(filename, retrieved_crc));
}

}   // namespace lps
