#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

struct Header {
  std::uint16_t version = 1;
  std::uint64_t fileSize = 0;
  std::uint32_t checksum = 0;
};

enum class AckStatus : std::uint8_t { Success = 0, Error = 1 };

class Protocol {
public:
  static constexpr std::size_t headerSize = 24;
  static constexpr std::size_t ackSize = 8;

  static std::array<std::byte, headerSize> encodeHeader(const Header &header);
  static Header decodeHeader(const std::byte *data, std::size_t size);
  static std::array<std::byte, ackSize> encodeAck(AckStatus status);
  static AckStatus decodeAck(const std::byte *data, std::size_t size);
};
