#!/bin/bash

CORES=`grep processor /proc/cpuinfo | wc -l`
MAKEOPT=$(($CORES + 1))

make clean
make -j${MAKEOPT} || exit;
cd bin && ./a && cd ..

