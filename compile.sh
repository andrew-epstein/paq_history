#!/usr/bin/env bash

CXX='clang++'
FILENAME=$(basename "$1")
FILENAME="${FILENAME%.*}"

$CXX -Ofast -march=native -mtune=native $1 -o $FILENAME
