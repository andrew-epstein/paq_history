/* paq8pxd10 file compressor/archiver.  Release by Kaido Orav, Jun. 21, 2014

    Copyright (C) 2008 Matt Mahoney, Serge Osnach, Alexander Ratushnyak,
    Bill Pettis, Przemyslaw Skibinski, Matthew Fite, wowtiger, Andrew Paterson,
    Jan Ondrus, Andreas Morphis, Pavel L. Holoborodko, KZ., Simon Berger,
    Neill Corlett

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

- To install, put paq8pxd_v10.exe or a shortcut to it on your desktop.
- To compress a file or folder, drop it on the paq8pxd10 icon.
- To decompress, drop a .paq8pxd10 file on the icon.

A .paq8pxd10 extension is added for compression, removed for decompression.
The output will go in the same folder as the input.

While paq8pxd10 is working, a command window will appear and report
progress.  When it is done you can close the window by pressing
ENTER or clicking [X].


COMMAND LINE INTERFACE

- To install, put paq8pxd_v10.exe somewhere in your PATH.
- To compress:      paq8pxd_v10 [-N] file1 [file2...]
- To decompress:    paq8pxd_v10 [-d] file1.paq8pxd10 [dir2]
- To view contents: paq8pxd_v10 -l file1.paq8pxd10

The compressed output file is named by adding ".paq8pxd10" extension to
the first named file (file1.paq8pxd10).  Each file that exists will be
added to the archive and its name will be stored without a path.
The option -N specifies a compression level ranging from -0
(fastest) to -8 (smallest).  The default is -5.  If there is
no option and only one file, then the program will pause when
finished until you press the ENTER key (to support drag and drop).
If file1.paq8pxd10 exists then it is overwritten.

If the first named file ends in ".paq8pxd10" then it is assumed to be
an archive and the files within are extracted to the same directory
as the archive unless a different directory (dir2) is specified.
The -d option forces extraction even if there is not a ".paq8pxd10"
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

  paq8pxd_v10 -4 c:\tmp\foo bar

compresses foo and bar (if they exist) to c:\tmp\foo.paq8pxd10 at level 4.

  paq8pxd_v10 -d c:\tmp\foo.paq8pxd10 .

extracts foo and compares bar in the current directory.  If foo and bar
are directories then their contents are extracted/compared.

There are no commands to update an existing archive or to extract
part of an archive.  Files and archives larger than 2GB are not
supported (but might work on 64-bit machines, not tested).
File names with nonprintable characters are not supported (spaces
are OK).


TO COMPILE

There are 2 files: paq8pxd_v10.cpp (C++) and wrtpre.cpp (C++).
paq8pxd_v10.cpp recognizes the following compiler options:

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
1997) The program will still work but it will be slower. 


Recommended compiler commands and optimizations:

  MINGW g++:
    g++ paq8pxd_v10.cpp -DWINDOWS -Ofast -msse2 -s -march=pentium4 -o paq8pxd_v10.exe 

  UNIX/Linux (PC):
    g++ paq8pxd_v10.cpp -DUNIX -Ofast -msse2 -s -march=pentium4 -o paq8pxd_v10

  Non PC (e.g. PowerPC under MacOS X)
    g++ paq8pxd_v10.cpp -O2 -DUNIX -DNOASM -s -o paq8pxd_v10



ARCHIVE FILE FORMAT

An archive has the following format.  

  paq8pxd10 -N 
  segment size 
  segment offset
  \0 file list size
  compressed file list(
    size TAB filename CR LF
    size TAB filename CR LF
    ...)
  compressed binary data
  file segmentation data

-N is the option (-0 to -9), even if a default was used.

segment size is total size of file segmentation data in bytes
at segmnet offset after compressed binary data.

file segmentation data is full list of detected blocks:
type size info
type size info
type size 
type size info
.....

info is present if block type is image or audio.

Plain file names are stored without a path.  Files in compressed
directories are stored with path relative to the compressed directory
(using UNIX style forward slashes "/").  For example, given these files:

  123 C:\dir1\file1.txt
  456 C:\dir2\file2.txt

Then

  paq8pxd_v10 archive \dir1\file1.txt \dir2

will create archive.paq8pxd10 

The command:

  paq8pxd_v10 archive.paq8pxd10 C:\dir3

will create the files:

  C:\dir3\file1.txt
  C:\dir3\dir2\file2.txt

Decompression will fail if the first 10 bytes are not "paq8pxd10 -".  Sizes
are stored as decimal numbers.  CR, LF, TAB are ASCII codes
13, 10, 9 respectively.


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

paq8pxd_v10 uses a neural network to combine a large number of models.  The
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

- Image.  For uncompressed 8,24-bit color BMP, TIFF and TGA images.
  Contexts are the high order bits of the surrounding pixels and linear
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

paq8pxd_v10 uses preprocessing transforms on certain data types to improve
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
  
- BASE64: Decodes BASE64 encoded data and recursively transformed
  up to level 5. Input can be full stream or end-of-line coded.

- 24-bit images: 24-bit image data uses simple color transform
  (b, g, r) -> (g, g-r, g-b)
  
- CD: mode 1 and mode 2 form 1+2 - 2352 bytes

- TEXT: All detected text blocks are transformed using dynamic dictionary
  preprocessing (based on XWRT). If transformed block is larger from original
  then transform is skipped.
  


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


DIFFERENCES FROM PAQ8PXD_V9
finally fix jpeg error (from fp8)
AVX2 is compatible with SSE and non SIMD

*/

#define PROGNAME "paq8pxd10"  // Please change this if you change the program.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#define NDEBUG  // remove for debugging (turns on Array bound checks)
#include <assert.h>

#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#endif

#ifdef WINDOWS
#include <windows.h>
#endif

#ifndef DEFAULT_OPTION
#define DEFAULT_OPTION 5
#endif
#include <stdint.h>
#ifdef _MSC_VER

typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#endif
// 8, 16, 32 bit unsigned types (adjust as appropriate)
typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int   U32;

// min, max functions
#ifndef WINDOWS
inline int min(int a, int b) {return a<b?a:b;}
inline int max(int a, int b) {return a<b?b:a;}
#endif

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
class ProgramChecker {
  int memused;  // bytes allocated by Array<T> now
  int maxmem;   // most bytes allocated ever
  clock_t start_time;  // in ticks
public:
  void alloc(int n) {  // report memory allocated, may be negative
    memused+=n;
    if (memused>maxmem) maxmem=memused;
  }
  ProgramChecker(): memused(0), maxmem(0) {
    start_time=clock();
    assert(sizeof(U8)==1);
    assert(sizeof(U16)==2);
    assert(sizeof(U32)==4);
    assert(sizeof(short)==2);
    assert(sizeof(int)==4);
  }
  void print() const {  // print time and memory used
    printf("Time %1.2f sec, used %d bytes of memory\n",
      double(clock()-start_time)/CLOCKS_PER_SEC, maxmem);
  }
} programChecker;

//////////////////////////// Array ////////////////////////////

// Array<T, ALIGN> a(n); creates n elements of T initialized to 0 bits.
// Constructors for T are not called.
// Indexing is bounds checked if assertions are on.
// a.size() returns n.
// a.resize(n) changes size to n, padding with 0 bits or truncating.
// a.push_back(x) appends x and increases size by 1, reserving up to size*2.
// a.pop_back() decreases size by 1, does not free memory.
// Copy and assignment are not supported.
// Memory is aligned on a ALIGN byte boundary (power of 2), default is none.

template <class T, int ALIGN=0> class Array {
private:
  int n;     // user size
  int reserved;  // actual size
  char *ptr; // allocated memory, zeroed
  T* data;   // start of n elements of aligned data
  void create(int i);  // create with size i
public:
  explicit Array(int i=0) {create(i);}
  ~Array();
  T& operator[](int i) {
#ifndef NDEBUG
    if (i<0 || i>=n) fprintf(stderr, "%d out of bounds %d\n", i, n), quit();
#endif
    return data[i];
  }
  const T& operator[](int i) const {
#ifndef NDEBUG
    if (i<0 || i>=n) fprintf(stderr, "%d out of bounds %d\n", i, n), quit();
#endif
    return data[i];
  }
  int size() const {return n;}
  void resize(int i);  // change size to i
  void pop_back() {if (n>0) --n;}  // decrement size
  void push_back(const T& x);  // increment size, append x
private:
  Array(const Array&);  // no copy or assignment
  Array& operator=(const Array&);
};

template<class T, int ALIGN> void Array<T, ALIGN>::resize(int i) {
  if (i<=reserved) {
    n=i;
    return;
  }
  char *saveptr=ptr;
  T *savedata=data;
  int saven=n;
  create(i);
  if (saveptr) {
    if (savedata) {
      memcpy(data, savedata, sizeof(T)*min(i, saven));
      programChecker.alloc(-ALIGN-n*sizeof(T));
    }
    free(saveptr);
  }
}

template<class T, int ALIGN> void Array<T, ALIGN>::create(int i) {
  n=reserved=i;
  if (i<=0) {
    data=0;
    ptr=0;
    return;
  }
  const int sz=ALIGN+n*sizeof(T);
  programChecker.alloc(sz);
  ptr = (char*)calloc(sz, 1);
  if (!ptr) quit("Out of memory");
  data = (ALIGN ? (T*)(ptr+ALIGN-(((long)ptr)&(ALIGN-1))) : (T*)ptr);
  assert((char*)data>=ptr && (char*)data<=ptr+ALIGN);
}

template<class T, int ALIGN> Array<T, ALIGN>::~Array() {
  programChecker.alloc(-ALIGN-n*sizeof(T));
  free(ptr);
}

template<class T, int ALIGN> void Array<T, ALIGN>::push_back(const T& x) {
  if (n==reserved) {
    int saven=n;
    resize(max(1, n*2));
    n=saven;
  }
  data[n++]=x;
}

/////////////////////////// String /////////////////////////////

// A tiny subset of std::string
// size() includes NUL terminator.

