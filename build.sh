#!/bin/bash

filename=$(basename $1)
name="${filename%.*}"
./stella2c compile < $1 > $name.c
gcc -g -std=c11 \
    -DSTELLA_DEBUG \
    -DSTELLA_GC_STATS \
    -DSTELLA_RUNTIME_STATS \
    -DSTELLA_GC_STATE_ON_GC_START \
    -DSTELLA_GC_STATE_ON_GC_END \
    -DSTELLA_STATS_ON_OOM \
    $name.c stella/runtime.c stella/gc.c -o $name
echo $name