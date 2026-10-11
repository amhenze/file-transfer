#pragma once

#include <cstddef>
#include <cstdint>

class Crc32 {
public:
  Crc32() noexcept = default;
  void update(const void *data, std::size_t size) noexcept;
  std::uint32_t getValue() const noexcept;

private:
  std::uint32_t crc = 0xFFFFFFFFu;
};
