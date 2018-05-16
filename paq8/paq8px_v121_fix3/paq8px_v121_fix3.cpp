/* paq8px file compressor/archiver.  Released on December 09, 2017

    Copyright (C) 2008 Matt Mahoney, Serge Osnach, Alexander Ratushnyak,
    Bill Pettis, Przemyslaw Skibinski, Matthew Fite, wowtiger, Andrew Paterson,
    Jan Ondrus, Andreas Morphis, Pavel L. Holoborodko, Kaido Orav, Simon Berger,
    Neill Corlett, Márcio Pais

    LICENSE

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details at
    Visit <http://www.gnu.org/copyleft/gpl.html>.

To install and use in Windows:

- To install, put paq8px.exe or a shortcut to it on your desktop.
- To compress a file or folder, drop it on the paq8px icon.
- To decompress, drop a .paq8px file on the icon.

A .paq8px extension is added for compression, removed for decompression.
The output will go in the same folder as the input.

While paq8px is working, a command window will appear and report
progress.  When it is done you can close the window by pressing
ENTER or clicking [X].


COMMAND LINE INTERFACE

- To install, put paq8px.exe somewhere in your PATH.
- To compress:      paq8px [-N] file1 [file2...]
- To decompress:    paq8px [-d] file1.paq8px [dir2]
- To view contents: more < file1.paq8px

The compressed output file is named by adding ".paq8px" extension to
the first named file (file1.paq8px).  Each file that exists will be
added to the archive and its name will be stored without a path.
The option -N specifies a compression level ranging from -0
(fastest) to -8 (smallest).  The default is -5.  If there is
no option and only one file, then the program will pause when
finished until you press the ENTER key (to support drag and drop).
If file1.paq8px exists then it is overwritten.

If the first named file ends in ".paq8px" then it is assumed to be
an archive and the files within are extracted to the same directory
as the archive unless a different directory (dir2) is specified.
The -d option forces extraction even if there is not a ".paq8px"
extension.  If any output file already exists, then it is compared
with the archive content and the first byte that differs is reported.
No files are overwritten or deleted.  If there is only one argument
(no -d or dir2) then the program will pause when finished until
you press ENTER.

For compression, if any named file is actually a directory, then all
files and subdirectories are compressed, preserving the directory
structure, except that empty directories are not stored, and file
attributes (timestamps, permissions, etc.) are not preserved.
During extraction, directories are created as needed.  For example:

  paq8px -4 c:\tmp\foo bar

compresses foo and bar (if they exist) to c:\tmp\foo.paq8px at level 4.

  paq8px -d c:\tmp\foo.paq8px .

extracts foo and compares bar in the current directory.  If foo and bar
are directories then their contents are extracted/compared.

There are no commands to update an existing archive or to extract
part of an archive.  Files and archives larger than 2GB are not
supported (but might work on 64-bit machines, not tested).
File names with nonprintable characters are not supported (spaces
are OK).


TO COMPILE

There are 2 files: paq8px.cpp (C++) and paq7asm.asm (NASM/YASM).
paq7asm.asm is the same as in paq7 and paq8x.  paq8px.cpp recognizes the
following compiler options:

  -DWINDOWS           (to compile in Windows)
  -DUNIX              (to compile in Unix, Linux, Solairs, MacOS/Darwin, etc)
  -DNOASM             (to replace paq7asm.asm with equivalent C++)
  -DDEFAULT_OPTION=N  (to change the default compression level from 5 to N).

If you compile without -DWINDOWS or -DUNIX, you can still compress files,
but you cannot compress directories or create them during extraction.
You can extract directories if you manually create the empty directories
first.

Use -DEFAULT_OPTION=N to change the default compression level to support
drag and drop on machines with less than 256 MB of memory.  Use
-DDEFAULT_OPTION=4 for 128 MB, 3 for 64 MB, 2 for 32 MB, etc.

Use -DNOASM for non x86-32 machines, or older than a Pentium-MMX (about
1997), or if you don't have NASM or YASM to assemble paq7asm.asm.  The
program will still work but it will be slower.  For NASM in Windows,
use the options "--prefix _" and either "-f win32" or "-f obj" depending
on your C++ compiler.  In Linux, use "-f elf".

Recommended compiler commands and optimizations:

  MINGW g++:
    g++ paq8px.cpp -DWINDOWS -lz -Wall -Wextra -O3 -static -static-libgcc -opaq8px.exe


ARCHIVE FILE FORMAT

An archive has the following format.  It is intended to be both
human and machine readable.  The header ends with CTRL-Z (Windows EOF)
so that the binary compressed data is not displayed on the screen.

  paq8px -N CR LF
  size TAB filename CR LF
  size TAB filename CR LF
  ...
  CTRL-Z
  compressed binary data

-N is the option (-0 to -9), even if a default was used.
Plain file names are stored without a path.  Files in compressed
directories are stored with path relative to the compressed directory
(using UNIX style forward slashes "/").  For example, given these files:

  123 C:\dir1\file1.txt
  456 C:\dir2\file2.txt

Then

  paq8px archive \dir1\file1.txt \dir2

will create archive.paq8px with the header:

  paq8px -5
  123     file1.txt
  456     dir2/file2.txt

The command:

  paq8px archive.paq8px C:\dir3

will create the files:

  C:\dir3\file1.txt
  C:\dir3\dir2\file2.txt

Decompression will fail if the first 7 bytes are not "paq8px -".  Sizes
are stored as decimal numbers.  CR, LF, TAB, CTRL-Z are ASCII codes
13, 10, 9, 26 respectively.


ARITHMETIC CODING

The binary data is arithmetic coded as the shortest base 256 fixed point
number x = SUM_i x_i 256^-1-i such that p(<y) <= x < p(<=y), where y is the
input string, x_i is the i'th coded byte, p(<y) (and p(<=y)) means the
probability that a string is lexicographcally less than (less than
or equal to) y according to the model, _ denotes subscript, and ^ denotes
exponentiation.

The model p(y) for y is a conditional bit stream,
p(y) = PROD_j p(y_j | y_0..j-1) where y_0..j-1 denotes the first j
bits of y, and y_j is the next bit.  Compression depends almost entirely
on the ability to predict the next bit accurately.


MODEL MIXING

paq8px uses a neural network to combine a large number of models.  The
i'th model independently predicts
p1_i = p(y_j = 1 | y_0..j-1), p0_i = 1 - p1_i.
The network computes the next bit probabilty

  p1 = squash(SUM_i w_i t_i), p0 = 1 - p1                        (1)

where t_i = stretch(p1_i) is the i'th input, p1_i is the prediction of
the i'th model, p1 is the output prediction, stretch(p) = ln(p/(1-p)),
and squash(s) = 1/(1+exp(-s)).  Note that squash() and stretch() are
inverses of each other.

After bit y_j (0 or 1) is received, the network is trained:

  w_i := w_i + eta t_i (y_j - p1)                                (2)

where eta is an ad-hoc learning rate, t_i is the i'th input, (y_j - p1)
is the prediction error for the j'th input but, and w_i is the i'th
weight.  Note that this differs from back propagation:

  w_i := w_i + eta t_i (y_j - p1) p0 p1                          (3)

which is a gradient descent in weight space to minimize root mean square
error.  Rather, the goal in compression is to minimize coding cost,
which is -log(p0) if y = 1 or -log(p1) if y = 0.  Taking
the partial derivative of cost with respect to w_i yields (2).


MODELS

Most models are context models.  A function of the context (last few
bytes) is mapped by a lookup table or hash table to a state which depends
on the bit history (prior sequence of 0 and 1 bits seen in this context).
The bit history is then mapped to p1_i by a fixed or adaptive function.
There are several types of bit history states:

- Run Map. The state is (b,n) where b is the last bit seen (0 or 1) and
  n is the number of consecutive times this value was seen.  The initial
  state is (0,0).  The output is computed directly:

    t_i = (2b - 1)K log(n + 1).

  where K is ad-hoc, around 4 to 10.  When bit y_j is seen, the state
  is updated:

    (b,n) := (b,n+1) if y_j = b, else (y_j,1).

- Stationary Map.  The state is p, initially 1/2.  The output is
  t_i = stretch(p).  The state is updated at ad-hoc rate K (around 0.01):

    p := p + K(y_j - p)

- Nonstationary Map.  This is a compromise between a stationary map, which
  assumes uniform statistics, and a run map, which adapts quickly by
  discarding old statistics.  An 8 bit state represents (n0,n1,h), initially
  (0,0,0) where:

    n0 is the number of 0 bits seen "recently".
    n1 is the number of 1 bits seen "recently".
    n = n0 + n1.
    h is the full bit history for 0 <= n <= 4,
      the last bit seen (0 or 1) if 5 <= n <= 15,
      0 for n >= 16.

  The primaty output is t_i := stretch(sm(n0,n1,h)), where sm(.) is
  a stationary map with K = 1/256, initialized to
  sm(n0,n1,h) = (n1+(1/64))/(n+2/64).  Four additional inputs are also
  be computed to improve compression slightly:

    p1_i = sm(n0,n1,h)
    p0_i = 1 - p1_i
    t_i   := stretch(p_1)
    t_i+1 := K1 (p1_i - p0_i)
    t_i+2 := K2 stretch(p1) if n0 = 0, -K2 stretch(p1) if n1 = 0, else 0
    t_i+3 := K3 (-p0_i if n1 = 0, p1_i if n0 = 0, else 0)
    t_i+4 := K3 (-p0_i if n0 = 0, p1_i if n1 = 0, else 0)

  where K1..K4 are ad-hoc constants.

  h is updated as follows:
    If n < 4, append y_j to h.
    Else if n <= 16, set h := y_j.
    Else h = 0.

  The update rule is biased toward newer data in a way that allows
  n0 or n1, but not both, to grow large by discarding counts of the
  opposite bit.  Large counts are incremented probabilistically.
  Specifically, when y_j = 0 then the update rule is:

    n0 := n0 + 1, n < 29
          n0 + 1 with probability 2^(27-n0)/2 else n0, 29 <= n0 < 41
          n0, n = 41.
    n1 := n1, n1 <= 5
          round(8/3 lg n1), if n1 > 5

  swapping (n0,n1) when y_j = 1.

  Furthermore, to allow an 8 bit representation for (n0,n1,h), states
  exceeding the following values of n0 or n1 are replaced with the
  state with the closest ratio n0:n1 obtained by decrementing the
  smaller count: (41,0,h), (40,1,h), (12,2,h), (5,3,h), (4,4,h),
  (3,5,h), (2,12,h), (1,40,h), (0,41,h).  For example:
  (12,2,1) 0-> (7,1,0) because there is no state (13,2,0).

- Match Model.  The state is (c,b), initially (0,0), where c is 1 if
  the context was previously seen, else 0, and b is the next bit in
  this context.  The prediction is:

    t_i := (2b - 1)Kc log(m + 1)

  where m is the length of the context.  The update rule is c := 1,
  b := y_j.  A match model can be implemented efficiently by storing
  input in a buffer and storing pointers into the buffer into a hash
  table indexed by context.  Then c is indicated by a hash table entry
  and b can be retrieved from the buffer.


CONTEXTS

High compression is achieved by combining a large number of contexts.
Most (not all) contexts start on a byte boundary and end on the bit
immediately preceding the predicted bit.  The contexts below are
modeled with both a run map and a nonstationary map unless indicated.

- Order n.  The last n bytes, up to about 16.  For general purpose data.
  Most of the compression occurs here for orders up to about 6.
  An order 0 context includes only the 0-7 bits of the partially coded
  byte and the number of these bits (255 possible values).

- Sparse.  Usually 1 or 2 of the last 8 bytes preceding the byte containing
  the predicted bit, e.g (2), (3),..., (8), (1,3), (1,4), (1,5), (1,6),
  (2,3), (2,4), (3,6), (4,8).  The ordinary order 1 and 2 context, (1)
  or (1,2) are included above.  Useful for binary data.

- Text.  Contexts consists of whole words (a-z, converted to lower case
  and skipping other values).  Contexts may be sparse, e.g (0,2) meaning
  the current (partially coded) word and the second word preceding the
  current one.  Useful contexts are (0), (0,1), (0,1,2), (0,2), (0,3),
  (0,4).  The preceding byte may or may not be included as context in the
  current word.

- Formatted text.  The column number (determined by the position of
  the last linefeed) is combined with other contexts: the charater to
  the left and the character above it.

- Fixed record length.  The record length is determined by searching for
  byte sequences with a uniform stride length.  Once this is found, then
  the record length is combined with the context of the bytes immediately
  preceding it and the corresponding byte locations in the previous
  one or two records (as with formatted text).

- Context gap.  The distance to the previous occurrence of the order 1
  or order 2 context is combined with other low order (1-2) contexts.

- FAX.  For 2-level bitmapped images.  Contexts are the surrounding
  pixels already seen.  Image width is assumed to be 1728 bits (as
  in calgary/pic).

- Image.  For uncompressed 24-bit color BMP, TIFF and TGA images.  Contexts
  are the high order bits of the surrounding pixels and linear
  combinations of those pixels, including other color planes.  The
  image width is detected from the file header.  When an image is
  detected, other models are turned off to improve speed.

- JPEG.  Files are further compressed by partially uncompressing back
  to the DCT coefficients to provide context for the next Huffman code.
  Only baseline DCT-Huffman coded files are modeled.  (This ia about
  90% of images, the others are usually progresssive coded).  JPEG images
  embedded in other files (quite common) are detected by headers.  The
  baseline JPEG coding process is:
  - Convert to grayscale and 2 chroma colorspace.
  - Sometimes downsample the chroma images 2:1 or 4:1 in X and/or Y.
  - Divide each of the 3 images into 8x8 blocks.
  - Convert using 2-D discrete cosine transform (DCT) to 64 12-bit signed
    coefficients.
  - Quantize the coefficients by integer division (lossy).
  - Split the image into horizontal slices coded independently, separated
    by restart codes.
  - Scan each block starting with the DC (0,0) coefficient in zigzag order
    to the (7,7) coefficient, interleaving the 3 color components in
    order to scan the whole image left to right starting at the top.
  - Subtract the previous DC component from the current in each color.
  - Code the coefficients using RS codes, where R is a run of R zeros (0-15)
    and S indicates 0-11 bits of a signed value to follow.  (There is a
    special RS code (EOB) to indicate the rest of the 64 coefficients are 0).
  - Huffman code the RS symbol, followed by S literal bits.
  The most useful contexts are the current partially coded Huffman code
  (including S following bits) combined with the coefficient position
  (0-63), color (0-2), and last few RS codes.

- Match.  When a context match of 400 bytes or longer is detected,
  the next bit of the match is predicted and other models are turned
  off to improve speed.

- Exe.  When a x86 file (.exe, .obj, .dll) is detected, sparse contexts
  with gaps of 1-12 selecting only the prefix, opcode, and the bits
  of the modR/M byte that are relevant to parsing are selected.
  This model is turned off otherwise.

- Indirect.  The history of the last 1-3 bytes in the context of the
  last 1-2 bytes is combined with this 1-2 byte context.

- DMC. A bitwise n-th order context is built from a state machine using
  DMC, described in http://plg.uwaterloo.ca/~ftp/dmc/dmc.c
  The effect is to extend a single context, one bit at a time and predict
  the next bit based on the history in this context.  The model here differs
  in that two predictors are used.  One is a pair of counts as in the original
  DMC.  The second predictor is a bit history state mapped adaptively to
  a probability as as in a Nonstationary Map.

ARCHITECTURE

The context models are mixed by several of several hundred neural networks
selected by a low-order context.  The outputs of these networks are
combined using a second neural network, then fed through several stages of
adaptive probability maps (APM) before arithmetic coding.

For images, only one neural network is used and its context is fixed.

An APM is a stationary map combining a context and an input probability.
The input probability is stretched and divided into 32 segments to
combine with other contexts.  The output is interpolated between two
adjacent quantized values of stretch(p1).  There are 2 APM stages in series:

  p1 := (p1 + 3 APM(order 0, p1)) / 4.
  p1 := (APM(order 1, p1) + 2 APM(order 2, p1) + APM(order 3, p1)) / 4.

PREPROCESSING

paq8px uses preprocessing transforms on certain data types to improve
compression.  To improve reliability, the decoding transform is
tested during compression to ensure that the input file can be
restored.  If the decoder output is not identical to the input file
due to a bug, then the transform is abandoned and the data is compressed
without a transform so that it will still decompress correctly.

The input is split into blocks with the format <type> <decoded size> <data>
where <type> is 1 byte (0 = no transform), <decoded size> is the size
of the data after decoding, which may be different than the size of <data>.
Blocks do not span file boundaries, and have a maximum size of 4MB to
2GB depending on compression level.  Large files are split into blocks
of this size.  The preprocessor has 3 parts:

- Detector.  Splits the input into smaller blocks depending on data type.

- Coder.  Input is a block to be compressed.  Output is a temporary
  file.  The coder determines whether a transform is to be applied
  based on file type, and if so, which one.  A coder may use lots
  of resources (memory, time) and make multiple passes through the
  input file.  The file type is stored (as one byte) during compression.

- Decoder.  Performs the inverse transform of the coder.  It uses few
  resorces (fast, low memory) and runs in a single pass (stream oriented).
  It takes input either from a file or the arithmetic decoder.  Each call
  to the decoder returns a single decoded byte.

The following transforms are used:

- EXE:  CALL (0xE8) and JMP (0xE9) address operands are converted from
  relative to absolute address.  The transform is to replace the sequence
  E8/E9 xx xx xx 00/FF by adding file offset modulo 2^25 (signed range,
  little-endian format).  Data to transform is identified by trying the
  transform and applying a crude compression test: testing whether the
  byte following the E8/E8 (LSB of the address) occurred more recently
  in the transformed data than the original and within 4KB 4 times in
  a row.  The block ends when this does not happen for 4KB.

- JPEG: detected by SOI and SOF and ending with EOI or any nondecodable
  data.  No transform is applied.  The purpose is to separate images
  embedded in execuables to block the EXE transform, and for a future
  place to insert a transform.


IMPLEMENTATION

Hash tables are designed to minimize cache misses, which consume most
of the CPU time.

Most of the memory is used by the nonstationary context models.
Contexts are represented by 32 bits, possibly a hash.  These are
mapped to a bit history, represented by 1 byte.  The hash table is
organized into 64-byte buckets on cache line boundaries.  Each bucket
contains 7 x 7 bit histories, 7 16-bit checksums, and a 2 element LRU
queue packed into one byte.  Each 7 byte element represents 7 histories
for a context ending on a 3-bit boundary plus 0-2 more bits.  One
element (for bits 0-1, which have 4 unused bytes) also contains a run model
consisting of the last byte seen and a count (as 1 byte each).

Run models use 4 byte hash elements consisting of a 2 byte checksum, a
repeat count (0-255) and the byte value.  The count also serves as
a priority.

Stationary models are most appropriate for small contexts, so the
context is used as a direct table lookup without hashing.

The match model maintains a pointer to the last match until a mismatching
bit is found.  At the start of the next byte, the hash table is referenced
to find another match.  The hash table of pointers is updated after each
whole byte.  There is no checksum.  Collisions are detected by comparing
the current and matched context in a rotating buffer.

The inner loops of the neural network prediction (1) and training (2)
algorithms are implemented in MMX assembler, which computes 4 elements
at a time.  Using assembler is 8 times faster than C++ for this code
and 1/3 faster overall.  (However I found that SSE2 code on an AMD-64,
which computes 8 elements at a time, is not any faster).


DIFFERENCES FROM PAQ7

An .exe model and filter are added.  Context maps are improved using 16-bit
checksums to reduce collisions.  The state table uses probabilistic updates
for large counts, more states that remember the last bit, and decreased
discounting of the opposite count.  It is implemented as a fixed table.
There are also many minor changes.

DIFFERENCES FROM PAQ8A

The user interface supports directory compression and drag and drop.
The preprocessor segments the input into blocks and uses more robust
EXE detection.  An indirect context model was added.  There is no
dictionary preprocesor like PAQ8B/C/D/E.

DIFFERENCES FROM PAQ8F

Different models, usually from paq8hp*. Also changed rate from 8 to 7. A bug
in Array was fixed that caused the program to silently crash upon exit.

DIFFERENCES FROM PAQ8J

1) Slightly improved sparse model.
2) Added new family of sparse contexts. Each byte mapped to 3-bit value, where
different values corresponds to different byte classes. For example, input
byte 0x00 transformed into 0, all bytes that less then 16 -- into 5, all
punctuation marks (ispunct(c)!=0) -- into 2 etc. Then this flags from 11
previous bytes combined into 32-bit pseudo-context.

All this improvements gives only 62 byte on BOOK1, but on binaries archive size
reduced on 1-2%.

DIFFERENCES FROM PAQ8JA

Introduced distance model. Distance model uses distance to last occurence
of some anchor char (0x00, space, newline, 0xff), combined with previous
charactes as context. This slightly improves compression of files with
variable-width record data.

DIFFERENCES FROM PAQ8JB

Restored recordModel(), broken in paq8hp*. Slightly tuned indirectModel().

DIFFERENCES FROM PAQ8JC

Changed the APMs in the Predictor. Up to a 0.2% improvement for some files.

DIFFERENCES FROM PAQ8JD

Added DMCModel.  Removed some redundant models from SparseModel and other
minor tuneups.  Changes introduced in PAQ8K were not carried over.

PAQ8L v.2

Changed Mixer::p() to p() to fix a compiler error in Linux
(patched by Indrek Kruusa, Apr. 15, 2007).

DIFFERENCES FROM PAQ8L, PAQ8M

Modified JPEG model by Jan Ondrus (paq8fthis2).  The new model improves
compression by using decoded pixel values of current and adjacent blocks
as context.  PAQ8M was an earlier version of the new JPEG model
(from paq8fthis).

DIFFERENCES FROM PAQ8N

Improved bmp model. Slightly faster.

DIFFERENCES FROM PAQ8O

Modified JPEG model by Jan Ondrus (paq8fthis4).
Added PGM (grayscale image) model form PAQ8I.
Added grayscale BMP model to PGM model.
Ver. 2 can be compiled using either old or new "for" loop scoping rules.
Added APM and StateMap from LPAQ1
Code optimizations from Enrico Zeidler
Detection of BMP 4,8,24 bit and PGM 8 bit images before compress
On non BMP,PGM,JPEG data mem is lower
Fixed bug in BMP 8-bit detection in other files like .exe
15. oct 2007
Updates JPEG model by Jan Ondrus
PGM detection bug fix
22. oct 2007
improved JPEG model by Jan Ondrus
16. feb 2008
fixed bmp detection bug
added .rgb file support (uncompressed grayscale)

DIFFERENCES FROM PAQ8O9

Added wav Model. Slightly improved bmp model.

DIFFERENCES FROM PAQ8P

Added nestModel from paq8p3
Modified wordModel from paq8p3
Modified .pbm, .pgm, .ppm, .bmp, .rgb detection (from paq8p3)
Modified WAV model (from paq8p_)
Modified JPEG model (from paq8p2)
Added im1bitModel (1-bit) (from paq8p3)
Added compression of PPM, PBM images (from paq8p3)
Removed pic model
Modified EXE transformation (e8/e9)
Changed image and audio data handling (separated in blocks)
Added compression of (8-bit, 24-bit) TGA image data
Improved TIFF image detection
Added zlib stream recompression
Added base64 transform (from paq8pxd)
Modified im8bitModel (8-bit) (from paq8pxd)
Added gif recompression
*/

//////////////////////// Build /////////////////////////////////////////////

//Change the following values on a new build if applicable

#define PROGNAME     "paq8px"  // Change this if you make a branch
#define PROGVERSION  "121_fixed2"
#define PROGYEAR     "2017"

#define DEFAULT_LEVEL 5

#define NDEBUG  // Remove (comment out) this line for debugging (turns on Array bound checks and asserts)
#include <assert.h>

//////////////////////// Target OS /////////////////////////////////////////

#if defined(_WIN32) || defined(_MSC_VER)
#define WINDOWS  //to compile for Windows
#endif

#if defined(unix) || defined(__unix__) || defined(__unix)
#define UNIX //to compile for Unix, Linux, Solairs, MacOS / Darwin, etc)
#endif

#if !defined(WINDOWS ) && !defined(UNIX)
#error Unknown target system, some functions (like creating folders) may not word
#endif

//////////////////////// Includes /////////////////////////////////////////

#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#else
#define NOMINMAX
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

#include <zlib.h>

#if defined(__x86_64) || defined(_M_X64)
#define X64
#else
#undef X64
#endif

#if !defined(__GNUC__)
#if defined( _M_X64 ) || _M_IX86_FP >= 2
#define __SSE2__
#endif
#endif

//////////////////////// Cross platform definitions /////////////////////////////////////////

#ifdef _MSC_VER  
#define fseeko(a,b,c) _fseeki64(a,b,c)
#define ftello(a) _ftelli64(a)
#else
#ifndef UNIX
#ifndef fseeko
#define fseeko(a,b,c) fseeko64(a,b,c)
#endif
#ifndef ftello
#define ftello(a) ftello64(a)
#endif
#endif
#endif

//////////////////////// Type definitions /////////////////////////////////////////

// shortcuts for 8, 16, 32 bit integer types
typedef unsigned char      U8;
typedef unsigned short     U16;
typedef unsigned int       U32;
typedef unsigned long long U64;
typedef signed char        int8_t;

//////////////////////// Helper functions /////////////////////////////////////////

// min, max functions
inline int min(int a, int b) {return a<b?a:b;}
inline int max(int a, int b) {return a<b?b:a;}

// Error handler: print message if any, and exit
void quit(const char* message=0) {
  throw message;
}

// strings are equal ignoring case?
int equals(const char* a, const char* b) {
  assert(a && b);
  while (*a && *b) {
    int c1=*a;
    if (c1>='A'&&c1<='Z') c1+='a'-'A';
    int c2=*b;
    if (c2>='A'&&c2<='Z') c2+='a'-'A';
    if (c1!=c2) return 0;
    ++a;
    ++b;
  }
  return *a==*b;
}

//////////////////////// Program Checker /////////////////////

// Track time and memory used
// Remark: only Array<T> reports its memory usage, we don't know about other types
class ProgramChecker {
private:
  U64 memused;  // Bytes currently in use (all allocated-all freed)
  U64 maxmem;   // Most bytes allocated ever
  clock_t start_time;  // in ticks
public:
  void alloc(U64 n) {
    memused+=n;
    if (memused>maxmem) maxmem=memused;
  }
  void free(U64 n) {
    assert(memused>=n);
    memused-=n;
  }
  ProgramChecker(): memused(0), maxmem(0) {
    start_time=clock();
    assert(sizeof(U8)==1);
    assert(sizeof(U16)==2);
    assert(sizeof(U32)==4);
    assert(sizeof(U64)==8);
    assert(sizeof(short)==2);
    assert(sizeof(int)==4);
  }
  void print() const {  // Print elapsed time and used memory
      printf("Time %1.2f sec, used %I64u MB (%I64u bytes) of memory\n",double(clock()-start_time)/CLOCKS_PER_SEC,maxmem>>20,maxmem);
  }
  ~ProgramChecker()
  {
    assert(memused==0); // We expect that all reserved memory is already properly freed
  }

} programChecker;

//////////////////////////// Array ////////////////////////////

// Array<T,Align> a(n); allocates memory for n elements of T.
// The base address is aligned if the "alignment" parameter is given.
// Constructors for T are not called, the allocated memory is initialized to 0s.
// It's the caller's responsibility to populate the array with elements.
// Parameters are checked and indexing is bounds checked if assertions are on.
// Copy and assignment are not supported.
//
// a.size(): returns the number of T elements currently in the array.
// a.resize(newsize): grows or shrinks the array.
// a.append(x): appends x to the end of the array and reserving space for more elements if needed.
// a.pop_back(): removes the last element by reducing the size by one (but does not free memory).

static void chkindex(U32 index, U32 upper_bound)
{
  if (index>=upper_bound) {
    fprintf(stderr, "%d out of bounds %d\n", index, upper_bound);
    quit();
  }
}

template <class T, const int Align=16> class Array {
private:
  U32 used_size;
  U32 reserved_size;
  char *ptr; // Address of allocated memory (may not be aligned)
  T* data;   // Aligned base address of the elements, (ptr <= T)
  void create(U32 requested_size);
  inline U32 padding(){return (U32)sizeof(T)-1;}
  inline U32 allocated_bytes(){return reserved_size*sizeof(T)+padding();}
public:
  explicit Array(U32 requested_size) {create(requested_size);}
  ~Array();
  T& operator[](U32 i) {
    #ifndef NDEBUG
    chkindex(i,used_size);
    #endif
    return data[i];
  }
  const T& operator[](U32 i) const {
    #ifndef NDEBUG
    chkindex(i,used_size);
    #endif
    return data[i];
  }
  U32 size() const {return used_size;}
  void resize(U32 new_size);
  void pop_back() {assert(used_size>0); --used_size; }  // decrement size
  void push_back(const T& x);  // increment size, append x
private:
  Array(const Array&);  // no copy or assignment
  Array& operator=(const Array&);
};

template<class T, const int Align> void Array<T,Align>::create(U32 reqested_size) {
  assert((Align&(Align-1))==0);
  used_size=reserved_size=reqested_size;
  if (reqested_size==0) {
    data=0;ptr=0;
    return;
  }
  U32 bytes_to_allocate=allocated_bytes();
  ptr=(char*)calloc(bytes_to_allocate,1);
  if(!ptr)quit("Out of memory.");
  U32 pad=padding();
  data=(T*)(((uintptr_t)ptr+pad) & ~(uintptr_t)pad);
  assert(ptr<=(char*)data && (char*)data<=ptr+Align);
  assert(((uintptr_t)data & (Align-1))==0); //aligned as expected?
  programChecker.alloc(bytes_to_allocate);
}

template<class T, const int Align> void Array<T,Align>::resize(U32 new_size) {
  if (new_size<=reserved_size) {
    used_size=new_size;
    return;
  }
  char *old_ptr=ptr;
  T *old_data=data;
  U32 old_size=used_size;
  programChecker.free(allocated_bytes());
  create(new_size);
  if(old_size>0) {
    assert(old_ptr && old_data);
    memcpy(data, old_data, sizeof(T)*old_size);
  }
  if(old_ptr)free(old_ptr);
}

template<class T, const int Align> void Array<T,Align>::push_back(const T& x) {
  if(used_size==reserved_size) {
    U32 old_size=used_size;
    resize(max(1,used_size*2));
    used_size=old_size;
  }
  data[used_size++]=x;
}

template<class T, const int Align> Array<T, Align>::~Array() {
  programChecker.free(allocated_bytes());
  free(ptr);
  used_size=reserved_size=0;
  data=0;ptr=0;
}

/////////////////////////// String /////////////////////////////

// A tiny subset of std::string
// size() includes NUL terminator.

class String: public Array<char> {
public:
  const char* c_str() const {return &(*this)[0];}
  void operator=(const char* s) {
    resize(int(strlen(s))+1);
    strcpy(&(*this)[0], s);
  }
  void operator+=(const char* s) {
    assert(s);
    pop_back();
    while (*s) push_back(*s++);
    push_back(0);
  }
  String(const char* s=""): Array<char>(1) {
    (*this)+=s;
  }
};


//////////////////////////// rnd ///////////////////////////////

// 32-bit pseudo random number generator
class Random{
  Array<U32> table;
  int i;
public:
  Random(): table(64) {
    table[0]=123456789;
    table[1]=987654321;
    for (int j=0; j<62; j++) table[j+2]=table[j+1]*11+table[j]*23/16;
    i=0;
  }
  U32 operator()() {
    return ++i, table[i&63]=table[(i-24)&63]^table[(i-55)&63];
  }
} rnd;

////////////////////////////// Buf /////////////////////////////

// Buf(n) buf; creates an array of n bytes (must be a power of 2).
// buf[i] returns a reference to the i'th byte with wrap (no out of bounds).
// buf(i) returns i'th byte back from pos (i > 0)
// buf.size() returns n.

int pos;  // Number of input bytes in buf (not wrapped)

class Buf {
  Array<U8> b;
public:
  Buf(int i=0): b(i) {}
  void setsize(int i) {
    if (!i) return;
    assert(i>0 && (i&(i-1))==0);
    b.resize(i);
  }
  U8& operator[](int i) {
    return b[i&(b.size()-1)];
  }
  U8 operator()(int i) const {
    return (i>0)?b[(pos-i)&(b.size()-1)]:0;
  }
  int size() const {
    return b.size();
  }
};

// IntBuf(n) is a buffer of n int (must be a power of 2).
// intBuf[i] returns a reference to i'th element with wrap.

class IntBuf {
  Array<int> b;
public:
  IntBuf(int i=0): b(i) {}
  int& operator[](int i) {
    return b[i&(b.size()-1)];
  }
};

/////////////////////// Global context /////////////////////////

typedef enum {DEFAULT=0, JPEG, HDR, IMAGE1, IMAGE4, IMAGE8, IMAGE8GRAY, IMAGE24, IMAGE32, AUDIO, EXE, CD, ZLIB, BASE64, GIF, PNG8, PNG8GRAY, PNG24, PNG32} Filetype;

inline bool hasRecursion(Filetype ft) { return ft==CD || ft==ZLIB || ft==BASE64 || ft==GIF; }
inline bool hasInfo(Filetype ft) { return ft==IMAGE1 || ft==IMAGE4 || ft==IMAGE8 || ft==IMAGE8GRAY || ft==IMAGE24 || ft==IMAGE32 || ft==AUDIO || ft==PNG8 || ft==PNG8GRAY || ft==PNG24 || ft==PNG32; }
inline bool hasTransform(Filetype ft) { return ft==IMAGE24 || ft==IMAGE32 || ft==EXE || ft==CD || ft==ZLIB || ft==BASE64 || ft==GIF; }

int level=DEFAULT_LEVEL;  // Compression level 0 to 8
#define MEM (U64(65536)<<level)
int y=0;  // Last bit, 0 or 1, set by encoder
bool brute = false, trainEXE = false, trainTXT = false;

struct ModelStats{
  U32 XML, x86_64, Record;
};

// Global context set by Predictor and available to all models.

int c0=1; // Last 0-7 bits of the partial byte with a leading 1 bit (1-255)
U32 c4=0, f4=0, b2=0; // Last 4 whole bytes, packed.  Last byte is bits 0-7.
int bpos=0; // bits in c0 (0 to 7)
Buf buf;  // Rotating input queue set by Predictor
int blpos=0; // Relative position in block

#define CacheSize 32

#if (CacheSize&(CacheSize-1)) || (CacheSize<8)
  #error Cache size must be a power of 2 bigger than 4
#endif


///////////////////////////// ilog //////////////////////////////

// ilog(x) = round(log2(x) * 16), 0 <= x < 64K
class Ilog {
  Array<U8> t;
public:
  int operator()(U16 x) const {return t[x];}
  Ilog();
} ilog;

// Compute lookup table by numerical integration of 1/x
Ilog::Ilog(): t(65536) {
  U32 x=14155776;
  for (int i=2; i<65536; ++i) {
    x+=774541002/(i*2-1);  // numerator is 2^29/ln 2
    t[i]=x>>24;
  }
}

// llog(x) accepts 32 bits
inline int llog(U32 x) {
  if (x>=0x1000000)
    return 256+ilog(x>>16);
  else if (x>=0x10000)
    return 128+ilog(x>>8);
  else
    return ilog(x);
}

///////////////////////// state table ////////////////////////

// State table:
//   nex(state, 0) = next state if bit y is 0, 0 <= state < 256
//   nex(state, 1) = next state if bit y is 1
//   nex(state, 2) = number of zeros in bit history represented by state
//   nex(state, 3) = number of ones represented
//
// States represent a bit history within some context.
// State 0 is the starting state (no bits seen).
// States 1-30 represent all possible sequences of 1-4 bits.
// States 31-252 represent a pair of counts, (n0,n1), the number
//   of 0 and 1 bits respectively.  If n0+n1 < 16 then there are
//   two states for each pair, depending on if a 0 or 1 was the last
//   bit seen.
// If n0 and n1 are too large, then there is no state to represent this
// pair, so another state with about the same ratio of n0/n1 is substituted.
// Also, when a bit is observed and the count of the opposite bit is large,
// then part of this count is discarded to favor newer data over old.

#if 1 // change to #if 0 to generate this table at run time (4% slower)
static const U8 State_table[256][4]={
  {  1,  2, 0, 0},{  3,  5, 1, 0},{  4,  6, 0, 1},{  7, 10, 2, 0}, // 0-3
  {  8, 12, 1, 1},{  9, 13, 1, 1},{ 11, 14, 0, 2},{ 15, 19, 3, 0}, // 4-7
  { 16, 23, 2, 1},{ 17, 24, 2, 1},{ 18, 25, 2, 1},{ 20, 27, 1, 2}, // 8-11
  { 21, 28, 1, 2},{ 22, 29, 1, 2},{ 26, 30, 0, 3},{ 31, 33, 4, 0}, // 12-15
  { 32, 35, 3, 1},{ 32, 35, 3, 1},{ 32, 35, 3, 1},{ 32, 35, 3, 1}, // 16-19
  { 34, 37, 2, 2},{ 34, 37, 2, 2},{ 34, 37, 2, 2},{ 34, 37, 2, 2}, // 20-23
  { 34, 37, 2, 2},{ 34, 37, 2, 2},{ 36, 39, 1, 3},{ 36, 39, 1, 3}, // 24-27
  { 36, 39, 1, 3},{ 36, 39, 1, 3},{ 38, 40, 0, 4},{ 41, 43, 5, 0}, // 28-31
  { 42, 45, 4, 1},{ 42, 45, 4, 1},{ 44, 47, 3, 2},{ 44, 47, 3, 2}, // 32-35
  { 46, 49, 2, 3},{ 46, 49, 2, 3},{ 48, 51, 1, 4},{ 48, 51, 1, 4}, // 36-39
  { 50, 52, 0, 5},{ 53, 43, 6, 0},{ 54, 57, 5, 1},{ 54, 57, 5, 1}, // 40-43
  { 56, 59, 4, 2},{ 56, 59, 4, 2},{ 58, 61, 3, 3},{ 58, 61, 3, 3}, // 44-47
  { 60, 63, 2, 4},{ 60, 63, 2, 4},{ 62, 65, 1, 5},{ 62, 65, 1, 5}, // 48-51
  { 50, 66, 0, 6},{ 67, 55, 7, 0},{ 68, 57, 6, 1},{ 68, 57, 6, 1}, // 52-55
  { 70, 73, 5, 2},{ 70, 73, 5, 2},{ 72, 75, 4, 3},{ 72, 75, 4, 3}, // 56-59
  { 74, 77, 3, 4},{ 74, 77, 3, 4},{ 76, 79, 2, 5},{ 76, 79, 2, 5}, // 60-63
  { 62, 81, 1, 6},{ 62, 81, 1, 6},{ 64, 82, 0, 7},{ 83, 69, 8, 0}, // 64-67
  { 84, 71, 7, 1},{ 84, 71, 7, 1},{ 86, 73, 6, 2},{ 86, 73, 6, 2}, // 68-71
  { 44, 59, 5, 3},{ 44, 59, 5, 3},{ 58, 61, 4, 4},{ 58, 61, 4, 4}, // 72-75
  { 60, 49, 3, 5},{ 60, 49, 3, 5},{ 76, 89, 2, 6},{ 76, 89, 2, 6}, // 76-79
  { 78, 91, 1, 7},{ 78, 91, 1, 7},{ 80, 92, 0, 8},{ 93, 69, 9, 0}, // 80-83
  { 94, 87, 8, 1},{ 94, 87, 8, 1},{ 96, 45, 7, 2},{ 96, 45, 7, 2}, // 84-87
  { 48, 99, 2, 7},{ 48, 99, 2, 7},{ 88,101, 1, 8},{ 88,101, 1, 8}, // 88-91
  { 80,102, 0, 9},{103, 69,10, 0},{104, 87, 9, 1},{104, 87, 9, 1}, // 92-95
  {106, 57, 8, 2},{106, 57, 8, 2},{ 62,109, 2, 8},{ 62,109, 2, 8}, // 96-99
  { 88,111, 1, 9},{ 88,111, 1, 9},{ 80,112, 0,10},{113, 85,11, 0}, // 100-103
  {114, 87,10, 1},{114, 87,10, 1},{116, 57, 9, 2},{116, 57, 9, 2}, // 104-107
  { 62,119, 2, 9},{ 62,119, 2, 9},{ 88,121, 1,10},{ 88,121, 1,10}, // 108-111
  { 90,122, 0,11},{123, 85,12, 0},{124, 97,11, 1},{124, 97,11, 1}, // 112-115
  {126, 57,10, 2},{126, 57,10, 2},{ 62,129, 2,10},{ 62,129, 2,10}, // 116-119
  { 98,131, 1,11},{ 98,131, 1,11},{ 90,132, 0,12},{133, 85,13, 0}, // 120-123
  {134, 97,12, 1},{134, 97,12, 1},{136, 57,11, 2},{136, 57,11, 2}, // 124-127
  { 62,139, 2,11},{ 62,139, 2,11},{ 98,141, 1,12},{ 98,141, 1,12}, // 128-131
  { 90,142, 0,13},{143, 95,14, 0},{144, 97,13, 1},{144, 97,13, 1}, // 132-135
  { 68, 57,12, 2},{ 68, 57,12, 2},{ 62, 81, 2,12},{ 62, 81, 2,12}, // 136-139
  { 98,147, 1,13},{ 98,147, 1,13},{100,148, 0,14},{149, 95,15, 0}, // 140-143
  {150,107,14, 1},{150,107,14, 1},{108,151, 1,14},{108,151, 1,14}, // 144-147
  {100,152, 0,15},{153, 95,16, 0},{154,107,15, 1},{108,155, 1,15}, // 148-151
  {100,156, 0,16},{157, 95,17, 0},{158,107,16, 1},{108,159, 1,16}, // 152-155
  {100,160, 0,17},{161,105,18, 0},{162,107,17, 1},{108,163, 1,17}, // 156-159
  {110,164, 0,18},{165,105,19, 0},{166,117,18, 1},{118,167, 1,18}, // 160-163
  {110,168, 0,19},{169,105,20, 0},{170,117,19, 1},{118,171, 1,19}, // 164-167
  {110,172, 0,20},{173,105,21, 0},{174,117,20, 1},{118,175, 1,20}, // 168-171
  {110,176, 0,21},{177,105,22, 0},{178,117,21, 1},{118,179, 1,21}, // 172-175
  {110,180, 0,22},{181,115,23, 0},{182,117,22, 1},{118,183, 1,22}, // 176-179
  {120,184, 0,23},{185,115,24, 0},{186,127,23, 1},{128,187, 1,23}, // 180-183
  {120,188, 0,24},{189,115,25, 0},{190,127,24, 1},{128,191, 1,24}, // 184-187
  {120,192, 0,25},{193,115,26, 0},{194,127,25, 1},{128,195, 1,25}, // 188-191
  {120,196, 0,26},{197,115,27, 0},{198,127,26, 1},{128,199, 1,26}, // 192-195
  {120,200, 0,27},{201,115,28, 0},{202,127,27, 1},{128,203, 1,27}, // 196-199
  {120,204, 0,28},{205,115,29, 0},{206,127,28, 1},{128,207, 1,28}, // 200-203
  {120,208, 0,29},{209,125,30, 0},{210,127,29, 1},{128,211, 1,29}, // 204-207
  {130,212, 0,30},{213,125,31, 0},{214,137,30, 1},{138,215, 1,30}, // 208-211
  {130,216, 0,31},{217,125,32, 0},{218,137,31, 1},{138,219, 1,31}, // 212-215
  {130,220, 0,32},{221,125,33, 0},{222,137,32, 1},{138,223, 1,32}, // 216-219
  {130,224, 0,33},{225,125,34, 0},{226,137,33, 1},{138,227, 1,33}, // 220-223
  {130,228, 0,34},{229,125,35, 0},{230,137,34, 1},{138,231, 1,34}, // 224-227
  {130,232, 0,35},{233,125,36, 0},{234,137,35, 1},{138,235, 1,35}, // 228-231
  {130,236, 0,36},{237,125,37, 0},{238,137,36, 1},{138,239, 1,36}, // 232-235
  {130,240, 0,37},{241,125,38, 0},{242,137,37, 1},{138,243, 1,37}, // 236-239
  {130,244, 0,38},{245,135,39, 0},{246,137,38, 1},{138,247, 1,38}, // 240-243
  {140,248, 0,39},{249,135,40, 0},{250, 69,39, 1},{ 80,251, 1,39}, // 244-247
  {140,252, 0,40},{249,135,41, 0},{250, 69,40, 1},{ 80,251, 1,40}, // 248-251
  {140,252, 0,41}};  // 252, 253-255 are reserved

#define nex(state,sel) State_table[state][sel]

// The code used to generate the above table at run time (4% slower).
// To print the table, uncomment the 4 lines of print statements below.
// In this code x,y = n0,n1 is the number of 0,1 bits represented by a state.
#else

class StateTable {
  Array<U8> ns;  // state*4 -> next state if 0, if 1, n0, n1
  enum {B=5, N=64}; // sizes of b, t
  static const int b[B];  // x -> max y, y -> max x
  static U8 t[N][N][2];  // x,y -> state number, number of states
  int num_states(int x, int y);  // compute t[x][y][1]
  void discount(int& x);  // set new value of x after 1 or y after 0
  void next_state(int& x, int& y, int b);  // new (x,y) after bit b
public:
  int operator()(int state, int sel) {return ns[state*4+sel];}
  StateTable();
} nex;

const int StateTable::b[B]={42,41,13,6,5};  // x -> max y, y -> max x
U8 StateTable::t[N][N][2];

int StateTable::num_states(int x, int y) {
  if (x<y) return num_states(y, x);
  if (x<0 || y<0 || x>=N || y>=N || y>=B || x>=b[y]) return 0;

  // States 0-30 are a history of the last 0-4 bits
  if (x+y<=4) {  // x+y choose x = (x+y)!/x!y!
    int r=1;
    for (int i=x+1; i<=x+y; ++i) r*=i;
    for (int i=2; i<=y; ++i) r/=i;
    return r;
  }

  // States 31-255 represent a 0,1 count and possibly the last bit
  // if the state is reachable by either a 0 or 1.
  else
    return 1+(y>0 && x+y<16);
}

// New value of count x if the opposite bit is observed
void StateTable::discount(int& x) {
  if (x>2) x=ilog(x)/6-1;
}

// compute next x,y (0 to N) given input b (0 or 1)
void StateTable::next_state(int& x, int& y, int b) {
  if (x<y)
    next_state(y, x, 1-b);
  else {
    if (b) {
      ++y;
      discount(x);
    }
    else {
      ++x;
      discount(y);
    }
    while (!t[x][y][1]) {
      if (y<2) --x;
      else {
        x=(x*(y-1)+(y/2))/y;
        --y;
      }
    }
  }
}

// Initialize next state table ns[state*4] -> next if 0, next if 1, x, y
StateTable::StateTable(): ns(1024) {

  // Assign states
  int state=0;
  for (int i=0; i<256; ++i) {
    for (int y=0; y<=i; ++y) {
      int x=i-y;
      int n=num_states(x, y);
      if (n) {
        t[x][y][0]=state;
        t[x][y][1]=n;
        state+=n;
      }
    }
  }

  // Print/generate next state table
  state=0;
  for (int i=0; i<N; ++i) {
    for (int y=0; y<=i; ++y) {
      int x=i-y;
      for (int k=0; k<t[x][y][1]; ++k) {
        int x0=x, y0=y, x1=x, y1=y;  // next x,y for input 0,1
        int ns0=0, ns1=0;
        if (state<15) {
          ++x0;
          ++y1;
          ns0=t[x0][y0][0]+state-t[x][y][0];
          ns1=t[x1][y1][0]+state-t[x][y][0];
          if (x>0) ns1+=t[x-1][y+1][1];
          ns[state*4]=ns0;
          ns[state*4+1]=ns1;
          ns[state*4+2]=x;
          ns[state*4+3]=y;
        }
        else if (t[x][y][1]) {
          next_state(x0, y0, 0);
          next_state(x1, y1, 1);
          ns[state*4]=ns0=t[x0][y0][0];
          ns[state*4+1]=ns1=t[x1][y1][0]+(t[x1][y1][1]>1);
          ns[state*4+2]=x;
          ns[state*4+3]=y;
        }
          // uncomment to print table above
//        printf("{%3d,%3d,%2d,%2d},", ns[state*4], ns[state*4+1],
//          ns[state*4+2], ns[state*4+3]);
//        if (state%4==3) printf(" // %d-%d\n  ", state-3, state);
        assert(state>=0 && state<256);
        assert(t[x][y][1]>0);
        assert(t[x][y][0]<=state);
        assert(t[x][y][0]+t[x][y][1]>state);
        assert(t[x][y][1]<=6);
        assert(t[x0][y0][1]>0);
        assert(t[x1][y1][1]>0);
        assert(ns0-t[x0][y0][0]<t[x0][y0][1]);
        assert(ns0-t[x0][y0][0]>=0);
        assert(ns1-t[x1][y1][0]<t[x1][y1][1]);
        assert(ns1-t[x1][y1][0]>=0);
        ++state;
      }
    }
  }
//  printf("%d states\n", state); exit(0);  // uncomment to print table above
}

#endif

///////////////////////////// Squash //////////////////////////////

// return p = 1/(1 + exp(-d)), d scaled by 8 bits, p scaled by 12 bits
int squash(int d) {
  static const int t[33]={
    1,2,3,6,10,16,27,45,73,120,194,310,488,747,1101,
    1546,2047,2549,2994,3348,3607,3785,3901,3975,4022,
    4050,4068,4079,4085,4089,4092,4093,4094};
  if (d>2047) return 4095;
  if (d<-2047) return 0;
  int w=d&127;
  d=(d>>7)+16;
  return (t[d]*(128-w)+t[(d+1)]*w+64) >> 7;
}

//////////////////////////// Stretch ///////////////////////////////

// Inverse of squash. d = ln(p/(1-p)), d scaled by 8 bits, p by 12 bits.
// d has range -2047 to 2047 representing -8 to 8.  p has range 0 to 4095.

class Stretch {
  Array<short> t;
public:
  Stretch();
  int operator()(int p) const {
    assert(p>=0 && p<4096);
    return t[p];
  }
} stretch;

Stretch::Stretch(): t(4096) {
  int pi=0;
  for (int x=-2047; x<=2047; ++x) {  // invert squash()
    int i=squash(x);
    for (int j=pi; j<=i; ++j)
      t[j]=x;
    pi=i+1;
  }
  t[4095]=2047;
}

//////////////////////////// Mixer /////////////////////////////

// Mixer m(N, M, S=1, w=0) combines models using M neural networks with
//   N inputs each, of which up to S may be selected.  If S > 1 then
//   the outputs of these neural networks are combined using another
//   neural network (with parameters S, 1, 1).  If S = 1 then the
//   output is direct.  The weights are initially w (+-32K).
//   It is used as follows:
// m.update() trains the network where the expected output is the
//   last bit (in the global variable y).
// m.add(stretch(p)) inputs prediction from one of N models.  The
//   prediction should be positive to predict a 1 bit, negative for 0,
//   nominally +-256 to +-2K.  The maximum allowed value is +-32K but
//   using such large values may cause overflow if N is large.
// m.set(cxt, range) selects cxt as one of 'range' neural networks to
//   use.  0 <= cxt < range.  Should be called up to S times such
//   that the total of the ranges is <= M.
// m.p() returns the output prediction that the next bit is 1 as a
//   12 bit number (0 to 4095).

#if defined(__SSE2__)
#include <emmintrin.h>

static int dot_product (const short* const t, const short* const w, int n) {
  __m128i sum = _mm_setzero_si128 ();
  while ((n -= 8) >= 0) {
    __m128i tmp = _mm_madd_epi16 (*(__m128i *) &t[n], *(__m128i *) &w[n]);
    tmp = _mm_srai_epi32 (tmp, 8);
    sum = _mm_add_epi32 (sum, tmp);
  }
  sum = _mm_add_epi32 (sum, _mm_srli_si128 (sum, 8));
  sum = _mm_add_epi32 (sum, _mm_srli_si128 (sum, 4));
  return _mm_cvtsi128_si32 (sum);
}

static void train (const short* const t, short* const w, int n, const int e) {
  if (e) {
    const __m128i one = _mm_set1_epi16 (1);
    const __m128i err = _mm_set1_epi16 (short(e));
    while ((n -= 8) >= 0) {
      __m128i tmp = _mm_adds_epi16 (*(__m128i *) &t[n], *(__m128i *) &t[n]);
      tmp = _mm_mulhi_epi16 (tmp, err);
      tmp = _mm_adds_epi16 (tmp, one);
      tmp = _mm_srai_epi16 (tmp, 1);
      tmp = _mm_adds_epi16 (tmp, *(__m128i *) &w[n]);
      *(__m128i *) &w[n] = tmp;
    }
  }
}
#else

static int dot_product (const short* const t, const short* const w, int n) {
  int sum = 0;
  while ((n -= 2) >= 0) {
    sum += (t[n] * w[n] + t[n + 1] * w[n + 1]) >> 8;
  }
  return sum;
}

static void train (const short* const t, short* const w, int n, const int err) {
  if (err) {
    while ((n -= 1) >= 0) {
      int wt = w[n] + ((((t[n] * err * 2) >> 16) + 1) >> 1);
      if (wt < -32768) {
        w[n] = -32768;
      } else if (wt > 32767) {
        w[n] = 32767;
      } else {
        w[n] = wt;
      }
    }
  }
}
#endif

class Mixer {
  const int N, M, S;   // max inputs, max contexts, max context sets
  Array<short> tx; // N inputs from add()
  Array<short> wx; // N*M weights
  Array<int> cxt;  // S contexts
  int ncxt;        // number of contexts (0 to S)
  int base;        // offset of next context
  int nx;          // Number of inputs in tx, 0 to N
  Array<int> pr;   // last result (scaled 12 bits)
  Mixer* mp;       // points to a Mixer to combine results
public:
  Mixer(int n, int m, int s=1, int w=0);

  // Adjust weights to minimize coding cost of last prediction
  void update() {
    for (int i=0; i<ncxt; ++i) {
      int err=((y<<12)-pr[i])*7;
      assert(err>=-32768 && err<32768);
      train(&tx[0], &wx[cxt[i]*N], nx, err);
    }
    nx=base=ncxt=0;
  }

  // Input x (call up to N times)
  void add(int x) {
    assert(nx<N);
    tx[nx++]=x;
  }

  // Set a context (call S times, sum of ranges <= M)
  void set(int cx, int range) {
    assert(range>=0);
    assert(ncxt<S);
    assert(0<=cx && cx<range);
    assert(base+range<=M);
    cxt[ncxt++]=base+cx;
    base+=range;
  }

  // predict next bit
  int p() {
    while (nx&7) tx[nx++]=0;  // pad
    if (mp) {  // combine outputs
      mp->update();
      for (int i=0; i<ncxt; ++i) {
        pr[i]=squash(dot_product(&tx[0], &wx[cxt[i]*N], nx)>>5);
        mp->add(stretch(pr[i]));
      }
      mp->set(0, 1);
      return mp->p();
    }
    else {  // S=1 context
      return pr[0]=squash(dot_product(&tx[0], &wx[0], nx)>>8);
    }
  }
  ~Mixer();
};

Mixer::~Mixer() {
  delete mp;
}


Mixer::Mixer(int n, int m, int s, int w):
    N((n+7)&-8), M(m), S(s), tx(N), wx(N*M),
    cxt(S), ncxt(0), base(0), nx(0), pr(S), mp(0) {
  assert(n>0 && N>0 && (N&7)==0 && M>0);
  int i;
  for (i=0; i<S; ++i)
    pr[i]=2048;
  for (i=0; i<N*M; ++i)
    wx[i]=w;
  if (S>1) mp=new Mixer(S, 1, 1);
}




//////////////////////////// APM1 //////////////////////////////

// APM1 maps a probability and a context into a new probability
// that bit y will next be 1.  After each guess it updates
// its state to improve future guesses.  Methods:
//
// APM1 a(N) creates with N contexts, uses 66*N bytes memory.
// a.p(pr, cx, rate=7) returned adjusted probability in context cx (0 to
//   N-1).  rate determines the learning rate (smaller = faster, default 7).
//   Probabilities are scaled 12 bits (0-4095).

class APM1 {
  int index;     // last p, context
  const int N;   // number of contexts
  Array<U16> t;  // [N][33]:  p, context -> p
public:
  APM1(int n);
  int p(int pr=2048, int cxt=0, int rate=7) {
    assert(pr>=0 && pr<4096 && cxt>=0 && cxt<N && rate>0 && rate<32);
    pr=stretch(pr);
    int g=(y<<16)+(y<<rate)-y-y;
    t[index] += (g-t[index]) >> rate;
    t[index+1] += (g-t[index+1]) >> rate;
    const int w=pr&127;  // interpolation weight (33 points)
    index=((pr+2048)>>7)+cxt*33;
    return (t[index]*(128-w)+t[index+1]*w) >> 11;
  }
};

// maps p, cxt -> p initially
APM1::APM1(int n): index(0), N(n), t(n*33) {
  for (int i=0; i<N; ++i)
    for (int j=0; j<33; ++j)
      t[i*33+j] = i==0 ? squash((j-16)*128)*16 : t[j];
}

//////////////////////////// StateMap, APM //////////////////////////

// A StateMap maps a context to a probability.  Methods:
//
// Statemap sm(n) creates a StateMap with n contexts using 4*n bytes memory.
// sm.p(y, cx, limit) converts state cx (0..n-1) to a probability (0..4095).
//     that the next y=1, updating the previous prediction with y (0..1).
//     limit (1..1023, default 1023) is the maximum count for computing a
//     prediction.  Larger values are better for stationary sources.

static int dt[1024];  // i -> 16K/(i+i+3)

class StateMap {
protected:
  const int N;  // Number of contexts
  int cxt;      // Context of last prediction
  Array<U32> t;       // cxt -> prediction in high 22 bits, count in low 10 bits
  inline void update(int limit) {
    assert(cxt>=0 && cxt<N);
    U32 *p=&t[cxt], p0=p[0];
    int n=p0&1023, pr=p0>>10;  // count, prediction
    if (n<limit) ++p0;
    else p0=(p0&0xfffffc00)|limit;
    p0+=(((y<<22)-pr)>>3)*dt[n]&0xfffffc00;
    p[0]=p0;
  }

public:
  StateMap(int n=256);
  void Reset(int Rate=0){
    for (int i=0; i<N; ++i)
      t[i]=(t[i]&0xfffffc00)|min(Rate, t[i]&0x3FF);
  }
  // update bit y (0..1), predict next bit in context cx
  int p(int cx, int limit=1023) {
    assert(cx>=0 && cx<N);
    assert(limit>0 && limit<1024);
    update(limit);
    return t[cxt=cx]>>20;
  }
};

StateMap::StateMap(int n): N(n), cxt(0), t(n) {
  for (int i=0; i<N; ++i)
    t[i]=1u<<31;
}

// An APM maps a probability and a context to a new probability.  Methods:
//
// APM a(n) creates with n contexts using 96*n bytes memory.
// a.pp(y, pr, cx, limit) updates and returns a new probability (0..4095)
//     like with StateMap.  pr (0..4095) is considered part of the context.
//     The output is computed by interpolating pr into 24 ranges nonlinearly
//     with smaller ranges near the ends.  The initial output is pr.
//     y=(0..1) is the last bit.  cx=(0..n-1) is the other context.
//     limit=(0..1023) defaults to 255.

class APM: public StateMap {
public:
  APM(int n);
  int p(int pr, int cx, int limit=255) {
   // assert(y>>1==0);
    assert(pr>=0 && pr<4096);
    assert(cx>=0 && cx<N/24);
    assert(limit>0 && limit<1024);
    update(limit);
    pr=(stretch(pr)+2048)*23;
    int wt=pr&0xfff;  // interpolation weight of next element
    cx=cx*24+(pr>>12);
    assert(cx>=0 && cx<N-1);
    cxt=cx+(wt>>11);
    pr=((t[cx]>>13)*(0x1000-wt)+(t[cx+1]>>13)*wt)>>19;
    return pr;
  }
};

APM::APM(int n): StateMap(n*24) {
  for (int i=0; i<N; ++i) {
    int p=((i%24*2+1)*4096)/48-2048;
    t[i]=(U32(squash(p))<<20)+6;
  }
}


//////////////////////////// hash //////////////////////////////

// Hash 2-5 ints.
inline U32 hash(U32 a, U32 b, U32 c=0xffffffff, U32 d=0xffffffff,
    U32 e=0xffffffff) {
  U32 h=a*200002979u+b*30005491u+c*50004239u+d*70004807u+e*110002499u;
  return h^h>>9^a>>2^b>>3^c>>4^d>>5^e>>6;
}

///////////////////////////// BH ////////////////////////////////

// A BH maps a 32 bit hash to an array of B bytes (checksum and B-2 values)
//
// BH bh(N); creates N element table with B bytes each.
//   N must be a power of 2.  The first byte of each element is
//   reserved for a checksum to detect collisions.  The remaining
//   B-1 bytes are values, prioritized by the first value.  This
//   byte is 0 to mark an unused element.
//
// bh[i] returns a pointer to the i'th element, such that
//   bh[i][0] is a checksum of i, bh[i][1] is the priority, and
//   bh[i][2..B-1] are other values (0-255).
//   The low lg(n) bits as an index into the table.
//   If a collision is detected, up to M nearby locations in the same
//   cache line are tested and the first matching checksum or
//   empty element is returned.
//   If no match or empty element is found, then the lowest priority
//   element is replaced.

// 2 byte checksum with LRU replacement (except last 2 by priority)
template <int B> class BH {
  enum {M=8};  // search limit
  Array<U8> t; // elements
  U32 n; // size-1
public:
  BH(int i): t(i*B), n(i-1) {
    assert(B>=2 && i>0 && (i&(i-1))==0); // size a power of 2?
  }
  U8* operator[](U32 i);
};

template <int B>
inline  U8* BH<B>::operator[](U32 i) {
  U16 chk=(i>>16^i)&0xffff;
  i=i*M&n;
  U8 *p;
  U16 *cp;
  int j;
  for (j=0; j<M; ++j) {
    p=&t[(i+j)*B];
    cp=(U16*)p;
    if (p[2]==0) {*cp=chk;break;}
    if (*cp==chk) break;  // found
  }
  if (j==0) return p+1;  // front
  static U8 tmp[B];  // element to move to front
  if (j==M) {
    --j;
    memset(tmp, 0, B);
    memmove(tmp, &chk, 2);
    if (M>2 && t[(i+j)*B+2]>t[(i+j-1)*B+2]) --j;
  }
  else memcpy(tmp, cp, B);
  memmove(&t[(i+1)*B], &t[i*B], j*B);
  memcpy(&t[i*B], tmp, B);
  return &t[i*B+1];
}

//////////////////////////// HashTable /////////////////////////

// A HashTable is an array of n items representing n contexts.
// Each item is a storage area of B bytes. In every slot the first 
// byte is a checksum using the upper 8 bits of the context 
// selector. The second byte is a priority (0 = empty) for hash
// replacement. Priorities must be set by the caller (0: lowest, 
// 255: highest). Items with lower priorities will be replaced in 
// case of a collision. Only 2 additional items are probed for the 
// given checksum.
// The caller can store any information about the given context
// in bytes [2..B-1].
// Checksums must not be modified by the caller.
// The index need not be a hash.

// HashTable<B> h(n) - creates a hashtable with n slots where
//     n and B must be powers of 2 with n >= B*4, and B >= 2.
// h[i] returns a pointer to the storage area starting from 
//     position 1 (e.g. without the checksum)

template <int B>
class HashTable {
private:
  Array<U8,64> t;  // storage area for n items (1 item = B bytes): 0:checksum 1:priority 2:data 3:data  ... B-1:data
  const int N;     // number of items in table
public:
  HashTable(int n): t(n), N(n) {
    assert(B>=2 && (B&B-1)==0);
    assert(N>=B*4 && (N&N-1)==0);
  }
  U8* operator[](U32 i);
};

template <int B>
inline U8* HashTable<B>::operator[](U32 i) { //i: context selector
  i*=123456791;
  i=i<<16|i>>16;
  i*=234567891;
  int chk=i>>24; //8-bit checksum
  i=(i*B)&(N-B); //force bounds
  //search for the checksum in t
  U8 *p = &t[0];
  if (p[i]==chk) return p+i+1;
  if (p[i^B]==chk) return p+(i^B)+1;
  if (p[i^(B*2)]==chk) return p+(i^(B*2))+1;
  //not found, let's overwrite the lowest priority element
  if (p[i+1]>p[(i+1)^B] || p[i+1]>p[(i+1)^(B*2)]) i^=B;
  if (p[i+1]>p[(i+1)^B^(B*2)]) i^=B^(B*2);
  memset(p+i, 0, B);
  p[i]=chk;
  return p+i+1;
}

/////////////////////////// ContextMap /////////////////////////
//
// A ContextMap maps contexts to a bit histories and makes predictions
// to a Mixer.  Methods common to all classes:
//
// ContextMap cm(M, C); creates using about M bytes of memory (a power
//   of 2) for C contexts.
// cm.set(cx);  sets the next context to cx, called up to C times
//   cx is an arbitrary 32 bit value that identifies the context.
//   It should be called before predicting the first bit of each byte.
// cm.mix(m) updates Mixer m with the next prediction.  Returns 1
//   if context cx is found, else 0.  Then it extends all the contexts with
//   global bit y.  It should be called for every bit:
//
//     if (bpos==0)
//       for (int i=0; i<C; ++i) cm.set(cxt[i]);
//     cm.mix(m);
//
// The different types are as follows:
//
// - RunContextMap.  The bit history is a count of 0-255 consecutive
//     zeros or ones.  Uses 4 bytes per whole byte context.  C=1.
//     The context should be a hash.
// - SmallStationaryContextMap.  0 <= cx*256 < M.
//     The state is a 16-bit probability that is adjusted after each
//     prediction.  C=1.
// - ContextMap.  For large contexts, C >= 1.  Context need not be hashed.

// Predict to mixer m from bit history state s, using sm to map s to
// a probability.
inline void mix2(Mixer& m, int s, StateMap& sm) {
  int p1=sm.p(s);
  int n0=-!nex(s,2);
  int n1=-!nex(s,3);
  int st=stretch(p1)>>2;
  m.add(st);
  p1>>=4;
  int p0=255-p1;
  m.add(p1-p0);
  m.add(st*(n1-n0));
  m.add((p1&n0)-(p0&n1));
  m.add((p1&n1)-(p0&n0));
}

// A RunContextMap maps a context into the next byte and a repeat
// count up to M.  Size should be a power of 2.  Memory usage is 3M/4.
class RunContextMap {
  BH<4> t;
  U8* cp;
public:
  RunContextMap(int m): t(m/4) {cp=t[0]+1;}
  void set(U32 cx) {  // update count
    if (cp[0]==0 || cp[1]!=buf(1)) cp[0]=1, cp[1]=buf(1);
    else if (cp[0]<255) ++cp[0];
    cp=t[cx]+1;
  }
  int p() {  // predict next bit
    if ((cp[1]+256)>>(8-bpos)==c0)
      return ((cp[1]>>(7-bpos)&1)*2-1)*ilog(cp[0]+1)*8;
    else
      return 0;
  }
  int mix(Mixer& m) {  // return run length
    m.add(p());
    return cp[0]!=0;
  }
};

// Context is looked up directly.
// m: Count of (bit) contexts. m must be a power of 2.
// c0 (the partial byte) and some of the lower bits of cx are used for context selection.
// For example with m=65536 (=2^16) c0 (8 bits) and the lowest 8 bits of cx are used thus giving the total of 16 bits.
class SmallStationaryContextMap {
private:
  Array<U16> t; // 0..65535 representing p = 0.0 .. 1.0
  U32 cxt;      // selected bit context
  U16 *cp;      // pointer to last context (to update in next pass)
  const int mask;
public:
  SmallStationaryContextMap(int m): t(m), cxt(0), mask(m-1) {
    assert((m&mask)==0); // power of 2?
    for (U32 i=0; i<t.size(); ++i)
      t[i]=32768; // p=0.5
    cp=&t[0];
  }
  void set(U32 cx) {
    cxt=cx;
  }
  void mix(Mixer& m, int rate=7) {
    //adapt (update prediction from previous pass)
    *cp += ((y<<16)-(*cp)+(1<<(rate-1))) >> rate;
    //predict
    U32 idx=(cxt*256+c0)&mask;  //c0: the partial byte
    assert(idx<t.size());
    cp=&t[idx];
    m.add(stretch((*cp)>>4));
  }
};

/*
  Map for modelling contexts of (nearly-)stationary data.
  The context is looked up directly. For each bit modelled, a 32bit element stores
  a 22 bit prediction and a 10 bit adaptation rate offset.

  - BitsOfContext: How many bits to use for each context. Higher bits are discarded.
  - BitsPerContext: How many bits [1..8] of input are to be modelled for each context.
    New contexts must be set at those intervals.
  - Rate: Initial adaptation rate offset [0..1023]. Lower offsets mean faster adaptation.
    Will be increased on every occurrence until the higher bound is reached.

  Uses 2^(BitsOfContext+BitsPerContext+2) bytes of memory.
*/

class StationaryMap {
  Array<U32> Data;
  int Context, Mask, bCount, B;
  U32 *cp;
public:
  StationaryMap(int BitsOfContext, int BitsPerContext = 8, int Rate = 0): Data(1<<(BitsOfContext+BitsPerContext)), Context(0), Mask(1<<BitsPerContext), bCount(1), B(1) {
    assert(BitsOfContext<16);
    assert(BitsPerContext && BitsPerContext<=8);
    Reset(Rate);
    cp=&Data[0];
  }
  void set(U32 ctx) {
    Context = (ctx*Mask)&(Data.size()-Mask);
  }
  void Reset( int Rate = 0 ){
    for (U32 i=0; i<Data.size(); ++i)
      Data[i]=(0x7FF<<20)|min(0x3FF,Rate);
  }
  void mix(Mixer& m) {
    U32 Count = min(0x3FF, ((*cp)&0x3FF)+1);
    int Prediction = (*cp)>>10, Error = (y<<22)-Prediction;
    Error = ((Error/8)*dt[Count])/1024;
    Prediction = min(0x3FFFFF,max(0,Prediction+Error));
    *cp = (Prediction<<10)|Count;
    B+=(y && B>1);
    cp=&Data[Context+B];
    Prediction = (*cp)>>20;
    m.add(stretch(Prediction)/4);
    m.add((Prediction-2048)/4);
    bCount<<=1; B<<=1;
    if (bCount==Mask){
      bCount=1;
      B=1;
    }
  }
};

// Context map for large contexts.  Most modeling uses this type of context
// map.  It includes a built in RunContextMap to predict the last byte seen
// in the same context, and also bit-level contexts that map to a bit
// history state.
//
// Bit histories are stored in a hash table.  The table is organized into
// 64-byte buckets alinged on cache page boundaries.  Each bucket contains
// a hash chain of 7 elements, plus a 2 element queue (packed into 1 byte)
// of the last 2 elements accessed for LRU replacement.  Each element has
// a 2 byte checksum for detecting collisions, and an array of 7 bit history
// states indexed by the last 0 to 2 bits of context.  The buckets are indexed
// by a context ending after 0, 2, or 5 bits of the current byte.  Thus, each
// byte modeled results in 3 main memory accesses per context, with all other
// accesses to cache.
//
// On bits 0, 2 and 5, the context is updated and a new bucket is selected.
// The most recently accessed element is tried first, by comparing the
// 16 bit checksum, then the 7 elements are searched linearly.  If no match
// is found, then the element with the lowest priority among the 5 elements
// not in the LRU queue is replaced.  After a replacement, the queue is
// emptied (so that consecutive misses favor a LFU replacement policy).
// In all cases, the found/replaced element is put in the front of the queue.
//
// The priority is the state number of the first element (the one with 0
// additional bits of context).  The states are sorted by increasing n0+n1
// (number of bits seen), implementing a LFU replacement policy.
//
// When the context ends on a byte boundary (bit 0), only 3 of the 7 bit
// history states are used.  The remaining 4 bytes implement a run model
// as follows: <count:7,d:1> <b1> <unused> <unused> where <b1> is the last byte
// seen, possibly repeated.  <count:7,d:1> is a 7 bit count and a 1 bit
// flag (represented by count * 2 + d).  If d=0 then <count> = 1..127 is the
// number of repeats of <b1> and no other bytes have been seen.  If d is 1 then
// other byte values have been seen in this context prior to the last <count>
// copies of <b1>.
//
// As an optimization, the last two hash elements of each byte (representing
// contexts with 2-7 bits) are not updated until a context is seen for
// a second time.  This is indicated by <count,d> = <1,0> (2).  After update,
// <count,d> is updated to <2,0> or <1,1> (4 or 3).

class ContextMap {
  const int C;  // max number of contexts
  class E {  // hash element, 64 bytes
    U16 chk[7];  // byte context checksums
    U8 last;     // last 2 accesses (0-6) in low, high nibble
  public:
    U8 bh[7][7]; // byte context, 3-bit context -> bit history state
      // bh[][0] = 1st bit, bh[][1,2] = 2nd bit, bh[][3..6] = 3rd bit
      // bh[][0] is also a replacement priority, 0 = empty
    U8* get(U16 chk);  // Find element (0-6) matching checksum.
      // If not found, insert or replace lowest priority (not last).
  };
  Array<E,64> t;  // bit histories for bits 0-1, 2-4, 5-7
    // For 0-1, also contains a run count in bh[][4] and value in bh[][5]
    // and pending update count in bh[7]
  Array<U8*> cp;   // C pointers to current bit history
  Array<U8*> cp0;  // First element of 7 element array containing cp[i]
  Array<U32> cxt;  // C whole byte contexts (hashes)
  Array<U8*> runp; // C [0..3] = count, value, unused, unused
  StateMap *sm;    // C maps of state -> p
  int cn;          // Next context to set by set()
  U8 bit, lastByte;
  //void update(U32 cx, int c);  // train model that context cx predicts c
  int mix1(Mixer& m, int cc, int bp, int c1, int y1);
    // mix() with global context passed as arguments to improve speed.
public:
  ContextMap(int m, int c=1);  // m = memory in bytes, a power of 2, C = c
  ~ContextMap();
  void Train(U8 B);
  void FinishTraining(int Rate=0){ for (int i=0; i<C; ++i) sm[i].Reset(Rate); }
  void set(U32 cx, int next=-1);   // set next whole byte context to cx
    // if next is 0 then set order does not matter
  int mix(Mixer& m) {return mix1(m, c0, bpos, buf(1), y);}
};

// Find or create hash element matching checksum ch
inline U8* ContextMap::E::get(U16 ch) {
  if (chk[last&15]==ch) return &bh[last&15][0];
  int b=0xffff, bi=0;
  for (int i=0; i<7; ++i) {
    if (chk[i]==ch) return last=last<<4|i, (U8*)&bh[i][0];
    int pri=bh[i][0];
    if (pri<b && (last&15)!=i && last>>4!=i) b=pri, bi=i;
  }
  return last=0xf0|bi, chk[bi]=ch, (U8*)memset(&bh[bi][0], 0, 7);
}

// Construct using m bytes of memory for c contexts
ContextMap::ContextMap(int m, int c): C(c), t(m>>6), cp(c), cp0(c),
    cxt(c), runp(c), cn(0) {
  assert(m>=64 && (m&m-1)==0);  // power of 2?
  assert(sizeof(E)==64);
  sm=new StateMap[C];
  for (int i=0; i<C; ++i) {
    cp0[i]=cp[i]=&t[0].bh[0][0];
    runp[i]=cp[i]+3;
  }
  bit=lastByte=0;
}

ContextMap::~ContextMap() {
  delete[] sm;
}

// Set the i'th context to cx
inline void ContextMap::set(U32 cx, int next) {
  int i=cn++;
  i&=next;
  assert(i>=0 && i<C);
  cx=cx*987654323+i;  // permute (don't hash) cx to spread the distribution
  cx=cx<<16|cx>>16;
  cxt[i]=cx*123456791+i;
}

void ContextMap::Train(U8 B){
  U8 bits = 1;
  for (int k=0;k<8;k++){
    for (int i=0; i<cn; ++i){
      if (cp[i])
        *cp[i]=nex(*cp[i], bit);

      // Update context pointers
      if (k>1 && runp[i][0]==0)
        cp[i]=0;
      else
      {
       U16 chksum=cxt[i]>>16;
       int tmask=t.size()-1;
       switch(k)
       {
        case 1: case 3: case 6: cp[i]=cp0[i]+1+(bits&1); break;
        case 4: case 7: cp[i]=cp0[i]+3+(bits&3); break;
        case 2: case 5: cp0[i]=cp[i]=t[(cxt[i]+bits)&tmask].get(chksum); break;
        default:
        {
         cp0[i]=cp[i]=t[(cxt[i]+bits)&tmask].get(chksum);
         // Update pending bit histories for bits 2-7
         if (cp0[i][3]==2) {
           const int c=cp0[i][4]+256;
           U8 *p=t[(cxt[i]+(c>>6))&tmask].get(chksum);
           p[0]=1+((c>>5)&1);
           p[1+((c>>5)&1)]=1+((c>>4)&1);
           p[3+((c>>4)&3)]=1+((c>>3)&1);
           p=t[(cxt[i]+(c>>3))&tmask].get(chksum);
           p[0]=1+((c>>2)&1);
           p[1+((c>>2)&1)]=1+((c>>1)&1);
           p[3+((c>>1)&3)]=1+(c&1);
           cp0[i][6]=0;
         }
         // Update run count of previous context
         if (runp[i][0]==0)  // new context
           runp[i][0]=2, runp[i][1]=lastByte;
         else if (runp[i][1]!=lastByte)  // different byte in context
           runp[i][0]=1, runp[i][1]=lastByte;
         else if (runp[i][0]<254)  // same byte in context
           runp[i][0]+=2;
         else if (runp[i][0]==255)
           runp[i][0]=128;
         runp[i]=cp0[i]+3;
        } break;
       }
      }
      
      if (cp[i])
        sm[i].p(*cp[i]);
    }
    bit = (B>>(7-k))&1;
    bits+=bits + bit;
  }
  cn=0;
  lastByte=B;
}

// Update the model with bit y1, and predict next bit to mixer m.
// Context: cc=c0, bp=bpos, c1=buf(1), y1=y.
int ContextMap::mix1(Mixer& m, int cc, int bp, int c1, int y1) {

  // Update model with y
  int result=0;
  for (int i=0; i<cn; ++i) {
    if (cp[i]) {
      assert(cp[i]>=&t[0].bh[0][0] && cp[i]<=&t[t.size()-1].bh[6][6]);
      #ifdef X64
      assert(((U64)(cp[i])&63)>=15);
      #else
      assert((long(cp[i])&63)>=15);
      #endif
      int ns=nex(*cp[i], y1);
      if (ns>=204 && rnd() << ((452-ns)>>3)) ns-=4;  // probabilistic increment
      *cp[i]=ns;
    }

    // Update context pointers
    if (bpos>1 && runp[i][0]==0)
      cp[i]=0;
    else
    {
      U16 chksum=cxt[i]>>16;
      int tmask=t.size()-1; 
      switch(bpos)
     {
      case 1: case 3: case 6: cp[i]=cp0[i]+1+(cc&1); break;
      case 4: case 7: cp[i]=cp0[i]+3+(cc&3); break;
      case 2: case 5: cp0[i]=cp[i]=t[(cxt[i]+cc)&tmask].get(chksum); break;
      default:
      {
       cp0[i]=cp[i]=t[(cxt[i]+cc)&tmask].get(chksum);
       // Update pending bit histories for bits 2-7
       if (cp0[i][3]==2) {
         const int c=cp0[i][4]+256;
         U8 *p=t[(cxt[i]+(c>>6))&tmask].get(chksum);
         p[0]=1+((c>>5)&1);
         p[1+((c>>5)&1)]=1+((c>>4)&1);
         p[3+((c>>4)&3)]=1+((c>>3)&1);
         p=t[(cxt[i]+(c>>3))&tmask].get(chksum);
         p[0]=1+((c>>2)&1);
         p[1+((c>>2)&1)]=1+((c>>1)&1);
         p[3+((c>>1)&3)]=1+(c&1);
         cp0[i][6]=0;
       }
       // Update run count of previous context
       if (runp[i][0]==0)  // new context
         runp[i][0]=2, runp[i][1]=c1;
       else if (runp[i][1]!=c1)  // different byte in context
         runp[i][0]=1, runp[i][1]=c1;
       else if (runp[i][0]<254)  // same byte in context
         runp[i][0]+=2;
       else if (runp[i][0]==255)
         runp[i][0]=128;
       runp[i]=cp0[i]+3;
      } break;
     }
    }

    // predict from last byte in context
    if ((runp[i][1]+256)>>(8-bp)==cc) {
      int rc=runp[i][0];  // count*2, +1 if 2 different bytes seen
      int b=(runp[i][1]>>(7-bp)&1)*2-1;  // predicted bit + for 1, - for 0
      int c=ilog(rc+1)<<(2+(~rc&1));
      m.add(b*c);
    }
    else
      m.add(0);

    // predict from bit context
    int s = 0;
    if (cp[i]) s = *cp[i];
    mix2(m,s,sm[i]);
    
    if(s>0)result++;

  }
  if (bp==7) cn=0;
  return result;
}

//////////////////////////// Models //////////////////////////////

// All of the models below take a Mixer as a parameter and write
// predictions to it.

//////////////////////////// matchModel ///////////////////////////

// matchModel() finds the longest matching context and returns its length

int matchModel(Mixer& m) {
  const int MAXLEN=65534;  // longest allowed match + 1
  static Array<int> t(MEM);  // hash table of pointers to contexts
  static int h=0;  // hash of last 7 bytes
  static int ptr=0;  // points to next byte of match if any
  static int len=0;  // length of match, or 0 if no match
  static int result=0;

  static SmallStationaryContextMap scm1(0x10000);

  if (bpos==0) {
    h=(h*997*8+buf(1)+1)&(t.size()-1);  // update context hash
    if (len) ++len, ++ptr;
    else {  // find match
      ptr=t[h];
      if (ptr && pos-ptr<buf.size())
        while (buf(len+1)==buf[ptr-len-1] && len<MAXLEN) ++len;
    }
    t[h]=pos;  // update hash table
    result=len;
//    if (result>0 && !(result&0xfff)) printf("pos=%d len=%d ptr=%d\n", pos, len, ptr);
    scm1.set(pos);
  }

  // predict
  if (len)
  {
   if (buf(1)==buf[ptr-1] && c0==(buf[ptr]+256)>>(8-bpos))
   {
    if (len>MAXLEN) len=MAXLEN;
    if (buf[ptr]>>(7-bpos)&1)
    {
     m.add(ilog(len)<<2);
     m.add(min(len, 32)<<6);
    }
    else
    {
     m.add(-(ilog(len)<<2));
     m.add(-(min(len, 32)<<6));
    }
   }
   else
   {
    len=0;
    m.add(0);
    m.add(0);
   }
  }
  else
  {
   m.add(0);
   m.add(0);
  }

  scm1.mix(m);
  return result;
}

//////////////////////////// wordModel /////////////////////////

// Model English text (words and columns/end of line)
static U32 frstchar=0, spafdo=0, spaces=0, spacecount=0, words=0, wordcount=0,wordlen=0,wordlen1=0;
void wordModel(Mixer& m, Filetype filetype) {
  static U32 word0=0, word1=0, word2=0, word3=0, word4=0, word5=0;  // hashes
  static U32 xword0=0,xword1=0,xword2=0,cword0=0,ccword=0;
  static U32 number0=0, number1=0;  // hashes
  static U32 text0=0;  // hash stream of letters
  static U32 wrdhsh=0, lastLetter=0, firstLetter=0, lastUpper=0, lastDigit=0, wordGap=0;
  static ContextMap cm(MEM*16, 47);
  static int nl1=-3, nl=-2, w=0;  // previous, current newline position
  static U32 mask=0, mask2=0;
  static Array<int> wpos(0x10000);  // last position of word

  // Update word hashes
  if (bpos==0) {
    int c=c4&255,pC=(U8)(c4>>8),f=0;
    if (spaces&0x80000000) --spacecount;
    if (words&0x80000000) --wordcount;
    spaces=spaces*2;
    words=words*2;
    lastUpper=min(lastUpper+1,63);
    lastLetter=min(lastLetter+1,63);
    mask2<<=2;
    if (c>='A' && c<='Z') c+='a'-'A', lastUpper=0;
    if ((c>='a' && c<='z') || c==1 || c==2 ||(c>=128 &&(b2!=3))) {
      if (!wordlen){
        // model syllabification with "+"
        if ((lastLetter==3 && (c4&0xFFFF00)==0x2B0A00) || (lastLetter==4 && (c4&0xFFFFFF00)==0x2B0D0A00)){
          word0 = word1;
          wordlen = wordlen1;
        }
        else{
          wordGap = lastLetter;
          firstLetter = c;
          wrdhsh = 0;
        }
      }
      lastLetter=0;
      ++words, ++wordcount;
      word0^=hash(word0, c,0);
      text0=text0*997*16+c;
      wordlen++;
      wordlen=min(wordlen,45);
      f=0;
      w=word0&(wpos.size()-1);
      if ((c=='a' || c=='e' || c=='i' || c=='o' || c=='u') || (c=='y' && (wordlen>0 && pC!='a' && pC!='e' && pC!='i' && pC!='o' && pC!='u'))){
        mask2++;
        wrdhsh = wrdhsh*997*8+(c/4-22);
      }
      else if (c>='b' && c<='z'){
        mask2+=2;
        wrdhsh = wrdhsh*271*32+(c-97);
      }
      else
        wrdhsh = wrdhsh*11*32+c;
    }
    else {
      if (word0) {
        word5=word4;
        word4=word3;
        word3=word2;
        word2=word1;
        word1=word0;
        wordlen1=wordlen;
        wpos[w]=blpos;
        if (c==':'|| c=='=') cword0=word0;
        if (c==']'&& (frstchar==':' || frstchar=='*')) xword0=word0;
        ccword=0;
        word0=wordlen=0;
        if((c=='.'||c=='!'||c=='?' ||c=='}' ||c==')') && buf(2)!=10 && filetype!=EXE) f=1;
      }
      if ((c4&0xFFFF)==0x3D3D) xword1=word1,xword2=word2; // ==
      if ((c4&0xFFFF)==0x2727) xword1=word1,xword2=word2; // ''
      if (c==32 || c==10) { ++spaces, ++spacecount; if (c==10 ) nl1=nl, nl=pos-1;}
      else if (c=='.' || c=='!' || c=='?' || c==',' || c==';' || c==':') spafdo=0,ccword=c,mask2+=3;
      else { ++spafdo; spafdo=min(63,spafdo); }
    }
    lastDigit=min(0xFF,lastDigit+1);
    if (c>='0' && c<='9') {
        number0^=hash(number0, c,1);
        lastDigit = 0;
    }
    else if (number0) {
      number1=number0;
      number0=0,ccword=0;
    }

    U32 col=min(255, pos-nl);
    int above=buf[nl1+col];
    if (col<=2) frstchar=(col==2?min(c,96):0);
    if (frstchar=='[' && c==32)    {if(buf(3)==']' || buf(4)==']' ) frstchar=96,xword0=0;}
    cm.set(hash(513,spafdo, spaces,ccword));
    cm.set(hash(514,frstchar, c));
    cm.set(hash(515,col, frstchar, (lastUpper<col)*4+(mask2&3)));
    cm.set(hash(516,spaces, (words&255)));
    cm.set(hash(256,number0, word2));
    cm.set(hash(257,number0, word1));
    cm.set(hash(258,number1, c,ccword));
    cm.set(hash(259,number0, number1));
    cm.set(hash(260,word0, number1, lastDigit<wordGap+wordlen));
    cm.set(hash(518,wordlen1,col));
    cm.set(hash(519,c,spacecount/2));
    U32 h=wordcount*64+spacecount;
    cm.set(hash(520,c,h,ccword));
    cm.set(hash(517,frstchar,h));
    cm.set(hash(521,h,spafdo));

    U32 d=c4&0xf0ff;
    cm.set(hash(522,d,frstchar,ccword));
    h=word0*271;
    h=h+buf(1);
    cm.set(hash(262,h, 0));
    cm.set(hash(263,word0, 0));
    cm.set(hash(264,h, word1));
    cm.set(hash(265,word0, word1));
    cm.set(hash(266,h, word1,word2,lastUpper<wordlen));
    cm.set(hash(268,text0&0xfffff, 0));
    cm.set(hash(269,word0, xword0));
    cm.set(hash(270,word0, xword1));
    cm.set(hash(271,word0, xword2));
    cm.set(hash(272,frstchar, xword2));
    cm.set(hash(273,word0, cword0));
    cm.set(hash(274,number0, cword0));
    cm.set(hash(275,h, word2));
    cm.set(hash(276,h, word3));
    cm.set(hash(277,h, word4));
    cm.set(hash(278,h, word5));
    cm.set(hash(279,h, word1,word3));
    cm.set(hash(280,h, word2,word3));
    if (f) {
      word5=word4;
      word4=word3;
      word3=word2;
      word2=word1;
      word1='.';
    }
    cm.set(hash(523,col,buf(1),above));
    cm.set(hash(524,buf(1),above));
    cm.set(hash(525,col,buf(1)));
    cm.set(hash(526,col,c==32));
    cm.set(hash(281, w, llog(blpos-wpos[w])>>4));
    cm.set(hash(282,buf(1),llog(blpos-wpos[w])>>2));

    int fl = 0;
    if ((c4&0xff) != 0) {
      if (isalpha(c4&0xff)) fl = 1;
      else if (ispunct(c4&0xff)) fl = 2;
      else if (isspace(c4&0xff)) fl = 3;
      else if ((c4&0xff) == 0xff) fl = 4;
      else if ((c4&0xff) < 16) fl = 5;
      else if ((c4&0xff) < 64) fl = 6;
      else fl = 7;
    }
    mask = (mask<<3)|fl;
    cm.set(hash(528,mask,0));
    cm.set(hash(529,mask,buf(1)));
    cm.set(hash(530,mask&0xff,col));
    cm.set(hash(531,mask,buf(2),buf(3)));
    cm.set(hash(532,mask&0x1ff,f4&0x00fff0));

    cm.set(hash(h, llog(wordGap), mask&0x1FF,
      ((wordlen1 > 3)<<6)|
      ((wordlen > 0)<<5)|
      ((spafdo == wordlen + 2)<<4)|
      ((spafdo == wordlen + wordlen1 + 3)<<3)|
      ((spafdo >= lastLetter + wordlen1 + wordGap)<<2)|
      ((lastUpper < lastLetter + wordlen1)<<1)|
      (lastUpper < wordlen + wordlen1 + wordGap)
    ));
    cm.set(hash(col,wordlen1,above&0x5F,c4&0x5F));
    cm.set(hash( mask2&0x3F, wrdhsh&0xFFF, (0x100|firstLetter)*(wordlen<6),(wordGap>4)*2+(wordlen1>5)) );
  }
  cm.mix(m);
}

//////////////////////////// recordModel ///////////////////////

// Model 2-D data with fixed record length.  Also order 1-2 models
// that include the distance to the last match.

// ilog2
// returns floor(log2(x)), e.g. 30->4  31->4  32->5,  33->5
#ifdef _MSC_VER
#include <intrin.h>
inline U32 ilog2(U32 x) {
  DWORD tmp=0;
  if(x!=0)_BitScanReverse(&tmp,x);
  return tmp;
}
#elif __GNUC__
inline U32 ilog2(U32 x) {
  if(x!=0)x=31-__builtin_clz(x);
  return x;
}
#else
inline U32 BitCount(U32 v) {
  v -= ((v >> 1) & 0x55555555);
  v = ((v >> 2) & 0x33333333) + (v & 0x33333333);
  v = ((v >> 4) + v) & 0x0f0f0f0f;
  v = ((v >> 8) + v) & 0x00ff00ff;
  v = ((v >> 16) + v) & 0x0000ffff;
  return v;
}

inline U32 ilog2(U32 x) {
  //copy the leading "1" bit to its left (0x03000000 -> 0x03ffffff)
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >>16);
  //how many trailing bits do we have (except the first)? 
  return BitCount(x >> 1);
}
#endif

inline U8 Clip(int const Px){
  if(Px>255)return 255;
  if(Px<0)return 0;
  return Px;
}
inline U8 Clamp4(const int Px, const U8 n1, const U8 n2, const U8 n3, const U8 n4){
  int maximum=n1;if(maximum<n2)maximum=n2;if(maximum<n3)maximum=n3;if(maximum<n4)maximum=n4;
  int minimum=n1;if(minimum>n2)minimum=n2;if(minimum>n3)minimum=n3;if(minimum>n4)minimum=n4;
  if(Px<minimum)return minimum;
  if(Px>maximum)return maximum;
  return Px;
}
inline U8 LogMeanDiffQt(const U8 a, const U8 b){
  if(a==b)return 0;
  U8 sign=a>b?8:0;
  return sign | ilog2((a+b)/max(2,abs(a-b)*2)+1);
}

#define SPACE 0x20

void recordModel(Mixer& m, ModelStats *Stats = NULL) {
  static int cpos1[256] , cpos2[256], cpos3[256], cpos4[256];
  static int wpos1[0x10000]; // buf(1..2) -> last position
  static int rlen[3] = {2,3,4}; // run length and 2 candidates
  static int rcount[2] = {0,0}; // candidate counts
  static U8 padding = 0; // detected padding byte
  static int prevTransition = 0, nTransition = 0, col = 0, mxCtx = 0; // position of the last padding transition
  static ContextMap cm(32768, 3), cn(32768/2, 3), co(32768*2, 3), cp(MEM, 7);
  static StationaryMap Map0(10), Map1(10);
  static bool MayBeImg24b = false;

  // Find record length
  if (bpos==0) {
    int w=c4&0xffff, c=w&255, d=w>>8;
    if (Stats && (*Stats).Record && ((*Stats).Record>>16)>2 &&
          ((*Stats).Record>>16) != (U32)rlen[0]) {
      rlen[0] = (*Stats).Record>>16;
      rcount[0]=rcount[1]=0;
    }
    else{
      int r=pos-cpos1[c];
      if (r>1 && r==cpos1[c]-cpos2[c]
          && r==cpos2[c]-cpos3[c] && (r>32 || r==cpos3[c]-cpos4[c])
          && (r>10 || ((c==buf(r*5+1)) && c==buf(r*6+1)))) {
        if (r==rlen[1]) ++rcount[0];
        else if (r==rlen[2]) ++rcount[1];
        else if (rcount[0]>rcount[1]) rlen[2]=r, rcount[1]=1;
        else rlen[1]=r, rcount[0]=1;
      }

      // check candidate lengths
      for (int i=0; i < 2; i++) {
        if (rcount[i] > max(0,12-(int)ilog2(rlen[i+1]))){
          if (rlen[0] != rlen[i+1]){
            if (MayBeImg24b && rlen[i+1]==3){
              rcount[0]>>=1;
              rcount[1]>>=1;
              continue;
            }
            else if ( (rlen[i+1] > rlen[0]) && (rlen[i+1] % rlen[0] == 0) ){
              // maybe we found a multiple of the real record size..?
              // in that case, it is probably an immediate multiple (2x).
              // that is probably more likely the bigger the length, so
              // check for small lengths too
              if ((rlen[0] > 32) && (rlen[i+1] == rlen[0]*2)){
                rcount[0]>>=1;
                rcount[1]>>=1;
                continue;
              }
            }
            rlen[0] = rlen[i+1];
            rcount[i] = 0;
            MayBeImg24b = (rlen[0]>30 && (rlen[0]%3)==0);
            nTransition = 0;
          }
          else
            // we found the same length again, that's positive reinforcement that
            // this really is the correct record size, so give it a little boost
            rcount[i]>>=2;

          // if the other candidate record length is orders of
          // magnitude larger, it will probably never have enough time
          // to increase its counter before it's reset again. and if
          // this length is not a multiple of the other, than it might
          // really be worthwhile to investigate it, so we won't set its
          // counter to 0
          if (rlen[i+1]<<4 > rlen[1+(i^1)])
            rcount[i^1] = 0;
        }
      }
    }
    // Set 2 dimensional contexts
    assert(rlen[0]>0);
    cm.set(c<<8| (min(255, pos-cpos1[c])/4));
    cm.set(w<<9| llog(pos-wpos1[w])>>2);
    cm.set(rlen[0]|buf(rlen[0])<<10|buf(rlen[0]*2)<<18);

    cn.set(w|rlen[0]<<8);
    cn.set(d|rlen[0]<<16);
    cn.set(c|rlen[0]<<8);

    co.set(buf(1)<<8|min(255, pos-cpos1[buf(1)]));
    co.set(buf(1)<<17|buf(2)<<9|llog(pos-wpos1[w])>>2);
    co.set(buf(1)<<8|buf(rlen[0]));

    col=pos%rlen[0];
    if (!col)
      nTransition = 0;
    cp.set(rlen[0]|buf(rlen[0])<<10|col<<18);
    cp.set(rlen[0]|buf(1)<<10|col<<18);
    cp.set(col|rlen[0]<<12);

    /*
    Consider record structures that include fixed-length strings.
    These usually contain the text followed by either spaces or 0's,
    depending on whether they're to be trimmed or they're null-terminated.
    That means we can guess the length of the string field by looking
    for small repetitions of one of these padding bytes followed by a
    different byte. By storing the last position where this transition
    ocurred, and what was the padding byte, we are able to model these
    runs of padding bytes.
    Special care is taken to skip record structures of less than 9 bytes,
    since those may be little-endian 64 bit integers. If they contain
    relatively low values (<2^40), we may consistently get runs of 3 or
    even more 0's at the end of each record, and so we could assume that
    to be the general case. But with integers, we can't be reasonably sure
    that a number won't have 3 or more 0's just before a final non-zero MSB.
    And with such simple structures, there's probably no need to be fancy
    anyway
    */

    if ((((c4>>8) == SPACE*0x010101) && (c != SPACE)) || (!(c4>>8) && c && ((padding != SPACE) || (pos-prevTransition > rlen[0])))){
      prevTransition = pos;
      nTransition+=(nTransition<31);
      padding = (U8)d;
    }
    if (rlen[0]>8){
      cp.set( hash( min(min(0xFF,rlen[0]),pos-prevTransition), min(0x3FF,col), (w&0xF0F0)|(w==((padding<<8)|padding)), nTransition ) );
      cp.set( hash( w, (buf(rlen[0]+1)==padding && buf(rlen[0])==padding), col/max(1,rlen[0]/32) ) );
    }
    else
      cp.set(0), cp.set(0);

    int last4 = (buf(rlen[0]*4)<<24)|(buf(rlen[0]*3)<<16)|(buf(rlen[0]*2)<<8)|buf(rlen[0]);
    cp.set( (last4&0xFF)|((last4&0xF000)>>4)|((last4&0xE00000)>>9)|((last4&0xE0000000)>>14)|((col/max(1,rlen[0]/16))<<18) );
    cp.set( (last4&0xF8F8)|(col<<16) );

    int i=0x300;
    if (MayBeImg24b)
      i = (col%3)<<8, Map0.set(Clip(((U8)(c4>>16))+c-(c4>>24))|i);
    else
      Map0.set(Clip(c*2-d)|i);
    Map1.set(Clip(c+buf(rlen[0])-buf(rlen[0]+1))|i);
      
    // update last context positions
    cpos4[c]=cpos3[c];
    cpos3[c]=cpos2[c];
    cpos2[c]=cpos1[c];
    cpos1[c]=pos;
    wpos1[w]=pos;

    mxCtx = (rlen[0]>128)?(min(0x7F,col/max(1,rlen[0]/128))):col;
  }
  cm.mix(m);
  cn.mix(m);
  co.mix(m);
  cp.mix(m);
  Map0.mix(m);
  Map1.mix(m);

  m.set( (rlen[0]>2)*( (bpos<<7)|mxCtx ), 1024 );
  m.set( ((buf(rlen[0])^((U8)(c0<<(8-bpos))))>>4)|(min(0x1F,col/max(1,rlen[0]/32))<<4), 512 );
  if (Stats)
    (*Stats).Record = (min(0xFFFF,rlen[0])<<16)|min(0xFFFF,col);
}


//////////////////////////// sparseModel ///////////////////////

// Model order 1-2 contexts with gaps.

void sparseModel(Mixer& m, int seenbefore, int howmany) {
  static ContextMap cm(MEM*2, 42);
  if (bpos==0) {
    cm.set(seenbefore);
    cm.set(howmany);
    cm.set(buf(1)|buf(5)<<8);
    cm.set(buf(1)|buf(6)<<8);
    cm.set(buf(3)|buf(6)<<8);
    cm.set(buf(4)|buf(8)<<8);
    cm.set(buf(1)|buf(3)<<8|buf(5)<<16);
    cm.set(buf(2)|buf(4)<<8|buf(6)<<16);
    cm.set(c4&0x00f0f0ff);
    cm.set(c4&0x00ff00ff);
    cm.set(c4&0xff0000ff);
    cm.set(c4&0x00f8f8f8);
    cm.set(c4&0xf8f8f8f8);
    cm.set(f4&0x00000fff);
    cm.set(f4);
    cm.set(c4&0x00e0e0e0);
    cm.set(c4&0xe0e0e0e0);
    cm.set(c4&0x810000c1);
    cm.set(c4&0xC3CCC38C);
    cm.set(c4&0x0081CC81);
    cm.set(c4&0x00c10081);
    for (int i=1; i<8; ++i) {
      cm.set(seenbefore|buf(i)<<8);
      cm.set((buf(i+2)<<8)|buf(i+1));
      cm.set((buf(i+3)<<8)|buf(i+1));
    }
  }
  cm.mix(m);
}

//////////////////////////// distanceModel ///////////////////////

// Model for modelling distances between symbols

void distanceModel(Mixer& m) {
  static ContextMap cr(MEM, 3);
  if (bpos == 0) {
    static int pos00=0,pos20=0,posnl=0;
    int c=c4&0xff;
    if (c==0x00) pos00=pos;
    if (c==0x20) pos20=pos;
    if (c==0xff||c=='\r'||c=='\n') posnl=pos;
    cr.set(min(pos-pos00,255)|(c<<8));
    cr.set(min(pos-pos20,255)|(c<<8));
    cr.set(min(pos-posnl,255)|((c<<8)+234567));
  }
  cr.mix(m);
}


//////////////////////////// im24bitModel /////////////////////////////////

// Square buf(i)
inline int sqrbuf(int i) {
  assert(i>0);
  return buf(i)*buf(i);
}

class RingBuffer {
  Array<U8> b;
  U32 offset;
public:
  RingBuffer(int i=0): b(i), offset(0) {}
  void Add(U8 B){
    b[offset&(b.size()-1)] = B;
    offset++;
  }
  int operator()(int i) const {
    return b[(offset-i)&(b.size()-1)];
  }
};

inline U8 Paeth(U8 W, U8 N, U8 NW){
  int p = W+N-NW;
  int pW=abs(p-(int)W), pN=abs(p-(int)N), pNW=abs(p-(int)NW);
  if (pW<=pN && pW<=pNW) return W;
  else if (pN<=pNW) return N;
  return NW;
}

// Model for filtered (PNG) or unfiltered 24/32-bit image data

void im24bitModel(Mixer& m, int info, int alpha=0, int isPNG=0) {
  static const int SC=0x10000;
  static const int nMaps = 30;
  static SmallStationaryContextMap scm1(SC), scm2(SC), scm3(SC), scm4(SC), scm5(SC), scm6(SC), scm7(SC), scm8(SC), scm9(SC*2), scm_fixed(256);
  static ContextMap cm(MEM*4, 45);
  static StationaryMap Map[nMaps] = { 12, 12, 12, 12, 10, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0};
  static RingBuffer buffer(0x100000); // internal rotating buffer for PNG unfiltered pixel data
  static U8 WWW, WW, W, NWW, NW, N, NE, NEE, NNWW, NNW, NN, NNE, NNEE, NNN; //pixel neighborhood
  static U8 px = 0; // current PNG filter prediction
  static int color = -1;
  static int stride = 3;
  static int ctx, padding, lastPos=0, lastWasPNG=0, filter=0, x=0, w=0, line=0, R1=0, R2=0;
  static bool filterOn = false;
  static int columns[2] = {1,1}, column[2];

  if (bpos==0) {
    if ((color < 0) || (pos-lastPos != 1)){
      stride = 3+alpha;
      w = info&0xFFFFFF;
      padding = w%stride;
      x = color = line = 0;
      filterOn = false;
      columns[0] = max(1,w/max(1,ilog2(w)*3));
      columns[1] = max(1,columns[0]/max(1,ilog2(columns[0])));
      if (lastPos && lastWasPNG!=isPNG){
        for (int i=0;i<nMaps;i++)
          Map[i].Reset();
      }
      lastWasPNG = isPNG;
    }
    else{
      x++;
      if(x>=w+isPNG){x=0;line++;}
    }
    lastPos = pos;

    if (x==1 && isPNG)
      filter = c4 & 0xFF;
    else{
      if (x+padding<w)
        color*=(++color)<stride;
      else
        color=(padding>0)*(stride+1);

      if (isPNG){
        U8 B = c4 & 0xFF;
        switch (filter){
          case 1: {
            buffer.Add((U8)( B + buffer(stride)*(x>stride+1 || !x) ) );
            filterOn = x>stride;
            px = buffer(stride);
            break;
          }
          case 2: {
            buffer.Add((U8)( B + buffer(w)*(filterOn=(line>0)) ) );
            px = buffer(w);
            break;
          }
          case 3: {
            buffer.Add((U8)( B + (buffer(w)*(line>0) + buffer(stride)*(x>stride+1 || !x))/2 ) );
            filterOn = (x>stride || line>0);
            px = (buffer(stride)*(x>stride)+buffer(w)*(line>0))/2;
            break;
          }
          case 4: {
            buffer.Add((U8)( B + Paeth(buffer(stride)*(x>stride+1 || !x), buffer(w)*(line>0), buffer(w+stride)*(line>0 && (x>stride+1 || !x))) ) );
            filterOn = (x>stride || line>0);
            px = Paeth(buffer(stride)*(x>stride),buffer(w)*(line>0),buffer(w+stride)*(x>stride && line>0));
            break;
          }
          default: buffer.Add(B);
            filterOn = false;
            px = 0;
        }
        if(!filterOn)px=0;
      }
    }

    if (x || !isPNG){
      int i=color<<5;
      column[0]=(x-isPNG)/columns[0];
      column[1]=(x-isPNG)/columns[1];

      if (!isPNG){
        WWW=buf(3*stride), WW=buf(2*stride), W=buf(stride), NW=buf(w+stride), N=buf(w), NE=buf(w-stride), NEE=buf(w-2*stride), NNWW=buf((w+stride)*2), NNW=buf(w*2+stride), NN=buf(w*2), NNE=buf(w*2-stride), NNEE=buf((w-stride)*2), NNN=buf(w*3);
        int buf_stride_plus_1=buf(stride+1); //precalculate value to circumvent a Visual C++ 2015 optimization bug
        int mean=W+NW+N+NE;
        const int var=(W*W+NW*NW+N*N+NE*NE-mean*mean/4)>>2;
        mean>>=2;
        const int logvar=ilog(var);

        cm.set(hash((N+1)>>1, LogMeanDiffQt(N,Clip(NN*2-NNN))));
        cm.set(hash((W+1)>>1, LogMeanDiffQt(W,Clip(WW*2-WWW))));
        cm.set(hash(Clamp4(W+N-NW,W,NW,N,NE), LogMeanDiffQt(Clip(N+NE-NNE), Clip(N+NW-NNW))));
        cm.set(hash((NNN+N+4)/8, Clip(N*3-NN*3+NNN)>>1 ));
        cm.set(hash((WWW+W+4)/8, Clip(W*3-WW*3+WWW)>>1 ));
        cm.set(hash(++i, (W+Clip(NE*3-NNE*3+buf(w*3-stride)))/4, LogMeanDiffQt(N,(NW+NE)/2)));
        cm.set(hash(++i, Clip((-buf(4*stride)+5*WWW-10*WW+10*W+Clamp4(NE*4-NNE*6+buf(w*3-stride)*4-buf(w*4-stride),N,NE,buf(w-2*stride),buf(w-3*stride)))/5)/4));
        cm.set(hash(Clip(NEE+N-NNEE), LogMeanDiffQt(W,Clip(NW+NE-NNE))));
        cm.set(hash(Clip(NN+W-NNW), LogMeanDiffQt(W,Clip(NNW+WW-NNWW))));
        cm.set(hash(++i, buf(1)));
        cm.set(hash(++i, buf(2)));
        cm.set(hash(++i, Clip(W+N-NW)/2, Clip(W+buf(1)-buf_stride_plus_1)/2));
        cm.set(hash(Clip(N*2-NN)/2, LogMeanDiffQt(N,Clip(NN*2-NNN))));
        cm.set(hash(Clip(W*2-WW)/2, LogMeanDiffQt(W,Clip(WW*2-WWW))));
        cm.set(Clamp4(N*3-NN*3+NNN,W,NW,N,NE)/2);
        cm.set(Clamp4(W*3-WW*3+WWW,W,N,NE,NEE)/2);
        cm.set(hash(++i, LogMeanDiffQt(W,buf_stride_plus_1), Clamp4((buf(1)*W)/max(1,buf_stride_plus_1),W,N,NE,NEE)));
        cm.set(hash(++i, Clamp4(N+buf(2)-buf(w+2),W,NW,N,NE)));
        cm.set(hash(++i, Clip(W+N-NW), column[0]));
        cm.set(hash(++i, Clip(N*2-NN), LogMeanDiffQt(W,Clip(NW*2-NNW))));
        cm.set(hash(++i, Clip(W*2-WW), LogMeanDiffQt(N,Clip(NW*2-NWW))));
        cm.set(hash( (W+NEE)/2, LogMeanDiffQt(W,(WW+NE)/2) ));
        cm.set(Clamp4(Clip(W*2-WW) + Clip(N*2-NN) - Clip(NW*2-NNWW), W, NW, N, NE));
        cm.set(hash(++i, W, buf(2) ));
        cm.set(hash( N, NN&0x1F, NNN&0x1F ));
        cm.set(hash( W, WW&0x1F, WWW&0x1F ));
        cm.set(hash(++i, N, column[0] ));
        cm.set(hash(++i, Clip(W+NEE-NE), LogMeanDiffQt(W,Clip(WW+NE-N))));
        cm.set(hash( NN, buf(w*4)&0x1F, buf(w*6)&0x1F, column[1] ));
        cm.set(hash( WW, buf(stride*4)&0x1F, buf(stride*6)&0x1F, column[1] ));
        cm.set(hash( NNN, buf(w*6)&0x1F, buf(w*9)&0x1F, column[1] ));
        cm.set(hash(++i, column[1]));
        
        cm.set(hash(++i, W, LogMeanDiffQt(W,WW)));
        cm.set(hash(++i, W, buf(1)));
        cm.set(hash(++i, W/4, LogMeanDiffQt(W,buf(1)), LogMeanDiffQt(W,buf(2)) ));
        cm.set(hash(++i, N, LogMeanDiffQt(N,NN)));
        cm.set(hash(++i, N, buf(1)));
        cm.set(hash(++i, N/4, LogMeanDiffQt(N,buf(1)), LogMeanDiffQt(N,buf(2)) ));
        cm.set(hash(++i, (W+N)>>3, buf(1)>>4, buf(2)>>4));
        cm.set(hash(++i, buf(1)/2, buf(2)/2));
        cm.set(hash(++i, W, buf(1)-buf_stride_plus_1));
        cm.set(hash(++i, W+buf(1)-buf_stride_plus_1));
        cm.set(hash(++i, N, buf(1)-buf(w+1)));
        cm.set(hash(++i, N+buf(1)-buf(w+1)));
        cm.set(hash(++i, mean, logvar>>4));
        scm1.set(W+N-NW);
        scm2.set(W+NE-N);
        scm3.set(W*2-WW);
        scm4.set(N*2-NN);
        scm5.set(NW*2-NNWW);
        scm6.set(NE*2-NNEE);
        scm7.set(NE+buf(1)-buf(w-stride+1));
        scm8.set(N+NE-NNE);
        scm9.set(mean>>1|(logvar<<1&0x180));
        //scm_fixed.set(0); //unnecessary: context is set to 0 (by default)

        ctx = (min(color,stride-1)<<9)|((abs(W-N)>8)<<8)|((W>N)<<7)|((W>NW)<<6)|((abs(N-NW)>8)<<5)|((N>NW)<<4)|((abs(N-NE)>8)<<3)|((N>NE)<<2)|((W>WW)<<1)|(N>NN);
      }
      else{
        i|=(filterOn)?((0x100|filter)<<8):0;
        WWW=buffer(3*stride), WW=buffer(2*stride), W=buffer(stride), NWW=buffer(w+2*stride), NW=buffer(w+stride), N=buffer(w), NE=buffer(w-stride), NEE=buffer(w-2*stride), NNWW=buffer((w+stride)*2), NNW=buffer(w*2+stride), NN=buffer(w*2), NNE=buffer(w*2-stride), NNEE=buffer((w-stride)*2), NNN=buffer(w*3);
        int residuals[5] = { ((int8_t)buf(stride+(x<=stride)))+128,
                             ((int8_t)buf(1+(x<2)))+128,
                             ((int8_t)buf(stride+1+(x<=stride)))+128,
                             ((int8_t)buf(2+(x<3)))+128,
                             ((int8_t)buf(stride+2+(x<=stride)))+128
                           };
        R1 = (residuals[1]*residuals[0])/max(1,residuals[2]);
        R2 = (residuals[3]*residuals[0])/max(1,residuals[4]);

        cm.set(hash(++i, Clip(W+N-NW)-px, Clip(W+buffer(1)-buffer(stride+1))-px, R1));
        cm.set(hash(++i, Clip(W+N-NW)-px, LogMeanDiffQt(buffer(1),Clip(buffer(stride+1)+buffer(w+1)-buffer(w+stride+1)))));
        cm.set(hash(++i, Clip(W+N-NW)-px, LogMeanDiffQt(buffer(2),Clip(buffer(stride+2)+buffer(w+2)-buffer(w+stride+2))), R2/4));
        cm.set(hash(++i, Clip(W+N-NW)-px, Clip(N+NE-NNE)-Clip(N+NW-NNW)));
        cm.set(hash(++i, Clip(W+N-NW+buffer(1)-(buffer(stride+1)+buffer(w+1)-buffer(w+stride+1))), px, R1 ));
        cm.set(hash(++i, Clamp4(W+N-NW,W,NW,N,NE)-px, column[0]));
        cm.set(hash(i>>8, Clamp4(W+N-NW,W,NW,N,NE)/8, px));
        cm.set(hash(++i, N-px, Clip(N+buffer(1)-buffer(w+1))-px));
        cm.set(hash(++i, Clip(W+buffer(1)-buffer(stride+1))-px, R1));
        cm.set(hash(++i, Clip(N+buffer(1)-buffer(w+1))-px));
        cm.set(hash(++i, Clip(N+buffer(1)-buffer(w+1))-px, Clip(N+buffer(2)-buffer(w+2))-px));
        cm.set(hash(++i, Clip(W+buffer(1)-buffer(stride+1))-px, Clip(W+buffer(2)-buffer(stride+2))-px));
        cm.set(hash(++i, Clip(NW+buffer(1)-buffer(w+stride+1))-px));
        cm.set(hash(++i, Clip(NW+buffer(1)-buffer(w+stride+1))-px, column[0]));
        cm.set(hash(++i, Clip(NE+buffer(1)-buffer(w-stride+1))-px, column[0]));
        cm.set(hash(++i, Clip(NE+N-NNE)-px, Clip(NE+buffer(1)-buffer(w-stride+1))-px));
        cm.set(hash(i>>8, Clip(N+NE-NNE)-px, column[0]));
        cm.set(hash(++i, Clip(NW+N-NNW)-px, Clip(NW+buffer(1)-buffer(w+stride+1))-px));
        cm.set(hash(i>>8, Clip(N+NW-NNW)-px, column[0]));
        cm.set(hash(i>>8, Clip(NN+W-NNW)-px, LogMeanDiffQt(N,Clip(NNN+NW-buffer(w*3+stride)))));
        cm.set(hash(i>>8, Clip(W+NEE-NE)-px, LogMeanDiffQt(W,Clip(WW+NE-N))));
        cm.set(hash(++i, Clip(N+NN-NNN+buffer(1+(!color))-Clip(buffer(w+1+(!color))+buffer(w*2+1+(!color))-buffer(w*3+1+(!color))))-px));
        cm.set(hash(i>>8, Clip(N+NN-NNN)-px, Clip( 5*N-10*NN+10*NNN-5*buffer(w*4)+buffer(w*5) )-px ));
        cm.set(hash(++i, Clip(N*2-NN)-px, LogMeanDiffQt(N,Clip(NN*2-NNN))));
        cm.set(hash(++i, Clip(W*2-WW)-px, LogMeanDiffQt(W,Clip(WW*2-WWW))));
        cm.set(hash(i>>8, Clip(N*3-NN*3+NNN)-px));
        cm.set(hash(++i, Clip(N*3-NN*3+NNN)-px, LogMeanDiffQt(W,Clip(NW*2-NNW))));
        cm.set(hash(i>>8, Clip(W*3-WW*3+WWW)-px));
        cm.set(hash(++i, Clip(W*3-WW*3+WWW)-px, LogMeanDiffQt(N,Clip(NW*2-NWW))));
        cm.set(hash(i>>8, Clip( (35*N-35*NNN+21*buffer(w*5)-5*buffer(w*7))/16 )-px ));
        cm.set(hash(++i, (W+Clip(NE*3-NNE*3+buffer(w*3-stride)))/2-px, R2));
        cm.set(hash(++i, (W+Clamp4(NE*3-NNE*3+buffer(w*3-stride),W,N,NE,NEE))/2-px, LogMeanDiffQt(N,(NW+NE)/2)));
        cm.set(hash(++i, (W+NEE)/2-px, R1/2));
        cm.set(hash(++i, Clamp4(Clip(W*2-WW)+Clip(N*2-NN)-Clip(NW*2-NNWW),W,NW,N,NE)-px));
        cm.set(hash(++i, buf(stride+(x<=stride)), buf(1+(x<2)), buf(2+(x<3))));
        cm.set(hash(++i, buf(1+(x<2)), px));
        cm.set(~0x5ca1ab1e);

        ctx = (min(color,stride-1)<<9)|((abs(W-N)>8)<<8)|((W>N)<<7)|((W>NW)<<6)|((abs(N-NW)>8)<<5)|((N>NW)<<4)|((N>NE)<<3)|min(5, filterOn?filter+1:0);
      }

      Map[0].set( ((U8)(Clip(W+N-NW)-px))|(LogMeanDiffQt(Clip(N+NE-NNE),Clip(N+NW-NNW))<<8) );
      Map[1].set( ((U8)(Clip(N*2-NN)-px))|(LogMeanDiffQt(W,Clip(NW*2-NNW))<<8) );
      Map[2].set( ((U8)(Clip(W*2-WW)-px))|(LogMeanDiffQt(N,Clip(NW*2-NWW))<<8) );
      if (isPNG){
        Map[3].set( ((U8)(Clip(W+N-NW)-px))|(LogMeanDiffQt(buffer(1),Clip(buffer(stride+1)+buffer(w+1)-buffer(w+stride+1)))<<8) );
        Map[4].set( (min(color,stride-1)<<8)|((U8)( Clip(N+buffer(1)-buffer(w+1))-px )) );
        Map[5].set( Clip((-buffer(4*stride)+5*WWW-10*WW+10*W+Clamp4(NE*4-NNE*6+buffer(w*3-stride)*4-buf(w*4-stride),N,NE,buffer(w-2*stride),buffer(w-3*stride)))/5)-px );
        Map[6].set( (W+Clamp4(NE*3-NNE*3+buffer(w*3-stride),W,N,NE,NEE))/2-px );
        Map[7].set( Clip((buffer(w*5)-6*buffer(w*4)+15*NNN-20*NN+15*N+Clamp4(W*4-NWW*6+buffer(w*2+3*stride)*4-buffer(w*3+4*stride),W,NW,N,NN))/6)-px );
        Map[8].set( Clip((-3*WW+8*W+Clamp4(NEE*3-NNEE*3+buffer(w*3-stride*2),NE,NEE,buffer(w-3*stride),buffer(w-4*stride)))/6)-px );
        Map[9].set( Clip((buffer(w*3-3*stride)-4*NNEE+6*NE+Clip(W*4-NW*6+NNW*4-buffer(w*3+stride)))/4)-px );
        Map[10].set( Clip(W+N-NW+buffer(1)-Clip(buffer(stride+1)+buffer(w+1)-buffer(w+stride+1)))-px );
        Map[11].set( Clip(W+N-NW+buffer(2)-Clip(buffer(stride+2)+buffer(w+2)-buffer(w+stride+2)))-px );       
        Map[12].set( Clip(N*2-NN + buffer(1) - Clip(buffer(w+1)*2-buffer(w*2+1)))-px );
        Map[13].set( Clip(W*2-WW + buffer(1) - Clip(buffer(stride+1)*2-buffer(stride*2+1)))-px );
        Map[14].set( Clip(N+NN-NNN+buffer(1)-Clip(buffer(w+1)+buffer(w*2+1)-buffer(w*3+1)) )-px );
        Map[15].set( Clip(W+WW-WWW+buffer(1)-Clip(buffer(stride+1)+buffer(stride*2+1)-buffer(stride*3+1)) )-px );
        Map[16].set(Clip(W+NEE-NE+buffer(1)-Clip(buffer(stride+1)+buffer(w-stride*2+1)-buffer(w-stride+1)))-px);
        Map[17].set(Clip(NN+W-NNW+buffer(1)-Clip(buffer(w*2+1)+buffer(stride+1)-buffer(w*2+stride+1)))-px);
        Map[18].set(Clip(NN+NW-buffer(w*3+stride)+buffer(1)-Clip(buffer(w*2+1)+buffer(w+stride+1)-buffer(w*3+stride+1)))-px);
        Map[19].set(Clip(NN+NE-buffer(w*3-stride)+buffer(1)-Clip(buffer(w*2+1)+buffer(w-stride+1)-buffer(w*3-stride+1)))-px);
        Map[20].set(Clip(NN+buffer(w*4)-buffer(w*6)+buffer(1)-Clip(buffer(w*2+1)+buffer(w*4+1)-buffer(w*6+1)))-px);
        Map[21].set(Clip(WW+buffer(stride*4)-buffer(stride*6)+buffer(1)-Clip(buffer(stride*2+1)+buffer(stride*4+1)-buffer(stride*6+1)))-px);
        Map[22].set(Clip(WW+NEE-N+buffer(1)-Clip(buffer(stride*2+1)+buffer(w-stride*2+1)-buffer(w+1)))-px);
      }
      else{
        Map[3].set( ((U8)Clip(W+N-NW))|(LogMeanDiffQt(buf(1),Clip(buf(stride+1)+buf(w+1)-buf(w+stride+1)))<<8) );
        Map[4].set( (min(color,stride-1)<<8)|Clip(N+buf(1)-buf(w+1)) );
        Map[5].set( Clip((-buf(4*stride)+5*WWW-10*WW+10*W+Clamp4(NE*4-NNE*6+buf(w*3-stride)*4-buf(w*4-stride),N,NE,buf(w-2*stride),buf(w-3*stride)))/5));
        Map[6].set((W+Clamp4(NE*3-NNE*3+buf(w*3-stride),W,N,NE,NEE))/2);
        Map[7].set( Clip((buf(w*5)-6*buf(w*4)+15*NNN-20*NN+15*N+Clamp4(W*4-NWW*6+buf(w*2+3*stride)*4-buf(w*3+4*stride),W,NW,N,NN))/6) );
        Map[8].set( Clip((-3*WW+8*W+Clamp4(NEE*3-NNEE*3+buf(w*3-stride*2),NE,NEE,buf(w-3*stride),buf(w-4*stride)))/6) );
        Map[9].set( Clip((buf(w*3-3*stride)-4*NNEE+6*NE+Clip(W*4-NW*6+NNW*4-buf(w*3+stride)))/4) );
        Map[10].set( Clip(W+N-NW+buf(1)-Clip(buf(stride+1)+buf(w+1)-buf(w+stride+1))) );
        Map[11].set( Clip(W+N-NW+buf(2)-Clip(buf(stride+2)+buf(w+2)-buf(w+stride+2))) );
        Map[12].set( Clip(N*2-NN + buf(1) - Clip(buf(w+1)*2-buf(w*2+1))) );
        Map[13].set( Clip(W*2-WW + buf(1) - Clip(buf(stride+1)*2-buf(stride*2+1))) );
        Map[14].set( Clip(N+NN-NNN));
        Map[15].set( Clip(W+WW-WWW));
        Map[16].set(Clip(W+NEE-NE));
        Map[17].set(Clip(NN+W-NNW));
        Map[18].set(Clip(NN+NW-buf(w*3+stride)));
        Map[19].set(Clip(NN+NE-buf(w*3-stride)));
        Map[20].set(Clip(NN+buf(w*4)-buf(w*6)));
        Map[21].set(Clip(WW+buf(stride*4)-buf(stride*6)));
        Map[22].set(Clip(WW+NEE-N+buf(1)-Clip(buf(stride*2+1)+buf(w-stride*2+1)-buf(w+1))));
      }
      Map[23].set(Clip(W+N-NW)-px);
      Map[24].set(buf(1+(isPNG && x<2)));
      Map[25].set((W+NEE)/2-px);
      Map[26].set(Clip(N*3-NN*3+NNN)-px);
      Map[27].set(Clip(N+NW-NNW)-px);
      Map[28].set(Clip(N+NE-NNE)-px);
    }
  }

  // Predict next bit
  if (x || !isPNG){
    cm.mix(m);
    for (int i=0;i<nMaps;i++)
      Map[i].mix(m);
    if (!isPNG){
      scm1.mix(m);
      scm2.mix(m);
      scm3.mix(m);
      scm4.mix(m);
      scm5.mix(m);
      scm6.mix(m);
      scm7.mix(m);
      scm8.mix(m);
      scm9.mix(m);
      scm_fixed.mix(m);
    }
    static int col=0;
    if (++col>=stride*8) col=0;
    m.set(5, 6);
    m.set(min(63,column[0])+((ctx>>3)&0xC0), 256);
    m.set(min(127,column[1])+((ctx>>2)&0x180), 512);
    m.set(ctx, 2048);
    m.set(col+(isPNG?(ctx&7)+1:(c0==((0x100|((N+W)/2))>>(8-bpos))))*32, 8*32);
    m.set(((isPNG?buffer(1):0)>>4)*stride+(x%stride) + min(5,filterOn?filter+1:0)*64, 6*64);
    m.set(c0+256*(isPNG && abs(R1-128)>8), 256*2);
  }
  else{
    m.add( -2048+((filter>>(7-bpos))&1)*4096 );
    m.set(min(4,filter), 6);
  }
}

//////////////////////////// im8bitModel /////////////////////////////////

// Model for 8-bit image data
void im8bitModel(Mixer& m, int w, int gray = 0, int isPNG=0) {
  static const int nMaps = 20;
  static ContextMap cm(MEM*4, 48);
  static StationaryMap Map[nMaps] = { 12, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0 };
  static RingBuffer buffer(0x100000); // internal rotating buffer for PNG unfiltered pixel data
  static U8 WWW, WW, W, NWW, NW, N, NE, NEE, NNWW, NNW, NN, NNE, NNEE, NNN; //pixel neighborhood
  static U8 px = 0, res = 0; // current PNG filter prediction, expected residual
  static int ctx, lastPos=0, lastWasPNG=0, col=0, line=0, x=0, filter=0;
  static bool filterOn = false;
  static int columns[2] = {1,1}, column[2];
  // Select nearby pixels as context
  if (bpos==0) {
    if (pos!=lastPos+1){
      x = line = px= 0;
      filterOn = false;
      columns[0] = max(1,w/max(1,ilog2(w)*2));
      columns[1] = max(1,columns[0]/max(1,ilog2(columns[0])));
      if (gray){
        if (lastPos && lastWasPNG!=isPNG){
          for (int i=0;i<nMaps;i++)
            Map[i].Reset();
        }
        lastWasPNG = isPNG;
      }
    }
    else{
      x++;
      if(x>=w+isPNG){x=0;line++;}
    }
    lastPos = pos;

    if (isPNG){
      if (x==1)
        filter = (U8)c4;
      else{
        U8 B = (U8)c4;

        switch (filter){
          case 1: {
            buffer.Add((U8)( B + buffer(1)*(x>2 || !x) ) );
            filterOn = x>1;
            px = buffer(1);
            break;
          }
          case 2: {
            buffer.Add((U8)( B + buffer(w)*(filterOn=(line>0)) ) );
            px = buffer(w);
            break;
          }
          case 3: {
            buffer.Add((U8)( B + (buffer(w)*(line>0) + buffer(1)*(x>2 || !x))/2 ) );
            filterOn = (x>1 || line>0);
            px = (buffer(1)*(x>1)+buffer(w)*(line>0))/2;
            break;
          }
          case 4: {
            buffer.Add((U8)( B + Paeth(buffer(1)*(x>2 || !x), buffer(w)*(line>0), buffer(w+1)*(line>0 && (x>2 || !x))) ) );
            filterOn = (x>1 || line>0);
            px = Paeth(buffer(1)*(x>1),buffer(w)*(line>0),buffer(w+1)*(x>1 && line>0));
            break;
          }
          default: buffer.Add(B);
            filterOn = false;
            px = 0;
        }
        if(!filterOn)px=0;
      }
    }  
    if (x || !isPNG){
      int i= filterOn ? (filter+1)<<6 : 0;
      column[0]=(x-isPNG)/columns[0];
      column[1]=(x-isPNG)/columns[1];
      if (isPNG)
        WWW=buffer(3), WW=buffer(2), W=buffer(1), NWW=buffer(w+2), NW=buffer(w+1), N=buffer(w), NE=buffer(w-1), NEE=buffer(w-2), NNWW=buffer(w*2+2), NNW=buffer(w*2+1), NN=buffer(w*2), NNE=buffer(w*2-1), NNEE=buffer(w*2-2), NNN=buffer(w*3);
      else
        WWW=buf(3), WW=buf(2), W=buf(1), NWW=buf(w+2), NW=buf(w+1), N=buf(w), NE=buf(w-1), NEE=buf(w-2), NNW=buf(w*2+1), NN=buf(w*2), NNE=buf(w*2-1), NNN=buf(w*3);

      if (!gray){
        cm.set(hash(++i, W, px));
        cm.set(hash(++i, W, px, column[0]));
        cm.set(hash(++i, N, px));
        cm.set(hash(++i, N, px, column[0]));
        cm.set(hash(++i, NW, px));
        cm.set(hash(++i, NW, px, column[0]));
        cm.set(hash(++i, NE, px));
        cm.set(hash(++i, NE, px, column[0]));
        cm.set(hash(++i, NWW, px));
        cm.set(hash(++i, NEE, px));
        cm.set(hash(++i, WW, px));
        cm.set(hash(++i, NN, px));
        cm.set(hash(++i, W, N, px));
        cm.set(hash(++i, W, NW, px));
        cm.set(hash(++i, W, NE, px));
        cm.set(hash(++i, W, NEE, px));
        cm.set(hash(++i, W, NWW, px));
        cm.set(hash(++i, N, NW, px));
        cm.set(hash(++i, N, NE, px));
        cm.set(hash(++i, NW, NE, px));
        cm.set(hash(++i, W, WW, px));
        cm.set(hash(++i, N, NN, px));
        cm.set(hash(++i, NW, NNWW, px));
        cm.set(hash(++i, NE, NNEE, px));
        cm.set(hash(++i, NW, NWW, px));
        cm.set(hash(++i, NW, NNW, px));
        cm.set(hash(++i, NE, NEE, px));
        cm.set(hash(++i, NE, NNE, px));
        cm.set(hash(++i, N, NNW, px));
        cm.set(hash(++i, N, NNE, px));
        cm.set(hash(++i, N, NNN, px));
        cm.set(hash(++i, W, WWW, px));
        cm.set(hash(++i, WW, NEE, px));
        cm.set(hash(++i, WW, NN, px));
        if (isPNG){
          cm.set(hash(++i, W, buffer(w-3), px));
          cm.set(hash(++i, W, buffer(w-4), px));
        }
        else{
          cm.set(hash(++i, W, buf(w-3)));
          cm.set(hash(++i, W, buf(w-4)));
        }
        cm.set(hash(++i, W, hash(N,NW)&0x7FF, px));
        cm.set(hash(++i, N, hash(NN,NNN)&0x7FF, px));
        cm.set(hash(++i, W, hash(NE,NEE)&0x7FF, px));
        cm.set(hash(++i, W, hash(NW,N,NE)&0x7FF, px));
        cm.set(hash(++i, N, hash(NE,NN,NNE)&0x7FF, px));
        cm.set(hash(++i, N, hash(NW,NNW,NN)&0x7FF, px));
        cm.set(hash(++i, W, hash(WW,NWW,NW)&0x7FF, px));
        cm.set(hash(++i, W, hash(NW,N)&0xFF, hash(WW,NWW)&0xFF, px));
        cm.set(hash(++i, px, column[0]));
        cm.set(hash(++i, px));

        cm.set(hash(++i, N, px, column[1] ));
        cm.set(hash(++i, W, px, column[1] ));
        

        ctx = min(0x1F,(x-1)/min(0x20,columns[0]));
        res = W;
      }
      else{
        cm.set(hash(++i, N, px));
        cm.set(hash(++i, N-px));
        cm.set(hash(++i, W, px));
        cm.set(hash(++i, NW, px));
        cm.set(hash(++i, NE, px));
        cm.set(hash(++i, N, NN, px));
        cm.set(hash(++i, W, WW, px));
        cm.set(hash(++i, NE, NNEE, px ));
        cm.set(hash(++i, NW, NNWW, px ));
        cm.set(hash(++i, W, NEE, px));
        cm.set(hash(++i, (Clamp4(W+N-NW,W,NW,N,NE)-px)/2, LogMeanDiffQt(Clip(N+NE-NNE), Clip(N+NW-NNW))));
        cm.set(hash(++i, (W-px)/4, (NE-px)/4, column[0]));
        cm.set(hash(++i, (Clip(W*2-WW)-px)/4, (Clip(N*2-NN)-px)/4));
        cm.set(hash(++i, (Clamp4(N+NE-NNE,W,N,NE,NEE)-px)/4, column[0]));
        cm.set(hash(++i, (Clamp4(N+NW-NNW,W,NW,N,NE)-px)/4, column[0]));
        cm.set(hash(++i, (W+NEE)/4, px, column[0]));
        cm.set(hash(++i, Clip(W+N-NW)-px, column[0]));
        cm.set(hash(++i, Clamp4(N*3-NN*3+NNN,W,N,NN,NE), px, LogMeanDiffQt(W,Clip(NW*2-NNW))));
        cm.set(hash(++i, Clamp4(W*3-WW*3+WWW,W,N,NE,NEE), px, LogMeanDiffQt(N,Clip(NW*2-NWW))));
        cm.set(hash(++i, (W+Clamp4(NE*3-NNE*3+(isPNG?buffer(w*3-1):buf(w*3-1)),W,N,NE,NEE))/2, px, LogMeanDiffQt(N,(NW+NE)/2)));
        cm.set(hash(++i, (N+NNN)/8, Clip(N*3-NN*3+NNN)/4, px));
        cm.set(hash(++i, (W+WWW)/8, Clip(W*3-WW*3+WWW)/4, px));
        if (isPNG)
          cm.set(hash(++i, Clip((-buffer(4)+5*WWW-10*WW+10*W+Clamp4(NE*4-NNE*6+buffer(w*3-1)*4-buffer(w*4-1),N,NE,buffer(w-2),buffer(w-3)))/5)-px));
        else
          cm.set(hash(++i, Clip((-buf(4)+5*WWW-10*WW+10*W+Clamp4(NE*4-NNE*6+buf(w*3-1)*4-buf(w*4-1),N,NE,buf(w-2),buf(w-3)))/5)));
        cm.set(hash(++i, Clip(N*2-NN)-px, LogMeanDiffQt(N,Clip(NN*2-NNN))));
        cm.set(hash(++i, Clip(W*2-WW)-px, LogMeanDiffQt(NE,Clip(N*2-NW))));
        cm.set(~0xde7ec7ed);

        Map[0].set( ((U8)(Clip(W+N-NW)-px))|(LogMeanDiffQt(Clip(N+NE-NNE),Clip(N+NW-NNW))<<8) );
        Map[1].set(Clamp4(W+N-NW,W,NW,N,NE)-px);
        Map[2].set(Clip(W+N-NW)-px);
        Map[3].set(Clamp4(W+NE-N,W,NW,N,NE)-px);
        Map[4].set(Clip(W+NE-N)-px);
        Map[5].set(Clamp4(N+NW-NNW,W,NW,N,NE)-px);
        Map[6].set(Clip(N+NW-NNW)-px);
        Map[7].set(Clamp4(N+NE-NNE,W,N,NE,NEE)-px);
        Map[8].set(Clip(N+NE-NNE)-px);
        Map[9].set((W+NEE)/2-px);
        Map[10].set(Clip(N*3-NN*3+NNN)-px);
        Map[11].set(Clip(W*3-WW*3+WWW)-px);
        if (isPNG){
          Map[12].set((W+Clip(NE*3-NNE*3+buffer(w*3-1)))/2-px);
          Map[13].set((W+Clip(NEE*3-buffer(w*2-3)*3+buffer(w*3-4)))/2-px);
          Map[14].set(Clip(NN+buffer(w*4)-buffer(w*6))-px);
          Map[15].set(Clip(WW+buffer(4)-buffer(6))-px);
        }
        else{
          Map[12].set((W+Clip(NE*3-NNE*3+buf(w*3-1)))/2);
          Map[13].set((W+Clip(NEE*3-buf(w*2-3)*3+buf(w*3-4)))/2);
          Map[14].set(Clip(NN+buf(w*4)-buf(w*6)));
          Map[15].set(Clip(WW+buf(4)-buf(6)));
        }
        Map[16].set(Clip(N+NN-NNN)-px);
        Map[17].set(Clip(W+WW-WWW)-px);
        Map[18].set(Clip(W+NEE-NE)-px);

        if (isPNG)
          ctx = ((abs(W-N)>8)<<10)|((W>N)<<9)|((abs(N-NW)>8)<<8)|((N>NW)<<7)|((abs(N-NE)>8)<<6)|((N>NE)<<5)|((W>WW)<<4)|((N>NN)<<3)|min(5,filterOn?filter+1:0);
        else
          ctx = min(0x1F,x/max(1,w/min(32,columns[0])))|( ( ((abs(W-N)*16>W+N)<<1)|(abs(N-NW)>8) )<<5 )|((W+N)&0x180);

        res = Clamp4(W+N-NW,W,NW,N,NE)-px;
      }
    }
  }

  // Predict next bit
  if (x || !isPNG){
    cm.mix(m);
    if (gray){
      for (int i=0;i<nMaps;i++)
        Map[i].mix(m);
    }
    col=(col+1)&7;
    m.set(5+ctx, 2048+5);
    m.set(col*2+(isPNG && c0==((0x100|res)>>(8-bpos))) + min(5, filterOn?filter+1:0)*16, 6*16);
    m.set(((isPNG?px:buf(w)+buf(1))>>4) + min(5, filterOn?filter+1:0)*32, 6*32);
    m.set(c0, 256);
    m.set( ((abs((int)(W-N))>4)<<9)|((abs((int)(N-NE))>4)<<8)|((abs((int)(W-NW))>4)<<7)|((W>N)<<6)|((N>NE)<<5)|((W>NW)<<4)|((W>WW)<<3)|((N>NN)<<2)|((NW>NNWW)<<1)|(NE>NNEE), 1024 );
    m.set(min(63,column[0]), 64);
    m.set(min(127,column[1]), 128);
  }
  else{
    m.add( -2048+((filter>>(7-bpos))&1)*4096 );
    m.set(min(4,filter),5);
  }
}

//////////////////////////// im4bitModel /////////////////////////////////
/*
  Model for 4-bit image data

  Changelog:
  (31/08/2017) v103: Initial release by Márcio Pais
*/
void im4bitModel(Mixer& m, int w) {
  static HashTable<16> t(MEM/2);
  const int S=11; // number of contexts
  static U8* cp[S]; // context pointers
  static StateMap sm[S];
  static U8 WW=0, W=0, NWW=0, NW=0, N=0, NE=0, NEE=0, NNWW = 0, NNW=0, NN=0, NNE=0, NNEE=0;
  static int col=0, line=0, run=0, prevColor=0, px=0;
  int i;
  if (!cp[0]){
    for (i=0;i<S;i++)
      cp[i]=t[263*i]; //set the initial context to an arbitrary slot in the hashtable
  }
  for (i=0;i<S;i++)
    *cp[i]=nex(*cp[i],y); //update hashtable item priorities using predicted counts

  if (bpos==0 || bpos==4){
      WW=W; NWW=NW; NW=N; N=NE; NE=NEE; NNWW=NWW; NNW=NN; NN=NNE; NNE=NNEE;
      if (bpos==0)
        {W=c4&0xF; NEE=buf(w-1)>>4; NNEE=buf(w*2-1)>>4;}
      else
        {W=c0&0xF; NEE=buf(w-1)&0xF; NNEE=buf(w*2-1)&0xF;}
      run=(W!=WW || col==0)?(prevColor=WW,0):min(0xFFF,run+1);
      px=1; //partial pixel (4 bits) with a leading "1"
      i=0;

      cp[i++]=t[hash(W,NW,N)];
      cp[i++]=t[hash(N, min(0xFFF, col/8))];
      cp[i++]=t[hash(W,NW,N,NN,NE)];
      cp[i++]=t[hash(W, N, NE+NNE*16, NEE+NNEE*16)];
      cp[i++]=t[hash(W, N, NW+NNW*16, NWW+NNWW*16)];
      cp[i++]=t[hash(W, ilog2(run+1), prevColor, col/max(1,w/2) )];
      cp[i++]=t[hash(NE, min(0x3FF, (col+line)/max(1,w*8)))];
      cp[i++]=t[hash(NW, (col-line)/max(1,w*8))];
      cp[i++]=t[hash(WW*16+W,NN*16+N,NNWW*16+NW)];
      cp[i++]=t[N+NN*16];
      cp[i++]=t[-1];
      assert(i==S);

      col++;
      if(col==w*2){col=0;line++;}
  }
  else{
    px+=px+y;
    int j=(y+1)<<(bpos&3);
    for (i=0;i<S;i++)
      cp[i]+=j;
  }

  // predict
  for (i=0; i<S; i++)
    m.add(stretch(sm[i].p(*cp[i])));

  m.set(W*16+px, 256);
  m.set(min(31,col/max(1,w/16))+N*32, 512);
  m.set((bpos&3)+4*W+64*min(7,ilog2(run+1)), 512);
  m.set(W+NE*16+(bpos&3)*256, 1024);
  m.set(px, 16);
  m.set(0,1);
}

//////////////////////////// im1bitModel /////////////////////////////////

// Model for 1-bit image data

void im1bitModel(Mixer& m, int w) {
  static U32 r0, r1, r2, r3;  // last 4 rows, bit 8 is over current pixel
  static Array<U8> t(0x23000);  // model: cxt -> state
  const int N=11;  // number of contexts
  static int cxt[N];  // contexts
  static StateMap sm[N];

  // update the model
  int i;
  for (i=0; i<N; ++i)
    t[cxt[i]]=nex(t[cxt[i]],y);

  // update the contexts (pixels surrounding the predicted one)
  r0+=r0+y;
  r1+=r1+((buf(w-1)>>(7-bpos))&1);
  r2+=r2+((buf(w+w-1)>>(7-bpos))&1);
  r3+=r3+((buf(w+w+w-1)>>(7-bpos))&1);
  cxt[0]=(r0&0x7)|(r1>>4&0x38)|(r2>>3&0xc0);
  cxt[1]=0x100+((r0&1)|(r1>>4&0x3e)|(r2>>2&0x40)|(r3>>1&0x80));
  cxt[2]=0x200+((r0&1)|(r1>>4&0x1d)|(r2>>1&0x60)|(r3&0xC0));
  cxt[3]=0x300+(y|((r0<<1)&4)|((r1>>1)&0xF0)|((r2>>3)&0xA));
  cxt[4]=0x400+((r0>>4&0x2AC)|(r1&0xA4)|(r2&0x349)|(!(r3&0x14D)));
  cxt[5]=0x800+(y|((r1>>4)&0xE)|((r2>>1)&0x70)|((r3<<2)&0x380));
  cxt[6]=0xC00+(((r1&0x30)^(r3&0x0c0c))|(r0&3));
  cxt[7]=0x1000+((!(r0&0x444))|(r1&0xC0C)|(r2&0xAE3)|(r3&0x51C));
  cxt[8]=0x2000+((r0&7)|((r1>>1)&0x3F8)|((r2<<5)&0xC00));
  cxt[9]=0x3000+((r0&0x3f)^(r1&0x3ffe)^(r2<<2&0x7f00)^(r3<<5&0xf800));
  cxt[10]=0x13000+((r0&0x3e)^(r1&0x0c0c)^(r2&0xc800));

  // predict
  for (i=0; i<N; ++i) m.add(stretch(sm[i].p(t[cxt[i]])));
}

//////////////////////////// jpegModel /////////////////////////

// Model JPEG. Return 1 if a JPEG file is detected or else 0.
// Only the baseline and 8 bit extended Huffman coded DCT modes are
// supported.  The model partially decodes the JPEG image to provide
// context for the Huffman coded symbols.

// Print a JPEG segment at buf[p...] for debugging
/*
void dump(const char* msg, int p) {
  printf("%s:", msg);
  int len=buf[p+2]*256+buf[p+3];
  for (int i=0; i<len+2; ++i)
    printf(" %02X", buf[p+i]);
  printf("\n");
}
*/

#define finish(success){ \
  int length = pos - images[idx].offset; \
  /*if (success && idx && pos-lastPos==1)*/ \
    /*printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bEmbedded JPEG at offset %d, size: %d bytes, level %d\nCompressing... ", images[idx].offset-pos+blpos, length, idx), fflush(stdout);*/ \
  memset(&images[idx], 0, sizeof(JPEGImage)); \
  mcusize=0,dqt_state=-1; \
  idx-=(idx>0); \
  images[idx].app-=length; \
  if (images[idx].app < 0) \
    images[idx].app = 0; \
}

// Detect invalid JPEG data.  The proper response is to silently
// fall back to a non-JPEG model.
#define jassert(x) if (!(x)) { \
/*  printf("JPEG error at %d, line %d: %s\n", pos, __LINE__, #x); */ \
  if (idx>0) \
    finish(false) \
  else \
    images[idx].jpeg=0; \
  return images[idx].next_jpeg;}

struct HUF {U32 min, max; int val;}; // Huffman decode tables
  // huf[Tc][Th][m] is the minimum, maximum+1, and pointer to codes for
  // coefficient type Tc (0=DC, 1=AC), table Th (0-3), length m+1 (m=0-15)

struct JPEGImage{
  int offset, // offset of SOI marker
  jpeg, // 1 if JPEG is header detected, 2 if image data
  next_jpeg, // updated with jpeg on next byte boundary
  app, // Bytes remaining to skip in this marker
  sof, sos, data, // pointers to buf
  htsize; // number of pointers in ht
  int ht[8]; // pointers to Huffman table headers
  U8 qtab[256]; // table
  int qmap[10]; // block -> table number
};

int jpegModel(Mixer& m) {

  // State of parser
  enum {SOF0=0xc0, SOF1, SOF2, SOF3, DHT, RST0=0xd0, SOI=0xd8, EOI, SOS, DQT,
    DNL, DRI, APP0=0xe0, COM=0xfe, FF};  // Second byte of 2 byte codes
  const static int MaxEmbeddedLevel = 3;
  static JPEGImage images[MaxEmbeddedLevel];
  static int idx=-1;
  static int lastPos=0;

  // Huffman decode state
  static U32 huffcode=0;  // Current Huffman code including extra bits
  static int huffbits=0;  // Number of valid bits in huffcode
  static int huffsize=0;  // Number of bits without extra bits
  static int rs=-1;  // Decoded huffcode without extra bits.  It represents
    // 2 packed 4-bit numbers, r=run of zeros, s=number of extra bits for
    // first nonzero code.  huffcode is complete when rs >= 0.
    // rs is -1 prior to decoding incomplete huffcode.

  static int mcupos=0;  // position in MCU (0-639).  The low 6 bits mark
    // the coefficient in zigzag scan order (0=DC, 1-63=AC).  The high
    // bits mark the block within the MCU, used to select Huffman tables.

  // Decoding tables
  static Array<HUF> huf(128);  // Tc*64+Th*16+m -> min, max, val
  static int mcusize=0;  // number of coefficients in an MCU
  static int hufsel[2][10];  // DC/AC, mcupos/64 -> huf decode table
  static Array<U8> hbuf(2048);  // Tc*1024+Th*256+hufcode -> RS

  // Image state
  static Array<int> color(10);  // block -> component (0-3)
  static Array<int> pred(4);  // component -> last DC value
  static int dc=0;  // DC value of the current block
  static int width=0;  // Image width in MCU
  static int row=0, column=0;  // in MCU (column 0 to width-1)
  static Buf cbuf(0x20000); // Rotating buffer of coefficients, coded as:
    // DC: level shifted absolute value, low 4 bits discarded, i.e.
    //   [-1023...1024] -> [0...255].
    // AC: as an RS code: a run of R (0-15) zeros followed by an S (0-15)
    //   bit number, or 00 for end of block (in zigzag order).
    //   However if R=0, then the format is ssss11xx where ssss is S,
    //   xx is the first 2 extra bits, and the last 2 bits are 1 (since
    //   this never occurs in a valid RS code).
  static int cpos=0;  // position in cbuf
  static int rs1;  // last RS code
  static int rstpos=0,rstlen=0; // reset position
  static int ssum=0, ssum1=0, ssum2=0, ssum3=0;
    // sum of S in RS codes in block and sum of S in first component

  static IntBuf cbuf2(0x20000);
  static Array<int> adv_pred(4), sumu(8), sumv(8), run_pred(6);
  static int prev_coef=0, prev_coef2=0, prev_coef_rs=0;
  static Array<int> ls(10);  // block -> distance to previous block
  static Array<int> blockW(10), blockN(10), SamplingFactors(4);
  static Array<int> lcp(7), zpos(64);

    //for parsing Quantization tables
  static int dqt_state = -1, dqt_end = 0, qnum = 0;

  const static U8 zzu[64]={  // zigzag coef -> u,v
    0,1,0,0,1,2,3,2,1,0,0,1,2,3,4,5,4,3,2,1,0,0,1,2,3,4,5,6,7,6,5,4,
    3,2,1,0,1,2,3,4,5,6,7,7,6,5,4,3,2,3,4,5,6,7,7,6,5,4,5,6,7,7,6,7};
  const static U8 zzv[64]={
    0,0,1,2,1,0,0,1,2,3,4,3,2,1,0,0,1,2,3,4,5,6,5,4,3,2,1,0,0,1,2,3,
    4,5,6,7,7,6,5,4,3,2,1,2,3,4,5,6,7,7,6,5,4,3,4,5,6,7,7,6,5,6,7,7};

  // Standard Huffman tables (cf. JPEG standard section K.3)
  // IMPORTANT: these are only valid for 8-bit data precision
  const static U8 bits_dc_luminance[16] = {
    0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0
  };
  const static U8 values_dc_luminance[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
  };

  const static U8 bits_dc_chrominance[16] = {
    0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
  };
  const static U8 values_dc_chrominance[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
  };

  const static U8 bits_ac_luminance[16] = {
    0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d
  };
  const static U8 values_ac_luminance[162] = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
    0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
    0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
    0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
    0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
    0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
  };

  const static U8 bits_ac_chrominance[16] = {
    0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77
  };
  const static U8 values_ac_chrominance[162] = {
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
    0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
    0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
    0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
  };

  if (idx < 0){
    memset(&images[0], 0, sizeof(images));
    idx = 0;
    lastPos = pos;
  }

  // Be sure to quit on a byte boundary
  if (bpos==0) images[idx].next_jpeg=images[idx].jpeg>1;
  if (bpos!=0 && !images[idx].jpeg) return images[idx].next_jpeg;
  if (bpos==0 && images[idx].app>0){
    --images[idx].app;
    if (idx<MaxEmbeddedLevel && buf(4)==FF && buf(3)==SOI && buf(2)==FF && ((buf(1)&0xFE)==0xC0 || buf(1)==0xC4 || (buf(1)>=0xDB && buf(1)<=0xFE)) )
      memset(&images[++idx], 0, sizeof(JPEGImage));
  }
  if (images[idx].app>0) return images[idx].next_jpeg;
  if (bpos==0) {

    // Parse.  Baseline DCT-Huffman JPEG syntax is:
    // SOI APPx... misc... SOF0 DHT... SOS data EOI
    // SOI (= FF D8) start of image.
    // APPx (= FF Ex) len ... where len is always a 2 byte big-endian length
    //   including the length itself but not the 2 byte preceding code.
    //   Application data is ignored.  There may be more than one APPx.
    // misc codes are DQT, DNL, DRI, COM (ignored).
    // SOF0 (= FF C0) len 08 height width Nf [C HV Tq]...
    //   where len, height, width (in pixels) are 2 bytes, Nf is the repeat
    //   count (1 byte) of [C HV Tq], where C is a component identifier
    //   (color, 0-3), HV is the horizontal and vertical dimensions
    //   of the MCU (high, low bits, packed), and Tq is the quantization
    //   table ID (not used).  An MCU (minimum compression unit) consists
    //   of 64*H*V DCT coefficients for each color.
    // DHT (= FF C4) len [TcTh L1...L16 V1,1..V1,L1 ... V16,1..V16,L16]...
    //   defines Huffman table Th (1-4) for Tc (0=DC (first coefficient)
    //   1=AC (next 63 coefficients)).  L1..L16 are the number of codes
    //   of length 1-16 (in ascending order) and Vx,y are the 8-bit values.
    //   A V code of RS means a run of R (0-15) zeros followed by S (0-15)
    //   additional bits to specify the next nonzero value, negative if
    //   the first additional bit is 0 (e.g. code x63 followed by the
    //   3 bits 1,0,1 specify 7 coefficients: 0, 0, 0, 0, 0, 0, 5.
    //   Code 00 means end of block (remainder of 63 AC coefficients is 0).
    // SOS (= FF DA) len Ns [Cs TdTa]... 0 3F 00
    //   Start of scan.  TdTa specifies DC/AC Huffman tables (0-3, packed
    //   into one byte) for component Cs matching C in SOF0, repeated
    //   Ns (1-4) times.
    // EOI (= FF D9) is end of image.
    // Huffman coded data is between SOI and EOI.  Codes may be embedded:
    // RST0-RST7 (= FF D0 to FF D7) mark the start of an independently
    //   compressed region.
    // DNL (= FF DC) 04 00 height
    //   might appear at the end of the scan (ignored).
    // FF 00 is interpreted as FF (to distinguish from RSTx, DNL, EOI).

    // Detect JPEG (SOI followed by a valid marker)
    if (!images[idx].jpeg && buf(4)==FF && buf(3)==SOI && buf(2)==FF && ((buf(1)&0xFE)==0xC0 || buf(1)==0xC4 || (buf(1)>=0xDB && buf(1)<=0xFE)) ){
      images[idx].jpeg=1;
      images[idx].offset = pos-4;
      images[idx].sos=images[idx].sof=images[idx].htsize=images[idx].data=0, images[idx].app=(buf(1)>>4==0xE)*2;
      mcusize=huffcode=huffbits=huffsize=mcupos=cpos=0, rs=-1;
      memset(&huf[0], 0, sizeof(huf));
      memset(&pred[0], 0, pred.size()*sizeof(int));
      rstpos=rstlen=0;
    }

    // Detect end of JPEG when data contains a marker other than RSTx
    // or byte stuff (00), or if we jumped in position since the last byte seen
    if (images[idx].jpeg && images[idx].data && ((buf(2)==FF && buf(1) && (buf(1)&0xf8)!=RST0) || (pos-lastPos>1)) ) {
      jassert((buf(1)==EOI) || (pos-lastPos>1));
      finish(true);
    }
    lastPos = pos;
    if (!images[idx].jpeg) return images[idx].next_jpeg;

    // Detect APPx, COM or other markers, so we can skip them
    if (!images[idx].data && !images[idx].app && buf(4)==FF && (((buf(3)>0xC1) && (buf(3)<=0xCF) && (buf(3)!=DHT)) || ((buf(3)>=0xDC) && (buf(3)<=0xFE)))){
      images[idx].app=buf(2)*256+buf(1)+2;
      if (idx>0)
        jassert( pos + images[idx].app < images[idx].offset + images[idx-1].app );
    }

    // Save pointers to sof, ht, sos, data,
    if (buf(5)==FF && buf(4)==SOS) {
      int len=buf(3)*256+buf(2);
      if (len==6+2*buf(1) && buf(1) && buf(1)<=4)  // buf(1) is Ns
        images[idx].sos=pos-5, images[idx].data=images[idx].sos+len+2, images[idx].jpeg=2;
    }
    if (buf(4)==FF && buf(3)==DHT && images[idx].htsize<8) images[idx].ht[images[idx].htsize++]=pos-4;
    if (buf(4)==FF && (buf(3)&0xFE)==SOF0) images[idx].sof=pos-4;

    // Parse Quantizazion tables
    if (buf(4)==FF && buf(3)==DQT)
      dqt_end=pos+buf(2)*256+buf(1)-1, dqt_state=0;
    else if (dqt_state>=0) {
      if (pos>=dqt_end)
        dqt_state = -1;
      else {
        if (dqt_state%65==0)
          qnum = buf(1);
        else {
          jassert(buf(1)>0);
          jassert(qnum>=0 && qnum<4);
          images[idx].qtab[qnum*64+((dqt_state%65)-1)]=buf(1)-1;
        }
        dqt_state++;
      }
    }

    // Restart
    if (buf(2)==FF && (buf(1)&0xf8)==RST0) {
      huffcode=huffbits=huffsize=mcupos=0, rs=-1;
      memset(&pred[0], 0, pred.size()*sizeof(int));
      rstlen=column+row*width-rstpos;
      rstpos=column+row*width;
    }
  }

  {
    // Build Huffman tables
    // huf[Tc][Th][m] = min, max+1 codes of length m, pointer to byte values
    if (pos==images[idx].data && bpos==1) {
      for (int i=0; i<images[idx].htsize; ++i) {
        int p=images[idx].ht[i]+4;  // pointer to current table after length field
        int end=p+buf[p-2]*256+buf[p-1]-2;  // end of Huffman table
        int count=0;  // sanity check
        while (p<end && end<pos && end<p+2100 && ++count<10) {
          int tc=buf[p]>>4, th=buf[p]&15;
          if (tc>=2 || th>=4) break;
          jassert(tc>=0 && tc<2 && th>=0 && th<4);
          HUF* h=&huf[tc*64+th*16]; // [tc][th][0];
          int val=p+17;  // pointer to values
          int hval=tc*1024+th*256;  // pointer to RS values in hbuf
          int j;
          for (j=0; j<256; ++j) // copy RS codes
            hbuf[hval+j]=buf[val+j];
          int code=0;
          for (j=0; j<16; ++j) {
            h[j].min=code;
            h[j].max=code+=buf[p+j+1];
            h[j].val=hval;
            val+=buf[p+j+1];
            hval+=buf[p+j+1];
            code*=2;
          }
          p=val;
          jassert(hval>=0 && hval<2048);
        }
        jassert(p==end);
      }
      huffcode=huffbits=huffsize=0, rs=-1;

      // load default tables
      if (!images[idx].htsize){
        for (int tc = 0; tc < 2; tc++) {
          for (int th = 0; th < 2; th++) {
            HUF* h = &huf[tc*64+th*16];
            int hval = tc*1024 + th*256;
            int code = 0, c = 0, x = 0;

            for (int i = 0; i < 16; i++) {
              switch (tc*2+th) {
                case 0: x = bits_dc_luminance[i]; break;
                case 1: x = bits_dc_chrominance[i]; break;
                case 2: x = bits_ac_luminance[i]; break;
                case 3: x = bits_ac_chrominance[i];
              }

              h[i].min = code;
              h[i].max = (code+=x);
              h[i].val = hval;
              hval+=x;
              code+=code;
              c+=x;
            }

            hval = tc*1024 + th*256;
            c--;

            while (c >= 0){
              switch (tc*2+th) {
                case 0: x = values_dc_luminance[c]; break;
                case 1: x = values_dc_chrominance[c]; break;
                case 2: x = values_ac_luminance[c]; break;
                case 3: x = values_ac_chrominance[c];
              }

              hbuf[hval+c] = x;
              c--;
            }
          }
        }
        images[idx].htsize = 4;
      }

      // Build Huffman table selection table (indexed by mcupos).
      // Get image width.
      if (!images[idx].sof && images[idx].sos) return images[idx].next_jpeg;
      int ns=buf[images[idx].sos+4];
      int nf=buf[images[idx].sof+9];
      jassert(ns<=4 && nf<=4);
      mcusize=0;  // blocks per MCU
      int hmax=0;  // MCU horizontal dimension
      for (int i=0; i<ns; ++i) {
        for (int j=0; j<nf; ++j) {
          if (buf[images[idx].sos+2*i+5]==buf[images[idx].sof+3*j+10]) { // Cs == C ?
            int hv=buf[images[idx].sof+3*j+11];  // packed dimensions H x V
            SamplingFactors[j] = hv;
            if (hv>>4>hmax) hmax=hv>>4;
            hv=(hv&15)*(hv>>4);  // number of blocks in component C
            jassert(hv>=1 && hv+mcusize<=10);
            while (hv) {
              jassert(mcusize<10);
              hufsel[0][mcusize]=buf[images[idx].sos+2*i+6]>>4&15;
              hufsel[1][mcusize]=buf[images[idx].sos+2*i+6]&15;
              jassert (hufsel[0][mcusize]<4 && hufsel[1][mcusize]<4);
              color[mcusize]=i;
              int tq=buf[images[idx].sof+3*j+12];  // quantization table index (0..3)
              jassert(tq>=0 && tq<4);
              images[idx].qmap[mcusize]=tq; // quantizazion table mapping
              --hv;
              ++mcusize;
            }
          }
        }
      }
      jassert(hmax>=1 && hmax<=10);
      int j;
      for (j=0; j<mcusize; ++j) {
        ls[j]=0;
        for (int i=1; i<mcusize; ++i) if (color[(j+i)%mcusize]==color[j]) ls[j]=i;
        ls[j]=(mcusize-ls[j])<<6;
      }
      for (j=0; j<64; ++j) zpos[zzu[j]+8*zzv[j]]=j;
      width=buf[images[idx].sof+7]*256+buf[images[idx].sof+8];  // in pixels
      width=(width-1)/(hmax*8)+1;  // in MCU
      jassert(width>0);
      mcusize*=64;  // coefficients per MCU
      row=column=0;

      // we can have more blocks than components then we have subsampling
      int x=0, y=0;
      for (j = 0; j<(mcusize>>6); j++) {
        int i = color[j];
        int w = SamplingFactors[i]>>4, h = SamplingFactors[i]&0xf;
        blockW[j] = x==0?mcusize-64*(w-1):64;
        blockN[j] = y==0?mcusize*width-64*w*(h-1):w*64;
        x++;
        if (x>=w) { x=0; y++; }
        if (y>=h) { x=0; y=0; }
      }
    }
  }


  // Decode Huffman
  {
    if (mcusize && buf(1+(bpos==0))!=FF) {  // skip stuffed byte
      jassert(huffbits<=32);
      huffcode+=huffcode+y;
      ++huffbits;
      if (rs<0) {
        jassert(huffbits>=1 && huffbits<=16);
        const int ac=(mcupos&63)>0;
        jassert(mcupos>=0 && (mcupos>>6)<10);
        jassert(ac==0 || ac==1);
        const int sel=hufsel[ac][mcupos>>6];
        jassert(sel>=0 && sel<4);
        const int i=huffbits-1;
        jassert(i>=0 && i<16);
        const HUF *h=&huf[ac*64+sel*16]; // [ac][sel];
        jassert(h[i].min<=h[i].max && h[i].val<2048 && huffbits>0);
        if (huffcode<h[i].max) {
          jassert(huffcode>=h[i].min);
          int k=h[i].val+huffcode-h[i].min;
          jassert(k>=0 && k<2048);
          rs=hbuf[k];
          huffsize=huffbits;
        }
      }
      if (rs>=0) {
        if (huffsize+(rs&15)==huffbits) { // done decoding
          rs1=rs;
          int x=0;  // decoded extra bits
          if (mcupos&63) {  // AC
            if (rs==0) { // EOB
              mcupos=(mcupos+63)&-64;
              jassert(mcupos>=0 && mcupos<=mcusize && mcupos<=640);
              while (cpos&63) {
                cbuf2[cpos]=0;
                cbuf[cpos]=(!rs)?0:(63-(cpos&63))<<4; cpos++; rs++;
              }
            }
            else {  // rs = r zeros + s extra bits for the next nonzero value
                    // If first extra bit is 0 then value is negative.
              jassert((rs&15)<=10);
              const int r=rs>>4;
              const int s=rs&15;
              jassert(mcupos>>6==(mcupos+r)>>6);
              mcupos+=r+1;
              x=huffcode&((1<<s)-1);
              if (s && !(x>>(s-1))) x-=(1<<s)-1;
              for (int i=r; i>=1; --i) {
                cbuf2[cpos]=0;
                cbuf[cpos++]=i<<4|s;
              }
              cbuf2[cpos]=x;
              cbuf[cpos++]=(s<<4)|(huffcode<<2>>s&3)|12;
              ssum+=s;
            }
          }
          else {  // DC: rs = 0S, s<12
            jassert(rs<12);
            ++mcupos;
            x=huffcode&((1<<rs)-1);
            if (rs && !(x>>(rs-1))) x-=(1<<rs)-1;
            jassert(mcupos>=0 && mcupos>>6<10);
            const int comp=color[mcupos>>6];
            jassert(comp>=0 && comp<4);
            dc=pred[comp]+=x;
            jassert((cpos&63)==0);
            cbuf2[cpos]=dc;
            cbuf[cpos++]=(dc+1023)>>3;
            if ((mcupos>>6)==0) {
              ssum1=0;
              ssum2=ssum3;
            } else {
              if (color[(mcupos>>6)-1]==color[0]) ssum1+=(ssum3=ssum);
              ssum2=ssum1;
            }
            ssum=rs;
          }
          jassert(mcupos>=0 && mcupos<=mcusize);
          if (mcupos>=mcusize) {
            mcupos=0;
            if (++column==width) column=0, ++row;
          }
          huffcode=huffsize=huffbits=0, rs=-1;

          // UPDATE_ADV_PRED !!!!
          {
            const int acomp=mcupos>>6, q=64*images[idx].qmap[acomp];
            const int zz=mcupos&63, cpos_dc=cpos-zz;
            const bool norst=rstpos!=column+row*width;
            if (zz==0) {
              for (int i=0; i<8; ++i) sumu[i]=sumv[i]=0;
              // position in the buffer of first (DC) coefficient of the block
              // of this same component that is to the west of this one, not
              // necessarily in this MCU
              int offset_DC_W = cpos_dc - blockW[acomp];
              // position in the buffer of first (DC) coefficient of the block
              // of this same component that is to the north of this one, not
              // necessarily in this MCU
              int offset_DC_N = cpos_dc - blockN[acomp];
              for (int i=0; i<64; ++i) {
                sumu[zzu[i]]+=(zzv[i]&1?-1:1)*(zzv[i]?16*(16+zzv[i]):185)*(images[idx].qtab[q+i]+1)*cbuf2[offset_DC_N+i];
                sumv[zzv[i]]+=(zzu[i]&1?-1:1)*(zzu[i]?16*(16+zzu[i]):185)*(images[idx].qtab[q+i]+1)*cbuf2[offset_DC_W+i];
              }
            }
            else {
              sumu[zzu[zz-1]]-=(zzv[zz-1]?16*(16+zzv[zz-1]):185)*(images[idx].qtab[q+zz-1]+1)*cbuf2[cpos-1];
              sumv[zzv[zz-1]]-=(zzu[zz-1]?16*(16+zzu[zz-1]):185)*(images[idx].qtab[q+zz-1]+1)*cbuf2[cpos-1];
            }

            for (int i=0; i<3; ++i)
            {
              run_pred[i]=run_pred[i+3]=0;
              for (int st=0; st<10 && zz+st<64; ++st) {
                const int zz2=zz+st;
                int p=sumu[zzu[zz2]]*i+sumv[zzv[zz2]]*(2-i);
                p/=(images[idx].qtab[q+zz2]+1)*185*(16+zzv[zz2])*(16+zzu[zz2])/128;
                if (zz2==0 && (norst || ls[acomp]==64)) p-=cbuf2[cpos_dc-ls[acomp]];
                p=(p<0?-1:+1)*ilog(abs(p)+1);
                if (st==0) {
                  adv_pred[i]=p;
                }
                else if (abs(p)>abs(adv_pred[i])+2 && abs(adv_pred[i]) < 210) {
                  if (run_pred[i]==0) run_pred[i]=st*2+(p>0);
                  if (abs(p)>abs(adv_pred[i])+21 && run_pred[i+3]==0) run_pred[i+3]=st*2+(p>0);
                }
              }
            }
            x=0;
            for (int i=0; i<8; ++i) x+=(zzu[zz]<i)*sumu[i]+(zzv[zz]<i)*sumv[i];
            x=(sumu[zzu[zz]]*(2+zzu[zz])+sumv[zzv[zz]]*(2+zzv[zz])-x*2)*4/(zzu[zz]+zzv[zz]+16);
            x/=(images[idx].qtab[q+zz]+1)*185;
            if (zz==0 && (norst || ls[acomp]==64)) x-=cbuf2[cpos_dc-ls[acomp]];
            adv_pred[3]=(x<0?-1:+1)*ilog(abs(x)+1);

            for (int i=0; i<4; ++i) {
              const int a=(i&1?zzv[zz]:zzu[zz]), b=(i&2?2:1);
              if (a<b) x=65535;
              else {
                const int zz2=zpos[zzu[zz]+8*zzv[zz]-(i&1?8:1)*b];
                x=(images[idx].qtab[q+zz2]+1)*cbuf2[cpos_dc+zz2]/(images[idx].qtab[q+zz]+1);
                x=(x<0?-1:+1)*(ilog(abs(x)+1)+(x!=0?17:0));
              }
              lcp[i]=x;
            }
            if ((zzu[zz]*zzv[zz])!=0){
              const int zz2=zpos[zzu[zz]+8*zzv[zz]-9];
              x=(images[idx].qtab[q+zz2]+1)*cbuf2[cpos_dc+zz2]/(images[idx].qtab[q+zz]+1);
              lcp[4]=(x<0?-1:+1)*(ilog(abs(x)+1)+(x!=0?17:0));

              x=(images[idx].qtab[q+zpos[8*zzv[zz]]]+1)*cbuf2[cpos_dc+zpos[8*zzv[zz]]]/(images[idx].qtab[q+zz]+1);
              lcp[5]=(x<0?-1:+1)*(ilog(abs(x)+1)+(x!=0?17:0));

              x=(images[idx].qtab[q+zpos[zzu[zz]]]+1)*cbuf2[cpos_dc+zpos[zzu[zz]]]/(images[idx].qtab[q+zz]+1);
              lcp[6]=(x<0?-1:+1)*(ilog(abs(x)+1)+(x!=0?17:0));
            }
            else
              lcp[4]=lcp[5]=lcp[6]=65535;

            int prev1=0,prev2=0,cnt1=0,cnt2=0,r=0,s=0;
            prev_coef_rs = cbuf[cpos-64];
            for (int i=0; i<acomp; i++) {
              x=0;
              x+=cbuf2[cpos-(acomp-i)*64];
              if (zz==0 && (norst || ls[i]==64)) x-=cbuf2[cpos_dc-(acomp-i)*64-ls[i]];
              if (color[i]==color[acomp]-1) { prev1+=x; cnt1++; r+=cbuf[cpos-(acomp-i)*64]>>4; s+=cbuf[cpos-(acomp-i)*64]&0xF; }
              if (color[acomp]>1 && color[i]==color[0]) { prev2+=x; cnt2++; }
            }
            if (cnt1>0) prev1/=cnt1, r/=cnt1, s/=cnt1, prev_coef_rs=(r<<4)|s;
            if (cnt2>0) prev2/=cnt2;
            prev_coef=(prev1<0?-1:+1)*ilog(11*abs(prev1)+1)+(cnt1<<20);
            prev_coef2=(prev2<0?-1:+1)*ilog(11*abs(prev2)+1);

            if (column==0 && blockW[acomp]>64*acomp) run_pred[1]=run_pred[2], run_pred[0]=0, adv_pred[1]=adv_pred[2], adv_pred[0]=0;
            if (row==0 && blockN[acomp]>64*acomp) run_pred[1]=run_pred[0], run_pred[2]=0, adv_pred[1]=adv_pred[0], adv_pred[2]=0;
          } // !!!!

        }
      }
    }
  }

  // Estimate next bit probability
  if (!images[idx].jpeg || !images[idx].data) return images[idx].next_jpeg;
  if (buf(1+(bpos==0))==FF) {
    m.add(128);
    m.set(0, 9);
    m.set(0, 1025);
    m.set(buf(1), 1024);
    return true;
  }
  if (rstlen>0 && rstlen==column+row*width-rstpos && mcupos==0 && (int)huffcode==(1<<huffbits)-1) {
    m.add(4095);
    m.set(0, 9);
    m.set(0, 1025);
    m.set(buf(1), 1024);
    return true;
  }

  // Context model
  const int N=32; // size of t, number of contexts
  static BH<9> t(MEM);  // context hash -> bit history
    // As a cache optimization, the context does not include the last 1-2
    // bits of huffcode if the length (huffbits) is not a multiple of 3.
    // The 7 mapped values are for context+{"", 0, 00, 01, 1, 10, 11}.
  static Array<U32> cxt(N);  // context hashes
  static Array<U8*> cp(N);  // context pointers
  static StateMap sm[N];
  static Mixer m1(N+1, 2050, 3);
  static APM a1(0x8000), a2(0x20000);

  // Update model
  if (cp[N-1]) {
    for (int i=0; i<N; ++i)
      *cp[i]=nex(*cp[i],y);
  }
  m1.update();

  // Update context
  const int comp=color[mcupos>>6];
  const int coef=(mcupos&63)|comp<<6;
  const int hc=(huffcode*4+((mcupos&63)==0)*2+(comp==0))|1<<(huffbits+2);
  const bool firstcol=column==0 && blockW[mcupos>>6]>mcupos;
  static int hbcount=2;
  if (++hbcount>2 || huffbits==0) hbcount=0;
  jassert(coef>=0 && coef<256);
  const int zu=zzu[mcupos&63], zv=zzv[mcupos&63];
  if (hbcount==0) {
    int n=hc*32;
    cxt[0]=hash(++n, coef, adv_pred[2]/12+(run_pred[2]<<8), ssum2>>6, prev_coef/72);
    cxt[1]=hash(++n, coef, adv_pred[0]/12+(run_pred[0]<<8), ssum2>>6, prev_coef/72);
    cxt[2]=hash(++n, coef, adv_pred[1]/11+(run_pred[1]<<8), ssum2>>6);
    cxt[3]=hash(++n, rs1, adv_pred[2]/7, run_pred[5]/2, prev_coef/10);
    cxt[4]=hash(++n, rs1, adv_pred[0]/7, run_pred[3]/2, prev_coef/10);
    cxt[5]=hash(++n, rs1, adv_pred[1]/11, run_pred[4]);
    cxt[6]=hash(++n, adv_pred[2]/14, run_pred[2], adv_pred[0]/14, run_pred[0]);
    cxt[7]=hash(++n, cbuf[cpos-blockN[mcupos>>6]]>>4, adv_pred[3]/17, run_pred[1], run_pred[5]);
    cxt[8]=hash(++n, cbuf[cpos-blockW[mcupos>>6]]>>4, adv_pred[3]/17, run_pred[1], run_pred[3]);
    cxt[9]=hash(++n, lcp[0]/22, lcp[1]/22, adv_pred[1]/7, run_pred[1]);
    cxt[10]=hash(++n, lcp[0]/22, lcp[1]/22, mcupos&63, lcp[4]/30);
    cxt[11]=hash(++n, zu/2, lcp[0]/13, lcp[2]/30, prev_coef/40+((prev_coef2/28)<<20));
    cxt[12]=hash(++n, zv/2, lcp[1]/13, lcp[3]/30, prev_coef/40+((prev_coef2/28)<<20));
    cxt[13]=hash(++n, rs1, prev_coef/42, prev_coef2/34, hash(lcp[0]/60,lcp[2]/14,lcp[1]/60,lcp[3]/14));
    cxt[14]=hash(++n, mcupos&63, column>>1);
    cxt[15]=hash(++n, column>>3, min(5+2*(!comp),zu+zv), hash(lcp[0]/10,lcp[2]/40,lcp[1]/10,lcp[3]/40));
    cxt[16]=hash(++n, ssum>>3, mcupos&63);
    cxt[17]=hash(++n, rs1, mcupos&63, run_pred[1]);
    cxt[18]=hash(++n, coef, ssum2>>5, adv_pred[3]/30, (comp)?hash(prev_coef/22,prev_coef2/50):ssum/((mcupos&0x3F)+1));
    cxt[19]=hash(++n, lcp[0]/40, lcp[1]/40, adv_pred[1]/28, hash( (comp)?prev_coef/40+((prev_coef2/40)<<20):lcp[4]/22, min(7,zu+zv), ssum/(2*(zu+zv)+1) ) );
    cxt[20]=hash(++n, zv, cbuf[cpos-blockN[mcupos>>6]], adv_pred[2]/28, run_pred[2]);
    cxt[21]=hash(++n, zu, cbuf[cpos-blockW[mcupos>>6]], adv_pred[0]/28, run_pred[0]);
    cxt[22]=hash(++n, adv_pred[2]/7, run_pred[2]);
    cxt[23]=hash(n, adv_pred[0]/7, run_pred[0]);
    cxt[24]=hash(n, adv_pred[1]/7, run_pred[1]);
    cxt[25]=hash(++n, zv, lcp[1]/14, adv_pred[2]/16, run_pred[5]);
    cxt[26]=hash(++n, zu, lcp[0]/14, adv_pred[0]/16, run_pred[3]);
    cxt[27]=hash(++n, lcp[0]/14, lcp[1]/14, adv_pred[3]/16);
    cxt[28]=hash(++n, coef, prev_coef/10, prev_coef2/20);
    cxt[29]=hash(++n, coef, ssum>>2, prev_coef_rs);
    cxt[30]=hash(++n, coef, adv_pred[1]/17, hash(lcp[(zu<zv)]/24,lcp[2]/20,lcp[3]/24));
    cxt[31]=hash(++n, coef, adv_pred[3]/11, hash(lcp[(zu<zv)]/50,lcp[2+3*(zu*zv>1)]/50,lcp[3+3*(zu*zv>1)]/50));
  }

  // Predict next bit
  m1.add(128); //network bias
  assert(hbcount<=2);
  int p;
 switch(hbcount)
  {
   case 0: for (int i=0; i<N; ++i){ cp[i]=t[cxt[i]]+1, m1.add(p=stretch(sm[i].p(*cp[i]))); m.add(p>>2);} break;
   case 1: { int hc=1+(huffcode&1)*3; for (int i=0; i<N; ++i){ cp[i]+=hc, m1.add(p=stretch(sm[i].p(*cp[i]))); m.add(p>>2); }} break;
   default: { int hc=1+(huffcode&1); for (int i=0; i<N; ++i){ cp[i]+=hc, m1.add(p=stretch(sm[i].p(*cp[i]))); m.add(p>>2); }} break;
  }

  m1.set(firstcol, 2);
  m1.set(coef+256*min(3,huffbits), 1024);
  m1.set((hc&0x1FE)*2+min(3,ilog2(zu+zv)), 1024);
  int pr=m1.p();
  m.add(stretch(pr)>>2);
  m.add((pr>>4)-(255-((pr>>4))));
  pr=a1.p(pr, (hc&511)|(((adv_pred[1]/16)&63)<<9), 1023);
  m.add(stretch(pr)>>2);
  m.add((pr>>4)-(255-((pr>>4))));
  pr=a2.p(pr, (hc&511)|(coef<<9), 1023);
  m.add(stretch(pr)>>2);
  m.add((pr>>4)-(255-((pr>>4))));
  m.set(1 + (zu+zv<5)+(huffbits>8)*2+firstcol*4, 9);
  m.set(1 + (hc&0xFF) + 256*min(3,(zu+zv)/3), 1025);
  m.set(coef+256*min(3,huffbits/2), 1024);
  return true;

}

//////////////////////////// wavModel /////////////////////////////////

// Model a 16/8-bit stereo/mono uncompressed .wav file.
// Based on 'An asymptotically Optimal Predictor for Stereo Lossless Audio Compression'
// by Florin Ghido.

static int S,D;
static int wmode;

inline int s2(int i) { return int(short(buf(i)+256*buf(i-1))); }
inline int t2(int i) { return int(short(buf(i-1)+256*buf(i))); }

inline int X1(int i) {
  switch (wmode) {
    case 0: return buf(i)-128;
    case 1: return buf(i<<1)-128;
    case 2: return s2(i<<1);
    case 3: return s2(i<<2);
    case 4: return (buf(i)^128)-128;
    case 5: return (buf(i<<1)^128)-128;
    case 6: return t2(i<<1);
    case 7: return t2(i<<2);
    default: return 0;
  }
}

inline int X2(int i) {
  switch (wmode) {
    case 0: return buf(i+S)-128;
    case 1: return buf((i<<1)-1)-128;
    case 2: return s2((i+S)<<1);
    case 3: return s2((i<<2)-2);
    case 4: return (buf(i+S)^128)-128;
    case 5: return (buf((i<<1)-1)^128)-128;
    case 6: return t2((i+S)<<1);
    case 7: return t2((i<<2)-2);
    default: return 0;
  }
}

#define pr_(i,chn)(pr[2*(i)+chn])
#define F_(k,l,chn)(F[2*(49*(k)+l)+chn])
#define L_(i,k)(L[49*(i)+k])

void wavModel(Mixer& m, int info, ModelStats *Stats = NULL) {
  static int col=0;
  static Array<int> pr{3*2}; // [3][2]
  static Array<int> n{2};
  static Array<int> counter{2};
  static Array<double> F{49*49*2}; //[49][49][2]
  static Array<double> L{49*49}; //[49][49]
  static const double a=0.996,a2=1.0/a;
  static const int SC=0x10000;
  static SmallStationaryContextMap scm1(SC), scm2(SC), scm3(SC), scm4(SC), scm5(SC), scm6(SC), scm7(SC);
  static ContextMap cm(MEM*4, 11);
  static int bits, channels, w, ch;
  static int z1, z2, z3, z4, z5, z6, z7;

  int j, k, l, i = 0;
  long double sum;

  if (bpos==0 && blpos==0) {
    bits=((info%4)/2)*8+8;
    channels=info%2+1;
    w=channels*(bits>>3);
    wmode=info;
    z1=z2=z3=z4=z5=z6=z7=0;
    if (channels==1) {S=48;D=0;} else {S=36;D=12;}
    for (k=0; k<=S+D; k++) 
      for (l=0; l<=S+D; l++) {
        L_(k,l)=0.0;
        for (int chn=0; chn<channels; chn++)
          F_(k,l,chn)=0.0;
      }
    for (int chn=0; chn<channels; chn++) {
      F_(1,0,chn)=1.0;
      n[chn]=counter[chn]=pr_(2,chn)=pr_(1,chn)=pr_(0,chn)=0;
    }
  }
  // Select previous samples and predicted sample as context
  if (bpos==0 && blpos>=w) {
    ch=blpos%w;
    const int msb=ch%(bits>>3);
    const int chn=ch/(bits>>3);
    if (msb==0) {
      z1=X1(1);z2=X1(2);z3=X1(3);z4=X1(4);z5=X1(5);
      k=X1(1);
      const int mS=min(S,counter[chn]-1);
      const int mD=min(D,counter[chn]);
      for (l=0; l<=mS; l++) { F_(0,l,chn)*=a; F_(0,l,chn)+=X1(l+1)*k; }
      for (l=1; l<=mD; l++) { F_(0,l+S,chn)*=a; F_(0,l+S,chn)+=X2(l+1)*k; }
      if (channels==2) {
        k=X2(2);
        for (l=1; l<=mD; l++) { F_(S+1,l+S,chn)*=a; F_(S+1,l+S,chn)+=X2(l+1)*k; }
        for (l=1; l<=mS; l++) { F_(l,S+1,chn)*=a; F_(l,S+1,chn)+=X1(l+1)*k; }
        z6=X2(1)+X1(1)-X2(2);
        z7=X2(1);
      } else {
        z6=2*X1(1)-X1(2); 
        z7=X1(1);
      }
      n[chn]++;
      if (n[chn]==(256>>level)) {
        if (channels==1) for (k=1; k<=S+D; k++) for (l=k; l<=S+D; l++) F_(k,l,chn)=(F_(k-1,l-1,chn)-X1(k)*X1(l))*a2;
        else for (k=1; k<=S+D; k++) if (k!=S+1) for (l=k; l<=S+D; l++) if (l!=S+1) F_(k,l,chn)=(F_(k-1,l-1,chn)-(k-1<=S?X1(k):X2(k-S))*(l-1<=S?X1(l):X2(l-S)))*a2;
        for (i=1; i<=S+D; i++) {
           sum=F_(i,i,chn);
           for (k=1; k<i; k++) sum-=L_(i,k)*L_(i,k);
           sum=floor(sum+0.5);
           sum=1.0/sum;
           if (sum>0.0) {
             L_(i,i)=sqrt(sum);
             for (j=(i+1); j<=S+D; j++) {
               sum=F_(i,j,chn);
               for (k=1; k<i; k++) sum-=L_(j,k)*L_(i,k);
               sum=floor(sum+0.5);
               L_(j,i)=sum*L_(i,i);
             }
           } else break;
        }
        if (i>S+D && counter[chn]>S+1) {
          for (k=1; k<=S+D; k++) {
            F_(k,0,chn)=F_(0,k,chn);
            for (j=1; j<k; j++) F_(k,0,chn)-=L_(k,j)*F_(j,0,chn);
            F_(k,0,chn)*=L_(k,k);
          }
          for (k=S+D; k>0; k--) {
            for (j=k+1; j<=S+D; j++) F_(k,0,chn)-=L_(j,k)*F_(j,0,chn);
            F_(k,0,chn)*=L_(k,k);
          }
        }
        n[chn]=0;
      }
      sum=0.0;
      for (l=1; l<=S+D; l++) sum+=F_(l,0,chn)*(l<=S?X1(l):X2(l-S));
      pr_(2,chn)=pr_(1,chn);
      pr_(1,chn)=pr_(0,chn);
      pr_(0,chn)=int(floor(sum));
      counter[chn]++;
    }
    const int y1=pr_(0,chn), y2=pr_(1,chn), y3=pr_(2,chn);
    int x1=buf(1), x2=buf(2), x3=buf(3);
    if (wmode==4 || wmode==5) {x1^=128; x2^=128;}
    if (bits==8) {x1-=128; x2-=128;}
    const int t=((bits==8) || ((!msb)^(wmode<6)));
    i=ch<<4;
    if ((msb)^(wmode<6)) {
      cm.set(hash(++i, y1&0xff));
      cm.set(hash(++i, y1&0xff, ((z1-y2+z2-y3)>>1)&0xff));
      cm.set(hash(++i, x1, y1&0xff));
      cm.set(hash(++i, x1, x2>>3, x3));
      if (bits==8)
        cm.set(hash(++i, y1&0xFE, ilog2(abs((int)(z1-y2)))*2+(z1>y2) ));
      else
        cm.set(hash(++i, (y1+z1-y2)&0xff));
      cm.set(hash(++i, x1));
      cm.set(hash(++i, x1, x2));
      cm.set(hash(++i, z1&0xff));
      cm.set(hash(++i, (z1*2-z2)&0xff));
      cm.set(hash(++i, z6&0xff));
      cm.set(hash(++i, y1&0xFF, ((z1-y2+z2-y3)/(bits>>3))&0xFF ));
    } else {
      cm.set(hash(++i, (y1-x1+z1-y2)>>8));
      cm.set(hash(++i, (y1-x1)>>8));
      cm.set(hash(++i, (y1-x1+z1*2-y2*2-z2+y3)>>8));
      cm.set(hash(++i, (y1-x1)>>8, (z1-y2+z2-y3)>>9));
      cm.set(hash(++i, z1>>12));
      cm.set(hash(++i, x1));
      cm.set(hash(++i, x1>>7, x2, x3>>7));
      cm.set(hash(++i, z1>>8));
      cm.set(hash(++i, (z1*2-z2)>>8));
      cm.set(hash(++i, y1>>8));
      cm.set(hash(++i, (y1-x1)>>6 ));
    }
    scm1.set(t*ch);
    scm2.set(t*((z1-x1+y1)>>9)&0xff);
    scm3.set(t*((z1*2-z2-x1+y1)>>8)&0xff);
    scm4.set(t*((z1*3-z2*3+z3-x1)>>7)&0xff);
    scm5.set(t*((z1+z7-x1+y1*2)>>10)&0xff);
    scm6.set(t*((z1*4-z2*6+z3*4-z4-x1)>>7)&0xff);
    scm7.set(t*((z1*5-z2*10+z3*10-z4*5+z5-x1+y1)>>9)&0xff);
  }

  // Predict next bit
  scm1.mix(m);
  scm2.mix(m);
  scm3.mix(m);
  scm4.mix(m);
  scm5.mix(m);
  scm6.mix(m);
  scm7.mix(m);
  cm.mix(m);
  if (level>=4){
    if (Stats)
      (*Stats).Record = (w<<16)|((*Stats).Record&0xFFFF);
    recordModel(m, Stats);
  }
  col++;
  if(col==w*8)col=0;
  m.set(ch+4*ilog2(col&(bits-1)), 4*8);
  m.set(col%bits<8, 2);
  m.set(col%bits, bits);
  m.set(col, w*8);
  m.set(c0, 256);
}

//////////////////////////// exeModel /////////////////////////
/*
  Model for x86/x64 code.
  Based on the previous paq* exe models and on DisFilter (http://www.farbrausch.de/~fg/code/disfilter/) by Fabian Giesen.

  Attemps to parse the input stream as x86/x64 instructions, and quantizes them into 32-bit representations that are then
  used as context to predict the next bits, and also extracts possible sparse contexts at several previous positions that
  are relevant to parsing (prefixes, opcode, Mod and R/M fields of the ModRM byte, Scale field of the SIB byte)

  Changelog:
  (18/08/2017) v98: Initial release by Márcio Pais
  (19/08/2017) v99: Bug fixes, tables for instruction categorization, other small improvements
  (29/08/2017) v102: Added variable context dependent on parsing break point
  (17/09/2017) v112: Allow pre-training of the model
  (22/10/2017) v116: Fixed a bug in function ProcessMode (missing break, thank you Mauro Vezzosi)
*/

// formats
enum InstructionFormat {
  // encoding mode
  fNM = 0x0,      // no ModRM
  fAM = 0x1,      // no ModRM, "address mode" (jumps or direct addresses)
  fMR = 0x2,      // ModRM present
  fMEXTRA = 0x3,  // ModRM present, includes extra bits for opcode
  fMODE = 0x3,    // bitmask for mode

  // no ModRM: size of immediate operand
  fNI = 0x0,      // no immediate
  fBI = 0x4,      // byte immediate
  fWI = 0x8,      // word immediate
  fDI = 0xc,      // dword immediate
  fTYPE = 0xc,    // type mask

  // address mode: type of address operand
  fAD = 0x0,      // absolute address
  fDA = 0x4,      // dword absolute jump target
  fBR = 0x8,      // byte relative jump target
  fDR = 0xc,      // dword relative jump target

  // others
  fERR = 0xf,     // denotes invalid opcodes
};

// 1 byte opcodes
const static U8 Table1[256] = {
  // 0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI, // 0
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI, // 1
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI, // 2
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI, // 3

  fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // 4
  fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // 5
  fNM|fNI,fNM|fNI,fMR|fNI,fMR|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fDI,fMR|fDI,fNM|fBI,fMR|fBI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // 6
  fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR, // 7

  fMR|fBI,fMR|fDI,fMR|fBI,fMR|fBI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // 8
  fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fAM|fDA,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // 9
  fAM|fAD,fAM|fAD,fAM|fAD,fAM|fAD,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fBI,fNM|fDI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // a
  fNM|fBI,fNM|fBI,fNM|fBI,fNM|fBI,fNM|fBI,fNM|fBI,fNM|fBI,fNM|fBI,fNM|fDI,fNM|fDI,fNM|fDI,fNM|fDI,fNM|fDI,fNM|fDI,fNM|fDI,fNM|fDI, // b

  fMR|fBI,fMR|fBI,fNM|fWI,fNM|fNI,fMR|fNI,fMR|fNI,fMR|fBI,fMR|fDI,fNM|fBI,fNM|fNI,fNM|fWI,fNM|fNI,fNM|fNI,fNM|fBI,fERR   ,fNM|fNI, // c
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fBI,fNM|fBI,fNM|fNI,fNM|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // d
  fAM|fBR,fAM|fBR,fAM|fBR,fAM|fBR,fNM|fBI,fNM|fBI,fNM|fBI,fNM|fBI,fAM|fDR,fAM|fDR,fAM|fAD,fAM|fBR,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // e
  fNM|fNI,fERR   ,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fMEXTRA,fMEXTRA,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fMEXTRA,fMEXTRA, // f
};

// 2 byte opcodes
const static U8 Table2[256] = {
  // 0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fNM|fNI,fERR   ,fNM|fNI,fNM|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 0
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 1
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // 2
  fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fERR   ,fNM|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 3

  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // 4
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // 5
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // 6
  fMR|fBI,fMR|fBI,fMR|fBI,fMR|fBI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fMR|fNI,fMR|fNI, // 7

  fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR,fAM|fDR, // 8
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // 9
  fNM|fNI,fNM|fNI,fNM|fNI,fMR|fNI,fMR|fBI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fMR|fNI,fMR|fBI,fMR|fNI,fERR   ,fMR|fNI, // a
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // b

  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI,fNM|fNI, // c
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // d
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // e
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   , // f
};

// 3 byte opcodes 0F38XX
const static U8 Table3_38[256] = {
  // 0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fERR   , // 0
  fMR|fNI,fERR   ,fERR   ,fERR   ,fMR|fNI,fMR|fNI,fERR   ,fMR|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fMR|fNI,fMR|fNI,fMR|fNI,fERR   , // 1
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fERR   ,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fERR   , // 2
  fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // 3
  fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 4
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 5
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 6
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 7
  fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 8
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 9
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // a
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // b
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // c
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // d
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // e
  fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // f
};

// 3 byte opcodes 0F3AXX
const static U8 Table3_3A[256] = {
  // 0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fMR|fBI,fMR|fBI,fMR|fBI,fMR|fBI,fMR|fBI,fMR|fBI,fMR|fBI,fMR|fBI, // 0
  fERR   ,fERR   ,fERR   ,fERR   ,fMR|fBI,fMR|fBI,fMR|fBI,fMR|fBI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 1
  fMR|fBI,fMR|fBI,fMR|fBI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 2
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 3
  fMR|fBI,fMR|fBI,fMR|fBI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 4
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 5
  fMR|fBI,fMR|fBI,fMR|fBI,fMR|fBI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 6
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 7
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 8
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // 9
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // a
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // b
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // c
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // d
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // e
  fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // f
};

// escape opcodes using ModRM byte to get more variants
const static U8 TableX[32] = {
  // 0       1       2       3       4       5       6       7
  fMR|fBI,fERR   ,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // escapes for 0xf6
  fMR|fDI,fERR   ,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI,fMR|fNI, // escapes for 0xf7
  fMR|fNI,fMR|fNI,fERR   ,fERR   ,fERR   ,fERR   ,fERR   ,fERR   , // escapes for 0xfe
  fMR|fNI,fMR|fNI,fMR|fNI,fERR   ,fMR|fNI,fERR   ,fMR|fNI,fERR   , // escapes for 0xff
};

const static U8 InvalidX64Ops[19] = {0x06, 0x07, 0x16, 0x17, 0x1E, 0x1F, 0x27, 0x2F, 0x37, 0x3F, 0x60, 0x61, 0x62, 0x82, 0x9A, 0xD4, 0xD5, 0xD6, 0xEA,};
const static U8 X64Prefixes[8] = {0x26, 0x2E, 0x36, 0x3E, 0x9B, 0xF0, 0xF2, 0xF3,};

enum InstructionCategory {
  OP_INVALID              =  0,
  OP_PREFIX_SEGREG        =  1,
  OP_PREFIX               =  2,
  OP_PREFIX_X87FPU        =  3,
  OP_GEN_DATAMOV          =  4,
  OP_GEN_STACK            =  5,
  OP_GEN_CONVERSION       =  6,
  OP_GEN_ARITH_DECIMAL    =  7,
  OP_GEN_ARITH_BINARY     =  8,
  OP_GEN_LOGICAL          =  9,
  OP_GEN_SHF_ROT          = 10,
  OP_GEN_BIT              = 11,
  OP_GEN_BRANCH           = 12,
  OP_GEN_BRANCH_COND      = 13,
  OP_GEN_BREAK            = 14,
  OP_GEN_STRING           = 15,
  OP_GEN_INOUT            = 16,
  OP_GEN_FLAG_CONTROL     = 17,
  OP_GEN_SEGREG           = 18,
  OP_GEN_CONTROL          = 19,
  OP_SYSTEM               = 20,
  OP_X87_DATAMOV          = 21,
  OP_X87_ARITH            = 22,
  OP_X87_COMPARISON       = 23,
  OP_X87_TRANSCENDENTAL   = 24,
  OP_X87_LOAD_CONSTANT    = 25,
  OP_X87_CONTROL          = 26,
  OP_X87_CONVERSION       = 27,
  OP_STATE_MANAGEMENT     = 28,
  OP_MMX                  = 29,
  OP_SSE                  = 30,
  OP_SSE_DATAMOV          = 31,
};

const static U8 TypeOp1[256] = {
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , //03
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_STACK         , OP_GEN_STACK         , //07
  OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , //0B
  OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_GEN_STACK         , OP_PREFIX            , //0F
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , //13
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_STACK         , OP_GEN_STACK         , //17
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , //1B
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_STACK         , OP_GEN_STACK         , //1F
  OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , //23
  OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_PREFIX_SEGREG     , OP_GEN_ARITH_DECIMAL , //27
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , //2B
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_PREFIX_SEGREG     , OP_GEN_ARITH_DECIMAL , //2F
  OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , //33
  OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_PREFIX_SEGREG     , OP_GEN_ARITH_DECIMAL , //37
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , //3B
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_PREFIX_SEGREG     , OP_GEN_ARITH_DECIMAL , //3F
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , //43
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , //47
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , //4B
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , //4F
  OP_GEN_STACK         , OP_GEN_STACK         , OP_GEN_STACK         , OP_GEN_STACK         , //53
  OP_GEN_STACK         , OP_GEN_STACK         , OP_GEN_STACK         , OP_GEN_STACK         , //57
  OP_GEN_STACK         , OP_GEN_STACK         , OP_GEN_STACK         , OP_GEN_STACK         , //5B
  OP_GEN_STACK         , OP_GEN_STACK         , OP_GEN_STACK         , OP_GEN_STACK         , //5F
  OP_GEN_STACK         , OP_GEN_STACK         , OP_GEN_BREAK         , OP_GEN_CONVERSION    , //63
  OP_PREFIX_SEGREG     , OP_PREFIX_SEGREG     , OP_PREFIX            , OP_PREFIX            , //67
  OP_GEN_STACK         , OP_GEN_ARITH_BINARY  , OP_GEN_STACK         , OP_GEN_ARITH_BINARY  , //6B
  OP_GEN_INOUT         , OP_GEN_INOUT         , OP_GEN_INOUT         , OP_GEN_INOUT         , //6F
  OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , //73
  OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , //77
  OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , //7B
  OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , //7F
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , //83
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //87
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //8B
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_STACK         , //8F
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //93
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //97
  OP_GEN_CONVERSION    , OP_GEN_CONVERSION    , OP_GEN_BRANCH        , OP_PREFIX_X87FPU     , //9B
  OP_GEN_STACK         , OP_GEN_STACK         , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //9F
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //A3
  OP_GEN_STRING        , OP_GEN_STRING        , OP_GEN_STRING        , OP_GEN_STRING        , //A7
  OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_GEN_STRING        , OP_GEN_STRING        , //AB
  OP_GEN_STRING        , OP_GEN_STRING        , OP_GEN_STRING        , OP_GEN_STRING        , //AF
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //B3
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //B7
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //BB
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //BF
  OP_GEN_SHF_ROT       , OP_GEN_SHF_ROT       , OP_GEN_BRANCH        , OP_GEN_BRANCH        , //C3
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //C7
  OP_GEN_STACK         , OP_GEN_STACK         , OP_GEN_BRANCH        , OP_GEN_BRANCH        , //CB
  OP_GEN_BREAK         , OP_GEN_BREAK         , OP_GEN_BREAK         , OP_GEN_BREAK         , //CF
  OP_GEN_SHF_ROT       , OP_GEN_SHF_ROT       , OP_GEN_SHF_ROT       , OP_GEN_SHF_ROT       , //D3
  OP_GEN_ARITH_DECIMAL , OP_GEN_ARITH_DECIMAL , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //D7
  OP_X87_ARITH         , OP_X87_DATAMOV       , OP_X87_ARITH         , OP_X87_DATAMOV       , //DB
  OP_X87_ARITH         , OP_X87_DATAMOV       , OP_X87_ARITH         , OP_X87_DATAMOV       , //DF
  OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , //E3
  OP_GEN_INOUT         , OP_GEN_INOUT         , OP_GEN_INOUT         , OP_GEN_INOUT         , //E7
  OP_GEN_BRANCH        , OP_GEN_BRANCH        , OP_GEN_BRANCH        , OP_GEN_BRANCH        , //EB
  OP_GEN_INOUT         , OP_GEN_INOUT         , OP_GEN_INOUT         , OP_GEN_INOUT         , //EF
  OP_PREFIX            , OP_GEN_BREAK         , OP_PREFIX            , OP_PREFIX            , //F3
  OP_SYSTEM            , OP_GEN_FLAG_CONTROL  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , //F7
  OP_GEN_FLAG_CONTROL  , OP_GEN_FLAG_CONTROL  , OP_GEN_FLAG_CONTROL  , OP_GEN_FLAG_CONTROL  , //FB
  OP_GEN_FLAG_CONTROL  , OP_GEN_FLAG_CONTROL  , OP_GEN_ARITH_BINARY  , OP_GEN_BRANCH        , //FF
};

const static U8 TypeOp2[256] = {
  OP_SYSTEM            , OP_SYSTEM            , OP_SYSTEM            , OP_SYSTEM            , //03
  OP_INVALID           , OP_SYSTEM            , OP_SYSTEM            , OP_SYSTEM            , //07
  OP_SYSTEM            , OP_SYSTEM            , OP_INVALID           , OP_GEN_CONTROL       , //0B
  OP_INVALID           , OP_GEN_CONTROL       , OP_INVALID           , OP_INVALID           , //0F
  OP_SSE_DATAMOV       , OP_SSE_DATAMOV       , OP_SSE_DATAMOV       , OP_SSE_DATAMOV       , //13
  OP_SSE               , OP_SSE               , OP_SSE_DATAMOV       , OP_SSE_DATAMOV       , //17
  OP_SSE               , OP_GEN_CONTROL       , OP_GEN_CONTROL       , OP_GEN_CONTROL       , //1B
  OP_GEN_CONTROL       , OP_GEN_CONTROL       , OP_GEN_CONTROL       , OP_GEN_CONTROL       , //1F
  OP_SYSTEM            , OP_SYSTEM            , OP_SYSTEM            , OP_SYSTEM            , //23
  OP_SYSTEM            , OP_INVALID           , OP_SYSTEM            , OP_INVALID           , //27
  OP_SSE_DATAMOV       , OP_SSE_DATAMOV       , OP_SSE               , OP_SSE               , //2B
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //2F
  OP_SYSTEM            , OP_SYSTEM            , OP_SYSTEM            , OP_SYSTEM            , //33
  OP_SYSTEM            , OP_SYSTEM            , OP_INVALID           , OP_INVALID           , //37
  OP_PREFIX            , OP_INVALID           , OP_PREFIX            , OP_INVALID           , //3B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //3F
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //43
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //47
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //4B
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //4F
  OP_SSE_DATAMOV       , OP_SSE               , OP_SSE               , OP_SSE               , //53
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //57
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //5B
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //5F
  OP_MMX               , OP_MMX               , OP_MMX               , OP_MMX               , //63
  OP_MMX               , OP_MMX               , OP_MMX               , OP_MMX               , //67
  OP_MMX               , OP_MMX               , OP_MMX               , OP_MMX               , //6B
  OP_INVALID           , OP_INVALID           , OP_MMX               , OP_MMX               , //6F
  OP_SSE               , OP_MMX               , OP_MMX               , OP_MMX               , //73
  OP_MMX               , OP_MMX               , OP_MMX               , OP_MMX               , //77
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //7B
  OP_INVALID           , OP_INVALID           , OP_MMX               , OP_MMX               , //7F
  OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , //83
  OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , //87
  OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , //8B
  OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , OP_GEN_BRANCH_COND   , //8F
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //93
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //97
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //9B
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //9F
  OP_GEN_STACK         , OP_GEN_STACK         , OP_GEN_CONTROL       , OP_GEN_BIT           , //A3
  OP_GEN_SHF_ROT       , OP_GEN_SHF_ROT       , OP_INVALID           , OP_INVALID           , //A7
  OP_GEN_STACK         , OP_GEN_STACK         , OP_SYSTEM            , OP_GEN_BIT           , //AB
  OP_GEN_SHF_ROT       , OP_GEN_SHF_ROT       , OP_STATE_MANAGEMENT  , OP_GEN_ARITH_BINARY  , //AF
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_BIT           , //B3
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_CONVERSION    , OP_GEN_CONVERSION    , //B7
  OP_INVALID           , OP_GEN_CONTROL       , OP_GEN_BIT           , OP_GEN_BIT           , //BB
  OP_GEN_BIT           , OP_GEN_BIT           , OP_GEN_CONVERSION    , OP_GEN_CONVERSION    , //BF
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_SSE               , OP_SSE               , //C3
  OP_SSE               , OP_SSE               , OP_SSE               , OP_GEN_DATAMOV       , //C7
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //CB
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , //CF
  OP_INVALID           , OP_MMX               , OP_MMX               , OP_MMX               , //D3
  OP_SSE               , OP_MMX               , OP_INVALID           , OP_SSE               , //D7
  OP_MMX               , OP_MMX               , OP_SSE               , OP_MMX               , //DB
  OP_MMX               , OP_MMX               , OP_SSE               , OP_MMX               , //DF
  OP_SSE               , OP_MMX               , OP_SSE               , OP_MMX               , //E3
  OP_SSE               , OP_MMX               , OP_INVALID           , OP_SSE               , //E7
  OP_MMX               , OP_MMX               , OP_SSE               , OP_MMX               , //EB
  OP_MMX               , OP_MMX               , OP_SSE               , OP_MMX               , //EF
  OP_INVALID           , OP_MMX               , OP_MMX               , OP_MMX               , //F3
  OP_SSE               , OP_MMX               , OP_SSE               , OP_SSE               , //F7
  OP_MMX               , OP_MMX               , OP_MMX               , OP_SSE               , //FB
  OP_MMX               , OP_MMX               , OP_MMX               , OP_INVALID           , //FF
};

const static U8 TypeOp3_38[256] = {
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //03
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //07
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //0B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //0F
  OP_SSE               , OP_INVALID           , OP_INVALID           , OP_INVALID           , //13
  OP_SSE               , OP_SSE               , OP_INVALID           , OP_SSE               , //17
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //1B
  OP_SSE               , OP_SSE               , OP_SSE               , OP_INVALID           , //1F
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //23
  OP_SSE               , OP_SSE               , OP_INVALID           , OP_INVALID           , //27
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //2B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //2F
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //33
  OP_SSE               , OP_SSE               , OP_INVALID           , OP_SSE               , //37
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //3B
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //3F
  OP_SSE               , OP_SSE               , OP_INVALID           , OP_INVALID           , //43
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //47
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //4B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //4F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //53
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //57
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //5B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //5F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //63
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //67
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //6B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //6F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //73
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //77
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //7B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //7F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //83
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //87
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //8B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //8F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //93
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //97
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //9B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //9F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //A3
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //A7
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //AB
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //AF
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //B3
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //B7
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //BB
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //BF
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //C3
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //C7
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //CB
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //CF
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //D3
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //D7
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //DB
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //DF
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //E3
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //E7
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //EB
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //EF
  OP_GEN_DATAMOV       , OP_GEN_DATAMOV       , OP_INVALID           , OP_INVALID           , //F3
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //F7
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //FB
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //FF
};

const static U8 TypeOp3_3A[256] = {
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //03
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //07
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //0B
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //0F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //13
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //17
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //1B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //1F
  OP_SSE               , OP_SSE               , OP_SSE               , OP_INVALID           , //23
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //27
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //2B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //2F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //33
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //37
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //3B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //3F
  OP_SSE               , OP_SSE               , OP_SSE               , OP_INVALID           , //43
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //47
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //4B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //4F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //53
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //57
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //5B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //5F
  OP_SSE               , OP_SSE               , OP_SSE               , OP_SSE               , //63
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //67
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //6B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //6F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //73
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //77
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //7B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //7F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //83
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //87
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //8B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //8F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //93
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //97
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //9B
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //9F
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //A3
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //A7
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //AB
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //AF
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //B3
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //B7
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //BB
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //BF
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //C3
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //C7
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //CB
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //CF
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //D3
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //D7
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //DB
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //DF
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //E3
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //E7
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //EB
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //EF
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //F3
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //F7
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //FB
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           , //FF
};

const static U8 TypeOpX[32] = {
  // escapes for F6
  OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_GEN_ARITH_BINARY  ,
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  ,
  // escapes for F7
  OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_GEN_LOGICAL       , OP_GEN_ARITH_BINARY  ,
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  ,
  // escapes for FE
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_INVALID           , OP_INVALID           ,
  OP_INVALID           , OP_INVALID           , OP_INVALID           , OP_INVALID           ,
  // escapes for FF
  OP_GEN_ARITH_BINARY  , OP_GEN_ARITH_BINARY  , OP_GEN_BRANCH        , OP_GEN_BRANCH        ,
  OP_GEN_BRANCH        , OP_GEN_BRANCH        , OP_GEN_STACK         , OP_INVALID           ,
};

enum Prefixes {
  ES_OVERRIDE = 0x26,
  CS_OVERRIDE = 0x2E,
  SS_OVERRIDE = 0x36,
  DS_OVERRIDE = 0x3E,
  FS_OVERRIDE = 0x64,
  GS_OVERRIDE = 0x65,
  AD_OVERRIDE = 0x67,
  WAIT_FPU    = 0x9B,
  LOCK        = 0xF0,
  REP_N_STR   = 0xF2,
  REP_STR     = 0xF3,
};

enum Opcodes {
  // 1-byte opcodes of special interest (for one reason or another)
  OP_2BYTE  = 0x0f,     // start of 2-byte opcode
  OP_OSIZE  = 0x66,     // operand size prefix
  OP_CALLF  = 0x9a,
  OP_RETNI  = 0xc2,     // ret near+immediate
  OP_RETN   = 0xc3,
  OP_ENTER  = 0xc8,
  OP_INT3   = 0xcc,
  OP_INTO   = 0xce,
  OP_CALLN  = 0xe8,
  OP_JMPF   = 0xea,
  OP_ICEBP  = 0xf1,
};

enum ExeState {
  Start             =  0,
  Pref_Op_Size      =  1,
  Pref_MultiByte_Op =  2,
  ParseFlags        =  3,
  ExtraFlags        =  4,
  ReadModRM         =  5,
  Read_OP3_38       =  6,
  Read_OP3_3A       =  7,
  ReadSIB           =  8,
  Read8             =  9,
  Read16            = 10,
  Read32            = 11,
  Read8_ModRM       = 12,
  Read16_f          = 13,
  Read32_ModRM      = 14,
  Error             = 15,
};

struct OpCache {
  U32 Op[CacheSize];
  U32 Index;
};

struct Instruction {
  U32 Data;
  U8 Prefix, Code, ModRM, SIB, REX, Flags, BytesRead, Size, Category;
  bool MustCheckREX, Decoding, o16, imm8;
};

#define CodeShift            3
#define CodeMask             (0xFF<<CodeShift)
#define ClearCodeMask        ((-1)^CodeMask)
#define PrefixMask           ((1<<CodeShift)-1)
#define OperandSizeOverride  (0x01<<(8+CodeShift))
#define MultiByteOpcode      (0x02<<(8+CodeShift))
#define PrefixREX            (0x04<<(8+CodeShift))
#define Prefix38             (0x08<<(8+CodeShift))
#define Prefix3A             (0x10<<(8+CodeShift))
#define HasExtraFlags        (0x20<<(8+CodeShift))
#define HasModRM             (0x40<<(8+CodeShift))
#define ModRMShift           (7+8+CodeShift)
#define SIBScaleShift        (ModRMShift+8-6)
#define RegDWordDisplacement (0x01<<(8+SIBScaleShift))
#define AddressMode          (0x02<<(8+SIBScaleShift))
#define TypeShift            (2+8+SIBScaleShift)
#define CategoryShift        5
#define CategoryMask         ((1<<CategoryShift)-1)
#define ModRM_mod            0xC0
#define ModRM_reg            0x38
#define ModRM_rm             0x07
#define SIB_scale            0xC0
#define SIB_index            0x38
#define SIB_base             0x07
#define REX_w                0x08

#define MinRequired          8 // minimum required consecutive valid instructions to be considered as code

inline bool IsInvalidX64Op(U8 Op){
  for (int i=0; i<19; i++){
    if (Op == InvalidX64Ops[i])
      return true;
  }
  return false;
}

inline bool IsValidX64Prefix(U8 Prefix){
  for (int i=0; i<8; i++){
    if (Prefix == X64Prefixes[i])
      return true;
  }
  return ((Prefix>=0x40 && Prefix<=0x4F) || (Prefix>=0x64 && Prefix<=0x67));
}

void ProcessMode(Instruction &Op, ExeState &State){
  if ((Op.Flags&fMODE)==fAM){
    Op.Data|=AddressMode;
    Op.BytesRead = 0;
    switch (Op.Flags&fTYPE){
      case fDR : Op.Data|=(2<<TypeShift);
      case fDA : Op.Data|=(1<<TypeShift);
      case fAD : {
        State = Read32;
        break;
      }
      case fBR : {
        Op.Data|=(2<<TypeShift);
        State = Read8;
      }
    }
  }
  else{
    switch (Op.Flags&fTYPE){
      case fBI : State = Read8; break;
      case fWI : {
        State = Read16;
        Op.Data|=(1<<TypeShift);
        Op.BytesRead = 0;
        break;
      }
      case fDI : {
        // x64 Move with 8byte immediate? [REX.W is set, opcodes 0xB8+r]
        Op.imm8=((Op.REX & REX_w)>0 && (Op.Code&0xF8)==0xB8);
        if (!Op.o16 || Op.imm8){
          State = Read32;
          Op.Data|=(2<<TypeShift);
        }
        else{
          State = Read16;
          Op.Data|=(3<<TypeShift);
        }
        Op.BytesRead = 0;
        break;
      }
      default: State = Start; /*no immediate*/
    }
  }
}

void ProcessFlags2(Instruction &Op, ExeState &State){
  //if arriving from state ExtraFlags, we've already read the ModRM byte
  if ((Op.Flags&fMODE)==fMR && State!=ExtraFlags){
    State = ReadModRM;
    return;
  }
  ProcessMode(Op, State);
}

void ProcessFlags(Instruction &Op, ExeState &State){
  if (Op.Code==OP_CALLF || Op.Code==OP_JMPF || Op.Code==OP_ENTER){
    Op.BytesRead = 0;
    State = Read16_f;
    return; //must exit, ENTER has ModRM too
  }
  ProcessFlags2(Op, State);
}

void CheckFlags(Instruction &Op, ExeState &State){
  //must peek at ModRM byte to read the REG part, so we can know the opcode
  if (Op.Flags==fMEXTRA)
    State = ExtraFlags;
  else if (Op.Flags==fERR){
    memset(&Op, 0, sizeof(Instruction));
    State = Error;
  }
  else
    ProcessFlags(Op, State);
}

void ReadFlags(Instruction &Op, ExeState &State){
  Op.Flags = Table1[Op.Code];
  Op.Category = TypeOp1[Op.Code];
  CheckFlags(Op, State);
}

void ProcessModRM(Instruction &Op, ExeState &State){
  if ((Op.ModRM & ModRM_mod)==0x40)
    State = Read8_ModRM; //register+byte displacement
  else if ((Op.ModRM & ModRM_mod)==0x80 || (Op.ModRM & (ModRM_mod|ModRM_rm))==0x05 || (Op.ModRM<0x40 && (Op.SIB & SIB_base)==0x05) ){
    State = Read32_ModRM; //register+dword displacement
    Op.BytesRead = 0;
  }
  else
    ProcessMode(Op, State);
}

void ApplyCodeAndSetFlag(Instruction &Op, U32 Flag = 0){
  Op.Data&=ClearCodeMask; \
  Op.Data|=(Op.Code<<CodeShift)|Flag;
}

inline U32 OpN(OpCache &Cache, U32 n){
  return Cache.Op[ (Cache.Index-n)&(CacheSize-1) ];
}

inline U32 OpNCateg(U32 &Mask, U32 n){
  return ((Mask>>(CategoryShift*(n-1)))&CategoryMask);
}

inline int pref(int i) { return (buf(i)==0x0f)+2*(buf(i)==0x66)+3*(buf(i)==0x67); }

// Get context at buf(i) relevant to parsing 32-bit x86 code
U32 execxt(int i, int x=0) {
  int prefix=0, opcode=0, modrm=0, sib=0;
  if (i) prefix+=4*pref(i--);
  if (i) prefix+=pref(i--);
  if (i) opcode+=buf(i--);
  if (i) modrm+=buf(i--)&(ModRM_mod|ModRM_rm);
  if (i&&((modrm&ModRM_rm)==4)&&(modrm<ModRM_mod)) sib=buf(i)&SIB_scale;
  return prefix|opcode<<4|modrm<<12|x<<20|sib<<(28-6);
}

class exeModel{
  static const int N1=8, N2=10;
  ContextMap cm;
  OpCache Cache;
  U32 StateBH[256];
  ExeState pState, State;
  Instruction Op;
  U32 TotalOps, OpMask, OpCategMask, Context, BrkPoint, BrkCtx;
  bool Valid;
  void Update(U8 B, bool Forced = false);
  void Train();
public:
  exeModel() : cm(MEM*2, N1+N2), pState (Start), State( Start), TotalOps(0), OpMask(0), OpCategMask(0), Context(0), BrkPoint(0), BrkCtx(0), Valid(false){
    memset(&Cache, 0, sizeof(OpCache));
    memset(&Op, 0, sizeof(Instruction));
    memset(&StateBH, 0, sizeof(StateBH));
    if (trainEXE) Train();
  }
  bool Predict(Mixer& m, bool Forced = false, ModelStats *Stats = NULL);
};

void exeModel::Train(){
  FILE *f;
  int i;
  char filename[MAX_PATH+1];
  if ((i=GetModuleFileName(NULL, filename,MAX_PATH)) && i<=MAX_PATH){
    if ((f = fopen(filename,"rb"))!=NULL){
      printf("Pre-training x86/x64 model...");
      i=0;
      do{
        Update(buf(1));
        if (Valid)
          cm.Train(i);
        buf[pos++]=i;
        blpos++;
      } while ((i=getc(f))!=EOF);
      printf(" done [%d bytes]\n",pos-1);
      fclose(f);
      cm.FinishTraining();
      pos=blpos=0;
      memset(&buf[0], 0, buf.size());
    }
  }
}

void exeModel::Update(U8 B, bool Forced){
  pState = State;
  Op.Size++;
  switch (State){
    case Start: case Error: {
      // previous code may have just been a REX prefix
      bool Skip = false;
      if (Op.MustCheckREX){
        Op.MustCheckREX = false;
        // valid x64 code?
        if (!IsInvalidX64Op(B) && !IsValidX64Prefix(B)){
          Op.REX = Op.Code;
          Op.Code = B;
          Op.Data = PrefixREX|(Op.Code<<CodeShift)|(Op.Data&PrefixMask);
          Skip = true;
        }
      }

      Op.ModRM = Op.SIB = Op.REX = Op.Flags = Op.BytesRead = 0;
      if (!Skip){
        Op.Code = B;
        // possible REX prefix?
        Op.MustCheckREX = ((Op.Code&0xF0)==0x40) && (!(Op.Decoding && ((Op.Data&PrefixMask)==1)));

        // check prefixes
        Op.Prefix = (Op.Code==ES_OVERRIDE || Op.Code==CS_OVERRIDE || Op.Code==SS_OVERRIDE || Op.Code==DS_OVERRIDE) + //invalid in x64
                    (Op.Code==FS_OVERRIDE)*2 +
                    (Op.Code==GS_OVERRIDE)*3 +
                    (Op.Code==AD_OVERRIDE)*4 +
                    (Op.Code==WAIT_FPU)*5 +
                    (Op.Code==LOCK)*6 +
                    (Op.Code==REP_N_STR || Op.Code==REP_STR)*7;

        if (!Op.Decoding){
          TotalOps+=(Op.Data!=0)-(Cache.Index && Cache.Op[ Cache.Index&(CacheSize-1) ]!=0);
          OpMask = (OpMask<<1)|(State!=Error);
          OpCategMask = (OpCategMask<<CategoryShift)|(Op.Category);
          Op.Size = 0;

          Cache.Op[ Cache.Index&(CacheSize-1) ] = Op.Data;
          Cache.Index++;

          if (!Op.Prefix)
            Op.Data = Op.Code<<CodeShift;
          else{
            Op.Data = Op.Prefix;
            Op.Category = TypeOp1[Op.Code];
            Op.Decoding = true;
            BrkCtx = hash(1+(BrkPoint = 0), Op.Prefix, OpCategMask&CategoryMask);
            break;
          }
        }
        else{
          // we only have enough bits for one prefix, so the
          // instruction will be encoded with the last one
          if (!Op.Prefix){
            Op.Data|=(Op.Code<<CodeShift);
            Op.Decoding = false;
          }
          else{
            Op.Data = Op.Prefix;
            Op.Category = TypeOp1[Op.Code];
            BrkCtx = hash(1+(BrkPoint = 1), Op.Prefix, OpCategMask&CategoryMask);
            break;
          }
        }
      }

      if ((Op.o16=(Op.Code==OP_OSIZE)))
        State = Pref_Op_Size;
      else if (Op.Code==OP_2BYTE)
        State = Pref_MultiByte_Op;
      else
        ReadFlags(Op, State);
      BrkCtx = hash(1+(BrkPoint = 2), State, Op.Code, (OpCategMask&CategoryMask), OpN(Cache,1)&((ModRM_mod|ModRM_reg|ModRM_rm)<<ModRMShift));
      break;
    }
    case Pref_Op_Size : {
      Op.Code = B;
      ApplyCodeAndSetFlag(Op, OperandSizeOverride);
      ReadFlags(Op, State);
      BrkCtx = hash(1+(BrkPoint = 3), State);
      break;
    }
    case Pref_MultiByte_Op : {
      Op.Code = B;
      Op.Data|=MultiByteOpcode;

      if (Op.Code==0x38)
        State = Read_OP3_38;
      else if (Op.Code==0x3A)
        State = Read_OP3_3A;
      else{
        ApplyCodeAndSetFlag(Op);
        Op.Flags = Table2[Op.Code];
        Op.Category = TypeOp2[Op.Code];
        CheckFlags(Op, State);
      }
      BrkCtx = hash(1+(BrkPoint = 4), State);
      break;
    }
    case ParseFlags : {
      ProcessFlags(Op, State);
      BrkCtx = hash(1+(BrkPoint = 5), State);
      break;
    }
    case ExtraFlags : case ReadModRM : {
      Op.ModRM = B;
      Op.Data|=(Op.ModRM<<ModRMShift)|HasModRM;
      Op.SIB = 0;
      if (Op.Flags==fMEXTRA){
        Op.Data|=HasExtraFlags;
        int i = ((Op.ModRM>>3)&0x07) | ((Op.Code&0x01)<<3) | ((Op.Code&0x08)<<1);
        Op.Flags = TableX[i];
        Op.Category = TypeOpX[i];
        if (Op.Flags==fERR){
          memset(&Op, 0, sizeof(Instruction));
          State = Error;
          BrkCtx = hash(1+(BrkPoint = 6), State);
          break;
        }
        ProcessFlags(Op, State);
        BrkCtx = hash(1+(BrkPoint = 7), State);
        break;
      }

      if ((Op.ModRM & ModRM_rm)==4 && Op.ModRM<ModRM_mod){
        State = ReadSIB;
        BrkCtx = hash(1+(BrkPoint = 8), State);
        break;
      }

      ProcessModRM(Op, State);
      BrkCtx = hash(1+(BrkPoint = 9), State, Op.Code );
      break;
    }
    case Read_OP3_38 : case Read_OP3_3A : {
      Op.Code = B;
      ApplyCodeAndSetFlag(Op, Prefix38<<(State-Read_OP3_38));
      if (State==Read_OP3_38){
        Op.Flags = Table3_38[Op.Code];
        Op.Category = TypeOp3_38[Op.Code];
      }
      else{
        Op.Flags = Table3_3A[Op.Code];
        Op.Category = TypeOp3_3A[Op.Code];
      }
      CheckFlags(Op, State);
      BrkCtx = hash(1+(BrkPoint = 10), State);
      break;
    }
    case ReadSIB : {
      Op.SIB = B;
      Op.Data|=((Op.SIB & SIB_scale)<<SIBScaleShift);
      ProcessModRM(Op, State);
      BrkCtx = hash(1+(BrkPoint = 11), State, Op.SIB&SIB_scale);
      break;
    }
    case Read8 : case Read16 : case Read32 : {
      if (++Op.BytesRead>=((State-Read8)<<int(Op.imm8+1))){
        Op.BytesRead = 0;
        Op.imm8 = false;
        State = Start;
      }
      BrkCtx = hash(1+(BrkPoint = 12), State, Op.Flags&fMODE, Op.BytesRead, ((Op.BytesRead>1)?(buf(Op.BytesRead)<<8):0)|((Op.BytesRead)?B:0) );
      break;
    }
    case Read8_ModRM : {
      ProcessMode(Op, State);
      BrkCtx = hash(1+(BrkPoint = 13), State);
      break;
    }
    case Read16_f : {
      if (++Op.BytesRead==2){
        Op.BytesRead = 0;
        ProcessFlags2(Op, State);
      }
      BrkCtx = hash(1+(BrkPoint = 14), State);
      break;
    }
    case Read32_ModRM : {
      Op.Data|=RegDWordDisplacement;
      if (++Op.BytesRead==4){
        Op.BytesRead = 0;
        ProcessMode(Op, State);
      }
      BrkCtx = hash(1+(BrkPoint = 15), State);
      break;
    }
  }
  Valid = (TotalOps>2*MinRequired) && ((OpMask&((1<<MinRequired)-1))==((1<<MinRequired)-1));
  Context = State+16*Op.BytesRead+16*(Op.REX & REX_w);
  StateBH[Context] = (StateBH[Context]<<8)|B;

  if (Valid || Forced){
    int mask=0, count0=0;
    for (int i=0, j=0; i<N1; ++i){
      if (i>1) mask=mask*2+(buf(i-1)==0), count0+=mask&1;
      j=(i<4)?i+1:5+(i-4)*(2+(i>6));
      cm.set(hash(execxt(j, buf(1)*(j>6)), ((1<<N1)|mask)*(count0*N1/2>=i), (0x08|(blpos&0x07))*(i<4)));
    }

    cm.set(BrkCtx);
    mask = PrefixMask|(0xF8<<CodeShift)|MultiByteOpcode|Prefix38|Prefix3A;
    cm.set(hash(OpN(Cache, 1)&(mask|RegDWordDisplacement|AddressMode), State+16*Op.BytesRead, Op.Data&mask, Op.REX, Op.Category));

    mask = 0x04|(0xFE<<CodeShift)|MultiByteOpcode|Prefix38|Prefix3A|((ModRM_mod|ModRM_reg)<<ModRMShift);
    cm.set(hash(
      OpN(Cache, 1)&mask, OpN(Cache, 2)&mask, OpN(Cache, 3)&mask,
      Context+256*((Op.ModRM & ModRM_mod)==ModRM_mod),
      Op.Data&((mask|PrefixREX)^(ModRM_mod<<ModRMShift))
    ));

    mask = 0x04|CodeMask;
    cm.set(hash(OpN(Cache, 1)&mask, OpN(Cache, 2)&mask, OpN(Cache, 3)&mask, OpN(Cache, 4)&mask, (Op.Data&mask)|(State<<11)|(Op.BytesRead<<15)));

    mask = 0x04|(0xFC<<CodeShift)|MultiByteOpcode|Prefix38|Prefix3A;
    cm.set(hash(State+16*Op.BytesRead, Op.Data&mask, Op.Category*8 + (OpMask&0x07), Op.Flags, ((Op.SIB & SIB_base)==5)*4+((Op.ModRM & ModRM_reg)==ModRM_reg)*2+((Op.ModRM & ModRM_mod)==0)));

    mask = PrefixMask|CodeMask|OperandSizeOverride|MultiByteOpcode|PrefixREX|Prefix38|Prefix3A|HasExtraFlags|HasModRM|((ModRM_mod|ModRM_rm)<<ModRMShift);
    cm.set(hash(Op.Data&mask, State+16*Op.BytesRead, Op.Flags));

    mask = PrefixMask|CodeMask|OperandSizeOverride|MultiByteOpcode|Prefix38|Prefix3A|HasExtraFlags|HasModRM;
    cm.set(hash(OpN(Cache, 1)&mask, State, Op.BytesRead*2+((Op.REX&REX_w)>0), Op.Data&((U16)(mask^OperandSizeOverride))));

    mask = 0x04|(0xFE<<CodeShift)|MultiByteOpcode|Prefix38|Prefix3A|(ModRM_reg<<ModRMShift);
    cm.set(hash(OpN(Cache, 1)&mask, OpN(Cache, 2)&mask, State+16*Op.BytesRead, Op.Data&(mask|PrefixMask|CodeMask)));

    cm.set(State+16*Op.BytesRead);

    cm.set(hash(
      (0x100|B)*(Op.BytesRead>0),
      State+16*pState+256*Op.BytesRead,
      ((Op.Flags&fMODE)==fAM)*16 + (Op.REX & REX_w) + (Op.o16)*4 + ((Op.Code & 0xFE)==0xE8)*2 + ((Op.Data & MultiByteOpcode)!=0 && (Op.Code & 0xF0)==0x80)
    ));
  }
}

bool exeModel::Predict(Mixer& m, bool Forced, ModelStats *Stats){
  if (bpos==0)
    Update(buf(1), Forced);

  if (Valid || Forced)
    cm.mix(m);
  else{
      for (int i=0; i<N1+N2; ++i)
        m.add(0);
  }
  U8 s = ((StateBH[Context]>>(28-bpos))&0x08) |
         ((StateBH[Context]>>(21-bpos))&0x04) |
         ((StateBH[Context]>>(14-bpos))&0x02) |
         ((StateBH[Context]>>( 7-bpos))&0x01) |
         ((Op.Category==OP_GEN_BRANCH)<<4)|
         (((c0&((1<<bpos)-1))==0)<<5);

  m.set(Context*4+(s>>4), 1024);
  m.set(State*64+bpos*8+(Op.BytesRead>0)*4+(s>>4), 1024);
  m.set( (BrkCtx&0x1FF)|((s&0x20)<<4), 1024 );

  if (Stats)
    (*Stats).x86_64 = (Valid?1:0)|(Context<<1)|(s<<9);
  return Valid;
}

//////////////////////////// indirectModel /////////////////////

// The context is a byte string history that occurs within a
// 1 or 2 byte context.

void indirectModel(Mixer& m) {
  static ContextMap cm(MEM, 12);
  static U32 t1[256];
  static U16 t2[0x10000];
  static U16 t3[0x8000];
  static U16 t4[0x8000];

  if (bpos==0) {
    U32 d=c4&0xffff, c=d&255, d2=(buf(1)&31)+32*(buf(2)&31)+1024*(buf(3)&31);
    U32 d3=(buf(1)>>3&31)+32*(buf(3)>>3&31)+1024*(buf(4)>>3&31);
    U32& r1=t1[d>>8];
    r1=r1<<8|c;
    U16& r2=t2[c4>>8&0xffff];
    r2=r2<<8|c;
    U16& r3=t3[(buf(2)&31)+32*(buf(3)&31)+1024*(buf(4)&31)];
    r3=r3<<8|c;
    U16& r4=t4[(buf(2)>>3&31)+32*(buf(4)>>3&31)+1024*(buf(5)>>3&31)];
    r4=r4<<8|c;
    const U32 t=c|t1[c]<<8;
    const U32 t0=d|t2[d]<<16;
    const U32 ta=d2|t3[d2]<<16;
    const U32 tc=d3|t4[d3]<<16;
    cm.set(t);
    cm.set(t0);
    cm.set(ta);
    cm.set(tc);
    cm.set(t&0xff00);
    cm.set(t0&0xff0000);
    cm.set(ta&0xff0000);
    cm.set(tc&0xff0000);
    cm.set(t&0xffff);
    cm.set(t0&0xffffff);
    cm.set(ta&0xffffff);
    cm.set(tc&0xffffff);
  }
  cm.mix(m);
}

//////////////////////////// dmcModel //////////////////////////

// Model using DMC.  The bitwise context is represented by a state graph,
// initilaized to a bytewise order 1 model as in
// http://plg.uwaterloo.ca/~ftp/dmc/dmc.c but with the following difference:
// - It uses integer arithmetic.
// - The threshold for cloning a state increases as memory is used up.
// - Each state maintains both a 0,1 count and a bit history (as in a
//   context model).  The 0,1 count is best for stationary data, and the
//   bit history for nonstationary data.  The bit history is mapped to
//   a probability adaptively using a StateMap.  The two computed probabilities
//   are combined.
// - When memory is used up the state graph is reinitialized to a bytewise
//   order 1 context as in the original DMC.  However, the bit histories
//   are not cleared.

struct DMCNode {  // 12 bytes
  U32 nx[2];  // next pointers
  U8 state;  // bit history
  U32 c0:12, c1:12;  // counts * 256
};

void dmcModel(Mixer& m) {
  static U32 top=0, curr=0;  // allocated, current node
  static Array<DMCNode> t(MEM*2);  // state graph
  static StateMap sm;
  static int threshold=256;

  // clone next state
  if (top>0 && top<t.size()) {
    int next=t[curr].nx[y];
    int n=y?t[curr].c1:t[curr].c0;
    int nn=t[next].c0+t[next].c1;
    if (n>=threshold*2 && nn-n>=threshold*3) {
      int r=n*4096/nn;
      assert(r>=0 && r<=4096);
      t[next].c0 -= t[top].c0 = t[next].c0*r>>12;
      t[next].c1 -= t[top].c1 = t[next].c1*r>>12;
      t[top].nx[0]=t[next].nx[0];
      t[top].nx[1]=t[next].nx[1];
      t[top].state=t[next].state;
      t[curr].nx[y]=top;
      ++top;
      if (top==(MEM/2) || top==MEM)
        threshold+=256;
    }
  }

  // Initialize to a bytewise order 1 model at startup or when flushing memory
  if (top==t.size() && bpos==1) top=0;
  if (top==0) {
    assert(t.size()>=65536);
    for (int i=0; i<256; ++i) {
      for (int j=0; j<256; ++j) {
        if (i<127) {
          t[j*256+i].nx[0]=j*256+i*2+1;
          t[j*256+i].nx[1]=j*256+i*2+2;
        }
        else {
          t[j*256+i].nx[0]=(i-127)*256;
          t[j*256+i].nx[1]=(i+1)*256;
        }
        t[j*256+i].c0=128;
        t[j*256+i].c1=128;
      }
    }
    top=65536;
    curr=0;
    threshold=256;
  }

  // update count, state
  if (y) {
    if (t[curr].c1<3840) t[curr].c1+=256;
  }
  else if (t[curr].c0<3840) t[curr].c0+=256;
  t[curr].state=nex(t[curr].state, y);
  curr=t[curr].nx[y];

  // predict
  const int pr1=sm.p(t[curr].state);
  const int n1=t[curr].c1;
  const int n0=t[curr].c0;
  const int pr2=(n1+5)*4096/(n0+n1+10);
  m.add(stretch(pr1)/4);
  m.add(stretch(pr2)/4);
}

void nestModel(Mixer& m)
{
  static int ic=0, bc=0, pc=0,vc=0, qc=0, lvc=0, wc=0, ac=0, ec=0, uc=0, sense1=0, sense2=0, w=0;
  static ContextMap cm(MEM, 10+2);
  if (bpos==0) {
    int c=c4&255, matched=1, vv;
    w*=((vc&7)>0 && (vc&7)<3);
    if (c&0x80) w = w*11*32 + c;
    const int lc = (c >= 'A' && c <= 'Z'?c+'a'-'A':c);
    if (lc == 'a' || lc == 'e' || lc == 'i' || lc == 'o' || lc == 'u'){ vv = 1; w = w*997*8 + (lc/4-22); } else
    if (lc >= 'a' && lc <= 'z'){ vv = 2; w = w*271*32 + lc-97; } else
    if (lc == ' ' || lc == '.' || lc == ',' || lc == '\n') vv = 3; else
    if (lc >= '0' && lc <= '9') vv = 4; else
    if (lc == 'y') vv = 5; else
    if (lc == '\'') vv = 6; else vv=(c&32)?7:0;
    vc = (vc << 3) | vv;
    if (vv != lvc) {
      wc = (wc << 3) | vv;
      lvc = vv;
    }
    switch(c) {
      case ' ': qc = 0; break;
      case '(': ic += 31; break;
      case ')': ic -= 31; break;
      case '[': ic += 11; break;
      case ']': ic -= 11; break;
      case '<': ic += 23; qc += 34; break;
      case '>': ic -= 23; qc /= 5; break;
      case ':': pc = 20; break;
      case '{': ic += 17; break;
      case '}': ic -= 17; break;
      case '|': pc += 223; break;
      case '"': pc += 0x40; break;
      case '\'': pc += 0x42; if (c!=(U8)(c4>>8)) sense2^=1; else ac+=(2*sense2-1); break;
      case '\n': pc = qc = 0; break;
      case '.': pc = 0; break;
      case '!': pc = 0; break;
      case '?': pc = 0; break;
      case '#': pc += 0x08; break;
      case '%': pc += 0x76; break;
      case '$': pc += 0x45; break;
      case '*': pc += 0x35; break;
      case '-': pc += 0x3; break;
      case '@': pc += 0x72; break;
      case '&': qc += 0x12; break;
      case ';': qc /= 3; break;
      case '\\': pc += 0x29; break;
      case '/': pc += 0x11;
                if (buf.size() > 1 && buf(1) == '<') qc += 74;
                break;
      case '=': pc += 87; if (c!=(U8)(c4>>8)) sense1^=1; else ec+=(2*sense1-1); break;
      default: matched = 0;
    }
    if (c4==0x266C743B) uc=min(7,uc+1);
    else if (c4==0x2667743B) uc-=(uc>0);
    if (matched) bc = 0; else bc += 1;
    if (bc > 300) bc = ic = pc = qc = uc = 0;

    cm.set(hash( (vv>0 && vv<3)?0:(lc|0x100), ic&0x3FF, ec&0x7, ac&0x7, uc ));
    cm.set(hash(ic, w, ilog2(bc+1)));
    cm.set((3*vc+77*pc+373*ic+qc)&0xffff);
    cm.set((31*vc+27*pc+281*qc)&0xffff);
    cm.set((13*vc+271*ic+qc+bc)&0xffff);
    cm.set((17*pc+7*ic)&0xffff);
    cm.set((13*vc+ic)&0xffff);
    cm.set((vc/3+pc)&0xffff);
    cm.set((7*wc+qc)&0xffff);
    cm.set((vc&0xffff)|((c4&0xff)<<16));
    cm.set(((3*pc)&0xffff)|((c4&0xff)<<16));
    cm.set((ic&0xffff)|((c4&0xff)<<16));

  }
  cm.mix(m);
}

/*
  XML Model.
  Attempts to parse the tag structure and detect specific content types.

  Changelog:
  (17/08/2017) v96: Initial release by Márcio Pais
  (17/08/2017) v97: Bug fixes (thank you Mauro Vezzosi) and improvements.
*/

struct XMLAttribute {
  U32 Name, Value, Length;
};

struct XMLContent {
  U32 Data, Length, Type;
};

struct XMLTag {
  U32 Name, Length;
  int Level;
  bool EndTag, Empty;
  XMLContent Content;
  struct XMLAttributes {
    XMLAttribute Items[4];
    U32 Index;
  } Attributes;
};

struct XMLTagCache {
  XMLTag Tags[CacheSize];
  U32 Index;
};

enum ContentFlags {
  Text        = 0x001,
  Number      = 0x002,
  Date        = 0x004,
  Time        = 0x008,
  URL         = 0x010,
  Link        = 0x020,
  Coordinates = 0x040,
  Temperature = 0x080,
  ISBN        = 0x100,
};

enum XMLState {
  None               = 0,
  ReadTagName        = 1,
  ReadTag            = 2,
  ReadAttributeName  = 3,
  ReadAttributeValue = 4,
  ReadContent        = 5,
  ReadCDATA          = 6,
  ReadComment        = 7,
};

#define DetectContent(){ \
  if ((c4&0xF0F0F0F0)==0x30303030){ \
    int i = 0, j = 0; \
    while ((i<4) && ( (j=(c4>>(8*i))&0xFF)>=0x30 && j<=0x39 )) \
      i++; \
\
    if (i==4 && ( ((c8&0xFDF0F0FD)==0x2D30302D && buf(9)>=0x30 && buf(9)<=0x39) || ((c8&0xF0FDF0FD)==0x302D302D) )) \
      (*Content).Type |= Date; \
  } \
  else if (((c8&0xF0F0FDF0)==0x30302D30 || (c8&0xF0F0F0FD)==0x3030302D) && buf(9)>=0x30 && buf(9)<=0x39){ \
    int i = 2, j = 0; \
    while ((i<4) && ( (j=(c8>>(8*i))&0xFF)>=0x30 && j<=0x39 )) \
      i++; \
\
    if (i==4 && (c4&0xF0FDF0F0)==0x302D3030) \
      (*Content).Type |= Date; \
  } \
\
  if ((c4&0xF0FFF0F0)==0x303A3030 && buf(5)>=0x30 && buf(5)<=0x39 && ((buf(6)<0x30 || buf(6)>0x39) || ((c8&0xF0F0FF00)==0x30303A00 && (buf(9)<0x30 || buf(9)>0x39)))) \
    (*Content).Type |= Time; \
\
  if ((*Content).Length>=8 && (c8&0x80808080)==0 && (c4&0x80808080)==0) \
    (*Content).Type |= Text; \
\
  if ((c8&0xF0F0FF)==0x3030C2 && (c4&0xFFF0F0FF)==0xB0303027){ \
    int i = 2; \
    while ((i<7) && buf(i)>=0x30 && buf(i)<=0x39) \
      i+=(i&1)*2+1; \
\
    if (i==10) \
      (*Content).Type |= Coordinates; \
  } \
\
  if ((c4&0xFFFFFA)==0xC2B042 && B!=0x47 && (((c4>>24)>=0x30 && (c4>>24)<=0x39) || ((c4>>24)==0x20 && (buf(5)>=0x30 && buf(5)<=0x39)))) \
    (*Content).Type |= Temperature; \
\
  if (B>=0x30 && B<=0x39) \
    (*Content).Type |= Number; \
\
  if (c4==0x4953424E && buf(5)==0x20) \
    (*Content).Type |= ISBN; \
}

void XMLModel(Mixer& m, ModelStats *Stats = NULL){
  static ContextMap cm(MEM/4, 4);
  static XMLTagCache Cache;
  static U32 StateBH[8];
  static XMLState State = None, pState = None;
  static U32 c8, WhiteSpaceRun = 0, pWSRun = 0, IndentTab = 0, IndentStep = 2, LineEnding = 2;

  if (bpos==0) {
    U8 B = (U8)c4;
    XMLTag *pTag = &Cache.Tags[ (Cache.Index-1)&(CacheSize-1) ], *Tag = &Cache.Tags[ Cache.Index&(CacheSize-1) ];
    XMLAttribute *Attribute = &((*Tag).Attributes.Items[ (*Tag).Attributes.Index&3 ]);
    XMLContent *Content = &(*Tag).Content;
    pState = State;
    c8 = (c8<<8)|buf(5);
    if ((B==0x09 || B==0x20) && (B==(U8)(c4>>8) || !WhiteSpaceRun)){
      WhiteSpaceRun++;
      IndentTab = (B==0x09);
    }
    else{
      if ((State==None || (State==ReadContent && (*Content).Length<=LineEnding+WhiteSpaceRun)) && WhiteSpaceRun>1+IndentTab && WhiteSpaceRun!=pWSRun){
        IndentStep=abs((int)(WhiteSpaceRun-pWSRun));
        pWSRun = WhiteSpaceRun;
      }
      WhiteSpaceRun=0;
    }
    if (B==0x0A)
      LineEnding = 1+((U8)(c4>>8)==0x0D);

    switch (State){
      case None : {
        if (B==0x3C){
          State = ReadTagName;
          memset(Tag, 0, sizeof(XMLTag));
          (*Tag).Level = ((*pTag).EndTag || (*pTag).Empty)?(*pTag).Level:(*pTag).Level+1;
        }
        if ((*Tag).Level>1)
          DetectContent();

        cm.set(hash(pState, State, ((*pTag).Level+1)*IndentStep - WhiteSpaceRun));
        break;
      }
      case ReadTagName : {
        if ((*Tag).Length>0 && (B==0x09 || B==0x0A || B==0x0D || B==0x20))
          State = ReadTag;
        else if ((B==0x3A || (B>='A' && B<='Z') || B==0x5F || (B>='a' && B<='z')) || ((*Tag).Length>0 && (B==0x2D || B==0x2E || (B>='0' && B<='9')))){
          (*Tag).Length++;
          (*Tag).Name = (*Tag).Name * 263 * 32 + (B&0xDF);
        }
        else if (B == 0x3E){
          if ((*Tag).EndTag){
            State = None;
            Cache.Index++;
          }
          else
            State = ReadContent;
        }
        else if (B!=0x21 && B!=0x2D && B!=0x2F && B!=0x5B){
          State = None;
          Cache.Index++;
        }
        else if ((*Tag).Length==0){
          if (B==0x2F){
            (*Tag).EndTag = true;
            (*Tag).Level = max(0,(*Tag).Level-1);
          }
          else if (c4==0x3C212D2D){
            State = ReadComment;
            (*Tag).Level = max(0,(*Tag).Level-1);
          }
        }

        if ((*Tag).Length==1 && (c4&0xFFFF00)==0x3C2100){
          memset(Tag, 0, sizeof(XMLTag));
          State = None;
        }
        else if ((*Tag).Length==5 && c8==0x215B4344 && c4==0x4154415B){
          State = ReadCDATA;
          (*Tag).Level = max(0,(*Tag).Level-1);
        }

        int i = 1;
        do{
          pTag = &Cache.Tags[ (Cache.Index-i)&(CacheSize-1) ];
          i+=1+((*pTag).EndTag && Cache.Tags[ (Cache.Index-i-1)&(CacheSize-1) ].Name==(*pTag).Name);
        }
        while ( i<CacheSize && ((*pTag).EndTag || (*pTag).Empty) );

        cm.set(hash(pState*8+State, (*Tag).Name, (*Tag).Level, (*pTag).Name, (*pTag).Level!=(*Tag).Level ));
        break;
      }
      case ReadTag : {
        if (B==0x2F)
          (*Tag).Empty = true;
        else if (B==0x3E){
          if ((*Tag).Empty){
            State = None;
            Cache.Index++;
          }
          else
            State = ReadContent;
        }
        else if (B!=0x09 && B!=0x0A && B!=0x0D && B!=0x20){
          State = ReadAttributeName;
          (*Attribute).Name = B&0xDF;
        }
        cm.set(hash(pState, State, (*Tag).Name, B, (*Tag).Attributes.Index ));
        break;
      }
      case ReadAttributeName : {
        if ((c4&0xFFF0)==0x3D20 && (B==0x22 || B==0x27)){
          State = ReadAttributeValue;
          if ((c8&0xDFDF)==0x4852 && (c4&0xDFDF0000)==0x45460000)
            (*Content).Type |= Link;
        }
        else if (B!=0x22 && B!=0x27 && B!=0x3D)
          (*Attribute).Name = (*Attribute).Name * 263 * 32 + (B&0xDF);

        cm.set(hash(pState*8+State, (*Attribute).Name, (*Tag).Attributes.Index, (*Tag).Name, (*Content).Type ));
        break;
      }
      case ReadAttributeValue : {
        if (B==0x22 || B==0x27){
          (*Tag).Attributes.Index++;
          State = ReadTag;
        }
        else{
          (*Attribute).Value = (*Attribute).Value* 263 * 32 + (B&0xDF);
          (*Attribute).Length++;
          if ((c8&0xDFDFDFDF)==0x48545450 && ((c4>>8)==0x3A2F2F || c4==0x733A2F2F))
            (*Content).Type |= URL;
        }
        cm.set(hash(pState, State, (*Attribute).Name, (*Content).Type ));
        break;
      }
      case ReadContent : {
        if (B==0x3C){
          State = ReadTagName;
          Cache.Index++;
          memset(&Cache.Tags[ Cache.Index&(CacheSize-1) ], 0, sizeof(XMLTag));
          Cache.Tags[ Cache.Index&(CacheSize-1) ].Level = (*Tag).Level+1;
        }
        else{
          (*Content).Length++;
          (*Content).Data = (*Content).Data * 997*16 + (B&0xDF);

          DetectContent();
        }
        cm.set(hash(pState, State, (*Tag).Name, c4&0xC0FF ));
        break;
      }
      case ReadCDATA : {
        if ((c4&0xFFFFFF)==0x5D5D3E){
          State = None;
          Cache.Index++;
        }
        cm.set(hash(pState, State));
        break;
      }
      case ReadComment : {
        if ((c4&0xFFFFFF)==0x2D2D3E){
          State = None;
          Cache.Index++;
        }
        cm.set(hash(pState, State));
        break;
      }
    }

    StateBH[pState] = (StateBH[pState]<<8)|B;
    pTag = &Cache.Tags[ (Cache.Index-1)&(CacheSize-1) ];
    cm.set(hash(State, (*Tag).Level, pState*2+(*Tag).EndTag, (*Tag).Name));
    cm.set(hash((*pTag).Name, State*2+(*pTag).EndTag, (*pTag).Content.Type, (*Tag).Content.Type));
    cm.set(hash(State*2+(*Tag).EndTag, (*Tag).Name, (*Tag).Content.Type, c4&0xE0FF));
  }
  cm.mix(m);
  U8 s = ((StateBH[State]>>(28-bpos))&0x08) |
         ((StateBH[State]>>(21-bpos))&0x04) |
         ((StateBH[State]>>(14-bpos))&0x02) |
         ((StateBH[State]>>( 7-bpos))&0x01) |
         ((bpos)<<4);
  if (Stats)
    (*Stats).XML = (s<<3)|State;
}

//////////////////////////// contextModel //////////////////////

// This combines all the context models with a Mixer.

class ContextModel{
  ContextMap cm;
  exeModel exeModel1;
  RunContextMap rcm7, rcm9, rcm10;
  Mixer m;
  U32 cxt[16];
  Filetype ft2, filetype;
  int blocksize, blockinfo;
  void UpdateContexts(U8 B);
  void Train();
public:
  ContextModel() : cm(MEM*32, 9), rcm7(MEM), rcm9(MEM), rcm10(MEM), m(1000, 4096+(1024+512+1024*3)*(level>=4), 7+5*(level>=4)), ft2(DEFAULT), filetype(DEFAULT), blocksize(0), blockinfo(0){
    memset(&cxt, 0, 16*sizeof(U32));
    if (trainTXT) Train();
  }
  int Predict(ModelStats *Stats = NULL);
};

void ContextModel::Train(){
  FILE *f;
  int i;
  char filename[MAX_PATH+12];
  if ((i=GetModuleFileName(NULL, filename,MAX_PATH)) && i<=MAX_PATH){
    char *p=strrchr(filename, '\\');
    if (p!=0){
      p++;
      strcpy(p,"english.dic");

      if ((f = fopen(filename,"rb"))!=NULL){
        printf("Pre-training main model...");
        i=0;
        do{
          if (!i || i==10 || i==13)
            i = SPACE;
          UpdateContexts(buf(1));
          cm.Train(i);
          buf[pos++]=i;
        } while ((i=getc(f))!=EOF);
        printf(" done [%d bytes]\n",pos);
        fclose(f);
        cm.FinishTraining();
        pos=0;
        memset(&cxt[0], 0, 16*sizeof(U32));
        memset(&buf[0], 0, buf.size());
      }
    }
  }
}

void ContextModel::UpdateContexts(U8 B){
  for (int i=15; i>0; --i)
    cxt[i]=cxt[i-1]*257+B+1;
  for (int i=0; i<7; ++i)
    cm.set(cxt[i]);
  cm.set(cxt[8]);
  cm.set(cxt[14]);
}

int ContextModel::Predict(ModelStats *Stats){
  // Parse block type and block size
  if (bpos==0) {
    --blocksize;
    ++blpos;
    if (blocksize==-1) ft2=(Filetype)buf(1);
    if (blocksize==-5 && !hasInfo(ft2)) {
      blocksize=buf(4)<<24|buf(3)<<16|buf(2)<<8|buf(1);
      if (hasRecursion(ft2)) blocksize=0;
      blpos=0;
    }
    if (blocksize==-9) {
      blocksize=buf(8)<<24|buf(7)<<16|buf(6)<<8|buf(5);
      blockinfo=buf(4)<<24|buf(3)<<16|buf(2)<<8|buf(1);
      blpos=0;
    }
    if (!blpos) filetype=ft2;
    if (blocksize==0) filetype=DEFAULT;
  }

  m.update();
  m.add(256); //network bias

  // Test for special block types
  int ismatch=ilog(matchModel(m));  // Length of longest matching context
  if (filetype==IMAGE1) im1bitModel(m, blockinfo);
  if (filetype==IMAGE4) return im4bitModel(m, blockinfo), m.p();
  if (filetype==IMAGE8) return im8bitModel(m, blockinfo), m.p();
  if (filetype==IMAGE8GRAY) return im8bitModel(m, blockinfo, 1), m.p();
  if (filetype==IMAGE24) return im24bitModel(m, blockinfo), m.p();
  if (filetype==IMAGE32) return im24bitModel(m, blockinfo, 1), m.p();
  if (filetype==PNG8) return im8bitModel(m, blockinfo, 0, 1), m.p();
  if (filetype==PNG8GRAY) return im8bitModel(m, blockinfo, 1, 1), m.p();
  if (filetype==PNG24) return im24bitModel(m, blockinfo, 0, 1), m.p();
  if (filetype==PNG32) return im24bitModel(m, blockinfo, 1, 1), m.p();
  if (filetype==AUDIO) return wavModel(m, blockinfo, Stats), m.p();
  if ((filetype==JPEG || filetype==HDR)) if (jpegModel(m)) return m.p();

  // Normal model
  if (bpos==0) {
    UpdateContexts(buf(1));
    rcm7.set(cxt[7]);
    rcm9.set(cxt[10]);
    rcm10.set(cxt[12]);
  }
  int order=cm.mix(m);

  rcm7.mix(m);
  rcm9.mix(m);
  rcm10.mix(m);

  if (level>=4 && filetype!=IMAGE1) {
    sparseModel(m,ismatch,order);
    distanceModel(m);
    recordModel(m, Stats);
    wordModel(m, filetype);
    indirectModel(m);
    dmcModel(m);
    nestModel(m);
    XMLModel(m, Stats);
    exeModel1.Predict(m, filetype==EXE, Stats);
  }

  order = order-2;
  if (order<0) order=0;

  U32 c1=buf(1), c2=buf(2), c3=buf(3), c;

  m.set(8+ c1 + (bpos>5)*256 + ( ((c0&((1<<bpos)-1))==0) || (c0==((2<<bpos)-1)) )*512, 8+1024);
  m.set(c0, 256);
  m.set(order+8*(c4>>6&3)+32*(bpos==0)+64*(c1==c2)+128*(filetype==EXE), 256);
  m.set(c2, 256);
  m.set(c3, 256);
  m.set(ismatch, 256);

  if (bpos!=0)
  {
    c=c0<<(8-bpos); if (bpos==1)c+=c3/2;
    c=(min(bpos,5))*256+c1/32+8*(c2/32)+(c&192);
  }
  else c=c3/128+(c4>>31)*2+4*(c2/64)+(c1&240);
  m.set(c, 1536);
  int pr=m.p();
  return pr;
}

//////////////////////////// Predictor /////////////////////////

// A Predictor estimates the probability that the next bit of
// uncompressed data is 1.  Methods:
// p() returns P(1) as a 12 bit number (0-4095).
// update(y) trains the predictor with the actual bit (0 or 1).

class Predictor {
  int pr;  // next prediction
  ContextModel ContextModel2;
  ModelStats stats;
public:
  Predictor();
  int p() const {assert(pr>=0 && pr<4096); return pr;}
  void update();
};

Predictor::Predictor(): pr(2048) {
  memset(&stats, 0, sizeof(ModelStats));
}

void Predictor::update() {
  static APM1 a(256), a1(0x10000), a2(0x10000), a3(0x10000),
                      a4(0x10000), a5(0x10000), a6(0x10000);

  // Update global context: pos, bpos, c0, c4, buf, f4
  c0+=c0+y;
  if (c0>=256) {
    buf[pos++]=c0;
    c4=(c4<<8)+c0-256;
    c0=1;

    int b1=buf(1);
    b2=b1;
    if(b1=='.' || b1=='!' || b1=='?' || b1=='/'|| b1==')'|| b1=='}') f4=(f4&0xfffffff0)+2;
    if (b1==32) --b1;
    f4=f4*16+(b1>>4);

  }
  bpos=(bpos+1)&7;

  // Filter the context model with APMs
  int pr0=ContextModel2.Predict(&stats);

  pr=a.p(pr0, c0);

  int pr1=a1.p(pr0, c0+256*buf(1));
  int pr2=a2.p(pr0, (c0^hash(buf(1), buf(2)))&0xffff);
  int pr3=a3.p(pr0, (c0^hash(buf(1), buf(2), buf(3)))&0xffff);
  pr0=(pr0+pr1+pr2+pr3+2)>>2;

  pr1=a4.p(pr, c0+256*buf(1));
  pr2=a5.p(pr, (c0^hash(buf(1), buf(2)))&0xffff);
  pr3=a6.p(pr, (c0^hash(buf(1), buf(2), buf(3)))&0xffff);
  pr=(pr+pr1+pr2+pr3+2)>>2;

  pr=(pr+pr0+1)>>1;
}

//////////////////////////// Encoder ////////////////////////////

// An Encoder does arithmetic encoding.  Methods:
// Encoder(COMPRESS, f) creates encoder for compression to archive f, which
//   must be open past any header for writing in binary mode.
// Encoder(DECOMPRESS, f) creates encoder for decompression from archive f,
//   which must be open past any header for reading in binary mode.
// code(i) in COMPRESS mode compresses bit i (0 or 1) to file f.
// code() in DECOMPRESS mode returns the next decompressed bit from file f.
//   Global y is set to the last bit coded or decoded by code().
// compress(c) in COMPRESS mode compresses one byte.
// decompress() in DECOMPRESS mode decompresses and returns one byte.
// flush() should be called exactly once after compression is done and
//   before closing f.  It does nothing in DECOMPRESS mode.
// size() returns current length of archive
// setFile(f) sets alternate source to FILE* f for decompress() in COMPRESS
//   mode (for testing transforms).
// If level (global) is 0, then data is stored without arithmetic coding.

typedef enum {COMPRESS, DECOMPRESS} Mode;
class Encoder {
private:
  Predictor predictor;
  const Mode mode;       // Compress or decompress?
  FILE* archive;         // Compressed data file
  U32 x1, x2;            // Range, initially [0, 1), scaled by 2^32
  U32 x;                 // Decompress mode: last 4 input bytes of archive
  FILE *alt;             // decompress() source in COMPRESS mode
  float p1, p2;

  // Compress bit y or return decompressed bit
  int code(int i=0) {
    int p=predictor.p();
    assert(p>=0 && p<4096);
    p+=p<2048;
    U32 xmid=x1 + ((x2-x1)>>12)*p + (((x2-x1)&0xfff)*p>>12);
    assert(xmid>=x1 && xmid<x2);
    if (mode==DECOMPRESS) y=x<=xmid; else y=i;
    y ? (x2=xmid) : (x1=xmid+1);
    predictor.update();
    while (((x1^x2)&0xff000000)==0) {  // pass equal leading bytes of range
      if (mode==COMPRESS) putc(x2>>24, archive);
      x1<<=8;
      x2=(x2<<8)+255;
      if (mode==DECOMPRESS) x=(x<<8)+(getc(archive)&255);  // EOF is OK
    }
    return y;
  }

public:
  Encoder(Mode m, FILE* f);
  Mode getMode() const {return mode;}
  U64 size() const {return ftello(archive);}  // length of archive so far
  void flush();  // call this when compression is finished
  void setFile(FILE* f) {alt=f;}

  // Compress one byte
  void compress(int c) {
    assert(mode==COMPRESS);
    if (level==0)
      putc(c, archive);
    else
      for (int i=7; i>=0; --i)
        code((c>>i)&1);
  }

  // Decompress and return one byte
  int decompress() {
    if (mode==COMPRESS) {
      assert(alt);
      return getc(alt);
    }
    else if (level==0)
      return getc(archive);
    else {
      int c=0;
      for (int i=0; i<8; ++i)
        c+=c+code();
      return c;
    }
  }

  void set_status_range(float perc1, float perc2) { p1=perc1; p2=perc2; }
  void print_status(U64 n, U64 size) {
    printf("%6.2f%%\b\b\b\b\b\b\b", (p1+(p2-p1)*n/(size+1))*100), fflush(stdout);
  }
  void print_status() {
    printf("%6.2f%%\b\b\b\b\b\b\b", float(size())/(p2+1)*100), fflush(stdout);
  }
};

Encoder::Encoder(Mode m, FILE* f):
    mode(m), archive(f), x1(0), x2(0xffffffff), x(0), alt(0) {
  if (mode==DECOMPRESS) {
    U64 start=size();
    fseeko(archive, 0, SEEK_END);
    U64 end=size();
    if (end>=(1u<<31))quit("Large archives not yet supported.");
    set_status_range(0.0, (float)end);
    fseeko(archive, start, SEEK_SET);
  }
  if (level>0 && mode==DECOMPRESS) {  // x = first 4 bytes of archive
    for (int i=0; i<4; ++i)
      x=(x<<8)+(getc(archive)&255);
  }
  for (int i=0; i<1024; ++i)
    dt[i]=16384/(i+i+3);

}

void Encoder::flush() {
  if (mode==COMPRESS && level>0)
    putc(x1>>24, archive);  // Flush first unequal byte of range
}

/////////////////////////// Filters /////////////////////////////////
//
// Before compression, data is encoded in blocks with the following format:
//
//   <type> <size> <encoded-data>
//
// Type is 1 byte (type Filetype): DEFAULT=0, JPEG, EXE
// Size is 4 bytes in big-endian format.
// Encoded-data decodes to <size> bytes.  The encoded size might be
// different.  Encoded data is designed to be more compressible.
//
//   void encode(FILE* in, FILE* out, int n);
//
// Reads n bytes of in (open in "rb" mode) and encodes one or
// more blocks to temporary file out (open in "wb+" mode).
// The file pointer of in is advanced n bytes.  The file pointer of
// out is positioned after the last byte written.
//
//   en.setFile(FILE* out);
//   int decode(Encoder& en);
//
// Decodes and returns one byte.  Input is from en.decompress(), which
// reads from out if in COMPRESS mode.  During compression, n calls
// to decode() must exactly match n bytes of in, or else it is compressed
// as type 0 without encoding.
//
//   Filetype detect(FILE* in, int n, Filetype type);
//
// Reads n bytes of in, and detects when the type changes to
// something else.  If it does, then the file pointer is repositioned
// to the start of the change and the new type is returned.  If the type
// does not change, then it repositions the file pointer n bytes ahead
// and returns the old type.
//
// For each type X there are the following 2 functions:
//
//   void encode_X(FILE* in, FILE* out, int n, ...);
//
// encodes n bytes from in to out.
//
//   int decode_X(Encoder& en);
//
// decodes one byte from en and returns it.  decode() and decode_X()
// maintain state information using static variables.
#define bswap(x) \
+   ((((x) & 0xff000000) >> 24) | \
+    (((x) & 0x00ff0000) >>  8) | \
+    (((x) & 0x0000ff00) <<  8) | \
+    (((x) & 0x000000ff) << 24))

#define IMG_DET(type,start_pos,header_len,width,height) return dett=(type),\
deth=(header_len),detd=(width)*(height),info=(width),\
fseeko(in, start+(start_pos), SEEK_SET),HDR

#define AUD_DET(type,start_pos,header_len,data_len,wmode) return dett=(type),\
deth=(header_len),detd=(data_len),info=(wmode),\
fseeko(in, start+(start_pos), SEEK_SET),HDR


// Function ecc_compute(), edc_compute() and eccedc_init() taken from
// ** UNECM - Decoder for ECM (Error Code Modeler) format.
// ** Version 1.0
// ** Copyright (C) 2002 Neill Corlett

/* LUTs used for computing ECC/EDC */
static U8 ecc_f_lut[256];
static U8 ecc_b_lut[256];
static U32 edc_lut[256];
static int luts_init=0;

void eccedc_init(void) {
  if (luts_init) return;
  U32 i, j, edc;
  for(i = 0; i < 256; i++) {
    j = (i << 1) ^ (i & 0x80 ? 0x11D : 0);
    ecc_f_lut[i] = j;
    ecc_b_lut[i ^ j] = i;
    edc = i;
    for(j = 0; j < 8; j++) edc = (edc >> 1) ^ (edc & 1 ? 0xD8018001 : 0);
    edc_lut[i] = edc;
  }
  luts_init=1;
}

void ecc_compute(U8 *src, U32 major_count, U32 minor_count, U32 major_mult, U32 minor_inc, U8 *dest) {
  U32 size = major_count * minor_count;
  U32 major, minor;
  for(major = 0; major < major_count; major++) {
    U32 index = (major >> 1) * major_mult + (major & 1);
    U8 ecc_a = 0;
    U8 ecc_b = 0;
    for(minor = 0; minor < minor_count; minor++) {
      U8 temp = src[index];
      index += minor_inc;
      if(index >= size) index -= size;
      ecc_a ^= temp;
      ecc_b ^= temp;
      ecc_a = ecc_f_lut[ecc_a];
    }
    ecc_a = ecc_b_lut[ecc_f_lut[ecc_a] ^ ecc_b];
    dest[major              ] = ecc_a;
    dest[major + major_count] = ecc_a ^ ecc_b;
  }
}

U32 edc_compute(const U8  *src, int size) {
  U32 edc = 0;
  while(size--) edc = (edc >> 8) ^ edc_lut[(edc ^ (*src++)) & 0xFF];
  return edc;
}

int expand_cd_sector(U8 *data, int a, int test) {
  U8 d2[2352];
  eccedc_init();
  d2[0]=d2[11]=0;
  for (int i=1; i<11; i++) d2[i]=255;
  int mode=(data[15]!=1?2:1);
  int form=(data[15]==3?2:1);
  if (a==-1) for (int i=12; i<15; i++) d2[i]=data[i]; else {
    int c1=(a&15)+((a>>4)&15)*10;
    int c2=((a>>8)&15)+((a>>12)&15)*10;
    int c3=((a>>16)&15)+((a>>20)&15)*10;
    c1=(c1+1)%75;
    if (c1==0) {
      c2=(c2+1)%60;
      if (c2==0) c3++;
    }
    d2[12]=(c3%10)+16*(c3/10);
    d2[13]=(c2%10)+16*(c2/10);
    d2[14]=(c1%10)+16*(c1/10);
  }
  d2[15]=mode;
  if (mode==2) for (int i=16; i<24; i++) d2[i]=data[i-4*(i>=20)];
  if (form==1) {
    if (mode==2) {
      d2[1]=d2[12],d2[2]=d2[13],d2[3]=d2[14];
      d2[12]=d2[13]=d2[14]=d2[15]=0;
    } else {
      for(int i=2068; i<2076; i++) d2[i]=0;
    }
    for (int i=16+8*(mode==2); i<2064+8*(mode==2); i++) d2[i]=data[i];
    U32 edc=edc_compute(d2+16*(mode==2), 2064-8*(mode==2));
    for (int i=0; i<4; i++) d2[2064+8*(mode==2)+i]=(edc>>(8*i))&0xff;
    ecc_compute(d2+12, 86, 24,  2, 86, d2+2076);
    ecc_compute(d2+12, 52, 43, 86, 88, d2+2248);
    if (mode==2) {
      d2[12]=d2[1],d2[13]=d2[2],d2[14]=d2[3],d2[15]=2;
      d2[1]=d2[2]=d2[3]=255;
    }
  }
  for (int i=0; i<2352; i++) if (d2[i]!=data[i] && test) form=2;
  if (form==2) {
    for (int i=24; i<2348; i++) d2[i]=data[i];
    U32 edc=edc_compute(d2+16, 2332);
    for (int i=0; i<4; i++) d2[2348+i]=(edc>>(8*i))&0xff;
  }
  for (int i=0; i<2352; i++) if (d2[i]!=data[i] && test) return 0; else data[i]=d2[i];
  return mode+form-1;
}

int parse_zlib_header(int header) {
    switch (header) {
        case 0x2815 : return 0;  case 0x2853 : return 1;  case 0x2891 : return 2;  case 0x28cf : return 3;
        case 0x3811 : return 4;  case 0x384f : return 5;  case 0x388d : return 6;  case 0x38cb : return 7;
        case 0x480d : return 8;  case 0x484b : return 9;  case 0x4889 : return 10; case 0x48c7 : return 11;
        case 0x5809 : return 12; case 0x5847 : return 13; case 0x5885 : return 14; case 0x58c3 : return 15;
        case 0x6805 : return 16; case 0x6843 : return 17; case 0x6881 : return 18; case 0x68de : return 19;
        case 0x7801 : return 20; case 0x785e : return 21; case 0x789c : return 22; case 0x78da : return 23;
    }
    return -1;
}
int zlib_inflateInit(z_streamp strm, int zh) {
    if (zh==-1) return inflateInit2(strm, -MAX_WBITS); else return inflateInit(strm);
}

bool IsGrayscalePalette(FILE* in, int n = 256, int isRGBA = 0){
  U64 offset = ftello(in);
  int stride = 3+isRGBA, res = (n>0)<<8, order=1;
  for (int i = 0; (i < n*stride) && (res>>8); i++) {
    int b = getc(in);
    if (b==EOF){
      res = 0;
      break;
    }
    if (!i) {
      res = 0x100|b;
      order = 1-2*(b>0);
      continue;
    }

    //"j" is the index of the current byte in this color entry
    int j = i%stride;
    if (!j){
      // load first component of this entry
      res = (res&((b-(res&0xFF)==order)<<8));
      res|=(res)?b:0;
    }
    else if (j==3)
      res&=((!b || (b==0xFF))*0x1FF); // alpha/attribute component must be zero or 0xFF
    else
      res&=((b==(res&0xFF))<<9)-1;
  }
  fseeko(in, offset, SEEK_SET);
  return (res>>8)>0;
}

// Detect blocks
Filetype detect(FILE* in, U64 blocksize, Filetype type, int &info) {
  int n = (int)blocksize;
  //TODO: Large file support
  U32 buf3=0, buf2=0, buf1=0, buf0=0;  // last 16 bytes
  U64 start=ftello(in);

  // For EXE detection
  Array<int> abspos(256),  // CALL/JMP abs. addr. low byte -> last offset
    relpos(256);    // CALL/JMP relative addr. low byte -> last offset
  int e8e9count=0;  // number of consecutive CALL/JMPs
  int e8e9pos=0;    // offset of first CALL or JMP instruction
  int e8e9last=0;   // offset of most recent CALL or JMP

  int soi=0, sof=0, sos=0, app=0;  // For JPEG detection - position where found
  int wavi=0,wavsize=0,wavch=0,wavbps=0,wavm=0,wavtype=0,wavlen=0,wavlist=0;  // For WAVE detection
  int aiff=0,aiffm=0,aiffs=0;  // For AIFF detection
  int s3mi=0,s3mno=0,s3mni=0;  // For S3M detection
  int bmp=0,imgbpp=0,bmpx=0,bmpy=0,bmpof=0,bmps=0,hdrless=0;  // For BMP detection
  int rgbi=0,rgbx=0,rgby=0;  // For RGB detection
  int tga=0,tgax=0,tgay=0,tgaz=0,tgat=0,tgaid=0,tgamap=0;  // For TGA detection
  int pgm=0,pgmcomment=0,pgmw=0,pgmh=0,pgm_ptr=0,pgmc=0,pgmn=0,pamatr=0,pamd=0;  // For PBM, PGM, PPM, PAM detection
  char pgm_buf[32];
  int cdi=0,cda=0,cdm=0;  // For CD sectors detection
  U32 cdf=0;
  unsigned char zbuf[256+32], zin[1<<16], zout[1<<16]; // For ZLIB stream detection
  int zbufpos=0,zzippos=-1, histogram[256]={0};
  int pdfim=0,pdfimw=0,pdfimh=0,pdfimb=0,pdfgray=0,pdfimp=0;
  int b64s=0,b64i=0,b64line=0,b64nl=0; // For base64 detection
  int gif=0,gifa=0,gifi=0,gifw=0,gifc=0,gifb=0,gifplt=0,gifgray=0; // For GIF detection
  int png=0, pngw=0, pngh=0, pngbps=0, pngtype=0, pnggray=0, lastchunk=0, nextchunk=0; // For PNG detection

  // For image detection
  static int deth=0,detd=0;  // detected header/data size in bytes
  static Filetype dett;  // detected block type
  if (deth) return fseeko(in, start+deth, SEEK_SET),deth=0,dett;
  else if (detd) return fseeko(in, start+detd, SEEK_SET),detd=0,DEFAULT;

  for (int i=0; i<n; ++i) {
    int c=getc(in);
    if (c==EOF) return (Filetype)(-1);
    buf3=buf3<<8|buf2>>24;
    buf2=buf2<<8|buf1>>24;
    buf1=buf1<<8|buf0>>24;
    buf0=buf0<<8|c;

    // detect PNG images
    if (!png && buf3==0x89504E47 /*%PNG*/ && buf2==0x0D0A1A0A && buf1==0x0000000D && buf0==0x49484452) png=i, pngtype=-1, lastchunk=buf3;
    if (png){
      const int p=i-png;
      if (p==12){
        pngw = buf2;
        pngh = buf1;
        pngbps = buf0>>24;
        pngtype = (U8)(buf0>>16);
        pnggray = 0;
        png*=((buf0&0xFFFF)==0 && pngw && pngh && pngbps==8 && (!pngtype || pngtype==2 || pngtype==3 || pngtype==4 || pngtype==6));
      }
      else if (p>12 && pngtype<0)
        png = 0;
      else if (p==17){
        png*=((buf1&0xFF)==0);
        nextchunk =(png)?i+8:0;
      }
      else if (p>17 && i==nextchunk){
        nextchunk+=buf1+4/*CRC*/+8/*Chunk length+Id*/;
        lastchunk = buf0;
        png*=(lastchunk!=0x49454E44/*IEND*/);
        if (lastchunk==0x504C5445/*PLTE*/){
          png*=(buf1%3==0);
          pnggray = (png && IsGrayscalePalette(in, buf1/3));
        }
      }
    }

    // ZLIB stream detection
    histogram[c]++;
    if (i>=256)
      histogram[zbuf[zbufpos]]--;
    zbuf[zbufpos] = c;
    if (zbufpos<32)
      zbuf[zbufpos+256] = c;
    zbufpos=(zbufpos+1)&0xFF;

    int zh=parse_zlib_header(((int)zbuf[(zbufpos-32)&0xFF])*256+(int)zbuf[(zbufpos-32+1)&0xFF]);
    bool valid = (i>=31 && zh!=-1);
    if (!valid && brute && i>=255){
      U8 BTYPE = (zbuf[zbufpos]&7)>>1;
      if ((valid=(BTYPE==1 || BTYPE==2))){
        int maximum=0, used=0, offset=zbufpos;
        for (int i=0;i<4;i++,offset+=64){
          for (int j=0;j<64;j++){
            int freq = histogram[zbuf[(offset+j)&0xFF]];
            used+=(freq>0);
            maximum+=(freq>maximum);
          }
          if (maximum>=((12+i)<<i) || used*(6-i)<(i+1)*64){
            valid = false;
            break;
          }
        }
      }
    }
    if (valid || zzippos==i) {
      int streamLength=0, ret=0, brute=(zh==-1 && zzippos!=i);

      // Quick check possible stream by decompressing first 32 bytes
      z_stream strm;
      strm.zalloc=Z_NULL; strm.zfree=Z_NULL; strm.opaque=Z_NULL;
      strm.next_in=Z_NULL; strm.avail_in=0;
      if (zlib_inflateInit(&strm,zh)==Z_OK) {
        strm.next_in=&zbuf[(zbufpos-(brute?0:32))&0xFF]; strm.avail_in=32;
        strm.next_out=zout; strm.avail_out=1<<16;
        ret=inflate(&strm, Z_FINISH);
        ret=(inflateEnd(&strm)==Z_OK && (ret==Z_STREAM_END || ret==Z_BUF_ERROR) && strm.total_in>=16);
      }
      if (ret) {
        // Verify valid stream and determine stream length
        U64 savedpos=ftello(in);
        strm.zalloc=Z_NULL; strm.zfree=Z_NULL; strm.opaque=Z_NULL;
        strm.next_in=Z_NULL; strm.avail_in=0; strm.total_in=strm.total_out=0;
        if (zlib_inflateInit(&strm,zh)==Z_OK) {
          for (int j=i-(brute?255:31); j<n; j+=1<<16) {
            unsigned int blsize=min(n-j,1<<16);
            fseeko(in, start+j, SEEK_SET);
            if (fread(zin, 1, blsize, in)!=blsize) break;
            strm.next_in=zin; strm.avail_in=blsize;
            do {
              strm.next_out=zout; strm.avail_out=1<<16;
              ret=inflate(&strm, Z_FINISH);
            } while (strm.avail_out==0 && ret==Z_BUF_ERROR);
            if (ret==Z_STREAM_END) streamLength=strm.total_in;
            if (ret!=Z_BUF_ERROR) break;
          }
          if (inflateEnd(&strm)!=Z_OK) streamLength=0;
        }
        fseeko(in, savedpos, SEEK_SET);
      }
      if (streamLength>brute<<7) {
        info=0;
        if (pdfimw>0 && pdfimw<0x1000000 && pdfimh>0) {
          if (pdfimb==8 && (int)strm.total_out==pdfimw*pdfimh) info=((pdfgray?IMAGE8GRAY:IMAGE8)<<24)|pdfimw;
          if (pdfimb==8 && (int)strm.total_out==pdfimw*pdfimh*3) info=(IMAGE24<<24)|pdfimw*3;
          if (pdfimb==4 && (int)strm.total_out==((pdfimw+1)/2)*pdfimh) info=(IMAGE4<<24)|((pdfimw+1)/2);
          if (pdfimb==1 && (int)strm.total_out==((pdfimw+7)/8)*pdfimh) info=(IMAGE1<<24)|((pdfimw+7)/8);
          pdfgray=0;
        }
        else if (png && pngw<0x1000000 && lastchunk==0x49444154/*IDAT*/){
          if (pngbps==8 && pngtype==2 && (int)strm.total_out==(pngw*3+1)*pngh) info=(PNG24<<24)|(pngw*3), png=0;
          else if (pngbps==8 && pngtype==6 && (int)strm.total_out==(pngw*4+1)*pngh) info=(PNG32<<24)|(pngw*4), png=0;
          else if (pngbps==8 && (!pngtype || pngtype==3) && (int)strm.total_out==(pngw+1)*pngh) info=(((!pngtype || pnggray)?PNG8GRAY:PNG8)<<24)|(pngw), png=0;
        }
        return fseeko(in, start+i-(brute?255:31), SEEK_SET),detd=streamLength,ZLIB;
      }
    }
    if (zh==-1 && zbuf[(zbufpos-32)&0xFF]=='P' && zbuf[(zbufpos-32+1)&0xFF]=='K' && zbuf[(zbufpos-32+2)&0xFF]=='\x3'
      && zbuf[(zbufpos-32+3)&0xFF]=='\x4' && zbuf[(zbufpos-32+8)&0xFF]=='\x8' && zbuf[(zbufpos-32+9)&0xFF]=='\0') {
        int nlen=(int)zbuf[(zbufpos-32+26)&0xFF]+((int)zbuf[(zbufpos-32+27)&0xFF])*256
                +(int)zbuf[(zbufpos-32+28)&0xFF]+((int)zbuf[(zbufpos-32+29)&0xFF])*256;
        if (nlen<256 && i+30+nlen<n) zzippos=i+30+nlen;
    }
    if (i-pdfimp>1024) pdfim=pdfimw=pdfimh=pdfimb=pdfgray=0;
    if (pdfim>1 && !(isspace(c) || isdigit(c))) pdfim=1;
    if (pdfim==2 && isdigit(c)) pdfimw=pdfimw*10+(c-'0');
    if (pdfim==3 && isdigit(c)) pdfimh=pdfimh*10+(c-'0');
    if (pdfim==4 && isdigit(c)) pdfimb=pdfimb*10+(c-'0');
    if ((buf0&0xffff)==0x3c3c) pdfimp=i,pdfim=1; // <<
    if (pdfim && (buf1&0xffff)==0x2f57 && buf0==0x69647468) pdfim=2,pdfimw=0; // /Width
    if (pdfim && (buf1&0xffffff)==0x2f4865 && buf0==0x69676874) pdfim=3,pdfimh=0; // /Height
    if (pdfim && buf3==0x42697473 && buf2==0x50657243 && buf1==0x6f6d706f
       && buf0==0x6e656e74 && zbuf[(zbufpos-32+15)&0xFF]=='/') pdfim=4,pdfimb=0; // /BitsPerComponent
    if (pdfim && (buf2&0xFFFFFF)==0x2F4465 && buf1==0x76696365 && buf0==0x47726179) pdfgray=1; // /DeviceGray

    // CD sectors detection (mode 1 and mode 2 form 1+2 - 2352 bytes)
    if (buf1==0x00ffffff && buf0==0xffffffff && !cdi) cdi=i,cda=-1,cdm=0;
    if (cdi && i>cdi) {
      const int p=(i-cdi)%2352;
      if (p==8 && (buf1!=0xffffff00 || ((buf0&0xff)!=1 && (buf0&0xff)!=2))) cdi=0;
      else if (p==16 && i+2336<n) {
        U8 data[2352];
        U64 savedpos=ftello(in);
        fseeko(in, start+i-23, SEEK_SET);
        fread(data, 1, 2352, in);
        fseeko(in, savedpos, SEEK_SET);
        int t=expand_cd_sector(data, cda, 1);
        if (t!=cdm) cdm=t*(i-cdi<2352);
        if (cdm && cda!=10 && (cdm==1 || buf0==buf1)) {
          if (type!=CD) return info=cdm,fseeko(in, start+cdi-7, SEEK_SET), CD;
          cda=(data[12]<<16)+(data[13]<<8)+data[14];
          if (cdm!=1 && i-cdi>2352 && buf0!=cdf) cda=10;
          if (cdm!=1) cdf=buf0;
        } else cdi=0;
      }
      if (!cdi && type==CD) return fseeko(in, start+i-p-7, SEEK_SET), DEFAULT;
    }
    if (type==CD) continue;

    // Detect JPEG by code SOI APPx (FF D8 FF Ex) followed by
    // SOF0 (FF C0 xx xx 08) and SOS (FF DA) within a reasonable distance.
    // Detect end by any code other than RST0-RST7 (FF D9-D7) or
    // a byte stuff (FF 00).

    if (!soi && i>=3 && (buf0&0xffffff00)==0xffd8ff00 && ((buf0&0xFE)==0xC0 || (U8)buf0==0xC4 || ((U8)buf0>=0xDB && (U8)buf0<=0xFE) )) soi=i, app=i+2, sos=sof=0;
    if (soi) {
      if (app==i && (buf0>>24)==0xff &&
         ((buf0>>16)&0xff)>0xc1 && ((buf0>>16)&0xff)<0xff) app=i+(buf0&0xffff)+2;
      if (app<i && (buf1&0xff)==0xff && (buf0&0xfe0000ff)==0xc0000008) sof=i;
      if (sof && sof>soi && i-sof<0x1000 && (buf0&0xffff)==0xffda) {
        sos=i;
        if (type!=JPEG) return fseeko(in, start+soi-3, SEEK_SET), JPEG;
      }
      if (i-soi>0x40000 && !sos) soi=0;
    }
    if (type==JPEG && sos && i>sos && (buf0&0xff00)==0xff00
        && (buf0&0xff)!=0 && (buf0&0xf8)!=0xd0) return DEFAULT;

    // Detect .wav file header
    if (buf0==0x52494646) wavi=i,wavm=wavlen=0;
    if (wavi) {
      int p=i-wavi;
      if (p==4) wavsize=bswap(buf0);
      else if (p==8){
        wavtype=(buf0==0x57415645)?1:(buf0==0x7366626B)?2:0;
        if (!wavtype) wavi=0;
      }
      else if (wavtype){
        if (wavtype==1) {
          if (p==16+wavlen && (buf1!=0x666d7420 || bswap(buf0)!=16)) wavlen=((bswap(buf0)+1)&(-2))+8, wavi*=(buf1==0x666d7420 && bswap(buf0)!=16);
          else if (p==22+wavlen) wavch=bswap(buf0)&0xffff;
          else if (p==34+wavlen) wavbps=bswap(buf0)&0xffff;
          else if (p==40+wavlen+wavm && buf1!=0x64617461) wavm+=((bswap(buf0)+1)&(-2))+8,wavi=(wavm>0xfffff?0:wavi);
          else if (p==40+wavlen+wavm) {
            int wavd=bswap(buf0);
            wavlen=0;
            if ((wavch==1 || wavch==2) && (wavbps==8 || wavbps==16) && wavd>0 && wavsize>=wavd+36
               && wavd%((wavbps/8)*wavch)==0) AUD_DET(AUDIO,wavi-3,44+wavm,wavd,wavch+wavbps/4-3);
            wavi=0;
          }
        }
        else{
          if ((p==16 && buf1!=0x4C495354) || (p==20 && buf0!=0x494E464F))
            wavi=0;
          else if (p>20 && buf1==0x4C495354 && (wavi*=(buf0!=0))){
            wavlen = bswap(buf0);
            wavlist = i;
          }
          else if (wavlist){
            p=i-wavlist;
            if (p==8 && (buf1!=0x73647461 || buf0!=0x736D706C))
              wavi=0;
            else if (p==12){
              int wavd = bswap(buf0);
              if (wavd && (wavd+12)==wavlen)
                AUD_DET(AUDIO,wavi-3,(12+wavlist-(wavi-3)+1)&~1,wavd,1+16/4-3);
              wavi=0;
            }
          }
        }
      }
    }

    // Detect .aiff file header
    if (buf0==0x464f524d) aiff=i,aiffs=0; // FORM
    if (aiff) {
      const int p=i-aiff;
      if (p==12 && (buf1!=0x41494646 || buf0!=0x434f4d4d)) aiff=0; // AIFF COMM
      else if (p==24) {
        const int bits=buf0&0xffff, chn=buf1>>16;
        if ((bits==8 || bits==16) && (chn==1 || chn==2)) aiffm=chn+bits/4+1; else aiff=0;
      } else if (p==42+aiffs && buf1!=0x53534e44) aiffs+=(buf0+8)+(buf0&1),aiff=(aiffs>0x400?0:aiff);
      else if (p==42+aiffs) AUD_DET(AUDIO,aiff-3,54+aiffs,buf0-8,aiffm);
    }

    // Detect .mod file header
    if ((buf0==0x4d2e4b2e || buf0==0x3643484e || buf0==0x3843484e  // M.K. 6CHN 8CHN
       || buf0==0x464c5434 || buf0==0x464c5438) && (buf1&0xc0c0c0c0)==0 && i>=1083) {
      U64 savedpos=ftello(in);
      const int chn=((buf0>>24)==0x36?6:(((buf0>>24)==0x38 || (buf0&0xff)==0x38)?8:4));
      int len=0; // total length of samples
      int numpat=1; // number of patterns
      for (int j=0; j<31; j++) {
        fseeko(in, start+i-1083+42+j*30, SEEK_SET);
        const int i1=getc(in);
        const int i2=getc(in);
        len+=i1*512+i2*2;
      }
      fseeko(in, start+i-131, SEEK_SET);
      for (int j=0; j<128; j++) {
        int x=getc(in);
        if (x+1>numpat) numpat=x+1;
      }
      if (numpat<65) AUD_DET(AUDIO,i-1083,1084+numpat*256*chn,len,4);
      fseeko(in, savedpos, SEEK_SET);
    }

    // Detect .s3m file header
    if (buf0==0x1a100000) s3mi=i,s3mno=s3mni=0;
    if (s3mi) {
      const int p=i-s3mi;
      if (p==4) s3mno=bswap(buf0)&0xffff,s3mni=(bswap(buf0)>>16);
      else if (p==16 && (((buf1>>16)&0xff)!=0x13 || buf0!=0x5343524d)) s3mi=0;
      else if (p==16) {
        U64 savedpos=ftello(in);
        int b[31],sam_start=(1<<16),sam_end=0,ok=1;
        for (int j=0;j<s3mni;j++) {
          fseeko(in, start+s3mi-31+0x60+s3mno+j*2, SEEK_SET);
          int i1=getc(in);
          i1+=getc(in)*256;
          fseeko(in, start+s3mi-31+i1*16, SEEK_SET);
          i1=getc(in);
          if (i1==1) { // type: sample
            for (int k=0;k<31;k++) b[k]=fgetc(in);
            int len=b[15]+(b[16]<<8);
            int ofs=b[13]+(b[14]<<8);
            if (b[30]>1) ok=0;
            if (ofs*16<sam_start) sam_start=ofs*16;
            if (ofs*16+len>sam_end) sam_end=ofs*16+len;
          }
        }
        if (ok && sam_start<(1<<16)) AUD_DET(AUDIO,s3mi-31,sam_start,sam_end-sam_start,0);
        s3mi=0;
        fseeko(in, savedpos, SEEK_SET);
      }
    }

    // Detect .bmp image
    if ( !(bmp || hdrless) && (((buf0&0xffff)==16973) || (!(buf0&0xFFFFFF) && ((buf0>>24)==0x28))) ) //possible 'BM' or headerless DIB
      imgbpp=bmpx=bmpy=0,hdrless=!(U8)buf0,bmpof=hdrless*54,bmp=i-hdrless*16;
    if (bmp || hdrless) {
      const int p=i-bmp;
      if (p==12) bmpof=bswap(buf0);
      else if (p==16 && buf0!=0x28000000) bmp=hdrless=0; //BITMAPINFOHEADER (0x28)
      else if (p==20) bmpx=bswap(buf0),bmp=((bmpx==0||bmpx>0x30000)?(hdrless=0):bmp); //width
      else if (p==24) bmpy=abs((int)bswap(buf0)),bmp=((bmpy==0||bmpy>0x10000)?(hdrless=0):bmp); //height
      else if (p==27) imgbpp=c,bmp=((imgbpp!=1 && imgbpp!=4 && imgbpp!=8 && imgbpp!=24 && imgbpp!=32)?(hdrless=0):bmp);
      else if ((p==31) && buf0) bmp=hdrless=0;
      else if (p==36) bmps=bswap(buf0);
      // check number of colors in palette (4 bytes), must be 0 (default) or <= 1<<bpp.
      // also check if image is too small, since it might not be worth it to use the image models
      else if (p==48){
        if ( (!buf0 || ((bswap(buf0)<=(U32)(1<<imgbpp)) && (imgbpp<=8))) && (((bmpx*bmpy*imgbpp)>>3)>64) ) {
          // possible icon/cursor?
          if (hdrless && (bmpx*2==bmpy) && imgbpp>1 &&
             (
              (bmps>0 && bmps==( (bmpx*bmpy*(imgbpp+1))>>4 )) ||
              ((!bmps || bmps<((bmpx*bmpy*imgbpp)>>3)) && (
               (bmpx==8)   || (bmpx==10) || (bmpx==14) || (bmpx==16) || (bmpx==20) ||
               (bmpx==22)  || (bmpx==24) || (bmpx==32) || (bmpx==40) || (bmpx==48) ||
               (bmpx==60)  || (bmpx==64) || (bmpx==72) || (bmpx==80) || (bmpx==96) ||
               (bmpx==128) || (bmpx==256)
              ))
             )
          )
            bmpy=bmpx;

          // if DIB and not 24bpp, we must calculate the data offset based on BPP or num. of entries in color palette
          if (hdrless && (imgbpp<24))
            bmpof+=((buf0)?bswap(buf0)*4:4<<imgbpp);
          bmpof+=(bmp-1)*(bmp<1);

          if (hdrless && bmps && bmps<((bmpx*bmpy*imgbpp)>>3)) { /*Guard against erroneous DIB detections*/ }
          else if (imgbpp==1) IMG_DET(IMAGE1,max(0,bmp-1),bmpof,(((bmpx-1)>>5)+1)*4,bmpy);
          else if (imgbpp==4) IMG_DET(IMAGE4,max(0,bmp-1),bmpof,((bmpx*4+31)>>5)*4,bmpy);
          else if (imgbpp==8){
            fseeko(in, start+bmp+53, SEEK_SET);
            IMG_DET( (IsGrayscalePalette(in, (buf0)?bswap(buf0):1<<imgbpp, 1))?IMAGE8GRAY:IMAGE8,max(0,bmp-1),bmpof,(bmpx+3)&-4,bmpy);
          }
          else if (imgbpp==24) IMG_DET(IMAGE24,max(0,bmp-1),bmpof,((bmpx*3)+3)&-4,bmpy);
          else if (imgbpp==32) IMG_DET(IMAGE32,max(0,bmp-1),bmpof,bmpx*4,bmpy);
        }
        bmp=hdrless=0;
      }
    }

    // Detect .pbm .pgm .ppm .pam image
    if ((buf0&0xfff0ff)==0x50300a) {
      pgmn=(buf0&0xf00)>>8;
      if ((pgmn>=4 && pgmn<=6) || pgmn==7) pgm=i,pgm_ptr=pgmw=pgmh=pgmc=pgmcomment=pamatr=pamd=0;
    }
    if (pgm) {
      if (i-pgm==1 && c==0x23) pgmcomment=1; //pgm comment
      if (!pgmcomment && pgm_ptr) {
        int s=0;
        if (pgmn==7) {
           if ((buf1&0xdfdf)==0x5749 && (buf0&0xdfdfdfff)==0x44544820) pgm_ptr=0, pamatr=1; // WIDTH
           if ((buf1&0xdfdfdf)==0x484549 && (buf0&0xdfdfdfff)==0x47485420) pgm_ptr=0, pamatr=2; // HEIGHT
           if ((buf1&0xdfdfdf)==0x4d4158 && (buf0&0xdfdfdfff)==0x56414c20) pgm_ptr=0, pamatr=3; // MAXVAL
           if ((buf1&0xdfdf)==0x4445 && (buf0&0xdfdfdfff)==0x50544820) pgm_ptr=0, pamatr=4; // DEPTH
           if ((buf2&0xdf)==0x54 && (buf1&0xdfdfdfdf)==0x55504c54 && (buf0&0xdfdfdfff)==0x59504520) pgm_ptr=0, pamatr=5; // TUPLTYPE
           if ((buf1&0xdfdfdf)==0x454e44 && (buf0&0xdfdfdfff)==0x4844520a) pgm_ptr=0, pamatr=6; // ENDHDR
           if (c==0x0a) {
             if (pamatr==0) pgm=0;
             else if (pamatr<5) s=pamatr;
             if (pamatr!=6) pamatr=0;
           }
        } else if (c==0x20 && !pgmw) s=1;
        else if (c==0x0a && !pgmh) s=2;
        else if (c==0x0a && !pgmc && pgmn!=4) s=3;
        if (s) {
          pgm_buf[pgm_ptr++]=0;
          int v=atoi(pgm_buf);
          if (s==1) pgmw=v; else if (s==2) pgmh=v; else if (s==3) pgmc=v; else if (s==4) pamd=v;
          if (v==0 || (s==3 && v>255)) pgm=0; else pgm_ptr=0;
        }
      }
      if (!pgmcomment) pgm_buf[pgm_ptr++]=c;
      if (pgm_ptr>=32) pgm=0;
      if (pgmcomment && c==0x0a) pgmcomment=0;
      if (pgmw && pgmh && !pgmc && pgmn==4) IMG_DET(IMAGE1,pgm-2,i-pgm+3,(pgmw+7)/8,pgmh);
      if (pgmw && pgmh && pgmc && (pgmn==5 || (pgmn==7 && pamd==1 && pamatr==6))) IMG_DET(IMAGE8GRAY,pgm-2,i-pgm+3,pgmw,pgmh);
      if (pgmw && pgmh && pgmc && (pgmn==6 || (pgmn==7 && pamd==3 && pamatr==6))) IMG_DET(IMAGE24,pgm-2,i-pgm+3,pgmw*3,pgmh);
      if (pgmw && pgmh && pgmc && (pgmn==7 && pamd==4 && pamatr==6)) IMG_DET(IMAGE32,pgm-2,i-pgm+3,pgmw*4,pgmh);
    }

    // Detect .rgb image
    if ((buf0&0xffff)==0x01da) rgbi=i,rgbx=rgby=0;
    if (rgbi) {
      const int p=i-rgbi;
      if (p==1 && c!=0) rgbi=0;
      else if (p==2 && c!=1) rgbi=0;
      else if (p==4 && (buf0&0xffff)!=1 && (buf0&0xffff)!=2 && (buf0&0xffff)!=3) rgbi=0;
      else if (p==6) rgbx=buf0&0xffff,rgbi=(rgbx==0?0:rgbi);
      else if (p==8) rgby=buf0&0xffff,rgbi=(rgby==0?0:rgbi);
      else if (p==10) {
        int z=buf0&0xffff;
        if (rgbx && rgby && (z==1 || z==3 || z==4)) IMG_DET(IMAGE8,rgbi-1,512,rgbx,rgby*z);
        rgbi=0;
      }
    }

    // Detect .tiff file header (2/8/24 bit color, not compressed).
    if (buf1==0x49492a00 && n>i+(int)bswap(buf0)) {
      U64 savedpos=ftello(in);
      fseeko(in, start+i+ (U64)bswap(buf0)-7, SEEK_SET);

      // read directory
      int dirsize=getc(in);
      int tifx=0,tify=0,tifz=0,tifzb=0,tifc=0,tifofs=0,tifofval=0,b[12];
      if (getc(in)==0) {
        for (int i=0; i<dirsize; i++) {
          for (int j=0; j<12; j++) b[j]=getc(in);
          if (b[11]==EOF) break;
          int tag=b[0]+(b[1]<<8);
          int tagfmt=b[2]+(b[3]<<8);
          int taglen=b[4]+(b[5]<<8)+(b[6]<<16)+(b[7]<<24);
          int tagval=b[8]+(b[9]<<8)+(b[10]<<16)+(b[11]<<24);
          if (tagfmt==3||tagfmt==4) {
            if (tag==256) tifx=tagval;
            else if (tag==257) tify=tagval;
            else if (tag==258) tifzb=taglen==1?tagval:8; // bits per component
            else if (tag==259) tifc=tagval; // 1 = no compression
            else if (tag==273 && tagfmt==4) tifofs=tagval,tifofval=(taglen<=1);
            else if (tag==277) tifz=tagval; // components per pixel
          }
        }
      }
      if (tifx && tify && tifzb && (tifz==1 || tifz==3) && (tifc==1) && (tifofs && tifofs+i<n)) {
        if (!tifofval) {
          fseeko(in, start+i+tifofs-7, SEEK_SET);
          for (int j=0; j<4; j++) b[j]=getc(in);
          tifofs=b[0]+(b[1]<<8)+(b[2]<<16)+(b[3]<<24);
        }
        if (tifofs && tifofs<(1<<18) && tifofs+i<n) {
          if (tifz==1 && tifzb==1) IMG_DET(IMAGE1,i-7,tifofs,((tifx-1)>>3)+1,tify);
          else if (tifz==1 && tifzb==8) IMG_DET(IMAGE8,i-7,tifofs,tifx,tify);
          else if (tifz==3 && tifzb==8) IMG_DET(IMAGE24,i-7,tifofs,tifx*3,tify);
        }
      }
      fseeko(in, savedpos, SEEK_SET);
    }

    // Detect .tga image (8-bit 256 colors or 24-bit uncompressed)
    if ((buf1&0xFFFFFF)==0x00010100 && (buf0&0xFFFFFFC7)==0x00000100 && (c==16 || c==24 || c==32)) tga=i,tgax=tgay,tgaz=8,tgat=1,tgaid=buf1>>24,tgamap=c/8;
    else if ((buf1&0xFFFFFF)==0x00000200 && buf0==0x00000000) tga=i,tgax=tgay,tgaz=24,tgat=2;
    else if ((buf1&0xFFFFFF)==0x00000300 && buf0==0x00000000) tga=i,tgax=tgay,tgaz=8,tgat=3;
    if (tga) {
      if (i-tga==8) tga=(buf1==0?tga:0),tgax=(bswap(buf0)&0xffff),tgay=(bswap(buf0)>>16);
      else if (i-tga==10) {
        if ((buf0&0xFFF7)==32<<8)
          tgaz=32;
        if ((tgaz<<8)==(int)(buf0&0xFFD7) && tgax && tgay) {
          if (tgat==1){
            fseeko(in, start+tga+11, SEEK_SET);
            IMG_DET( (IsGrayscalePalette(in))?IMAGE8GRAY:IMAGE8,tga-7,18+tgaid+256*tgamap,tgax,tgay);
          }
          else if (tgat==2) IMG_DET((tgaz==24)?IMAGE24:IMAGE32,tga-7,18+tgaid,tgax*(tgaz>>3),tgay);
          else if (tgat==3) IMG_DET(IMAGE8GRAY,tga-7,18+tgaid,tgax,tgay);
        }
        tga=0;
      }
    }

    // Detect .gif
    if (type==DEFAULT && dett==GIF && i==0) {
      dett=DEFAULT;
      if (c==0x2c || c==0x21) gif=2,gifi=2;
    }
    if (!gif && (buf1&0xffff)==0x4749 && (buf0==0x46383961 || buf0==0x46383761)) gif=1,gifi=i+5;
    if (gif) {
      if (gif==1 && i==gifi) gif=2, gifi = i+5+(gifplt=(c&128)?(3*(2<<(c&7))):0);
      if (gif==2 && gifplt && i==gifi-gifplt-2) gifgray = IsGrayscalePalette(in, gifplt/3), gifplt = 0;
      if (gif==2 && i==gifi) {
        if ((buf0&0xff0000)==0x210000) gif=5,gifi=i;
        else if ((buf0&0xff0000)==0x2c0000) gif=3,gifi=i;
        else gif=0;
      }
      if (gif==3 && i==gifi+6) gifw=(bswap(buf0)&0xffff);
      if (gif==3 && i==gifi+7) gif=4,gifc=gifb=0,gifa=gifi=i+2+((c&128)?(3*(2<<(c&7))):0);
      if (gif==4 && i==gifi) {
        if (c>0 && gifb && gifc!=gifb) gifw=0;
        if (c>0) gifb=gifc,gifc=c,gifi+=c+1;
        else if (!gifw) gif=2,gifi=i+3;
        else return fseeko(in, start+gifa-1, SEEK_SET),detd=i-gifa+2,info=((gifgray?IMAGE8GRAY:IMAGE8)<<24)|gifw,dett=GIF;
      }
      if (gif==5 && i==gifi) {
        if (c>0) gifi+=c+1; else gif=2,gifi=i+3;
      }
    }

    // Detect EXE if the low order byte (little-endian) XX is more
    // recently seen (and within 4K) if a relative to absolute address
    // conversion is done in the context CALL/JMP (E8/E9) XX xx xx 00/FF
    // 4 times in a row.  Detect end of EXE at the last
    // place this happens when it does not happen for 64KB.

    if (((buf1&0xfe)==0xe8 || (buf1&0xfff0)==0x0f80) && ((buf0+1)&0xfe)==0) {
      int r=buf0>>24;  // relative address low 8 bits
      int a=((buf0>>24)+i)&0xff;  // absolute address low 8 bits
      int rdist=i-relpos[r];
      int adist=i-abspos[a];
      if (adist<rdist && adist<0x800 && abspos[a]>5) {
        e8e9last=i;
        ++e8e9count;
        if (e8e9pos==0 || e8e9pos>abspos[a]) e8e9pos=abspos[a];
      }
      else e8e9count=0;
      if (type==DEFAULT && e8e9count>=4 && e8e9pos>5)
        return fseeko(in, start+e8e9pos-5, SEEK_SET), EXE;
      abspos[a]=i;
      relpos[r]=i;
    }
    if (i-e8e9last>0x4000) {
      //TODO: Large file support
      if (type==EXE) return info=(int)start, fseeko(in, start+e8e9last, SEEK_SET), DEFAULT;
      e8e9count=e8e9pos=0;
    }

    // Detect base64 encoded data
    if (b64s==0 && buf0==0x73653634 && ((buf1&0xffffff)==0x206261 || (buf1&0xffffff)==0x204261)) b64s=1,b64i=i-6; //' base64' ' Base64'
    if (b64s==0 && ((buf1==0x3b626173 && buf0==0x6536342c) || (buf1==0x215b4344 && buf0==0x4154415b))) b64s=3,b64i=i+1; // ';base64,' '![CDATA['
    if (b64s>0) {
      if (b64s==1 && buf0==0x0d0a0d0a) b64s=((i-b64i>=128)?0:2),b64i=i+1,b64line=0;
      else if (b64s==2 && (buf0&0xffff)==0x0d0a && b64line==0) b64line=i+1-b64i,b64nl=i;
      else if (b64s==2 && (buf0&0xffff)==0x0d0a && b64line>0 && (buf0&0xffffff)!=0x3d0d0a) {
         if (i-b64nl<b64line && buf0!=0x0d0a0d0a) i-=1,b64s=5;
         else if (buf0==0x0d0a0d0a) i-=3,b64s=5;
         else if (i-b64nl==b64line) b64nl=i;
         else b64s=0;
      }
      else if (b64s==2 && (buf0&0xffffff)==0x3d0d0a) i-=1,b64s=5; // '=' or '=='
      else if (b64s==2 && !(isalnum(c) || c=='+' || c=='/' || c==10 || c==13 || c=='=')) b64s=0;
      if (b64line>0 && (b64line<=4 || b64line>255)) b64s=0;
      if (b64s==3 && i>=b64i && !(isalnum(c) || c=='+' || c=='/' || c=='=')) b64s=4;
      if ((b64s==4 && i-b64i>128) || (b64s==5 && i-b64i>512 && i-b64i<(1<<27))) return fseeko(in, start+b64i, SEEK_SET),detd=i-b64i,BASE64;
      if (b64s>3) b64s=0;
    }
  }
  return type;
}


typedef enum {FDECOMPRESS, FCOMPARE, FDISCARD} FMode;

void encode_cd(FILE* in, FILE* out, U64 len, int info) {
  const int BLOCK=2352;
  U8 blk[BLOCK];
  fputc((len%BLOCK)>>8,out);
  fputc(len%BLOCK,out);
  for (int offset=0; offset<len; offset+=BLOCK) {
    if (offset+BLOCK > len) {
      fread(&blk[0], 1, len-offset, in);
      fwrite(&blk[0], 1, len-offset, out);
    } else {
      fread(&blk[0], 1, BLOCK, in);
      if (info==3) blk[15]=3;
      if (offset==0) fwrite(&blk[12], 1, 4+4*(blk[15]!=1), out);
      fwrite(&blk[16+8*(blk[15]!=1)], 1, 2048+276*(info==3), out);
      if (offset+BLOCK*2 > len && blk[15]!=1) fwrite(&blk[16], 1, 4, out);
    }
  }
}

U64 decode_cd(FILE *in, U64 size, FILE *out, FMode mode, U64 &diffFound) {
  const int BLOCK=2352;
  U8 blk[BLOCK];
  U64 i=0, i2=0;
  int a=-1, bsize=0, q=fgetc(in);
  q=(q<<8)+fgetc(in);
  size-=2;
  while (i<size) {
    if (size-i==q) {
      fread(blk, q, 1, in);
      fwrite(blk, q, 1, out);
      i+=q;
      i2+=q;
    } else if (i==0) {
      fread(blk+12, 4, 1, in);
      if (blk[15]!=1) fread(blk+16, 4, 1, in);
      bsize=2048+(blk[15]==3)*276;
      i+=4*(blk[15]!=1)+4;
    } else {
      a=(blk[12]<<16)+(blk[13]<<8)+blk[14];
    }
    fread(blk+16+(blk[15]!=1)*8, bsize, 1, in);
    i+=bsize;
    if (bsize>2048) blk[15]=3;
    if (blk[15]!=1 && size-q-i==4) {
      fread(blk+16, 4, 1, in);
      i+=4;
    }
    expand_cd_sector(blk, a, 0);
    if (mode==FDECOMPRESS) fwrite(blk, BLOCK, 1, out);
    else if (mode==FCOMPARE) for (int j=0; j<BLOCK; ++j) if (blk[j]!=getc(out) && !diffFound) diffFound=i2+j+1;
    i2+=BLOCK;
  }
  return i2;
}


// 24-bit image data transform:
// simple color transform (b, g, r) -> (g, g-r, g-b)

void encode_bmp(FILE* in, FILE* out, U64 len, int width) {
  int r,g,b;
  for (int i=0; i<(int)(len/width); i++) {
    for (int j=0; j<width/3; j++) {
      b=fgetc(in), g=fgetc(in), r=fgetc(in);
      fputc(g, out);
      fputc(g-r, out);
      fputc(g-b, out);
    }
    for (int j=0; j<width%3; j++) fputc(fgetc(in), out);
  }
}

U64 decode_bmp(Encoder& en, U64 size, int width, FILE *out, FMode mode, U64 &diffFound) {
  int r,g,b,p;
  for (int i=0; i<(int)(size/width); i++) {
    p=i*width;
    for (int j=0; j<width/3; j++) {
      b=en.decompress(), g=en.decompress(), r=en.decompress();
      if (mode==FDECOMPRESS) {
        fputc(b-r, out);
        fputc(b, out);
        fputc(b-g, out);
        if (!j && !(i&0xf)) en.print_status();
      }
      else if (mode==FCOMPARE) {
        if (((b-r)&255)!=getc(out) && !diffFound) diffFound=p+1;
        if (b!=getc(out) && !diffFound) diffFound=p+2;
        if (((b-g)&255)!=getc(out) && !diffFound) diffFound=p+3;
        p+=3;
      }
    }
    for (int j=0; j<width%3; j++) {
      if (mode==FDECOMPRESS) {
        fputc(en.decompress(), out);
      }
      else if (mode==FCOMPARE) {
        if (en.decompress()!=getc(out) && !diffFound) diffFound=p+j+1;
      }
    }
  }
  return size;
}

// 32-bit image
void encode_im32(FILE* in, FILE* out, U64 len, int width) {
  int r,g,b,a;
  for (int i=0; i<(int)(len/width); i++) {
    for (int j=0; j<width/4; j++) {
      b=fgetc(in), g=fgetc(in), r=fgetc(in); a=fgetc(in);
      fputc(g, out);
      fputc(g-r, out);
      fputc(g-b, out);
      fputc(a, out);
    }
    for (int j=0; j<width%4; j++) fputc(fgetc(in), out);
  }
}

U64 decode_im32(Encoder& en, U64 size, int width, FILE *out, FMode mode, U64 &diffFound) {
  int r,g,b,a,p;
  bool rgb = (width&(1<<31))>0;
  if (rgb) width^=(1<<31);
  for (int i=0; i<(int)(size/width); i++) {
    p=i*width;
    for (int j=0; j<width/4; j++) {
      b=en.decompress(), g=en.decompress(), r=en.decompress(), a=en.decompress();
      if (mode==FDECOMPRESS) {
        fputc(b-r, out); fputc(b, out); fputc(b-g, out); fputc(a, out);
        if (!j && !(i&0xf)) en.print_status();
      }
      else if (mode==FCOMPARE) {
        if (((b-r)&255)!=getc(out) && !diffFound) diffFound=p+1;
        if (b!=getc(out) && !diffFound) diffFound=p+2;
        if (((b-g)&255)!=getc(out) && !diffFound) diffFound=p+3;
        if (((a)&255)!=getc(out) && !diffFound) diffFound=p+4;
        p+=4;
      }
    }
    for (int j=0; j<width%4; j++) {
      if (mode==FDECOMPRESS) {
        fputc(en.decompress(), out);
      }
      else if (mode==FCOMPARE) {
        if (en.decompress()!=getc(out) && !diffFound) diffFound=p+j+1;
      }
    }
  }
  return size;
}

// EXE transform: <encoded-size> <begin> <block>...
// Encoded-size is 4 bytes, MSB first.
// begin is the offset of the start of the input file, 4 bytes, MSB first.
// Each block applies the e8e9 transform to strings falling entirely
// within the block starting from the end and working backwards.
// The 5 byte pattern is E8/E9 xx xx xx 00/FF (x86 CALL/JMP xxxxxxxx)
// where xxxxxxxx is a relative address LSB first.  The address is
// converted to an absolute address by adding the offset mod 2^25
// (in range +-2^24).

void encode_exe(FILE* in, FILE* out, U64 len, U64 begin) {
  const int BLOCK=0x10000;
  Array<U8> blk(BLOCK);
  //TODO: Large file support
  fprintf(out, "%c%c%c%c", (int)(begin>>24)&0xFF, (int)(begin>>16)&0xFF, (int)(begin>>8)&0xFF, (int)begin&0xFF);

  // Transform
  for (U64 offset=0; offset<len; offset+=BLOCK) {
    U32 size=min(U32(len-offset), BLOCK);
    int bytesRead=(int)fread(&blk[0], 1, size, in);
    if (bytesRead!=(int)size) quit("encode_exe read error");
    for (int i=bytesRead-1; i>=5; --i) {
      if ((blk[i-4]==0xe8 || blk[i-4]==0xe9 || (blk[i-5]==0x0f && (blk[i-4]&0xf0)==0x80))
         && (blk[i]==0||blk[i]==0xff)) {
        int a=(blk[i-3]|blk[i-2]<<8|blk[i-1]<<16|blk[i]<<24)+(int)(offset+begin)+i+1;
        a<<=7;
        a>>=7;
        blk[i]=a>>24;
        blk[i-1]=a^176;
        blk[i-2]=(a>>8)^176;
        blk[i-3]=(a>>16)^176;
      }
    }
    fwrite(&blk[0], 1, bytesRead, out);
  }
}

U64 decode_exe(Encoder& en, U64 size, FILE *out, FMode mode, U64 &diffFound) {
  const int BLOCK=0x10000;  // block size
  int begin, offset=6, a;
  U8 c[6];
  begin=en.decompress()<<24;
  begin|=en.decompress()<<16;
  begin|=en.decompress()<<8;
  begin|=en.decompress();
  size-=4;
  for (int i=4; i>=0; i--) c[i]=en.decompress();  // Fill queue

  while (offset<(int)size+6) {
    memmove(c+1, c, 5);
    if (offset<=(int)size) c[0]=en.decompress();
    // E8E9 transform: E8/E9 xx xx xx 00/FF -> subtract location from x
    if ((c[0]==0x00 || c[0]==0xFF) && (c[4]==0xE8 || c[4]==0xE9 || (c[5]==0x0F && (c[4]&0xF0)==0x80))
     && (((offset-1)^(offset-6))&-BLOCK)==0 && offset<=(int)size) { // not crossing block boundary
      a=((c[1]^176)|(c[2]^176)<<8|(c[3]^176)<<16|c[0]<<24)-offset-begin;
      a<<=7;
      a>>=7;
      c[3]=a;
      c[2]=a>>8;
      c[1]=a>>16;
      c[0]=a>>24;
    }
    if (mode==FDECOMPRESS) putc(c[5], out);
    else if (mode==FCOMPARE && c[5]!=getc(out) && !diffFound) diffFound=offset-6+1;
    if (mode==FDECOMPRESS && !(offset&0xfff)) en.print_status();
    offset++;
  }
  return size;
}

class zLibMTF{
private:
  int Root, Index;
  Array<int, 16> Previous;
  Array<int, 16> Next;
public:
  zLibMTF(): Root(0), Index(0), Previous(81), Next(81) {
    for (int i=0;i<81;i++) {
      Previous[i] = i-1;
      Next[i] = i+1;
    }
    Next[80] = -1;
  }
  inline int GetFirst(){
    return Index=Root;
  }
  inline int GetNext(){
    if(Index>=0){Index=Next[Index];return Index;}
    return Index; //-1
  }
  inline void MoveToFront(int i){
    assert(i>=0 && i<=80);
    if ((Index=i)==Root) return;
    int p=Previous[Index];
    int n=Next[Index];
    if(p>=0)Next[p] = Next[Index];
    if(n>=0)Previous[n] = Previous[Index];
    Previous[Root] = Index;
    Next[Index] = Root;
    Root=Index;
    Previous[Root]=-1;
  }
} MTF;

int encode_zlib(FILE* in, FILE* out, U64 len, int &hdrsize) {
  const int BLOCK=1<<16, LIMIT=128;
  U8 zin[BLOCK*2],zout[BLOCK],zrec[BLOCK*2], diffByte[81*LIMIT];
  U64 diffPos[81*LIMIT];

  // Step 1 - parse offset type form zlib stream header
  U64 pos=ftello(in);
  U32 h1=fgetc(in), h2=fgetc(in);
  fseeko(in, pos, SEEK_SET);
  int zh=parse_zlib_header(h1*256+h2);
  int memlevel,clevel,window=zh==-1?0:MAX_WBITS+10+zh/4,ctype=zh%4;
  int minclevel=window==0?1:ctype==3?7:ctype==2?6:ctype==1?2:1;
  int maxclevel=window==0?9:ctype==3?9:ctype==2?6:ctype==1?5:1;
  int index=-1, found=0;

  // Step 2 - check recompressiblitiy, determine parameters and save differences
  z_stream main_strm, rec_strm[81];
  int diffCount[81], recpos[81], main_ret=Z_STREAM_END;
  main_strm.zalloc=Z_NULL; main_strm.zfree=Z_NULL; main_strm.opaque=Z_NULL;
  main_strm.next_in=Z_NULL; main_strm.avail_in=0;
  if (zlib_inflateInit(&main_strm,zh)!=Z_OK) return false;
  for (int i=0; i<81; i++) {
    clevel=(i/9)+1;
    // Early skip if invalid parameter
    if (clevel<minclevel || clevel>maxclevel){
      diffCount[i]=LIMIT;
      continue;
    }
    memlevel=(i%9)+1;
    rec_strm[i].zalloc=Z_NULL; rec_strm[i].zfree=Z_NULL; rec_strm[i].opaque=Z_NULL;
    rec_strm[i].next_in=Z_NULL; rec_strm[i].avail_in=0;
    int ret=deflateInit2(&rec_strm[i], clevel, Z_DEFLATED, window-MAX_WBITS, memlevel, Z_DEFAULT_STRATEGY);
    diffCount[i]=(ret==Z_OK)?0:LIMIT;
    recpos[i]=BLOCK*2;
    diffPos[i*LIMIT]=0xFFFFFFFFFFFFFFFF;
    diffByte[i*LIMIT]=0;
  }
  for (U64 i=0; i<len; i+=BLOCK) {
    U32 blsize=min(U32(len-i),BLOCK);
    for (int j=0; j<81; j++) {
      if (diffCount[j]>=LIMIT) continue;
      memmove(&zrec[0], &zrec[BLOCK], BLOCK);
      recpos[j]-=BLOCK;
    }
    memmove(&zin[0], &zin[BLOCK], BLOCK);
    fread(&zin[BLOCK], 1, blsize, in); // Read block from input file

    // Decompress/inflate block
    main_strm.next_in=&zin[BLOCK]; main_strm.avail_in=blsize;
    do {
      main_strm.next_out=&zout[0]; main_strm.avail_out=BLOCK;
      main_ret=inflate(&main_strm, Z_FINISH);

      // Recompress/deflate block with all possible parameters
      for (int j=MTF.GetFirst(); j>=0; j=MTF.GetNext()){
        if (diffCount[j]>=LIMIT) continue;
        rec_strm[j].next_in=&zout[0];  rec_strm[j].avail_in=BLOCK-main_strm.avail_out;
        rec_strm[j].next_out=&zrec[recpos[j]]; rec_strm[j].avail_out=BLOCK*2-recpos[j];
        int ret=deflate(&rec_strm[j], main_strm.total_in == len ? Z_FINISH : Z_NO_FLUSH);
        if (ret!=Z_BUF_ERROR && ret!=Z_STREAM_END && ret!=Z_OK) { diffCount[j]=LIMIT; continue; }

        // Compare
        int end=2*BLOCK-(int)rec_strm[j].avail_out;
        int tail=max(main_ret==Z_STREAM_END ? (int)len-(int)rec_strm[j].total_out : 0,0);
        for (int k=recpos[j]; k<end+tail; k++) {
          if ((k<end && i+k-BLOCK<len && zrec[k]!=zin[k]) || k>=end) {
            if (++diffCount[j]<LIMIT) {
              const int p=j*LIMIT+diffCount[j];
              diffPos[p]=i+k-BLOCK;
              assert(k < sizeof(zin)/sizeof(*zin));
              diffByte[p]=zin[k];
            }
          }
        }
        // Early break on perfect match
        if (main_ret==Z_STREAM_END && !diffCount[j]){
          index=j;
          found=1;
          break;
        }
        recpos[j]=2*BLOCK-rec_strm[j].avail_out;
      }
    } while (main_strm.avail_out==0 && main_ret==Z_BUF_ERROR);
    if (main_ret!=Z_BUF_ERROR && main_ret!=Z_STREAM_END) break;
  }
  int minCount=(found)?0:LIMIT;
  for (int i=80; i>=0; i--) {
    clevel=(i/9)+1;
    if (clevel>=minclevel && clevel<=maxclevel)
      deflateEnd(&rec_strm[i]);
    if (!found && diffCount[i]<minCount)
      minCount=diffCount[index=i];
  }
  inflateEnd(&main_strm);
  if (minCount==LIMIT) return false;
  MTF.MoveToFront(index);

  // Step 3 - write parameters, differences and precompressed (inflated) data
  fputc(diffCount[index], out);
  fputc(window, out);
  fputc(index, out);
  for (int i=0; i<=diffCount[index]; i++) {
    const int v=i==diffCount[index] ? int(len-diffPos[index*LIMIT+i])
                                    : int(diffPos[index*LIMIT+i+1]-diffPos[index*LIMIT+i])-1;
    fputc(v>>24, out); fputc(v>>16, out); fputc(v>>8, out); fputc(v, out);
  }
  for (int i=0; i<diffCount[index]; i++) fputc(diffByte[index*LIMIT+i+1], out);

  fseeko(in, pos, SEEK_SET);
  main_strm.zalloc=Z_NULL; main_strm.zfree=Z_NULL; main_strm.opaque=Z_NULL;
  main_strm.next_in=Z_NULL; main_strm.avail_in=0;
  if (zlib_inflateInit(&main_strm,zh)!=Z_OK) return false;
  for (U64 i=0; i<len; i+=BLOCK) {
    U32 blsize=min(U32(len-i),BLOCK);
    fread(&zin[0], 1, blsize, in);
    main_strm.next_in=&zin[0]; main_strm.avail_in=blsize;
    do {
      main_strm.next_out=&zout[0]; main_strm.avail_out=BLOCK;
      main_ret=inflate(&main_strm, Z_FINISH);
      fwrite(&zout[0], 1, BLOCK-main_strm.avail_out, out);
    } while (main_strm.avail_out==0 && main_ret==Z_BUF_ERROR);
    if (main_ret!=Z_BUF_ERROR && main_ret!=Z_STREAM_END) break;
  }
  hdrsize=diffCount[index]*5+7;
  return main_ret==Z_STREAM_END;
}

int decode_zlib(FILE* in, U64 size, FILE *out, FMode mode, U64 &diffFound) {
  const int BLOCK=1<<16, LIMIT=128;
  U8 zin[BLOCK],zout[BLOCK];
  int diffCount=min(fgetc(in),LIMIT-1);
  int window=fgetc(in)-MAX_WBITS;
  int index=fgetc(in);
  int memlevel=(index%9)+1;
  int clevel=(index/9)+1;
  int len=0;
  int diffPos[LIMIT];
  diffPos[0]=-1;
  for (int i=0; i<=diffCount; i++) {
    int v=fgetc(in)<<24; v|=fgetc(in)<<16; v|=fgetc(in)<<8; v|=fgetc(in);
    if (i==diffCount) len=v+diffPos[i]; else diffPos[i+1]=v+diffPos[i]+1;
  }
  U8 diffByte[LIMIT];
  diffByte[0]=0;
  for (int i=0; i<diffCount; i++) diffByte[i+1]=fgetc(in);
  size-=7+5*diffCount;

  z_stream rec_strm;
  int diffIndex=1,recpos=0;
  rec_strm.zalloc=Z_NULL; rec_strm.zfree=Z_NULL; rec_strm.opaque=Z_NULL;
  rec_strm.next_in=Z_NULL; rec_strm.avail_in=0;
  int ret=deflateInit2(&rec_strm, clevel, Z_DEFLATED, window, memlevel, Z_DEFAULT_STRATEGY);
  if (ret!=Z_OK) return 0;
  for (U64 i=0; i<size; i+=BLOCK) {
    U32 blsize=min(U32(size-i),BLOCK);
    fread(&zin[0], 1, blsize, in);
    rec_strm.next_in=&zin[0];  rec_strm.avail_in=blsize;
    do {
      rec_strm.next_out=&zout[0]; rec_strm.avail_out=BLOCK;
      ret=deflate(&rec_strm, i+blsize==size ? Z_FINISH : Z_NO_FLUSH);
      if (ret!=Z_BUF_ERROR && ret!=Z_STREAM_END && ret!=Z_OK) break;
      const int have=min(BLOCK-rec_strm.avail_out,len-recpos);
      while (diffIndex<=diffCount && diffPos[diffIndex]>=recpos && diffPos[diffIndex]<recpos+have) {
        zout[diffPos[diffIndex]-recpos]=diffByte[diffIndex];
        diffIndex++;
      }
      if (mode==FDECOMPRESS) fwrite(&zout[0], 1, have, out);
      else if (mode==FCOMPARE) for (int j=0; j<have; j++) if (zout[j]!=getc(out) && !diffFound) diffFound=recpos+j+1;
      recpos+=have;

    } while (rec_strm.avail_out==0);
  }
  while (diffIndex<=diffCount) {
    if (mode==FDECOMPRESS) fputc(diffByte[diffIndex], out);
    else if (mode==FCOMPARE) if (diffByte[diffIndex]!=getc(out) && !diffFound) diffFound=recpos+1;
    diffIndex++;
    recpos++;
  }
  deflateEnd(&rec_strm);
  return recpos==len ? len : 0;
}

//
// decode/encode base64
//
static const char  table1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
bool isbase64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/')|| (c == 10) || (c == 13));
}

U64 decode_base64(FILE *in, FILE *out, FMode mode, U64 &diffFound){
    U8 inn[3];
    int i,len1=0, len=0, blocksout = 0;
    int fle=0;
    int linesize=0;
    int outlen=0;
    int tlf=0;
    linesize=getc(in);
    outlen=getc(in);
    outlen+=(getc(in)<<8);
    outlen+=(getc(in)<<16);
    tlf=(getc(in));
    outlen+=((tlf&63)<<24);
    U8 *ptr,*fptr;
    ptr = (U8*)calloc((outlen>>2)*4+10, 1);
    if (!ptr) quit("Out of memory (d_B64)");
    fptr=&ptr[0];
    tlf=(tlf&192);
    if (tlf==128)
       tlf=10;        // LF: 10
    else if (tlf==64)
         tlf=13;        // LF: 13
    else
         tlf=0;

    while(fle<outlen){
        len=0;
        for(i=0;i<3;i++){
            inn[i] = getc( in );
            if(!feof(in)){
                len++;
                len1++;
            }
            else {
                inn[i] = 0;
            }
        }
        if(len){
            U8 in0,in1,in2;
            in0=inn[0],in1=inn[1],in2=inn[2];
            fptr[fle++]=(table1[in0>>2]);
            fptr[fle++]=(table1[((in0&0x03)<<4)|((in1&0xf0)>>4)]);
            fptr[fle++]=((len>1?table1[((in1&0x0f)<<2)|((in2&0xc0)>>6)]:'='));
            fptr[fle++]=((len>2?table1[in2&0x3f]:'='));
            blocksout++;
        }
        if(blocksout>=(linesize/4) && linesize!=0){ //no lf if linesize==0
            if( blocksout &&  !feof(in) && fle<=outlen) { //no lf if eof
                if (tlf) fptr[fle++]=(tlf);
                else fptr[fle++]=13,fptr[fle++]=10;
            }
            blocksout = 0;
        }
    }
    //Write out or compare
    if (mode==FDECOMPRESS){
      fwrite(&ptr[0], 1, outlen, out);
    }
    else if (mode==FCOMPARE){
      for(i=0;i<outlen;i++){
        U8 b=fptr[i];
        if (b!=fgetc(out) && !diffFound) diffFound=int(ftello(out));
      }
    }
    free(ptr);
    return outlen;
}

inline char valueb(char c){
  const char *p = strchr(table1, c);
  if (p) return (char)(p - table1);
  return 0;
}

void encode_base64(FILE* in, FILE* out, U64 len64) {
  int len=(int)len64;
  int in_len = 0;
  int i = 0;
  int j = 0;
  int b=0;
  int lfp=0;
  int tlf=0;
  char src[4];
  U8 *ptr,*fptr;
  int b64mem=(len>>2)*3+10;
    ptr = (U8*)calloc(b64mem, 1);
    if (!ptr) quit("Out of memory (e_B64)");
    fptr=&ptr[0];
    int olen=5;

  while (b=fgetc(in),in_len++ , ( b != '=') && isbase64(b) && in_len<=len) {
    if (b==13 || b==10) {
       if (lfp==0) lfp=in_len ,tlf=b;
       if (tlf!=b) tlf=0;
       continue;
    }
    src[i++] = b;
    if (i ==4){
          for (j = 0; j <4; j++) src[j] = valueb(src[j]);
          src[0] = (src[0] << 2) + ((src[1] & 0x30) >> 4);
          src[1] = ((src[1] & 0xf) << 4) + ((src[2] & 0x3c) >> 2);
          src[2] = ((src[2] & 0x3) << 6) + src[3];

          fptr[olen++]=src[0];
          fptr[olen++]=src[1];
          fptr[olen++]=src[2];
      i = 0;
    }
  }

  if (i){
    for (j=i;j<4;j++)
      src[j] = 0;

    for (j=0;j<4;j++)
      src[j] = valueb(src[j]);

    src[0] = (src[0] << 2) + ((src[1] & 0x30) >> 4);
    src[1] = ((src[1] & 0xf) << 4) + ((src[2] & 0x3c) >> 2);
    src[2] = ((src[2] & 0x3) << 6) + src[3];

    for (j=0;(j<i-1);j++) {
        fptr[olen++]=src[j];
    }
  }
  fptr[0]=lfp&255; //nl lenght
  fptr[1]=len&255;
  fptr[2]=(len>>8)&255;
  fptr[3]=(len>>16)&255;
  if (tlf!=0) {
    if (tlf==10) fptr[4]=128;
    else fptr[4]=64;
  }
  else
      fptr[4]=(len>>24)&63; //1100 0000
  fwrite(&ptr[0], 1, olen, out);
  free(ptr);
}

int encode_gif(FILE* in, FILE* out, U64 len, int &hdrsize) {
  int codesize=fgetc(in),diffpos=0,clearpos=0,bsize=0;
  U64 beginin=ftello(in),beginout=ftello(out);
  U8 output[4096];
  hdrsize=6;
  fputc(hdrsize>>8, out);
  fputc(hdrsize&255, out);
  fputc(bsize, out);
  fputc(clearpos>>8, out);
  fputc(clearpos&255, out);
  fputc(codesize, out);
  for (int phase=0; phase<2; phase++) {
    fseeko(in, beginin, SEEK_SET);
    int bits=codesize+1,shift=0,buffer=0;
    int blocksize=0,maxcode=(1<<codesize)+1,last=-1,dict[4096];
    bool end=false;
    while ((blocksize=fgetc(in))>0 && ftello(in)-beginin<len && !end) {
      for (int i=0; i<blocksize; i++) {
        buffer|=fgetc(in)<<shift;
        shift+=8;
        while (shift>=bits && !end) {
          int code=buffer&((1<<bits)-1);
          buffer>>=bits;
          shift-=bits;
          if (!bsize && code!=(1<<codesize)) {
            hdrsize+=4; fputc(0, out); fputc(0, out); fputc(0, out); fputc(0, out);
          }
          if (!bsize) bsize=blocksize;
          if (code==(1<<codesize)) {
            if (maxcode>(1<<codesize)+1) {
              if (clearpos && clearpos!=69631-maxcode) return 0;
              clearpos=69631-maxcode;
            }
            bits=codesize+1, maxcode=(1<<codesize)+1, last=-1;
          }
          else if (code==(1<<codesize)+1) end=true;
          else if (code>maxcode+1) return 0;
          else {
            int j=(code<=maxcode?code:last),size=1;
            while (j>=(1<<codesize)) {
              output[4096-(size++)]=dict[j]&255;
              j=dict[j]>>8;
            }
            output[4096-size]=j;
            if (phase==1) fwrite(&output[4096-size], 1, size, out); else diffpos+=size;
            if (code==maxcode+1) { if (phase==1) fputc(j, out); else diffpos++; }
            if (last!=-1) {
              if (++maxcode>=8191) return 0;
              if (maxcode<=4095)
              {
                dict[maxcode]=(last<<8)+j;
                if (phase==0) {
                  bool diff=false;
                  for (int m=(1<<codesize)+2;m<min(maxcode,4095);m++) if (dict[maxcode]==dict[m]) { diff=true; break; }
                  if (diff) {
                    hdrsize+=4;
                    j=diffpos-size-(code==maxcode);
                    fputc((j>>24)&255, out); fputc((j>>16)&255, out); fputc((j>>8)&255, out); fputc(j&255, out);
                    diffpos=size+(code==maxcode);
                  }
                }
              }
              if (maxcode>=((1<<bits)-1) && bits<12) bits++;
            }
            last=code;
          }
        }
      }
    }
  }
  diffpos=(int)ftello(out);
  fseeko(out, beginout, SEEK_SET);
  fputc(hdrsize>>8, out);
  fputc(hdrsize&255, out);
  fputc(255-bsize, out);
  fputc((clearpos>>8)&255, out);
  fputc(clearpos&255, out);
  fseeko(out, diffpos, SEEK_SET);
  return ftello(in)-beginin==len-1;
}

#define gif_write_block(count) { output[0]=(count);\
if (mode==FDECOMPRESS) fwrite(&output[0], 1, (count)+1, out);\
else if (mode==FCOMPARE) for (int j=0; j<(count)+1; j++) if (output[j]!=getc(out) && !diffFound) diffFound=outsize+j+1;\
outsize+=(count)+1; blocksize=0; }

#define gif_write_code(c) { buffer+=(c)<<shift; shift+=bits;\
while (shift>=8) { output[++blocksize]=buffer&255; buffer>>=8;shift-=8;\
if (blocksize==bsize) gif_write_block(bsize); }}

int decode_gif(FILE* in, U64 size, FILE *out, FMode mode, U64 &diffFound) {
  int diffcount=fgetc(in), curdiff=0, diffpos[4096];
  diffcount=((diffcount<<8)+fgetc(in)-6)/4;
  int bsize=255-fgetc(in);
  int clearpos=fgetc(in); clearpos=(clearpos<<8)+fgetc(in);
  clearpos=(69631-clearpos)&0xffff;
  int codesize=fgetc(in),bits=codesize+1,shift=0,buffer=0,blocksize=0;
  if (diffcount>4096 || clearpos<=(1<<codesize)+2) return 1;
  int maxcode=(1<<codesize)+1,dict[4096],input;
  for (int i=0; i<diffcount; i++) {
    diffpos[i]=fgetc(in);
    diffpos[i]=(diffpos[i]<<8)+fgetc(in);
    diffpos[i]=(diffpos[i]<<8)+fgetc(in);
    diffpos[i]=(diffpos[i]<<8)+fgetc(in);
    if (i>0) diffpos[i]+=diffpos[i-1];
  }
  U8 output[256];
  size-=6+diffcount*4;
  int last=fgetc(in),total=(int)size+1,outsize=1;
  if (mode==FDECOMPRESS) fputc(codesize, out);
  else if (mode==FCOMPARE) if (codesize!=getc(out) && !diffFound) diffFound=1;
  if (diffcount==0 || diffpos[0]!=0) gif_write_code(1<<codesize) else curdiff++;
  while (size-->=0 && (input=fgetc(in))>=0) {
    int code=-1, key=(last<<8)+input;
    for (int i=(1<<codesize)+2; i<=min(maxcode,4095); i++) if (dict[i]==key) code=i;
    if (curdiff<diffcount && total-(int)size>diffpos[curdiff]) curdiff++,code=-1;
    if (code==-1) {
      gif_write_code(last);
      if (maxcode==clearpos) { gif_write_code(1<<codesize); bits=codesize+1, maxcode=(1<<codesize)+1; }
      else
      {
        ++maxcode;
        if (maxcode<=4095) dict[maxcode]=key;
        if (maxcode>=(1<<bits) && bits<12) bits++;
      }
      code=input;
    }
    last=code;
  }
  gif_write_code(last);
  gif_write_code((1<<codesize)+1);
  if (shift>0) {
    output[++blocksize]=buffer&255;
    if (blocksize==bsize) gif_write_block(bsize);
  }
  if (blocksize>0) gif_write_block(blocksize);
  if (mode==FDECOMPRESS) fputc(0, out);
  else if (mode==FCOMPARE) if (0!=getc(out) && !diffFound) diffFound=outsize+1;
  return outsize+1;
}


//////////////////// Compress, Decompress ////////////////////////////

void direct_encode_block(Filetype type, FILE *in, U64 len, Encoder &en, int info=-1) {
  //TODO: Large file support
  en.compress(type);
  en.compress((len>>24)&0xFF);
  en.compress((len>>16)&0xFF);
  en.compress((len>>8)&0xFF);
  en.compress((len)&0xFF);
  if (info!=-1) {
    en.compress((info>>24)&0xFF);
    en.compress((info>>16)&0xFF);
    en.compress((info>>8)&0xFF);
    en.compress((info)&0xFF);
  }
  printf("Compressing... ");
  for (U64 j=0; j<len; ++j) {
    if (!(j&0xfff)) en.print_status(j, len);
    en.compress(getc(in));
  }
  printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
}

void compressRecursive(FILE *in, U64 blocksize, Encoder &en, char *blstr, int recursion_level=0, float p1=0.0, float p2=1.0);

U64 decode_func(Filetype type, Encoder &en, FILE *tmp, U64 len, int info, FILE *out, FMode mode, U64 &diffFound) {
  if (type==IMAGE24) return decode_bmp(en, len, info, out, mode, diffFound);
  else if (type==IMAGE32) return decode_im32(en, len, info, out, mode, diffFound);
  else if (type==EXE) return decode_exe(en, len, out, mode, diffFound);
  else if (type==CD) return decode_cd(tmp, len, out, mode, diffFound);
  else if (type==ZLIB) return decode_zlib(tmp, len, out, mode, diffFound);
  else if (type==BASE64) return decode_base64(tmp, out, mode, diffFound);
  else if (type==GIF) return decode_gif(tmp, len, out, mode, diffFound);
  else { assert(true); return 0; }
}

U64 encode_func(Filetype type, FILE *in, FILE *tmp, U64 len, int info, int &hdrsize) {
  if (type==IMAGE24) encode_bmp(in, tmp, len, info);
  else if (type==IMAGE32) encode_im32(in, tmp, len, info);
  else if (type==EXE) encode_exe(in, tmp, len, info);
  else if (type==CD) encode_cd(in, tmp, len, info);
  else if (type==ZLIB) return encode_zlib(in, tmp, len, hdrsize)?0:1;
  else if (type==BASE64) encode_base64(in, tmp, len);
  else if (type==GIF) return encode_gif(in, tmp, len, hdrsize)?0:1;
  else assert(true);
  return 0;
}

class TmpFile {
private:
  char tmp_file_name[L_tmpnam + 4];
public:
  FILE* Create(const int recursion_level) {
    //generate a temporary filename
    char *t1 = (char*)tmp_file_name;
    *t1++ = 't'; *t1++ = 'm'; *t1++ = 'p'; *t1++ = (char)('0' + recursion_level); //precede filename with "tmp" + recursion_level
    if(!tmpnam(t1))perror("tmpnam"),quit("Temporary file name gereration error.");
    //remove non alphanumeric characters from returned pathname+filename
    char *t2 = t1;
    while (*t1) {
      if (isalpha(*t1))
        *t2++ = 'a' + 'z' - tolower(*t1);
      else if (isdigit(*t1))
        *t2++ = '0' + '9' - *t1;
      t1++;
    }
    *t2 = 0; //end of string
    FILE* tmp = fopen((char*)tmp_file_name, "wb+");
    if(!tmp)quit("Temporary file creation error.");
    return tmp;
  }
  ~TmpFile() {
    remove(tmp_file_name);
  }
};

void transform_encode_block(Filetype type, FILE *in, U64 len, Encoder &en, int info, char *blstr, int recursion_level, float p1, float p2, U64 begin) {
  if (hasTransform(type)) {
    TmpFile TMP;
    FILE* tmp = TMP.Create(recursion_level);
    int hdrsize=0;
    U64 diffFound=encode_func(type, in, tmp, len, info, hdrsize);
    const U64 tmpsize=ftello(tmp);
    fseeko(tmp, tmpsize, SEEK_SET);
    if (!diffFound) {
      rewind(tmp);
      en.setFile(tmp);
      fseeko(in, begin, SEEK_SET);
      decode_func(type, en, tmp, tmpsize, info, in, FCOMPARE, diffFound);
    }
    // Test fails, compress without transform
    if (diffFound || fgetc(tmp)!=EOF) {
      printf("Transform fails at %I64u, skipping...\n", diffFound-1);
      fseeko(in, begin, SEEK_SET);
      direct_encode_block(DEFAULT, in, len, en);
    } else {
      rewind(tmp);
      if (hasRecursion(type)) {
        //TODO: Large file support
        en.compress(type);
        en.compress((tmpsize>>24)&0xFF);
        en.compress((tmpsize>>16)&0xFF);
        en.compress((tmpsize>>8)&0xFF);
        en.compress((tmpsize)&0xFF);
        Filetype type2=(Filetype)((info>>24)&0xFF);
        if (type2!=DEFAULT) {
          direct_encode_block(HDR, tmp, hdrsize, en);
          transform_encode_block(type2, tmp, tmpsize-hdrsize, en, info&0xffffff, blstr, recursion_level, p1, p2, hdrsize);
        } else {
          compressRecursive(tmp, tmpsize, en, blstr, recursion_level+1, p1, p2);
        }
      } else {
        direct_encode_block(type, tmp, tmpsize, en, hasInfo(type)?info:-1);
      }
    }
    fclose(tmp);
  } else {
    direct_encode_block(type, in, len, en, hasInfo(type)?info:-1);
  }
}

void compressRecursive(FILE *in, const U64 blocksize, Encoder &en, char *blstr, int recursion_level, float p1, float p2) {
  static const char* typenames[19]={"default", "jpeg", "hdr", "1b-image", "4b-image", "8b-image", "8b-img-grayscale",
    "24b-image", "32b-image", "audio", "exe", "cd", "zlib", "base64", "gif", "png-8b", "png-8b-grayscale", "png-24b", "png-32b"};
  static const char* audiotypes[4]={"8b mono", "8b stereo", "16b mono","16b stereo"};
  Filetype type=DEFAULT;
  int blnum=0, info;  // image width or audio type
  U64 begin=ftello(in);
  U64 block_end=begin+blocksize;
  char b2[32];
  strcpy(b2, blstr);
  if (b2[0]) strcat(b2, "-");
  if (recursion_level==5) {
    direct_encode_block(DEFAULT, in, blocksize, en);
    return;
  }
  float pscale=blocksize>0?(p2-p1)/blocksize:0;

  // Transform and test in blocks
  U64 bytes_to_go=blocksize;
  while (bytes_to_go>0) {
    Filetype nextType=detect(in, bytes_to_go, type, info);
    U64 detected_end=ftello(in);
    fseeko(in, begin, SEEK_SET);
    if (detected_end>block_end) {  // if a detection reports a larger size than the actual block size, fall back
      detected_end=begin+1;
      type=DEFAULT;
    }
    U64 len=detected_end-begin;
    if (len>0) {
      en.set_status_range(p1,p2=p1+pscale*len);
      sprintf(blstr,"%s%d",b2,blnum++);
      printf(" %-11s | %-16s |%10I64u bytes [%I64u - %I64u]",blstr,typenames[(type==ZLIB && (info>>24)>=PNG8)?info>>24:type],len,begin,detected_end-1);
      if (type==AUDIO) printf(" (%s)", audiotypes[info%4]);
      else if (type==IMAGE1 || type==IMAGE4 || type==IMAGE8 || type==IMAGE8GRAY || type==IMAGE24 || type==IMAGE32 || (type==ZLIB && (info>>24)>=PNG8)) printf(" (width: %d)", (type==ZLIB)?(info&0xFFFFFF):info);
      else if (hasRecursion(type) && (info>>24) > 0) printf(" (%s)",typenames[info>>24]);
      else if (type==CD) printf(" (m%d/f%d)", info==1?1:2, info!=3?1:2);
      printf("\n");
      transform_encode_block(type, in, len, en, info, blstr, recursion_level, p1, p2, begin);
      p1=p2;
      bytes_to_go-=len;
    }
    type=nextType;
    begin=detected_end;
  }
}

// Compress a file. Split filesize bytes into blocks by type.
// For each block, output
// <type> <size> and call encode_X to convert to type X.
// Test transform and compress.
void compress(const char* filename, U64 filesize, Encoder& en) {
  if(filesize>=(1u<<31))quit("Large files not yet supported.");
  assert(en.getMode()==COMPRESS);
  assert(filename && filename[0]);
  FILE *in=fopen(filename, "rb");
  if (!in) perror(filename), quit();
  U64 start=en.size();
  printf("Block segmentation:\n");
  char blstr[32]="";
  compressRecursive(in, filesize, en, blstr);
  if (in) fclose(in);
  printf("Compressed from %I64u to %I64u bytes.\n",filesize,en.size()-start);
}

// Try to make a directory, return true if successful
bool makedir(const char* dir) {
#ifdef WINDOWS
  return CreateDirectory(dir, 0)==TRUE;
#else
#ifdef UNIX
  return mkdir(dir, 0777)==0;
#else
  return false;
#endif
#endif
}

U64 decompressRecursive(FILE *out, U64 blocksize, Encoder& en, FMode mode, int recursion_level=0) {
  Filetype type;
  U64 len, i=0;
  U64 diffFound=0;
  int info=0;
  while (i<blocksize) {
    type=(Filetype)en.decompress();
    //TODO: Large file support
    len=en.decompress()<<24;
    len|=en.decompress()<<16;
    len|=en.decompress()<<8;
    len|=en.decompress();

    if (hasInfo(type)) {
      info=0; for (int i=0; i<4; ++i) { info<<=8; info+=en.decompress(); }
    }
    if (hasRecursion(type)) {
      TmpFile TMP;
      FILE* tmp = TMP.Create(recursion_level);
      decompressRecursive(tmp, len, en, FDECOMPRESS, recursion_level+1);
      if (mode!=FDISCARD) {
        rewind(tmp);
        if (hasTransform(type)) len=decode_func(type, en, tmp, len, info, out, mode, diffFound);
      }
      fclose(tmp);
    } else if (hasTransform(type)) {
      len=decode_func(type, en, NULL, len, info, out, mode, diffFound);
    } else {
      for (U64 j=0; j<len; ++j) {
        if (!(j&0xfff)) en.print_status();
        if (mode==FDECOMPRESS) putc(en.decompress(), out);
        else if (mode==FCOMPARE) {
          if (en.decompress()!=fgetc(out) && !diffFound) {
            mode=FDISCARD;
            diffFound=i+j+1;
          }
        } else en.decompress();
      }
    }
    i+=len;
  }
  return diffFound;
}

// Decompress a file
void decompress(const char* filename, U64 filesize, Encoder& en) {
  FMode mode=FDECOMPRESS;
  assert(en.getMode()==DECOMPRESS);
  assert(filename && filename[0]);

  // Test if output file exists.  If so, then compare.
  FILE* f=fopen(filename, "rb");
  if (f) mode=FCOMPARE,printf("Comparing");
  else {
    // Create file
    f=fopen(filename, "wb");
    if (!f) {  // Try creating directories in path and try again
      String path(filename);
      for (int i=0; path[i]; ++i) {
        if (path[i]=='/' || path[i]=='\\') {
          char savechar=path[i];
          path[i]=0;
          if (makedir(path.c_str()))
            printf("Created directory %s\n", path.c_str());
          path[i]=savechar;
        }
      }
      f=fopen(filename, "wb");
    }
    if (!f) mode=FDISCARD,printf("Skipping"); else printf("Extracting");
  }
  printf(" %s %I64u -> ", filename, filesize);

  // Decompress/Compare
  U64 r=decompressRecursive(f, filesize, en, mode);
  if (mode==FCOMPARE && !r && getc(f)!=EOF) printf("file is longer\n");
  else if (mode==FCOMPARE && r) printf("differ at %I64u\n",r-1);
  else if (mode==FCOMPARE) printf("identical\n");
  else printf("done   \n");
  if (f) fclose(f);
}

//////////////////////////// User Interface ////////////////////////////


// int expand(String& archive, String& s, const char* fname, int base) {
// Given file name fname, print its length and base name (beginning
// at fname+base) to archive in format "%ld\t%s\r\n" and append the
// full name (including path) to String s in format "%s\n".  If fname
// is a directory then substitute all of its regular files and recursively
// expand any subdirectories.  Base initially points to the first
// character after the last / in fname, but in subdirectories includes
// the path from the topmost directory.  Return the number of files
// whose names are appended to s and archive.

// Same as expand() except fname is an ordinary file
int putsize(String& archive, String& s, const char* fname, int base) {
  int result=0;
  FILE *f=fopen(fname, "rb");
  if (f) {
    fseeko(f, 0, SEEK_END);
    U64 len=ftello(f);
    if (len>=0) {
      static char blk[24];
      sprintf(blk, "%I64u\t", len);
      archive+=blk;
      archive+=(fname+base);
      archive+="\n";
      s+=fname;
      s+="\n";
      ++result;
    }
    fclose(f);
  }
  return result;
}

#ifdef WINDOWS

int expand(String& archive, String& s, const char* fname, int base) {
  int result=0;
  DWORD attr=GetFileAttributes(fname);
  if ((attr != 0xFFFFFFFF) && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
    WIN32_FIND_DATA ffd;
    String fdir(fname);
    fdir+="/*";
    HANDLE h=FindFirstFile(fdir.c_str(), &ffd);
    while (h!=INVALID_HANDLE_VALUE) {
      if (!equals(ffd.cFileName, ".") && !equals(ffd.cFileName, "..")) {
        String d(fname);
        d+="/";
        d+=ffd.cFileName;
        result+=expand(archive, s, d.c_str(), base);
      }
      if (FindNextFile(h, &ffd)!=TRUE) break;
    }
    FindClose(h);
  }
  else // ordinary file
    result=putsize(archive, s, fname, base);
  return result;
}

#else
#ifdef UNIX

int expand(String& archive, String& s, const char* fname, int base) {
  int result=0;
  struct stat sb;
  if (stat(fname, &sb)<0) return 0;

  // If a regular file and readable, get file size
  if (sb.st_mode & S_IFREG && sb.st_mode & 0400)
    result+=putsize(archive, s, fname, base);

  // If a directory with read and execute permission, traverse it
  else if (sb.st_mode & S_IFDIR && sb.st_mode & 0400 && sb.st_mode & 0100) {
    DIR *dirp=opendir(fname);
    if (!dirp) {
      perror("opendir");
      return result;
    }
    dirent *dp;
    while(errno=0, (dp=readdir(dirp))!=0) {
      if (!equals(dp->d_name, ".") && !equals(dp->d_name, "..")) {
        String d(fname);
        d+="/";
        d+=dp->d_name;
        result+=expand(archive, s, d.c_str(), base);
      }
    }
    if (errno) perror("readdir");
    closedir(dirp);
  }
  else printf("%s is not a readable file or directory\n", fname);
  return result;
}

#else  // Not WINDOWS or UNIX, ignore directories

int expand(String& archive, String& s, const char* fname, int base) {
  return putsize(archive, s, fname, base);
}

#endif
#endif


int main(int argc, char** argv) {
  bool pause=argc<=2;  // Pause when done?
  try {

    // Get option
    bool doExtract=false;  // -d option
    bool doList=false;  // -l option
    int l=0;
    if (argc>1 && argv[1][0]=='-' && argv[1][1] && (!argv[1][2] || (argv[1][2]=='[' && (l=(int)strlen(argv[1]))>3 && argv[1][l-1]==']'))) {
      if (argv[1][1]>='0' && argv[1][1]<='8'){
        level=argv[1][1]-'0';
        l-=2;
        while (l>2){
          switch (argv[1][l]&0xDF){
            case 'B': brute = true; break;
            case 'E': trainEXE = true; break;
            case 'T': trainTXT = true; break;
            default: printf("Invalid switch: %c\n",argv[1][l]); quit();
          }
          l--;
        }
      }
      else if ((argv[1][1]&0xDF)=='D' && !argv[1][2])
        doExtract=true;
      else if ((argv[1][1]&0xDF)=='L' && !argv[1][2])
        doList=true;
      else
        quit("Valid options are -0[switches] through -8[switches], -d, -l\n");
      --argc;
      ++argv;
      pause=false;
    }

    // Print help message
    if (argc<2) {
      printf(PROGNAME " archiver v" PROGVERSION " (C) " PROGYEAR ", Matt Mahoney et al.\n"
        "Free under GPL, http://www.gnu.org/licenses/gpl.txt\n\n"
#ifdef WINDOWS
        "To compress or extract, drop a file or folder on the "
        PROGNAME " icon.\n"
        "The output will be put in the same folder as the input.\n"
        "\n"
        "Or from a command window: "
#endif
        "To compress:\n"
        "  " PROGNAME " -level[switches] file               (compresses to file." PROGNAME ")\n"
        "  " PROGNAME " file                      (level -%d, pause when done)\n"
        "level: -0 = store, -1 -2 -3 = faster (uses 35, 48, 59 MB)\n"
        "-4 -5 -6 -7 -8 = smaller (uses 133, 233, 435, 837, 1643 MB)\n"
        "Optional switches:\nb = Brute-force detection of DEFLATE streams\ne = Pre-train x86/x64 model\nt = Pre-train main model with dictionary file (english.dic)\n"
        "Examples:\n" PROGNAME " -8 file\n" PROGNAME " -8[bet] file\n"
#if defined(WINDOWS) || defined (UNIX)
        "You may also compress directories.\n"
#endif
        "\n"
        "To extract or compare:\n"
        "  " PROGNAME " -d dir1/archive." PROGNAME "      (extract to dir1)\n"
        "  " PROGNAME " -d dir1/archive." PROGNAME " dir2 (extract to dir2)\n"
        "  " PROGNAME " archive." PROGNAME "              (extract, pause when done)\n"
        "\n"
        "To view contents: " PROGNAME " -l archive." PROGNAME "\n"
        "\n",
        DEFAULT_LEVEL);
      quit();
    }

    FILE* archive=0;  // compressed file
    U32 files=0;  // number of files to compress/decompress
    Array<const char*> fname(1);  // file names (resized to files)
    Array<U64> fsize(1);   // file lengths (resized to files)

    // Compress or decompress?  Get archive name
    Mode mode=COMPRESS;
    String archiveName(argv[1]);
    {
      const int prognamesize=(int)strlen(PROGNAME);
      const int arg1size=(int)strlen(argv[1]);
      if (arg1size>prognamesize+1 && argv[1][arg1size-prognamesize-1]=='.'
          && equals(PROGNAME, argv[1]+arg1size-prognamesize)) {
        mode=DECOMPRESS;
      }
      else if (doExtract || doList)
        mode=DECOMPRESS;
      else {
        archiveName+=".";
        archiveName+=PROGNAME;
      }
    }

    // Compress: write archive header, get file names and sizes
    String header_string;
    String filenames;
    if (mode==COMPRESS) {

      // Expand filenames to read later.  Write their base names and sizes
      // to archive.
      int i;
      for (i=1; i<argc; ++i) {
        String name(argv[i]);
        int len=name.size()-1;
        for (int j=0; j<=len; ++j)  // change \ to /
          if (name[j]=='\\') name[j]='/';
        while (len>0 && name[len-1]=='/')  // remove trailing /
          name[--len]=0;
        int base=len-1;
        while (base>=0 && name[base]!='/') --base;  // find last /
        ++base;
        if (base==0 && len>=2 && name[1]==':') base=2;  // chop "C:"
        int expanded=expand(header_string, filenames, name.c_str(), base);
        if (!expanded && (i>1||argc==2))
          printf("%s: not found, skipping...\n", name.c_str());
        files+=expanded;
      }

      // If there is at least one file to compress
      // then create the archive header.
      if (files<1) quit("Nothing to compress\n");
      archive=fopen(archiveName.c_str(), "wb+");
      if (!archive) perror(archiveName.c_str()), quit();
      fprintf(archive, PROGNAME "%c%c", 0, level|(trainEXE<<4)|(trainTXT<<5) );
      printf("Creating archive %s with %d file(s)...\n",
        archiveName.c_str(), files);
    }

    // Decompress: open archive for reading and store file names and sizes
    if (mode==DECOMPRESS) {
      archive=fopen(archiveName.c_str(), "rb+");
      if (!archive) perror(archiveName.c_str()), quit();

      // Check for proper format and get option
      String header;
      int len=(int)strlen(PROGNAME)+2, c, i=0;
      header.resize(len+1);
      while (i<len && (c=getc(archive))!=EOF) {
        header[i]=c;
        i++;
      }
      header[i]=0;
      if (strncmp(header.c_str(), PROGNAME "\0", strlen(PROGNAME)+1))
        printf("%s: not a %s file\n", archiveName.c_str(), PROGNAME), quit();
      level=header[(int)strlen(PROGNAME)+1];
      trainEXE = (level&0x10)!=0; trainTXT = (level&0x20)!=0; level&=0xF;
      if (level<0||level>8) level=DEFAULT_LEVEL;
    }

    // Set globals according to option
    assert(level>=0 && level<=8);
    buf.setsize(MEM*8);
    Encoder en(mode, archive);

    // Compress header
    if (mode==COMPRESS) {
      U64 len=header_string.size();
      printf("\nFile list (%I64u bytes)\n", len);
      assert(en.getMode()==COMPRESS);
      U64 start=en.size();
      en.compress(0); // block type 0
      //TODO: Large file support
      // block length
      en.compress((len>>24)&0xFF);
      en.compress((len>>16)&0xFF);
      en.compress((len>>8)&0xFF);
      en.compress((len)&0xFF);
      for (U32 i=0; i<(U32)len; i++) en.compress(header_string[i]);
      printf("Compressed from %I64u to %I64u bytes.\n",len,en.size()-start);
    }

    // Deompress header
    if (mode==DECOMPRESS) {
      if (en.decompress()!=0) printf("%s: header corrupted\n", archiveName.c_str()), quit();
      int len=0;
      len+=en.decompress()<<24;
      len+=en.decompress()<<16;
      len+=en.decompress()<<8;
      len+=en.decompress();
      header_string.resize(len);
      for (int i=0; i<len; i++) {
        header_string[i]=en.decompress();
        if (header_string[i]=='\n') files++;
      }
      if (doList) printf("File list of %s archive:\n%s", archiveName.c_str(), header_string.c_str());
    }

    // Fill fname[files], fsize[files] with input filenames and sizes
    fname.resize(files);
    fsize.resize(files);
    char* p=&header_string[0];
    char* q=&filenames[0];
    for (U32 i=0; i<files; ++i) {
      assert(p);
      fsize[i]=atol(p);
      assert(fsize[i]>=0);
      while (*p!='\t') ++p;
      *(p++)='\0';
      fname[i]=mode==COMPRESS?q:p;
      while (*p!='\n') ++p;
      *(p++)='\0';
      if (mode==COMPRESS) { while (*q!='\n') ++q; *(q++)='\0'; }
    }

    // Compress or decompress files
    assert(fname.size()==files);
    assert(fsize.size()==files);
    U64 total_size=0;  // sum of file sizes
    for (U32 i=0; i<files; ++i) total_size+=fsize[i];
    if (mode==COMPRESS) {
      for (U32 i=0; i<files; ++i) {
        printf("\n%d/%d  Filename: %s (%I64u bytes)\n", i+1, files, fname[i], fsize[i]);
        compress(fname[i], fsize[i], en);
      }
      en.flush();
      printf("\nTotal %I64u bytes compressed to %I64u bytes.\n", total_size, en.size());
    }

    // Decompress files to dir2: paq8px -d dir1/archive.paq8px dir2
    // If there is no dir2, then extract to dir1
    // If there is no dir1, then extract to .
    else if (!doList) {
      assert(argc>=2);
      String dir(argc>2?argv[2]:argv[1]);
      if (argc==2) {  // chop "/archive.paq8px"
        int i;
        for (i=dir.size()-2; i>=0; --i) {
          if (dir[i]=='/' || dir[i]=='\\') {
            dir[i]=0;
            break;
          }
          if (i==1 && dir[i]==':') {  // leave "C:"
            dir[i+1]=0;
            break;
          }
        }
        if (i==-1) dir=".";  // "/" not found
      }
      dir=dir.c_str();
      if (dir[0] && (dir.size()!=3 || dir[1]!=':')) dir+="/";
      for (U32 i=0; i<files; ++i) {
        String out(dir.c_str());
        out+=fname[i];
        decompress(out.c_str(), fsize[i], en);
      }
    }
    fclose(archive);
    if (!doList) programChecker.print();
  }
  catch(const char* s) {
    if (s) printf("%s\n", s);
  }
  if (pause) {
    printf("\nClose this window or press ENTER to continue...\n");
    getchar();
  }
  return 0;
}
