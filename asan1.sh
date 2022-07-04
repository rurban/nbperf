#~/bin/sh
make clean
make CFLAGS="-g -DASAN -fsanitize=address,undefined -fno-omit-frame-pointer -fno-strict-aliasing -Wall -Wextra -pedantic" check
