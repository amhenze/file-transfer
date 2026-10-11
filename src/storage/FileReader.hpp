#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>

class FileReader {
public:
  explicit FileReader(const std::filesystem::path &path);

  std::uint64_t getSize() const;
  std::uint32_t getChecksum();
  bool read(char *buffer, std::size_t capacity, std::size_t &bytesRead);
  bool eof() const noexcept;

private:
  std::filesystem::path path;
  std::ifstream file;
};
