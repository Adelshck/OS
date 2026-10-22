#!/bin/sh
FILE=${1:-text.txt}
PAT=${2:-abababababababababababab}
CORES=$(nproc)
for t in 1 2 3 4 $CORES 16 128 1024; do
    ./lab2 "$FILE" "$PAT" "$t" | grep threads=
done