#include "Transfer.hpp"
#include "storage/FileReader.hpp"
#include <cstddef>

Transfer::Transfer(IConnection &connection) : connection(connection) {}

bool Transfer::writeAllBlocking(const void *data, std::size_t size) {
  const std::byte *p = static_cast<const std::byte *>(data);
  while (size > 0) {
    const IoResult result = this->connection.write(p, size);
    if (result.status == IoStatus::Progress) {
      p += result.bytes;
      size -= result.bytes;
      continue;
    }

    this->error = result.message.empty()
                      ? "connection cannot make write progress"
                      : result.message;
    return false;
  }

  return true;
}

bool Transfer::readExactBlocking(void *data, std::size_t size) {
  std::byte *p = static_cast<std::byte *>(data);
  while (size > 0) {
    const IoResult result = this->connection.read(p, size);
    if (result.status == IoStatus::Progress) {
      p += result.bytes;
      size -= result.bytes;
      continue;
    }

    this->error = result.message.empty()
                      ? "connection cannot make read progress"
                      : result.message;
    return false;
  }

  return true;
}

bool Transfer::sendFile(const std::filesystem::path &path, std::string &error) {
  try {
    FileReader reader(path);
    Header h;
    h.fileSize = reader.getSize();
    h.checksum = reader.getChecksum();

    const std::array<std::byte, Protocol::headerSize> encoded =
        Protocol::encodeHeader(h);
    if (!writeAllBlocking(encoded.data(), encoded.size())) {
      error = this->error;
      return false;
    }

    char buffer[64 * 1024];
    std::uint64_t sent = 0;
    while (sent < h.fileSize) {
      std::size_t n = 0;
      if (!reader.read(buffer, sizeof(buffer), n)) {
        error = "error reading input file";
        return false;
      }

      if (n == 0) {
        error = "unexpected end of input file";
        return false;
      }

      const std::size_t remaining = static_cast<std::size_t>(h.fileSize - sent);
      const std::size_t toSend = n > remaining ? remaining : n;
      if (!writeAllBlocking(buffer, toSend)) {
        error = this->error;
        return false;
      }

      sent += static_cast<std::uint64_t>(toSend);
    }

    std::array<std::byte, Protocol::ackSize> ack{};
    if (!readExactBlocking(ack.data(), ack.size())) {
      error = this->error;
      return false;
    }

    if (Protocol::decodeAck(ack.data(), ack.size()) != AckStatus::Success) {
      error = "server rejected file";
      return false;
    }

    return true;
  } catch (const std::exception &e) {
    error = e.what();
    return false;
  }
}

bool Transfer::receiveHeader() {
  while (headerReceived < headerBuffer.size()) {
    const IoResult result =
        this->connection.read(headerBuffer.data() + headerReceived,
                              headerBuffer.size() - headerReceived);
    if (result.status == IoStatus::Progress) {
      headerReceived += result.bytes;
      continue;
    }

    if (result.status == IoStatus::WouldBlock) {
      return true;
    }

    fail(result.message.empty() ? "failed to receive header" : result.message);
    return false;
  }

  try {
    this->header =
        Protocol::decodeHeader(headerBuffer.data(), headerBuffer.size());
    this->writer = std::make_unique<FileWriter>();
    this->state = State::ReceiveData;
    return true;
  } catch (const std::exception &e) {
    fail(e.what());
    return false;
  }
}

bool Transfer::receiveData() {
  while (this->received < this->header.fileSize) {
    const std::uint64_t remaining = this->header.fileSize - received;
    const std::size_t requestSize = remaining < this->dataBuffer.size()
                                        ? static_cast<std::size_t>(remaining)
                                        : this->dataBuffer.size();
    const IoResult result =
        this->connection.read(this->dataBuffer.data(), requestSize);
    if (result.status == IoStatus::Progress) {
      const std::size_t count = result.bytes;
      try {
        this->writer->write(this->dataBuffer.data(), count);
      } catch (const std::exception &e) {
        fail(e.what());
        return false;
      }

      this->checksum.update(this->dataBuffer.data(), count);
      this->received += static_cast<std::uint64_t>(count);
      continue;
    }

    if (result.status == IoStatus::WouldBlock) {
      return true;
    }

    fail(result.message.empty()
             ? "client closed connection before file was complete"
             : result.message);
    return false;
  }

  if (this->checksum.getValue() != header.checksum) {
    fail("checksum mismatch");
    return false;
  }

  try {
    this->outputPath = writer->finalize();
    this->writer.reset();
  } catch (const std::exception &e) {
    fail(e.what());
    return false;
  }

  queueAck(AckStatus::Success);
  this->state = State::SendAck;
  return true;
}

bool Transfer::queueAck(AckStatus status) {
  this->ackBuffer = Protocol::encodeAck(status);
  this->ackSent = 0;
  this->ackQueued = true;
  return true;
}

bool Transfer::flushSendBuffer() {
  while (this->ackSent < this->ackBuffer.size()) {
    const IoResult result =
        this->connection.write(this->ackBuffer.data() + this->ackSent,
                               this->ackBuffer.size() - this->ackSent);
    if (result.status == IoStatus::Progress) {
      this->ackSent += result.bytes;
      continue;
    }

    if (result.status == IoStatus::WouldBlock) {
      return false;
    }

    fail(result.message.empty() ? "failed to send ACK" : result.message);
    return false;
  }

  return true;
}

Transfer::ProcessResult Transfer::sendAck() {
  if (!this->ackQueued) {
    return ProcessResult::Error;
  }

  if (!flushSendBuffer()) {
    return this->error.empty() ? ProcessResult::Progress : ProcessResult::Error;
  }

  this->ackQueued = false;
  this->state = State::Done;
  return ProcessResult::Done;
}

Transfer::ProcessResult Transfer::process() {
  if (this->state == State::Done) {
    return ProcessResult::Done;
  }

  if (this->state == State::Error) {
    return ProcessResult::Error;
  }

  if (this->state == State::ReceiveHeader && !receiveHeader()) {
    return ProcessResult::Error;
  }
  if (this->state == State::ReceiveData && !receiveData()) {
    if (this->state == State::SendAck) {
      return ProcessResult::Progress;
    }

    return ProcessResult::Error;
  }

  if (this->state == State::SendAck) {
    return sendAck();
  }

  if (this->state == State::Done) {
    return ProcessResult::Done;
  }

  return ProcessResult::Progress;
}

bool Transfer::needsWrite() const noexcept {
  return this->state == State::SendAck && this->ackQueued;
}

const std::filesystem::path &Transfer::getOutputPath() const noexcept {
  return this->outputPath;
}
const std::string &Transfer::getError() const noexcept { return this->error; }

void Transfer::fail(std::string message) {
  this->error = std::move(message);
  if (this->writer) {
    this->writer->discard();
    this->writer.reset();
  }

  this->state = State::Error;
}
