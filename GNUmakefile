CC = cc
CFLAGS = -O2 -g -fno-strict-aliasing -Wall -Wextra

MACHINE := $(shell uname -m)
ifeq (x86_64,$(MACHINE))
CFLAGS += -march=native
endif

include common.mk
