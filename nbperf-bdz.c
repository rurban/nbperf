/*	$NetBSD: nbperf-bdz.c,v 1.10 2021/01/07 16:03:08 joerg Exp $	*/
/*-
 * Copyright (c) 2009, 2012 The NetBSD Foundation, Inc.
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

#ifdef __FreeBSD__
#include <sys/cdefs.h>
__RCSID("$NetBSD: nbperf-bdz.c,v 1.10 2021/01/07 16:03:08 joerg Exp $");
#endif

#include <err.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "nbperf.h"

/*
 * A full description of the algorithm can be found in:
 * "Simple and Space-Efficient Minimal Perfect Hash Functions"
 * by Botelho, Pagh and Ziviani, proceeedings of WADS 2007.
 */

/*
 * The algorithm is based on random, acyclic 3-graphs.
 *
 * Each edge in the represents a key.  The vertices are the reminder of
 * the hash function mod n.  n = cm with c > 1.23.  This ensures that
 * an acyclic graph can be found with a very high probality.
 *
 * An acyclic graph has an edge order, where at least one vertex of
 * each edge hasn't been seen before.   It declares the first unvisited
 * vertex as authoritive for the edge and assigns a 2bit value to unvisited
 * vertices, so that the sum of all vertices of the edge modulo 4 is
 * the index of the authoritive vertex.
 */

#define GRAPH_SIZE 3
#include "graph2.h"

struct state {
	struct SIZED(graph) graph;
	//uint32_t r;
	uint8_t *visited;
	uint32_t *ranking;
	uint8_t *g;
        unsigned g_size;
        unsigned visited_size;
        unsigned ranking_size;
};

static const uint8_t bitmask[] = {
        1, 1 << 1,  1 << 2,  1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7
};
static const uint8_t valuemask[] = { 0xfc, 0xf3, 0xcf, 0x3f };

#define GETBIT(visited, i) ((visited[i >> 3] & bitmask[i & 7]) >> (i & 7))
#define SETBIT(visited, i) (visited[i >> 3] |= bitmask[i & 7])
#define CLRBIT(visited, i) (visited[i >> 3] &= ~bitmask[i & 7])
// g is an array of 2 bits
#define GETI2(g, i) ((uint8_t)((g[i >> 2] >> ((i & 3) << 1U)) & 3))
#define SETI2(g, i, v) (g[i >> 2] &= (uint8_t)((v << ((i & 3) << 1)) | valuemask[i & 3]))
#define UNVISITED 3

static const uint8_t bits_per_byte[256] = {
        4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
        4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
        4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
        3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
        4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
        4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
        4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
        3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
        4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
        4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
        4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
        3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
        3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
        3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
        3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
        2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 1, 1, 1, 0
};

static void
assign_nodes(struct state *state)
{
	struct SIZED(edge) *e;
	size_t i, j;

	memset(state->g, 0xff, state->g_size);
	//for (i = 0; i < state->graph.v; ++i)
	//	SETI2(state->g, i, UNVISITED);

	for (i = 0; i < state->graph.e; ++i) {
                
		j = state->graph.output_order[i];
		e = &state->graph.edges[j];
                const uint32_t v0 = e->vertices[0];
                const uint32_t v1 = e->vertices[1];
                const uint32_t v2 = e->vertices[2];
		DEBUGP("B:%u %u %u -- %u %u %u edge %lu\n", v0, v1, v2,
                       GETI2(state->g, v0), GETI2(state->g, v1), GETI2(state->g, v2), j);
		if (!GETBIT(state->visited, v0))
                {
			if (!GETBIT(state->visited,v1))
			{
				SETI2(state->g, v1, UNVISITED);
				SETBIT(state->visited, v1);
			}
			if (!GETBIT(state->visited,v2))
			{
				SETI2(state->g, v2, UNVISITED);
				SETBIT(state->visited, v2);
			}
			SETI2(state->g, v0, (6 - (GETI2(state->g, v1) + GETI2(state->g, v2))) % 3);
			SETBIT(state->visited, v0);
		} else if (!GETBIT(state->visited, v1))
                {
			if (!GETBIT(state->visited,v2))
			{
				SETI2(state->g, v2, UNVISITED);
				SETBIT(state->visited, v2);
			}
			SETI2(state->g, v1, (7 - (GETI2(state->g, v0) + GETI2(state->g, v2))) % 3);
			SETBIT(state->visited, v1);
		} else
                {
			SETI2(state->g, v2, (8 - (GETI2(state->g, v0) + GETI2(state->g, v1))) % 3);
			SETBIT(state->visited, v2);
		}
		DEBUGP("A:%u %u %u -- %u %u %u\n", v0, v1, v2,
                       GETI2(state->g, v0), GETI2(state->g, v1), GETI2(state->g, v2));
	}
}

