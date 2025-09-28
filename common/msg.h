#ifndef MSG_H
#define MSG_H

#include <stdint.h>
#include <memory>
#include <string>


struct MSG_HEAD
{
	int32_t length;			//不包含头长度
	int32_t cmd;
};



struct MsgInfo 
{
	int64_t conid;
	MSG_HEAD head;
	std::shared_ptr<std::string> pstr;
};





#endif