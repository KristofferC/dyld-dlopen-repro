#!/bin/sh
# Build N trivial dylibs and time loading them. Usage: ./run.sh [N]
set -e
N=${1:-500}
cd "$(dirname "$0")"
DIR=$PWD/libs
rm -rf "$DIR" && mkdir "$DIR"
echo 'int FN(void) { return 1; }' > "$DIR/lib.c"
echo 'int umbrella(void) { return 0; }' > "$DIR/umbrella.c"
seq 0 $((N - 1)) | xargs -P 8 -I{} cc -O1 -dynamiclib -DFN=f{} \
    -install_name "$DIR/lib{}.dylib" -o "$DIR/lib{}.dylib" "$DIR/lib.c"
cc -dynamiclib -o "$DIR/umbrella.dylib" "$DIR/umbrella.c" "$DIR"/lib[0-9]*.dylib
cc -O1 -o repro repro.c
sw_vers | tr '\n' ' '; echo; uname -m
./repro "$DIR" "$N" > /dev/null   # warm the file cache and code signature checks
./repro "$DIR" "$N"
./repro "$DIR" umbrella
