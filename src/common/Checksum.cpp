#include "Checksum.hpp"

#include <array>

namespace {
std::array<std::uint32_t, 256> makeTable() {
  std::array<std::uint32_t, 256> table{};
  for (std::uint32_t i = 0; i < 256; ++i) {
    std::uint32_t c = i;
    for (int j = 0; j < 8; ++j) {
      c = (c & 1u) ? (0xEDB88320u ^ (c >> 1u)) : (c >> 1u);
    }

    table[i] = c;
  }

  return table;
}

const std::array<std::uint32_t, 256> table = makeTable();
} // namespace

void Crc32::update(const void *data, std::size_t size) noexcept {
  const unsigned char *bytes = static_cast<const unsigned char *>(data);
  for (std::size_t i = 0; i < size; ++i) {
    this->crc = table[(this->crc ^ bytes[i]) & 0xFFu] ^ (this->crc >> 8u);
  }
}

std::uint32_t Crc32::getValue() const noexcept {
  return this->crc ^ 0xFFFFFFFFu;
}
