CC := cc
CFLAGS := -DNDEBUG -O2 -g -fno-strict-aliasing -Wall -Wextra

MACHINE:sh =uname -m
.if "x86_64" != ${MACHINE}
.else
CFLAGS += -march=native
.endif

include common.mk
