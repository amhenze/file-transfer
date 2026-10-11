#pragma once

#include "IEventLoop.hpp"
#include <cstdint>
#include <unordered_map>

class EpollEventLoop final : public IEventLoop {
public:
  EpollEventLoop();
  ~EpollEventLoop() override;

  EpollEventLoop(const EpollEventLoop &) = delete;
  EpollEventLoop &operator=(const EpollEventLoop &) = delete;

  void watch(IListener &listener, EventMask events) override;
  void watch(IConnection &connection, EventMask events) override;
  void modify(IConnection &connection, EventMask events) override;
  void remove(IConnection &connection) noexcept override;
  std::vector<EventNotification> wait() override;

private:
  struct Target {
    IListener *listener = nullptr;
    IConnection *connection = nullptr;
  };

  std::uint64_t addTarget(int nativeHandle, Target target, EventMask events);
  void updateTarget(int nativeHandle, std::uint64_t token, EventMask events,
                    int operation);
  static std::uint32_t toNativeEvents(EventMask events) noexcept;
  static EventMask fromNativeEvents(std::uint32_t events) noexcept;

  int nativeHandle = -1;
  std::uint64_t nextToken = 1;
  std::unordered_map<std::uint64_t, Target> targets;
  std::unordered_map<IConnection *, std::uint64_t> connectionTokens;
};
