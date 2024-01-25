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

ProxyServer::ProxyServer(const std::string& listen_port,
                         const std::string& server_host,
                         const std::string& server_port) {
  listen_port_ = std::stoi(listen_port);
  server_host_ = server_host;
  server_port_ = std::stoi(server_port);

  InitListenSocket();
  InitServerSocket();
}

void ProxyServer::InitListenSocket() {
  // AF_INET for IPv4, SOCK_STREAM for TCP
  if ((listen_socket_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    throw std::runtime_error("listen socket creating error");
  }

  BindListenSocket();
}

void ProxyServer::BindListenSocket() {
  struct sockaddr_in listen_addr;

  listen_addr.sin_family = AF_INET;
  listen_addr.sin_addr.s_addr = INADDR_ANY;
  listen_addr.sin_port = htons(listen_port_);

  if (bind(listen_socket_, reinterpret_cast<sockaddr*>(&listen_addr),
           sizeof(listen_addr)) < 0) {
    close(listen_socket_);
    throw std::runtime_error("Error binding listen socket");
  }

  if (listen(listen_socket_, kMaxConnections) < 0) {
    throw std::runtime_error("Error listening on listen socket");
    close(listen_socket_);
  }

  std::cout << "Proxy server started on port " << listen_port_ << std::endl;
}

void ProxyServer::InitServerSocket() {
  if ((server_socket_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    close(listen_socket_);
    throw std::runtime_error("server socket creating error");
  }

  BindServerSocket();
}

void ProxyServer::BindServerSocket() {
  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(server_host_.c_str());
  server_addr.sin_port = htons(server_port_);

  std::cout << "Connecting to server: " << server_host_ << ":" << server_port_
            << std::endl;

  if (connect(server_socket_, reinterpret_cast<sockaddr*>(&server_addr),
              sizeof(server_addr)) < 0) {
    std::cerr << "Error code: " << errno << std::endl;
    std::cerr << "Error message: " << strerror(errno) << std::endl;

    close(listen_socket_);
    close(server_socket_);
    throw std::runtime_error("Connecting error: server socket\n");
  } else {
    std::cout << "Successfull\n";
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
    }
  }
}

void ProxyServer::Run() {
  std::ofstream log_stream("logfile.log", std::ios::app);
  server_status_ = ServerStatus::WORK;

  console_input_thread_ = std::thread(&ProxyServer::ReadConsoleInput, this);

  while (server_status_ == ServerStatus::WORK) {
    HandleClient(log_stream);
  }

  log_stream.close();
  console_input_thread_.join();
}

void ProxyServer::HandleClient([[maybe_unused]] std::ofstream& log_stream) {
  sockaddr_in client_addr;
  socklen_t client_addr_size = sizeof(client_addr);
  int client_socket =
      accept(listen_socket_, reinterpret_cast<sockaddr*>(&client_addr),
             &client_addr_size);

  if (client_socket < 0) {
    throw std::runtime_error("Accepting client error");
    return;
  }

  std::cout << "Accepted connection from " << inet_ntoa(client_addr.sin_addr)
            << ":" << ntohs(client_addr.sin_port) << std::endl;

  if (fork() == 0) {
    close(listen_socket_);

    HandleClientLogic(client_socket, log_stream);

    std::cout << "Connecting closed\n";
    exit(0);
  }
}

void ProxyServer::HandleClientLogic(
    int client_socket, [[maybe_unused]] std::ofstream& log_stream) {
  char buffer[100];
  memset(buffer, 0, sizeof(buffer));

  int backend_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (backend_socket < -1) {
    close(client_socket);
    throw std::runtime_error("Error creating backend socket");
  }

  sockaddr_in backend_addr{};
  backend_addr.sin_family = AF_INET;
  backend_addr.sin_addr.s_addr = inet_addr(server_host_.c_str());
  backend_addr.sin_port = htons(server_port_);

  std::cout << "Connecting to backend: " << server_host_ << ":" << server_port_
            << std::endl;

  if (connect(server_socket_, reinterpret_cast<sockaddr*>(&backend_addr),
              sizeof(backend_addr)) < 0) {
    std::cerr << "Error code: " << errno << std::endl;
    std::cerr << "Error message: " << strerror(errno) << std::endl;
    close(client_socket);
    close(server_socket_);
    throw std::runtime_error("Error connecting to backend");
  } else {
    std::cout << "Successfull\n";
    close(server_socket_);
    std::cout << "Disconnect to server\n";
    return;
  }
}
