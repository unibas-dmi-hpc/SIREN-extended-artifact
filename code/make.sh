#!/bin/bash
set -e
set -x

make clean
make all
chmod a+r siren.so