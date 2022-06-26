#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#ifdef _INTKEYS
uint32_t inthash(const uint32_t key);
#else
uint32_t hash(const void * __restrict key, size_t keylen);
#endif

int main(int argc, char **argv)
{
    char *input = argc > 1 ? argv[1] : "_words1000";
    char *line;
    ssize_t line_len;
    size_t line_allocated;
    FILE *f;
    int verbose = argc > 2;
    unsigned i = 0;
    uint32_t h;

#ifdef _INTKEYS
# ifdef bdz
    const char *mapfile = "_rand100.map";
# endif
    h = inthash(1);
    printf("%u: %u\n", 1, h);
#else
# ifdef bdz
    const char *mapfile = "_words1000.map";
# endif
    char *w = "englis";
    h = hash(w, strlen(w));
    printf("%s: %x\n", w, h); // false-positive! englis == Luz's
#endif

#if defined bdz
    uint32_t map[1000];
    // read map file for the indices
    f = fopen(mapfile, "r");
    while (fscanf(f, "%u\n", &map[i])) {
        i++;
        if (i >= 1000)
            break;
    }
    fclose(f);
#endif
    i = 0;
    f = fopen(input, "r");
    if (!f) {
	perror("fopen");
	exit(1);
    }
    line = NULL;
    line_allocated = 0;
    while ((line_len = getline(&line, &line_allocated, f)) != -1) {
	if (line_len && line[line_len - 1] == '\n') {
	    --line_len;
	    line[line_len] = '\0';
	}
#ifdef _INTKEYS
        h = inthash(atoi(line));
#else
        h = hash(line, strlen(line));
#endif
	if (verbose || i < 5)
            printf("%s: %u\n", line, h);
#if defined chm || defined chm3
        assert(h == i);
#else
        assert(h == map[i]);
#endif
        i++;
    }
    free(line);
    fclose(f);
}