class String: public Array<char> {
public:
  const char* c_str() const {return &(*this)[0];}
  void operator=(const char* s) {
    resize(strlen(s)+1);
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

int pos;  // Number of input bytes in buf (is wrapped)
int poswr; //wrapp
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
  int operator()(int i) const {
    assert(i>0);
    return b[(pos-i)&(b.size()-1)];
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

// Buffer for file segment info 
// type size info(if not -1)
class Segment {
  Array<U8> b;
public:
	int pos;  //size of buffer
	uint64_t hpos; //header pos points to segment info at archive end
	int count; //count of segments
  Segment(int i=0): b(i),pos(0),hpos(0),count(0) {}
  void setsize(int i) {
    if (!i) return;
    assert(i>0);
    b.resize(i);
  }
  U8& operator[](int i) {
  	if (i>=b.size()) setsize(i+1);
    return b[i];
  }
  int operator()(int i) const {
    assert(i>=0);
    return b[i];
  }
  /*int size() const {
    return b.size();
  }*/
};

#ifdef _MSC_VER  // Microsoft C++
#define fseeko(a,b,c) _fseeki64(a,b,c)
#define ftello(a) _ftelli64(a)
#else
#ifndef unix
#ifndef fseeko
#define fseeko(a,b,c) fseeko64(a,b,c)
#endif
#ifndef ftello
#define ftello(a) ftello64(a)
#endif
#endif
#endif
// 
// 

/////////////////////// Global context /////////////////////////

int level=DEFAULT_OPTION;  // Compression level 0 to 8
#define MEM (0x10000<<level)
int y=0;  // Last bit, 0 or 1, set by encoder
Segment segment; //for file segments type size info(if not -1)
// Global context set by Predictor and available to all models.
int c0=1; // Last 0-7 bits of the partial byte with a leading 1 bit (1-255)
U32 c4=0; // Last 4 whole bytes, packed.  Last byte is bits 0-7.
int bpos=0; // bits in c0 (0 to 7)
Buf buf;  // Rotating input queue set by Predictor
int blpos=0; // Relative position in block
typedef enum {DEFAULT, JPEG, HDR, IMAGE1,IMAGE4, IMAGE8, IMAGE24, AUDIO, EXE, CD, TEXT, TXTUTF8, BASE64, DICTTXT} Filetype;
Filetype filetype=DEFAULT;
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
/*int squash(int d) {
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
*/
class Squash {
  Array<U16> t;
public:
  Squash();
  int operator()(int p) const {
    if (p>2047) return 4095;
  if (p<-2047) return 0;
    return t[p+2048];
  }
} squash;

Squash::Squash(): t(4096) {
                  int ts[33]={
    1,2,3,6,10,16,27,45,73,120,194,310,488,747,1101,
    1546,2047,2549,2994,3348,3607,3785,3901,3975,4022,
    4050,4068,4079,4085,4089,4092,4093,4094};
    int w,d;
  for (int i=-2047; i<=2047; ++i){
    w=i&127;
  d=(i>>7)+16;
  t[i+2048]=(ts[d]*(128-w)+ts[(d+1)]*w+64) >> 7;
    }
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

// dot_product returns dot product t*w of n elements.  n is rounded
// up to a multiple of 8.  Result is scaled down by 8 bits.
/*#ifdef NOASM  // no assembly language
int dot_product(short *t, short *w, int n) {
  int sum=0;
  n=(n+7)&-8;
  for (int i=0; i<n; i+=2)
    sum+=(t[i]*w[i]+t[i+1]*w[i+1]) >> 8;
  return sum;
}
#else  // The NASM version uses MMX and is about 8 times faster.
extern "C" int dot_product(short *t, short *w, int n);  // in NASM
#endif

// Train neural network weights w[n] given inputs t[n] and err.
// w[i] += t[i]*err, i=0..n-1.  t, w, err are signed 16 bits (+- 32K).
// err is scaled 16 bits (representing +- 1/2).  w[i] is clamped to +- 32K
// and rounded.  n is rounded up to a multiple of 8.
#ifdef NOASM
void train(short *t, short *w, int n, int err) {
  n=(n+7)&-8;
  for (int i=0; i<n; ++i) {
    int wt=w[i]+(((t[i]*err*2>>16)+1)>>1);
    if (wt<-32768) wt=-32768;
    if (wt>32767) wt=32767;
    w[i]=wt;
  }
}
#else
extern "C" void train(short *t, short *w, int n, int err);  // in NASM
#endif
*/

#if !defined(__GNUC__)

#if (2 == _M_IX86_FP) // 2 if /arch:SSE2 was used.
# define __SSE2__
#elif (1 == _M_IX86_FP) // 1 if /arch:SSE was used.
# define __SSE__
#endif

#endif /* __GNUC__ */

/**
 * Vector product a*b of n signed words, returning signed integer scaled down by 8 bits.
 * n is rounded up to a multiple of 8.
 */
static int dot_product (const short* const t, const short* const w, int n);

/**
 * Train n neural network weights w[n] on inputs t[n] and err.
 * w[i] += ((t[i]*2*err)+(1<<16))>>17 bounded to +- 32K.
 * n is rounded up to a multiple of 8.
 */
static void train (const short* const t, short* const w, int n, const int e);
#if defined(__AVX2__) // fast
#include<immintrin.h>
#define OPTIMIZE "AVX2-"
static int dot_product (const short* const t, const short* const w, int n) {
  assert(n == ((n + 15) & -16));
  __m256i sum = _mm256_setzero_si256 ();
  while ((n -= 16) >= 0) { // Each loop sums 16 products
    __m256i tmp = _mm256_madd_epi16 (*(__m256i *) &t[n], *(__m256i *) &w[n]); // t[n] * w[n] + t[n+1] * w[n+1]
    tmp = _mm256_srai_epi32 (tmp, 8); //                                        (t[n] * w[n] + t[n+1] * w[n+1]) >> 8
    sum = _mm256_add_epi32 (sum, tmp); //                                sum += (t[n] * w[n] + t[n+1] * w[n+1]) >> 8
  } 
 // exctract high and low of sum and adds
  __m128i low = _mm_add_epi32 (_mm256_extracti128_si256(sum,0),_mm256_extracti128_si256(sum,1)); 
  low = _mm_add_epi32 (low, _mm_srli_si128 (low, 8)); // Add eight sums together ...
  low = _mm_add_epi32 (low, _mm_srli_si128 (low, 4));
  return _mm_cvtsi128_si32 (low); //                     ...  and scale back to integer
}

static void train (const short* const t, short* const w, int n, const int e) {
  assert(n == ((n + 15) & -16));
  if (e) {
    const __m256i one = _mm256_set1_epi16 (1);
    const __m256i err = _mm256_set1_epi16 (short(e));
    while ((n -= 16) >= 0) { // Each iteration adjusts 16 weights
      __m256i tmp = _mm256_adds_epi16 (*(__m256i *) &t[n], *(__m256i *) &t[n]); // t[n] * 2
      tmp = _mm256_mulhi_epi16 (tmp, err); //                                     (t[n] * 2 * err) >> 16
      tmp = _mm256_adds_epi16 (tmp, one); //                                     ((t[n] * 2 * err) >> 16) + 1
      tmp = _mm256_srai_epi16 (tmp, 1); //                                      (((t[n] * 2 * err) >> 16) + 1) >> 1
      tmp = _mm256_adds_epi16 (tmp, *(__m256i *) &w[n]); //                    ((((t[n] * 2 * err) >> 16) + 1) >> 1) + w[n]
      *(__m256i *) &w[n] = tmp; //                                          save the new eight weights, bounded to +- 32K
    }
  }
}

#elif defined(__SSE2__) // faster
#include <emmintrin.h>
#define OPTIMIZE "SSE2-"

static int dot_product (const short* const t, const short* const w, int n) {
  assert(n == ((n + 15) & -16));
  __m128i sum = _mm_setzero_si128 ();
  while ((n -= 8) >= 0) { // Each loop sums eight products
    __m128i tmp = _mm_madd_epi16 (*(__m128i *) &t[n], *(__m128i *) &w[n]); // t[n] * w[n] + t[n+1] * w[n+1]
    tmp = _mm_srai_epi32 (tmp, 8); //                                        (t[n] * w[n] + t[n+1] * w[n+1]) >> 8
    sum = _mm_add_epi32 (sum, tmp); //                                sum += (t[n] * w[n] + t[n+1] * w[n+1]) >> 8
  }
  sum = _mm_add_epi32 (sum, _mm_srli_si128 (sum, 8)); // Add eight sums together ...
  sum = _mm_add_epi32 (sum, _mm_srli_si128 (sum, 4));
  return _mm_cvtsi128_si32 (sum); //                     ...  and scale back to integer
}

static void train (const short* const t, short* const w, int n, const int e) {
  assert(n == ((n + 15) & -16));
  if (e) {
    const __m128i one = _mm_set1_epi16 (1);
    const __m128i err = _mm_set1_epi16 (short(e));
    while ((n -= 8) >= 0) { // Each iteration adjusts eight weights
      __m128i tmp = _mm_adds_epi16 (*(__m128i *) &t[n], *(__m128i *) &t[n]); // t[n] * 2
      tmp = _mm_mulhi_epi16 (tmp, err); //                                     (t[n] * 2 * err) >> 16
      tmp = _mm_adds_epi16 (tmp, one); //                                     ((t[n] * 2 * err) >> 16) + 1
      tmp = _mm_srai_epi16 (tmp, 1); //                                      (((t[n] * 2 * err) >> 16) + 1) >> 1
      tmp = _mm_adds_epi16 (tmp, *(__m128i *) &w[n]); //                    ((((t[n] * 2 * err) >> 16) + 1) >> 1) + w[n]
      *(__m128i *) &w[n] = tmp; //                                          save the new eight weights, bounded to +- 32K
    }
  }
}

#elif defined(__SSE__) // fast
#include <xmmintrin.h>
#define OPTIMIZE "SSE-"

static sint32 dot_product (const sint16* const t, const sint16* const w, sint32 n) {
  assert(n == ((n + 15) & -16));
  __m64 sum = _mm_setzero_si64 ();
  while ((n -= 8) >= 0) { // Each loop sums eight products
    __m64 tmp = _mm_madd_pi16 (*(__m64 *) &t[n], *(__m64 *) &w[n]); //   t[n] * w[n] + t[n+1] * w[n+1]
    tmp = _mm_srai_pi32 (tmp, 8); //                                    (t[n] * w[n] + t[n+1] * w[n+1]) >> 8
    sum = _mm_add_pi32 (sum, tmp); //                            sum += (t[n] * w[n] + t[n+1] * w[n+1]) >> 8

    tmp = _mm_madd_pi16 (*(__m64 *) &t[n + 4], *(__m64 *) &w[n + 4]); // t[n+4] * w[n+4] + t[n+5] * w[n+5]
    tmp = _mm_srai_pi32 (tmp, 8); //                                    (t[n+4] * w[n+4] + t[n+5] * w[n+5]) >> 8
    sum = _mm_add_pi32 (sum, tmp); //                            sum += (t[n+4] * w[n+4] + t[n+5] * w[n+5]) >> 8
  }
  sum = _mm_add_pi32 (sum, _mm_srli_si64 (sum, 32)); // Add eight sums together ...
  const sint32 retval = _mm_cvtsi64_si32 (sum); //                     ...  and scale back to integer
  _mm_empty(); // Empty the multimedia state
  return retval;
}

static void train (const sint16* const t, sint16* const w, sint32 n, const sint32 e) {
  assert(n == ((n + 15) & -16));
  if (e) {
    const __m64 one = _mm_set1_pi16 (1);
    const __m64 err = _mm_set1_pi16 (sint16(e));
    while ((n -= 8) >= 0) { // Each iteration adjusts eight weights
      __m64 tmp = _mm_adds_pi16 (*(__m64 *) &t[n], *(__m64 *) &t[n]); //   t[n] * 2
      tmp = _mm_mulhi_pi16 (tmp, err); //                                 (t[n] * 2 * err) >> 16
      tmp = _mm_adds_pi16 (tmp, one); //                                 ((t[n] * 2 * err) >> 16) + 1
      tmp = _mm_srai_pi16 (tmp, 1); //                                  (((t[n] * 2 * err) >> 16) + 1) >> 1
      tmp = _mm_adds_pi16 (tmp, *(__m64 *) &w[n]); //                  ((((t[n] * 2 * err) >> 16) + 1) >> 1) + w[n]
      *(__m64 *) &w[n] = tmp; //                                       save the new four weights, bounded to +- 32K

      tmp = _mm_adds_pi16 (*(__m64 *) &t[n + 4], *(__m64 *) &t[n + 4]); // t[n+4] * 2
      tmp = _mm_mulhi_pi16 (tmp, err); //                                 (t[n+4] * 2 * err) >> 16
      tmp = _mm_adds_pi16 (tmp, one); //                                 ((t[n+4] * 2 * err) >> 16) + 1
      tmp = _mm_srai_pi16 (tmp, 1); //                                  (((t[n+4] * 2 * err) >> 16) + 1) >> 1
      tmp = _mm_adds_pi16 (tmp, *(__m64 *) &w[n + 4]); //              ((((t[n+4] * 2 * err) >> 16) + 1) >> 1) + w[n]
      *(__m64 *) &w[n + 4] = tmp; //                                   save the new four weights, bounded to +- 32K
    }
    _mm_empty(); // Empty the multimedia state
  }
}
#else
// dot_product returns dot product t*w of n elements.  n is rounded
// up to a multiple of 8.  Result is scaled down by 8 bits.
int dot_product(short *t, short *w, int n) {
  int sum=0;
  n=(n+15)&-16;
  for (int i=0; i<n; i+=2)
    sum+=(t[i]*w[i]+t[i+1]*w[i+1]) >> 8;
  return sum;
}

// Train neural network weights w[n] given inputs t[n] and err.
// w[i] += t[i]*err, i=0..n-1.  t, w, err are signed 16 bits (+- 32K).
// err is scaled 16 bits (representing +- 1/2).  w[i] is clamped to +- 32K
// and rounded.  n is rounded up to a multiple of 8.

void train(short *t, short *w, int n, int err) {
  n=(n+15)&-16;
  for (int i=0; i<n; ++i) {
    int wt=w[i]+(((t[i]*err*2>>16)+1)>>1);
    if (wt<-32768) wt=-32768;
    if (wt>32767) wt=32767;
    w[i]=wt;
  }
}
#endif // slow!


class Mixer {
  const int N, M, S;   // max inputs, max contexts, max context sets
  Array<short, 32> tx; // N inputs from add()
  Array<short, 32> wx; // N*M weights
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
  
  void update2() {
    if(filetype==DICTTXT) train(&tx[0], &wx[0], nx, ((y<<12)-base)*3/2), nx=base=ncxt=0;
    else update();
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
    assert(cx>=0);
    assert(base+cx<M);
    cxt[ncxt++]=base+cx;
    base+=range;
  }
  // predict next bit
  int p() {
    while (nx&15) tx[nx++]=0;  // pad
    if (mp) {  // combine outputs
      mp->update2();
      for (int i=0; i<ncxt; ++i) {
         int dp=((dot_product(&tx[0], &wx[cxt[i]*N], nx)));//*7)>>8);
          if(filetype==DICTTXT) dp=(dp*9)>>9;  else dp=dp>>5;
          pr[i]=squash(dp);
          mp->add(dp);
      }
     if(filetype!=DICTTXT)  mp->set(0, 1);
      return mp->p();
    }
    else {  // S=1 context
    if(filetype!=DICTTXT)  return pr[0]=squash(dot_product(&tx[0], &wx[0], nx)>>8);
      int z=dot_product(&tx[0], &wx[0], nx);
	base=squash( (z*15) >>13);
	return squash(z>>9);
    }
  }
  ~Mixer();
};


Mixer::~Mixer() {
  delete mp;
}


Mixer::Mixer(int n, int m, int s, int w):
    N((n+15)&-16), M(m), S(s), tx(N), wx(N*M),
    cxt(S), ncxt(0), base(0), nx(0), pr(S), mp(0) {
  assert(n>0 && N>0 && (N&15)==0 && M>0);
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

static int dt[1024];  // i -> 16K/(i+3)

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
    t[i]=1<<31;
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
  Array<U8, 64> t; // elements
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
// - SmallStationaryContextMap.  0 <= cx < M/512.
//     The state is a 16-bit probability that is adjusted after each
//     prediction.  C=1.
// - ContextMap.  For large contexts, C >= 1.  Context need not be hashed.

// Predict to mixer m from bit history state s, using sm to map s to
// a probability.

inline int mix2(Mixer& m, int s, StateMap& sm) {
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
  return s>0;
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

// Context is looked up directly.  m=size is power of 2 in bytes.
// Context should be < m/512.  High bits are discarded.
class SmallStationaryContextMap {
  Array<U16> t;
  int cxt;
  U16 *cp;
public:
  SmallStationaryContextMap(int m): t(m/2), cxt(0) {
    assert((m/2&m/2-1)==0); // power of 2?
    for (int i=0; i<t.size(); ++i)
      t[i]=32768;
    cp=&t[0];
  }
  void set(U32 cx) {
    cxt=cx*256&(t.size()-256);
  }
  void mix(Mixer& m, int rate=7) {
    *cp += ((y<<16)-(*cp)+(1<<(rate-1))) >> rate;
    cp=&t[cxt+c0];
     m.add(stretch((*cp)>>4));
  }
};

 
inline int mix3(Mixer& m, int s, StateMap& sm) {
  int p1=sm.p(s);
  //int n0=-!nex(s,2);
  //int n1=-!nex(s,3);
  int st=stretch(p1)>>2;
  m.add(st);
  p1>>=4;
  int p0=255-p1;
  m.add(p1-p0);
 /// m.add(st*(n1-n0));
 // m.add((p1&n0)-(p0&n1));
  //m.add((p1&n1)-(p0&n0));
  return s>0;
}

// Medium ContextMap
class MContextMap {
  Array<U8, 64> t;
  Array<U8*>  cp;
  Array<U32> cxt;
  int cn;
  StateMap *sm;
public:
  MContextMap(size_t m,int c):t(m), cn(0) , cp(c), cxt(c) {
          sm=new StateMap[c];
    for (int i = 0; i < c; ++i) { cp[i] = 0; }
  }
  ~MContextMap(){ delete[] sm;}
  void set(U32 cx) {
    cxt[cn++] = cx * 16;
  }
  int mix(Mixer&m) {
      int result=0;
    for (int i = 0; i < cn; ++i) {
      if (cp[i]) {
      *cp[i] = nex(*cp[i], y);
      }
    }
    const int c0b = c0 < 16 ? c0 : (16 * (1 + ((c0 >> (bpos - 4)) & 15))) +
              ((c0 & ((1 << (bpos - 4)) - 1)) | (1 << (bpos - 4)));
    for (int i = 0; i < cn; ++i) {
      cp[i] = &t[(cxt[i] + c0b) & (t.size() - 1)]; // Update bit context pointers
    }
    for (int i = 0; i < cn; ++i) {
    // mix3(m, *cp[i], sm[i]); // Predict from bit context
      if (cp[i])
   {
    result+=mix3(m, *cp[i], sm[i]); // Predict from bit context
   }
   else
   {
    mix3(m, 0, sm[i]);
   }
    }
  
    if (bpos == 7) cn = 0;
    return result;
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
  Array<E, 64> t;  // bit histories for bits 0-1, 2-4, 5-7
    // For 0-1, also contains a run count in bh[][4] and value in bh[][5]
    // and pending update count in bh[7]
  Array<U8*> cp;   // C pointers to current bit history
  Array<U8*> cp0;  // First element of 7 element array containing cp[i]
  Array<U32> cxt;  // C whole byte contexts (hashes)
  Array<U8*> runp; // C [0..3] = count, value, unused, unused
  StateMap *sm;    // C maps of state -> p
  int cn;          // Next context to set by set()
  void update(U32 cx, int c);  // train model that context cx predicts c
  int mix1(Mixer& m, int cc, int bp, int c1, int y1);
    // mix() with global context passed as arguments to improve speed.
public:
  ContextMap(int m, int c=1);  // m = memory in bytes, a power of 2, C = c
  ~ContextMap();
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

// Update the model with bit y1, and predict next bit to mixer m.
// Context: cc=c0, bp=bpos, c1=buf(1), y1=y.
int ContextMap::mix1(Mixer& m, int cc, int bp, int c1, int y1) {
  // Update model with y
  int result=0;
  for (int i=0; i<cn; ++i) {
    if (cp[i]) {
      assert(cp[i]>=&t[0].bh[0][0] && cp[i]<=&t[t.size()-1].bh[6][6]);
      assert((long(cp[i])&63)>=15);
      int ns=nex(*cp[i], y1);
      if (ns>=204 && rnd() << ((452-ns)>>3)) ns-=4;  // probabilistic increment
      *cp[i]=ns;
    }

    // Update context pointers
    if (bpos>1 && runp[i][0]==0)
    {
     cp[i]=0;
    }
    else
    {
     switch(bpos)
     {
      case 1: case 3: case 6: cp[i]=cp0[i]+1+(cc&1); break;
      case 4: case 7: cp[i]=cp0[i]+3+(cc&3); break;
      case 2: case 5: cp0[i]=cp[i]=t[(cxt[i]+cc)&(t.size()-1)].get(cxt[i]>>16); break;
      default:
      {
       cp0[i]=cp[i]=t[(cxt[i]+cc)&(t.size()-1)].get(cxt[i]>>16);
       // Update pending bit histories for bits 2-7
       if (cp0[i][3]==2) {
         const int c=cp0[i][4]+256;
         U8 *p=t[(cxt[i]+(c>>6))&(t.size()-1)].get(cxt[i]>>16);
         p[0]=1+((c>>5)&1);
         p[1+((c>>5)&1)]=1+((c>>4)&1);
         p[3+((c>>4)&3)]=1+((c>>3)&1);
         p=t[(cxt[i]+(c>>3))&(t.size()-1)].get(cxt[i]>>16);
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
   if (cp[i])
   {
    result+=mix2(m, *cp[i], sm[i]);
   }
   else
   {
    mix2(m, 0, sm[i]);
   }


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

  static SmallStationaryContextMap scm1(0x20000);

  if (!bpos) {
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
U32 b2=0,b3=0,w4=0;
U32 w5=0,f4=0,tt=0;
U32 WRT_mpw[16]= { 4, 4, 3, 2, 2, 2, 1, 1,  1, 1, 1, 1, 0, 0, 0, 0 };
U32 WRT_mtt[16]= { 0, 0, 1, 2, 3, 4, 5, 5,  6, 6, 6, 6, 7, 7, 7, 7 };
int col=0;

// Model English text (words and columns/end of line)
static U32 frstchar=0,spafdo=0,spaces=0,spacecount=0, words=0,wordcount=0,wordlen=0,wordlen1=0;
void wordModel(Mixer& m) {
	static U32 word0=0, word1=0, word2=0, word3=0, word4=0, word5=0;  // hashes
	static U32 xword0=0,xword1=0,xword2=0,cword0=0,ccword=0;
	static U32 number0=0, number1=0;  // hashes
	static U32 text0=0;  // hash stream of letters
	static ContextMap cm(MEM*16, 45);
	static int nl1=-3, nl=-2;  // previous, current newline position
	static U32 mask = 0;
	// Update word hashes
	if (bpos==0) {
		int c=c4&255,f=0;
		if (spaces&0x80000000) --spacecount;
		if (words&0x80000000) --wordcount;
		spaces=spaces*2;
		words=words*2;

		if (c>='A' && c<='Z') c+='a'-'A';
		if ((c>='a' && c<='z') || c==1 || c==2 ||(c>=128 &&(b2!=3))) {
			++words, ++wordcount;
			word0=word0*263*32+c;
			text0=text0*997*16+c;
			wordlen++;
			wordlen=min(wordlen,45);
			f=0;
		}
		else {
			if (word0) {
				word5=word4*23;
				word4=word3*19;
				word3=word2*17;
				word2=word1*13;
				word1=word0*11;
				wordlen1=wordlen;
				if (c==':') cword0=word0;
				if (c==']'&& (frstchar!=':' || frstchar!='*')) xword0=word0;
			//	if (c==0x27) xword0=word0;
			    if (c=='=') cword0=word0;
				ccword=0;
				word0=wordlen=0;
				if((c=='.'||c=='!'||c=='?') && buf(2)!=10 && filetype!=EXE) f=1; 
				
			}
			if ((c4&0xFFFF)==0x3D3D) xword1=word1,xword2=word2; // '=='
				if ((c4&0xFFFF)==0x2727) xword1=word1,xword2=word2; // ''
			if (c==32 || c==10 ) { ++spaces, ++spacecount; if (c==10 ) nl1=nl, nl=pos-1;}
			else if (c=='.' || c=='!' || c=='?' || c==',' || c==';' || c==':') spafdo=0,ccword=c*31; 
			else { ++spafdo; spafdo=min(63,spafdo); }
		}
		if (c>='0' && c<='9') {
			number0=number0*263*32+c;
		}
		else if (number0) {
			number1=number0*11;
			number0=0,ccword=0;
		}
	
		col=min(255, pos-nl);
		int above=buf[nl1+col]; // text column context
		if (col<=2) frstchar=(col==2?min(c,96):0);
//	cm.set(spafdo|col<<8);
		cm.set(spafdo|spaces<<8);
		if (frstchar=='[' && c==32)	{if(buf(3)==']' || buf(4)==']' ) frstchar=96,xword0=0;}
		cm.set(frstchar<<11|c);
		cm.set(col<<8|frstchar);
		cm.set(spaces<<8|(words&255));
		
		cm.set(number0+word2);
		cm.set(number0+word1);
		cm.set(number1+c);
		cm.set(number0+number1);
		cm.set(word0+number1);

		cm.set(frstchar<<7);
//	cm.set(wordlen<<16|c);
		cm.set(wordlen1<<8|col);
		cm.set(c*64+spacecount/2);
		U32 h=wordcount*64+spacecount;
		cm.set((c<<13)+h);
//	cm.set(h); // 
		cm.set(h+spafdo*8);

		U32 d=c4&0xf0ff;
		cm.set(d<<9|frstchar);

		h=word0*271;
		cm.set(h+ccword);
		h=h+buf(1);
		cm.set(h);
		cm.set(word0);
		//cm.set(word1+(c==32));
		cm.set(h+word1);
		cm.set(word0+word1*31);
		cm.set(h+word1+word2*29);
		cm.set(text0&0xffffff);
		cm.set(text0&0xfffff);
          cm.set(word0+xword0*31);
		  cm.set(word0+xword1*31);
		  cm.set(word0+xword2*31);
		  cm.set(frstchar+xword2*31);
        
        cm.set(word0+cword0*31);
		cm.set(number0+cword0*31);
		cm.set(h+word2);
		cm.set(h+word3);
		cm.set(h+word4);
		cm.set(h+word5);
//		cm.set(buf(1)|buf(3)<<8|buf(5)<<16);
//		cm.set(buf(2)|buf(4)<<8|buf(6)<<16);
		cm.set(h+word1+word3);
		cm.set(h+word2+word3);
		if (f) {
			word5=word4*29;
			word4=word3*31;
			word3=word2*37;
			word2=word1*41;
			word1='.';
		}
		cm.set(col<<16|buf(1)<<8|above);
		cm.set(buf(1)<<8|above);
		cm.set(col<<8|buf(1));
		cm.set(col*(c==32));
		cm.set(col);
		
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
 
    cm.set(mask);
    cm.set((mask<<8)|buf(1));
    //cm.set((mask&0xff)|col<<8);
    cm.set((mask<<17)|(buf(2)<<8)|buf(3));
    cm.set((mask&0x1ff)|((f4&0x00fff0)<<9));
	}
	cm.mix(m);
}
// Model for 1-bit image data

void im1bitModel(Mixer& m, int w) {
  static U32 r0, r1, r2, r3;  // last 4 rows, bit 8 is over current pixel
  static Array<U8> t(0x10200);  // model: cxt -> state
  const int N=4+1+1+1+1+1;  // number of contexts
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
  cxt[2]=0x200+((r0&0x3f)^(r1&0x3ffe)^(r2<<2&0x7f00)^(r3<<5&0xf800));
  cxt[3]=0x400+((r0&0x3e)^(r1&0x0c0c)^(r2&0xc800));
  cxt[4]=0x800+(((r1&0x30)^(r3&0x0c0c))|(r0&3));
  cxt[5]=0x1000+((!r0&0x444)|(r1&0xC0C)|(r2&0xAE3)|(r3&0x51C));
  cxt[6]=0x2000+((r0&1)|(r1>>4&0x1d)|(r2>>1&0x60)|(r3&0xC0));
  cxt[7]=0x4000+((r0>>4&0x2AC)|(r1&0xA4)|(r2&0x349)|(!r3&0x14D));

  // predict
  for (i=0; i<N; ++i) m.add(stretch(sm[i].p(t[cxt[i]])));
  m.set((r0&0x7)|(r1>>4&0x38)|(r2>>3&0xc0), 256);
  m.set(((r1&0x30)^(r3&0x0c))|(r0&3),256);
  m.set((r0&1)|(r1>>4&0x3e)|(r2>>2&0x40)|(r3>>1&0x80), 256);
  m.set((r0&0x3e)^((r1>>8)&0x0c)^((r2>>8)&0xc8),256);
}

//////////////////////////// recordModel ///////////////////////

// Model 2-D data with fixed record length.  Also order 1-2 models
// that include the distance to the last match.

int recordModel(Mixer& m, int rrlen=0) {
  static int cpos1[256] , cpos2[256], cpos3[256], cpos4[256];
  static int wpos1[0x10000]; // buf(1..2) -> last position
  static int rlen=2, rlen1=3, rlen2=4, rlen3=5,rlenl=0;  // run length and 2 candidates
  static int rcount1=0, rcount2=0,rcount3=0;  // candidate counts
  static ContextMap cm(32768, 3), cn(32768/2, 3), co(32768*2, 3), cp(MEM, 3);
  // Find record length
  if (!bpos) {
    int w=c4&0xffff, c=w&255, d=w>>8;
    int r=pos-cpos1[c];
    if (r>1 ){
            if( rrlen==0 ) {
      if ((r==cpos1[c]-cpos2[c] || r==cpos2[c]-cpos3[c] || r==cpos3[c]-cpos4[c] 
        )&& (r>10 || ((c==buf(r*5+1)) && c==buf(r*6+1)))) {
      if (r==rlen1) ++rcount1;
      else if (r==rlen2) ++rcount2;
      else if (r==rlen3) ++rcount3;
      else if (rcount1>rcount2) rlen2=r, rcount2=1;
      else if (rcount2>rcount3) rlen3=r, rcount3=1;
      else rlen1=r, rcount1=1;
     }  
     }
     else   rlen=rrlen;
    } 
 
    if (rcount1>12 && rlen!=rlen1 && rlenl*2!=rlen1) rlenl=rlen=rlen1, rcount1=rcount2=rcount3=0/*, printf("R1: %d  \n",rlen)*/;
    if (rcount2>18 && rlen!=rlen2 && rlenl*2!=rlen2) rlenl=rlen,rlen=rlen2, rcount1=rcount2=rcount3=0/*, printf("R2: %d  \n",rlen)*/;
    if (rcount3>24 && rlen!=rlen3 && rlenl*2!=rlen3) rlenl=rlen,rlen=rlen3, rcount1=rcount2=rcount3=0/*, printf("R3: %d  \n",rlen)*/;

    // Set 2 dimensional contexts
    assert(rlen>0);
    cm.set(c<<8| (min(255, pos-cpos1[c])/4));
    cm.set(w<<9| llog(pos-wpos1[w])>>2);
    //cm.set((buf(rlen)>>4)<<12|(buf(rlen2)>>4)<<8|(buf(rlen3)>>4));

    cm.set(rlen|buf(rlen)<<10|buf(rlen*2)<<18);
    cn.set(w|rlen<<8);
    cn.set(d|rlen<<16);
    cn.set(c|rlen<<8);

    co.set(buf(1)<<8|min(255, pos-cpos1[buf(1)]));
    co.set(buf(1)<<17|buf(2)<<9|llog(pos-wpos1[w])>>2);
    int col=pos%rlen;
    co.set(buf(1)<<8|buf(rlen));

    //cp.set(w*16);
    //cp.set(d*32);
    //cp.set(c*64);
    cp.set(rlen|buf(rlen)<<10|col<<18);
    cp.set(rlen|buf(1)<<10|col<<18);
    cp.set(col|rlen<<12);
    // update last context positions
    cpos4[c]=cpos3[c];
    cpos3[c]=cpos2[c];
    cpos2[c]=cpos1[c];
    cpos1[c]=pos;
    wpos1[w]=pos;
  }
  cm.mix(m);
  cn.mix(m);
  co.mix(m);
  cp.mix(m);
  return rlen;
}

void recordModel1(Mixer& m) {
  static int cpos1[256];
  static int wpos1[0x10000]; // buf(1..2) -> last position
  static ContextMap cm(32768, 2), cn(32768/2, 4+1), co(32768*4, 4),cp(32768*2, 3), cq(32768*2, 3);

  // Find record length
  if (!bpos) {
    int w=c4&0xffff, c=w&255, d=w&0xf0ff,e=c4&0xffffff;
   
    cm.set(c<<8| (min(255, pos-cpos1[c])/4));
    cm.set(w<<9| llog(pos-wpos1[w])>>2);
  
    cn.set(w);
    cn.set(d<<8);
    cn.set(c<<16);
    cn.set((f4&0xfffff)); 
    int col=pos&3;
    cn.set(col|2<<12);

    co.set(c    );
    co.set(w<<8 );
    co.set(w5&0x3ffff);
    co.set(e<<3);
    
    cp.set(d    );
    cp.set(c<<8 );
    cp.set(w<<16);

    cq.set(w<<3 );
    cq.set(c<<19);
    cq.set(e);
    // update last context positions

    cpos1[c]=pos;
    wpos1[w]=pos;
  }
  cm.mix(m);
  cn.mix(m);
  co.mix(m);
  cq.mix(m);
  cp.mix(m);
}

//////////////////////////// sparseModel ///////////////////////

// Model order 1-2 contexts with gaps.

void sparseModel(Mixer& m, int seenbefore, int howmany) {
  static ContextMap cm(MEM*2, 40+2);
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
    cm.set(c4&0x00f0f0f0);
    cm.set(c4&0xf0f0f0f0);
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

// Model for 24-bit image data

// Square buf(i)
inline int sqrbuf(int i) {
  assert(i>0);
  return buf(i)*buf(i);
}

void im24bitModel(Mixer& m, int w) {
  const int SC=0x20000;
  static SmallStationaryContextMap scm1(SC), scm2(SC),
    scm3(SC), scm4(SC), scm5(SC), scm6(SC), scm7(SC), scm8(SC), scm9(SC*2), scm10(512);
  static ContextMap cm(MEM*4, 15);
  // Select nearby pixels as context
  if (!bpos) {
    assert(w>3);
    int color=pos%3;
    int mean=buf(3)+buf(w-3)+buf(w)+buf(w+3);
    const int var=(sqrbuf(3)+sqrbuf(w-3)+sqrbuf(w)+sqrbuf(w+3)-mean*mean/4)>>2;
    mean>>=2;
    const int logvar=ilog(var);
    int i=color<<4;
    cm.set(hash(++i, buf(3)));
    cm.set(hash(++i, buf(3), buf(1)));
    cm.set(hash(++i, buf(3), buf(1), buf(2)));
    cm.set(hash(++i, buf(w)));
    cm.set(hash(++i, buf(w), buf(1)));
    cm.set(hash(++i, buf(w), buf(1), buf(2)));
    cm.set(hash(++i, (buf(3)+buf(w))>>3, buf(1)>>4, buf(2)>>4));
    cm.set(hash(++i, buf(1), buf(2)));
    cm.set(hash(++i, buf(3), buf(1)-buf(4)));
    cm.set(hash(++i, buf(3)+buf(1)-buf(4)));
    cm.set(hash(++i, buf(w), buf(1)-buf(w+1)));
    cm.set(hash(++i, buf(w)+buf(1)-buf(w+1)));
    cm.set(hash(++i, buf(w*3-3), buf(w*3-6)));
    cm.set(hash(++i, buf(w*3+3), buf(w*3+6)));
    cm.set(hash(++i, mean, logvar>>4));

    scm1.set(buf(3)+buf(w)-buf(w+3));
    scm2.set(buf(3)+buf(w-3)-buf(w));
    scm3.set(buf(3)*2-buf(6));
    scm4.set(buf(w)*2-buf(w*2));
    scm5.set(buf(w+3)*2-buf(w*2+6));
    scm6.set(buf(w-3)*2-buf(w*2-6));
    scm7.set(buf(w-3)+buf(1)-buf(w-2));
    scm8.set(buf(w)+buf(w-3)-buf(w*2-3));
    scm9.set(mean>>1|(logvar<<1&0x180));
  }

  // Predict next bit
  scm1.mix(m);
  scm2.mix(m);
  scm3.mix(m);
  scm4.mix(m);
  scm5.mix(m);
  scm6.mix(m);
  scm7.mix(m);
  scm8.mix(m);
  scm9.mix(m);
  scm10.mix(m);
  cm.mix(m);
  static int col=0;
  if (++col>=24) col=0;
  m.set((buf(3)+buf(6))>>6, 8);
  m.set(col, 24);
  m.set((buf(1)>>4)*3+(pos%3), 48);
  m.set(c0, 256);
   
}

//////////////////////////// im8bitModel /////////////////////////////////

// Model for 8-bit image data

void im8bitModel(Mixer& m, int w) {
  const int SC=0x20000;
  static SmallStationaryContextMap scm1(SC), scm2(SC),
    scm3(SC), scm4(SC), scm5(SC), scm6(SC*2)/*,scm7(SC*2)*/,scm8(SC);
  static MContextMap cm(MEM*4, 32+1+1+4+2+4+7-7);
static int ml,ml1=0;
  // Select nearby pixels as context
  if (!bpos) {
    assert(w>3);
    int mean=buf(1)+buf(w-1)+buf(w)+buf(w+1);
    const int var=(sqrbuf(1)+sqrbuf(w-1)+sqrbuf(w)+sqrbuf(w+1)-mean*mean/4)>>2;
    mean>>=2;
    const int logvar=ilog(var);
    int i=0;
    /*int edg=0;
    if (buf(w+1)>=max(buf(2),buf(w))) edg=max(buf(2),buf(w));
    else if (buf(w+1)<=min(buf(2),buf(w))) edg=min(buf(2),buf(w));
    else edg=buf(2)+buf(w)-buf(w+1);*/
    // 2 x
    cm.set(hash(++i, buf(1),0));
    cm.set(hash(++i, buf(2), 0));
    cm.set(hash(++i, buf(w), 0));
    cm.set(hash(++i, buf(w+1), 0));
    cm.set(hash(++i, buf(w-1), 0));
    cm.set(hash(++i, (buf(2)+buf(w)-buf(w+1)), 0));
    cm.set(hash(++i,(buf(w)+(buf(2)-buf(w+1))>>1), 0));
    cm.set(hash(++i, (buf(2)+buf(w+1))>>1, 0));
    cm.set(hash(++i, (buf(w-1)-buf(w)), buf(1)>>1));
    cm.set(hash(++i,(buf(w)-buf(w+1)), buf(1)>>1));
    cm.set(hash(++i, (buf(w+1)+buf(2)), buf(1)>>1));
    cm.set(hash(++i, (buf(w)+buf(w*2))>>1));
    cm.set(hash(++i, (buf(1)+buf(w-1))>>1));
    cm.set(hash(++i, (buf(w)+buf(w+1))>>1));
    // 3 x
    cm.set(hash(++i, buf(w)>>2, buf(1)>>2, buf(w-1)>>2));
    cm.set(hash(++i, buf(w-1)>>2, buf(w)>>2, buf(w+1)>>2));
    cm.set(hash(++i, buf(1)>>2, buf(w-1)>>2, buf(w*2-1)>>2));
    // mixed
    cm.set(hash(++i, (buf(3)+buf(w))>>1, buf(1)>>2, buf(2)>>2));
    cm.set(hash(++i, (buf(2)+buf(1))>>1,(buf(w)+buf(w*2))>>1,buf(w-1)>>2));
    cm.set(hash(++i, (buf(2)+buf(1))>>2,(buf(w-1)+buf(w))>>2));
    cm.set(hash(++i, (buf(2)+buf(1))>>1,(buf(w)+buf(w*2))>>1));
    cm.set(hash(++i, (buf(2)+buf(1))>>1,(buf(w-1)+buf(w*2-2))>>1));
    cm.set(hash(++i, (buf(2)+buf(1))>>1,(buf(w+1)+buf(w*2+2))>>1));
    cm.set(hash(++i, (buf(w)+buf(w*2))>>1,(buf(w-1)+buf(w*2+2))>>1));
    cm.set(hash(++i, (buf(w-1)+buf(w))>>1,(buf(w)+buf(w+1))>>1));
    cm.set(hash(++i, (buf(1)+buf(w-1))>>1,(buf(w)+buf(w*2))>>1));
    cm.set(hash(++i, (buf(1)+buf(w-1))>>2,(buf(w)+buf(w+1))>>2));
    cm.set(hash(++i, (((buf(1)-buf(w-1))>>1)+buf(w))>>2));
    cm.set(hash(++i, (((buf(w-1)-buf(w))>>1)+buf(1))>>2));
    cm.set(hash(++i, (-buf(1)+buf(w-1)+buf(w))>>2));
    
    cm.set(hash(++i,(buf(1)*2-buf(2))>>1));
    cm.set(hash(++i,mean,logvar));
    cm.set(hash(++i,(buf(w)*2-buf(w*2))>>1));
    cm.set(hash(++i,(buf(1)+buf(w)-buf(w+1))>>1));
    
    cm.set(hash(++i, (buf(4)+buf(3))>>2,(buf(w-1)+buf(w))>>2));
    cm.set(hash(++i, (buf(4)+buf(3))>>1,(buf(w)+buf(w*2))>>1));
    cm.set(hash(++i, (buf(4)+buf(3))>>1,(buf(w-1)+buf(w*2-2))>>1));
    cm.set(hash(++i, (buf(4)+buf(3))>>1,(buf(w+1)+buf(w*2+2))>>1));
    cm.set(hash(++i, (buf(4)+buf(1))>>2,(buf(w-3)+buf(w))>>2));
    cm.set(hash(++i, (buf(4)+buf(1))>>1,(buf(w)+buf(w*2))>>1));
    cm.set(hash(++i, (buf(4)+buf(1))>>1,(buf(w-3)+buf(w*2-3))>>1));
    cm.set(hash(++i, (buf(4)+buf(1))>>1,(buf(w+3)+buf(w*2+3))>>1));
    cm.set(hash(++i, buf(w)>>2, buf(3)>>2, buf(w-1)>>2));
    cm.set(hash(++i, buf(3)>>2, buf(w-2)>>2, buf(w*2-2)>>2));
        
    scm1.set((buf(1)+buf(w))>>1);
    scm2.set((buf(1)+buf(w)-buf(w+1))>>1);
    scm3.set((buf(1)*2-buf(2))>>1);
    scm4.set((buf(w)*2-buf(w*2))>>1);
    scm5.set((buf(1)+buf(w)-buf(w-1))>>1);
    ml1=mean;
    ml=mean>>1|(logvar<<1&0x180);
    scm6.set(mean>>1|(logvar<<1&0x180));
   // scm7.set(edg);
  }

  // Predict next bit
  scm1.mix(m);
  scm2.mix(m);
  scm3.mix(m);
  scm4.mix(m);
  scm5.mix(m);
  scm6.mix(m);
 // scm7.mix(m);
  scm8.mix(m); // Amazingly but improves compression!
  cm.mix(m);
  static int col=0;
  if (++col>=8) col=0; // reset after every 24 columns?
  m.set(2, 8);
  m.set(col, 8);
  m.set((buf(w)+buf(1))>>4, 32);
  m.set(c0, 256);
  m.set(ml, 512);
  m.set(ml1, 1024);
}

//////////////////////////// im1bitModel /////////////////////////////////




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

// Detect invalid JPEG data.  The proper response is to silently
// fall back to a non-JPEG model.
#define jassert(x) if (!(x)) { \
/*  printf("JPEG error at %d, line %d: %s\n", pos, __LINE__, #x); */ \
  jpeg=0; \
  return 0;}

struct HUF {U32 min, max; int val;}; // Huffman decode tables
  // huf[Tc][Th][m] is the minimum, maximum+1, and pointer to codes for
  // coefficient type Tc (0=DC, 1=AC), table Th (0-3), length m+1 (m=0-15)

int jpegModel(Mixer& m) {

  // State of parser
  enum {SOF0=0xc0, SOF1, SOF2, SOF3, DHT, RST0=0xd0, SOI=0xd8, EOI, SOS, DQT,
    DNL, DRI, APP0=0xe0, COM=0xfe, FF};  // Second byte of 2 byte codes
  static int jpeg=0;  // 1 if JPEG is header detected, 2 if image data
  //static int next_jpeg=0;  // updated with jpeg on next byte boundary
  static int app;  // Bytes remaining to skip in APPx or COM field
  static int sof=0, sos=0, data=0;  // pointers to buf
  static Array<int> ht(8);  // pointers to Huffman table headers
  static int htsize=0;  // number of pointers in ht

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
  static int linesize=0; // width of image in MCU
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
  //static U32 huff1=0, huff2=0, huff3=0, huff4=0;  // hashes of last codes
  static int rs1;//, rs2, rs3, rs4;  // last 4 RS codes
  static int ssum=0, ssum1=0, ssum2=0, ssum3=0;
    // sum of S in RS codes in block and sum of S in first component

  static IntBuf cbuf2(0x20000);
  static Array<int> adv_pred(7), sumu(8), sumv(8);
  static Array<int> ls(10);  // block -> distance to previous block
  static Array<int> lcp(4), zpos(64);

    //for parsing Quantization tables
  static int dqt_state = -1, dqt_end = 0, qnum = 0;
  static Array<U8> qtab(256); // table
  static Array<int> qmap(10); // block -> table number

  const static U8 zzu[64]={  // zigzag coef -> u,v
    0,1,0,0,1,2,3,2,1,0,0,1,2,3,4,5,4,3,2,1,0,0,1,2,3,4,5,6,7,6,5,4,
    3,2,1,0,1,2,3,4,5,6,7,7,6,5,4,3,2,3,4,5,6,7,7,6,5,4,5,6,7,7,6,7};
  const static U8 zzv[64]={
    0,0,1,2,1,0,0,1,2,3,4,3,2,1,0,0,1,2,3,4,5,6,5,4,3,2,1,0,0,1,2,3,
    4,5,6,7,7,6,5,4,3,2,1,2,3,4,5,6,7,7,6,5,4,3,4,5,6,7,7,6,5,6,7,7};
  //reset all static variables to be sure
  if (!blpos && bpos==1) {
	jpeg=0;//, next_jpeg=0; 
 	sof=0, sos=0, data=0; 
 	htsize=0,huffcode=0, huffbits=0,huffsize=0; 
 	rs=-1, mcupos=0, mcusize=0, linesize=0; 
 	dc=0, width=0,row=0, column=0;
 	cpos=0,ssum=0, ssum1=0, ssum2=0, ssum3=0;
 	dqt_state = -1, dqt_end = 0, qnum = 0;
  }


  if (!bpos && !blpos) jpeg=0;
  // Be sure to quit on a byte boundary
  if (bpos && !jpeg) return 0;
  if (!bpos && app>=0) --app;
  if (app>0) return 0;
  if (!bpos) {


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

    // Detect JPEG (SOI, APPx)
    if (!jpeg && buf(4)==FF && buf(3)==SOI && buf(2)==FF && buf(1)>>4==0xe) {
      jpeg=1;
      sos=sof=htsize=data=mcusize=linesize=0, app=2;
      huffcode=huffbits=huffsize=mcupos=cpos=0, rs=-1;
      memset(&huf[0], 0, huf.size()*sizeof(HUF));
      memset(&pred[0], 0, pred.size()*sizeof(int));
    }

    // Detect end of JPEG when data contains a marker other than RSTx
    // or byte stuff (00).
    if (jpeg && data && buf(2)==FF && buf(1) && (buf(1)&0xf8)!=RST0) {
      jassert(buf(1)==EOI);
      jpeg=0;
    }
    if (!jpeg) return 0;

    // Detect APPx or COM field
    if (!data && !app && buf(4)==FF && (buf(3)>>4==0xe || buf(3)==COM))
      app=buf(2)*256+buf(1)+2;

    // Save pointers to sof, ht, sos, data,
    if (buf(5)==FF && buf(4)==SOS) {
      int len=buf(3)*256+buf(2);
      if (len==6+2*buf(1) && buf(1) && buf(1)<=4)  // buf(1) is Ns
        sos=pos-5, data=sos+len+2, jpeg=2;
    }
    if (buf(4)==FF && buf(3)==DHT && htsize<8) ht[htsize++]=pos-4;
    if (buf(4)==FF && buf(3)==SOF0) sof=pos-4;

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
          qtab[qnum*64+((dqt_state%65)-1)]=buf(1)-1;
        }
        dqt_state++;
      }
    }

    // Restart
    if (buf(2)==FF && (buf(1)&0xf8)==RST0) {
      huffcode=huffbits=huffsize=mcupos=0, rs=-1;
      memset(&pred[0], 0, pred.size()*sizeof(int));
    }
  }

  {
    // Build Huffman tables
    // huf[Tc][Th][m] = min, max+1 codes of length m, pointer to byte values
    if (pos==data && bpos==1) {
      jassert(htsize>0);
      int i;
      for (i=0; i<htsize; ++i) {
        int p=ht[i]+4;  // pointer to current table after length field
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

      // Build Huffman table selection table (indexed by mcupos).
      // Get image width.
      if (!sof && sos) return 0;
      int ns=buf[sos+4];
      int nf=buf[sof+9];
      jassert(ns<=4 && nf<=4);
      mcusize=0;  // blocks per MCU
      int hmax=0;  // MCU horizontal dimension
      for (i=0; i<ns; ++i) {
        for (int j=0; j<nf; ++j) {
          if (buf[sos+2*i+5]==buf[sof+3*j+10]) { // Cs == C ?
            int hv=buf[sof+3*j+11];  // packed dimensions H x V
            if (hv>>4>hmax) hmax=hv>>4;
            hv=(hv&15)*(hv>>4);  // number of blocks in component C
            jassert(hv>=1 && hv+mcusize<=10);
            while (hv) {
              jassert(mcusize<10);
              hufsel[0][mcusize]=buf[sos+2*i+6]>>4&15;
              hufsel[1][mcusize]=buf[sos+2*i+6]&15;
              jassert (hufsel[0][mcusize]<4 && hufsel[1][mcusize]<4);
              color[mcusize]=i;
              int tq=buf[sof+3*j+12];  // quantization table index (0..3)
              jassert(tq>=0 && tq<4);
              qmap[mcusize]=tq; // quantizazion table mapping
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
      width=buf[sof+7]*256+buf[sof+8];  // in pixels
      width=(width-1)/(hmax*8)+1;  // in MCU
      jassert(width>0);
      mcusize*=64;  // coefficients per MCU
      row=column=0;
    }
  }


  // Decode Huffman
  {
    if (mcusize && buf(1+(!bpos))!=FF) {  // skip stuffed byte
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
         // huff4=huff3;
         // huff3=huff2;
         // huff2=huff1;
         // huff1=hash(huffcode, huffbits);
         // rs4=rs3;
         // rs3=rs2;
         // rs2=rs1;
          rs1=rs;
          int x=0;  // decoded extra bits
          if (mcupos&63) {  // AC
            if (rs==0) { // EOB
              mcupos=(mcupos+63)&-64;
              jassert(mcupos>=0 && mcupos<=mcusize && mcupos<=640);
              while (cpos&63) {
                cbuf2[cpos]=0;
                cbuf[cpos++]=0;
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
            const int acomp=mcupos>>6, q=64*qmap[acomp];
            const int zz=mcupos&63, cpos_dc=cpos-zz;
            if (zz==0) {
              for (int i=0; i<8; ++i) sumu[i]=sumv[i]=0;
              int cpos_dc_ls_acomp = cpos_dc-ls[acomp];
              int cpos_dc_mcusize_width = cpos_dc-mcusize*width;
              for (int i=0; i<64; ++i) {
                sumu[zzu[i]]+=(zzv[i]&1?-1:1)*(zzv[i]?16*(16+zzv[i]):181)*(qtab[q+i]+1)*cbuf2[cpos_dc_mcusize_width+i];
                sumv[zzv[i]]+=(zzu[i]&1?-1:1)*(zzu[i]?16*(16+zzu[i]):181)*(qtab[q+i]+1)*cbuf2[cpos_dc_ls_acomp+i];
              }
            }
            else {
              sumu[zzu[zz-1]]-=(zzv[zz-1]?16*(16+zzv[zz-1]):181)*(qtab[q+zz-1]+1)*cbuf2[cpos-1];
              sumv[zzv[zz-1]]-=(zzu[zz-1]?16*(16+zzu[zz-1]):181)*(qtab[q+zz-1]+1)*cbuf2[cpos-1];
            }

            for (int i=0; i<3; ++i)
              for (int st=0; st<8; ++st) {
                const int zz2=min(zz+st, 63);
                int p=(sumu[zzu[zz2]]*i+sumv[zzv[zz2]]*(2-i))/2;
                p/=(qtab[q+zz2]+1)*181*(16+zzv[zz2])*(16+zzu[zz2])/256;
                if (zz2==0) p-=cbuf2[cpos_dc-ls[acomp]];
                p=(p<0?-1:+1)*ilog(10*abs(p)+1)/10;
                if (st==0) {
                  adv_pred[i]=p;
                  adv_pred[i+4]=p/4;
                }
                else if (abs(p)>abs(adv_pred[i])+1) {
                  adv_pred[i]+=(st*2+(p>0))<<6;
                  if (abs(p/4)>abs(adv_pred[i+4])+1) adv_pred[i+4]+=(st*2+(p>0))<<6;
                  break;
                }
              }
            x=2*sumu[zzu[zz]]+2*sumv[zzv[zz]];
            for (int i=0; i<8; ++i) x-=(zzu[zz]<i)*sumu[i]+(zzv[zz]<i)*sumv[i];
            x/=(qtab[q+zz]+1)*181;
            if (zz==0) x-=cbuf2[cpos_dc-ls[acomp]];
            adv_pred[3]=(x<0?-1:+1)*ilog(10*abs(x)+1)/10;

            for (int i=0; i<4; ++i) {
              const int a=(i&1?zzv[zz]:zzu[zz]), b=(i&2?2:1);
              if (a<b) x=255;
              else {
                const int zz2=zpos[zzu[zz]+8*zzv[zz]-(i&1?8:1)*b];
                x=(qtab[q+zz2]+1)*cbuf2[cpos_dc+zz2]/(qtab[q+zz]+1);
                x=(x<0?-1:+1)*ilog(10*abs(x)+1)/10;
              }
              lcp[i]=x;
            }
            if (column==0) adv_pred[1]=adv_pred[2], adv_pred[0]=1;
            if (row==0) adv_pred[1]=adv_pred[0], adv_pred[2]=1;
          } // !!!!

        }
      }
    }
  }

  // Estimate next bit probability
  if (!jpeg || !data) return 0;
  if (buf(1+(!bpos))==FF) {
    m.add(128);
    m.set(1, 8);
    m.set(0, 257);
    m.set(buf(1), 256);
    return 1;
  }

  // Context model
  const int N=28; // size of t, number of contexts
  static BH<9> t(MEM);  // context hash -> bit history
    // As a cache optimization, the context does not include the last 1-2
    // bits of huffcode if the length (huffbits) is not a multiple of 3.
    // The 7 mapped values are for context+{"", 0, 00, 01, 1, 10, 11}.
  static Array<U32> cxt(N);  // context hashes
  static Array<U8*> cp(N);  // context pointers
  static StateMap sm[N];
  static Mixer m1(32, 770, 3);
  static APM a1(0x8000), a2(0x10000);


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
  static int hbcount=2;
  if (++hbcount>2 || huffbits==0) hbcount=0;
  jassert(coef>=0 && coef<256);
  const int zu=zzu[mcupos&63], zv=zzv[mcupos&63];
  if (hbcount==0) {
    int n=0;
    cxt[0]=hash(++n, hc, coef, adv_pred[2], ssum2>>6);
    cxt[1]=hash(++n, hc, coef, adv_pred[0], ssum2>>6);
    cxt[2]=hash(++n, hc, coef, adv_pred[1], ssum2>>6);
    cxt[3]=hash(++n, hc, rs1, adv_pred[2]);
    cxt[4]=hash(++n, hc, rs1, adv_pred[0]);
    cxt[5]=hash(++n, hc, rs1, adv_pred[1]);
    cxt[6]=hash(++n, hc, adv_pred[2], adv_pred[0]);
    cxt[7]=hash(++n, hc, cbuf[cpos-width*mcusize], adv_pred[3]);
    cxt[8]=hash(++n, hc, cbuf[cpos-ls[mcupos>>6]], adv_pred[3]);
    cxt[9]=hash(++n, hc, lcp[0], lcp[1], adv_pred[1]);
    cxt[10]=hash(++n, hc, lcp[0], lcp[1], mcupos&63);
    cxt[11]=hash(++n, hc, zu, lcp[0], lcp[2]/3);
    cxt[12]=hash(++n, hc, zv, lcp[1], lcp[3]/3);
    cxt[13]=hash(++n, hc, mcupos>>1);
    cxt[14]=hash(++n, hc, mcupos&63, column>>1);
    cxt[15]=hash(++n, hc, column>>3, lcp[0]+256*(lcp[2]/4), lcp[1]+256*(lcp[3]/4));
    cxt[16]=hash(++n, hc, ssum>>3, mcupos&63);
    cxt[17]=hash(++n, hc, rs1, mcupos&63);
    cxt[18]=hash(++n, hc, mcupos>>3, ssum2>>5, adv_pred[3]);
    cxt[19]=hash(++n, hc, lcp[0]/4, lcp[1]/4, adv_pred[5]);
    cxt[20]=hash(++n, hc, cbuf[cpos-width*mcusize], adv_pred[6]);
    cxt[21]=hash(++n, hc, cbuf[cpos-ls[mcupos>>6]], adv_pred[4]);
    cxt[22]=hash(++n, hc, adv_pred[2]);
    cxt[23]=hash(n, hc, adv_pred[0]);
    cxt[24]=hash(n, hc, adv_pred[1]);
    cxt[25]=hash(++n, hc, zv, lcp[1], adv_pred[6]);
    cxt[26]=hash(++n, hc, zu, lcp[0], adv_pred[4]);
    cxt[27]=hash(++n, hc, lcp[0], lcp[1], adv_pred[3]);
  }

  // Predict next bit
  m1.add(128);
  assert(hbcount<=2);
 switch(hbcount)
  {
   case 0: for (int i=0; i<N; ++i) cp[i]=t[cxt[i]]+1, m1.add(stretch(sm[i].p(*cp[i]))); break;
   case 1: { int hc=1+(huffcode&1)*3; for (int i=0; i<N; ++i) cp[i]+=hc, m1.add(stretch(sm[i].p(*cp[i]))); } break;
   default: { int hc=1+(huffcode&1); for (int i=0; i<N; ++i) cp[i]+=hc, m1.add(stretch(sm[i].p(*cp[i]))); } break;
  }

  m1.set(column==0, 2);
  m1.set(coef, 256);
  m1.set(hc&511, 512);
  int pr=m1.p();
  m.add(stretch(pr));
  pr=a1.p(pr, (hc&511)|((adv_pred[1]==0?0:(abs(adv_pred[1])-4)&63)<<9), 1023);
  pr=a2.p(pr, (hc&255)|(coef<<8), 255);
  m.add(stretch(pr));
  m.set(1, 8);
  m.set(1+(hc&255), 257);
  m.set(buf(1), 256);
  return 1;
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

void wavModel(Mixer& m, int info) {
  static int pr[3][2], n[2], counter[2];
  static double F[49][49][2],L[49][49];
  int j,k,l,i=0;
  long double sum;
  const double a=0.996,a2=1/a;
  const int SC=0x20000;
  static SmallStationaryContextMap scm1(SC), scm2(SC), scm3(SC), scm4(SC), scm5(SC), scm6(SC), scm7(SC);
  static ContextMap cm(MEM*4, 10);
  static int bits, channels, w,rlen=0;
  static int z1, z2, z3, z4, z5, z6, z7;

  if (!blpos && bpos==1) {
    bits=((info%4)/2)*8+8;
    channels=info%2+1;
    w=channels*(bits>>3);
    wmode=info;
    rlen=(bits/8*channels);
    if (channels==1) S=48,D=0; else S=36,D=12;
    for (int j=0; j<channels; j++) {
      for (k=0; k<=S+D; k++) for (l=0; l<=S+D; l++) F[k][l][j]=0, L[k][l]=0;
      F[1][0][j]=1;
      n[j]=counter[j]=pr[2][j]=pr[1][j]=pr[0][j]=0;
      z1=z2=z3=z4=z5=z6=z7=0;
    }
  }
  // Select previous samples and predicted sample as context
  if (!bpos && blpos>=w) {
    const int ch=blpos%w;
    const int msb=ch%(bits>>3);
    const int chn=ch/(bits>>3);
    if (!msb) {
      z1=X1(1), z2=X1(2), z3=X1(3), z4=X1(4), z5=X1(5);
      k=X1(1);
      for (l=0; l<=min(S,counter[chn]-1); l++) { F[0][l][chn]*=a; F[0][l][chn]+=X1(l+1)*k; }
      for (l=1; l<=min(D,counter[chn]); l++) { F[0][l+S][chn]*=a; F[0][l+S][chn]+=X2(l+1)*k; }
      if (channels==2) {
        k=X2(2);
        for (l=1; l<=min(D,counter[chn]); l++) { F[S+1][l+S][chn]*=a; F[S+1][l+S][chn]+=X2(l+1)*k; }
        for (l=1; l<=min(S,counter[chn]-1); l++) { F[l][S+1][chn]*=a; F[l][S+1][chn]+=X1(l+1)*k; }
        z6=X2(1)+X1(1)-X2(2), z7=X2(1);
      } else z6=2*X1(1)-X1(2), z7=X1(1);
      if (++n[chn]==(256>>level)) {
        if (channels==1) for (k=1; k<=S+D; k++) for (l=k; l<=S+D; l++) F[k][l][chn]=(F[k-1][l-1][chn]-X1(k)*X1(l))*a2;
        else for (k=1; k<=S+D; k++) if (k!=S+1) for (l=k; l<=S+D; l++) if (l!=S+1) F[k][l][chn]=(F[k-1][l-1][chn]-(k-1<=S?X1(k):X2(k-S))*(l-1<=S?X1(l):X2(l-S)))*a2;
        for (i=1; i<=S+D; i++) {
           sum=F[i][i][chn];
           for (k=1; k<i; k++) sum-=L[i][k]*L[i][k];
           sum=floor(sum+0.5);
           sum=1/sum;
           if (sum>0) {
             L[i][i]=sqrt(sum);
             for (j=(i+1); j<=S+D; j++) {
               sum=F[i][j][chn];
               for (k=1; k<i; k++) sum-=L[j][k]*L[i][k];
               sum=floor(sum+0.5);
               L[j][i]=sum*L[i][i];
             }
           } else break;
        }
        if (i>S+D && counter[chn]>S+1) {
          for (k=1; k<=S+D; k++) {
            F[k][0][chn]=F[0][k][chn];
            for (j=1; j<k; j++) F[k][0][chn]-=L[k][j]*F[j][0][chn];
            F[k][0][chn]*=L[k][k];
          }
          for (k=S+D; k>0; k--) {
            for (j=k+1; j<=S+D; j++) F[k][0][chn]-=L[j][k]*F[j][0][chn];
            F[k][0][chn]*=L[k][k];
          }
        }
        n[chn]=0;
      }
      sum=0;
      for (l=1; l<=S+D; l++) sum+=F[l][0][chn]*(l<=S?X1(l):X2(l-S));
      pr[2][chn]=pr[1][chn];
      pr[1][chn]=pr[0][chn];
      pr[0][chn]=int(floor(sum));
      counter[chn]++;
    }
    const int y1=pr[0][chn], y2=pr[1][chn], y3=pr[2][chn];
    int x1=buf(1), x2=buf(2), x3=buf(3);
    if (wmode==4 || wmode==5) x1^=128, x2^=128;
    if (bits==8) x1-=128, x2-=128;
    const int t=((bits==8) || ((!msb)^(wmode<6)));
    i=ch<<4;
    if ((msb)^(wmode<6)) {
      cm.set(hash(++i, y1&0xff));
      cm.set(hash(++i, y1&0xff, ((z1-y2+z2-y3)>>1)&0xff));
      cm.set(hash(++i, x1, y1&0xff));
      cm.set(hash(++i, x1, x2>>3, x3));
      cm.set(hash(++i, (y1+z1-y2)&0xff));
      cm.set(hash(++i, x1));
      cm.set(hash(++i, x1, x2));
      cm.set(hash(++i, z1&0xff));
      cm.set(hash(++i, (z1*2-z2)&0xff));
      cm.set(hash(++i, z6&0xff));
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
  if (level>=4) recordModel(m,rlen);
  static int col=0;
  if (++col>=w*8) col=0;
  m.set(3, 8);
  m.set(col%bits<8, 2);
  m.set(col%bits, bits);
  m.set(col, w*8);
  m.set(c0, 256);
}

//////////////////////////// exeModel /////////////////////////

// Model x86 code.  The contexts are sparse containing only those
// bits relevant to parsing (2 prefixes, opcode, and mod and r/m fields
// of modR/M byte).

inline int pref(int i) { return (buf(i)==0x0f)+2*(buf(i)==0x66)+3*(buf(i)==0x67); }

// Get context at buf(i) relevant to parsing 32-bit x86 code
U32 execxt(int i, int x=0) {
  int prefix=0, opcode=0, modrm=0;
  if (i) prefix+=4*pref(i--);
  if (i) prefix+=pref(i--);
  if (i) opcode+=buf(i--);
  if (i) modrm+=buf(i)&0xc7;
  return prefix|opcode<<4|modrm<<12|x<<20;
}

void exeModel(Mixer& m) {
  const int N=14;
  static ContextMap cm(MEM, N);
  if (!bpos) {
    for (int i=0; i<N; ++i) cm.set(execxt(i+1, buf(1)*(i>6)));
  }
  cm.mix(m);
}

//////////////////////////// indirectModel /////////////////////

// The context is a byte string history that occurs within a
// 1 or 2 byte context.

void indirectModel(Mixer& m) {
  static ContextMap cm(MEM, 9+3);
  static U32 t1[256];
  static U16 t2[0x10000];
  static U16 t3[0x8000];
  static U16 t4[0x8000];

  if (!bpos) {
  	
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
  unsigned int nx[2];  // next pointers
  U8 state;  // bit history
  unsigned int c0:12, c1:12;  // counts * 256
};

void dmcModel(Mixer& m) {
  static int top=0, curr=0;  // allocated, current node
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
      if (top==(t.size()*4)/8) { //        5/8, 4/8, 3/8
        threshold=512;
      } else if (top==(t.size()*6)/8) { // 6/8, 6/8, 7/8
        threshold=768;
      }
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
    if (t[curr].c1<=3840) t[curr].c1+=256;
   } else  if (t[curr].c0<=3840)   t[curr].c0+=256;
  t[curr].state=nex(t[curr].state, y);
  curr=t[curr].nx[y];

  // predict
  const int pr1=sm.p(t[curr].state);
  const int n1=t[curr].c1;
  const int n0=t[curr].c0;
  const int pr2=(n1+5)*4096/(n0+n1+10);
  m.add(stretch(pr1));
  m.add(stretch(pr2));
}

void nestModel(Mixer& m)
{
  static int ic=0, bc=0, pc=0,vc=0, qc=0, lvc=0, wc=0;
  static ContextMap cm(MEM/2, 14-4);
 // static U32 mask = 0;
  if (bpos==0) {
    int c=c4&255, matched=1, vv;
    const int lc = (c >= 'A' && c <= 'Z'?c+'a'-'A':c);
    if (lc == 'a' || lc == 'e' || lc == 'i' || lc == 'o' || lc == 'u') vv = 1; else
    if (lc >= 'a' && lc <= 'z') vv = 2; else
    if (lc == ' ' || lc == '.' || lc == ',' || lc == '!' || lc == '?' || lc == '\n') vv = 3; else
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
      case '(': ic += 513; break;
      case ')': ic -= 513; break;
      case '[': ic += 17; break;
      case ']': ic -= 17; break;
      case '<': ic += 23; qc += 34; break;
      case '>': ic -= 23; qc /= 5; break;
      case ':': pc = 20; break;
      case '{': ic += 22; break;
      case '}': ic -= 22; break;
      case '|': pc += 223; break;
      case '"': pc += 0x40; break;
      case '\'': pc += 0x42; break;
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
      case '=': pc += 87; break;
      default: matched = 0;
    }
    if (matched) bc = 0; else bc += 1;
    if (bc > 300) bc = ic = pc = qc = 0;

    cm.set((3*vc+77*pc+373*ic+qc)&0xffff);
    cm.set((31*vc+27*pc+281*qc)&0xffff);
    cm.set((13*vc+271*ic+qc+bc)&0xffff);
    cm.set((17*pc+7*ic)&0xffff);
    cm.set((13*vc+ic)&0xffff);
    cm.set((vc/3+pc)&0xffff);
    cm.set((7*wc+qc)&0xffff);
    cm.set((vc&0xffff)|((f4&0xf)<<16));
    cm.set(((3*pc)&0xffff)|((f4&0xf)<<16));
    cm.set((ic&0xffff)|((f4&0xf)<<16));
  }
  cm.mix(m);
}
U32 x4=0,s4=0;
void sparseModel1(Mixer& m, int seenbefore, int howmany) {
   static ContextMap cm(MEM*4, 31);
    static SmallStationaryContextMap scm1(0x10000), scm2(0x20000), scm3(0x2000),
     scm4(0x8000), scm5(0x2000),scm6(0x2000), scma(0x10000);
  if (bpos==0) {
    scm5.set(seenbefore);
    scm6.set(howmany);
  /*  cm.set(x4&0x00ff00ff);
    cm.set(x4&0xff0000ff);
    cm.set(x4&0x00ffff00);
    cm.set((x4&0xf8f8f8f8));
    cm.set((x4&0x80f0f0ff));*/
  U32  h=x4<<6;
    cm.set(buf(1)+(h&0xffffff00));
    cm.set(buf(1)+(h&0x00ffff00));
    cm.set(buf(1)+(h&0x0000ff00));
      U32 d=c4&0xffff;
     h<<=6;
    cm.set(d+(h&0xffff0000));
    cm.set(d+(h&0x00ff0000));
     h<<=6, d=c4&0xffffff;
    cm.set(d+(h&0xff000000));

    for (int i=1; i<5; ++i) { 
      cm.set(seenbefore|buf(i)<<8);
      cm.set((buf(i+3)<<8)|buf(i+1));
    }
    cm.set(spaces&0x7fff);
    cm.set(spaces&0xff);
    cm.set(words&0x1ffff);
    cm.set(f4&0x000fffff);
    cm.set(tt&0x00000fff);
      h=w4<<6;
    cm.set(buf(1)+(h&0xffffff00));
    cm.set(buf(1)+(h&0x00ffff00));
    cm.set(buf(1)+(h&0x0000ff00));
      d=c4&0xffff;
     h<<=6;
    cm.set(d+(h&0xffff0000));
    cm.set(d+(h&0x00ff0000));
     h<<=6, d=c4&0xffffff;
    cm.set(d+(h&0xff000000));
    cm.set(w4&0xf0f0f0ff);
    
    //cm.set(f4);
    cm.set((w4&63)*128+(5<<17));
    cm.set((f4&0xffff)<<11|frstchar);
    cm.set(spafdo*8*((w4&3)==1));
	
      scm1.set(words&127);
      scm2.set((words&12)*16+(w4&12)*4+(buf(1)>>4));
      scm3.set(w4&15);
      scm4.set(spafdo*((w4&3)==1));
      scma.set(frstchar);
  }
  cm.mix(m);
  scm1.mix(m);
  scm2.mix(m);
  scm3.mix(m);
  scm4.mix(m);
  scm5.mix(m);
  scm6.mix(m);
  scma.mix(m);

} 

// Normal model
int normalModel(Mixer& m) {
  static ContextMap cm(MEM*32, 9);
  static RunContextMap  rcm7(MEM/4), rcm9(MEM/4), rcm10(MEM/2);
  static U32 cxt[14];  // order 0-11 contexts
  int primes[]={ 0, 257,251,241,239,233,229,227,223,211,199,197,193,191,181,179,173};   

  if (bpos==0) {
    int i;
    for (i=13; i>0; --i)  // update order 0-11 context hashes
      cxt[i]=cxt[i-1]*primes[i]+(c4&255);
    for (i=0; i<7; ++i)
      cm.set(cxt[i]);
    rcm7.set(cxt[7]);
    cm.set(cxt[8]);
    rcm9.set(cxt[10]);
    
    rcm10.set(cxt[12]);
    cm.set(cxt[13]);
  }
  rcm7.mix(m);
  rcm9.mix(m);
  rcm10.mix(m);
  return cm.mix(m);
}

//////////////////////////// contextModel //////////////////////

// This combines all the context models with a Mixer.
int finfo=0;
int contextModel2() {
  static Mixer m(925, 3095+256*7+256*8+256*7+2048+256*7-256*4-264, 7);
 
  m.update();
  m.add(256);
  // Test for special file types
  int ismatch=ilog(matchModel(m));  // Length of longest matching context
  if (filetype==IMAGE1)  return im1bitModel(m, finfo), m.p();;
  if (filetype==IMAGE8 ) return im8bitModel(m, finfo), m.p();
  if (filetype==IMAGE24) return im24bitModel(m, finfo), m.p();
  if (filetype==AUDIO) return wavModel(m, finfo), m.p();
  if (filetype==JPEG) if (jpegModel(m)) return m.p();

  int order=normalModel(m);
  order=order-2; if(order<0) order=0;
  
  if (level>=4 && filetype!=IMAGE1){
  	switch (filetype){
	case DICTTXT:
	case TXTUTF8:
		{ 
            wordModel(m);
			sparseModel1(m,ismatch,order);
			nestModel(m);
			indirectModel(m);
			dmcModel(m);
			recordModel1(m);
			U32 c3=buf(3), c;
			c=(words>>1)&63;
			m.set((w4&3)*64+c+order*256, 256*7); 
			m.set(c0, 256);
  			m.set(ismatch, 256);
			m.set(256*order + (w4&240) + (b3>>4), 256*7);
			c=(w4&255)+256*bpos;
			m.set(c, 256*8);
			if (bpos)
			{
			  c=c0<<(8-bpos); if (bpos==1)c+=c3/2;
              c=(min(bpos,5))*256+(tt&63)+(c&192);
		    }
	        else c=(words&12)*16+(tt&63);
			m.set(c, 1536);
			c=bpos*256+((c0<<(8-bpos))&255);
			c3 = (words<<bpos) & 255;
			m.set(c+(c3>>bpos), 2048);

			int pr=m.p();
			return pr;
		}
	case IMAGE4:
		{ 
            recordModel(m, finfo);
            break;
        }
	default: { 
            int rlen=0;
            if (filetype!=TEXT) rlen=recordModel(m);
            if (rlen==216) return im1bitModel(m, rlen),m.p();
            wordModel(m);
			sparseModel(m,ismatch,order);
			distanceModel(m);
			indirectModel(m);
			if (filetype!=EXE) nestModel(m);
			dmcModel(m);
			if (filetype==EXE) exeModel(m);
			break;
		} 
	}
  }

  U32 c1=buf(1), c2=buf(2), c3=buf(3), c;
  m.set(c1+8, 264); 
  m.set(c0, 256);
  if (filetype==TEXT) c=(c2&0x1F)|((c3&0x1F)<<5); else c=c2;
  m.set(c, 1023);
  U8 d=c0<<(8-bpos);
  if (filetype==EXE) {
    m.set(order+8*(c4>>6&3)+32*(bpos==0)+64*(c1==c2), 256);
    m.set(c3, 256);
  }else{
    m.set(order*256+(w4&240)+(b3>>4),1536);
    m.set(bpos*256+((words<<bpos&255)>>bpos|(d&255)),2048);//, 256);
   }
  m.set(ismatch, 256);

  if (bpos)
  {
    c=d; if (bpos==1)c+=c3/2;
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
public:
  Predictor();
  int p() const {assert(pr>=0 && pr<4096); return pr;}
  void update();
};

Predictor::Predictor(): pr(2048) {}


void Predictor::update() {
  static APM1 a(256), a1(0x10000), a2(0x10000), a3(0x10000),
                      a4(0x10000), a5(0x10000), a6(0x10000);
  static U32 tri[4]={0,4,3,7}, trj[4]={0,6,6,12};
  static U32 fails=0, failz=0, failcount=0,x5=0;
  // Update global context: pos, bpos, c0, c4, buf
c0+=c0+y;
if (c0>=256) {
	buf[pos++]=c0;
	c0-=256;
	c4=(c4<<8)+c0;
        int i=WRT_mpw[c0>>4];
		w4=w4*4+i;
		if (b2==3) i=2;
		w5=w5*4+i;
		b3=b2;
        b2=c0;   
	    x4=x4*256+c0,x5=(x5<<8)+c0;
	    if(c0=='.' || c0=='!' || c0=='?' || c0=='/'|| c0==')') {
			w5=(w5<<8)|0x3ff,f4=(f4&0xfffffff0)+2,x5=(x5<<8)+c0,x4=x4*256+c0;
            if(c0!='!') w4|=12, tt=(tt&0xfffffff8)+1,b3='.';
		}
		if (c0==32) --c0;
		tt=tt*8+WRT_mtt[c0>>4];
		f4=f4*16+(c0>>4);
	c0=1;
}
	
bpos=(bpos+1)&7;

  // Filter the context model with APMs
  int pr0=contextModel2();
  
 if (filetype==IMAGE1 || filetype==JPEG || filetype==AUDIO) {
      pr=pr0;
 }
 else {
     if (fails&0x00000080) --failcount;
        fails=fails*2;
        failz=failz*2;
        if (y) pr^=4095;
        if (pr>=1820) ++fails, ++failcount;
        if (pr>= 848) ++failz;
      int pv, pu,pz,pt;
        pu=(a.p(pr0, c0, 3)+7*pr0+4)>>3, pz=failcount+1;
        pz+=tri[(fails>>5)&3];
        pz+=trj[(fails>>3)&3];
        pz+=trj[(fails>>1)&3];
        if (fails&1) pz+=8;
        pz=pz/2;      
 
     int r=7;
     if (filetype==EXE )  r--;
   pu=a4.p(pu,   ( (c0*2)^hash(buf(1), (x5>>8)&255, (x5>>16)&0x80ff))&0xffff,r);
  pv=a2.p(pr0,  ( (c0*8)^hash(29,failz&2047))&0xffff,1+r);
  pv=a5.p(pv,           hash(c0,w5&0xfffff)&0xffff,r);
  pt=a3.p(pr0, ( (c0*32)^hash(19,     x5&0x80ffff))&0xffff,r);
  pz=a6.p(pu,   ( (c0*4)^hash(min(9,pz),x5&0x80ff))&0xffff,r);
  if (fails&255)  pr =(pt*6+pu  +pv*11+pz*14 +16)>>5;
  else		      pr =(pt*4+pu*5+pv*12+pz*11 +16)>>5;
}
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

  // Compress bit y or return decompressed bit
  void code(int i=0) {
    int p=predictor.p();
    assert(p>=0 && p<4096);
    p+=p<2048;
    U32 xmid=x1 + ((x2-x1)>>12)*p + (((x2-x1)&0xfff)*p>>12);
    assert(xmid>=x1 && xmid<x2);
    //if (mode==DECOMPRESS) y=x<=xmid; else y=i;
    y=i;
    i ? (x2=xmid) : (x1=xmid+1);
    predictor.update();
    while (((x1^x2)&0xff000000)==0) {  // pass equal leading bytes of range
      //if (mode==COMPRESS) 
      putc(x2>>24, archive);
      x1<<=8;
      x2=(x2<<8)+255;
      
     // if (mode==DECOMPRESS) x=(x<<8)+(getc(archive)&255);  // EOF is OK
    }
    //return i;
  }
  int decode() {
    int p=predictor.p();
    assert(p>=0 && p<4096);
    p+=p<2048;
    U32 xmid=x1 + ((x2-x1)>>12)*p + (((x2-x1)&0xfff)*p>>12);
    assert(xmid>=x1 && xmid<x2);
    //if (mode==DECOMPRESS)
    // y=x<=xmid;// else y=i;
    x<=xmid ? (x2=xmid,y=1) : (x1=xmid+1,y=0);
    predictor.update();
    while (((x1^x2)&0xff000000)==0) {  // pass equal leading bytes of range
     // if (mode==COMPRESS) putc(x2>>24, archive);
      x1<<=8;
      x2=(x2<<8)+255;
      //if (mode==DECOMPRESS) 
      x=(x<<8)+(getc(archive)&255);  // EOF is OK
    }
    return y;
  }
 
public:
  Encoder(Mode m, FILE* f);
  Mode getMode() const {return mode;}
  uint64_t size() const {return ftello(archive);}  // length of archive so far
  void flush();  // call this when compression is finished
  void setFile(FILE* f) {alt=f;}

  // Compress one byte
  void compress(int c) {
    assert(mode==COMPRESS);
    if (level==0)
      putc(c, archive);
    else {
      for (int i=7; i>=0; --i)
        code((c>>i)&1);
      pos=pos&poswr;   //wrap
      ++blpos;
    }
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
        c+=c+decode();
      ++blpos;
      pos=pos&poswr; //wrap
      return c;
    }
  }
};

Encoder::Encoder(Mode m, FILE* f):
    mode(m), archive(f), x1(0), x2(0xffffffff), x(0), alt(0) {
  if (level>0 && mode==DECOMPRESS) {  // x = first 4 bytes of archive
    for (int i=0; i<4; ++i)
      x=(x<<8)+(getc(archive)&255);
  }
  for (int i=0; i<1024; ++i)
    dt[i]=16384/(i+i+3);
}

void Encoder::flush() {
  if (mode==COMPRESS && level>0)
    putc(x1>>24, archive),putc(x1>>16, archive),putc(x1>>8, archive),putc(x1, archive);  // Flush first unequal byte of range
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
deth=int(header_len),detd=int((width)*(height)),info=int(width),\
fseeko(in, start+(start_pos), SEEK_SET),HDR

#define AUD_DET(type,start_pos,header_len,data_len,wmode) return dett=(type),\
deth=int(header_len),detd=(data_len),info=(wmode),\
fseeko(in, start+(start_pos), SEEK_SET),HDR

//Return only base64 data. No HDR.
#define B64_DET(type,start_pos,header_len,base64len) return dett=(type),\
deth=(-1),detd=int(base64len),\
fseeko(in, start+start_pos, SEEK_SET),DEFAULT

inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/')|| (c == 10) || (c == 13));
}
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
int wrtn=0;
// Detect EXE or JPEG data
Filetype detect(FILE* in, uint64_t n, Filetype type, int &info, int &info2, int it=0) {
  U32 buf1=0, buf0=0;  // last 8 bytes
  uint64_t start=ftello(in);

  // For EXE detection
  Array<uint64_t> abspos(256),  // CALL/JMP abs. addr. low byte -> last offset
    relpos(256);    // CALL/JMP relative addr. low byte -> last offset
  int e8e9count=0;  // number of consecutive CALL/JMPs
  uint64_t e8e9pos=0;    // offset of first CALL or JMP instruction
  uint64_t e8e9last=0;   // offset of most recent CALL or JMP

  uint64_t soi=0, sof=0, sos=0, app=0;  // For JPEG detection - position where found
  uint64_t wavi=0;
  int wavsize=0,wavch=0,wavbps=0,wavm=0;  // For WAVE detection
  uint64_t aiff=0;
  int aiffm=0,aiffs=0;  // For AIFF detection
  uint64_t s3mi=0;
  int s3mno=0,s3mni=0;  // For S3M detection
  uint64_t bmp=0;
  int imgbpp=0,bmpx=0,bmpy=0,bmpof=0;  // For BMP detection
  uint64_t rgbi=0;
  //int rgbx=0,rgby=0;  // For RGB detection
  uint64_t tga=0;
  //uint64_t tgax=0;
 // int tgay=0,tgaz=0,tgat=0;  // For TGA detection
  uint64_t pgm=0;
  //int pgmcomment=0,pgmw=0,pgmh=0,pgm_ptr=0,pgmc=0,pgmn=0;  // For PBM, PGM, PPM detection
  //Array<char> pgm_buf(32);
  uint64_t cdi=0;
  int cda=0,cdm=0;  // For CD sectors detection
  U32 cdf=0;
 // For TEXT
  uint64_t txtStart=0,txtLen=0,txtOff=0,txtbinc=0,txtbinp=0,txta=0,txt0=0;
  int utfc=0,utfb=0,txtIsUTF8=0; //utf count 2-6, current byte
  const int txtMinLen=1024*512;
  //const int txtSMinLen=1024*4;
  //base64
  uint64_t b64s=0,b64s1=0,b64p=0,b64slen=0,b64h=0;
  uint64_t base64start=0,base64end=0,b64line=0,b64nl=0,b64lcount=0;
  //int b64f=0,b64fstart=0,b64flen=0,b64send=0,b64fline=0; //force base64 detection


  // For image detection
  static int deth=0,detd=0;  // detected header/data size in bytes
  static Filetype dett;  // detected block type
  if (deth >1) return fseeko(in, start+deth, SEEK_SET),deth=0,dett;
  else if (deth ==-1) return fseeko(in, start, SEEK_SET),deth=0,dett;
  else if (detd) return fseeko(in, start+detd, SEEK_SET),detd=0,DEFAULT;

  for (uint64_t i=0; i<n; ++i) {
    int c=getc(in);
    if (c==EOF) return (Filetype)(-1);
    buf1=buf1<<8|buf0>>24;
    buf0=buf0<<8|c;

    // CD sectors detection (mode 1 and mode 2 form 1+2 - 2352 bytes)
    if (buf1==0x00ffffff && buf0==0xffffffff && !cdi) cdi=i,cda=-1,cdm=0;
    if (cdi && i>cdi) {
      const int p=(i-cdi)%2352;
      if (p==8 && (buf1!=0xffffff00 || ((buf0&0xff)!=1 && (buf0&0xff)!=2))) cdi=0;
      else if (p==16 && i+2336<n) {
        U8 data[2352];
        int64_t savedpos=ftello(in);
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

    if (!soi && i>=3 && (buf0&0xfffffff0)==0xffd8ffe0) soi=i, app=i+2, sos=sof=0;
    if (soi) {
      if (app==i && (buf0>>24)==0xff &&
         ((buf0>>16)&0xff)>0xc0 && ((buf0>>16)&0xff)<0xff) app=i+(buf0&0xffff)+2;
      if (app<i && (buf1&0xff)==0xff && (buf0&0xff0000ff)==0xc0000008) sof=i;
      if (sof && sof>soi && i-sof<0x1000 && (buf0&0xffff)==0xffda) {
        sos=i;
        if (type!=JPEG) return fseeko(in, start+soi-3, SEEK_SET), JPEG;
      }
      if (i-soi>0x40000 && !sos) soi=0;
    }
    if (type==JPEG && sos && i>sos && (buf0&0xff00)==0xff00
        && (buf0&0xff)!=0 && (buf0&0xf8)!=0xd0) return DEFAULT;
	
    // Detect .wav file header
    if (buf0==0x52494646) wavi=i,wavm=0;
    if (wavi) {
      const int p=int(i-wavi);
      if (p==4) wavsize=bswap(buf0);
      else if (p==8 && buf0!=0x57415645) wavi=0;
      else if (p==16 && (buf1!=0x666d7420 || bswap(buf0)!=16)) wavi=0;
      else if (p==22) wavch=bswap(buf0)&0xffff;
      else if (p==34) wavbps=bswap(buf0)&0xffff;
      else if (p==40+wavm && buf1!=0x64617461) wavm+=bswap(buf0)+8,wavi=(wavm>0xfffff?0:wavi);
      else if (p==40+wavm) {
        int wavd=bswap(buf0);
        if ((wavch==1 || wavch==2) && (wavbps==8 || wavbps==16) && wavd>0 && wavsize>=wavd+36
           && wavd%((wavbps/8)*wavch)==0) AUD_DET(AUDIO,wavi-3,44+wavm,wavd,wavch+wavbps/4-3);
        wavi=0;
      }
    }

    // Detect .aiff file header
    if (buf0==0x464f524d) aiff=i,aiffs=0; // FORM
    if (aiff) {
      const int p=int(i-aiff);
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
      int64_t savedpos=ftello(in);
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
      const int p=int(i-s3mi);
      if (p==4) s3mno=bswap(buf0)&0xffff,s3mni=(bswap(buf0)>>16);
      else if (p==16 && (((buf1>>16)&0xff)!=0x13 || buf0!=0x5343524d)) s3mi=0;
      else if (p==16) {
        int64_t savedpos=ftello(in);
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
    if ((buf0&0xffff)==16973) imgbpp=bmpx=bmpy=bmpof=0,bmp=i;  //possible 'BM'
    if (bmp) {
      const int p=int(i-bmp);
      if (p==12) bmpof=bswap(buf0);
      else if (p==16 && buf0!=0x28000000) bmp=0; //windows bmp?
      else if (p==20) bmpx=bswap(buf0),bmp=((bmpx==0||bmpx>0x40000)?0:bmp); //width
      else if (p==24) bmpy=abs((int)bswap(buf0)),bmp=((bmpy==0||bmpy>0x20000)?0:bmp); //height
      else if (p==27) imgbpp=c,bmp=((imgbpp!=1 && imgbpp!=4 && imgbpp!=8 && imgbpp!=24)?0:bmp);
      else if (p==31) {
        if (imgbpp!=0 && buf0==0 && bmpx>1) {
          if (imgbpp==1) IMG_DET(IMAGE1,bmp-1,bmpof,(((bmpx-1)>>5)+1)*4,bmpy);
          else if (imgbpp==4) IMG_DET(IMAGE4,bmp-1,bmpof,((bmpx>>1)+3)&-4,bmpy);
          else if (imgbpp==8) IMG_DET(IMAGE8,bmp-1,bmpof,(bmpx+3)&-4,bmpy);
          else if (imgbpp==24) IMG_DET(IMAGE24,bmp-1,bmpof,((bmpx*3)+3)&-4,bmpy);
        }
        bmp=0;
      }
    }
	
    // Detect .pbm .pgm .ppm image 
   /* if ((buf0&0xfff0ff)==0x50300a && txtLen<txtMinLen && start==0) { //see if text, only single files
      pgmn=(buf0&0xf00)>>8;
      if (pgmn>=4 && pgmn <=6) pgm=i,pgm_ptr=pgmw=pgmh=pgmc=pgmcomment=0;
    }
    if (pgm) {
      if (i-pgm==1 && c==0x23) pgmcomment=1; //pgm comment
      if (!pgmcomment && pgm_ptr) {
        int s=0;
        if (c==0x20 && !pgmw) s=1;
        else if (c==0x0a && !pgmh) s=2;
        else if (c==0x0a && !pgmc && pgmn!=4) s=3;
        if (s) {
          pgm_buf[pgm_ptr++]=0;
          int v=atoi(&pgm_buf[0]);
          if (v<5 || v>20000) v=0;
          if (s==1) pgmw=v; else if (s==2) pgmh=v; else if (s==3) pgmc=v;
          if (v==0 || (s==3 && v>255)) pgm=0; else pgm_ptr=0;
        }
      }
      if (!pgmcomment) pgm_buf[pgm_ptr++]=((c>='0' && c<='9') || ' ')?c:0;
      if (pgm_ptr>=32) pgm=pgm_ptr=0;
      if (i-pgm>255) pgm=pgm_ptr=0;
      if (pgmcomment && c==0x0a) pgmcomment=0;
      if (pgmw && pgmh && !pgmc && pgmn==4) IMG_DET(IMAGE1,pgm-2,i-pgm+3,(pgmw+7)/8,pgmh);
      if (pgmw && pgmh && pgmc && pgmn==5) IMG_DET(IMAGE8,pgm-2,i-pgm+3,pgmw,pgmh);
      if (pgmw && pgmh && pgmc && pgmn==6) IMG_DET(IMAGE24,pgm-2,i-pgm+3,pgmw*3,pgmh);
    }
	*/
    // Detect .rgb image
  /*  if ((buf0&0xffff)==0x01da) rgbi=i,rgbx=rgby=0;
    if (rgbi) {
      const int p=int(i-rgbi);
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
	*/
    // Detect .tiff file header (2/8/24 bit color, not compressed).
    if (buf1==0x49492a00 && n>i+(int)bswap(buf0)) {
      uint64_t savedpos=ftello(in);
      fseeko(in, start+i+(int)bswap(buf0)-7, SEEK_SET);

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
      if (tifx>1 && tify && tifzb && (tifz==1 || tifz==3) && (tifc==1) && (tifofs && tifofs+i<n)) {
        if (!tifofval) {
          fseeko(in, start+i+tifofs-7, SEEK_SET);
          for (int j=0; j<4; j++) b[j]=getc(in);
          tifofs=b[0]+(b[1]<<8)+(b[2]<<16)+(b[3]<<24);
        }
        if (tifofs && tifofs<(1<<18) && tifofs+i<n && tifx>1) {
          if (tifz==1 && tifzb==1) IMG_DET(IMAGE1,i-7,tifofs,((tifx-1)>>3)+1,tify);
          else if (tifz==1 && tifzb==8 && tifx<30000) IMG_DET(IMAGE8,i-7,tifofs,tifx,tify);
          else if (tifz==3 && tifzb==8 && tifx<30000) IMG_DET(IMAGE24,i-7,tifofs,tifx*3,tify);
        }
      }
      fseeko(in, savedpos, SEEK_SET);
    }
	
    // Detect .tga image (8-bit 256 colors or 24-bit uncompressed)
  /*  if (buf1==0x00010100 && buf0==0x00000118) tga=i,tgax=tgay,tgaz=8,tgat=1;
    else if (buf1==0x00000200 && buf0==0x00000000) tga=i,tgax=tgay,tgaz=24,tgat=2;
    else if (buf1==0x00000300 && buf0==0x00000000) tga=i,tgax=tgay,tgaz=8,tgat=3;
    if (tga) {
      if (i-tga==8) tga=(buf1==0?tga:0),tgax=(bswap(buf0)&0xffff),tgay=(bswap(buf0)>>16);
      else if (i-tga==10) {
        if (tgaz==(int)((buf0&0xffff)>>8) && tgax<30000 && tgax>1 && tgay) {
          if (tgat==1) IMG_DET(IMAGE8,tga-7,18+256*3,tgax,tgay);
          else if (tgat==2) IMG_DET(IMAGE24,tga-7,18,tgax*3,tgay);
          else if (tgat==3) IMG_DET(IMAGE8,tga-7,18,tgax,tgay);
        }
        tga=0;
      }
    }*/
	
    // Detect EXE if the low order byte (little-endian) XX is more
    // recently seen (and within 4K) if a relative to absolute address
    // conversion is done in the context CALL/JMP (E8/E9) XX xx xx 00/FF
    // 4 times in a row.  Detect end of EXE at the last
    // place this happens when it does not happen for 64KB.

    if (((buf1&0xfe)==0xe8 || (buf1&0xfff0)==0x0f80) && ((buf0+1)&0xfe)==0) {
      int r=buf0>>24;  // relative address low 8 bits
      int a=((buf0>>24)+i)&0xff;  // absolute address low 8 bits
      int rdist=int(i-relpos[r]);
      int adist=int(i-abspos[a]);
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
      if (type==EXE) return fseeko(in, start+e8e9last, SEEK_SET), DEFAULT;
      e8e9count=0,e8e9pos=0;
    }
  
    // base64 encoded data detection
    //
    // detect base64 in html/xml container, single stream
    // ';base64,' or '![CDATA['
    if (b64s1==0 && ((buf1==0x3b626173 && buf0==0x6536342c)||(buf1==0x215b4344 && buf0==0x4154415b))) b64s1=1,b64h=i+1,base64start=i+1; //' base64'
    else if (b64s1==1 && (isalnum(c) || (c == '+') || (c == '/')||(c == '=')) ) {
        }  
    else if (b64s1==1) {
         base64end=i,b64s1=0;
         if (base64end -base64start>128) B64_DET(BASE64,b64h, 8,base64end -base64start);
    }
   
   // detect base64 in eml, etc. multiline
   if (b64s==0 && buf0==0x73653634 && ((buf1&0xffffff)==0x206261 || (buf1&0xffffff)==0x204261)) b64s=1,b64p=i-6,b64h=0,b64slen=0,b64lcount=0; //' base64' ' Base64'
    else if (b64s==1 && buf0==0x0D0A0D0A ) {
        b64s=2,b64h=i+1,b64slen=b64h-b64p;
        base64start=i+1;
        if (b64slen>128) b64s=0; //drop if header is larger 
        }
    else if (b64s==2 && (buf0&0xffff)==0x0D0A && b64line==0) {
         b64line=i-base64start,b64nl=i+2;//capture line lenght
         if (b64line<=4 || b64line>255) b64s=0;
    }
    else if (b64s==2 && (buf0&0xffff)==0x0D0A  && b64line!=0 && (buf0&0xffffff)!=0x3D0D0A && buf0!=0x3D3D0D0A ){
         if (i-b64nl+1<b64line && buf0!=0x0D0A0D0A) { // if smaller and not padding
            base64end=i-1;
            if (base64end -base64start>512) {
             b64s=0;
             B64_DET(BASE64,b64h,b64slen,base64end -base64start);
            }
         }
         else if (buf0==0x0D0A0D0A) { // if smaller and not padding
           base64end=i-1-2;
           if (base64end -base64start>512) B64_DET(BASE64,b64h,b64slen,base64end -base64start);
           b64s=0;
         }
         b64nl=i+2; //update 0x0D0A pos
         b64lcount++;
         }
    else if (b64s==2 && ((buf0&0xffffff)==0x3D0D0A ||buf0==0x3D3D0D0A)) { //if padding '=' or '=='
        base64end=i-1;
        b64s=0;
        if (base64end -base64start>512) B64_DET(BASE64,b64h,b64slen,base64end -base64start);
    }
    else if (b64s==2 && !(is_base64(c) || c=='='))   b64s=0;
    
     // brute force base64 - need more work like end of line check & validation
     // like .ddoc files
   /* if (b64s==0 && b64f==0 && is_base64(c) && c!=10 && c!=13 ) b64f=1,b64fstart=i,b64send=0,b64fline==0;
    else if (b64f==1 && (buf0==0x0D0A0D0A) & (i -b64fstart<128)) b64f=b64fstart=b64send=b64fline=0;
    else if (b64f==1 && (is_base64(c) || (c==10 || c==13)) && b64send==0) {
        if((c==10 || c==13) && b64fline==0 ) b64fline=i-b64fstart;
        }
    else if (b64f==1 && !is_base64(c) ) {
        if ((buf0&0xff)==0x3D && b64send<1){ 
           b64send++;
        }
        else{
        if (b64send<2){
           if ((buf0&0xff)==0x3D) b64send++;
           if (i -b64fstart>(1024*4)) B64_DET(BASE64,b64fstart,0,i -b64fstart-1);
           b64f=b64fstart=b64send=0;
           }
           }
         }
    else  b64f=b64fstart=b64send=b64fline=0;*/

    
     //Detect text and utf-8 
    if (txtStart==0 && !soi && !pgm && !rgbi && !bmp && !wavi && !tga && !b64s1 && !b64s &&
        ((c<128 && c>=32) || c==10 || c==13 || c==0x12 || c==9 )) txtStart=1,txtOff=i,txt0=0,txta=0;
    if (txtStart   ) {
       if ((c<128 && c>=32) || c==0 || c==10 || c==13 || c==0x12 || c==9) {
            ++txtLen;
		if ((c>='a' && c<='z') ||  (c>='A' && c<='Z')) txta++;
		if (c>='0' && c<='9') txt0++;
            if (i-txtbinp>512) txtbinc=txtbinc>>2;
       }
       else if ((c&0xE0)==0xc0 && utfc==0){ //if possible UTF8 2 byte
            utfc=2,utfb=1;
            }
       else if ((c&0xF0)==0xE0 && utfc==0){//if possible UTF8 3 byte
            utfc=3,utfb=1;
            }
       else if ((c&0xF8)==0xF0 && utfc==0){//if possible UTF8 4 byte
            utfc=4,utfb=1;
            }
       else if ((c&0xFC)==0xF8 && utfc==0){ //if possible UTF8 5 byte
            utfc=5,utfb=1;
            }
       else if ((c&0xFE)==0xFC && utfc==0){//if possible UTF8 6 byte
            utfc=6,utfb=1;
            }
       else if (utfc>=2 && utfb>=1 && utfb<=6 && (c&0xC0)==0x80){
            utfb=utfb+1; //inc byte count
            //overlong UTF-8 sequence, UTF-16 surrogates as well as U+FFFE and U+FFFF test needed!!!
            switch (utfc){ //switch by detected type
            case 2:
                 if (utfb==2) txtLen=txtLen+2,utfc=0,utfb=0,txtIsUTF8=1;
                  break;
            case 3:
                 if (utfb==3) txtLen=txtLen+3,utfc=0,utfb=0,txtIsUTF8=1;
                  break;
            case 4:
                 if (utfb==4) txtLen=txtLen+4,utfc=0,utfb=0,txtIsUTF8=1;
                 break;
            case 5:
                 if (utfb==5) txtLen=txtLen+5,utfc=0,utfb=0,txtIsUTF8=1;
                 break;
            case 6:
                 if (utfb==6) txtLen=txtLen+6,utfc=0,utfb=0,txtIsUTF8=1;
                 break;
            }
       }
       else if (utfc>=2 && utfb>=1){ // wrong utf
             txtbinc=txtbinc+1;
             txtbinp=i;
             txtLen=txtLen+utfb;
             ++txtLen;
            utfc=utfb=txtIsUTF8=0;
            if (txtbinc>=128){
            if ( txtLen<txtMinLen) {
               txtStart=txtOff=txtLen=txtbinc=0;
				   txtIsUTF8=utfc=utfb=0;
            }
            else{
                    if (txta<txt0) wrtn=1; else wrtn=0; //use num0-9
                 if (type==TEXT ||type== TXTUTF8 ) return fseeko(in, start+txtLen, SEEK_SET),DEFAULT;
                if (txtIsUTF8==1)
                    return fseeko(in, start+txtOff, SEEK_SET),TXTUTF8;
                 else
                     return fseeko(in, start+txtOff, SEEK_SET),TEXT;
                     }
          
        }
       }
       else if (txtLen<txtMinLen) {
            txtbinc=txtbinc+1;
            txtbinp=i;
            ++txtLen;
            if (txtbinc>=128)
            txtStart=txtLen=txtOff=txtbinc=0;
			utfc=utfb=0;
       }
       else if (txtLen>=txtMinLen) {
            txtbinc=txtbinc+1;
            txtbinp=i;
            ++txtLen;
            if (txtbinc>=128){
                                 if (txta<txt0)   wrtn=1; else wrtn=0 ;
		       if (type==TEXT ||type== TXTUTF8 ) return fseeko(in, start+txtLen, SEEK_SET),DEFAULT;
                           
                 if (txtIsUTF8==1)
                    return fseeko(in, start+txtOff, SEEK_SET),TXTUTF8;
                 else
                     return fseeko(in, start+txtOff, SEEK_SET),TEXT;
                     }
      
   } 
    } 
  }
 if (txtStart) {
	   if ( txtLen>=txtMinLen) {
                                if (txta<txt0)   wrtn=1; else wrtn=0 ;
		   if (type==TEXT ||type== TXTUTF8 ) return fseeko(in, start+txtLen, SEEK_SET),DEFAULT;
         
                 if (txtIsUTF8==1)
                    return fseeko(in, start+txtOff, SEEK_SET),TXTUTF8;
                 else
                     return fseeko(in, start+txtOff, SEEK_SET),TEXT;
                     }
      
 }      
  return type;

}

typedef enum {FDECOMPRESS, FCOMPARE, FDISCARD} FMode;

// Print progress: n is the number of bytes compressed or decompressed
void printStatus(uint64_t n, uint64_t size) {
if (level>0)  printf("%6.2f%%\b\b\b\b\b\b\b", float(100)*n/(size+1)), fflush(stdout);
}

void encode_cd(FILE* in, FILE* out, int len, int info) {
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

int decode_cd(FILE *in, int size, FILE *out, FMode mode, uint64_t &diffFound) {
  const int BLOCK=2352;
  U8 blk[BLOCK];
  long i=0, i2=0;
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

void encode_bmp(FILE* in, FILE* out, int len, int width) {
  int r,g,b;
  for (int i=0; i<len/width; i++) {
    for (int j=0; j<width/3; j++) {
      b=fgetc(in), g=fgetc(in), r=fgetc(in);
      fputc(g, out);
      fputc(g-r, out);
      fputc(g-b, out);
    }
    for (int j=0; j<width%3; j++) fputc(fgetc(in), out);
  }
}

int decode_bmp(Encoder& en, int size, int width, FILE *out, FMode mode, uint64_t &diffFound) {
  int r,g,b,p;
  for (int i=0; i<size/width; i++) {
    p=i*width;
    for (int j=0; j<width/3; j++) {
      b=en.decompress(), g=en.decompress(), r=en.decompress();
      if (mode==FDECOMPRESS) {
        fputc(b-r, out);
        fputc(b, out);
        fputc(b-g, out);
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

// EXE transform: <encoded-size> <begin> <block>...
// Encoded-size is 4 bytes, MSB first.
// begin is the offset of the start of the input file, 4 bytes, MSB first.
// Each block applies the e8e9 transform to strings falling entirely
// within the block starting from the end and working backwards.
// The 5 byte pattern is E8/E9 xx xx xx 00/FF (x86 CALL/JMP xxxxxxxx)
// where xxxxxxxx is a relative address LSB first.  The address is
// converted to an absolute address by adding the offset mod 2^25
// (in range +-2^24).

void encode_exe(FILE* in, FILE* out, int len, int begin) {
  const int BLOCK=0x10000;
  Array<U8> blk(BLOCK);
  fprintf(out, "%c%c%c%c", begin>>24, begin>>16, begin>>8, begin);

  // Transform
  for (int offset=0; offset<len; offset+=BLOCK) {
    int size=min(int(len-offset), BLOCK);
    int bytesRead=fread(&blk[0], 1, size, in);
    if (bytesRead!=size) quit("encode_exe read error");
    for (int i=bytesRead-1; i>=5; --i) {
      if ((blk[i-4]==0xe8 || blk[i-4]==0xe9 || (blk[i-5]==0x0f && (blk[i-4]&0xf0)==0x80))
         && (blk[i]==0||blk[i]==0xff)) {
        int a=(blk[i-3]|blk[i-2]<<8|blk[i-1]<<16|blk[i]<<24)+offset+begin+i+1;
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

uint64_t decode_exe(Encoder& en, int size, FILE *out, FMode mode, uint64_t &diffFound, int s1=0, int s2=0) {
  const int BLOCK=0x10000;  // block size
  int begin, offset=6, a, showstatus=(s2!=0);
  U8 c[6];
  begin=en.decompress()<<24;
  begin|=en.decompress()<<16;
  begin|=en.decompress()<<8;
  begin|=en.decompress();
  size-=4;
  for (int i=4; i>=0; i--) c[i]=en.decompress();  // Fill queue

  while (offset<size+6) {
    memmove(c+1, c, 5);
    if (offset<=size) c[0]=en.decompress();
    // E8E9 transform: E8/E9 xx xx xx 00/FF -> subtract location from x
    if ((c[0]==0x00 || c[0]==0xFF) && (c[4]==0xE8 || c[4]==0xE9 || (c[5]==0x0F && (c[4]&0xF0)==0x80))
     && (((offset-1)^(offset-6))&-BLOCK)==0 && offset<=size) { // not crossing block boundary
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
    if (showstatus && !(offset&0xfff)) printStatus(s1+offset-6, s2);
    offset++;
  }
  return size;
}

FILE* tmpfile2(void){
	FILE *f;
#if defined(WINDOWS)	
	int i;
	char temppath[MAX_PATH]; 
	char filename[MAX_PATH];
	
	i=GetTempPath(MAX_PATH,temppath);
	if ((i==0) || (i>MAX_PATH)) return NULL;
	if (GetTempFileName(temppath,"tmp",0,filename)==0) return NULL;
	f=fopen(filename,"w+bTD");
	if (f==NULL) unlink(filename);
	return f;
#else
	f=tmpfile();  // temporary file
    if (!tmp) return NULL;
    return f;
#endif
}
//Based on XWRT 3.2 (29.10.2007) - XML compressor by P.Skibinski, inikep@gmail.com
#include "wrtpre.cpp"

void encode_txt(FILE* in, FILE* out, int len) {
   XWRT_Encoder* wrt;
   wrt=new XWRT_Encoder();
   wrt->defaultSettings(wrtn);
   wrt->WRT_start_encoding(in,out,len,false);
   delete wrt;
}

//called only when encode_txt output was smaller then input
int decode_txt(Encoder& en, int size, FILE *out, FMode mode, uint64_t &diffFound) {
    XWRT_Decoder* wrt;
    wrt=new XWRT_Decoder();
	FILE* dtmp;
	char c;
	int b=0;
    int bb=0;
	dtmp=tmpfile2();
	if (!dtmp) perror("ERR WRT tmpfile"), exit(1);
	//decompress file
	for (int i=0; i<size; i++) {
		c=en.decompress(); 
		putc(c,dtmp);	
	}
	fseeko(dtmp,0, SEEK_SET);
	wrt->defaultSettings(0);
	bb=wrt->WRT_start_decoding(dtmp);
    for ( int i=0; i<bb; i++) {
		b=wrt->WRT_decode();	
		if (mode==FDECOMPRESS) {
			fputc(b, out);
		}
		else if (mode==FCOMPARE) {
			if (b!=fgetc(out) && !diffFound) diffFound=i;
		}
	}
	fclose(dtmp);
	delete wrt;
	return bb; 
}

//
// decode/encode base64 
//
static const char  table1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
bool isbase64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/')|| (c == 10) || (c == 13));
}

int decode_base64(FILE *in, int size, FILE *out, FMode mode, uint64_t &diffFound){
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
    programChecker.alloc((outlen>>2)*4+10); //report used memory
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
    	
		
			if (b!=fgetc(out) && !diffFound) diffFound=ftello(out);
		}
    }
    free(ptr);
    programChecker.alloc(-(outlen+10));
    return outlen;
}
   
inline char valueb(char c){
       const char *p = strchr(table1, c);
       if(p) {
          return p-table1;
       } else {
          return 0;
       }
}

void encode_base64(FILE* in, FILE* out, int len) {
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
    programChecker.alloc(b64mem);
    fptr=&ptr[0];
    int olen=5;

  while (b=fgetc(in),in_len++ , ( b != '=') && is_base64(b) && in_len<=len) {
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
  fptr[2]=len>>8&255;
  fptr[3]=len>>16&255;
  if (tlf!=0) {
	if (tlf==10) fptr[4]=128;
    else fptr[4]=64;
  }
  else
  	fptr[4]=len>>24&63; //1100 0000
  fwrite(&ptr[0], 1, olen, out);
  free(ptr);
  programChecker.alloc(-b64mem);
}

//////////////////// Compress, Decompress ////////////////////////////

// put 4 bytes to segment buffer
inline void put4f(uint64_t num)
{
  segment[segment.pos++]=(num>>56)&0xff;
  segment[segment.pos++]=(num>>48)&0xff;
  segment[segment.pos++]=(num>>40)&0xff;
  segment[segment.pos++]=(num>>32)&0xff;
  segment[segment.pos++]=(num>>24)&0xff;
  segment[segment.pos++]=(num>>16)&0xff;
  segment[segment.pos++]=(num>>8)&0xff;
  segment[segment.pos++]=num&0xff;
  
}
inline void put4f4(int num)
{
  segment[segment.pos++]=(num>>24)&0xff;
  segment[segment.pos++]=(num>>16)&0xff;
  segment[segment.pos++]=(num>>8)&0xff;
  segment[segment.pos++]=num&0xff;
  
}
void direct_encode_block(Filetype type, FILE *in, uint64_t len, Encoder &en, uint64_t s1, uint64_t s2, int info=-1) {
  printf("\n");
  segment[segment.pos++]=type&0xff;
  put4f(len);
  filetype=type;
  blpos=0;
  finfo=0;
  if (type==JPEG) pos=0;
  if (info!=-1) {
    put4f4(info);
    finfo=info;
  }
 if (level>0)   printf("Compressing  ... ");
  const uint64_t total=s1+len+s2;
  for (uint64_t j=s1; j<s1+len; ++j) {
    if (!(j&0xfff)) printStatus(j, total);
    en.compress(getc(in));
  }
if (level>0)    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
}

//for block statistics, levels 0-5
uint64_t typenamess[14][5]={0}; //total type size for levels 0-5
U32 typenamesc[14][5]={0}; //total type count for levels 0-5
int itcount=0;			   //level count

void compressRecursive(FILE *in, uint64_t n, Encoder &en, char *blstr, int it=0, uint64_t s1=0, uint64_t s2=0) {
  static const char* typenames[13]={"default", "jpeg", "hdr",
    "1b-image", "4b-image", "8b-image", "24b-image", "audio",
    "exe", "cd", "text","utf-8","base64"};
  static const char* audiotypes[4]={"8b mono", "8b stereo", "16b mono",
    "16b stereo"};
  Filetype type=DEFAULT;
  int blnum=0, info,info2;  // image width or audio type
  uint64_t begin=ftello(in), end0=begin+n;
  FILE* tmp;
  char b2[32];
  strcpy(b2, blstr);
  if (b2[0]) strcat(b2, "-");
  if (it==5) {
    direct_encode_block(DEFAULT, in, n, en, s1, s2);
    return;
  }
  s2+=n;

  // Transform and test in blocks
  while (n>0) {
    Filetype nextType=detect(in, n, type, info,info2,it);
    uint64_t end=ftello(in);
    fseeko(in, begin, SEEK_SET);
    if (end>end0) {  // if some detection reports longer then actual size file is
      end=begin+1;
      type=DEFAULT;
    }
    uint64_t len=uint64_t(end-begin);
    
    if (len>0) {
    if (it>itcount)	itcount=it;
	typenamess[type][it]+=len,  typenamesc[type][it]++;
      s2-=len;
      sprintf(blstr,"%s%d",b2,blnum++);
      
      printf(" %-11s | %-9s |%10.0f b [%0.0f - %0.0f]",blstr,typenames[type],len+0.0,begin+0.0,end-1+0.0);
      if (type==AUDIO) printf(" (%s)", audiotypes[info%4]);
      else if (type==IMAGE1 || type==IMAGE4 || type==IMAGE8 || type==IMAGE24) printf(" (width: %d)", info);
      else if (type==CD) printf(" (m%d/f%d)", info==1?1:2, info!=3?1:2);
      
      if (type==EXE || type==CD || type==IMAGE24  || type==TEXT || type==TXTUTF8 || type==BASE64 ) {
        tmp=tmpfile2();  // temporary encoded file
        if (!tmp) perror("tmpfile"), quit();
        if (type==IMAGE24) encode_bmp(in, tmp, int(len), info);
        else if (type==EXE) encode_exe(in, tmp, int(len), int(begin));
        else if (type==TEXT || type==TXTUTF8 ) encode_txt(in, tmp, int(len));
        else if (type==BASE64) encode_base64(in, tmp, int(len));
        else if (type==CD) encode_cd(in, tmp, int(len), info);
        const uint64_t tmpsize=ftello(tmp);
        if ((type==TEXT || type==TXTUTF8) && (tmpsize<(len-256))) printf("(wt: %d)", int(tmpsize)); 
        rewind(tmp);
        en.setFile(tmp);
        fseeko(in, begin, SEEK_SET);
        uint64_t diffFound=0;
        if (type==IMAGE24) decode_bmp(en, int(tmpsize), info, in, FCOMPARE, diffFound);
        else if (type==EXE) decode_exe(en, int(tmpsize), in, FCOMPARE, diffFound);
        else if (type==TEXT || type==TXTUTF8) decode_txt(en, int(tmpsize), in, FCOMPARE, diffFound);
        else if (type==BASE64 ) decode_base64(tmp, int(tmpsize), in, FCOMPARE, diffFound);
        else if (type==CD) decode_cd(tmp, int(tmpsize), in, FCOMPARE, diffFound);

        // Test fails, compress without transform
        if ((diffFound || fgetc(tmp)!=EOF)) {
          printf("Transform fails at %0.0f, skipping...\n", diffFound-1+0.0);
          fseeko(in, begin, SEEK_SET);
          direct_encode_block(DEFAULT, in, len, en, s1, s2);
        } else {
          rewind(tmp);
          if (type==CD) {
            segment[segment.pos++]=type&0xff;
            put4f(tmpsize);
            filetype=type;
  			blpos=0;
  			finfo=0;
            compressRecursive(tmp, tmpsize, en, blstr, it+1, s1, s2);
          } else if (type==EXE) {
            direct_encode_block(type, tmp, tmpsize, en, s1, s2);
          } else if (type==IMAGE24) {
            direct_encode_block(type, tmp, tmpsize, en, s1, s2, info);
          } else if ((type==TEXT || type==TXTUTF8) ) {
                   if (tmpsize<(len-256)) //if WRT is smaller then original block
                      direct_encode_block(DICTTXT, tmp, tmpsize, en, s1, s2);
                   else {// encode as text
                      fseeko(in, begin, SEEK_SET);
                      direct_encode_block(type, in, len, en, s1, s2);}
          } else if ((type==BASE64) ) {
                      printf("\n");
                      segment[segment.pos++]=type&0xff;
            		  put4f(tmpsize);
            		  filetype=type;
  					  blpos=0;
  					  finfo=0;
            compressRecursive(tmp, tmpsize, en, blstr, it+1, s1, s2);
          }
        }
        fclose(tmp);  // deletes
      } else {
        const int i1=(type==IMAGE1 || type==IMAGE8 || type==IMAGE4 || type==AUDIO)?info:-1;
        direct_encode_block(type, in, len, en, s1, s2, i1);
      }
      s1+=len;
    }
    n-=len;
    type=nextType;
    begin=end;
  }
}

// Compress a file. Split filesize bytes into blocks by type.
// For each block, output
// <type> <size> and call encode_X to convert to type X.
// Test transform and compress.
void compress(const char* filename, uint64_t filesize, Encoder& en) {
  assert(en.getMode()==COMPRESS);
  assert(filename && filename[0]);
  FILE *in=fopen(filename, "rb");
  if (!in) perror(filename), quit();
  uint64_t start=en.size();
  printf("Block segmentation:\n");
  char blstr[32]="";
  compressRecursive(in, filesize, en, blstr);
  if (in) fclose(in);
  printf("Compressed from %0.0f to %0.0f bytes.\n",filesize+0.0,en.size()-start+0.0);
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


uint64_t decompressRecursive(FILE *out, uint64_t size, Encoder& en, FMode mode, int it=0, uint64_t s1=0, uint64_t s2=0) {
  Filetype type;
  uint64_t len=0L, i=0;
  uint64_t diffFound=0;
  int info=-1;
  FILE *tmp;
  s2+=size;
  while (i<size) {
    type=(Filetype)segment(segment.pos++);
	for (int ii=0; ii<8; ii++) len=len<<8,len+=segment(segment.pos++);
filetype=type;
  blpos=0;
  finfo=0;
  if (type==JPEG) pos=0;
    if (type==IMAGE1 || type==IMAGE8 || type==IMAGE4 || type==IMAGE24 || type==AUDIO) {
      info=0; for (int i=0; i<4; ++i) { info<<=8; info+=segment(segment.pos++);}
	  finfo=info;
    }
    if (type==IMAGE24) len=decode_bmp(en, int(len), info, out, mode, diffFound);
    else if (type==EXE) len=decode_exe(en, int(len), out, mode, diffFound, int(s1), int(s2));
    else if (type==DICTTXT) len=decode_txt(en, int(len), out, mode, diffFound);
    else if (type==BASE64) {
      tmp=tmpfile2();
      decompressRecursive(tmp, len, en, FDECOMPRESS, it+1, s1+i, s2-len);
      if (mode!=FDISCARD) {
        rewind(tmp);
        len=decode_base64(tmp, int(len), out, mode, diffFound);
      }
      fclose(tmp);
    }
    else if (type==CD) {
      tmp=tmpfile2();
      decompressRecursive(tmp, len, en, FDECOMPRESS, it+1, s1+i, s2-len);
      if (mode!=FDISCARD) {
        rewind(tmp);
        len=decode_cd(tmp, int(len), out, mode, diffFound);
      }
      fclose(tmp);
    } else {
      for (uint64_t j=i+s1; j<i+s1+len; ++j) {
        if (!(j&0xfff)) printStatus(j, s2);
        if (mode==FDECOMPRESS) putc(en.decompress(), out);
        else if (mode==FCOMPARE) {
          if (en.decompress()!=fgetc(out) && !diffFound) {
            mode=FDISCARD;
            diffFound=j+1;
          }
        } else en.decompress();
      }
    }
    i+=len;
  }
  return diffFound;
}

// Decompress a file
void decompress(const char* filename, uint64_t filesize, Encoder& en) {
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
  printf(" %s %0.0f -> ", filename, filesize+0.0);

  // Decompress/Compare
  uint64_t r=decompressRecursive(f, filesize, en, mode);
  if (mode==FCOMPARE && !r && getc(f)!=EOF) printf("file is longer\n");
  else if (mode==FCOMPARE && r) printf("differ at %0.0f\n",r-1+0.0);
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
    uint64_t len=ftello(f);
    if (len>=0) {
      static char blk[24];
      sprintf(blk, "%0.0f\t", len+0.0);
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


// To compress to file1.paq8pxd10: paq8pxd_v10 [-n] file1 [file2...]
// To decompress: paq8pxd_v10 file1.paq8pxd10 [output_dir]
int main(int argc, char** argv) {
  bool pause=argc<=2;  // Pause when done?
  try {

    // Get option
    bool doExtract=false;  // -d option
    bool doList=false;  // -l option
    if (argc>1 && argv[1][0]=='-' && argv[1][1] && !argv[1][2]) {
      if (argv[1][1]>='0' && argv[1][1]<='8')
        level=argv[1][1]-'0';
      else if (argv[1][1]=='d')
        doExtract=true;
      else if (argv[1][1]=='l')
        doList=true;
      else
        quit("Valid options are -0 through -8, -d, -l\n");
      --argc;
      ++argv;
      pause=false;
    }

    // Print help message
    if (argc<2) {
      printf(PROGNAME " archiver (C) 2008,2014, Matt Mahoney et al.\n"
        "Free under GPL, http://www.gnu.org/licenses/gpl.txt\n\n"
#ifdef WINDOWS
        "To compress or extract, drop a file or folder on the "
        PROGNAME " icon.\n"
        "The output will be put in the same folder as the input.\n"
        "\n"
        "Or from a command window: "
#endif
        "To compress:\n"
        "  " PROGNAME " -level file               (compresses to file." PROGNAME ")\n"
        "  " PROGNAME " -level archive files...   (creates archive." PROGNAME ")\n"
        "  " PROGNAME " file                      (level -%d, pause when done)\n"
        "level: -0 = store, -1 -2 -3 = faster (uses 35, 48, 59 MB)\n"
        "-4 -5 -6 -7 -8 = smaller (uses 133, 233, 435, 837, 1643 MB)\n"
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
        DEFAULT_OPTION);
      quit();
    }
	
    FILE* archive=0;  // compressed file
    int files=0;  // number of files to compress/decompress
    Array<const char*> fname(1);  // file names (resized to files)
    Array<uint64_t> fsize(1);   // file lengths (resized to files)

    // Compress or decompress?  Get archive name
    Mode mode=COMPRESS;
    String archiveName(argv[1]);
    {
      const int prognamesize=strlen(PROGNAME);
      const int arg1size=strlen(argv[1]);
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
      segment.setsize(2048); //inital segment buffer size (about 277 blocks)
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
      fprintf(archive, PROGNAME "%c%d", 0, level);
      segment.hpos=ftello(archive);
      
      for (int i=0; i<12; i++) putc(0,archive);
      
      printf("Creating archive %s with %d file(s)...\n",
        archiveName.c_str(), files);
    }

    // Decompress: open archive for reading and store file names and sizes
    if (mode==DECOMPRESS) {
      archive=fopen(archiveName.c_str(), "rb+");
      if (!archive) perror(archiveName.c_str()), quit();

      // Check for proper format and get option
      String header;
      int len=strlen(PROGNAME)+2, c, i=0;
      header.resize(len+1);
      while (i<len && (c=getc(archive))!=EOF) {
        header[i]=c;
        i++;
      }
      header[i]=0;
      if (strncmp(header.c_str(), PROGNAME "\0", strlen(PROGNAME)+1))
        printf("%s: not a %s file\n", archiveName.c_str(), PROGNAME), quit();
      level=header[strlen(PROGNAME)+1]-'0';
      if (level<0||level>8) level=DEFAULT_OPTION;
      
	  // Read segment data from archive end
	  uint64_t currentpos,datapos=0L;
	  for (int i=0; i<8; i++) datapos=datapos<<8,datapos+=getc(archive);
	  segment.hpos=datapos;
      segment.pos=getc(archive)<<24; //read segment data size
	  segment.pos+=getc(archive)<<16;
	  segment.pos+=getc(archive)<<8;
	  segment.pos+=getc(archive);
	  if (segment.hpos==0 || segment.pos==0) printf("Segment data not found."),quit();
	  segment.setsize(segment.pos);
      currentpos=ftello(archive);
      fseeko(archive, segment.hpos, SEEK_SET); 
      if (fread( &segment[0], 1, segment.pos, archive)<segment.pos) printf("Segment data corrupted."),quit();
	  fseeko(archive, currentpos, SEEK_SET); 
	  segment.pos=0; //reset to offset 0
    }

    // Set globals according to option
    assert(level>=0 && level<=8);
    buf.setsize(MEM*8);
    poswr=buf.size()-1;
    #ifdef DEBUG 
	printf("\n Buf size %d bytes)\n", poswr);
	#endif
    Encoder en(mode, archive);

    // Compress header
    if (mode==COMPRESS) {
      int len=header_string.size();
      printf("\nFile list (%d bytes)\n", len);
      assert(en.getMode()==COMPRESS);
      uint64_t start=en.size();
      en.compress(0); // block type 0
      en.compress(len>>24); en.compress(len>>16); en.compress(len>>8); en.compress(len); // block length
      for (int i=0; i<len; i++) en.compress(header_string[i]);
      printf("Compressed from %d to %0.0f bytes.\n",len,en.size()-start+0.0);
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
	#if defined(WINDOWS) || defined(_MSC_VER)
	#define atoll(S) _atoi64(S)
	#endif
    // Fill fname[files], fsize[files] with input filenames and sizes
    fname.resize(files);
    fsize.resize(files);
    char *p=&header_string[0];
    char* q=&filenames[0];
    for (int i=0; i<files; ++i) {
      assert(p);
      fsize[i]=atoll(p);
      assert(fsize[i]>=0);
      while (*p!='\t') ++p; *(p++)='\0';
      fname[i]=mode==COMPRESS?q:p;
      while (*p!='\n') ++p; *(p++)='\0';
      if (mode==COMPRESS) { while (*q!='\n') ++q; *(q++)='\0'; }
    }
    // Compress or decompress files
    assert(fname.size()==files);
    assert(fsize.size()==files);
    uint64_t total_size=0;  // sum of file sizes
    for (int i=0; i<files; ++i) total_size+=fsize[i];
    if (mode==COMPRESS) {
      for (int i=0; i<files; ++i) {
        printf("\n%d/%d  Filename: %s (%0.0f bytes)\n", i+1, files, fname[i], fsize[i]+0.0);
        compress(fname[i], fsize[i], en);
      }
      en.flush();

	  // Write out segment data
      uint64_t segmentpos;
      segmentpos=ftello(archive); //get segment data offset
      fseeko(archive, segment.hpos, SEEK_SET);
	  putc(segmentpos>>56&0xff,archive); //write segment data offset
      putc(segmentpos>>48&0xff,archive);
      putc(segmentpos>>40&0xff,archive);
      putc(segmentpos>>32&0xff,archive);
      putc(segmentpos>>24&0xff,archive); 
      putc(segmentpos>>16&0xff,archive);
      putc(segmentpos>>8&0xff,archive);
      putc(segmentpos&0xff,archive);

      putc(segment.pos>>24&0xff,archive); //write segment data size
      putc(segment.pos>>16&0xff,archive);
      putc(segment.pos>>8&0xff,archive);
      putc(segment.pos&0xff,archive);
      fseeko(archive, segmentpos, SEEK_SET); 
      fwrite(&segment[0], 1, segment.pos, archive); //write out segment data

	  printf("\nTotal %0.0f bytes compressed to %0.0f bytes.\n", total_size+0.0, en.size()+0.0); 
      printf("\n Segment data size: %d bytes\n",segment.pos);
      //Display Level statistics
      const char* typenames[13]={"default", "jpeg", "hdr", "1b-image", "4b-image", "8b-image", "24b-image", "audio",
    							"exe", "cd", "text","utf-8","base64"};
      U32 ttc;
	  uint64_t tts;
      for (int j=0; j<=itcount; ++j) {
      	printf("\n %-2s |%-9s |%-10s |%-10s\n","TN","Type name", "Count","Total size");
    	printf("-----------------------------------------\n");
    	ttc=0,tts=0;
     	for (int i=0; i<13; ++i)   if (typenamess[i][j]) printf(" %2d |%-9s |%10d |%11.0f\n",i,typenames[i], typenamesc[i][j],typenamess[i][j]+0.0),ttc+=typenamesc[i][j],tts+=typenamess[i][j];
     	printf("-----------------------------------------\n");
     	printf("%-13s%1d |%10d |%11.0f\n\n","Total level",j, ttc,tts+0.0);
      }
  }
    // Decompress files to dir2: paq8pxd_v10 -d dir1/archive.paq8pxd10 dir2
    // If there is no dir2, then extract to dir1
    // If there is no dir1, then extract to .
    else if (!doList) {
      assert(argc>=2);
      String dir(argc>2?argv[2]:argv[1]);
      if (argc==2) {  // chop "/archive.paq8pxd10"
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
      for (int i=0; i<files; ++i) {
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