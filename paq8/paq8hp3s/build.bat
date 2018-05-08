cl /O1 /D"WIN32" /I..\include paq8hp3.cpp PAQ7ASM.OBJ
split paq8hp3.exe p1       0         90236
split paq8hp3.exe p2a      90236     398210
split paq8hp3.exe p3       488446    999999
copy/b p1 + hp3.dic + p3  paq8hp3.exe