#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdlib>
#include <string>

struct ServerConfig
{
    std::string tcp_port;
    std::string udp_port;
    std::string name;
    std::string root;
    std::string os;
    std::string broadcast;

    ServerConfig()
    {
        tcp_port  = std::getenv("LPS_TCP_PORT") ? std::getenv("LPS_TCP_PORT") : "9176";
        udp_port  = std::getenv("LPS_UDP_PORT") ? std::getenv("LPS_UDP_PORT") : "9176";
        name      = std::getenv("LPS_NAME") ? std::getenv("LPS_NAME") : "LPS-Server";
        root      = std::getenv("LPS_ROOT") ? std::getenv("LPS_ROOT") : "./data";
        os        = std::getenv("LPS_OS") ? std::getenv("LPS_OS") : "linux";
        broadcast = std::getenv("LPS_BROADCAST") ? std::getenv("LPS_BROADCAST") : "255.255.255.255";
    }
};

#endif   // CONFIG_HPP