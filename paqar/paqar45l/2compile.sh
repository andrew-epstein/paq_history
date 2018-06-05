#!/bin/sh
# for mingw, use "-f win32" instead of "-f elf"
# for Mac OS-X,  "-f macho" and use NASM 0.98.40 or newer
nasm -O2 -f macho te8e9.s

# for mingw, you may have to add "-DWIN32" if paqar can't find its dictionary
g++ -O2 -opaqar PAQAR45.cpp te8e9.o
g++ -O2 -opaqarCC -DCALGARY_CORPUS_OPTIM PAQAR45.cpp te8e9.o
