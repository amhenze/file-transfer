#include "transfer/Protocol.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace {
constexpr std::uint32_t kMagic = 0x46545231u;    // FTR1
constexpr std::uint32_t kAckMagic = 0x41434B31u; // ACK1

void put16(std::byte *p, std::uint16_t v) {
  p[0] = std::byte(v >> 8);
  p[1] = std::byte(v);
}

void put32(std::byte *p, std::uint32_t v) {
  for (int i = 0; i < 4; ++i) {
    p[i] = std::byte(v >> (24 - 8 * i));
  }
}

void put64(std::byte *p, std::uint64_t v) {
  for (int i = 0; i < 8; ++i) {
    p[i] = std::byte(v >> (56 - 8 * i));
  }
}

std::uint16_t get16(const std::byte *p) {
  return (std::uint16_t(std::to_integer<unsigned>(p[0])) << 8) |
         std::uint16_t(std::to_integer<unsigned>(p[1]));
}

std::uint32_t get32(const std::byte *p) {
  std::uint32_t v = 0;
  for (int i = 0; i < 4; ++i) {
    v = (v << 8) | std::to_integer<unsigned>(p[i]);
  }

  return v;
}

std::uint64_t get64(const std::byte *p) {
  std::uint64_t v = 0;
  for (int i = 0; i < 8; ++i) {
    v = (v << 8) | std::to_integer<unsigned>(p[i]);
  }

  return v;
}
} // namespace

std::array<std::byte, Protocol::headerSize>
Protocol::encodeHeader(const Header &h) {
  std::array<std::byte, headerSize> b{};
  put32(b.data(), kMagic);
  put16(b.data() + 4, h.version);
  put16(b.data() + 6, 0);
  put64(b.data() + 8, h.fileSize);
  put32(b.data() + 16, h.checksum);
  put32(b.data() + 20, 0);
  return b;
}

Header Protocol::decodeHeader(const std::byte *data, std::size_t size) {
  if (size != headerSize) {
    throw std::runtime_error("invalid header size");
  }

  if (get32(data) != kMagic) {
    throw std::runtime_error("invalid protocol magic");
  }

  Header h;
  h.version = get16(data + 4);
  if (h.version != 1) {
    throw std::runtime_error("unsupported protocol version");
  }

  if (get16(data + 6) != 0 || get32(data + 20) != 0) {
    throw std::runtime_error("invalid header");
  }

  h.fileSize = get64(data + 8);
  h.checksum = get32(data + 16);
  return h;
}

std::array<std::byte, Protocol::ackSize> Protocol::encodeAck(AckStatus status) {
  std::array<std::byte, ackSize> b{};
  put32(b.data(), kAckMagic);
  put32(b.data() + 4, static_cast<std::uint32_t>(status));
  return b;
}

AckStatus Protocol::decodeAck(const std::byte *data, std::size_t size) {
  if (size != ackSize || get32(data) != kAckMagic) {
    throw std::runtime_error("invalid ACK");
  }

  const std::uint32_t value = get32(data + 4);
  if (value > 1) {
    throw std::runtime_error("invalid ACK status");
  }

  return static_cast<AckStatus>(value);
}
