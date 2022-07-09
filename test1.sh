#!/bin/sh
if [ -z $CFLAGS ]; then
    CFLAGS="-g -march=native -fno-strict-aliasing -Wall -Wextra"
else
    make clean
fi

usage() { echo "Usage: $0 [-bIfpsx] [-a alg] [-h hash]" 1>&2; exit 1; }

alg=chm
IN=_words1000
hashc=mi_vector_hash.c
while getopts "a:h:bIfps" o; do
    case "$o" in
        a) alg=$OPTARG
           if [ $alg != chm -a $alg != chm3 -a $alg != bdz ]; then
               echo Invalid -a $alg
               usage
           fi
           ;;
        h) hash=$OPTARG
           opts="$opts -h $hash"
           if [ x$hash != xmi_vector_hash ]; then unset hashc; fi
           ;;
        b) big=1 ;;
        I) intkeys=1
           unset hashc
           opts="$opts -I"
           copts="-D_INTKEYS"
           ;;
        f) opts="$opts -f" ;;
        s) opts="$opts -s" ;;
        x) intkeys=1
           unset hashc
           opts="$opts -I"
           copts="-D_INTKEYS"
           hex=1
           ;;
        *) usage ;;
    esac
done
shift $((OPTIND-1))
out=$alg

if [ -n "$intkeys" ]; then
    [ -z $hash ] || usage
    if [ -n "$hex" ]; then
        IN=_randhex
        out=inthex${alg}
    elif [ -n "$big" ]; then
        IN=_randbig
        out=intbig${alg}
    else
        IN=_rand100
        out=int${alg}
    fi
else
    if [ -n "$big" ]; then
        IN=_words
        out=big${alg}
    fi
fi
if [ x$alg = xbdz ]; then
    opts="$opts -m $IN.map"
fi
if [ -n "$hash" ]; then
    out="${out}_${hash}"
fi

echo make CFLAGS="\"${CFLAGS}\"" nbperf ${IN}
make CFLAGS="${CFLAGS}" nbperf ${IN}
test -e ${IN} || exit 1

echo ./nbperf -p $opts -a $alg -o _test_${out}.c $IN
./nbperf -p $opts -a $alg -o _test_${out}.c $IN
echo cc $CFLAGS -I. -D$alg $copts _test_${out}.c test_main.c $hashc -o _test_${out}
cc $CFLAGS -I. -D$alg $copts _test_${out}.c test_main.c $hashc -o _test_${out}
echo ./_test_${out} $IN
./_test_${out} $IN

