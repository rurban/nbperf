# $NetBSD: Makefile,v 1.1 2009/08/15 16:21:04 joerg Exp $

PROG=	nbperf
SRCS=	nbperf.c
SRCS+=	nbperf-bdz.c nbperf-chm.c nbperf-chm3.c	graph2.c graph3.c
HEADERS = mi_vector_hash.h mi_wyhash.h wyhash.h fnv3.h crc3.h
WORDS = /usr/share/dict/words
RANDBIG = _randbig
RANDHEX = _randhex

$(PROG): $(SRCS) mi_vector_hash.c mi_vector_hash.h wyhash.h nbtool_config.h VERSION
	$(CC) $(CFLAGS) -DVERSION="\"$(shell cat VERSION)\"" -DHAVE_NBTOOL_CONFIG_H $(SRCS) mi_vector_hash.c -o $@
perf: perf.h perf_test.c perf.cc
	c++ $(CFLAGS) perf.cc -o $@
VERSION: nbtool_config.h nbperf.c
	git describe --long --tags --always >$@

$(RANDBIG):
	seq 160000 | random > $@
$(RANDHEX):
	seq 2000 | random | xargs printf "0x%x\n" > $@
_rand100:
	seq 200 | random > $@
_words: $(WORDS)
	cp $(WORDS) $@
_words1000:
	head -n 1000 $(WORDS) > $@

check: $(PROG) _words1000 _words $(RANDBIG) $(RANDHEX)
	@echo test building a few with big sets
	./$(PROG) -o _test_chm.c $(WORDS)
	$(CC) $(CFLAGS) -I. -Dchm -o _test_chm _test_chm.c test_main.c mi_vector_hash.c
	./_test_chm $(WORDS)
	./$(PROG) -a chm3 -o _test_chm3.c $(WORDS)
	$(CC) $(CFLAGS) -I. -Dchm3 -o _test_chm3 _test_chm3.c test_main.c mi_vector_hash.c
	./_test_chm3 $(WORDS)
	./$(PROG) -a bdz -o _test_bdz.c -m _words.map _words
	$(CC) $(CFLAGS) -I. -Dbdz -o _test_bdz _test_bdz.c test_main.c mi_vector_hash.c
	./_test_bdz _words
	./$(PROG) -M -o _test_Mchm.c $(WORDS)
	$(CC) $(CFLAGS) -I. -Dchm -o _test_Mchm _test_Mchm.c test_main.c mi_vector_hash.c
	./_test_Mchm $(WORDS)
	./$(PROG) -M -o _test_Mchm3.c $(WORDS)
	$(CC) $(CFLAGS) -I. -Dchm3 -o _test_Mchm3 _test_Mchm3.c test_main.c mi_vector_hash.c
	./_test_Mchm3 $(WORDS)
	./$(PROG) -M -a bdz -o _test_Mbdz.c -m _words.map _words
	$(CC) $(CFLAGS) -I. -Dbdz -o _test_Mbdz _test_Mbdz.c test_main.c mi_vector_hash.c
	./_test_Mbdz _words
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
	./$(PROG) -I -o _test_inthex.c $(RANDHEX)
	$(CC) $(CFLAGS) -I. -Dchm -D_INTKEYS -o _test_inthex _test_inthex.c test_main.c
	./_test_inthex $(RANDHEX)
	./$(PROG) -IM -o _test_Mint.c $(RANDBIG)
	$(CC) $(CFLAGS) -I. -Dchm -D_INTKEYS -o _test_Mint _test_Mint.c test_main.c
	./_test_Mint $(RANDBIG)
	@echo
	@echo test all combinations and results with a small set
	CFLAGS="$(CFLAGS)" ./test
run-perf: $(PROG) perf
	@echo time all combinations with all set sizes
	git describe --long --tags --always >VERSION
	./perf && ./perf_img.sh && rm VERSION

clean:
	-rm -f $(PROG) _test_* test_{bdz,chm,chm3}* _words* _rand* a.out \
	  perf _perf_*
install: $(PROG)
	sudo cp $(PROG) /usr/local/bin/
	sudo cp $(HEADERS) /usr/local/include/
	sudo cp $(PROG).1 /usr/local/share/man/man1/
