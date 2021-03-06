#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#ifdef _INTKEYS
uint32_t inthash(const uint32_t key);
#else
uint32_t hash(const void * __restrict key, size_t keylen);
#endif
#define PERF_ROUNDS  100000

int main(int argc, char **argv)
{
    char *input;
    char *line;
    ssize_t line_len;
    size_t line_allocated;
    FILE *f;
    int verbose = 0;
    unsigned i = 1;
    uint32_t h;
#if defined bdz
    uint32_t *map;
#elif defined _INTKEYS
    int32_t *map;
#endif

    if (argc > 1 && strcmp(argv[i], "-v") == 0)
        verbose = 1, i++;
    input = i > 0 ? argv[i] : "_words1000";

#ifdef bdz
    char mapfile[80];
    assert(strlen(input) < 80);
    strncpy(mapfile, input, 79);
    strcat(mapfile, ".map");
#endif
#ifndef PERF
# ifdef _INTKEYS
    h = inthash(1);
    if (verbose)
        printf("%u: %u\n", 1, h);
# else
    char *w = "englis";
    h = hash(w, strlen(w));
    if (verbose)
        printf("%s: %x\n", w, h); // false-positive! englis == Luz's
# endif
#endif

#if defined bdz
    size_t lines = 1000;
    map = calloc (lines, 4);
    i = 0;
    // read map file for the indices
    f = fopen(mapfile, "r");
    if (!f) {
	perror("fopen mapfile");
	exit(1);
    }
    errno = 0;
    while (1 == fscanf(f, "%u\n", &map[i]) && !errno) {
        i++;
        if (i >= lines) {
            fprintf(stderr, "more than %lu lines in %s\n", lines, mapfile);
            lines *= 2;
            map = realloc (map, lines * 4);
        }
    }
    fclose(f);
#elif defined _INTKEYS
    size_t lines = 1000;
    map = calloc (lines, 4);
    i = 0;
    // read input file for the indices
    f = fopen(input, "r");
    if (!f) {
	perror("fopen input");
	exit(1);
    }
    errno = 0;
    while (1 == fscanf(f, "%d\n", &map[i]) && !errno) {
        i++;
        if (i >= lines) {
            fprintf(stderr, "more than %lu lines in %s\n", lines, input);
            lines *= 2;
            map = realloc (map, lines * 4);
        }
    }
    fclose(f);
#endif
    f = fopen(input, "r");
    if (!f) {
	perror("fopen input");
	exit(1);
    }
    i = 0;
    line = NULL;
    line_allocated = 0;
    while ((line_len = getline(&line, &line_allocated, f)) != -1) {
	if (line_len && line[line_len - 1] == '\n') {
	    --line_len;
	    line[line_len] = '\0';
	}
#ifdef _INTKEYS
        int32_t l = atoi(line);
#endif
#ifdef PERF
        for (int j=0; j < PERF_ROUNDS; j++) {
#endif
#ifdef _INTKEYS
            h = inthash(l);
#else
            h = hash(line, strlen(line));
#endif
#ifdef PERF
        } // perf loop
#endif
	if (verbose)
#if defined _INTKEYS || defined bdz
            printf("%s: %d == %d\n", line, (int)h, (int)map[i]);
#else
            printf("%s: %u\n", line, h);
#endif

#ifndef PERF
# if (defined chm || defined chm3) && !defined _INTKEYS
        assert(h == i);
#else
#if defined _INTKEYS && !defined bdz
	if (l != map[h] && verbose)
		printf("%s: %d != %d (%u)\n", line, l, map[h], h);
	assert(l == map[h]);
#else // bdz
	assert(h == map[i]);
#endif
#endif
#endif
	i++;
    }
    free(line);
#if defined _INTKEYS || defined bdz
    free(map);
#endif
    fclose(f);
}