// build the ranking table: number of bits in g
static void
ranking(struct state *state)
{
	size_t i, j;
        const uint32_t k = 1U << 7; // for 32bit ranking
        state->ranking_size = ceil(state->graph.v / k) + 1;
        state->ranking = (uint32_t*)calloc(state->ranking_size, sizeof(uint32_t));
        state->ranking[0] = 0;
        uint32_t offset = 0U, sum = 0U, size = (k >> 2U),
                nbytes_total = state->g_size * 4, nbytes;
	for (i=1; i < state->ranking_size; i++) {
		nbytes = size < nbytes_total ? size : nbytes_total;
		for (j = 0; j < nbytes; j++) {
			sum += bits_per_byte[*(state->g + offset + j)];
		}
		state->ranking[i] = sum;
		offset += nbytes;
		nbytes_total -= size;
                DEBUGP("ranking[%zu]: %u\n", i, sum);
	}
}

static void
print_hash(struct nbperf *nbperf, struct state *state)
{
	//uint64_t sum;
	size_t i;
        const char *g_type;
        int g_width, per_line;

	print_coda(nbperf);
	fprintf(nbperf->output, "#include <string.h>\n");
	if (nbperf->intkeys) {
		inthash4_addprint(nbperf);
	}
	fprintf(nbperf->output,
	    "#define GETI2(g, i) ((uint8_t)((g[i >> 2] >> ((i & 3) << 1U)) & 3))\n\n");
	if (nbperf->embed_data) {
		fprintf(nbperf->output, "%sconst char * const %s_keys[%" PRIu64 "] = {\n",
                        nbperf->static_hash ? "static " : "",
                        nbperf->hash_name, nbperf->n);
                for (size_t i = 0; i < nbperf->n; i++) {
                        if (!i)
                                fprintf(nbperf->output, "\t");
                        if ((i + 1) % 4)
                                fprintf(nbperf->output, "\"%s\", ", nbperf->keys[i]);
                        else
                                fprintf(nbperf->output, "\"%s\",\t/* %lu */\n\t", nbperf->keys[i], i+1);
                }
                fprintf(nbperf->output, "};\n\n");
	}

        const int has_ranking = state->ranking_size > 1 || state->ranking[0] != 0;
	const char* hashtype = nbperf->n >= 4294967295U ? "uint64_t" : "uint32_t";
	fprintf(nbperf->output, "%s%s\n",
                nbperf->static_hash ? "static " : "", hashtype);
	if (!nbperf->intkeys)
		fprintf(nbperf->output,
		    "%s(const void * __restrict key, size_t keylen)\n",
		    nbperf->hash_name);
	else
		fprintf(nbperf->output,	"%s(const int32_t key)\n", nbperf->hash_name);
	fprintf(nbperf->output, "{\n");

        if (nbperf->n >= 4294967295U) {
                g_type = "uint64_t";
                g_width = 16;
                per_line = 2;
        }
        else if (nbperf->n >= 65536) {
                g_type = "uint32_t";
                g_width = 10;
                per_line = 5;
        }
        else if (nbperf->n >= 256) {
                g_type = "uint16_t";
                g_width = 5;
                per_line = 8;
        }
        else {
                g_type = "uint8_t";
                g_width = 3;
                per_line = 10;
        }
	if (nbperf->embed_map) {
                assert(state->graph.e == nbperf->n);
                fprintf(nbperf->output,
                        "\tstatic const %s output_order[%zu] = {\n",
                        g_type, nbperf->n);
		for (i = 0; i < state->graph.e; ++i) {
                        if (!i)
                                fprintf(nbperf->output, "\t    ");
			fprintf(nbperf->output, "%*u,",
                                g_width, state->graph.output_order[i]);
                        if ((i + 1) % per_line == 0)
                                fprintf(nbperf->output, "\n\t    ");
                }
                fprintf(nbperf->output, "};\n");
	}
	if (has_ranking) {
		fprintf(nbperf->output,
		    "\tstatic const uint8_t ranking_lut[256] = {\n");
		fprintf(nbperf->output,
		    "\t\t4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,\n"
		    "\t\t4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,\n"
		    "\t\t4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,\n"
		    "\t\t3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,\n"
		    "\t\t4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,\n"
		    "\t\t4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,\n"
		    "\t\t4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,\n"
		    "\t\t3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,\n"
		    "\t\t4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,\n"
		    "\t\t4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,\n"
		    "\t\t4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,\n"
		    "\t\t3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,\n"
		    "\t\t3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,\n"
		    "\t\t3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,\n"
		    "\t\t3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,\n"
		    "\t\t2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 1, 1, 1, 0 };\n");
	}
	fprintf(nbperf->output, "\tstatic const uint8_t g[%" PRIu8 "] = {\n",
	    state->g_size + 1);
	for (i = 0; i < state->g_size; ++i) {
                if (!i)
                        fprintf(nbperf->output, "\t    ");
                fprintf(nbperf->output, " 0x%x,",
                        state->g[i]);
                if ((i + 1) % 10 == 0)
                        fprintf(nbperf->output, "\n\t    ");
	}
	fprintf(nbperf->output, "%s\t 0x00 };\n", (i % 10 ? "\n" : ""));

        //const uint32_t b = 7;       // number of bits of k
        //const uint32_t k = 1U << b; // kth index in ranking
        if (has_ranking) {
                fprintf(nbperf->output,
                        "\tstatic const uint32_t ranking[%" PRId32 "] = {\n",
                        state->ranking_size);
                for (i = 0; i < state->ranking_size; ++i) {
                        if (!i)
                                fprintf(nbperf->output, "\t    ");
                        fprintf(nbperf->output, "%" PRIu32 ", ", state->ranking[i]);
                        if ((i + 1) % 5 == 0)
                                fprintf(nbperf->output, "\n\t    ");
                }
                fprintf(nbperf->output, "\n\t};\n");
                fprintf(nbperf->output, "\tconst uint32_t b = 7;\n"
                        "\tuint32_t index, base_rank, idx_v, idx_b, end_idx_b;\n");
        }
        if (nbperf->embed_data)
                fprintf(nbperf->output, "\t%s result;\n", hashtype);
	fprintf(nbperf->output, "\tuint32_t vertex;\n");
        if (nbperf->hashes16)
                fprintf(nbperf->output, "\tuint16_t h[%u];\n\n", nbperf->hash_size * 2);
        else
                fprintf(nbperf->output, "\tuint32_t h[%u];\n\n", nbperf->hash_size);

	(*nbperf->print_hash)(nbperf, "\t", "key", "keylen", "h");

	if (nbperf->fastmod) { // TODO and state->graph.v is not power of 2
		if (nbperf->hashes16) {
			fprintf(nbperf->output,
			    "\n\tconst uint32_t m = UINT32_C(0xFFFFFFFF) / %" PRIu16
			    " + 1;\n",
			    state->graph.v);
			fprintf(nbperf->output,
			    "\tconst uint32_t low0 = m * h[0];\n");
			fprintf(nbperf->output,
			    "\tconst uint32_t low1 = m * h[1];\n");
			fprintf(nbperf->output,
			    "\tconst uint32_t low2 = m * h[2];\n");
			fprintf(nbperf->output,
			    "\th[0] = (uint16_t)((((uint64_t)low0) * %" PRIu32
			    ") >> 32);\n",
			    state->graph.v);
			fprintf(nbperf->output,
			    "\th[1] = (uint16_t)((((uint64_t)low1) * %" PRIu32
			    ") >> 32);\n",
			    state->graph.v);
			fprintf(nbperf->output,
			    "\th[2] = (uint16_t)((((uint64_t)low2) * %" PRIu32
			    ") >> 32);\n",
			    state->graph.v);
		} else {
			fprintf(nbperf->output,
			    "\n\tconst uint64_t m = UINT64_C(0xFFFFFFFFFFFFFFFF) / %" PRIu32
			    " + 1;\n",
			    state->graph.v);
			fprintf(nbperf->output,
			    "\tconst uint64_t low0 = m * h[0];\n");
			fprintf(nbperf->output,
			    "\tconst uint64_t low1 = m * h[1];\n");
			fprintf(nbperf->output,
			    "\tconst uint64_t low2 = m * h[2];\n");
			fprintf(nbperf->output,
			    "\th[0] = (uint32_t)((((__uint128_t)low0) * %" PRIu32
			    ") >> 64);\n",
			    state->graph.v);
			fprintf(nbperf->output,
			    "\th[1] = (uint32_t)((((__uint128_t)low1) * %" PRIu32
			    ") >> 64);\n",
			    state->graph.v);
			fprintf(nbperf->output,
			    "\th[2] = (uint32_t)((((__uint128_t)low2) * %" PRIu32
			    ") >> 64);\n",
			    state->graph.v);
		}
	} else {
		fprintf(nbperf->output, "\n\th[0] = h[0] %% %" PRIu32 ";\n",
                        state->graph.v);
		fprintf(nbperf->output, "\th[1] = h[1] %% %" PRIu32 " + %" PRIu32 ";\n",
                        state->graph.v, state->graph.r);
		fprintf(nbperf->output, "\th[2] = h[2] %% %" PRIu32 " + (%" PRIu32 " >> 1);\n",
                        state->graph.v, state->graph.r);
	}

	if (state->graph.hash_fudge & 1)
		fprintf(nbperf->output, "\th[1] ^= (h[0] == h[1]);\n");

	if (state->graph.hash_fudge & 2) {
		fprintf(nbperf->output,
		    "\th[2] ^= (h[0] == h[2] || h[1] == h[2]);\n");
		fprintf(nbperf->output,
		    "\th[2] ^= 2 * (h[0] == h[2] || h[1] == h[2]);\n");
	}

        fprintf(nbperf->output,
                "\tconst uint8_t i = GETI2(g, h[0]) + GETI2(g, h[1]) + GETI2(g, h[2]);\n");
        fprintf(nbperf->output,
                "\tvertex = h[i %% 3] %% %" PRIu32 ";\n\n", state->graph.v);
        if (has_ranking) {
                // rank lookup: vertex -> base_rank
                fprintf(nbperf->output,
                        "\tindex = vertex >> b;\n"
                        "\tbase_rank = ranking[index];\n"
                        "\tidx_v = index << b;\n"
                        "\tidx_b = idx_v >> 2;\n"
                        "\tend_idx_b = vertex >> 2;\n"
                        "\twhile (idx_b < end_idx_b)\n"
                        "\t    base_rank += ranking_lut[*(g + idx_b++)];\n"
                        "\tidx_v = idx_b << 2;\n"
                        "\twhile (idx_v < vertex)\n"
                        "\t{\n"
                        "\t      if (GETI2(g, idx_v) != 3) base_rank++;\n"
                        "\t      idx_v++;\n"
                        "\t}\n");
                if (nbperf->embed_map)
                        fprintf(nbperf->output, "\t%s (%s)output_order[base_rank];\n",
                                nbperf->embed_data ? "result =" : "return", hashtype);
                else
                        fprintf(nbperf->output, "\t%s base_rank;\n",
                                nbperf->embed_data ? "result =" : "return");
        } else {
                if (nbperf->embed_map)
                        fprintf(nbperf->output, "\t%s (%s)output_order[vertex];\n",
                                nbperf->embed_data ? "result =" : "return", hashtype);
                else
                        fprintf(nbperf->output, "\t%s (%s)vertex;\n",
                                nbperf->embed_data ? "result =" : "return", hashtype);
        }
        if (nbperf->embed_data)
                fprintf(nbperf->output, "\treturn (strcmp(%s_keys[result], key) == 0)"
                        " ? result : (%s)-1;\n",
                        nbperf->hash_name, hashtype);
	fprintf(nbperf->output, "}\n");

	if (nbperf->map_output != NULL) {
		for (i = 0; i < state->graph.e; ++i)
			fprintf(nbperf->map_output, "%" PRIu32 "\n",
			    state->graph.output_order[i]);
	}
}

