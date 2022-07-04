/*	$NetBSD: nbperf.c,v 1.7 2021/01/12 14:21:18 joerg Exp $	*/
/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * Copyright (c) 2022 Reini Urban
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger.
 * Integer keys and more hashes were added by Reini Urban.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: nbperf.c,v 1.7 2021/01/12 14:21:18 joerg Exp $")

#include <endian.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define HAVE_CRC
// clang-format off
#include "nbperf.h"
// clang-format on
#include "fnv.h"
#include "fnv3.h"
#include "mi_vector_hash.h"
#include "mi_wyhash.h"
#ifdef HAVE_CRC
#include "crc2.h"
#include "crc3.h"
#endif

static /*__dead*/
    void
    usage(void)
{
	fprintf(stderr,
	    "%s [-fIps] [-c utilisation] [-i iterations] [-n name] "
	    "[-h hash] [-o output] input\n",
	    "nbperf" /*getprogname()*/);
	exit(1);
}

#if HAVE_NBTOOL_CONFIG_H
#define arc4random() rand()
#endif

static void
mi_vector_hash_seed(struct nbperf *nbperf)
{
	static uint32_t predictable_counter;
	if (nbperf->predictable)
		nbperf->seed[0] = predictable_counter++;
	else
		nbperf->seed[0] = arc4random();
}

static void
mi_vector_hash_compute(struct nbperf *nbperf, const void *key, size_t keylen,
    uint32_t hashes[3])
{
	mi_vector_hash(key, keylen, nbperf->seed[0], hashes);
}

void
mi_vector_hash_print(struct nbperf *nbperf, const char *indent, const char *key,
    const char *keylen, const char *hash)
{
	fprintf(nbperf->output,
	    "%smi_vector_hash(%s, %s, UINT32_C(0x%08" PRIx32 "), %s%s);\n",
	    indent, key, keylen, nbperf->seed[0],
		nbperf->hashes16 ? "(uint32_t *)" : "", hash);
}

static void
wyhash_seed(struct nbperf *nbperf)
{
	static uint32_t predictable_counter;
	if (nbperf->predictable) {
		nbperf->seed[0] = predictable_counter++;
		nbperf->seed[1] = predictable_counter++;
	} else {
		nbperf->seed[0] = arc4random();
		nbperf->seed[1] = arc4random();
	}
	if (nbperf->seed[0] == UINT32_C(0x14cc886e) ||
	    nbperf->seed[0] == UINT32_C(0xd637dbf3))
		nbperf->seed[0]++;
	if (nbperf->seed[1] == UINT32_C(0x14cc886e) ||
	    nbperf->seed[1] == UINT32_C(0xd637dbf3))
		nbperf->seed[1]++;
}

static void
wyhash2_compute(struct nbperf *nbperf, const void *key, size_t keylen,
    uint32_t *hashes)
{
	uint64_t seed = *(uint64_t *)nbperf->seed;
	mi_wyhash2(key, keylen, seed, hashes);
}
static void
wyhash4_compute(struct nbperf *nbperf, const void *key, size_t keylen,
    uint32_t *hashes)
{
	uint64_t seed = *(uint64_t *)nbperf->seed;
	mi_wyhash4(key, keylen, seed, (uint64_t *)hashes);
}
static void
wyhash_print(struct nbperf *nbperf, const char *indent, const char *key,
    const char *keylen, const char *hash)
{
	uint64_t seed = *(uint64_t *)nbperf->seed;
        if (nbperf->compute_hash == wyhash2_compute)
		fprintf(nbperf->output,
		    "%smi_wyhash2(%s, %s, UINT64_C(0x%" PRIx64
		    "), (uint32_t*)%s);\n",
		    indent, key, keylen, seed, hash);
        else
		fprintf(nbperf->output,
		    "%smi_wyhash4(%s, %s, UINT64_C(0x%" PRIx64
		    "), (uint64_t*)%s);\n",
		    indent, key, keylen, seed, hash);
}
static void
fnv_compute(struct nbperf *nbperf, const void *key, size_t keylen,
    uint32_t *hashes)
{
	uint64_t seed = *(uint64_t *)nbperf->seed;
	fnv(key, keylen, seed, (uint64_t *)hashes);
}
static void
fnv_print(struct nbperf *nbperf, const char *indent, const char *key,
    const char *keylen, const char *hash)
{
	uint64_t seed = *(uint64_t *)nbperf->seed;
	fprintf(nbperf->output,
	    "%sfnv(%s, %s, UINT64_C(0x%" PRIx64 "), (uint64_t*)%s);\n", indent,
	    key, keylen, seed, hash);
}
static void
fnv_seed(struct nbperf *nbperf)
{
	static uint32_t predictable_counter = 0;
	if (nbperf->predictable) {
		nbperf->seed[0] = (uint32_t)(0x85ebca6b *
		    ++predictable_counter);
		nbperf->seed[1] = predictable_counter;
	} else {
		nbperf->seed[0] = arc4random();
		nbperf->seed[1] = arc4random();
	}
}

