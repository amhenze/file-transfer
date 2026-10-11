#pragma once

#include "IConnection.hpp"
#include "IListener.hpp"
#include <cstdint>
#include <vector>

enum class EventMask : std::uint8_t {
  None = 0,
  Readable = 1u << 0,
  Writable = 1u << 1,
  PeerClosed = 1u << 2,
  Error = 1u << 3
};

constexpr EventMask operator|(EventMask left, EventMask right) noexcept {
  return static_cast<EventMask>(static_cast<std::uint32_t>(left) |
                                static_cast<std::uint32_t>(right));
}

constexpr bool hasEvent(EventMask events, EventMask expected) noexcept {
  return (static_cast<std::uint32_t>(events) &
          static_cast<std::uint32_t>(expected)) != 0;
}

struct EventNotification {
  IListener *listener = nullptr;
  IConnection *connection = nullptr;
  EventMask events = EventMask::None;

  bool isListenerEvent() const noexcept { return listener != nullptr; }
};

class IEventLoop {
public:
  virtual ~IEventLoop() = default;
  virtual void watch(IListener &listener, EventMask events) = 0;
  virtual void watch(IConnection &connection, EventMask events) = 0;
  virtual void modify(IConnection &connection, EventMask events) = 0;
  virtual void remove(IConnection &connection) noexcept = 0;
  virtual std::vector<EventNotification> wait() = 0;
};
