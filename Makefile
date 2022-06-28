# $NetBSD: Makefile,v 1.1 2009/08/15 16:21:04 joerg Exp $

PROG=	nbperf
SRCS=	nbperf.c
SRCS+=	nbperf-bdz.c nbperf-chm.c nbperf-chm3.c	graph2.c graph3.c
HEADERS = mi_vector_hash.h mi_wyhash.h wyhash.h fnv3.h fnv.h
WORDS = /usr/share/dict/words
RAND100 = _rand100
RANDBIG = _randbig
CC = cc
CFLAGS = -O2 -g

MACHINE := $(shell uname -m)
ifeq (x86_64,$(MACHINE))
CFLAGS += -march=native
endif

$(PROG): $(SRCS) mi_vector_hash.c mi_vector_hash.h wyhash.h nbtool_config.h
	$(CC) $(CFLAGS) -I. -DHAVE_NBTOOL_CONFIG_H $(SRCS) mi_vector_hash.c -o $@

$(RANDBIG):
	seq 160000 | random > $(RANDBIG)

check: $(PROG) $(RANDBIG)
	@echo test building a few with big sets
	./$(PROG) -o _test_chm.c $(WORDS)
	$(CC) $(CFLAGS) -c -I. _test_chm.c
	./$(PROG) -a bdz -o _test_bdz.c $(WORDS)
	$(CC) $(CFLAGS) -c -I. _test_bdz.c
	./$(PROG) -a chm3 -o _test_chm3.c $(WORDS)
	$(CC) $(CFLAGS) -c -I. _test_chm3.c
	./$(PROG) -h wyhash -o _test_chm_wy.c $(WORDS)
	$(CC) $(CFLAGS) -c -I. _test_chm_wy.c
	./$(PROG) -h wyhash -a bdz -o _test_bdz_wy.c $(WORDS)
	$(CC) $(CFLAGS) -c -I. _test_bdz_wy.c
	./$(PROG) -h wyhash -a chm3 -o _test_chm3_wy.c $(WORDS)
	$(CC) $(CFLAGS) -c -I. _test_chm3_wy.c
	@echo test building intkeys
	./$(PROG) -I -o _test_int.c $(RANDBIG)
	$(CC) $(CFLAGS) -c -I. _test_int.c
	@echo test all combinations and results with a small set
	./test
clean:
	-rm -f $(PROG) _test_* test_{bdz,chm,chm3}* _words1000* _rand100 _randbig a.out
install: $(PROG)
	sudo cp $(PROG) /usr/local/bin/
	sudo cp $(HEADERS) /usr/local/include/
	sudo cp $(PROG).1 /usr/local/share/man/man1/
