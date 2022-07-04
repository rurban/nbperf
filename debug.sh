#!/bin/sh
make clean
make CFLAGS="-g -fno-strict-aliasing -Wall -Wextra" check
