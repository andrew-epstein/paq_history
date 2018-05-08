cl /O2 /D"WIN32" /I..\include paq8hp5.cpp PAQ7ASM.OBJ
split paq8hp5.exe p1       0         135280
split paq8hp5.exe p2a      135280    411681
split paq8hp5.exe p3       546961    999999
copy/b p1 + hp5.dic + p3 paq8hp5.exe