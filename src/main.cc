#include <iostream>

#include "proxy-server/proxy.h"

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "Invalid input. Usage: " << argv[0]
              << "<server_host> <server_port> <listen_port>\n";
    return 1;
  }

  ProxyServer server(argv[1], argv[2], argv[3]);

  std::thread console_input_thread =
      std::thread(&ProxyServer::ReadConsoleInput, &server);

  server.Run();

  console_input_thread.join();

  return 0;
}