static void
fnv3_compute(struct nbperf *nbperf, const void *key, size_t keylen,
    uint32_t *hashes)
{
	uint64_t seed = *(uint64_t *)nbperf->seed;
	fnv3(key, keylen, seed, (uint64_t *)hashes);
}
static void
fnv3_print(struct nbperf *nbperf, const char *indent, const char *key,
    const char *keylen, const char *hash)
{
	uint64_t seed = *(uint64_t *)nbperf->seed;
	fprintf(nbperf->output,
	    "%sfnv3(%s, %s, UINT64_C(0x%" PRIx64 "), (uint64_t*)%s);\n", indent,
	    key, keylen, seed, hash);
}
#ifdef HAVE_CRC
static void
crc_compute(struct nbperf *nbperf, const void *key, size_t keylen,
    uint32_t *hashes)
{
	uint64_t seed = *(uint64_t *)nbperf->seed;
        // produces three 32bit hashes from 2 seeds
	crc3(key, keylen, seed, (uint64_t *)hashes);
}
static void
crc2_compute(struct nbperf *nbperf, const void *key, size_t keylen,
    uint32_t *hashes)
{
	uint64_t seed = *(uint64_t *)nbperf->seed;
        // produces one 64bit hash
	crc2(key, keylen, seed, (uint64_t *)hashes);
}
static void
crc_print(struct nbperf *nbperf, const char *indent, const char *key,
    const char *keylen, const char *hash)
{
	uint64_t seed = *(uint64_t *)nbperf->seed;
	fprintf(nbperf->output,
	    "%scrc%s(%s, %s, UINT64_C(0x%" PRIx64 "), (uint64_t*)%s);\n",
	    indent, nbperf->compute_hash == crc2_compute ? "2" : "3", key,
	    keylen, seed, hash);
}
#endif

void
inthash_compute(struct nbperf *nbperf, const void *key, size_t keylen,
    uint32_t *hashes)
{
	(void)keylen;
	*(uint64_t *)hashes = (int64_t)key *
		/* mult factor from CityHash to reach into 2nd 32bit slot */
		(UINT64_C(0x9DDFEA08EB382D69) + (uint64_t)nbperf->seed[0]) +
	    nbperf->seed[1];
}
void
inthash4_compute(struct nbperf *nbperf, const void *key, size_t keylen,
    uint32_t *hashes)
{
	(void)keylen;
	*(uint64_t *)hashes = (int64_t)key *
		/* mult factor from CityHash to reach into 2nd 32bit slot, but
		   not the 3rd */
		(UINT64_C(0x9DDFEA08EB382D69) + (uint64_t)nbperf->seed[0]) +
	    nbperf->seed[1];
	*((uint64_t *)hashes + 1) = (uint64_t)key * (uint64_t)nbperf->seed[0] +
	    nbperf->seed[1];
}
void
inthash_print(struct nbperf *nbperf, const char *indent, const char *key,
    const char *keylen, const char *hash)
{
	(void)keylen;
	fprintf(nbperf->output, "%s_inthash(%s, (uint64_t*)%s);\n", indent, key,
	    hash);
}

