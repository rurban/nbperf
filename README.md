# NAME

nbperf — compute a minimal perfect hash function

# SYNOPSIS

    nbperf [-fps] [-a algorithm] [-c utilisation] [-h hash] [-i iterations]
           [-m map-file] [-n name] [-o output] [input]

# DESCRIPTION

nbperf reads a number of keys one per line from standard input or
input.  It computes a minimal perfect hash function and writes it to
stdout or output.  The default algorithm is "**chm**".

The **-m** argument instructs **nbperf** to write the resulting key
mapping to _map-file_.  Each line gives the result of the hash
function for the corresponding input key.

The parameter _utilisation_ determines the space efficiency.

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

* **wyhash**:

  64bit version of wyhash, extended to 128bit.
  See [wyhash(3)](https://github.com/wangyi-fudan/wyhash).

* **fnv**:

  64bit fnv-1a. Only for -a chm.
  See [fnv(3)](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function).

* **fnv3**:

  2x 64bit fnv-1a. Also for -a chm3 and bdz.
  See [fnv(3)](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function).

The number of iterations can be limited with **-i**.  **nbperf**
outputs a function matching `uint32_t hash(const void * restrict, size_t)`
to stdout.  The function expects the key length as second
argument, for strings not including the terminating NUL.  It is the
responsibility of the caller to pass in only valid keys or compare the
resulting index to the key.  The function name can be changed using
**-n _name_**.  If the **-s** flag is specified, it will be static.

After each failing iteration, a dot is written to stderr.

**nbperf** checks for duplicate keys on the first iteration that passed
basic hash distribution tests.  In that case, an error message is
printed and the program terminates.

If the **-p** flag is specified, the hash function is seeded in a
stable way.  This may take longer than the normal random seed, but
ensures that the output is the same for repeated in‐ vocations as long
as the input is constant.

# EXIT STATUS

The **nbperf** utility exits 0 on success, and >0 if an error occurs.

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
