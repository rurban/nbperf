#!/bin/sh
# requires plotly
basedir="$(dirname $0)"
VERSION="rurban/nbperf `cat VERSION`"
if test -z "$PNG"; then
  PNG="size nbperf run"
fi

# PATH="~/.local/lib/python3.10/site-packages/kaleido/executable:$PATH"

perf_graph()
{
    LOG=$1
    TITLE=$2
    python3 ${basedir}/perf_plot.py "$LOG" "$TITLE"
}

size() {
  perf_graph \
    'size.log' \
    "sizes ($VERSION)"
}

nbperf() {
  perf_graph \
    'nbperf.log' \
    "compile-times ($VERSION)"
}

run() {
  perf_graph \
    'run.log' \
    "run-times ($VERSION)"
}

for png in $PNG; do
    $png
    test -d ../doc && mv $png.png $png.svg ../doc/
done
