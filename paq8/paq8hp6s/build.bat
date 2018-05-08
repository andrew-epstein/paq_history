cl /O2 /D"WIN32" /I..\include paq8hp6.cpp PAQ7ASM.OBJ
split paq8hp6.exe p1       0         131188
split paq8hp6.exe p2a      131188    411681
split paq8hp6.exe p3       542869    999999
copy/b p1 + hp6.dic + p3 paq8hp6.exe