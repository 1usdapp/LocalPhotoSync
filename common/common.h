#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <glog/logging.h>
#include <memory>
#include <google/protobuf/message.h>

int64_t GetID();

int64_t GetUniqueID();

void InitGlog(const char* argv0, int log_level);

bool IsIP(const std::string & domain);

bool IsEmail(const std::string &email);

uint32_t Ip2Int(std::string ip);

std::string Int2Ip(uint32_t ip);

std::shared_ptr<std::string> PacketMsg(int cmd, const google::protobuf::Message& msg);


#endif