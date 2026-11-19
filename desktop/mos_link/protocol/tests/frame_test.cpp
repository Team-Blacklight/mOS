#include "mos_link/frame.h"

#include <cassert>
#include <cstdint>
#include <vector>

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
  assert(mos_link::encodeFrame(original, encoded, &error));
  assert(encoded.size() == mos_link::kHeaderSize + original.payload.size());
  assert(encoded[0] == 'M' && encoded[1] == 'L' &&
         encoded[2] == 'N' && encoded[3] == 'K');
  assert(encoded[12] == 0x78 && encoded[15] == 0x12);

  FrameDecoder decoder;
  std::vector<Frame> decoded;
  assert(decoder.feed(encoded.data(), 5, decoded, &error));
  assert(decoded.empty());
  assert(decoder.bufferedBytes() == 5);
  assert(decoder.feed(encoded.data() + 5, encoded.size() - 5, decoded, &error));
  assert(decoded.size() == 1);
  assert(decoded[0].type == original.type);
  assert(decoded[0].sequence == original.sequence);
  assert(decoded[0].payload == original.payload);
  assert(decoder.bufferedBytes() == 0);

  // Two frames in one read must both be emitted.
  std::vector<std::uint8_t> joined = encoded;
  joined.insert(joined.end(), encoded.begin(), encoded.end());
  decoded.clear();
  assert(decoder.feed(joined.data(), joined.size(), decoded, &error));
  assert(decoded.size() == 2);

  // Reject reserved flags before accepting payload data.
  Frame invalid = original;
  invalid.flags = 1;
  assert(!mos_link::encodeFrame(invalid, encoded, &error));
  assert(error == FrameError::ReservedFlags);

  // Reject malformed magic and reset the parser.
  const std::uint8_t bad[] = {'N', 'O', 'P', 'E'};
  assert(!decoder.feed(bad, sizeof(bad), decoded, &error));
  assert(error == FrameError::BadMagic);
  assert(decoder.bufferedBytes() == 0);

  // A null pointer is valid only for an empty input chunk.
  assert(!decoder.feed(nullptr, 1, decoded, &error));
  assert(error == FrameError::InvalidArgument);
  return 0;
}
