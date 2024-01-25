#include <gtest/gtest.h>
#include <unistd.h>

#include "../proxy-server/proxy.h"

class ProxyServerTest : public ::testing::Test {
  const std::string listen_port = "4568";
  const std::string server_host = "127.0.0.1";
  const std::string server_port = "5432";

 public:
  explicit ProxyServerTest()
      : proxyServer_(ProxyServer(listen_port, server_host, server_port)) {}

 protected:
  ProxyServer proxyServer_;
};

TEST_F(ProxyServerTest, SocketCreation) {
  EXPECT_NO_THROW(proxyServer_.InitSockets());
  EXPECT_GE(proxyServer_.GetListenSocket(), 0);
}

TEST_F(ProxyServerTest, SocketBinding) {
  EXPECT_NO_THROW(proxyServer_.InitSockets());
  EXPECT_EQ(proxyServer_.GetListenPort(), 4568);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
