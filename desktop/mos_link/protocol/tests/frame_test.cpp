#include "mos_link/frame.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#define CHECK(condition) do { \
  if (!(condition)) { \
    std::cerr << "CHECK failed at " << __FILE__ << ":" << __LINE__ \
              << ": " #condition << std::endl; \
    std::abort(); \
  } \
} while (false)

using mos_link::Frame;
using mos_link::FrameDecoder;
using mos_link::FrameError;

int main() {
  Frame original;
  original.major = 1;
  original.minor = 2;
  original.type = 0x0041;
  original.sequence = 0x12345678;
  original.payload = {0x00, 0x7f, 0x80, 0xff};

  std::vector<std::uint8_t> encoded;
  FrameError error = FrameError::None;
  CHECK(mos_link::encodeFrame(original, encoded, &error));
  CHECK(encoded.size() == mos_link::kHeaderSize + original.payload.size());
  CHECK(encoded[0] == 'M' && encoded[1] == 'L' &&
        encoded[2] == 'N' && encoded[3] == 'K');
  CHECK(encoded[12] == 0x78 && encoded[15] == 0x12);

  FrameDecoder decoder;
  std::vector<Frame> decoded;
  CHECK(decoder.feed(encoded.data(), 5, decoded, &error));
  CHECK(decoded.empty());
  CHECK(decoder.bufferedBytes() == 5);
  CHECK(decoder.feed(encoded.data() + 5, encoded.size() - 5, decoded, &error));
  CHECK(decoded.size() == 1);
  CHECK(decoded[0].type == original.type);
  CHECK(decoded[0].sequence == original.sequence);
  CHECK(decoded[0].payload == original.payload);
  CHECK(decoder.bufferedBytes() == 0);

  // Feed one byte at a time and also test multiple frames in one chunk.
  decoded.clear();
  for (const auto byte : encoded)
    CHECK(decoder.feed(&byte, 1, decoded, &error));
  CHECK(decoded.size() == 1);
  std::vector<std::uint8_t> joined = encoded;
  joined.insert(joined.end(), encoded.begin(), encoded.end());
  decoded.clear();
  CHECK(decoder.feed(joined.data(), joined.size(), decoded, &error));
  CHECK(decoded.size() == 2);

  Frame invalid = original;
  invalid.flags = 1;
  CHECK(!mos_link::encodeFrame(invalid, encoded, &error));
  CHECK(error == FrameError::ReservedFlags);

  // The decoder must reject bad magic as soon as the four-byte prefix exists.
  const std::uint8_t bad[] = {'N', 'O', 'P', 'E'};
  CHECK(!decoder.feed(bad, sizeof(bad), decoded, &error));
  CHECK(error == FrameError::BadMagic);
  CHECK(decoder.bufferedBytes() == 0);

  CHECK(!decoder.feed(nullptr, 1, decoded, &error));
  CHECK(error == FrameError::InvalidArgument);
  CHECK(decoder.feed(nullptr, 0, decoded, &error));

  Frame maximum;
  maximum.payload.resize(mos_link::kMaxPayloadSize, 0x5a);
  CHECK(mos_link::encodeFrame(maximum, encoded, &error));
  CHECK(decoder.feed(encoded.data(), encoded.size(), decoded, &error));
  CHECK(decoded.back().payload.size() == mos_link::kMaxPayloadSize);

  maximum.payload.push_back(0);
  CHECK(!mos_link::encodeFrame(maximum, encoded, &error));
  CHECK(error == FrameError::PayloadTooLarge);
  return 0;
}
