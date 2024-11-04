#!/bin/bash

name="${1%.*}"
./stella2c compile < $1 > $name.c
gcc -g -std=c11 -DSTELLA_DEBUG -DSTELLA_GC_STATS -DSTELLA_RUNTIME_STATS -DSTELLA_DUMP_GC_STATE_ON_GC $name.c stella/runtime.c stella/gc.c -o $name
echo $name