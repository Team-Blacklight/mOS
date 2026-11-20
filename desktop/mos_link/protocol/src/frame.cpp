#include "mos_link/frame.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace mos_link {
namespace {

constexpr std::uint8_t kMagic[4] = {'M', 'L', 'N', 'K'};

void setError(FrameError* error, FrameError value) {
  if (error != nullptr) *error = value;
}

void write16(std::vector<std::uint8_t>& out, std::size_t offset,
             std::uint16_t value) {
  out[offset] = static_cast<std::uint8_t>(value & 0xff);
  out[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
}

void write32(std::vector<std::uint8_t>& out, std::size_t offset,
             std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i)
    out[offset + i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xff);
}

std::uint16_t read16(const std::vector<std::uint8_t>& in,
                     std::size_t offset) {
  return static_cast<std::uint16_t>(in[offset]) |
         static_cast<std::uint16_t>(in[offset + 1] << 8);
}

std::uint32_t read32(const std::vector<std::uint8_t>& in,
                     std::size_t offset) {
  std::uint32_t value = 0;
  for (unsigned i = 0; i < 4; ++i)
    value |= static_cast<std::uint32_t>(in[offset + i]) << (8 * i);
  return value;
}

}  // namespace

bool encodeFrame(const Frame& frame, std::vector<std::uint8_t>& output,
                 FrameError* error) {
  setError(error, FrameError::None);
  if (frame.flags != 0) {
    setError(error, FrameError::ReservedFlags);
    return false;
  }
  if (frame.payload.size() > kMaxPayloadSize) {
    setError(error, FrameError::PayloadTooLarge);
    return false;
  }

  output.assign(kHeaderSize + frame.payload.size(), 0);
  std::copy(std::begin(kMagic), std::end(kMagic), output.begin());
  output[4] = frame.major;
  output[5] = frame.minor;
  write16(output, 6, frame.type);
  write16(output, 8, frame.flags);
  write16(output, 10, static_cast<std::uint16_t>(frame.payload.size()));
  write32(output, 12, frame.sequence);
  std::copy(frame.payload.begin(), frame.payload.end(),
            output.begin() + kHeaderSize);
  return true;
}

bool FrameDecoder::feed(const std::uint8_t* data, std::size_t size,
                        std::vector<Frame>& output, FrameError* error) {
  setError(error, FrameError::None);
  if (data == nullptr && size != 0) {
    setError(error, FrameError::InvalidArgument);
    return false;
  }

  for (std::size_t i = 0; i < size; ++i) {
    if (buffer_.size() >= kHeaderSize + kMaxPayloadSize) {
      reset();
      setError(error, FrameError::PayloadTooLarge);
      return false;
    }
    buffer_.push_back(data[i]);

    // Validate each header field as soon as enough bytes have arrived.
    if (buffer_.size() == 4 &&
        !std::equal(std::begin(kMagic), std::end(kMagic), buffer_.begin())) {
      reset();
      setError(error, FrameError::BadMagic);
      return false;
    }
    if (buffer_.size() == 10 && read16(buffer_, 8) != 0) {
      reset();
      setError(error, FrameError::ReservedFlags);
      return false;
    }
    if (buffer_.size() == 12 && read16(buffer_, 10) > kMaxPayloadSize) {
      reset();
      setError(error, FrameError::PayloadTooLarge);
      return false;
    }

    if (buffer_.size() < kHeaderSize) continue;
    const std::size_t frameSize = kHeaderSize + read16(buffer_, 10);
    if (buffer_.size() != frameSize) continue;

    Frame frame;
    frame.major = buffer_[4];
    frame.minor = buffer_[5];
    frame.type = read16(buffer_, 6);
    frame.flags = read16(buffer_, 8);
    frame.sequence = read32(buffer_, 12);
    frame.payload.assign(buffer_.begin() + kHeaderSize, buffer_.end());
    output.push_back(std::move(frame));
    buffer_.clear();
  }
  return true;
}

void FrameDecoder::reset() noexcept { buffer_.clear(); }

}  // namespace mos_link
