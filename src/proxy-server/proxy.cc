#include "proxy.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

ProxyServer::ProxyServer(const std::string &listen_port,
                         const std::string &server_host,
                         const std::string &server_port) {
  listen_port_ = std::stoi(listen_port);
  server_host_ = server_host;
  server_port_ = std::stoi(server_port);

  CreateListenSocket();
  CreateServerSocket();
}

void ProxyServer::InitSocket(int &socket_) {
  // AF_INET for IPv4, SOCK_STREAM for TCP
  if ((socket_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    close(socket_);
    throw std::runtime_error("Socket creating error");
  }
}

void ProxyServer::BindSocket(int &socket_, const int &port) {
  struct sockaddr_in sock_addr;
  memset(&sock_addr, 0, sizeof(sock_addr));

  sock_addr.sin_family = AF_INET;
  sock_addr.sin_addr.s_addr = INADDR_ANY;
  sock_addr.sin_port = htons(port);

  if (bind(socket_, reinterpret_cast<sockaddr *>(&sock_addr),
           sizeof(sock_addr)) < 0) {
    close(socket_);
    throw std::runtime_error("Bind socket error");
  }
}

void ProxyServer::ListenSocket(int &socket_) {
  if (listen(socket_, kMaxConnections) < 0) {
    close(socket_);
    throw std::runtime_error("Error listen socket");
  }
}

void ProxyServer::CreateListenSocket() {
  try {
    InitSocket(listen_socket_);
    BindSocket(listen_socket_, listen_port_);
    ListenSocket(listen_socket_);
    std::cout << "Proxy server started on port " << listen_port_ << std::endl;
  } catch (std::exception &e) {
    close(listen_socket_);
    std::cout << "Exception throwed: " << e.what() << std::endl;
  }
}

void ProxyServer::ConnectSocket(int &socket_, const std::string &host,
                                const int &port) {
  struct sockaddr_in sock_addr;

  sock_addr.sin_family = AF_INET;
  sock_addr.sin_addr.s_addr = inet_addr(host.c_str());
  sock_addr.sin_port = htons(port);

  if (connect(socket_, reinterpret_cast<sockaddr *>(&sock_addr),
              sizeof(sock_addr)) < 0) {
    close(socket_);
    std::cerr << "Error code: " << errno << std::endl;
    std::cerr << "Error message: " << strerror(errno) << std::endl;

    throw std::runtime_error("Connecting error: server socket\n");
  }
}

void ProxyServer::CreateServerSocket() {
  try {
    InitSocket(server_socket_);
    std::cout << "Connecting to server: " << server_host_ << ":" << server_port_
              << std::endl;
    ConnectSocket(server_socket_, server_host_, server_port_);
    std::cout << "Successfull\n";
  } catch (std::exception &e) {
    close(listen_socket_);
    close(server_socket_);
    std::cout << "Exception throwed : " << e.what() << std::endl;
  }
}

void ProxyServer::ReadConsoleInput() {
  while (server_status_ == ServerStatus::WORK) {
    std::cout << "Enter \\stop to stop server\n";

    std::string input;
    std::getline(std::cin, input);

    if (input == "\\stop") {
      server_status_ = ServerStatus::STOP;
      std::cout << "Server is stopped\n";
      exit(0);
    }
  }
}

void ProxyServer::Run() {
  std::ofstream log_stream("logfile.log", std::ios::app);
  server_status_ = ServerStatus::WORK;

  console_input_thread_ = std::thread(&ProxyServer::ReadConsoleInput, this);

  for (; server_status_ == ServerStatus::WORK;) {
    HandleClient(log_stream);
  }

  log_stream.close();
  console_input_thread_.join();
}

void ProxyServer::CreateClientSocket(int &client_socket) {

  sockaddr_in client_addr;
  socklen_t client_addr_size = sizeof(client_addr);
  memset(&client_addr, 0, client_addr_size);

  client_socket =
      accept(listen_socket_, reinterpret_cast<sockaddr *>(&client_addr),
             &client_addr_size);

  if (client_socket < 0) {
    close(client_socket);
    throw std::runtime_error("Accepting client error");
  }

  std::cout << "Accepted connection from " << inet_ntoa(client_addr.sin_addr)
            << ":" << ntohs(client_addr.sin_port) << std::endl;
}

void ProxyServer::HandleClient([[maybe_unused]] std::ofstream &log_stream) {
  int client_socket = 0;

  try {
    CreateClientSocket(client_socket);
  } catch (std::exception &e) {
    std::cout << "Exception throwed: " << e.what() << std::endl;
  }

  if (fork() == 0) {
    close(listen_socket_);

    ConnectClientWithServer(client_socket);

    std::cout << "Connecting closed\n";
    exit(0);
  }
}

void ProxyServer::ConnectClientWithServer(int &client_socket) {
  int server_socket = 0;
  try {
    InitSocket(server_socket);

    std::cout << "Connecting to server: " << server_host_ << ":" << server_port_
              << std::endl;
    ConnectSocket(server_socket, server_host_, server_port_);
    std::cout << "Successfull\n";

    // shutdown(client_socket, SHUT_RDWR);
    close(server_socket);
    std::cout << "Disconnect to server\n";
  } catch (std::exception &e) {
    close(client_socket);
    close(server_socket);
    std::cerr << "Error code: " << errno << std::endl;
    std::cerr << "Error message: " << strerror(errno) << std::endl;
    std::cout << "Exception throwed: " << e.what() << std::endl;
  }
}
