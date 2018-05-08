#!/bin/sh
# for Mac OS X, use "-f macho" and NASM 0.98.40 or newer
nasm -f elf paq7asm.asm
sed -e s/^/\"/ -e s/$/\\\\n\"/ hp5.dic > hp5dict.h
g++ -O2 -opaq8hp5 paq8hp5.cpp paq7asm.o
