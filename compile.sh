#!/usr/bin/env bash

CXX='clang++'
FILENAME=$(basename "$1")
FILENAME="${FILENAME%.*}"

#$CXX -Ofast -march=native -mtune=native $1 -o $FILENAME
$CXX -DNOASM $1 -o $FILENAME 2>/dev/null
if [ $? -ne 0 ]; then
	$CXX -DUNIX -DNOASM -Wno-parentheses -Wno-c++11-compat-deprecated-writable-strings -Wno-pointer-bool-conversion -Wno-int-to-pointer-cast -Wno-return-type -Wno-logical-op-parentheses -Wno-format -Wno-bitwise-op-parentheses -Wno-shift-op-parentheses $1 -o $FILENAME
	exit 0
fi
