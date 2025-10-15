#ifndef CRC32_H
#define CRC32_H
#include <stdint.h>

#include <string>

uint32_t CRC32(const std::string& str);

uint32_t CRC32(unsigned char* buf, int nLength);


#endif
