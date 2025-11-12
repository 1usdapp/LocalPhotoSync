#include "crc.hpp"
#include <fstream>
#include <iostream>

namespace lps {
// 预计算的CRC32表
uint32_t crc32_table[256];

// 初始化CRC32表
void init_crc32_table()
{
    for (int i = 0; i < 256; i++)
    {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            }
            else
            {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
}

// 计算字节数据的CRC32
uint32_t crc32(const std::vector<uint8_t>& data)
{
    return crc32(data, 0, data.size());
}

// 计算字节数据的CRC32（从指定偏移开始）
uint32_t crc32(const std::vector<uint8_t>& data, size_t offset, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = offset; i < offset + length; i++)
    {
        crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ data[i]];
    }
    return crc ^ 0xFFFFFFFF;
}

// 计算文件的CRC32
uint32_t crc32_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        return 0;
    }

    std::vector<uint8_t> buffer(4096);
    uint32_t crc = 0xFFFFFFFF;

    while (file.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || file.gcount() > 0)
    {
        size_t bytes_read = file.gcount();
        for (size_t i = 0; i < bytes_read; i++)
        {
            crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ buffer[i]];
        }
    }

    file.close();
    return crc ^ 0xFFFFFFFF;
}
}   // namespace lps