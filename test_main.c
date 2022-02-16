#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
uint32_t hash(const void * __restrict key, size_t keylen);

int main(int argc, char **argv)
{
    char *input = "/usr/share/dict/words";
    char *line;
    ssize_t line_len;
    size_t line_allocated;
    FILE *f = fopen(input, "r");

    char *w = "englis";
    uint32_t h = hash(w, strlen(w));
    printf("%s: %x\n", w, h); // false-positive! englis == Luz's

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
	h = hash(line, strlen(line));
	//printf("%s: %x\n", line, h);
    }
    free(line);
    fclose(f);
}
