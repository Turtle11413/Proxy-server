#ifndef PROXY_SEREVER_PROXY_H_
#define PROXY_SEREVER_PROXY_H_

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <future>
#include <string>
#include <thread>

#define MaxConnections 100

class ProxyServer {
public:
  ProxyServer(const std::string &server_host, const std::string &server_port,
              const std::string &listen_port);

  ~ProxyServer() {
    close(listen_socket_);
    close(server_socket_);
  }

  void Run();
  void ReadConsoleInput();

private:
  void InitSocket(int &socket_);
  void BindSocket(int &socket_, const int &port);
  void ListenSocket(int &socket_);
  void CreateListenSocket(int &socket_, const int &listen_port);

  void ConnectSocket(int &socket_, const std::string &host, const int &port);
  void CreateServerSocket(int &socket_, const std::string &server_host,
                          const int &server_port);

  void CreateClientSocket(int &client_socket);
  void ManageClientSession(std::ofstream &log_stream);
  void ManageClientTraffic(int &client_socket, int &server_socket,
                           std::ofstream &log_stream);
  void ManageClientQuery(int &client_socket, int &server_socket,
                         fd_set &read_fd, std::ofstream &log_stream);
  std::string ExtractClientQuery(char buffer[], ssize_t bytes_received);
  std::string GetClientInfo(const int &client_socket,
                            const std::string &client_query);

  std::future<void> stop_server_;

  int listen_port_;
  int listen_socket_;

  std::string server_host_;
  int server_port_;
  int server_socket_;

  enum class ServerStatus { WORK, STOP };
  ServerStatus server_status_ = ServerStatus::STOP;
};

#endif // PROXY_SEREVER_PROXY_H_
