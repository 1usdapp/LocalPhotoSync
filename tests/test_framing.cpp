#include "test_common.hpp"

namespace lps {

// ============================================================================
// Framing Tests
// ============================================================================
TEST(FramingTest, EncodeDecodeLength)
{
    uint32_t len = 0xAABBCCDD;
    auto buf = encode_length(len);
    EXPECT_EQ(decode_length(buf), len);
}

TEST(FramingTest, DecodeTooShort)
{
    std::vector<uint8_t> buf = {0x00, 0x01, 0x02};
    EXPECT_EQ(decode_length(buf), 0u);
}

TEST(FramingTest, ReadExactReadsAll)
{
    boost::asio::io_context ioc;
    boost::asio::ip::tcp::acceptor acceptor(ioc, {boost::asio::ip::tcp::v4(), 0});
    auto port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket server(ioc);
    boost::asio::ip::tcp::socket client(ioc);

    client.connect({boost::asio::ip::address_v4::loopback(), port});
    acceptor.accept(server);

    std::vector<uint8_t> payload = {1, 2, 3, 4, 5, 6, 7, 8};
    boost::asio::write(server, boost::asio::buffer(payload));

    std::vector<uint8_t> buffer(payload.size());
    auto ec = read_exact(client, buffer, buffer.size());
    EXPECT_FALSE(ec);
    EXPECT_EQ(buffer, payload);
}

// ============================================================================
// Singleton Tests
// ============================================================================
namespace {
struct Counted
{
    static int constructed;
    Counted() { ++constructed; }
};
int Counted::constructed = 0;
}   // namespace

TEST(SingletonTest, SingleConstruction)
{
    auto& a = singleton<Counted>::instance();
    auto& b = singleton<Counted>::instance();
    (void)a;
    (void)b;
    EXPECT_EQ(Counted::constructed, 1);
    EXPECT_EQ(&a, &b);
}

}   // namespace lps
