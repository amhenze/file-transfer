#include "TcpConnection.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

TcpConnection::TcpConnection(int nativeHandle) noexcept
    : nativeHandle(nativeHandle) {}

TcpConnection::~TcpConnection() { closeConnection(); }

IoResult TcpConnection::write(const void *data, std::size_t size) {
  while (true) {
    const ssize_t bytes = send(this->nativeHandle, data, size, MSG_NOSIGNAL);
    if (bytes > 0) {
      return {IoStatus::Progress, static_cast<std::size_t>(bytes), {}};
    }

    if (bytes == 0) {
      return {IoStatus::Closed, 0, "connection closed while writing"};
    }

    if (errno == EINTR) {
      continue;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return {IoStatus::WouldBlock, 0, {}};
    }

    return {IoStatus::Error, 0, std::strerror(errno)};
  }
}

IoResult TcpConnection::read(void *data, std::size_t size) {
  while (true) {
    const ssize_t bytes = recv(this->nativeHandle, data, size, 0);
    if (bytes > 0) {
      return {IoStatus::Progress, static_cast<std::size_t>(bytes), {}};
    }

    if (bytes == 0) {
      return {IoStatus::Closed, 0, "connection closed by peer"};
    }

    if (errno == EINTR) {
      continue;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return {IoStatus::WouldBlock, 0, {}};
    }

    return {IoStatus::Error, 0, std::strerror(errno)};
  }
}

void TcpConnection::closeConnection() noexcept {
  if (this->nativeHandle >= 0) {
    close(this->nativeHandle);
    this->nativeHandle = -1;
  }
}

int TcpConnection::getNativeHandle() const noexcept {
  return this->nativeHandle;
}

TcpConnectionFactory::TcpConnectionFactory(unsigned short port,
                                           const char *address)
    : port(port), address(address) {}

std::unique_ptr<IConnection> TcpConnectionFactory::createConnection() {
  const int localNativeHandle = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (localNativeHandle < 0) {
    throw std::runtime_error("socket: " + std::string(std::strerror(errno)));
  }

  sockaddr_in sockAddress{};
  sockAddress.sin_family = AF_INET;
  sockAddress.sin_port = htons(port);
  if (inet_pton(AF_INET, this->address, &sockAddress.sin_addr) != 1) {
    close(localNativeHandle);
    throw std::runtime_error("invalid server address");
  }

  if (connect(localNativeHandle, reinterpret_cast<sockaddr *>(&sockAddress),
              sizeof(sockAddress)) < 0) {
    const std::string message = std::strerror(errno);
    close(localNativeHandle);
    throw std::runtime_error("connect: " + message);
  }

  return std::make_unique<TcpConnection>(localNativeHandle);
}
