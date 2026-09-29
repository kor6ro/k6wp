#include "sha1.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace k6wp {
namespace {

// --- Minimal SHA1 (FIPS 180-1). ---
// Consolidated from the two byte-identical copies that used to live in
// compressor/src/cache_manager.cpp and studio/src/thumbnailer.cpp (MED-3).

std::uint32_t Rotl(std::uint32_t v, unsigned n) {
  return (v << n) | (v >> (32U - n));
}

struct Sha1Ctx {
  std::uint32_t h[5] = {0x67452301U, 0xEFCDAB89U, 0x98BADCFEU,
                        0x10325476U, 0xC3D2E1F0U};
  std::uint64_t bit_len = 0;
  std::array<unsigned char, 64> block = {};
  std::size_t block_used = 0;
};

void Sha1Transform(Sha1Ctx& ctx, const unsigned char* p) {
  std::array<std::uint32_t, 80> w = {};
  for (int i = 0; i < 16; ++i) {
    w[static_cast<std::size_t>(i)] =
        (static_cast<std::uint32_t>(p[i * 4]) << 24) |
        (static_cast<std::uint32_t>(p[i * 4 + 1]) << 16) |
        (static_cast<std::uint32_t>(p[i * 4 + 2]) << 8) |
        static_cast<std::uint32_t>(p[i * 4 + 3]);
  }
  for (int i = 16; i < 80; ++i) {
    w[static_cast<std::size_t>(i)] =
        Rotl(w[static_cast<std::size_t>(i - 3)] ^
                 w[static_cast<std::size_t>(i - 8)] ^
                 w[static_cast<std::size_t>(i - 14)] ^
                 w[static_cast<std::size_t>(i - 16)],
             1);
  }
  std::uint32_t a = ctx.h[0];
  std::uint32_t b = ctx.h[1];
  std::uint32_t c = ctx.h[2];
  std::uint32_t d = ctx.h[3];
  std::uint32_t e = ctx.h[4];
  for (int i = 0; i < 80; ++i) {
    std::uint32_t f = 0;
    std::uint32_t k = 0;
    if (i < 20) {
      f = (b & c) | ((~b) & d);
      k = 0x5A827999U;
    } else if (i < 40) {
      f = b ^ c ^ d;
      k = 0x6ED9EBA1U;
    } else if (i < 60) {
      f = (b & c) | (b & d) | (c & d);
      k = 0x8F1BBCDCU;
    } else {
      f = b ^ c ^ d;
      k = 0xCA62C1D6U;
    }
    const std::uint32_t tmp =
        Rotl(a, 5) + f + e + k + w[static_cast<std::size_t>(i)];
    e = d;
    d = c;
    c = Rotl(b, 30);
    b = a;
    a = tmp;
  }
  ctx.h[0] += a;
  ctx.h[1] += b;
  ctx.h[2] += c;
  ctx.h[3] += d;
  ctx.h[4] += e;
}

void Sha1Update(Sha1Ctx& ctx, const unsigned char* data, std::size_t len) {
  ctx.bit_len += static_cast<std::uint64_t>(len) * 8ULL;
  std::size_t off = 0;
  while (off < len) {
    const std::size_t room = 64 - ctx.block_used;
    const std::size_t take = (std::min)(room, len - off);
    std::memcpy(ctx.block.data() + ctx.block_used, data + off, take);
    ctx.block_used += take;
    off += take;
    if (ctx.block_used == 64) {
      Sha1Transform(ctx, ctx.block.data());
      ctx.block_used = 0;
    }
  }
}

}  // namespace

std::string Sha1Hex(const std::string& data) {
  Sha1Ctx ctx;
  Sha1Update(ctx, reinterpret_cast<const unsigned char*>(data.data()),
             data.size());
  // Snapshot the message length before padding; Sha1Update grows bit_len.
  const std::uint64_t msg_bits = ctx.bit_len;
  unsigned char one = 0x80;
  Sha1Update(ctx, &one, 1);
  unsigned char zero = 0x00;
  while (ctx.block_used != 56) {
    Sha1Update(ctx, &zero, 1);
  }
  // Length block, big-endian, applied as a raw transform so bit_len
  // bookkeeping stays untouched.
  unsigned char len_bytes[8] = {};
  std::uint64_t tmp = msg_bits;
  for (int i = 7; i >= 0; --i) {
    len_bytes[i] = static_cast<unsigned char>(tmp & 0xFFULL);
    tmp >>= 8;
  }
  unsigned char final_block[64] = {};
  std::memcpy(final_block, ctx.block.data(), 56);
  std::memcpy(final_block + 56, len_bytes, 8);
  Sha1Transform(ctx, final_block);
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out;
  out.reserve(40);
  for (int i = 0; i < 5; ++i) {
    for (int shift = 28; shift >= 0; shift -= 4) {
      out.push_back(kHex[(ctx.h[static_cast<std::size_t>(i)] >>
                          static_cast<unsigned>(shift)) &
                         0xFU]);
    }
  }
  return out;
}

}  // namespace k6wp