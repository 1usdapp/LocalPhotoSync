#include "common.h"
#include "stdarg.h"
#include "unistd.h"
#include <sys/time.h>
#include <atomic>
#include <regex>
#include <fmt/format.h>
#include "msg.h"

using namespace std;
using namespace google;




int64_t generateStamp()
{
	timeval tv;
	gettimeofday(&tv, 0);
	return (int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000;
}

int64_t GetID()
{
	static int64_t id = 1;
	return id++;
}

int64_t GetUniqueID()
{
	//Snowflake 第1位不适用  41位时间戳    10位机器数   12位自增

	static int64_t last_time = 0;
	static atomic<int16_t> index{0};
	if ( last_time == 0 )
	{
		last_time = generateStamp();
	}

	int64_t now_time = generateStamp();

	if ( now_time != last_time )
	{
		index = 0;
	}
	else
	{
		index++;
	}

	return ((now_time << 22) + index)&(0x7FFFFFFFFFFFFFFF);
}

void InitGlog(const char* argv0, int log_level)
{
	InitGoogleLogging(argv0);

	FLAGS_log_dir = "/opt/log/";
	FLAGS_logbufsecs = 0;
	FLAGS_max_log_size = 100;
	FLAGS_minloglevel = log_level;
	FLAGS_stop_logging_if_full_disk = true;

	google::SetLogDestination(google::INFO, fmt::format("/opt/log/info_{}", argv0).data());
	google::SetLogDestination(google::WARNING, fmt::format("/opt/log/warn_{}", argv0).data());
	google::SetLogDestination(google::ERROR, fmt::format("/opt/log/error_{}", argv0).data());
	google::SetLogDestination(google::FATAL, fmt::format("/opt/log/fatal_{}", argv0).data());
}

bool IsIP(const std::string & domain)
{
	regex reg(R"((25[0-5]|2[0-4]\d|[0-1]\d{2}|[1-9]?\d)\.(25[0-5]|2[0-4]\d|[0-1]\d{2}|[1-9]?\d)\.(25[0-5]|2[0-4]\d|[0-1]\d{2}|[1-9]?\d)\.(25[0-5]|2[0-4]\d|[0-1]\d{2}|[1-9]?\d))");
	return regex_match(domain, reg);
}

bool IsEmail(const std::string &email)
{
	if ( email.length() >= 255)
	{
		return false;
	}
	regex reg(R"([\w\-]+\@[\w\-]+\.[\w\-]+)");
	return regex_match(email, reg);
}

uint32_t Ip2Int(std::string ip)
{
	string tmp;
	uint32_t ret = 0;
	int cnt = 0;
	for (auto ele : ip)
	{
		if (ele == '.')
		{
			cnt++;
			if (cnt > 3)
			{
				return 0;
			}
			uint32_t t = stol(tmp);
			if (t > 255)
			{
				return 0;
			}
			ret = (ret << 8) + t;
			tmp.clear();
			continue;
		}
		tmp.push_back(ele);
	}
	if (cnt != 3)
	{
		return 0;
	}
	uint32_t t = stol(tmp);
	if (t > 255)
	{
		return 0;
	}
	ret = (ret << 8) + t;
	tmp.clear();
	return ret;
}

std::string Int2Ip(uint32_t ip)
{
	int a0 = (ip & 0xFF000000) >> 24;
	int a1 = (ip & 0x00FF0000) >> 16;
	int a2 = (ip & 0x0000FF00) >> 8;
	int a3 = (ip & 0x000000FF);
	return to_string(a0) + "." + to_string(a1) + "." + to_string(a2) + "." + to_string(a3);
}

std::shared_ptr<std::string> PacketMsg(int cmd, const google::protobuf::Message& msg)
{
	auto pstring = make_shared<string>();
	MSG_HEAD head;
	head.cmd = cmd;
	head.length = msg.ByteSizeLong();
	pstring->append((char*)&head, sizeof(MSG_HEAD));
	msg.AppendToString(pstring.get());
	return pstring;
}
