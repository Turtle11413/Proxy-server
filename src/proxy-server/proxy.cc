#include "proxy.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

ProxyServer::ProxyServer(const std::string &server_host,
                         const std::string &server_port,
                         const std::string &listen_port) {
  server_host_ = server_host;
  server_port_ = std::stoi(server_port);
  listen_port_ = std::stoi(listen_port);

  CreateListenSocket(listen_socket_, listen_port_);
  CreateServerSocket(server_socket_, server_host_, server_port_);
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
  if (listen(socket_, MaxConnections) < 0) {
    close(socket_);
    throw std::runtime_error("Listen socket error");
  }
}

void ProxyServer::CreateListenSocket(int &socket_, const int &listen_port) {
  try {
    InitSocket(socket_);
    BindSocket(socket_, listen_port);
    ListenSocket(socket_);
    std::cout << "Proxy server started on port " << listen_port << std::endl;
  } catch (std::exception &e) {
    close(socket_);
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

    throw std::runtime_error("Connecting error: server socket\n");
  }
}

void ProxyServer::CreateServerSocket(int &socket_, const std::string &host,
                                     const int &port) {
  try {
    InitSocket(socket_);
    ConnectSocket(socket_, host, port);
  } catch (std::exception &e) {
    close(socket_);
    std::cout << "Exception throwed : " << e.what() << std::endl;
  }
}

void ProxyServer::ReadConsoleInput() {

  stop_server_ = std::async(std::launch::async, [this]() {
    std::string input;

    while (input != "\\stop") {
      std::cout << "\t>>>Enter \\stop to stop server\n";
      std::cin >> input;
    }

    server_status_ = ServerStatus::STOP;
    std::cout << "Server is stopping\n";
  });
}

void ProxyServer::Run() {
  std::ofstream logStream("logfile.log", std::ios::app);
  server_status_ = ServerStatus::WORK;

  while (server_status_ == ServerStatus::WORK) {
    ManageClientSession(logStream);
  }

  logStream.close();
}

void ProxyServer::ManageClientSession(std::ofstream &logStream) {
  int client_socket = -1;
  int server_socket = -1;

  try {
    CreateClientSocket(client_socket);
    CreateServerSocket(server_socket, server_host_, server_port_);
    std::cout << "New client connected\n";
  } catch (std::exception &e) {
    std::cout << "Exception throwed: " << e.what() << std::endl;
  }

  if (fork() == 0) {
    close(listen_socket_);

    ManageClientTraffic(client_socket, server_socket, logStream);

    close(client_socket);
    close(server_socket);
    std::cout << "Connecting closed\n";
    exit(0);
  }
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

  std::cout << "New connection from client: " << inet_ntoa(client_addr.sin_addr)
            << ":" << ntohs(client_addr.sin_port) << std::endl;
}

void ProxyServer::ManageClientTraffic(int &client_socket, int &server_socket,
                                      std::ofstream &logStream) {

  fd_set fds;
  for (; server_status_ == ServerStatus::WORK;) {
    FD_ZERO(&fds);
    FD_SET(client_socket, &fds);
    FD_SET(server_socket, &fds);

    int searched_fd = std::max(client_socket, server_socket);
    if (select(searched_fd + 1, &fds, nullptr, nullptr, nullptr) == -1) {
      perror("Select fd error");
      break;
    }

    try {
      ManageClientQuery(client_socket, server_socket, fds, logStream);
    } catch (std::exception &e) {
      std::cout << "Exception throwed: " << e.what() << std::endl;
      break;
    }
  }
}

void ProxyServer::ManageClientQuery(int &client_socket, int &server_socket,
                                    fd_set &fds, std::ofstream &logStream) {
  char buffer[1000];
  memset(buffer, 0, sizeof(buffer));

  if (FD_ISSET(client_socket, &fds)) {
    ssize_t received_query = recv(client_socket, buffer, sizeof(buffer), 0);

    if (received_query <= 0) {
      throw std::runtime_error("Receiving from client error");
    }

    std::string client_query = ExtractClientQuery(buffer, received_query);

    std::vector<std::string> search_strings = {"select", "insert", "update",
                                               "delete"};
    std::string log_info = GetClientInfo(client_socket, client_query);

    for (auto keyword : search_strings) {
      if (client_query.find(keyword) != std::string::npos) {
        std::cout << log_info;
        logStream << log_info;
        logStream.flush();
        break;
      }
    }

    send(server_socket, buffer, static_cast<size_t>(received_query), 0);
    memset(buffer, 0, sizeof(buffer));
  }

  if (FD_ISSET(server_socket, &fds)) {
    ssize_t bytes_received = recv(server_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
      throw std::runtime_error("Receiving from server error");
    }

    send(client_socket, buffer, static_cast<size_t>(bytes_received), 0);
    memset(buffer, 0, sizeof(buffer));
  }
}

std::string ProxyServer::ExtractClientQuery(char buffer[],
                                            ssize_t bytes_received) {

  std::string client_query(buffer, bytes_received);

  for (auto it = client_query.begin(); it != client_query.end();) {
    if (!std::isprint(*it) || *it == '\n') {
      it = client_query.erase(it);
    } else {
      ++it;
    }
  }

  std::string lower_query;
  std::transform(client_query.begin(), client_query.end(),
                 std::back_inserter(lower_query),
                 [](unsigned char c) { return std::tolower(c); });

  if (lower_query.find("q(") == 0)
    lower_query.erase(0, 2);
  if (lower_query.find("q") == 0)
    lower_query.erase(0, 1);

  return lower_query;
}

std::string ProxyServer::GetClientInfo(const int &client_socket,
                                       const std::string &client_query) {
  struct sockaddr_in client_addr;
  socklen_t client_addr_size = sizeof(client_addr);
  memset(&client_addr, 0, client_addr_size);

  getpeername(client_socket, reinterpret_cast<struct sockaddr *>(&client_addr),
              &client_addr_size);
  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

  auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  std::ostringstream output;
  output << "\nReceived from client: " << client_ip << ":"
         << ntohs(client_addr.sin_port)
         << "\nReceived time: " << std::ctime(&now) << "Client's query:\n"
         << client_query << "\n\n";

  return output.str();
}
