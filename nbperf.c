/*	$NetBSD: nbperf.c,v 1.7 2021/01/12 14:21:18 joerg Exp $	*/
/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger.
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
__RCSID("$NetBSD: nbperf.c,v 1.7 2021/01/12 14:21:18 joerg Exp $");

#include <endian.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "nbperf.h"
#include "mi_vector_hash.h"
#include "mi_wyhash.h"

static int predictable;

static /*__dead*/
void usage(void)
{
	fprintf(stderr,
	    "%s [-ps] [-c utilisation] [-i iterations] [-n name] "
	    "[-h hash] [-o output] input\n",
                "nbperf" /*getprogname()*/);
	exit(1);
}

#if HAVE_NBTOOL_CONFIG_H
#define	arc4random() rand()
#endif

static void
mi_vector_hash_seed_hash(struct nbperf *nbperf)
{
	static uint32_t predictable_counter;
	if (predictable)
		nbperf->seed[0] = predictable_counter++;
	else
		nbperf->seed[0] = arc4random();
}

static void
mi_vector_hash_compute(struct nbperf *nbperf, const void *key, size_t keylen,
    uint32_t *hashes)
{
	mi_vector_hash(key, keylen, nbperf->seed[0], hashes);
}

static void
mi_vector_hash_print_hash(struct nbperf *nbperf, const char *indent,
    const char *key, const char *keylen, const char *hash)
{
	fprintf(nbperf->output,
	    "%smi_vector_hash(%s, %s, 0x%08" PRIx32 "U, %s);\n",
	    indent, key, keylen, nbperf->seed[0], hash);
}

static void
wyhash_seed_hash(struct nbperf *nbperf)
{
	static uint32_t predictable_counter;
	if (predictable)
		nbperf->seed[0] = predictable_counter++;
	else
		nbperf->seed[0] = arc4random();
	if (nbperf->seed[0] == UINT32_C(0x14cc886e) ||
	    nbperf->seed[0] == UINT32_C(0xd637dbf3))
		nbperf->seed[0]++;
}

static void
wyhash_compute(struct nbperf *nbperf, const void *key, size_t keylen,
	       uint32_t *hashes)
{
	mi_wyhash3(key, keylen, *(uint64_t*)nbperf->seed, (uint64_t*)hashes);
}

static void
wyhash_print_hash(struct nbperf *nbperf, const char *indent,
		  const char *key, const char *keylen, const char *hash)
{
	fprintf(nbperf->output,
	    "%smi_wyhash3(%s, %s, 0x%08" PRIx32 "U, (uint64_t*)%s);\n",
	    indent, key, keylen, nbperf->seed[0], hash);
}

static void
set_hash(struct nbperf *nbperf, const char *arg)
{
	if (strcmp(arg, "mi_vector_hash") == 0) {
		nbperf->hash_size = 3; // needed for chm3 and bdz
		nbperf->hash_header = "mi_vector_hash.h";
		nbperf->seed_hash = mi_vector_hash_seed_hash;
		nbperf->compute_hash = mi_vector_hash_compute;
		nbperf->print_hash = mi_vector_hash_print_hash;
		return;
	} else if (strcmp(arg, "wyhash") == 0) {
		nbperf->hash_size = 4; // i.e. 2 u64 values
		nbperf->hash_header = "mi_wyhash.h";
		nbperf->seed_hash = wyhash_seed_hash;
		nbperf->compute_hash = wyhash_compute;
		nbperf->print_hash = wyhash_print_hash;
		return;
	}
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
	    .output = NULL,
	    .static_hash = 0,
	    .check_duplicates = 0,
	    .has_duplicates = 0,
	    .allow_hash_fudging = 0,
	};
	FILE *input;
	size_t curlen = 0, curalloc = 0;
	char *line, *eos;
	ssize_t line_len;
	size_t line_allocated;
	const void **keys = NULL;
	size_t *keylens = NULL;
	uint32_t max_iterations = 0xffffffU;
	long long tmp;
	int looped, ch;
	int (*build_hash)(struct nbperf *) = chm_compute;

	set_hash(&nbperf, "mi_vector_hash");

	while ((ch = getopt(argc, argv, "a:c:fh:i:m:n:o:ps")) != -1) {
		switch (ch) {
		case 'a':
			/* Accept bdz as alias for netbsd-6 compat. */
			if (strcmp(optarg, "chm") == 0)
				build_hash = chm_compute;
			else if (strcmp(optarg, "chm3") == 0)
				build_hash = chm3_compute;
			else if ((strcmp(optarg, "bpz") == 0 ||
				  strcmp(optarg, "bdz") == 0)
				 && nbperf.hash_size >= 3)
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
			if (errno || eos == optarg || eos[0] ||
			    tmp < 0 || tmp > 0xffffffffU)
				errx(2, "Iteration count must be "
				    "a 32bit integer");
			max_iterations = (uint32_t)tmp;
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
			predictable = 1;
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

	if (argc == 1) {
		input = fopen(argv[0], "r");
		if (input == NULL)
			err(1, "can't open input file");
	} else
		input = stdin;

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
			keylens = realloc(keylens,
			    curalloc * sizeof(*keylens));
			if (keylens == NULL)
				err(1, "realloc failed");
		}
	    	if (line_len % 4 == 0) {
		    if ((keys[curlen] = strndup(line, line_len)) == NULL)
			err(1, "strndup failed");
		}
		else {
		    ssize_t padded_len = line_len + (4 - (line_len % 4));
		    if ((keys[curlen] = calloc(padded_len, 1)) == NULL)
			err(1, "calloc failed");
		    memcpy((void*)keys[curlen], line, line_len);
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

	free (keylens);
	for (int i=0; i < curlen; i++)
	    free ((void*)keys[i]);
	free (keys);
	return 0;
}
