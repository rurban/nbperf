# $NetBSD: Makefile,v 1.1 2009/08/15 16:21:04 joerg Exp $

PROG=	nbperf
SRCS=	nbperf.c
SRCS+=	nbperf-bdz.c nbperf-chm.c nbperf-chm3.c
SRCS+=	graph2.c graph3.c
WORDS = /usr/share/dict/words
CC = cc
CFLAGS = -O2 -g

MACHINE := $(shell uname -m)
ifeq (x86_64,$(MACHINE))
CFLAGS += -march=native
endif

$(PROG): $(SRCS) mi_vector_hash.c mi_vector_hash.h wyhash.h nbtool_config.h
	$(CC) $(CFLAGS) -I. -DHAVE_NBTOOL_CONFIG_H $(SRCS) mi_vector_hash.c -o $@

check: $(PROG)
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
	./test
clean:
	rm $(PROG) _test_*.c _test_*.o
install: $(PROG)
	sudo cp $(PROG) /usr/local/bin/
	sudo cp $(PROG).1 /usr/local/share/man/man1/
