#!/bin/bash

CORES=`grep processor /proc/cpuinfo | wc -l`
MAKEOPT=$(($CORES + 1))

make clean
make V=1 -j${MAKEOPT} || exit;
cd bin && gdb ./a && cd ..

