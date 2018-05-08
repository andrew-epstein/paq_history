all:
	nasm -f macho64 --prefix _ paq7asmsse2.asm
	clang++ -Ofast -march=native paq8p.cpp paq7asmsse2.o -o paq8p
