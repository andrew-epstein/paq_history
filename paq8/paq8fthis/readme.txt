paq8f (C) 2005, 2007, Matt Mahoney, Jan Ondrus
This is free software under GPL, http://www.gnu.org/licenses/gpl.txt

paq8fthis is the paq8f archiver modified by Jan Ondrus to improve
compression of JPEG files as described below.
paq8fthis.exe was compiled with MINGW g++ 3.4.5 as follows:

nasm paq7asm.asm -f win32 --prefix _
g++ -Wall paq8fthis.cpp -O2 -Os -march=pentiumpro -fomit-frame-pointer -s -o paq8fthis.exe paq7asm.obj -DWINDOWS


More JPEG compression with PAQ8 by Jan Ondrus:

FOR all DCT blocks:
- actual color is COLOR
- decode two DCT 8x8 blocks (inverse DCT transformation + dequantization)
  - left from actual position (LB) same COLOR
  - top from actual position (TB) same COLOR
- look at 7th and 8th row in TB block to guess first row in decoded actual block (GROW)
- look at 7th and 8th column in LB block to guess first column in decoded actual block
 (GCOL)

FOR all coefficients:
- actual coded coefficient position in DCT block is [U, V]
- zz-position is POS in zig-zag order (0 <= POS <= 63)
- coefficients with zz-position < POS are encoded already
- make DCT block with coefficints
  - zz-position < POS : coefficient from actual block (+ dequantization)
  - zz-position >= POS : 0
- inverse DCT transform on this block (-> IB)
- calculate differencies:
  - subtract first row in IB from GROW (-> DROW)
  - subtract first column in IB from GCOL (-> DCOL)
- DCT transformation and quantization on DROW and DCOL (-> TDROW, TDCOL)
- ADV_PRED1 = TDROW[U]
- ADV_PRED2 = TDCOL[V]

Useful contexts:
 - [COLOR, POS, ADV_PRED1]
 - [COLOR, POS, ADV_PRED2]
 - [COLOR, POS, (ADV_PRED1 + ADV_PRED2 ) / 2]
 - [COLOR, RS1, ADV_PRED1]
 - [COLOR, RS1, ADV_PRED2]
 - [COLOR, RS1, (ADV_PRED1 + ADV_PRED2 ) / 2]
(RS1 is last coded RS-code)

Results:

 filename / uncompressed / PAQ8F result / PAQ8F+this result

 picture.jpg / 562043 / 456202 (-18.8%) / 431891 (-23.2%)
 a10.jpg / 842468 / 698224 (-17.1%) / 667224 (-20.8%)
 foto.jpg / 276820 / 230517 (-16.7%) / 218592 (-21.0%)
 1.jpg+2.jpg+3.jpg+4.jpg / 1660149 / 1345701 (-18.9%) / 1283003 (-22.7%)
