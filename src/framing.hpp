#ifndef FRAMING_HPP
#define FRAMING_HPP

#include <boost/asio.hpp>
#include <cstdint>
#include <vector>

namespace lps {
// 将4字节长度转换为大端字节序
inline std::vector<uint8_t> encode_length(uint32_t length)
{
    std::vector<uint8_t> buffer(4);
    buffer[0] = (length >> 24) & 0xFF;
    buffer[1] = (length >> 16) & 0xFF;
    buffer[2] = (length >> 8) & 0xFF;
    buffer[3] = length & 0xFF;
    return buffer;
}

// 将大端字节序的4字节转换为长度
inline uint32_t decode_length(const std::vector<uint8_t>& buffer)
{
    if (buffer.size() < 4)
        return 0;
    return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

// 从socket读取指定长度的数据
inline boost::system::error_code read_exact(boost::asio::ip::tcp::socket& socket,
                                            std::vector<uint8_t>& buffer, size_t length)
{
    boost::system::error_code error;
    size_t bytes_read = 0;
    while (bytes_read < length && !error)
    {
        bytes_read += socket.read_some(
            boost::asio::buffer(buffer.data() + bytes_read, length - bytes_read), error);
    }
    return error;
}
}   // namespace lps

#endif   // FRAMING_HPP