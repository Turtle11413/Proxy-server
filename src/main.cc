#include <iostream>

#include "proxy-server/proxy.h"

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "Invalid input. Usage: " << argv[0]
              << " <listen_port> <server_host> <server_port>\n";
    return 1;
  }

  ProxyServer server(argv[1], argv[2], argv[3]);
  server.Run();

  return 0;
}
