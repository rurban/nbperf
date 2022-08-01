#!/bin/sh
if [ -z $CFLAGS ]; then
    CFLAGS="-g -march=native -fno-strict-aliasing -Wall -Wextra"
else
    make clean
fi

usage() {
    echo "Usage: $0 [-bdfIMpsx] [-a alg] [-h hash] [-S size] [-m mapfile] [-c num]" 1>&2;
    exit 1;
}

alg=chm
WORDS=/usr/share/dict/words
IN=_words1000
hashc=mi_vector_hash.c

while getopts "a:h:S:c:m:bdIMfpsx" o; do
    case "$o" in
        a) alg=$OPTARG
           if [ $alg != chm -a $alg != chm3 -a $alg != bdz -a $alg != bpz ]; then
               echo Invalid -a $alg
               usage
           fi
           if [ $alg = bpz ]; then
               alg=bdz
           fi
           ;;
        b) big=1 ;;
        f) opts="$opts -f" ;;
        h) hash=$OPTARG
           opts="$opts -h $hash"
           if [ x$hash != xmi_vector_hash ]; then unset hashc; fi
           ;;
        c) opts="$opts -c $OPTARG" ;;
        d) opts="$opts -d"
           copts="-D_DATA $copts"
           ;;
        I) intkeys=1
           unset hashc
           opts="$opts -I"
           copts="-D_INTKEYS $copts"
           ;;
        m) mapfile=$OPTARG ;;
        M) opts="$opts -M" ;;
        p) opts="$opts -p" ;;
        s) opts="$opts -s" ;;
        S) size=$OPTARG ;;
        x) intkeys=1
           unset hashc
           opts="$opts -I"
           copts="-D_INTKEYS $copts"
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
    if [ -n "$size" ]; then
        IN=_rand${size}
        seq $(( $size * 2 )) | random > ${IN}
    fi
else
    if [ -n "$big" ]; then
        IN=_words
        out=big${alg}
    fi
    if [ -n "$size" ]; then
        IN=_words${size}
        head -n ${size} <${WORDS} >$IN
    fi
fi
copts="-D$alg $copts"
if [ -n "$mapfile" ]; then
    opts="$opts -m $mapfile"
elif [ x$alg = xbdz ]; then
    copts="$copts -D_NOMAP"
fi
if [ -n "$hash" ]; then
    out="${out}_${hash}"
fi

echo make CFLAGS="\"${CFLAGS}\"" nbperf ${IN}
make CFLAGS="${CFLAGS}" nbperf ${IN}
test -e ${IN} || exit 1

echo ./nbperf $opts -a $alg -o _test_${out}.c $IN
./nbperf $opts -a $alg -o _test_${out}.c $IN
echo cc $CFLAGS -I. $copts _test_${out}.c test_main.c $hashc -o _test_${out}
cc $CFLAGS -I. $copts _test_${out}.c test_main.c $hashc -o _test_${out}
echo ./_test_${out} $IN
./_test_${out} $IN

