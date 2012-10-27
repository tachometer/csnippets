#!/bin/bash

CORES=`grep processor /proc/cpuinfo | wc -l`
MAKEOPT=$(($CORES + 1))

make clean
make -j${MAKEOPT} || echo "errors occured!"
cd bin && ./a && cd ..

