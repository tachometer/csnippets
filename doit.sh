#!/bin/bash

CORES=`grep processor /proc/cpuinfo | wc -l`
MAKEOPT=$(($CORES + 1))

make clean
make -j${MAKEOPT} 
if [ $? != 0 ]
then
    echo "errors occured!"
    exit
fi

cd bin && ./a && cd ..

