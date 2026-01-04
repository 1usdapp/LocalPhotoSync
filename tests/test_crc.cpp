#include "test_common.hpp"

namespace lps {

// ============================================================================
// CRC32 Tests
// ============================================================================
class CRC32Test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        init_crc32_table();
    }
};

TEST_F(CRC32Test, InitTableCreatedSuccessfully)
{
    // CRC表应该被初始化
    // 检查表中至少有一些非零值
    bool has_nonzero = false;
    for (int i = 0; i < 256; i++)
    {
        if (crc32_table[i] != 0)
        {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
}

TEST_F(CRC32Test, CRC32EmptyData)
{
    std::vector<uint8_t> data;
    uint32_t result = crc32(data);
    // 对于空数据，CRC32 = 初始值 ^ 最终异或值 = 0xFFFFFFFF ^ 0xFFFFFFFF = 0
    EXPECT_EQ(result, 0);
}

TEST_F(CRC32Test, CRC32SingleByte)
{
    std::vector<uint8_t> data = {0x42};
    uint32_t result = crc32(data);
    // 验证单字节计算结果一致性
    EXPECT_NE(result, 0);
    EXPECT_NE(result, 0xFFFFFFFF);
}

TEST_F(CRC32Test, CRC32MultipleBytes)
{
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint32_t result1 = crc32(data);
    uint32_t result2 = crc32(data);
    // 相同的数据应该产生相同的CRC32
    EXPECT_EQ(result1, result2);
}

TEST_F(CRC32Test, CRC32WithOffset)
{
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint32_t result1 = crc32(data, 0, 4);
    uint32_t result2 = crc32(data, 4, 4);
    // 不同偏移应该产生不同结果
    EXPECT_NE(result1, result2);
}

TEST_F(CRC32Test, CRC32FullVsPartial)
{
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    uint32_t full = crc32(data);
    uint32_t partial = crc32(data, 0, 4);
    // 完整数据和部分数据应该相同
    EXPECT_EQ(full, partial);
}

TEST_F(CRC32Test, CRC32DifferentData)
{
    std::vector<uint8_t> data1 = {0x01, 0x02, 0x03, 0x04};
    std::vector<uint8_t> data2 = {0x05, 0x06, 0x07, 0x08};
    uint32_t crc1 = crc32(data1);
    uint32_t crc2 = crc32(data2);
    // 不同数据应该产生不同的CRC32
    EXPECT_NE(crc1, crc2);
}

TEST_F(CRC32Test, CRC32FileWithTestFile)
{
    // 创建临时测试文件
    const char* test_file = "/tmp/test_crc32_file.bin";
    std::ofstream out(test_file, std::ios::binary);
    std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04, 0x05};
    out.write(reinterpret_cast<const char*>(test_data.data()), test_data.size());
    out.close();

    uint32_t crc = crc32_file(test_file);
    // 文件CRC32应该非零
    EXPECT_NE(crc, 0);
    EXPECT_NE(crc, 0xFFFFFFFF);

    // 清理
    std::remove(test_file);
}

TEST_F(CRC32Test, CRC32FileNonexistent)
{
    const char* nonexistent_file = "/tmp/nonexistent_file_12345.bin";
    uint32_t crc = crc32_file(nonexistent_file);
    // 不存在的文件应该返回0
    EXPECT_EQ(crc, 0);
}

TEST_F(CRC32Test, CRC32LargeData)
{
    // 创建大数据块
    std::vector<uint8_t> large_data(10000);
    for (size_t i = 0; i < large_data.size(); ++i)
    {
        large_data[i] = (uint8_t)(i % 256);
    }

    uint32_t crc = crc32(large_data);
    // 大数据块的CRC32应该是有效的
    EXPECT_NE(crc, 0);
    EXPECT_NE(crc, 0xFFFFFFFF);
}

}   // namespace lps
