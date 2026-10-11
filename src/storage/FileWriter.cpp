#include "FileWriter.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
std::string timestampName() {
  using namespace std::chrono;
  const time_point now = system_clock::now();
  const std::time_t tt = system_clock::to_time_t(now);
  const std::chrono::milliseconds millisecondsPart =
      duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

  std::tm tm{};
  localtime_r(&tt, &tm);
  std::ostringstream out;
  out << std::put_time(&tm, "%Y%m%d_%H%M%S") << '_' << std::setw(3)
      << std::setfill('0') << millisecondsPart.count();
  return out.str();
}
} // namespace

FileWriter::FileWriter() {
  const std::filesystem::path base = std::filesystem::current_path();
  const std::string stem = timestampName();
  this->tmpPath = base / (stem + ".hex.part");

  int suffix = 1;
  while (std::filesystem::exists(this->tmpPath)) {
    this->tmpPath =
        base / (stem + "_" + std::to_string(suffix++) + ".hex.part");
  }

  this->file.open(this->tmpPath, std::ios::binary | std::ios::trunc);
  if (!this->file) {
    throw std::runtime_error("cannot create output file: " +
                             this->tmpPath.string());
  }
}

FileWriter::~FileWriter() {
  if (!this->finalized) {
    discard();
  }
}

void FileWriter::write(const void *data, std::size_t size) {
  this->file.write(static_cast<const char *>(data),
                   static_cast<std::streamsize>(size));
  if (!this->file) {
    throw std::runtime_error("error while writing output file");
  }
}

std::filesystem::path FileWriter::finalize() {
  if (this->finalized) {
    throw std::runtime_error("file already finalized");
  }

  this->file.flush();
  this->file.close();
  if (!this->file) {
    throw std::runtime_error("error while closing output file");
  }

  const std::string tempName = this->tmpPath.filename().string();
  const std::string finalName =
      tempName.substr(0, tempName.size() - 5); // remove ".part"
  const std::filesystem::path finalPath =
      this->tmpPath.parent_path() / finalName;
  std::filesystem::rename(this->tmpPath, finalPath);
  this->finalized = true;
  return finalPath;
}

void FileWriter::discard() noexcept {
  if (this->file.is_open()) {
    this->file.close();
  }

  std::error_code ec;
  std::filesystem::remove(this->tmpPath, ec);
}
