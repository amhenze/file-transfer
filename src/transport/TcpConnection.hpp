#pragma once

#include "IConnection.hpp"
#include "IConnectionFactory.hpp"
#include "detail/NativeHandle.hpp"

class TcpConnection final : public IConnection, public NativeHandle {
public:
  explicit TcpConnection(int nativeHandle) noexcept;
  ~TcpConnection() override;

  TcpConnection(const TcpConnection &) = delete;
  TcpConnection &operator=(const TcpConnection &) = delete;

  IoResult write(const void *data, std::size_t size) override;
  IoResult read(void *data, std::size_t size) override;
  void closeConnection() noexcept override;

private:
  int getNativeHandle() const noexcept override;
  int nativeHandle = -1;
};

class TcpConnectionFactory final : public IConnectionFactory {
public:
  explicit TcpConnectionFactory(unsigned short port,
                                const char *address = "127.0.0.1");
  std::unique_ptr<IConnection> createConnection() override;

private:
  unsigned short port;
  const char *address;
};
