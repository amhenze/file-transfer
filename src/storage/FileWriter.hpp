#pragma once

#include <filesystem>
#include <fstream>

class FileWriter {
public:
  FileWriter();
  ~FileWriter();

  FileWriter(const FileWriter &) = delete;
  FileWriter &operator=(const FileWriter &) = delete;

  void write(const void *data, std::size_t size);
  std::filesystem::path finalize();
  void discard() noexcept;

private:
  std::filesystem::path tmpPath;
  std::ofstream file;
  bool finalized = false;
};
