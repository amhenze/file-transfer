#include "application/Client.hpp"
#include "application/Server.hpp"
#include "transport/EpollEventLoop.hpp"
#include "transport/TcpConnection.hpp"
#include "transport/TcpListener.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr unsigned short defaultPort = 5000;

void printUsage(const char *programName) {
  std::cerr << "Usage:\n"
            << "  " << programName << " -s [port]\n"
            << "  " << programName << " -c <file> [port]\n";
}

std::uint16_t parsePort(const char *text) {
  const std::uint64_t value = std::stoul(text);
  if (value == 0 || value > 65535) {
    throw std::invalid_argument("invalid port");
  }

  return static_cast<std::uint16_t>(value);
}
} // namespace

int main(int argc, char *argv[]) {
  try {
    if (argc < 2) {
      printUsage(argv[0]);
      return 2;
    }

    if (std::string(argv[1]) == "-s") {
      const unsigned short port = argc >= 3 ? parsePort(argv[2]) : defaultPort;
      TcpListener listener(port);
      EpollEventLoop eventLoop;
      return Server{}.run(listener, eventLoop);
    }

    if (std::string(argv[1]) == "-c") {
      if (argc < 3) {
        printUsage(argv[0]);
        return 2;
      }
      const unsigned short port = argc >= 4 ? parsePort(argv[3]) : defaultPort;
      TcpConnectionFactory connectionFactory(port);
      return Client{}.run(argv[2], connectionFactory);
    }

    printUsage(argv[0]);
    return 2;
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << '\n';
    return 1;
  }
}
