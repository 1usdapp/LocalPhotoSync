#ifndef MSG_H
#define MSG_H

#include <stdint.h>
#include <memory>
#include <string>



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

struct MsgInfo 
{
	int64_t conid;
	PkgHead head;
	std::shared_ptr<std::string> pstr;
};





#endif