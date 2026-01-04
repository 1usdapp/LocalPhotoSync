#include "test_common.hpp"

namespace lps {

// ============================================================================
// Config Tests
// ============================================================================
TEST(ConfigTest, DefaultValues)
{
    ServerConfig cfg;
    EXPECT_EQ(cfg.tcp_port, "9176");
    EXPECT_EQ(cfg.udp_port, "9176");
    EXPECT_EQ(cfg.http_port, "9175");
    EXPECT_EQ(cfg.name, "LPS-Server");
    EXPECT_EQ(cfg.root, "./data");
    EXPECT_EQ(cfg.os, "linux");
    EXPECT_EQ(cfg.broadcast, "255.255.255.255");
}

TEST(ConfigTest, EnvOverride)
{
    setenv("LPS_TCP_PORT", "12345", 1);
    setenv("LPS_UDP_PORT", "22345", 1);
    setenv("LPS_HTTP_PORT", "32345", 1);
    setenv("LPS_NAME", "Custom", 1);
    setenv("LPS_ROOT", "/tmp/root", 1);
    setenv("LPS_OS", "testos", 1);
    setenv("LPS_BROADCAST", "1.2.3.4", 1);

    ServerConfig cfg;
    EXPECT_EQ(cfg.tcp_port, "12345");
    EXPECT_EQ(cfg.udp_port, "22345");
    EXPECT_EQ(cfg.http_port, "32345");
    EXPECT_EQ(cfg.name, "Custom");
    EXPECT_EQ(cfg.root, "/tmp/root");
    EXPECT_EQ(cfg.os, "testos");
    EXPECT_EQ(cfg.broadcast, "1.2.3.4");
}

}   // namespace lps
