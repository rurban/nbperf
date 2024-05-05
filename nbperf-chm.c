/*	$NetBSD: nbperf-chm.c,v 1.4 2021/01/07 16:03:08 joerg Exp $	*/
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

#ifdef __FreeBSD__
#include <sys/cdefs.h>
__RCSID("$NetBSD: nbperf-chm.c,v 1.4 2021/01/07 16:03:08 joerg Exp $");
#endif

#include <assert.h>
#include <err.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph2.h"
#include "nbperf.h"

/*
 * A full description of the algorithm can be found in:
 * "An optimal algorithm for generating minimal perfect hash functions"
 * by Czech, Havas and Majewski in Information Processing Letters,
 * 43(5):256-264, October 1992.
 */

/*
 * The algorithm is based on random, acyclic graphs.
 *
 * Each edge in the represents a key.  The vertices are the reminder of
 * the hash function mod n.  n = cm with c > 2, otherwise the probability
 * of finding an acyclic graph is very low (for 2-graphs).  The constant
 * for 3-graphs is 1.24.
 *
 * After the hashing phase, the graph is checked for cycles.
 * A cycle-free graph is either empty or has a vertex of degree 1.
 * Removing the edge for this vertex doesn't change this property,
 * so applying this recursively reduces the size of the graph.
 * If the graph is empty at the end of the process, it was acyclic.
 *
 * The assignment step now sets g[i] := 0 and processes the edges
 * in reverse order of removal.  That ensures that at least one vertex
 * is always unvisited and can be assigned.
 */

struct state {
	struct SIZED(graph) graph;
	uint32_t *g;
	uint8_t *visited;
};

#if GRAPH_SIZE >= 3
static void
assign_nodes(struct state *state)
{
	struct SIZED(edge) * e;
	size_t i;
	uint32_t e_idx, v0, v1, v2, g;

	for (i = 0; i < state->graph.e; ++i) {
		e_idx = state->graph.output_order[i];
		e = &state->graph.edges[e_idx];
		if (!state->visited[e->vertices[0]]) {
			v0 = e->vertices[0];
			v1 = e->vertices[1];
			v2 = e->vertices[2];
		} else if (!state->visited[e->vertices[1]]) {
			v0 = e->vertices[1];
			v1 = e->vertices[0];
			v2 = e->vertices[2];
		} else {
			v0 = e->vertices[2];
			v1 = e->vertices[0];
			v2 = e->vertices[1];
		}
		g = e_idx - state->g[v1] - state->g[v2];
		if (g >= state->graph.e) {
			g += state->graph.e;
			if (g >= state->graph.e)
				g += state->graph.e;
		}
		state->g[v0] = g;
		state->visited[v0] = 1;
		state->visited[v1] = 1;
		state->visited[v2] = 1;
	}
}
#else
static void
assign_nodes(struct state *state)
{
	struct SIZED(edge) * e;
	size_t i;
	uint32_t e_idx, v0, v1, g;

	for (i = 0; i < state->graph.e; ++i) {
		e_idx = state->graph.output_order[i];
		e = &state->graph.edges[e_idx];
		if (!state->visited[e->vertices[0]]) {
			v0 = e->vertices[0];
			v1 = e->vertices[1];
		} else {
			v0 = e->vertices[1];
			v1 = e->vertices[0];
		}
		g = e_idx - state->g[v1];
		if (g >= state->graph.e)
			g += state->graph.e;
		state->g[v0] = g;
		state->visited[v0] = 1;
		state->visited[v1] = 1;
	}
}
#endif

static void
embed_data_string(struct nbperf *nbperf)
{
	fprintf(nbperf->output,
	    "%sconst char * const %s_keys[%" PRIu64 "] = {\n",
	    nbperf->static_hash ? "static " : "", nbperf->hash_name, nbperf->n);
	for (size_t i = 0; i < nbperf->n; i++) {
		if (!i)
			fprintf(nbperf->output, "\t");
		if ((i + 1) % 4)
			fprintf(nbperf->output, "\"%s\", ", nbperf->keys[i]);
		else
			fprintf(nbperf->output, "\"%s\",\t/* %lu */\n\t",
			    nbperf->keys[i], i + 1);
	}
	fprintf(nbperf->output, "};\n");
}

static void
embed_data_int(struct nbperf *nbperf, const char *hashtype)
{
	fprintf(nbperf->output, "%sconst %s %s_keys[%" PRIu64 "] = {\n",
	    nbperf->static_hash ? "static " : "", hashtype, nbperf->hash_name,
	    nbperf->n);
	for (size_t i = 0; i < nbperf->n; i++) {
		if (!i)
			fprintf(nbperf->output, "\t");
		if ((i + 1) % 10)
			fprintf(nbperf->output, "%ld, ", (long)nbperf->keys[i]);
		else
			fprintf(nbperf->output, "%ld,\t/* %lu */\n\t",
			    (long)nbperf->keys[i], i + 1);
	}
	fprintf(nbperf->output, "};\n");
}

