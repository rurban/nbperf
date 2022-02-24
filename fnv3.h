#include <stdint.h>

/* MPH needs at least 96 bit */
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
