/*	$NetBSD: nbperf.h,v 1.5 2021/01/07 16:03:08 joerg Exp $	*/
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

// number of u32 results
#define NBPERF_MIN_HASH_SIZE 2
#define NBPERF_MAX_HASH_SIZE 3

struct nbperf {
	FILE *output;
	FILE *map_output;
	const char *input;
	const char *hash_name;
	const char *hash_header;
	size_t n;
	const char *__restrict *keys;
	const size_t *keylens;
	unsigned static_hash : 1;
	unsigned allow_hash_fudging : 1;
	unsigned predictable : 1;
	unsigned intkeys : 1;
	unsigned check_duplicates : 1;
	unsigned has_duplicates : 1;
	unsigned hashes16 : 1; // 16bit hashes only

	double c;

	size_t hash_size; /* number of 32bit hashes */
	void (*seed_hash)(struct nbperf *);
	void (*print_hash)(struct nbperf *, const char *, const char *,
	    const char *, const char *);
	void (*compute_hash)(struct nbperf *, const void *, size_t, uint32_t *);
	uint32_t seed[1];
};

int chm_compute(struct nbperf *);
int chm3_compute(struct nbperf *);
int bpz_compute(struct nbperf *);
void print_coda(struct nbperf *);
void mi_vector_hash_print(struct nbperf *nbperf, const char *indent, const char *key,
                          const char *keylen, const char *hash);
