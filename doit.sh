#!/bin/bash

# A simple bash script to suit my needs for faster testing.
# This script was not made to be "optimized" but just to
# "do the job" as fast as possible.
#
# Feel free to improve it(as in add features/optimize etc.).

# Figure out cores
CORES=`grep processor /proc/cpuinfo | wc -l`
# Process to be passed to make -jN
MAKEOPT=$(($CORES + 1))
# arguments to pass to the executable generated
ARGS="--run-all-tests"
# Executable
EXE="./a"
# Directory where the executable is.
EXE_DIR="bin"
# the "make" command
MAKE="make"
# the debugger
DBG="gdb"

echo "Cleaning up stuff..."
$MAKE clean

run() {
    echo "Running $EXE with $ARGS"
    cd $EXE_DIR
    if [ "$1" = "$DBG" ]; then
        $1 --args $EXE $ARGS
    else
        $EXE $ARGS
    fi
    # return back to the old directory
    cd ..
}

# http://shawnwilsher.com/archives/184
_make() {
    pathpat="(/[^/]*)+:[0-9]+"
    ccred=$(echo -e "\033[0;31m")
    ccyellow=$(echo -e "\033[0;33m")
    ccend=$(echo -e "\033[0m")

    $MAKE $1 $2 -j$MAKEOPT 2>&1 | sed -E -e "/[Ee]rror[: ]/ s%$pathpat%$ccred&$ccend%g" -e "/[Ww]arning[: ]/ s%$pathpat%$ccyellow&$ccend%g"
    $? -ne 0 && exit;
}

case "$1" in
    -g) _make
        run gdb
        ;;
    -a) _make
        run
        ;;
    -gv) _make V=1
        run gdb
        ;;
    -av) _make V=1
        run
        ;;
    -v) _make V=1
        ;;
    -l) _make staticlib
        ;;
    -e) _make staticlib
        _make binary
        ;;
    *) _make
        ;;
esac

