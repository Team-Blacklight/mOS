#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mos_link {

constexpr std::size_t kHeaderSize = 16;
constexpr std::size_t kMaxPayloadSize = 4096;
constexpr std::uint8_t kProtocolMajor = 1;
constexpr std::uint8_t kProtocolMinor = 0;

struct Frame {
  std::uint8_t major = kProtocolMajor;
  std::uint8_t minor = kProtocolMinor;
  std::uint16_t type = 0;
  std::uint16_t flags = 0;
  std::uint32_t sequence = 0;
  std::vector<std::uint8_t> payload;
};

enum class FrameError {
  None,
  InvalidArgument,
  BadMagic,
  ReservedFlags,
  PayloadTooLarge
};

// Encodes the 16-byte MLNK envelope. Authentication/encryption is deliberately
// a separate layer; this class does not imply that a frame is trusted.
bool encodeFrame(const Frame& frame, std::vector<std::uint8_t>& output,
                 FrameError* error = nullptr);

// Incremental decoder for arbitrary USB/serial read boundaries. Each call
// appends complete frames to output and retains at most one incomplete frame.
class FrameDecoder {
 public:
  bool feed(const std::uint8_t* data, std::size_t size,
            std::vector<Frame>& output, FrameError* error = nullptr);
  void reset() noexcept;
  std::size_t bufferedBytes() const noexcept { return buffer_.size(); }

 private:
  std::vector<std::uint8_t> buffer_;
};

}  // namespace mos_link
