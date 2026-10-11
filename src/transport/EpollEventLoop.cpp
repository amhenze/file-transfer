#include "EpollEventLoop.hpp"

#include "IConnection.hpp"
#include "IListener.hpp"
#include "detail/NativeHandle.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/epoll.h>
#include <unistd.h>

namespace {
int getNativeHandle(IListener &listener) {
  auto *handle = dynamic_cast<NativeHandle *>(&listener);
  if (handle == nullptr) {
    throw std::invalid_argument("listener is not supported by this event loop");
  }

  return handle->getNativeHandle();
}

int getNativeHandle(IConnection &connection) {
  auto *handle = dynamic_cast<NativeHandle *>(&connection);
  if (handle == nullptr) {
    throw std::invalid_argument(
        "connection is not supported by this event loop");
  }

  return handle->getNativeHandle();
}
} // namespace

EpollEventLoop::EpollEventLoop() {
  this->nativeHandle = epoll_create1(EPOLL_CLOEXEC);
  if (this->nativeHandle < 0) {
    throw std::runtime_error("epoll_create1: " +
                             std::string(std::strerror(errno)));
  }
}

EpollEventLoop::~EpollEventLoop() {
  if (this->nativeHandle >= 0) {
    close(this->nativeHandle);
  }
}

void EpollEventLoop::watch(IListener &listener, EventMask events) {
  addTarget(getNativeHandle(listener), Target{&listener, nullptr}, events);
}

void EpollEventLoop::watch(IConnection &connection, EventMask events) {
  const std::uint64_t token = addTarget(getNativeHandle(connection),
                                        Target{nullptr, &connection}, events);
  this->connectionTokens.emplace(&connection, token);
}

std::uint64_t EpollEventLoop::addTarget(int nativeHandle, Target target,
                                        EventMask events) {
  const std::uint64_t token = this->nextToken++;
  epoll_event event{};
  event.events = toNativeEvents(events);
  event.data.u64 = token;
  if (epoll_ctl(this->nativeHandle, EPOLL_CTL_ADD, nativeHandle, &event) < 0) {
    throw std::runtime_error("epoll_ctl(ADD): " +
                             std::string(std::strerror(errno)));
  }

  this->targets.emplace(token, target);
  return token;
}

void EpollEventLoop::updateTarget(int nativeHandle, std::uint64_t token,
                                  EventMask events, int operation) {
  epoll_event event{};
  event.events = toNativeEvents(events);
  event.data.u64 = token;
  if (epoll_ctl(this->nativeHandle, operation, nativeHandle, &event) < 0) {
    throw std::runtime_error("epoll_ctl(MOD): " +
                             std::string(std::strerror(errno)));
  }
}

void EpollEventLoop::modify(IConnection &connection, EventMask events) {
  const auto found = this->connectionTokens.find(&connection);
  if (found == this->connectionTokens.end()) {
    throw std::invalid_argument("connection is not registered with event loop");
  }

  updateTarget(getNativeHandle(connection), found->second, events,
               EPOLL_CTL_MOD);
}

void EpollEventLoop::remove(IConnection &connection) noexcept {
  const auto found = this->connectionTokens.find(&connection);
  if (found == this->connectionTokens.end()) {
    return;
  }

  const auto target = this->targets.find(found->second);
  if (target != this->targets.end()) {
    const int handle = getNativeHandle(connection);
    epoll_ctl(this->nativeHandle, EPOLL_CTL_DEL, handle, nullptr);

    this->targets.erase(target);
  }

  this->connectionTokens.erase(found);
}

std::vector<EventNotification> EpollEventLoop::wait() {
  epoll_event nativeEvents[64]{};
  int count = 0;
  do {
    count = epoll_wait(this->nativeHandle, nativeEvents, 64, -1);
  } while (count < 0 && errno == EINTR);

  if (count < 0) {
    throw std::runtime_error("epoll_wait: " +
                             std::string(std::strerror(errno)));
  }

  std::vector<EventNotification> notifications;
  notifications.reserve(static_cast<std::size_t>(count));
  for (int index = 0; index < count; ++index) {
    const auto target = this->targets.find(nativeEvents[index].data.u64);
    if (target == this->targets.end()) {
      continue;
    }

    notifications.push_back(
        EventNotification{target->second.listener, target->second.connection,
                          fromNativeEvents(nativeEvents[index].events)});
  }

  return notifications;
}

std::uint32_t EpollEventLoop::toNativeEvents(EventMask events) noexcept {
  std::uint32_t nativeEvents = EPOLLET;
  if (hasEvent(events, EventMask::Readable)) {
    nativeEvents |= EPOLLIN;
  }

  if (hasEvent(events, EventMask::Writable)) {
    nativeEvents |= EPOLLOUT;
  }

  if (hasEvent(events, EventMask::PeerClosed)) {
    nativeEvents |= EPOLLRDHUP;
  }

  return nativeEvents;
}

EventMask EpollEventLoop::fromNativeEvents(std::uint32_t events) noexcept {
  EventMask result = EventMask::None;
  if ((events & EPOLLIN) != 0) {
    result = result | EventMask::Readable;
  }

  if ((events & EPOLLOUT) != 0) {
    result = result | EventMask::Writable;
  }

  if ((events & (EPOLLRDHUP | EPOLLHUP)) != 0) {
    result = result | EventMask::PeerClosed;
  }

  if ((events & EPOLLERR) != 0) {
    result = result | EventMask::Error;
  }

  return result;
}
