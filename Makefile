# $NetBSD: Makefile,v 1.1 2009/08/15 16:21:04 joerg Exp $

PROG=	nbperf
SRCS=	nbperf.c
SRCS+=	nbperf-bdz.c nbperf-chm.c nbperf-chm3.c
SRCS+=	graph2.c graph3.c
WORDS = /usr/share/dict/words

$(PROG): $(SRCS)
	cc -O2 -I. -DHAVE_NBTOOL_CONFIG_H $(SRCS) mi_vector_hash.c -o $@

check: $(PROG)
	./$(PROG) <$(WORDS) >_test_chm.c
	cc -c -O2 -I. _test_chm.c
	./$(PROG) -a bdz <$(WORDS) >_test_bdz.c
	cc -c -O2 -I. _test_bdz.c
	./$(PROG) -a chm3 <$(WORDS) >_test_chm3.c
	cc -c -O2 -I. _test_chm3.c
clean:
	rm $(PROG) _test_*.c _test_*.o
install: $(PROG)
	sudo cp $(PROG) /usr/local/bin/
	sudo cp $(PROG).1 /usr/local/share/man/man1/
