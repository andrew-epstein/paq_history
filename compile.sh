#!/usr/bin/env bash

CXX='clang++'

WARN='-Wno-parentheses -Wno-format -Wno-return-type -Wno-shift-op-parentheses -Wno-unused-variable -Wno-logical-op-parentheses -Wno-bitwise-op-parentheses -Wno-dangling-else -Wno-logical-not-parentheses -Wno-reorder -Wno-pointer-bool-conversion -Wno-array-bounds -Wno-unused-private-field -Wno-uninitialized -Wno-int-to-pointer-cast -Wno-mismatched-new-delete -Wno-unknown-pragmas -Wno-switch -Wno-sometimes-uninitialized -Wno-writable-strings -Wno-c++11-narrowing -Wno-tautological-constant-out-of-range-compare -Wno-comment -Wno-unused-value -Wno-unused-macros -Wno-conversion -Wno-missing-variable-declarations -Wno-global-constructors -Wno-sign-conversion -Wno-zero-as-null-pointer-constant -Wno-old-style-cast -Wno-padded -Wno-weak-vtables -Wno-c++98-compat-pedantic -Wno-missing-prototypes -Wno-missing-noreturn -Wno-shadow -Wno-comma -Wno-unused-parameter -Wno-exit-time-destructors -Wno-non-virtual-dtor -Wno-extra-semi -Wno-cast-align -Wno-conditional-uninitialized -Wno-sign-compare -Wno-cast-qual -Wno-switch-enum -Wno-implicit-fallthrough -Wno-shift-sign-overflow -Wno-four-char-constants -Wno-double-promotion -Wno-unreachable-code -Wno-unreachable-code-break -Wno-reserved-id-macro -Wno-unsequenced -Wno-delete-non-virtual-dtor -Wno-newline-eof -Wno-missing-declarations -Wno-overlength-strings -Wno-format-nonliteral'

while IFS= read -r -d '' D; do
	FILENAME=$(basename "$D")
	echo $FILENAME
	FILENAME="${FILENAME%.*}"
	$CXX -Wall -Weverything $WARN -lz -fno-rtti -std=gnu++1z -O3 -m64 -march=native -funroll-loops -ftree-vectorize -fdeclspec -DUNIX $D asm/paq7asm.s -o bin/$FILENAME
	if [ $? -ne 0 ]; then
		echo $D >> bad.log
	fi
done < <(fd -e cpp paq -0)
