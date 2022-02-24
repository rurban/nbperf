#include <stdint.h>

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
