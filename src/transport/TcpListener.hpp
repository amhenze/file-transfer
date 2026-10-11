#pragma once

#include "IListener.hpp"
#include "detail/NativeHandle.hpp"

class TcpListener final : public IListener, public NativeHandle {
public:
  explicit TcpListener(unsigned short port, int backlog = 128);
  ~TcpListener() override;

  TcpListener(const TcpListener &) = delete;
  TcpListener &operator=(const TcpListener &) = delete;

  std::unique_ptr<IConnection> accept() override;

private:
  int getNativeHandle() const noexcept override;
  int nativeHandle = -1;
};