static void
print_hash(struct nbperf *nbperf, struct state *state)
{
	uint32_t i, per_line;
	const char *g_type;
	int g_width;

	print_coda(nbperf);
	if (nbperf->embed_data && !nbperf->intkeys)
		fprintf(nbperf->output, "#include <string.h>\n");
	if (nbperf->intkeys) {
#if GRAPH_SIZE >= 3
		inthash4_addprint(nbperf);
#else
		inthash_addprint(nbperf);
#endif
	}
	const char *hashtype = nbperf->n >= 4294967295U ? "uint64_t" :
	    !nbperf->hashes16				? "uint32_t" :
							  "uint16_t";
	if (nbperf->embed_data) {
		if (nbperf->intkeys)
			embed_data_int(nbperf, hashtype);
		else
			embed_data_string(nbperf);
	}

	fprintf(nbperf->output, "%s%s\n", nbperf->static_hash ? "static " : "",
	    hashtype);
	if (!nbperf->intkeys)
		fprintf(nbperf->output,
		    "%s(const void * __restrict key, size_t keylen)\n",
		    nbperf->hash_name);
	else
		fprintf(nbperf->output, "%s(const %s key)\n", nbperf->hash_name,
		    hashtype);
	fprintf(nbperf->output, "{\n");
	if (state->graph.v >= 65536) {
		g_type = "uint32_t";
		g_width = 6;
		per_line = 8;
	} else if (state->graph.v >= 256) {
		g_type = "uint16_t";
		g_width = 4;
		per_line = 8;
	} else {
		g_type = "uint8_t";
		g_width = 2;
		per_line = 10;
	}
	if (nbperf->embed_data)
		fprintf(nbperf->output, "\t%s result;\n", g_type);
	fprintf(nbperf->output, "\tstatic const %s g[%" PRId32 "] = {\n",
	    g_type, state->graph.v);
	for (i = 0; i < state->graph.v; ++i) {
		if (nbperf->intkeys)
			fprintf(nbperf->output, "%s%*" PRIu32 ",%s",
			    (i % per_line == 0 ? "\t    " : " "), g_width,
			    state->g[i],
			    (i % per_line == per_line - 1 ? "\n" : ""));
		else
			fprintf(nbperf->output, "%s0x%0*" PRIx32 ",%s",
			    (i % per_line == 0 ? "\t    " : " "), g_width,
			    state->g[i],
			    (i % per_line == per_line - 1 ? "\n" : ""));
	}
	if (i % per_line != 0)
		fprintf(nbperf->output, "\n\t};\n");
	else
		fprintf(nbperf->output, "\t};\n");
	if (nbperf->hashes16) {
		if (nbperf->hash_size == 2 && nbperf->intkeys)
			fprintf(nbperf->output, "\tuint16_t h[%u];\n\n", 2);
		else
			fprintf(nbperf->output, "\tuint16_t h[%u];\n\n",
			    nbperf->hash_size * 2);
	} else
		fprintf(nbperf->output, "\tuint32_t h[%u];\n\n",
		    nbperf->hash_size);
	(*nbperf->print_hash)(nbperf, "\t", "key", "keylen", "h");

	if (nbperf->fastmod) {
		if (nbperf->hashes16) {
			fprintf(nbperf->output,
			    "\n\tconst uint32_t m = UINT32_C(0xFFFFFFFF) / %" PRIu16
			    " + 1;\n",
			    state->graph.v);
			fprintf(nbperf->output,
			    "\tconst uint32_t low0 = m * h[0];\n");
			fprintf(nbperf->output,
			    "\tconst uint32_t low1 = m * h[1];\n");
#if GRAPH_SIZE >= 3
			fprintf(nbperf->output,
			    "\tconst uint32_t low2 = m * h[2];\n");
#endif
			fprintf(nbperf->output,
			    "\th[0] = (uint16_t)((((uint64_t)low0) * %" PRIu32
			    ") >> 32);\n",
			    state->graph.v);
			fprintf(nbperf->output,
			    "\th[1] = (uint16_t)((((uint64_t)low1) * %" PRIu32
			    ") >> 32);\n",
			    state->graph.v);
#if GRAPH_SIZE >= 3
			fprintf(nbperf->output,
			    "\th[2] = (uint16_t)((((uint64_t)low2) * %" PRIu32
			    ") >> 32);\n",
			    state->graph.v);
#endif
		} else {
			fprintf(nbperf->output,
			    "\n\tconst uint64_t m = UINT64_C(0xFFFFFFFFFFFFFFFF) / %" PRIu32
			    " + 1;\n",
			    state->graph.v);
			fprintf(nbperf->output,
			    "\tconst uint64_t low0 = m * h[0];\n");
			fprintf(nbperf->output,
			    "\tconst uint64_t low1 = m * h[1];\n");
#if GRAPH_SIZE >= 3
			fprintf(nbperf->output,
			    "\tconst uint64_t low2 = m * h[2];\n");
#endif
			fprintf(nbperf->output,
			    "\th[0] = (uint32_t)((((__uint128_t)low0) * %" PRIu32
			    ") >> 64);\n",
			    state->graph.v);
			fprintf(nbperf->output,
			    "\th[1] = (uint32_t)((((__uint128_t)low1) * %" PRIu32
			    ") >> 64);\n",
			    state->graph.v);
#if GRAPH_SIZE >= 3
			fprintf(nbperf->output,
			    "\th[2] = (uint32_t)((((__uint128_t)low2) * %" PRIu32
			    ") >> 64);\n",
			    state->graph.v);
#endif
		}
	} else {
		fprintf(nbperf->output, "\n\th[0] = h[0] %% %" PRIu32 ";\n",
		    state->graph.v);
		fprintf(nbperf->output, "\th[1] = h[1] %% %" PRIu32 ";\n",
		    state->graph.v);
#if GRAPH_SIZE >= 3
		fprintf(nbperf->output, "\th[2] = h[2] %% %" PRIu32 ";\n",
		    state->graph.v);
#endif
	}

	if (state->graph.hash_fudge & 1)
		fprintf(nbperf->output, "\th[1] ^= (h[0] == h[1]);\n");

#if GRAPH_SIZE >= 3
	if (state->graph.hash_fudge & 2) {
		fprintf(nbperf->output,
		    "\th[2] ^= (h[0] == h[2] || h[1] == h[2]);\n");
		fprintf(nbperf->output,
		    "\th[2] ^= 2 * (h[0] == h[2] || h[1] == h[2]);\n");
	}
	fprintf(nbperf->output,
	    "\t%s (g[h[0]] + g[h[1]] + g[h[2]]) %% "
	    "%" PRIu32 ";\n",
	    nbperf->embed_data ? "result =" : "return", state->graph.e);
#else
	fprintf(nbperf->output,
	    "\t%s (g[h[0]] + g[h[1]]) %% "
	    "%" PRIu32 ";\n",
	    nbperf->embed_data ? "result =" : "return", state->graph.e);
#endif
	if (nbperf->embed_data) {
		if (!nbperf->intkeys)
			fprintf(nbperf->output,
			    "\treturn (strcmp(%s_keys[result], key) == 0)"
			    " ? result : (%s)-1;\n",
			    nbperf->hash_name, hashtype);
		else
			fprintf(nbperf->output,
			    "\treturn (%s_keys[result] == key) ? result : (%s)-1;\n",
			    nbperf->hash_name, hashtype);
	}
	fprintf(nbperf->output, "}\n");

	if (nbperf->map_output != NULL) {
		for (i = 0; i < state->graph.e; ++i)
			fprintf(nbperf->map_output, "%" PRIu32 "\n", i);
	}
}

