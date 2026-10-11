#include "application/Server.hpp"
#include "transfer/Transfer.hpp"
#include "transport/IConnection.hpp"
#include "transport/IEventLoop.hpp"
#include "transport/IListener.hpp"

#include <iostream>
#include <memory>
#include <unordered_map>

namespace {
struct Session {
  std::unique_ptr<IConnection> connection;
  std::unique_ptr<Transfer> transfer;
};

constexpr EventMask readEvents = EventMask::Readable | EventMask::PeerClosed;
constexpr EventMask writeEvents = EventMask::Writable | EventMask::PeerClosed;
} // namespace

int Server::run(IListener &listener, IEventLoop &eventLoop) {
  eventLoop.watch(listener, EventMask::Readable);
  std::unordered_map<IConnection *, Session> sessions;
  std::cout << "Server started\n";

  while (true) {
    for (const EventNotification &notification : eventLoop.wait()) {
      if (notification.isListenerEvent()) {
        while (auto connection = listener.accept()) {
          auto *connectionPointer = connection.get();
          auto transfer = std::make_unique<Transfer>(*connection);
          eventLoop.watch(*connection, readEvents);
          sessions.emplace(connectionPointer,
                           Session{std::move(connection), std::move(transfer)});
          std::cout << "Client connected\n";
        }
        continue;
      }

      IConnection *connection = notification.connection;
      const auto sessionPosition = sessions.find(connection);
      if (sessionPosition == sessions.end()) {
        continue;
      }

      Session &session = sessionPosition->second;
      const EventMask events = notification.events;
      if (hasEvent(events, EventMask::Error)) {
        std::cerr << "Client connection error\n";
        eventLoop.remove(*connection);
        sessions.erase(sessionPosition);
        continue;
      }

      Transfer::ProcessResult result = Transfer::ProcessResult::Progress;
      if (hasEvent(events, EventMask::Readable) ||
          hasEvent(events, EventMask::Writable)) {
        result = session.transfer->process();
      }

      if (result == Transfer::ProcessResult::Done) {
        std::cout << "Transfer completed";
        if (!session.transfer->getOutputPath().empty()) {
          std::cout << " -> " << session.transfer->getOutputPath();
        }

        std::cout << '\n';
        eventLoop.remove(*connection);
        sessions.erase(sessionPosition);
        continue;
      }

      if (result == Transfer::ProcessResult::Error ||
          result == Transfer::ProcessResult::Closed ||
          hasEvent(events, EventMask::PeerClosed)) {
        std::cerr << "Transfer failed: " << session.transfer->getError()
                  << '\n';
        eventLoop.remove(*connection);
        sessions.erase(sessionPosition);
        continue;
      }

      eventLoop.modify(*connection, session.transfer->needsWrite()
                                        ? writeEvents
                                        : readEvents);
    }
  }
}
