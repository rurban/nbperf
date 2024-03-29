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

static inline void fnv32_2(const void *key, size_t len, uint32_t seed0, uint32_t seed1,
                           uint32_t *hashes)
{
  uint8_t *data = (uint8_t *)key;
  const uint8_t *const end = &data[len];
  uint32_t h;

  h = seed0 ^ 0x811c9dc5;
  while (data < end) {
    h ^= *data++;
    h *= 0x1000193;
  }
  hashes[0] = h;

  h = seed1 ^ 0x811c9dc5;
  data = (uint8_t *)key;
  while (data < end) {
    h ^= *data++;
    h *= 0x1000193;
  }
  hashes[1] = h;
}
