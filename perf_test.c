/*
   Measure the generated perfect hash functions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "perf.h"

#ifdef _INTKEYS
extern uint32_t inthash(const int32_t);
#else
extern uint32_t hash (const char *, size_t);
#endif

#define MAX_LEN 80

int
main (int argc, char *argv[])
{
  char *in = argv[1];
  char *log = argv[2];
  unsigned size;
  char buf[MAX_LEN];
  FILE *f = fopen (in, "r");
  int ret = 0;
  unsigned i = 0;
  bool isword = strstr(in, "int") ? false : true;
  sscanf(argv[3], "%u", &size);

  uint64_t t = timer_start ();
 restart:
  while (fgets (buf, MAX_LEN, f))
    {
      size_t len = strlen (buf);
      if (len > 0 && buf[len - 1] == '\n')
        buf[--len] = '\0';
#ifdef _INTKEYS
      if (!inthash (atoi(buf)))
#else
      if (!hash (buf, len))
#endif
        {
          ret = 1;
          printf ("NOT in word set %s\n", buf);
        }
      i++;
      if (i > PERF_LOOP)
        break;
    }
  if (i < PERF_LOOP)
    {
      rewind (f);
      goto restart;
    }
  t = timer_end () - t;
  fclose(f);
  f = fopen (log, "a");
  fprintf(f, "%20u %20lu\n", size, t / PERF_LOOP);
  fclose(f);
  
  return ret;
}
