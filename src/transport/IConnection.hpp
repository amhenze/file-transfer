#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

enum class IoStatus : std::uint8_t { Progress, WouldBlock, Closed, Error };

struct IoResult {
  IoStatus status = IoStatus::Progress;
  std::size_t bytes = 0;
  std::string message;
};

class IConnection {
public:
  virtual ~IConnection() = default;
  virtual IoResult write(const void *data, std::size_t size) = 0;
  virtual IoResult read(void *data, std::size_t size) = 0;
  virtual void closeConnection() noexcept = 0;
};
