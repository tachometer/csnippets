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

echo "Cleaning up stuff..."
$MAKE clean

run() {
    echo "Running $EXE with $ARGS"
    cd $EXE_DIR
    if [ "$1" = "gdb" ]; then
        $1 --args $EXE $ARGS
    else
        $EXE $ARGS
    fi
}

_make() {
    $MAKE -j$MAKEOPT || exit;
}

_make_verbose() {
    $MAKE -j$MAKEOPT V=1 || exit;
}

case "$1" in
    -g) $MAKE
        run gdb
        ;;
    -a) _make
        run
        ;;
    -gv) _make_verbose
        run gdb
        ;;
    -av) _make_verbose
        run
        ;;
    *) _make
        ;;
esac

# return back to the old directory
cd ..
exit

