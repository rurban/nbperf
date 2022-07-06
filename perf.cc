/*
  Written by Reini Urban, 2022
*/

#include <cstddef>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "perf.h"
#include <sys/stat.h>
#include <vector>
#include <string>
#include <climits>
using namespace std;

const string options[] = {
	"-a chm -p",
	"-a chm3 -p",
	"-a bpz -p",
	"-a chm -h wyhash -p",
	"-a chm3 -h wyhash -p",
	"-a bpz -h wyhash -p",
	"-a chm -h fnv -p",
	"-a chm3 -h fnv3 -p",
	"-a bpz -h fnv3 -p",
	"-a chm -h crc -p",
	"-a chm3 -h crc -p",
	"-a bpz -h crc -p",
	"-I -p",
};
const uint32_t sizes[] = { 200, 400, 800, 2000, 4000, 8000, 20000, 100000,
	500000, 2000000 };

#ifndef WORDS
#define WORDS "/usr/share/dict/words"
#endif

// in seconds
#define NBPERF_TIMEOUT 15
#define PERF_PRE "_perf_"
static char perf_in[32] = PERF_PRE "2000000.nbperf";
static char perf_c[32] = PERF_PRE "2000000.c";
static char perf_exe[32] = PERF_PRE "2000000";

#define PICK(n) ((unsigned)rand()) % (n)
char buf[128];

static size_t random_word(char *buf, const size_t buflen) {
  const char* const alpha =
    "abcdefeghijklmnopqrstuvwxyz0123456789_ABCDEFEGHIJKLMNOPQRSTUVWXYZ";
  size_t len = 2 + PICK(buflen / 2);
  const size_t l = len;
  while(len--)
    *buf++ = alpha[PICK(sizeof(alpha)-1)];
  *buf = '\0';
  return l;
}

static int random_int(char *buf, const size_t buflen) {
  snprintf(buf, buflen, "%d", rand());
  return 1;
}

static inline void set_names (const size_t size, const bool isword) {
  const char *w = isword ? "word": "int";
  snprintf(perf_in, sizeof perf_in, "%s%s%zu%s", PERF_PRE, w, size, ".nbperf");
  snprintf(perf_c, sizeof perf_c, "%s%s%zu%s", PERF_PRE, w, size, ".c");
  snprintf(perf_exe, sizeof perf_exe, "%s%s%zu", PERF_PRE, w, size);
}

static inline void cleanup_files (void) {
  unlink(perf_in);
  unlink(perf_c);
  unlink(perf_exe);
}

static inline void create_set (const size_t size, const bool isword) {
  char cmd[160];
  static unsigned lines = 0;
  int ret;

  (void)ret;
  printf("Creating %s set of size %zu\n", isword ? "word" : "int", size);
  set_names (size, isword);
  if (!isword) {
    FILE *f = fopen("words.tmp","w");
    for (unsigned i=0; i<size; i++)
      {
        random_int(buf, 128);
        fprintf(f, "%s\n", buf);
      }
    fclose(f);
    snprintf(cmd, sizeof cmd, "sort -n <words.tmp | uniq >%s", perf_in);
    ret = system(cmd);
    unlink("words.tmp");
    return;
  }
  if (!lines)
    {
       ret = system("wc -l " WORDS " >words.wc");
       FILE *f = fopen("words.wc","r");
       ret = fscanf(f, "%u", &lines);
       unlink("words.wc");
     }
   if (lines < size)
     {
       FILE *f = fopen("words.tmp","w");
       for (unsigned i=0; i<size; i++)
         {
           random_word(buf, 128);
           fprintf(f, "%s\n", buf);
         }
       fclose(f);
       snprintf(cmd, sizeof cmd, "sort <words.tmp | uniq >%s", perf_in);
       ret = system(cmd);
       unlink("words.tmp");
     }
   else
     {
       snprintf(cmd, sizeof cmd, "head -n %zu " WORDS
                " | grep -v \",\" | sort | uniq >%s", size + 2, perf_in);
       ret = system(cmd);
     }
 }
static inline int run_nbperf (FILE* f, const uint32_t size, const char *cmd) {
   uint64_t t = timer_start();
   int ret = system(cmd);
   t = timer_end () - t;
   fprintf(f, "%20u %20lu\n", size, t / PERF_LOOP);
   return ret;
}
static inline int compile_result (bool needs_mi_vector, const char *d_intkeys) {
   char cmd[128];
   snprintf(cmd, sizeof cmd, "cc -O2 -I. %s %s perf_test.c %s -o %s",
            d_intkeys, perf_c, needs_mi_vector ? "mi_vector_hash.c" : "", perf_exe);
   printf("%s", cmd);
   return system(cmd);
}
static inline int run_result (const char *log, const uint32_t size) {
   char cmd[128];
   snprintf(cmd, sizeof cmd, "./%s %s %s %u", perf_exe, perf_in,
            log, size);
   return system(cmd);
 }

 // measure creation-time (only nbperf, not cc), and run-time.
 // also measure compiled-size.
 // use sample sizes from 20, 200, 2k, 20k, 200k, 2m for all variants.
int main (int argc, char **argv)
{
   FILE *comp = fopen("nbperf.log", "w");
   FILE *run = fopen("run.log", "w");
   FILE *fsize = fopen("size.log", "w");
   srand(0xbeef);

   for (auto option : options) {

     const bool isword = option.find("-I") == string::npos;
     if (argc > 1) {
       option.push_back(' ');
       option.append(argv[1]);
     }
     printf("------------------------ %s ------------------------\n", option.c_str());
     fprintf(comp, "option: %s\n", option.c_str());
     fprintf(run, "option: %s\n", option.c_str());
     //fclose(comp);
     fclose(run);
     fprintf(fsize, "option: %s\n", option.c_str());

     for(unsigned i=0; i<(sizeof sizes)/(sizeof *sizes); i++)
       {
         char cmd[128];
         int ret = 0;
         const uint32_t size = sizes[i];
         create_set (sizes[i], isword);
         set_names (size, isword);
         bool needs_mi_vector = option.find("-h ") == string::npos;
         //bool is_fnv = option.find("-h fnv") != string::npos;
         //if (size == 20000 && is_fnv)
         //   option += " -f";
	 snprintf(cmd, sizeof cmd, "timeout %d ./nbperf %s -o %s %s",
	     NBPERF_TIMEOUT, option.c_str(), perf_c, perf_in);
	 printf("size %u: %s\n", size, cmd);

         //#ifndef DUMMY
         ret = run_nbperf (comp, size, cmd);
         //#endif
         printf("  nbperf: %20u => %d\n", size, ret);
         if (ret != 0)
             continue;
#ifndef DUMMY
         ret = compile_result (needs_mi_vector, isword ? "" : "-D_INTKEYS");
         printf("  => %d\n", ret);
#else
         ret = 0;
#endif
         if (ret == 0) {
           struct stat st;
           stat (perf_exe, &st);
           printf("  %s (%lu):\n", perf_exe, st.st_size);
           fprintf(fsize, "%20u %20lu\n", size, st.st_size);

#ifndef DUMMY
           run_result ("run.log", size);
#endif
         }
       }
     run = fopen("run.log", "a");
     //comp = fopen("nbperf.log", "a");
   }

#if 1
   for(unsigned i=0; i<(sizeof sizes)/(sizeof *sizes); i++)
     {
       const uint32_t size = sizes[i];
       set_names (size, true);
       cleanup_files ();
       set_names (size, false);
       cleanup_files ();
     }
#endif
   fclose(comp);
   fclose(run);
   fclose(fsize);
 }