void
print_coda(struct nbperf *nbperf)
{
	int saw_dash = 0;
	fprintf(nbperf->output, "/* generated with rurban/nbperf ");
	if (nbperf->allow_hash_fudging) {
		fprintf(nbperf->output, "%sf", saw_dash ? "" : "-");
		saw_dash = 1;
	}
	if (nbperf->intkeys) {
		fprintf(nbperf->output, "%sI", saw_dash ? "" : "-");
		saw_dash = 1;
	}
	if (nbperf->predictable) {
		fprintf(nbperf->output, "%sp", saw_dash ? "" : "-");
		saw_dash = 1;
	}
	if (nbperf->static_hash) {
		fprintf(nbperf->output, "%ss", saw_dash ? "" : "-");
		saw_dash = 1;
	}
	/*
	if (nbperf->c > 0)
	    fprintf(nbperf->output, " -c %f", nbperf->c);
	*/
	if (nbperf->input)
		fprintf(nbperf->output, " %s", nbperf->input);
	fprintf(nbperf->output,
	    " */\n/* seed[0]: %" PRIu32 ", seed[1]: %" PRIu32 " */\n",
	    nbperf->seed[0], nbperf->seed[1]);

	if (!nbperf->intkeys)
		fprintf(nbperf->output, "#include <stdlib.h>\n");
	fprintf(nbperf->output, "#include <stdint.h>\n");
	if (nbperf->hash_header)
		fprintf(nbperf->output, "#include \"%s\"\n\n",
		    nbperf->hash_header);
}

static void
set_hash(struct nbperf *nbperf, const char *arg)
{
	if (strcmp(arg, "mi_vector_hash") == 0) {
		nbperf->hash_size = 3; // needed for chm3 and bdz
		nbperf->hash_header = "mi_vector_hash.h";
		nbperf->seed_hash = mi_vector_hash_seed;
		nbperf->compute_hash = mi_vector_hash_compute;
		nbperf->print_hash = mi_vector_hash_print;
#ifdef ASAN
		errx(1, "This hash function is disallowed with address-sanitizer");
#else
# if defined(__has_feature)
#  if __has_feature(address_sanitizer)
		errx(1, "This hash function is disallowed with address-sanitizer");
#  endif
# endif
#endif
		return;
	} else if (strcmp(arg, "wyhash") == 0) {
		nbperf->hash_size = 8;
		nbperf->compute_hash = wyhash4_compute;
		nbperf->hash_header = "mi_wyhash.h";
		nbperf->seed_hash = wyhash_seed;
		nbperf->print_hash = wyhash_print;
		return;
	} else if (strcmp(arg, "fnv") == 0) {
		nbperf->hash_size = 2;
		nbperf->compute_hash = fnv_compute;
		nbperf->hash_header = "fnv.h";
		nbperf->seed_hash = fnv_seed;
		nbperf->print_hash = fnv_print;
		return;
	} else if (strcmp(arg, "fnv3") == 0) {
		nbperf->hash_size = 4;
		nbperf->compute_hash = fnv3_compute;
		nbperf->hash_header = "fnv3.h";
		nbperf->seed_hash = fnv_seed;
		nbperf->print_hash = fnv3_print;
		return;
	} else if (strcmp(arg, "inthash") == 0) {
		nbperf->hash_header = NULL;
		nbperf->hash_size = 2;
		nbperf->compute_hash = inthash_compute;
		nbperf->seed_hash = fnv_seed;
		nbperf->print_hash = inthash_print;
		return;
	}
#ifdef HAVE_CRC
	else if (strcmp(arg, "crc") == 0) {
		nbperf->hash_size = 4;
		nbperf->compute_hash = crc_compute;
		nbperf->hash_header = "crc3.h";
		nbperf->seed_hash = fnv_seed;
		nbperf->print_hash = crc_print;
		return;
	}
#endif
	if (nbperf->hash_size > NBPERF_MAX_HASH_SIZE)
		errx(1, "Hash function creates too many output values");
	if (nbperf->hash_size < NBPERF_MIN_HASH_SIZE)
		errx(1, "Hash function creates not enough output values");
	errx(1, "Unknown hash function: %s", arg);
}

