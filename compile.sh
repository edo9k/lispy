#!/bin/bash
# using gcc to compile the code 
[ -e build.out ] && rm build.out
cc -std=c99 -Wall $1 mpc.c -ledit -lm -o build.out
