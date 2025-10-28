#ifndef CRC_HPP
#define CRC_HPP

#include <cstdint>
#include <vector>

namespace lps {
// CRC32多项式
constexpr uint32_t CRC32_POLYNOMIAL = 0xEDB88320;

// 预计算的CRC32表
extern uint32_t crc32_table[256];

// 初始化CRC32表
void init_crc32_table();

// 计算字节数据的CRC32
uint32_t crc32(const std::vector<uint8_t>& data);

// 计算字节数据的CRC32（从指定偏移开始）
uint32_t crc32(const std::vector<uint8_t>& data, size_t offset, size_t length);

// 计算文件的CRC32
uint32_t crc32_file(const std::string& filename);
}   // namespace lps

#endif   // CRC_HPP