int
main(int argc, char **argv)
{
	struct nbperf nbperf = {
		.c = 0,
		.hash_name = "hash",
		.map_output = NULL,
		.input = NULL,
		.output = NULL,
		.static_hash = 0,
		.check_duplicates = 0,
		.has_duplicates = 0,
		.allow_hash_fudging = 0,
		.intkeys = 0,
	};
	FILE *input;
	size_t curlen = 0, curalloc = 0;
	char *line, *eos;
	ssize_t line_len;
	size_t line_allocated;
	const char **keys = NULL;
	size_t *keylens = NULL;
	uint32_t max_iterations = 0xffffffU;
	long long tmp;
	int looped, ch;
	int (*build_hash)(struct nbperf *) = chm_compute;

#ifdef ASAN
	set_hash(&nbperf, "wyhash");
#else
# if defined(__has_feature)
#  if __has_feature(address_sanitizer)
	set_hash(&nbperf, "wyhash");
#  else
	set_hash(&nbperf, "mi_vector_hash");
#  endif
# else
	set_hash(&nbperf, "mi_vector_hash");
# endif
#endif

	while ((ch = getopt(argc, argv, "a:c:fh:i:m:n:o:psI")) != -1) {
		switch (ch) {
		case 'a':
			/* Accept bdz as alias for netbsd-6 compat. */
			if (strcmp(optarg, "chm") == 0) {
				build_hash = chm_compute;
				//nbperf.hash_size = 2;
			} else if (strcmp(optarg, "chm3") == 0)
				build_hash = chm3_compute;
			else if ((strcmp(optarg, "bpz") == 0 ||
				     strcmp(optarg, "bdz") == 0))
				build_hash = bpz_compute;
			else
				errx(1, "Unsupport algorithm: %s", optarg);
			break;
		case 'c':
			errno = 0;
			nbperf.c = strtod(optarg, &eos);
			if (errno || eos[0] || !nbperf.c)
				errx(2, "Invalid argument for -c");
			break;
		case 'f':
			nbperf.allow_hash_fudging = 1;
			break;
		case 'h':
			set_hash(&nbperf, optarg);
			break;
		case 'i':
			errno = 0;
			tmp = strtoll(optarg, &eos, 0);
			if (errno || eos == optarg || eos[0] || tmp < 0 ||
			    tmp > 0xffffffffU)
				errx(2,
				    "Iteration count must be "
				    "a 32bit integer");
			max_iterations = (uint32_t)tmp;
			break;
		case 'I':
			nbperf.intkeys = 1;
			set_hash(&nbperf, "inthash");
			if (strcmp(nbperf.hash_name, "hash") == 0)
				nbperf.hash_name = "inthash";
			break;
		case 'm':
			if (nbperf.map_output)
				fclose(nbperf.map_output);
			nbperf.map_output = fopen(optarg, "w");
			if (nbperf.map_output == NULL)
				err(2, "cannot open map file");
			break;
		case 'n':
			nbperf.hash_name = optarg;
			break;
		case 'o':
			if (nbperf.output)
				fclose(nbperf.output);
			nbperf.output = fopen(optarg, "w");
			if (nbperf.output == NULL)
				err(2, "cannot open output file");
			break;
		case 'p':
			nbperf.predictable = 1;
			break;
		case 's':
			nbperf.static_hash = 1;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc > 1)
		usage();

	//if (build_hash == chm_compute && nbperf.hash_size == 3)
	//	nbperf.hash_size = 2; // wyhash not
	if (nbperf.intkeys &&
	    (build_hash == bpz_compute || build_hash == chm3_compute)) {
		nbperf.hash_size = 4;
		nbperf.compute_hash = inthash4_compute;
	}
	if (build_hash == bpz_compute && nbperf.hash_size < 3)
		errx(1, "Unsupport algorithm: %s", "bpz");
	//if (build_hash == chm3_compute && nbperf.intkeys)
	//	errx(1, "Unsupport algorithm: %s", "chm3");

	if (argc == 1) {
		input = fopen(argv[0], "r");
		if (input == NULL)
			err(1, "can't open input file");
		nbperf.input = argv[0];
	} else {
		input = stdin;
		nbperf.input = "<stdin>";
	}

	if (nbperf.output == NULL)
		nbperf.output = stdout;

	line = NULL;
	line_allocated = 0;
	while ((line_len = getline(&line, &line_allocated, input)) != -1) {
		if (line_len && line[line_len - 1] == '\n')
			--line_len;
		if (curlen == curalloc) {
			if (curalloc < 256)
				curalloc = 256;
			else
				curalloc += curalloc;
			keys = realloc(keys, curalloc * sizeof(*keys));
			if (keys == NULL)
				err(1, "realloc failed");
			keylens = realloc(keylens, curalloc * sizeof(*keylens));
			if (keylens == NULL)
				err(1, "realloc failed");
		}
		if (nbperf.intkeys) {
			uint64_t i;
			errno = 0;
			i = strtod(line, &eos);
			if (errno || (eos[0] != '\n' && eos[0] != '\r'))
				errx(2,
				    "Invalid integer key \"%s\" at line %lu of %s",
				    line, curlen, nbperf.input);
			memcpy(&keys[curlen], &i, sizeof(char *));
		} else if (line_len % 4 == 0) {
			if ((keys[curlen] = strndup(line, line_len)) == NULL)
				err(1, "strndup failed");
		} else {
			ssize_t padded_len = line_len + (4 - (line_len % 4));
			if ((keys[curlen] = calloc(padded_len, 1)) == NULL)
				err(1, "calloc failed");
			memcpy((void *)keys[curlen], line, line_len);
		}

		keylens[curlen] = line_len;
		++curlen;
	}
	free(line);

	if (input != stdin)
		fclose(input);

	nbperf.n = curlen;
	nbperf.keys = keys;
	nbperf.keylens = keylens;
	/* with less keys we can use smaller and esp. faster 16bit hashes */
	if (curlen <= 65534) {
		nbperf.hashes16 = 1;
		if (nbperf.intkeys > 0) {
			nbperf.compute_hash = inthash_compute;
		} else if (nbperf.compute_hash == crc_compute) {
			nbperf.hash_size = 2;
			nbperf.hash_header = "crc2.h";
			nbperf.compute_hash = crc2_compute;
		} else if (nbperf.compute_hash == wyhash4_compute) {
			nbperf.hash_size = 2;
			nbperf.compute_hash = wyhash2_compute;
                }
	}

	looped = 0;
	int rv;
	for (;;) {
		rv = (*build_hash)(&nbperf);
		if (!rv)
			break;
		if (nbperf.has_duplicates) {
			fputc('\n', stderr);
			errx(1, "Duplicate keys detected");
		}
		fputc('.', stderr);
		if (!looped)
			nbperf.check_duplicates = 1;
		looped = 1;
		if (max_iterations == 0xffffffffU)
			continue;
		if (--max_iterations == 0) {
			fputc('\n', stderr);
			errx(1, "Iteration count reached");
		}
	}
	if (looped)
		fputc('\n', stderr);

	free(keylens);
	if (!nbperf.intkeys)
		for (unsigned i = 0; i < curlen; i++)
			free((void *)keys[i]);
	free(keys);
	return 0;
}
