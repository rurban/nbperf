CC = cc
CFLAGS = -DDEBUG -g3 -fno-strict-aliasing -Wall -Wextra

MACHINE := $(shell uname -m)
ifeq (x86_64,$(MACHINE))
CFLAGS += -march=native
endif

include common.mk
