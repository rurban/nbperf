/*
   Measure the generated perfect hash functions.
*/

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "perf.h"

#ifdef _INTKEYS
extern uint32_t inthash(const int32_t);
#else
extern uint32_t hash(const char *, size_t);
#endif

#define MAX_LEN 80

int
main(int argc, char *argv[])
{
	char *in = argv[1];
	char *log = argv[2];
	unsigned size;
	char buf[MAX_LEN];
	FILE *f = fopen(in, "r");
	int ret = 0;
	unsigned i = 0, j = 0;
	bool isword = strstr(in, "int") ? false : true;
	sscanf(argv[3], "%u", &size);

#ifdef bdz
	char mapfile[80];
	assert(strlen(in) < 80);
	strncpy(mapfile, in, 79);
	strcat(mapfile, ".map");

	uint32_t *map;
	size_t lines = 1000;
	map = calloc(lines, 4);
	i = 0;
	// read map file for the indices
	FILE *m = fopen(mapfile, "r");
	if (!m) {
		perror("fopen mapfile");
		exit(1);
	}
	errno = 0;
	while (1 == fscanf(m, "%u\n", &map[i]) && !errno) {
		i++;
		if (i >= lines) {
			// fprintf(stderr, "more than %lu lines in %s\n", lines,
			// mapfile);
			lines *= 2;
			map = realloc(map, lines * 4);
		}
	}
	fclose(m);
#endif

	uint64_t t = timer_start();
restart:
	j = 0;
	while (fgets(buf, MAX_LEN, f)) {
		size_t len = strlen(buf);
		if (len > 0 && buf[len - 1] == '\n')
			buf[--len] = '\0';
		uint32_t h =
#ifdef _INTKEYS
		    inthash(atoi(buf));
#else
		    hash(buf, len);
#endif
#ifndef bdz
		if (h != j) {
			ret = 1;
			printf("NOT in word set %s, hash %u != index %u\n", buf,
			    h, j);
			fclose(f);
			abort();
		}
#else
		if (h != map[j]) {
			ret = 1;
			printf("NOT in word set %s, hash %u != index %u\n", buf,
			    h, map[j]);
			fclose(f);
			free(map);
			abort();
		}
#endif
		i++;
		j++;
		if (i > PERF_LOOP)
			break;
	}
	if (i < PERF_LOOP) {
		rewind(f);
		goto restart;
	}
	t = timer_end() - t;
	fclose(f);
	f = fopen(log, "a");
	fprintf(f, "%20u %20lu\n", size, t / PERF_LOOP);
	fclose(f);
#ifdef bdz
	free(map);
#endif

	return ret;
}
