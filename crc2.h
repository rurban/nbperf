#include <stdint.h>

#if defined(__aarch64__)
#define HAVE_HW
#include "sse2neon.h"
#elif defined __SSE4_2__ && (defined __x86_64__ || defined __i686_64__ || defined _M_AMD64 || defined _M_IX86)
#define HAVE_HW
#include <smmintrin.h>
#endif

#ifndef HAVE_HW
/* 2x 32bit hashes only, resp. 4x 16bit for smaller sets. */
static inline void crc2(const void *key, size_t len, uint64_t seed,
			uint64_t *hashes)
{
  uint64_t  h = seed;
  uint8_t  *data = (uint8_t *)key;
  const uint8_t *const end = &data[len];

  h ^= UINT64_C(0xcbf29ce484222325);
  while (data < end) {
    h ^= *data++;
    h *= UINT64_C(0x100000001b3);
  }
  hashes[0] = h;
}
#else

#define ALIGN_SIZE      0x08UL
#define ALIGN_MASK      (ALIGN_SIZE - 1)
#define CALC_CRC(op, h, type, buf, len)                               \
  do {                                                                  \
    for (; (len) >= sizeof (type); (len) -= sizeof(type), buf += sizeof (type)) { \
      (h) = op((h), *(type *) (buf));                               \
    }                                                                   \
  } while(0)

static inline void crc2(const void *key, size_t len, uint64_t seed,
			uint64_t *hashes)
{
    uint64_t  h = seed;
    uint8_t  *buf = (uint8_t *)key;

    // Align the input to the word boundary
    for (; (len > 0) && ((size_t)buf & ALIGN_MASK); len--, buf++) {
        h = _mm_crc32_u8(h, *buf);
    }

#if defined(__x86_64__) || defined(__aarch64__)
    CALC_CRC(_mm_crc32_u64, h, uint64_t, buf, len);
#endif
    CALC_CRC(_mm_crc32_u32, h, uint32_t, buf, len);
    CALC_CRC(_mm_crc32_u16, h, uint16_t, buf, len);
    CALC_CRC(_mm_crc32_u8, h, uint8_t, buf, len);

    hashes[0] = h;
}
#endif
