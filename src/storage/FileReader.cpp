#include "FileReader.hpp"
#include "common/Checksum.hpp"

#include <stdexcept>

FileReader::FileReader(const std::filesystem::path &path)
    : path(path), file(path, std::ios::binary) {
  if (!this->file) {
    throw std::runtime_error("cannot open input file: " + path.string());
  }
}

std::uint64_t FileReader::getSize() const {
  const uintmax_t value = std::filesystem::file_size(this->path);
  return static_cast<std::uint64_t>(value);
}

std::uint32_t FileReader::getChecksum() {
  this->file.clear();
  this->file.seekg(0, std::ios::beg);
  if (!this->file) {
    throw std::runtime_error("cannot seek input file");
  }

  Crc32 crc;
  char buffer[64 * 1024];
  while (this->file) {
    this->file.read(buffer, sizeof(buffer));
    const std::streamsize n = this->file.gcount();
    if (n > 0) {
      crc.update(buffer, static_cast<std::size_t>(n));
    }
  }
  if (!this->file.eof()) {
    throw std::runtime_error("error while reading input file");
  }

  this->file.clear();
  this->file.seekg(0, std::ios::beg);
  return crc.getValue();
}

bool FileReader::read(char *buffer, std::size_t capacity,
                      std::size_t &bytesRead) {
  this->file.read(buffer, static_cast<std::streamsize>(capacity));
  const std::streamsize n = this->file.gcount();
  if (n < 0) {
    return false;
  }

  bytesRead = static_cast<std::size_t>(n);
  if (this->file.bad()) {
    return false;
  }

  return true;
}

bool FileReader::eof() const noexcept { return this->file.eof(); }
