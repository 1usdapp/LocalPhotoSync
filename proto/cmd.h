#ifndef _CMD_H_
#define _CMD_H_

#include <stdint.h>

struct PkgHead
{
    uint16_t PackageLen;
    uint8_t  HeadLen;
    uint8_t  Version;
    uint32_t CMDID;
    uint64_t Reserve;
    uint64_t Reserve2;
};

const int kCurHeadLen = 20;




#endif