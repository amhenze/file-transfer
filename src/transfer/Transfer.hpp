#pragma once

#include "Protocol.hpp"
#include "common/Checksum.hpp"
#include "storage/FileWriter.hpp"
#include "transport/IConnection.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

class Transfer {
public:
  enum class ProcessResult : std::uint8_t { Progress, Done, Error, Closed };
  explicit Transfer(IConnection &connection);
  bool sendFile(const std::filesystem::path &path, std::string &error);
  ProcessResult process();
  bool needsWrite() const noexcept;
  const std::filesystem::path &getOutputPath() const noexcept;
  const std::string &getError() const noexcept;

private:
  enum class State : std::uint8_t {
    ReceiveHeader,
    ReceiveData,
    SendAck,
    Done,
    Error
  };

  bool receiveHeader();
  bool receiveData();
  ProcessResult sendAck();
  bool queueAck(AckStatus status);
  bool flushSendBuffer();
  bool readExactBlocking(void *data, std::size_t size);
  bool writeAllBlocking(const void *data, std::size_t size);
  void fail(std::string message);

  IConnection &connection;
  State state = State::ReceiveHeader;
  std::array<std::byte, Protocol::headerSize> headerBuffer{};
  std::size_t headerReceived = 0;
  Header header{};
  std::uint64_t received = 0;
  Crc32 checksum;
  std::unique_ptr<FileWriter> writer;
  std::array<std::byte, Protocol::ackSize> ackBuffer{};
  std::size_t ackSent = 0;
  bool ackQueued = false;
  std::filesystem::path outputPath;
  std::string error;
  std::array<char, 64 * 1024> dataBuffer{};
};
