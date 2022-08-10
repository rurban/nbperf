#include <stdint.h>
#include <stddef.h>

static inline void fnv(const void *key, size_t len, uint64_t seed,
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
  *hashes = h;
}

/* chm3/bpz need at least 96 bit, so produce 2x 64bit. */
static inline void fnv3(const void *key, size_t len, uint64_t seed,
			uint64_t *hashes)
{
  uint64_t  h0 = seed ^ UINT64_C(0xcbf29ce484222325);
  uint64_t  h1 = seed ^ UINT64_C(0xc4ceb9fe1a85ec53); /* from PMP_Multilinear */
  uint8_t  *data = (uint8_t *)key;
  const uint8_t *const end = &data[len];

  while (data < end) {
    h0 ^= *data;
    h0 *= UINT64_C(0x100000001b3);
    h1 ^= *data;
    h1 *= UINT64_C(0x100000001b3);
    data++;
  }
  hashes[0] = h0;
  hashes[1] = h1;
}

static inline void fnv32_3(const void *key, size_t len, uint32_t seed,
                           uint32_t *hashes)
{
  uint8_t *data = (uint8_t *)key;
  const uint8_t *const end = &data[len];
  uint32_t h0, h1, h2;

  h0 = seed;
  h1 = seed + 1;
  h2 = seed + 2;
  while (data < end) {
    h0 += (h0 << 1) + (h0 << 4) + (h0 << 7) + (h0 << 8) + (h0 << 24);
    h1 += (h1 << 1) + (h1 << 4) + (h1 << 7) + (h1 << 8) + (h1 << 24);
    h2 += (h2 << 1) + (h2 << 4) + (h2 << 7) + (h2 << 8) + (h2 << 24);
    h0 ^= *data;
    h1 ^= *data;
    h2 ^= *data;
    data++;
  }
  hashes[0] = h0;
  hashes[1] = h1;
  hashes[2] = h2;
}
