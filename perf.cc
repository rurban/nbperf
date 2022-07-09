/*
  Written by Reini Urban, 2022
*/

#include <cstddef>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>

#include "perf.h"

#include <vector>
#include <string>
#include <cstring>
#include <climits>
using namespace std;

vector<string> options = {
	"-a chm -p",
	"-a chm3 -p",
	"-a bdz -p",
	"-a chm -h wyhash -p",
	"-a chm3 -h wyhash -p",
	"-a bdz -h wyhash -p",
	"-a chm -h fnv -p",
	"-a chm3 -h fnv -p",
	"-a bpz -h fnv -p",
	"-a chm -h crc -p",
	"-a chm3 -h crc -p",
	"-a bpz -h crc -p",
	"-I -p",
	"-I -a chm3 -p",
	"-I -a bdz -p",
};
const uint32_t sizes[] = { 200, 400, 800, 2000, 4000, 8000, 20000, 100000,
	500000, 2000000 };

#ifndef WORDS
#define WORDS "/usr/share/dict/words"
#endif

// in seconds
#define NBPERF_TIMEOUT 15
#define PERF_PRE "_perf_"
static char perf_exe[32] = PERF_PRE "2000000";
static char perf_in[40] = PERF_PRE "2000000.nbperf";
static char perf_c[40] = PERF_PRE "2000000.c";

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

static inline void set_names (const char *alg, const char *hash,
                              const size_t size, const bool isword)
{
  const char *w = isword ? "": "int";
  snprintf(perf_exe, sizeof perf_exe, "%s%s%s_%s%zu", PERF_PRE, w, alg, hash, size);
  snprintf(perf_in, sizeof perf_in, "%s%s", perf_exe, ".nbperf");
  snprintf(perf_c, sizeof perf_c, "%s%s", perf_exe, ".c");
}

static inline void cleanup_files (void) {
  unlink(perf_in);
  unlink(perf_c);
  unlink(perf_exe);
}

static inline void create_set (const char *alg, const char *hash,
                               const size_t size, const bool isword) {
  char cmd[160];
  static unsigned lines = 0;
  int ret;

  (void)ret;
  set_names (alg, hash, size, isword);
  printf("Creating %s\n", perf_exe);
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
static inline int compile_result (bool needs_mi_vector, const char *defines) {
   char cmd[128];
   snprintf(cmd, sizeof cmd, "cc -O2 -I. %s %s perf_test.c %s -o %s",
            defines, perf_c, needs_mi_vector ? "mi_vector_hash.c" : "", perf_exe);
   printf("%s\n", cmd);
   return system(cmd);
}
static inline int run_result (const char *log, const uint32_t size) {
   char cmd[128];
   snprintf(cmd, sizeof cmd, "./%s %s %s %u", perf_exe, perf_in,
            log, size);
   //printf("%s\n", cmd);
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

   if (argc > 1) {
       options.clear();
       options.push_back("");
       for (int i = 1; i < argc; i++) {
           if (i > 1)
               options[0] += " ";
           options[0] += argv[i];
       }
   }

   for (auto option : options) {

     const bool isword = option.find("-I") == string::npos;
     const bool is_bdz = option.find("-a bdz") != string::npos;
     const bool is_chm3 = option.find("-a chm3") != string::npos;
     const bool is_chm = !is_bdz && !is_chm3;
     const char *alg = is_chm ? "chm"
         : is_chm3 ? "chm3"
         : is_bdz ? "bdz"
         : NULL;
     const char *hash;
     size_t hpos = option.find("-h ");
     if (hpos != string::npos) {
         string rest = option.substr(hpos+3);
         size_t ppos = rest.find(" ");
         if (ppos != string::npos)
             hash = rest.substr(0, ppos).c_str();
         else
             hash = rest.c_str();
     } else {
         hash = "";
     }
     printf("------------------------ %s ------------------------\n", option.c_str());
     fprintf(comp, "option: %s\n", option.c_str());
     fprintf(run, "option: %s\n", option.c_str());
     //fclose(comp);
     fclose(run);
     fprintf(fsize, "option: %s\n", option.c_str());

     for(unsigned i=0; i<(sizeof sizes)/(sizeof *sizes); i++)
       {
         char cmd[160];
         int ret = 0;
         const uint32_t size = sizes[i];
         create_set (alg, hash, sizes[i], isword);
         const bool needs_mi_vector = option.find("-h ") == string::npos;
         //const bool is_fnv = option.find("-h fnv") != string::npos;
         //if (size == 20000 && is_fnv)
         //   option += " -f";
         if (is_bdz)
             snprintf(cmd, sizeof cmd,
                      "timeout %d ./nbperf %s -m %s.map -o %s %s",
                      NBPERF_TIMEOUT, option.c_str(), perf_in, perf_c, perf_in);
         else
             snprintf(cmd, sizeof cmd,
                      "timeout %d ./nbperf %s -o %s %s",
                      NBPERF_TIMEOUT, option.c_str(), perf_c, perf_in);
	 printf("size %u: %s\n", size, cmd);

         //#ifndef DUMMY
         ret = run_nbperf (comp, size, cmd);
         //#endif
         printf("  nbperf: %20u => %d\n", size, ret);
         if (ret != 0)
             continue;
#ifndef DUMMY
         string defines = "";
         if (!isword)
             defines += "-D_INTKEYS ";
         if (is_bdz && isword)
             defines += "-Dbdz ";
#if defined __amd64__ || defined __x86_64__
         if (strcmp (hash, "crc") == 0)
             defines += "-march=native";
#endif
         ret = compile_result (needs_mi_vector, defines.c_str());
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
       for (auto alg: {"chm", "chm3", "bdz"}) {
           for (auto hash: {"", "wyhash", "fnv", "crc"}) {
               set_names (alg, hash, size, true);
               cleanup_files ();
           }
           set_names (alg, "", size, false);
           cleanup_files ();
       }
     }
#endif
   fclose(comp);
   fclose(run);
   fclose(fsize);
 }
