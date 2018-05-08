cl /O1 /D"WIN32" /I..\include paq8hp2.cpp PAQ7ASM.OBJ
split paq8hp2.exe p1       0         90236
split paq8hp2.exe p2a      90236     398210
split paq8hp2.exe p3       488446    999999
copy/b p1 + hp2.dic + p3 paq8hp2.exe