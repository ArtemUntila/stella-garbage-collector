#!/bin/bash

name="${1%.*}"
./stella2c compile < $1 > $name.c
gcc -std=c11 -DSTELLA_DEBUG -DSTELLA_GC_STATS -DSTELLA_RUNTIME_STATS $name.c stella/runtime.c stella/gc.c -o $name
./$name