#ifndef _CMD_H_
#define _CMD_H_

#include <stdint.h>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

// 消息包头，每个字段使用网络字节序
struct PkgHead
{
    uint16_t PackageLen;
    uint8_t  HeadLen;
    uint8_t  Version;
    uint32_t CMDID;
    uint64_t Reserve;
    uint64_t Reserve2;
};

const int kCurHeadLen = 24;

// 网络字节序转换辅助函数
#ifndef ntohll
inline uint64_t ntohll(uint64_t val)
{
    if constexpr (sizeof(uint32_t) == 4 && sizeof(uint64_t) == 8)
    {
        uint32_t high = ntohl(static_cast<uint32_t>(val >> 32));
        uint32_t low = ntohl(static_cast<uint32_t>(val & 0xFFFFFFFFUL));
        return (static_cast<uint64_t>(low) << 32) | high;
    }
    return val;
}
#endif

#ifndef htonll
inline uint64_t htonll(uint64_t val)
{
    if constexpr (sizeof(uint32_t) == 4 && sizeof(uint64_t) == 8)
    {
        uint32_t high = htonl(static_cast<uint32_t>(val >> 32));
        uint32_t low = htonl(static_cast<uint32_t>(val & 0xFFFFFFFFUL));
        return (static_cast<uint64_t>(low) << 32) | high;
    }
    return val;
}
#endif

// 从网络字节序字节流中解析PkgHead
inline bool parse_pkg_head(const uint8_t* buffer, size_t len, PkgHead& head)
{
    if (buffer == nullptr || len < kCurHeadLen)
    {
        return false;
    }

    // 按网络字节序读取各字段
    size_t offset = 0;
    
    // PackageLen (2字节)
    uint16_t pkg_len;
    memcpy(&pkg_len, buffer + offset, sizeof(uint16_t));
    head.PackageLen = ntohs(pkg_len);
    offset += sizeof(uint16_t);
    
    // HeadLen (1字节) - 单字节无需转换
    head.HeadLen = buffer[offset];
    offset += sizeof(uint8_t);
    
    // Version (1字节) - 单字节无需转换
    head.Version = buffer[offset];
    offset += sizeof(uint8_t);
    
    // CMDID (4字节)
    uint32_t cmd_id;
    memcpy(&cmd_id, buffer + offset, sizeof(uint32_t));
    head.CMDID = ntohl(cmd_id);
    offset += sizeof(uint32_t);
    
    // Reserve (8字节)
    uint64_t reserve;
    memcpy(&reserve, buffer + offset, sizeof(uint64_t));
    head.Reserve = ntohll(reserve);
    offset += sizeof(uint64_t);
    
    // Reserve2 (8字节)
    uint64_t reserve2;
    memcpy(&reserve2, buffer + offset, sizeof(uint64_t));
    head.Reserve2 = ntohll(reserve2);
    
    return true;
}

// 将PkgHead编码为网络字节序字节流
inline bool encode_pkg_head(const PkgHead& head, uint8_t* buffer, size_t len)
{
    if (buffer == nullptr || len != kCurHeadLen)
    {
        return false;
    }

    size_t offset = 0;
    
    // PackageLen (2字节)
    uint16_t pkg_len = htons(head.PackageLen);
    memcpy(buffer + offset, &pkg_len, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    
    // HeadLen (1字节) - 单字节无需转换
    buffer[offset] = head.HeadLen;
    offset += sizeof(uint8_t);
    
    // Version (1字节) - 单字节无需转换
    buffer[offset] = head.Version;
    offset += sizeof(uint8_t);
    
    // CMDID (4字节)
    uint32_t cmd_id = htonl(head.CMDID);
    memcpy(buffer + offset, &cmd_id, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    
    // Reserve (8字节)
    uint64_t reserve = htonll(head.Reserve);
    memcpy(buffer + offset, &reserve, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    
    // Reserve2 (8字节)
    uint64_t reserve2 = htonll(head.Reserve2);
    memcpy(buffer + offset, &reserve2, sizeof(uint64_t));
    
    return true;
}

#endif