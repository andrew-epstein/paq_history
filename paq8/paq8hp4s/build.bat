cl /O2 /D"WIN32" /I..\include paq8hp4.cpp PAQ7ASM.OBJ
split paq8hp4.exe p1       0         131196
split paq8hp4.exe p2a      131196    398210
split paq8hp4.exe p3       529406    999999
copy/b p1 + hp4.dic + p3 paq8hp4.exe