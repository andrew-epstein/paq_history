#!/usr/bin/env bash

N=12
CXX='clang++'
WARN='-Wno-parentheses -Wno-format -Wno-shift-op-parentheses -Wno-unused-variable -Wno-logical-op-parentheses -Wno-bitwise-op-parentheses -Wno-dangling-else -Wno-logical-not-parentheses -Wno-reorder -Wno-pointer-bool-conversion -Wno-array-bounds -Wno-unused-private-field -Wno-uninitialized -Wno-int-to-pointer-cast -Wno-mismatched-new-delete -Wno-unknown-pragmas -Wno-switch -Wno-sometimes-uninitialized -Wno-writable-strings -Wno-c++11-narrowing -Wno-tautological-constant-out-of-range-compare -Wno-comment -Wno-unused-value -Wno-unused-macros -Wno-conversion -Wno-missing-variable-declarations -Wno-global-constructors -Wno-sign-conversion -Wno-zero-as-null-pointer-constant -Wno-old-style-cast -Wno-padded -Wno-weak-vtables -Wno-c++98-compat-pedantic -Wno-missing-prototypes -Wno-missing-noreturn -Wno-shadow -Wno-comma -Wno-unused-parameter -Wno-exit-time-destructors -Wno-non-virtual-dtor -Wno-extra-semi -Wno-cast-align -Wno-conditional-uninitialized -Wno-sign-compare -Wno-cast-qual -Wno-switch-enum -Wno-implicit-fallthrough -Wno-shift-sign-overflow -Wno-four-char-constants -Wno-double-promotion -Wno-unreachable-code -Wno-unreachable-code-break -Wno-reserved-id-macro -Wno-unsequenced -Wno-delete-non-virtual-dtor -Wno-newline-eof -Wno-missing-declarations -Wno-overlength-strings -Wno-format-nonliteral -Wno-missing-braces -Wno-tautological-unsigned-zero-compare -Wno-integer-overflow -Wno-multichar -Wno-documentation -Wno-c99-extensions -Wno-unreachable-code-return -Wno-unused-label -Wno-sizeof-pointer-memaccess -Wno-missing-field-initializers -Wno-tautological-pointer-compare -Wno-return-type -Wno-tautological-type-limit-compare -Wno-extra-semi-stmt -Wno-shadow-field'

function task() {
	FILENAME=$(basename "$1")
	echo "$FILENAME"
	FILENAME="${FILENAME%.*}"
	$CXX -fno-stack-protector -Wall -Weverything $WARN -lz -fno-rtti -std=c++14 -O3 -m64 -march=native -funroll-loops -ftree-vectorize -fdeclspec -DUNIX $1 asm/paq7asm.s -o bin/$FILENAME
	#$CXX $WARN -lz -std=c++17 -O0 -fdeclspec -DUNIX "$1" asm/paq7asm.s -o "bin/$FILENAME"
	#clang-tidy -checks=readability-implicit-bool-conversion -fix $1
	if [ $? -ne 0 ]; then
		echo "$1" >> bad.log
	fi
}

mkdir -pv bin
mkdir -pv out

while IFS= read -r -d '' D; do
	((i = i % N))
	((i++ == 0)) && wait
	task "$D" &
done < <(fd -e cpp 'paq|fp8' -0)
