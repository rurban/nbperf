# NAME

nbperf — compute a minimal perfect hash function

# SYNOPSIS

    nbperf [-fIMps] [-a algorithm] [-c utilisation] [-h hash] [-i iterations]
           [-m map-file] [-n name] [-o output] [input]

# DESCRIPTION

nbperf reads a number of keys one per line from standard input or
input.  It computes a minimal perfect hash function and writes it to
stdout or output.  The default algorithm is "**chm**".

The **-m** argument instructs **nbperf** to write the resulting key
mapping to _map-file_.  Each line gives the result of the hash
function for the corresponding input key.

The **-c** _utilisation_ argument determines the space efficiency.

Supported arguments for **-a**:

* **chm**:

  This results in an order preserving minimal perfect hash function.
  The _utilisation_ must be at least 2, the default.  The number of
  iterations needed grows if the utilisation is very near to 2.

* **chm3**:

  Similar to _chm_.  The resulting hash function needs three instead of
  two table lookups when compared to _chm_.  The _utilisation_ must be at
  least 1.24, the default.  This makes the output for _chm3_ noticeably
  smaller than the output for _chm_.

* **bpz**:

  This results in a non-order preserving minimal perfect hash function.
  Output size is approximately 2.79 bit per key for the default value of
  _utilisation_, 1.24.  This is also the smallest supported value.

Supported arguments for **-h**:

* **mi_vector_hash**:

  Platform-independent version of Jenkins parallel hash.  This accesses the
  strings in 4-bytes, which will trip valgrind and asan. See `mi_vector_hash(3)`.
  The default hash.

* **wyhash**:

  64-bit version of wyhash, extended to 128-bit.
  See [wyhash(3)](https://github.com/wangyi-fudan/wyhash).
  This is usually the fastest and most stable hash.

* **fnv**:

  64-bit or 128-bit variants of FNV-1a.
  See [fnv(3)](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function).
  This is not as good as expected yet.

* **crc**:

  Various SW and HW variants of iSCSI CRC32c.
  See [crc(3)](https://en.wikipedia.org/wiki/Cyclic_redundancy_check).
  This is not as good as expected yet. Ensure that you use -msse4, -mcrc or
  -march=native or similar for the much faster HW variant.

The number of iterations can be limited with **-i**.  **nbperf**
outputs a function matching `uint32_t hash(const void * restrict, size_t)`
to stdout.  The function expects the key length as second
argument, for strings not including the terminating NUL.  It is the
responsibility of the caller to pass in only valid keys or compare the
resulting index to the key.  The function name can be changed using
**-n _name_**.  If the **-s** flag is specified, it will be static.

If the **-f** flag is specified, hash fudging will be allowed. I.e.
slightly slower hashes.

If the **-I** flag is specified, the keys interpreted as integers,
and the generated hash function will have the signature
`uint32_t inthash (const int32_t key)`.
With -I you can skip empty lines or comment lines with '#'.
The **-h** flag is ignored then.

If the **-M** flag is specified, the modulo operator uses an optimized
Lemire fastmod variant. This requires native 128bit multiplication and
`__uint128_t` with bigger keysets.

If the **-p** flag is specified, the hash function is seeded in a
stable way.  This may take longer than the normal random seed, but
ensures that the output is the same for repeated in‐ vocations as long
as the input is constant.

After each failing iteration, a dot is written to stderr.

**nbperf** checks for duplicate keys on the first iteration that passed
basic hash distribution tests.  In that case, an error message is
printed and the program terminates.

# EXIT STATUS

The **nbperf** utility exits 0 on success, and >0 if an error occurs.

# COMPARISON

1.
This nbperf variant extends the original NetBSD code with support
for much better, safer and faster hashes. The original
`mi_vector_hash` reads past strings for up to 3 bytes, and when such a
string is at the end of a page, it might fail. wyhash and on x86_64 or
aarch64 hardware-assisted CRC allows much faster run-time hashing,
whilst not reading out of bounds.

2.
It supports **integer keys** with much faster run-time lookup than
for strings.  It is currently the only perfect hash generator for
integers, such as e.g. for fast unicode property lookups.

3.
It supports smaller 16bit hashes when the key size fits into 16bit,
i.e. max 65534 keys.  This enables faster run-time hashing. The
internal hash must not generate 3 independent 32bit values, 3 16bit
values are enough, which is usually twice as fast.

4.
It supports Lemire's fastmod.

5.
**gperf** integer key support and perfect hash support for larger
keysizes is still in work. **PostgresQL** uses a perl script for its
CHM support with strings only.  **perl5** uses slower run-time perfect
hashes for its unicode tables.  **cmph** is only suitable for huge
keysizes, and carries a heavy run-time overhead, plus needs the
run-time library.

# SEE ALSO

* gperf(1)
* The Perfect::Hash perl library
* `PerfectHash.pm` in PostgreSQL, and a similar lib in perl5
* [Bob Jenkins's Minimal Perfect Hashing](https://github.com/rurban/jenkins-minimal-perfect-hash)
* `mi_vector_hash(3)` in NetBSD
* [wyhash](https://github.com/wangyi-fudan/wyhash)
* [CHM](http://cmph.sourceforge.net/chm.html) Algorithm
* [BPZ](http://cmph.sourceforge.net/bdz.html) Algorithm
* [smhasher](https://github.com/rurban/smhasher) hash comparisons

# AUTHORS

* Jörg Sonnenberger
* Reini Urban
