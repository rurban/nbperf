# $NetBSD: Makefile,v 1.1 2009/08/15 16:21:04 joerg Exp $

PROG=	nbperf
SRCS=	nbperf.c
SRCS+=	nbperf-bdz.c nbperf-chm.c nbperf-chm3.c	graph2.c graph3.c
HEADERS = mi_vector_hash.h mi_wyhash.h wyhash.h fnv3.h fnv.h
WORDS = /usr/share/dict/words
RANDBIG = _randbig

$(PROG): $(SRCS) mi_vector_hash.c mi_vector_hash.h wyhash.h nbtool_config.h
	$(CC) $(CFLAGS) -DHAVE_NBTOOL_CONFIG_H $(SRCS) mi_vector_hash.c -o $@

$(RANDBIG):
	seq 160000 | random > $@
_rand100:
	seq 200 | random > $@
_words: $(WORDS)
	cp $(WORDS) $@
_words1000:
	head -n 1000 $(WORDS) > $@

check: $(PROG) _words1000 _words $(RANDBIG)
	@echo test building a few with big sets
	./$(PROG) -o _test_chm.c $(WORDS)
	$(CC) $(CFLAGS) -I. -Dchm -o _test_chm _test_chm.c test_main.c mi_vector_hash.c
	./_test_chm $(WORDS)
	./$(PROG) -a bdz -o _test_bdz.c -m _words.map _words
	$(CC) $(CFLAGS) -I. -Dbdz -o _test_bdz _test_bdz.c test_main.c mi_vector_hash.c
	./_test_bdz _words
	./$(PROG) -a chm3 -o _test_chm3.c $(WORDS)
	$(CC) $(CFLAGS) -I. -Dchm3 -o _test_chm3 _test_chm3.c test_main.c mi_vector_hash.c
	./_test_chm3 $(WORDS)
	./$(PROG) -h wyhash -o _test_chm_wy.c $(WORDS)
	$(CC) $(CFLAGS) -I. -Dchm -o _test_chm_wy _test_chm_wy.c test_main.c
	./_test_chm_wy $(WORDS)
	./$(PROG) -h wyhash -a bdz -o _test_bdz_wy.c -m _words.map _words
	$(CC) $(CFLAGS) -I. -Dbdz -o _test_bdz_wy _test_bdz_wy.c test_main.c 
	./_test_bdz_wy _words
	./$(PROG) -h wyhash -a chm3 -o _test_chm3_wy.c $(WORDS)
	$(CC) $(CFLAGS) -I. -Dchm3 -o _test_chm3_wy _test_chm3_wy.c test_main.c
	./_test_chm3_wy $(WORDS)
	@echo
	@echo test building intkeys
	./$(PROG) -I -o _test_int.c $(RANDBIG)
	$(CC) $(CFLAGS) -I. -Dchm -D_INTKEYS -o _test_int _test_int.c test_main.c
	./_test_int $(RANDBIG)
	./$(PROG) -I -a bdz -o _test_intbdz.c -m $(RANDBIG).map $(RANDBIG)
	$(CC) $(CFLAGS) -I. -Dbdz -D_INTKEYS -o _test_intbdz _test_intbdz.c test_main.c
	./_test_intbdz $(RANDBIG)
	@echo
	@echo test all combinations and results with a small set
	CFLAGS="$(CFLAGS)" ./test
perf: $(PROG)
	@echo time all combinations with big sets
	./test 2000
	./test 5000
	./test 10000
	./test 20000
	./test 50000
	./test 100000
	./test 500000
	./test 1000000

clean:
	-rm -f $(PROG) _test_* test_{bdz,chm,chm3}* _words* _rand* a.out
install: $(PROG)
	sudo cp $(PROG) /usr/local/bin/
	sudo cp $(HEADERS) /usr/local/include/
	sudo cp $(PROG).1 /usr/local/share/man/man1/