int
#if GRAPH_SIZE >= 3
chm3_compute(struct nbperf *nbperf)
#else
chm_compute(struct nbperf *nbperf)
#endif
{
	struct state state;
	int retval = -1;
	uint32_t v, e, va;

	if (nbperf->n < 1)
		errx(1, "Not enough members, n < 1");
#if GRAPH_SIZE >= 3
	const double min_c = 1.24;
	if (nbperf->c == 0)
		nbperf->c = min_c;

	if (nbperf->c != -2 && nbperf->c < min_c)
		errx(1, "The argument for option -c must be at least 1.24");

	if (nbperf->hash_size < 3 && !nbperf->hashes16)
		errx(1, "The hash function must generate at least 3 values");
#else
	const double min_c = 2.0;
	if (nbperf->c == 0)
		nbperf->c = min_c;

	if (nbperf->c != -2 && nbperf->c < min_c)
		errx(1, "The argument for option -c must be at least 2");

	if (nbperf->hash_size < 2)
		errx(1, "The hash function must generate at least 2 values");
#endif

	(*nbperf->seed_hash)(nbperf);
	e = nbperf->n;
	v = nbperf->c * nbperf->n;

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
	if (v == min_c * nbperf->n)
		++v;
#if GRAPH_SIZE >= 3
	if (v < 8)
		v = 8;
	if (nbperf->allow_hash_fudging) // two more as reserve
		va = (v + 2) | 3;
	else
		va = v;
#else
	if (nbperf->allow_hash_fudging) // one more as reserve
		va = (v + 1) | 1;
	else
		va = v;
#endif

	state.g = calloc(v, sizeof(uint32_t));
	state.visited = calloc(v, sizeof(uint8_t));
	if (state.g == NULL || state.visited == NULL)
		err(1, "malloc failed");

	SIZED2(_setup)(&state.graph, v, e, va);
	if (SIZED2(_hash)(nbperf, &state.graph))
		goto failed;
	if (SIZED2(_output_order)(&state.graph))
		goto failed;
	assign_nodes(&state);
	print_hash(nbperf, &state);

	retval = 0;

failed:
	SIZED2(_free)(&state.graph);
	free(state.g);
	free(state.visited);
	return retval;
}
