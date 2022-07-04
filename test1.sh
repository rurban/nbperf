#!/bin/sh
# ./test1 chm [big] [int]
if [ -z $CFLAGS ]; then
    CFLAGS="-g -march=native -fno-strict-aliasing -Wall -Wextra"
else
    make clean
fi

if [ -z "$1" ]; then
    alg=chm
    out=$alg
    IN=_words
else
    alg=$1
    out=$alg
    if [ $alg != chm -a $alg != chm3 -a $alg != bdz ]; then
        echo Invalid alg=$alg
        exit 1
    fi
    if [ -z "$2" ]; then
        IN=_words1000
    elif [ x$2 = xbig ]; then
        IN=_words
        out=big${alg}
    elif [ x$2 = xint ]; then
        opts="-I"
        copts="-D_INTKEYS"
        IN=_rand100
        out=int${alg}
    fi
    if [ x$3 = xint ]; then
        opts="-I"
        copts="-D_INTKEYS"
        IN=_randbig
        out=intbig${alg}
    fi
    if [ $alg = bdz ]; then
        opts="$opts -m $IN.map"
    fi
fi

echo make CFLAGS="\"${CFLAGS}\"" nbperf ${IN}
make CFLAGS="${CFLAGS}" nbperf ${IN}
test -e ${IN} || exit 1

echo ./nbperf -p $opts -a $alg -o _test_${out}.c $IN
./nbperf -p $opts -a $alg -o _test_${out}.c $IN
echo cc $CFLAGS -I. -D$alg $copts _test_${out}.c test_main.c mi_vector_hash.c -o _test_${out}
cc $CFLAGS -I. -D$alg $copts _test_${out}.c test_main.c mi_vector_hash.c -o _test_${out}
echo ./_test_${out} $IN
./_test_${out} $IN

