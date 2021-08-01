#!/bin/bash

export PYTHONPATH=`pwd`
if [ -z $USE_GDB ]; then
    catchsegv python3 -m zenqt
else
    gdb python3 -ex 'r -m zenqt'
fi
