#include "TcpListener.hpp"
#include "TcpConnection.hpp"

#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

TcpListener::TcpListener(unsigned short port, int backlog) {
  this->nativeHandle =
      socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (this->nativeHandle < 0) {
    throw std::runtime_error("socket: " + std::string(std::strerror(errno)));
  }

  int reuseAddress = 1;
  if (setsockopt(this->nativeHandle, SOL_SOCKET, SO_REUSEADDR, &reuseAddress,
                 sizeof(reuseAddress)) < 0) {
    const std::string message = std::strerror(errno);
    close(this->nativeHandle);
    this->nativeHandle = -1;
    throw std::runtime_error("setsockopt(SO_REUSEADDR): " + message);
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);

  if (bind(this->nativeHandle, reinterpret_cast<sockaddr *>(&address),
           sizeof(address)) < 0) {
    const std::string message = std::strerror(errno);
    close(this->nativeHandle);
    this->nativeHandle = -1;
    throw std::runtime_error("bind: " + message);
  }
  if (listen(this->nativeHandle, backlog) < 0) {
    const std::string message = std::strerror(errno);
    close(this->nativeHandle);
    this->nativeHandle = -1;
    throw std::runtime_error("listen: " + message);
  }
}

TcpListener::~TcpListener() {
  if (this->nativeHandle >= 0) {
    close(this->nativeHandle);
  }
}

std::unique_ptr<IConnection> TcpListener::accept() {
  const int clientHandle = accept4(this->nativeHandle, nullptr, nullptr,
                                   SOCK_CLOEXEC | SOCK_NONBLOCK);
  if (clientHandle < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
      return nullptr;
    }

    throw std::runtime_error("accept: " + std::string(std::strerror(errno)));
  }
  return std::make_unique<TcpConnection>(clientHandle);
}

int TcpListener::getNativeHandle() const noexcept { return this->nativeHandle; }