int
bpz_compute(struct nbperf *nbperf)
{
	struct state state = {NULL};
	int retval = -1;
	uint32_t v, e, va, r;
        const double min_c = 1.23;

	if (nbperf->c == 0)
		nbperf->c = min_c;
	if (nbperf->c != -2 && nbperf->c < min_c)
		errx(1, "The argument for option -c must be at least 1.24");
	if (nbperf->hash_size < 3)
                errx(1, "The hash function must generate at least 3 values");

	(*nbperf->seed_hash)(nbperf);
	e = nbperf->n;
	r = ceil((nbperf->c * nbperf->n) / 3);
	if ((r % 2) == 0)
                r += 1;
        if (r == 1) // workaround for small key sets
		r = 3;
	v = r * 3;
	if (min_c * nbperf->n > v)
		v += 3;

        /* With -c -2 prefer v as next power of two.
           But with bigger sets the space overhead might be too much.
         */
        if (nbperf->c == -2) {
                v = 1 << (uint32_t)ceil(log2((double)nbperf->n));
                nbperf->c = (v * 1.0) / nbperf->n;
                // c might still be too small
                while (nbperf->c < min_c) {
                        v *= 2;
                        nbperf->c = (v * 1.0) / nbperf->n;
                }
        }
        
	if (v < 9)
		v = 9;
	if (nbperf->allow_hash_fudging) // two more as reserve
		va = (v + 2) | 3;
	else
		va = v;

	graph3_setup(&state.graph, v, e, va);

        state.g_size = ceil(v / 4);
	state.g = calloc(state.g_size, sizeof(uint32_t));
        state.visited_size = (v >> 3) + 1;
	state.visited = calloc(state.visited_size, sizeof(uint32_t));

	if (state.g == NULL || state.visited == NULL)
		err(1, "malloc failed");
	if (SIZED2(_hash)(nbperf, &state.graph))
		goto failed;
	if (SIZED2(_output_order)(&state.graph))
		goto failed;
	assign_nodes(&state);
	ranking(&state);
	print_hash(nbperf, &state);

	retval = 0;

failed:
	SIZED2(_free)(&state.graph);
	free(state.g);
        free(state.visited);
        free(state.ranking);
	return retval;
}
