cl /O2 /D"WIN32" /I..\include paq8hp7.cpp PAQ7ASM.OBJ
split paq8hp7.exe p1       0         131188
split paq8hp7.exe p2a      131188    420139
split paq8hp7.exe p3       551327    999999
copy/b p1 + hp7.dic + p3 paq8hp7.exe