/* PAQ8H file compressor/archiver. Release by Marwijn Hessel, May. 10, 2013

(C) 2006, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt

PAQ8H is a file compressor and archiver.  To use:

  paq8h archive files...

If the archive does not exist, then it is created and the files are
compresed and stored in the archive.  File names are stored in the
archive exactly as entered (with or without a path).

If the archive already exists, then the files are extracted in the
same order as they were stored.  If the file names are omitted,
then the stored names are used instead.

PAQ8H never clobbers files.  If the file to be extracted already
exists, then PAQ8H compares it with the archived copy and reports
whether they differ.  Example:

  paq8h xyz foo bar      Compresses foo and bar to archive xyz.
  paq8h xyz baz          Extracts baz as a copy of foo, compares bar.

If PAQ8H is run under UNIX or Linix, or compiled with g++ for Windows,
then wildcards may be used.  You can also enter the file names, one
per line to standard input ending with a blank line or EOF.  Example:

  dir/b *.txt | paq8h archive

is equivalent to:

  paq8h archive *.txt

The archive has a human-readable header.  To view the stored file
names and sizes:

  more < archive

PAQ8H cannot update an archive.  You must create a new one instead.
You cannot extract a subset of the stored files, but you can interrupt
it with ^C after the files you want are extracted.

Neither the archive nor the total size of all files can be larger than
2 gigabytes, even if your file system supports such large files.  File
names (including path) cannot be longer than 511 characters or contain
carriage returns, linefeeds, ^Z (ASCII 26) or NUL (ASCII 0).  If any file
names contain 8-bit (European) extended ASCII, then do not pipe from
dir/b, use wildcards.  File names with spaces on the command line
should be quoted "like this".


TO COMPILE


There are 2 files: paq8h.cpp (C++) and textfilter.hpp.
paq8h.cpp recognizes the following compiler options:

  -DWIN32             (to compile in Windows, already set in Microsoft Visual C++)
  -DDEFAULT_OPTION=N  (to change the default compression level from 4 to N).

Use -DEFAULT_OPTION=N to change the default compression level to support
drag and drop on machines with less than 256 MB of memory.  Use
-DDEFAULT_OPTION=4 for 128 MB, 3 for 64 MB, 2 for 32 MB, etc.

Recommended compiler commands and optimizations:

  MINGW g++:
    g++ paq8h.cpp -Wall -Wextra -O3 -s -march=pentium4 -mtune=pentium4 -fomit-frame-pointer -o paq8h.exe

  Borland:
    bcc32 -O -w-8027 paq8h.cpp

  Mars:
    dmc -Ae -O paq8h.cpp

  UNIX/Linux (PC):
    g++ paq8h.cpp -Wall -Wextra -O3 -s -march=pentium4 -mtune=pentium4 -fomit-frame-pointer -o paq8h

  Non PC (e.g. PowerPC under MacOS X)
    g++ paq8h.cpp -Wall -Wextra -O3 -s -o paq8h

MinGW produces faster executables than Borland or Mars, but Intel 9
is about 4% faster than MinGW).


ARCHIVE FILE FORMAT

An archive has the following format.  It is intended to be both
human and machine readable.  The header ends with CTRL-Z (Windows EOF)
so that the binary compressed data is not displayed on the screen.

  paq8h -N CR LF
  size TAB filename CR LF
  size TAB filename CR LF
  ...
  CTRL-Z
  compressed binary data

-N is the option (-0 to -9), even if a default was used.

Decompression will fail if the first 7 bytes are not "paq8h -".  Sizes
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

paq8h uses a neural network to combine a large number of models.  The
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
  a stationary map with K = 1/256, initiaized to 
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

- Image.  For uncompressed 24-bit color BMP and TIFF images.  Contexts
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

ARCHITECTURE

The context models are mixed by up to 4 of several hundred neural networks
selected by a low-order context.  The outputs of these networks are
combined using a second neural network, then fed through two stages of 
adaptive probability maps (APM) before arithmetic coding.  The four neural 
networks in the first layer are selected by the following contexts:

- NN1: Order 0: the current (partially coded) byte, and the number of
       bits coded (0-7).
- NN2: The previous whole byte (does not include current byte).
- NN3: The second from last byte.
- NN4: The file type (image, long match, or normal), order (0-7), 3 high bits
       of the previous byte, and whether the last 2 bytes are equal.

For images and long matches, only one neural network is used and its
context is fixed.

An APM is a stationary map combining a context and an input probability.
The input probability is stretched and divided into 32 segments to
combine with other contexts.  The output is interpolated between two
adjacent quantized values of stretch(p1).  There are 2 APM stages in series:

  p1 := (p1 + 3 APM(order 0, p1)) / 4.
  p1 := (APM(order 1, p1) + 2 APM(order 2, p1) + APM(order 3, p1)) / 4.

PREPROCESSING

PAQ8H uses preprocessing transforms on certain file types to improve
compression.  To improve reliability, the decoding transform is
tested during compression to ensure that the input file can be
restored.  If the decoder output is not identical to the input file
due to a bug, then the transform is abandoned.

Each transform consists of a coder and decoder which have the following
properties:

- Coder.  Input is a file to be compressed.  Output is a temporary
  file.  The coder determines whether a transform is to be applied
  based on file type, and if so, which one.  A coder may use lots
  of resources (memory, time) and make multiple passes through the
  input file.  The file type is stored (as one byte) during compression.

- Decoder.  Performs the inverse transform of the coder.  It uses few
  resorces (fast, low memory) and runs in a single pass (stream oriented).
  It takes input either from a file or the arithmetic decoder.  Each call
  to the decoder returns a single decoded byte.

Compression for each file is as follows:

  Determine file type (0 = no transform, 1-255 = transform type).
  If a transform is to be applied then:
    Transform to a temporary file.
    Report file type and temporary file size.
    Decode temporary file and compare with input file.
    If file mismatches or decoder reads wrong number of bytes then:
      Set file type = 0.
      Print a warning.
  If transform number > 0 then:
    Compress file type (1-255) as one byte.
    Compress temporary file, reporting progress.
  Else:
    Compress 0 byte.
    Compress input file, reporting progress.
  If temporary file exists then delete it.

Decompression:

  Decompress one byte to select file type.
  For each byte in original file (from header) do:
    If file type > 0 then
      read one byte from decoder (which reads from arithmetic encoder)
    Else read one byte from arithmetic encoder.
    Report progress.
    If output file exists then compare byte to it
    Else output byte.
  Report result (extracted, identical, not identical)

The following transforms are used:

- x86 (type 1).  Detected by .exe, .obj, or .dll file name extension.
  CALL (0xE8) and JMP (0xE9) address operands are converted from relative
  to absolute address.  The transform is to replace the sequence
  E8/E9 xx xx xx 00/FF by adding file offset modulo 2^25 (signed range,
  little-endian format).  Transform stops if an embedded JPEG is detected.


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


DIFFERENCES FROM PAQ8A
Resolved a lot of compiler warnings
Replaced NN assembly into C code without compatibility drop (instead it's faster then the assembler version)
Corrected the DMC implementation, this breaks the compatibility!
Replaced 'inline' by 'static' so that the compiler can inline code where possible (depending on the optimization flags)

*/

#define PROGNAME "paq8h" // Please change this if you change the program.

#include <algorithm>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define NDEBUG // remove for debugging (turns on Array bound checks)
#include <assert.h>

#ifndef DEFAULT_OPTION
#  define DEFAULT_OPTION 4
#endif

// 8, 16, 32 bit unsigned types (adjust as appropriate)
using sint8 = char;          ///<                signed  8 bits integer
using sint16 = short; ///<       signed 16 bits integer
using sint32 = int;   ///<         signed 32 bits integer

using uint8 = unsigned char;   ///<       unsigned  8 bits integer
using uint16 = unsigned short; ///<     unsigned 16 bits integer
using uint32 = unsigned int;   ///<       unsigned 32 bits integer

/** Computes the minimum of \a x and \a y. */
#if !defined( MIN )
#  define MIN( x, y ) ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )
#endif

/** Computes the maximum of \a x and \a y. */
#if !defined( MAX )
#  define MAX( x, y ) ( ( ( x ) < ( y ) ) ? ( y ) : ( x ) )
#endif
using std::max;

#if defined( __GNUC__ )
static void quit( const sint8 * ) __attribute__( ( noreturn ) );
#endif // __GNUC__

// Error handler: print message if any, and exit
static void quit( const char *message = 0 ) {
  throw message;
}

typedef enum { DEFAULT, JPEG, EXE, BINTEXT, TEXT } Filetype;

int fsize, filetype = DEFAULT; // Set by Filter
#define preprocFlag 1220
long size;

#define OPTION_UTF8 1
#define OPTION_USE_NGRAMS 2
#define OPTION_CAPITAL_CONVERSION 4
#define OPTION_WORD_SURROROUNDING_MODELING 8
#define OPTION_SPACE_AFTER_EOL 16
#define OPTION_EOL_CODING 32
#define OPTION_NORMAL_TEXT_FILTER 64
#define OPTION_USE_DICTIONARY 128
#define OPTION_RECORD_INTERLEAVING 256
#define OPTION_DNA_QUARTER_BYTE 512
#define OPTION_TRY_SHORTER_WORD 1024
#define OPTION_TO_LOWER_AFTER_PUNCTUATION 2048
#define OPTION_SPACELESS_WORDS 4096
#define OPTION_ADD_SYMBOLS_0_5 8192
#define OPTION_ADD_SYMBOLS_14_31 16384
#define OPTION_ADD_SYMBOLS_A_Z 32768
#define OPTION_ADD_SYMBOLS_MISC 65536
#define OPTION_SPACE_AFTER_CC_FLAG 131072
#define IF_OPTION( option ) ( ( preprocFlag & option ) != 0 )

//////////////////////// Program Checker /////////////////////

// Track time and memory used
class ProgramChecker {
  int memused{ 0 };        // bytes allocated by Array<T> now
  clock_t start_time; // in ticks
public:
  int maxmem{ 0 };           // most bytes allocated ever
  void alloc( int n ) { // report memory allocated, may be negative
    memused += n;
    if( memused > maxmem )
      maxmem = memused;
  }
  ProgramChecker()  {
    start_time = clock();
    assert( sizeof( uint8 ) == 1 );
    assert( sizeof( uint16 ) == 2 );
    assert( sizeof( uint32 ) == 4 );
    assert( sizeof( short ) == 2 );
    assert( sizeof( int ) == 4 );
  }
  void print() const { // print time and memory used
    printf( "Time %1.2f sec, used %d bytes of memory\n", double( clock() - start_time ) / CLOCKS_PER_SEC, maxmem );
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

template <class T, int ALIGN = 0>
class Array {
private:
  int n;                // user size
  int reserved;         // actual size
  char *ptr;            // allocated memory, zeroed
  T *data;              // start of n elements of aligned data
  void create( int i ); // create with size i
public:
  explicit Array( int i = 0 ) {
    create( i );
  }
  virtual ~Array();
  T &operator[]( int i ) {
#ifndef NDEBUG
    if( i < 0 || i >= n )
      fprintf( stderr, "%d out of bounds %d\n", i, n ), quit();
#endif
    return data[i];
  }
  const T &operator[]( int i ) const {
#ifndef NDEBUG
    if( i < 0 || i >= n )
      fprintf( stderr, "%d out of bounds %d\n", i, n ), quit();
#endif
    return data[i];
  }
  int size() const {
    return n;
  }
  void resize( int i ); // change size to i
  void pop_back() {
    if( n > 0 )
      --n;
  }                             // decrement size
  void push_back( const T &x ); // increment size, append x
private:
  Array( const Array & ); // no copy or assignment
  Array &operator=( const Array & );
};

template <class T, int ALIGN>
void Array<T, ALIGN>::resize( int i ) {
  if( i <= reserved ) {
    n = i;
    return;
  }
  char *saveptr = ptr;
  T *savedata = data;
  int saven = n;
  create( i );
  if( savedata && saveptr ) {
    memcpy( data, savedata, sizeof( T ) * MIN( i, saven ) );
    programChecker.alloc( -ALIGN - n * sizeof( T ) );
    free( saveptr );
  }
}

template <class T, int ALIGN>
void Array<T, ALIGN>::create( int i ) {
  n = reserved = i;
  if( i <= 0 ) {
    data = 0;
    ptr = 0;
    return;
  }
  const int sz = ALIGN + n * sizeof( T );
  programChecker.alloc( sz );
  ptr = ( char * ) calloc( sz, 1 );
  if( ptr == nullptr )
    quit( "Out of memory" );
  data = ( ALIGN != 0 ? ( T * ) ( ptr + ALIGN - ( ( ( long ) ptr ) & ( ALIGN - 1 ) ) ) : ( T * ) ptr );
  assert( ( char * ) data >= ptr && ( char * ) data <= ptr + ALIGN );
}

template <class T, int ALIGN>
Array<T, ALIGN>::~Array() {
  programChecker.alloc( -ALIGN - n * sizeof( T ) );
  free( ptr );
}

template <class T, int ALIGN>
void Array<T, ALIGN>::push_back( const T &x ) {
  if( n == reserved ) {
    int saven = n;
    resize( max( 1, n * 2 ) );
    n = saven;
  }
  data[n++] = x;
}

/////////////////////////// String /////////////////////////////

// A tiny subset of std::string
// size() includes NUL terminator.

class String : public Array<char> {
public:
  const char *c_str() const {
    return &( *this )[0];
  }
  void operator=( const char *s ) {
    resize( strlen( s ) + 1 );
    strcpy( &( *this )[0], s );
  }
  void operator+=( const char *s ) {
    assert( s );
    pop_back();
    while( *s != 0 )
      push_back( *s++ );
    push_back( 0 );
  }
  String( const char *s = "" ) : Array<char>( 1 ) {
    ( *this ) += s;
  }
};

//////////////////////////// rnd ///////////////////////////////

// 32-bit pseudo random number generator
static class Random {
  uint32 table[64];
  sint32 i;

public:
  Random() {
    table[0] = 123456789;
    table[1] = 987654321;
    for( int j = 0; j < 62; j++ )
      table[j + 2] = table[j + 1] * 11 + table[j] * 23 / 16;
    i = 0;
  }
  uint32 operator()() {
    return ++i, table[i & 63] = table[i - 24 & 63] ^ table[i - 55 & 63];
  }
} rnd;

////////////////////////////// Buf /////////////////////////////

// Buf(n) buf; creates an array of n bytes (must be a power of 2).
// buf[i] returns a reference to the i'th byte with wrap (no out of bounds).
// buf(i) returns i'th byte back from pos (i > 0)
// buf.size() returns n.

static int pos; // Number of input bytes in buf (not wrapped)

class Buf {
  Array<uint8> b;

public:
  Buf( int i = 0 ) : b( i ) {}
  void setsize( int i ) {
    if( i == 0 )
      return;
    assert( i > 0 && ( i & ( i - 1 ) ) == 0 );
    b.resize( i );
  }
  uint8 &operator[]( int i ) {
    return b[i & b.size() - 1];
  }
  int operator()( int i ) const {
    assert( i > 0 );
    return b[pos - i & b.size() - 1];
  }
  int size() const {
    return b.size();
  }
};

/////////////////////// Global context /////////////////////////

static int level = DEFAULT_OPTION; // Compression level 0 to 9
#define MEM ( 0x10000 << level )
static int y = 0; // Last bit, 0 or 1, set by encoder

// Global context set by Predictor and available to all models.
static int c0 = 1; // Last 0-7 bits of the partial byte with a leading 1 bit (1-255)
static uint32 b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0, b8 = 0, tt = 0, c4 = 0, x4 = 0, x5 = 0, w4 = 0,
              w5 = 0, f4 = 0; // Last 4 whole bytes, packed.  Last byte is bits 0-7.
static int order, bpos = 0, cxtfl = 3, sm_shft = 7, sm_add = 65535 + 127, sm_add_y = 0; // bits in c0 (0 to 7)
static Buf buf; // Rotating input queue set by Predictor

///////////////////////////// ilog //////////////////////////////

// ilog(x) = round(log2(x) * 16), 0 <= x < 64K
static class Ilog {
  uint8 t[65536];

public:
  // Compute lookup table by numerical integration of 1/x
  Ilog() {
    uint32 x = 14155776;
    for( int i = 2; i < 65536; ++i ) {
      x += 774541002 / ( i * 2 - 1 ); // numerator is 2^29/ln 2
      t[i] = x >> 24;
    }
  }
  int operator()( uint16 x ) const {
    return t[x];
  }
} ilog;

// llog(x) accepts 32 bits
int llog( uint32 x ) {
  if( x >= 0x1000000 )
    return 256 + ilog( x >> 16 );
  if( x >= 0x10000 )
    return 128 + ilog( x >> 8 );
  else
    return ilog( x );
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
static const uint8 State_table[256][4] = {
    {1, 2, 0, 0},      {3, 5, 1, 0},      {4, 6, 0, 1},      {7, 10, 2, 0},     // 0-3
    {8, 12, 1, 1},     {9, 13, 1, 1},     {11, 14, 0, 2},    {15, 19, 3, 0},    // 4-7
    {16, 23, 2, 1},    {17, 24, 2, 1},    {18, 25, 2, 1},    {20, 27, 1, 2},    // 8-11
    {21, 28, 1, 2},    {22, 29, 1, 2},    {26, 30, 0, 3},    {31, 33, 4, 0},    // 12-15
    {32, 35, 3, 1},    {32, 35, 3, 1},    {32, 35, 3, 1},    {32, 35, 3, 1},    // 16-19
    {34, 37, 2, 2},    {34, 37, 2, 2},    {34, 37, 2, 2},    {34, 37, 2, 2},    // 20-23
    {34, 37, 2, 2},    {34, 37, 2, 2},    {36, 39, 1, 3},    {36, 39, 1, 3},    // 24-27
    {36, 39, 1, 3},    {36, 39, 1, 3},    {38, 40, 0, 4},    {41, 43, 5, 0},    // 28-31
    {42, 45, 4, 1},    {42, 45, 4, 1},    {44, 47, 3, 2},    {44, 47, 3, 2},    // 32-35
    {46, 49, 2, 3},    {46, 49, 2, 3},    {48, 51, 1, 4},    {48, 51, 1, 4},    // 36-39
    {50, 52, 0, 5},    {53, 43, 6, 0},    {54, 57, 5, 1},    {54, 57, 5, 1},    // 40-43
    {56, 59, 4, 2},    {56, 59, 4, 2},    {58, 61, 3, 3},    {58, 61, 3, 3},    // 44-47
    {60, 63, 2, 4},    {60, 63, 2, 4},    {62, 65, 1, 5},    {62, 65, 1, 5},    // 48-51
    {50, 66, 0, 6},    {67, 55, 7, 0},    {68, 57, 6, 1},    {68, 57, 6, 1},    // 52-55
    {70, 73, 5, 2},    {70, 73, 5, 2},    {72, 75, 4, 3},    {72, 75, 4, 3},    // 56-59
    {74, 77, 3, 4},    {74, 77, 3, 4},    {76, 79, 2, 5},    {76, 79, 2, 5},    // 60-63
    {62, 81, 1, 6},    {62, 81, 1, 6},    {64, 82, 0, 7},    {83, 69, 8, 0},    // 64-67
    {84, 71, 7, 1},    {84, 71, 7, 1},    {86, 73, 6, 2},    {86, 73, 6, 2},    // 68-71
    {44, 59, 5, 3},    {44, 59, 5, 3},    {58, 61, 4, 4},    {58, 61, 4, 4},    // 72-75
    {60, 49, 3, 5},    {60, 49, 3, 5},    {76, 89, 2, 6},    {76, 89, 2, 6},    // 76-79
    {78, 91, 1, 7},    {78, 91, 1, 7},    {80, 92, 0, 8},    {93, 69, 9, 0},    // 80-83
    {94, 87, 8, 1},    {94, 87, 8, 1},    {96, 45, 7, 2},    {96, 45, 7, 2},    // 84-87
    {48, 99, 2, 7},    {48, 99, 2, 7},    {88, 101, 1, 8},   {88, 101, 1, 8},   // 88-91
    {80, 102, 0, 9},   {103, 69, 10, 0},  {104, 87, 9, 1},   {104, 87, 9, 1},   // 92-95
    {106, 57, 8, 2},   {106, 57, 8, 2},   {62, 109, 2, 8},   {62, 109, 2, 8},   // 96-99
    {88, 111, 1, 9},   {88, 111, 1, 9},   {80, 112, 0, 10},  {113, 85, 11, 0},  // 100-103
    {114, 87, 10, 1},  {114, 87, 10, 1},  {116, 57, 9, 2},   {116, 57, 9, 2},   // 104-107
    {62, 119, 2, 9},   {62, 119, 2, 9},   {88, 121, 1, 10},  {88, 121, 1, 10},  // 108-111
    {90, 122, 0, 11},  {123, 85, 12, 0},  {124, 97, 11, 1},  {124, 97, 11, 1},  // 112-115
    {126, 57, 10, 2},  {126, 57, 10, 2},  {62, 129, 2, 10},  {62, 129, 2, 10},  // 116-119
    {98, 131, 1, 11},  {98, 131, 1, 11},  {90, 132, 0, 12},  {133, 85, 13, 0},  // 120-123
    {134, 97, 12, 1},  {134, 97, 12, 1},  {136, 57, 11, 2},  {136, 57, 11, 2},  // 124-127
    {62, 139, 2, 11},  {62, 139, 2, 11},  {98, 141, 1, 12},  {98, 141, 1, 12},  // 128-131
    {90, 142, 0, 13},  {143, 95, 14, 0},  {144, 97, 13, 1},  {144, 97, 13, 1},  // 132-135
    {68, 57, 12, 2},   {68, 57, 12, 2},   {62, 81, 2, 12},   {62, 81, 2, 12},   // 136-139
    {98, 147, 1, 13},  {98, 147, 1, 13},  {100, 148, 0, 14}, {149, 95, 15, 0},  // 140-143
    {150, 107, 14, 1}, {150, 107, 14, 1}, {108, 151, 1, 14}, {108, 151, 1, 14}, // 144-147
    {100, 152, 0, 15}, {153, 95, 16, 0},  {154, 107, 15, 1}, {108, 155, 1, 15}, // 148-151
    {100, 156, 0, 16}, {157, 95, 17, 0},  {158, 107, 16, 1}, {108, 159, 1, 16}, // 152-155
    {100, 160, 0, 17}, {161, 105, 18, 0}, {162, 107, 17, 1}, {108, 163, 1, 17}, // 156-159
    {110, 164, 0, 18}, {165, 105, 19, 0}, {166, 117, 18, 1}, {118, 167, 1, 18}, // 160-163
    {110, 168, 0, 19}, {169, 105, 20, 0}, {170, 117, 19, 1}, {118, 171, 1, 19}, // 164-167
    {110, 172, 0, 20}, {173, 105, 21, 0}, {174, 117, 20, 1}, {118, 175, 1, 20}, // 168-171
    {110, 176, 0, 21}, {177, 105, 22, 0}, {178, 117, 21, 1}, {118, 179, 1, 21}, // 172-175
    {110, 180, 0, 22}, {181, 115, 23, 0}, {182, 117, 22, 1}, {118, 183, 1, 22}, // 176-179
    {120, 184, 0, 23}, {185, 115, 24, 0}, {186, 127, 23, 1}, {128, 187, 1, 23}, // 180-183
    {120, 188, 0, 24}, {189, 115, 25, 0}, {190, 127, 24, 1}, {128, 191, 1, 24}, // 184-187
    {120, 192, 0, 25}, {193, 115, 26, 0}, {194, 127, 25, 1}, {128, 195, 1, 25}, // 188-191
    {120, 196, 0, 26}, {197, 115, 27, 0}, {198, 127, 26, 1}, {128, 199, 1, 26}, // 192-195
    {120, 200, 0, 27}, {201, 115, 28, 0}, {202, 127, 27, 1}, {128, 203, 1, 27}, // 196-199
    {120, 204, 0, 28}, {205, 115, 29, 0}, {206, 127, 28, 1}, {128, 207, 1, 28}, // 200-203
    {120, 208, 0, 29}, {209, 125, 30, 0}, {210, 127, 29, 1}, {128, 211, 1, 29}, // 204-207
    {130, 212, 0, 30}, {213, 125, 31, 0}, {214, 137, 30, 1}, {138, 215, 1, 30}, // 208-211
    {130, 216, 0, 31}, {217, 125, 32, 0}, {218, 137, 31, 1}, {138, 219, 1, 31}, // 212-215
    {130, 220, 0, 32}, {221, 125, 33, 0}, {222, 137, 32, 1}, {138, 223, 1, 32}, // 216-219
    {130, 224, 0, 33}, {225, 125, 34, 0}, {226, 137, 33, 1}, {138, 227, 1, 33}, // 220-223
    {130, 228, 0, 34}, {229, 125, 35, 0}, {230, 137, 34, 1}, {138, 231, 1, 34}, // 224-227
    {130, 232, 0, 35}, {233, 125, 36, 0}, {234, 137, 35, 1}, {138, 235, 1, 35}, // 228-231
    {130, 236, 0, 36}, {237, 125, 37, 0}, {238, 137, 36, 1}, {138, 239, 1, 36}, // 232-235
    {130, 240, 0, 37}, {241, 125, 38, 0}, {242, 137, 37, 1}, {138, 243, 1, 37}, // 236-239
    {130, 244, 0, 38}, {245, 135, 39, 0}, {246, 137, 38, 1}, {138, 247, 1, 38}, // 240-243
    {140, 248, 0, 39}, {249, 135, 40, 0}, {250, 69, 39, 1},  {80, 251, 1, 39},  // 244-247
    {140, 252, 0, 40}, {249, 135, 41, 0}, {250, 69, 40, 1},  {80, 251, 1, 40},  // 248-251
    {140, 252, 0, 41}};                                                         // 252, 253-255 are reserved

#  define nex( state, sel ) State_table[state][sel]

// The code used to generate the above table at run time (4% slower).
// To print the table, uncomment the 4 lines of print statements below.
// In this code x,y = n0,n1 is the number of 0,1 bits represented by a state.
#else

class StateTable {
  Array<uint8> ns;                          // state*4 -> next state if 0, if 1, n0, n1
  enum { B = 5, N = 64 };                   // sizes of b, t
  static const int b[B];                    // x -> max y, y -> max x
  static uint8 t[N][N][2];                  // x,y -> state number, number of states
  int num_states( int x, int y );           // compute t[x][y][1]
  void discount( int &x );                  // set new value of x after 1 or y after 0
  void next_state( int &x, int &y, int b ); // new (x,y) after bit b
public:
  int operator()( int state, int sel ) {
    return ns[state * 4 + sel];
  }
  StateTable();
} nex;

const int StateTable::b[B] = {42, 41, 13, 6, 5}; // x -> max y, y -> max x
uint8 StateTable::t[N][N][2];

int StateTable::num_states( int x, int y ) {
  if( x < y )
    return num_states( y, x );
  if( x < 0 || y < 0 || x >= N || y >= N || y >= B || x >= b[y] )
    return 0;

  // States 0-30 are a history of the last 0-4 bits
  if( x + y <= 4 ) { // x+y choose x = (x+y)!/x!y!
    int r = 1;
    for( int i = x + 1; i <= x + y; ++i )
      r *= i;
    for( int i = 2; i <= y; ++i )
      r /= i;
    return r;
  }

  // States 31-255 represent a 0,1 count and possibly the last bit
  // if the state is reachable by either a 0 or 1.
  else
    return 1 + ( y > 0 && x + y < 16 );
}

// New value of count x if the opposite bit is observed
void StateTable::discount( int &x ) {
  if( x > 2 )
    x = ilog( x ) / 6 - 1;
}

// compute next x,y (0 to N) given input b (0 or 1)
void StateTable::next_state( int &x, int &y, int b ) {
  if( x < y )
    next_state( y, x, 1 - b );
  else {
    if( b ) {
      ++y;
      discount( x );
    } else {
      ++x;
      discount( y );
    }
    while( !t[x][y][1] ) {
      if( y < 2 )
        --x;
      else {
        x = ( x * ( y - 1 ) + ( y / 2 ) ) / y;
        --y;
      }
    }
  }
}

// Initialize next state table ns[state*4] -> next if 0, next if 1, x, y
StateTable::StateTable() : ns( 1024 ) {
  // Assign states
  int state = 0;
  for( int i = 0; i < 256; ++i ) {
    for( int y = 0; y <= i; ++y ) {
      int x = i - y;
      int n = num_states( x, y );
      if( n ) {
        t[x][y][0] = state;
        t[x][y][1] = n;
        state += n;
      }
    }
  }

  // Print/generate next state table
  state = 0;
  for( int i = 0; i < N; ++i ) {
    for( int y = 0; y <= i; ++y ) {
      int x = i - y;
      for( int k = 0; k < t[x][y][1]; ++k ) {
        int x0 = x, y0 = y, x1 = x, y1 = y; // next x,y for input 0,1
        int ns0 = 0, ns1 = 0;
        if( state < 15 ) {
          ++x0;
          ++y1;
          ns0 = t[x0][y0][0] + state - t[x][y][0];
          ns1 = t[x1][y1][0] + state - t[x][y][0];
          if( x > 0 )
            ns1 += t[x - 1][y + 1][1];
          ns[state * 4] = ns0;
          ns[state * 4 + 1] = ns1;
          ns[state * 4 + 2] = x;
          ns[state * 4 + 3] = y;
        } else if( t[x][y][1] ) {
          next_state( x0, y0, 0 );
          next_state( x1, y1, 1 );
          ns[state * 4] = ns0 = t[x0][y0][0];
          ns[state * 4 + 1] = ns1 = t[x1][y1][0] + ( t[x1][y1][1] > 1 );
          ns[state * 4 + 2] = x;
          ns[state * 4 + 3] = y;
        }
        // uncomment to print table above
        //        printf("{%3d,%3d,%2d,%2d},", ns[state*4], ns[state*4+1],
        //          ns[state*4+2], ns[state*4+3]);
        //        if (state%4==3) printf(" // %d-%d\n  ", state-3, state);
        assert( state >= 0 && state < 256 );
        assert( t[x][y][1] > 0 );
        assert( t[x][y][0] <= state );
        assert( t[x][y][0] + t[x][y][1] > state );
        assert( t[x][y][1] <= 6 );
        assert( t[x0][y0][1] > 0 );
        assert( t[x1][y1][1] > 0 );
        assert( ns0 - t[x0][y0][0] < t[x0][y0][1] );
        assert( ns0 - t[x0][y0][0] >= 0 );
        assert( ns1 - t[x1][y1][0] < t[x1][y1][1] );
        assert( ns1 - t[x1][y1][0] >= 0 );
        ++state;
      }
    }
  }
  //  printf("%d states\n", state); exit(0);  // uncomment to print table above
}

#endif

///////////////////////////// Squash //////////////////////////////

// return p = 1/(1 + exp(-d)), d scaled by 8 bits, p scaled by 12 bits
static int squash( int d ) {
  static const int t[33] = {1,    2,    3,    6,    10,   16,   27,   45,   73,   120,  194,
                            310,  488,  747,  1101, 1546, 2047, 2549, 2994, 3348, 3607, 3785,
                            3901, 3975, 4022, 4050, 4068, 4079, 4085, 4089, 4092, 4093, 4094};
  if( d > 2047 )
    return 4095;
  if( d < -2047 )
    return 0;
  int w = d & 127;
  d = ( d >> 7 ) + 16;
  return ( t[d] * ( 128 - w ) + t[( d + 1 )] * w + 64 ) >> 7;
}

//////////////////////////// Stretch ///////////////////////////////

// Inverse of squash. d = ln(p/(1-p)), d scaled by 8 bits, p by 12 bits.
// d has range -2047 to 2047 representing -8 to 8.  p has range 0 to 4095.

static class Stretch {
  short t[4096];

public:
  Stretch() {
    int pi = 0;
    for( int x = -2047; x <= 2047; ++x ) { // invert squash()
      int i = squash( x );
      for( int j = pi; j <= i; ++j )
        t[j] = x;
      pi = i + 1;
    }
    t[4095] = 2047;
  }
  int operator()( int p ) const {
    assert( p >= 0 && p < 4096 );
    return t[p];
  }
} stretch;

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

#if !defined( __GNUC__ )

#  if( 2 == _M_IX86_FP ) // 2 if /arch:SSE2 was used.
#    define __SSE2__
#  elif( 1 == _M_IX86_FP ) // 1 if /arch:SSE was used.
#    define __SSE__
#  endif

#endif /* __GNUC__ */

/**
 * Vector product a*b of n signed words, returning signed integer scaled down by 8 bits.
 * n is rounded up to a multiple of 8.
 */
static sint32 dot_product( const sint16 *const t, const sint16 *const w, sint32 n );

/**
 * Train n neural network weights w[n] on inputs t[n] and err.
 * w[i] += ((t[i]*2*err)+(1<<16))>>17 bounded to +- 32K.
 * n is rounded up to a multiple of 8.
 */
static void train( const sint16 *const t, sint16 *const w, sint32 n, const sint32 e );

#if defined( __SSE2__ ) // faster
#  include <emmintrin.h>
#  define OPTIMIZE "SSE2-"

static sint32 dot_product( const sint16 *const t, const sint16 *const w, sint32 n ) {
  assert( n == ( ( n + 7 ) & -8 ) );
  __m128i sum = _mm_setzero_si128();
  while( ( n -= 8 ) >= 0 ) {                                                    // Each loop sums eight products
    __m128i tmp = _mm_madd_epi16( *( __m128i * ) &t[n], *( __m128i * ) &w[n] ); // t[n] * w[n] + t[n+1] * w[n+1]
    tmp = _mm_srai_epi32( tmp, 8 );  //                                        (t[n] * w[n] + t[n+1] * w[n+1]) >> 8
    sum = _mm_add_epi32( sum, tmp ); //                                sum += (t[n] * w[n] + t[n+1] * w[n+1]) >> 8
  }
  sum = _mm_add_epi32( sum, _mm_srli_si128( sum, 8 ) ); // Add eight sums together ...
  sum = _mm_add_epi32( sum, _mm_srli_si128( sum, 4 ) );
  return _mm_cvtsi128_si32( sum ); //                     ...  and scale back to integer
}

static void train( const sint16 *const t, sint16 *const w, sint32 n, const sint32 e ) {
  assert( n == ( ( n + 7 ) & -8 ) );
  if( e != 0 ) {
    const __m128i one = _mm_set1_epi16( 1 );
    const __m128i err = _mm_set1_epi16( sint16( e ) );
    while( ( n -= 8 ) >= 0 ) { // Each iteration adjusts eight weights
      __m128i tmp = _mm_adds_epi16( *( __m128i * ) &t[n], *( __m128i * ) &t[n] ); // t[n] * 2
      tmp = _mm_mulhi_epi16( tmp, err ); //                                     (t[n] * 2 * err) >> 16
      tmp = _mm_adds_epi16( tmp, one );  //                                     ((t[n] * 2 * err) >> 16) + 1
      tmp = _mm_srai_epi16( tmp, 1 );    //                                      (((t[n] * 2 * err) >> 16) + 1) >> 1
      tmp = _mm_adds_epi16( tmp,
                            *( __m128i * ) &w[n] ); //                    ((((t[n] * 2 * err) >> 16) + 1) >> 1) + w[n]
      *( __m128i * ) &w[n] =
          tmp; //                                          save the new eight weights, bounded to +- 32K
    }
  }
}

#elif defined( __SSE__ ) // fast
#  include <xmmintrin.h>
#  define OPTIMIZE "SSE-"

static sint32 dot_product( const sint16 *const t, const sint16 *const w, sint32 n ) {
  assert( n == ( ( n + 7 ) & -8 ) );
  __m64 sum = _mm_setzero_si64();
  while( ( n -= 8 ) >= 0 ) {                                             // Each loop sums eight products
    __m64 tmp = _mm_madd_pi16( *( __m64 * ) &t[n], *( __m64 * ) &w[n] ); //   t[n] * w[n] + t[n+1] * w[n+1]
    tmp = _mm_srai_pi32( tmp, 8 );  //                                    (t[n] * w[n] + t[n+1] * w[n+1]) >> 8
    sum = _mm_add_pi32( sum, tmp ); //                            sum += (t[n] * w[n] + t[n+1] * w[n+1]) >> 8

    tmp = _mm_madd_pi16( *( __m64 * ) &t[n + 4], *( __m64 * ) &w[n + 4] ); // t[n+4] * w[n+4] + t[n+5] * w[n+5]
    tmp = _mm_srai_pi32( tmp, 8 );  //                                    (t[n+4] * w[n+4] + t[n+5] * w[n+5]) >> 8
    sum = _mm_add_pi32( sum, tmp ); //                            sum += (t[n+4] * w[n+4] + t[n+5] * w[n+5]) >> 8
  }
  sum = _mm_add_pi32( sum, _mm_srli_si64( sum, 32 ) ); // Add eight sums together ...
  const sint32 retval = _mm_cvtsi64_si32( sum );       //                     ...  and scale back to integer
  _mm_empty();                                         // Empty the multimedia state
  return retval;
}

static void train( const sint16 *const t, sint16 *const w, sint32 n, const sint32 e ) {
  assert( n == ( ( n + 7 ) & -8 ) );
  if( e ) {
    const __m64 one = _mm_set1_pi16( 1 );
    const __m64 err = _mm_set1_pi16( sint16( e ) );
    while( ( n -= 8 ) >= 0 ) {                                             // Each iteration adjusts eight weights
      __m64 tmp = _mm_adds_pi16( *( __m64 * ) &t[n], *( __m64 * ) &t[n] ); //   t[n] * 2
      tmp = _mm_mulhi_pi16( tmp, err ); //                                 (t[n] * 2 * err) >> 16
      tmp = _mm_adds_pi16( tmp, one );  //                                 ((t[n] * 2 * err) >> 16) + 1
      tmp = _mm_srai_pi16( tmp, 1 );    //                                  (((t[n] * 2 * err) >> 16) + 1) >> 1
      tmp = _mm_adds_pi16( tmp, *( __m64 * ) &w[n] ); //                  ((((t[n] * 2 * err) >> 16) + 1) >> 1) + w[n]
      *( __m64 * ) &w[n] = tmp; //                                       save the new four weights, bounded to +- 32K

      tmp = _mm_adds_pi16( *( __m64 * ) &t[n + 4], *( __m64 * ) &t[n + 4] ); // t[n+4] * 2
      tmp = _mm_mulhi_pi16( tmp, err ); //                                 (t[n+4] * 2 * err) >> 16
      tmp = _mm_adds_pi16( tmp, one );  //                                 ((t[n+4] * 2 * err) >> 16) + 1
      tmp = _mm_srai_pi16( tmp, 1 );    //                                  (((t[n+4] * 2 * err) >> 16) + 1) >> 1
      tmp = _mm_adds_pi16( tmp, *( __m64 * ) &w[n + 4] ); //              ((((t[n+4] * 2 * err) >> 16) + 1) >> 1) + w[n]
      *( __m64 * ) &w[n + 4] = tmp; //                                   save the new four weights, bounded to +- 32K
    }
    _mm_empty(); // Empty the multimedia state
  }
}

#else // slow!
#  define OPTIMIZE ""

static sint32 dot_product( const sint16 *const t, const sint16 *const w, sint32 n ) {
  assert( n == ( ( n + 7 ) & -8 ) );
  sint32 sum = 0;
  while( ( n -= 2 ) >= 0 ) {
    sum += ( t[n] * w[n] + t[n + 1] * w[n + 1] ) >> 8;
  }
  return sum;
}

static void train( const sint16 *const t, sint16 *const w, sint32 n, const sint32 err ) {
  assert( n == ( ( n + 7 ) & -8 ) );
  if( err ) {
    while( ( n -= 1 ) >= 0 ) {
      sint32 wt = w[n] + ( ( ( ( t[n] * err * 2 ) >> 16 ) + 1 ) >> 1 );
      if( wt < -32768 ) {
        w[n] = -32768;
      } else if( wt > 32767 ) {
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
  Array<short, 16> wx; // N*M weights
  Array<int> cxt;      // S contexts
  int ncxt;            // number of contexts (0 to S)
  int base;            // offset of next context
  Array<int> pr;       // last result (scaled 12 bits)
  Mixer *mp;           // points to a Mixer to combine results
public:
  Array<short, 16> tx; // N inputs from add()
  int nx;              // Number of inputs in tx, 0 to N
  Mixer( int n, int m, int s = 1, int w = 0 ) :
      N( ( n + 7 ) & -8 ),
      M( m ),
      S( s ),
      wx( N * M ),
      cxt( S ),
      ncxt( 0 ),
      base( 0 ),
      pr( S ),
      mp( 0 ),
      tx( N ),
      nx( 0 ) {
    assert( n > 0 && N > 0 && ( N & 7 ) == 0 && M > 0 );
    int i;
    for( i = 0; i < S; ++i )
      pr[i] = 2048;
    for( i = 0; i < N * M; ++i )
      wx[i] = w;
    if( S > 1 )
      mp = new Mixer( S, 1, 1, 0x7fff );
  }
  virtual ~Mixer() {
    delete mp;
  }

  // Adjust weights to minimize coding cost of last prediction
  void update() {
    for( int i = 0; i < ncxt; ++i ) {
      int err = ( ( y << 12 ) - pr[i] ) * 7;
      assert( err >= -32768 && err < 32768 );
      train( &tx[0], &wx[cxt[i] * N], nx, err );
    }
    nx = base = ncxt = 0;
  }

  void update2() {
    train( &tx[0], &wx[0], nx, ( ( y << 12 ) - base ) * 3 / 2 );
    nx = 0;
  }

  // Input x (call up to N times)
  void add( int x ) {
    assert( nx < N );
    tx[nx++] = x;
  }

  void mul( int x ) {
    int z = tx[nx];
    z = z * x / 4;
    tx[nx++] = z;
  }

  // Set a context (call S times, sum of ranges <= M)
  void set( int cx, int range ) {
    assert( range >= 0 );
    assert( ncxt < S );
    assert( cx >= 0 );
    assert( base + cx < M );
    cxt[ncxt++] = base + cx;
    base += range;
  }

  // predict next bit
  int p() {
    while( ( nx & 7 ) != 0 )
      tx[nx++] = 0;       // pad
    if( mp != nullptr ) { // combine outputs
      mp->update2();
      for( int i = 0; i < ncxt; ++i ) {
        int dp = dot_product( &tx[0], &wx[cxt[i] * N], nx );
        dp = ( dp * 9 ) >> 9;
        pr[i] = squash( dp );
        mp->add( dp );
      }
      return mp->p();
    } // S=1 context
      int z = dot_product( &tx[0], &wx[0], nx );
      base = squash( ( z * 15 ) >> 13 );
      return squash( z >> 9 );
    
  }
};

//////////////////////////// APM //////////////////////////////

// APM maps a probability and a context into a new probability
// that bit y will next be 1.  After each guess it updates
// its state to improve future guesses.  Methods:
//
// APM a(N) creates with N contexts, uses 66*N bytes memory.
// a.p(pr, cx, rate=8) returned adjusted probability in context cx (0 to
//   N-1).  rate determines the learning rate (smaller = faster, default 8).
//   Probabilities are scaled 16 bits (0-65535).

class APM {
  int index;       // last p, context
  Array<uint16> t; // [N][33]:  p, context -> p
public:
  // maps p, cxt -> p initially
  APM( int n ) : index( 0 ), t( n * 33 ) {
    for( int j = 0; j < 33; ++j )
      t[j] = squash( ( j - 16 ) * 128 ) * 16;
    for( int i = 33; i < n * 33; ++i )
      t[i] = t[i - 33];
  }
  int p( int pr = 2048, int cxt = 0, int rate = 8 ) {
    assert( pr >= 0 && pr < 4096 && cxt >= 0 && cxt < N && rate > 0 && rate < 32 );
    pr = stretch( pr );
    int g = ( y << 16 ) + ( y << rate ) - y * 2;
    t[index] += ( g - t[index] ) >> rate;
    t[index + 1] += ( g - t[index + 1] ) >> rate;
    const int w = pr & 127; // interpolation weight (33 points)
    index = ( ( pr + 2048 ) >> 7 ) + cxt * 33;
    return ( t[index] * ( 128 - w ) + t[index + 1] * w ) >> 11;
  }
};

//////////////////////////// StateMap //////////////////////////

// A StateMap maps a nonstationary counter state to a probability.
// After each mapping, the mapping is adjusted to improve future
// predictions.  Methods:
//
// sm.p(cx) converts state cx (0-255) to a probability (0-4095).

// Counter state -> probability * 256
class StateMap {
protected:
  int cxt{ 0 };       // context
  uint16 t[256]; // 256 states -> probability * 64K
public:
  StateMap()  {
    for( int i = 0; i < 256; ++i ) {
      int n0 = nex( i, 2 );
      int n1 = nex( i, 3 );
      if( n0 == 0 )
        n1 *= 128;
      if( n1 == 0 )
        n0 *= 128;
      t[i] = 65536 * ( n1 + 1 ) / ( n0 + n1 + 2 );
    }
  }
  int p( int cx ) {
    assert( cx >= 0 && cx < t.size() );
    int q = t[cxt];
    t[cxt] = q + ( ( sm_add_y - q ) >> sm_shft );
    return t[cxt = cx] >> 4;
  }
};

//////////////////////////// hash //////////////////////////////

static uint32 hash( uint32 a, uint32 b, uint32 c = 0xffffffff ) {
  uint32 h = a * 110002499U + b * 30005491U + c * 50004239U;
  return h ^ h >> 9 ^ a >> 3 ^ b >> 3 ^ c >> 4;
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
template <int B>
class BH {
  enum { M = 7 };     // search limit
  Array<uint8, 64> t; // elements
  uint32 n;           // size-1
public:
  BH( int i ) : t( i * B ), n( i - 1 ) {
    assert( B >= 2 && i > 0 && ( i & ( i - 1 ) ) == 0 ); // size a power of 2?
  }
  uint8 *operator[]( uint32 i );
};

template <int B>
uint8 *BH<B>::operator[]( uint32 i ) {
  int chk = ( i >> 16 ^ i ) & 0xffff;
  i = i * M & n;
  uint8 *p = &t[i * B - B];
  int j;
  for( j = 0; j < M; ++j ) {
    p += B;
    if( p[2] == 0 ) {
      *( uint16 * ) p = chk;
      break;
    }
    if( *( uint16 * ) p == chk )
      break; // found
  }
  if( j == 0 )
    return p; // front
#if 0
  static uint8 tmp[B];  // element to move to front
  if (j==M) {
    --j;
    memset(tmp, 0, B);
    *(uint16*)tmp=chk;
    if ( /*M>2&&*/ p[2]>p[2-B]) --j;
  }
  else memcpy(tmp, p, B);
  p = &t[i*B];
  memmove(p+B, p, j*B);
  memcpy(p, tmp, B);
#else
  if( j == M ) {
    --j;
    if( /*M>2&&*/ p[2] > p[-2] )
      --j;
  } else
    chk = *( int * ) p;
  p = &t[i * 4];
  memmove( p + 4, p, j * 4 );
  *( int * ) p = chk;
#endif
  return p;
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
static int mix2( Mixer &m, int s, StateMap &sm ) {
  int p1 = sm.p( s );
  int n0 = -static_cast<int>( !nex( s, 2 ) );
  int n1 = -static_cast<int>( !nex( s, 3 ) );
  int st = stretch( p1 );
  if( cxtfl != 0 ) {
    m.add( st / 4 );
    int p0 = 4095 - p1;
    m.add( ( p1 - p0 ) * 3 / 64 );
    m.add( st * ( n1 - n0 ) * 3 / 16 );
    m.add( ( ( p1 & n0 ) - ( p0 & n1 ) ) / 16 );
    m.add( ( ( p0 & n0 ) - ( p1 & n1 ) ) * 7 / 64 );
    return static_cast<int>( s > 0 );
  }
  m.add( st * 9 / 32 );
  m.add( st * ( n1 - n0 ) * 3 / 16 );
  int p0 = 4095 - p1;
  m.add( ( ( p1 & n0 ) - ( p0 & n1 ) ) / 16 );
  m.add( ( ( p0 & n0 ) - ( p1 & n1 ) ) * 7 / 64 );
  return static_cast<int>( s > 0 );
}

// A RunContextMap maps a context into the next byte and a repeat
// count up to M.  Size should be a power of 2.  Memory usage is 3M/4.
class RunContextMap {
  BH<4> t;
  uint8 *cp;
  int mulc;

public:
  RunContextMap( int m, int c ) : t( m / 4 ), mulc( c ) {
    cp = t[0] + 2;
  }
  void set( uint32 cx ) { // update count
    if( cp[0] == 0 || cp[1] != b1 )
      cp[0] = 1, cp[1] = b1;
    else if( cp[0] < 255 )
      ++cp[0];
    cp = t[cx] + 2;
  }
  int p() { // predict next bit
    if( ( cp[1] + 256 ) >> ( 8 - bpos ) == c0 )
      return ( ( cp[1] >> ( 7 - bpos ) & 1 ) * 2 - 1 ) * ilog( cp[0] + 1 ) * mulc;
    
      return 0;
  }
  int mix( Mixer &m ) { // return run length
    m.add( p() );
    return static_cast<int>( cp[0] != 0 );
  }
};

// Context is looked up directly.  m=size is power of 2 in bytes.
// Context should be < m/512.  High bits are discarded.
class SmallStationaryContextMap {
  Array<uint16> t;
  int cxt, mulc;
  uint16 *cp;

public:
  SmallStationaryContextMap( int m, int c ) : t( m / 2 ), cxt( 0 ), mulc( c ) {
    assert( ( m / 2 & m / 2 - 1 ) == 0 ); // power of 2?
    for( int i = 0; i < t.size(); ++i )
      t[i] = 32768;
    cp = &t[0];
  }
  void set( uint32 cx ) {
    cxt = cx * 256 & t.size() - 256;
  }
  void mix( Mixer &m /*, int rate=7*/ ) {
#if 1
    if( pos < 4000000 )
      *cp += ( ( y << 16 ) - *cp + ( 1 << 8 ) ) >> 9;
    else
      *cp += ( ( y << 16 ) - *cp + ( 1 << 9 ) ) >> 10;
#else
    int q = *cp;
    if( y )
      q += 65790 - q >> 8;
    else
      q += -q >> 8;
    *cp = q;
#endif
    cp = &t[cxt + c0];
    m.add( stretch( *cp >> 4 ) * mulc / 32 );
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
// as follows: <count:7,d:1> <b1> <b2> <b3> where <b1> is the last byte
// seen, possibly repeated, and <b2> and <b3> are the two bytes seen
// before the first <b1>.  <count:7,d:1> is a 7 bit count and a 1 bit
// flag.  If d=0 then <count> = 1..127 is the number of repeats of <b1>
// and no other bytes have been seen, and <b2><b3> are not used.
// If <d> = 1 then the history is <b3>, <b2>, and <count> - 2 repeats
// of <b1>.  In this case, <b3> is valid only if <count> >= 3 and
// <b2> is valid only if <count> >= 2.
//
// As an optimization, the last two hash elements of each byte (representing
// contexts with 2-7 bits) are not updated until a context is seen for
// a second time.  This is indicated by <count,d> = <1,0>.  After update,
// <count,d> is updated to <2,0> or <2,1>.

class ContextMap {
  const int C, Sz; // max number of contexts
  class E {        // hash element, 64 bytes
    uint16 chk[7]; // byte context checksums
    uint8 last;    // last 2 accesses (0-6) in low, high nibble
  public:
    uint8 bh[7][7];                  // byte context, 3-bit context -> bit history state
                                     // bh[][0] = 1st bit, bh[][1,2] = 2nd bit, bh[][3..6] = 3rd bit
                                     // bh[][0] is also a replacement priority, 0 = empty
    uint8 *get( uint16 chk, int i ); // Find element (0-6) matching checksum.
                                     // If not found, insert or replace lowest priority (not last).
  };
  Array<E, 64> t;                  // bit histories for bits 0-1, 2-4, 5-7
                                   // For 0-1, also contains a run count in bh[][4] and value in bh[][5]
                                   // and pending update count in bh[7]
  Array<uint8 *> cp;               // C pointers to current bit history
  Array<uint8 *> cp0;              // First element of 7 element array containing cp[i]
  Array<uint32> cxt;               // C whole byte contexts (hashes)
  Array<uint8 *> runp;             // C [0..3] = count, value, unused, unused
  StateMap *sm;                    // C maps of state -> p
  int cn;                          // Next context to set by set()
  void update( uint32 cx, int c ); // train model that context cx predicts c
  int mix1( Mixer &m, int cc, int c1, int y1 );
  // mix() with global context passed as arguments to improve speed.
public:
  // m = memory in bytes, a power of 2, C = c
  ContextMap( int m, int c ) :
      C( c = 1 ),
      Sz( ( m >> 6 ) - 1 ),
      t( m >> 6 ),
      cp( c ),
      cp0( c ),
      cxt( c ),
      runp( c ),
      cn( 0 ) {
    assert( m >= 64 && ( m & m - 1 ) == 0 ); // power of 2?
    assert( sizeof( E ) == 64 );
    sm = new StateMap[C];
    for( int i = 0; i < C; ++i ) {
      cp0[i] = cp[i] = &t[0].bh[0][0];
      runp[i] = cp[i] + 3;
    }
  }
  void set( uint32 cx ); // set next whole byte context
  int mix( Mixer &m ) {
    return mix1( m, c0, b1, y );
  }
};

// Find or create hash element matching checksum ch
uint8 *ContextMap::E::get( uint16 ch, int j ) {
  ch += j;
  if( chk[last & 15] == ch )
    return &bh[last & 15][0];
  int b = 0xffff;
  int bi = 0;
  for( int i = 0; i < 7; ++i ) {
    if( chk[i] == ch )
      return last = last << 4 | i, &bh[i][0];
    int pri = bh[i][0];
    if( ( last & 15 ) != i && last >> 4 != i && pri < b )
      b = pri, bi = i;
  }
  return last = 0xf0 | bi, chk[bi] = ch, ( uint8 * ) memset( &bh[bi][0], 0, 7 );
}

// Set the i'th context to cx
void ContextMap::set( uint32 cx ) {
  int i = cn++;
  assert( i >= 0 && i < C );
  cx = cx * 123456791 + i; // permute (don't hash) cx to spread the distribution
  cx = cx << 16 | cx >> 16;
  cxt[i] = cx * 987654323 + i;
}

// Update the model with bit y1, and predict next bit to mixer m.
// Context: cc=c0, bp=bpos, c1=buf(1), y1=y.
int ContextMap::mix1( Mixer &m, int cc, int c1, int y1 ) {
  // Update model with y
  int result = 0;
  for( int i = 0; i < cn; ++i ) {
    uint8 *cpi = cp[i];
    if( cpi != nullptr ) {
      assert( cpi >= &t[0].bh[0][0] && cpi <= &t[Sz].bh[6][6] );
      assert( ( long( cpi ) & 63 ) >= 15 );
      int ns = nex( *cpi, y1 );
      if( ns >= 204 && ( ( rnd() << ( ( 452 - ns ) >> 3 ) ) != 0U ) )
        ns -= 4; // probabilistic increment
      *cpi = ns;
    }

    // Update context pointers
    if( bpos > 1 && runp[i][0] == 0 )
      cpi = 0;
    else if( bpos == 1 || bpos == 3 || bpos == 6 )
      cpi = cp0[i] + 1 + ( cc & 1 );
    else if( bpos == 4 || bpos == 7 )
      cpi = cp0[i] + 3 + ( cc & 3 );
    else {
      cp0[i] = cpi = t[cxt[i] + cc & Sz].get( cxt[i] >> 16, i );

      // Update pending bit histories for bits 2-7
      if( bpos == 0 ) {
        if( cpi[3] == 2 ) {
          const int c = cpi[4] + 256;
          uint8 *p = t[cxt[i] + ( c >> 6 ) & Sz].get( cxt[i] >> 16, i );
          p[0] = 1 + ( ( c >> 5 ) & 1 );
          p[p[0]] = 1 + ( ( c >> 4 ) & 1 );
          p[3 + ( ( c >> 4 ) & 3 )] = 1 + ( ( c >> 3 ) & 1 );
          p = t[cxt[i] + ( c >> 3 ) & Sz].get( cxt[i] >> 16, i );
          p[0] = 1 + ( ( c >> 2 ) & 1 );
          p[p[0]] = 1 + ( ( c >> 1 ) & 1 );
          p[3 + ( ( c >> 1 ) & 3 )] = 1 + ( c & 1 );
          cpi[6] = 0;
        }

        uint8 c0 = runp[i][0];
        // Update run count of previous context
        if( c0 == 0 ) // new context
          c0 = 2, runp[i][1] = c1;
        else if( runp[i][1] != c1 ) // different byte in context
          c0 = 1, runp[i][1] = c1;
        else if( c0 < 254 ) // same byte in context
          c0 += 2;
        runp[i][0] = c0;
        runp[i] = cpi + 3;
      }
    }

    // predict from last byte in context
    int rc = runp[i][0]; // count*2, +1 if 2 different bytes seen
    if( ( runp[i][1] + 256 ) >> ( 8 - bpos ) == cc ) {
      int b = ( runp[i][1] >> ( 7 - bpos ) & 1 ) * 2 - 1; // predicted bit + for 1, - for 0
      int c = ilog( rc + 1 );
      if( ( rc & 1 ) != 0 )
        c = ( c * 15 ) / 4;
      else
        c *= 13;
      m.add( b * c );
    } else
      m.add( 0 );

    // predict from bit context
    result += mix2( m, cpi != nullptr ? *cpi : 0, sm[i] );
    cp[i] = cpi;
  }
  if( bpos == 7 )
    cn = 0;
  return result;
}

//////////////////////////// Models //////////////////////////////

// All of the models below take a Mixer as a parameter and write
// predictions to it.

//////////////////////////// matchModel ///////////////////////////

// matchModel() finds the longest matching context and returns its length

static int matchModel( Mixer &m ) {
  const int MAXLEN = 2047;    // longest allowed match + 1
  static Array<int> t( MEM ); // hash table of pointers to contexts
  static int h = 0;           // hash of last 7 bytes
  static int ptr = 0;         // points to next byte of match if any
  static int len = 0;         // length of match, or 0 if no match
  static int result = 0;

  if( bpos == 0 ) {
    h = h * 887 * 8 + b1 + 1 & t.size() - 1; // update context hash
    if( len != 0 )
      ++len, ++ptr;
    else { // find match
      ptr = t[h];
      if( ( ptr != 0 ) && pos - ptr < buf.size() )
        while( buf( len + 1 ) == buf[ptr - len - 1] && len < MAXLEN )
          ++len;
    }
    t[h] = pos; // update hash table
    result = len;
    //    if (result>0 && !(result&0xfff)) printf("pos=%d len=%d ptr=%d\n", pos, len, ptr);
  }

  // predict
  if( len > MAXLEN )
    len = MAXLEN;
  int sgn;
  if( ( len != 0 ) && b1 == buf[ptr - 1] && c0 == ( buf[ptr] + 256 ) >> ( 8 - bpos ) ) {
    if( ( buf[ptr] >> ( 7 - bpos ) & 1 ) != 0 )
      sgn = 8;
    else
      sgn = -8;
  } else
    sgn = len = 0;
  m.add( sgn * ilog( len ) );
  m.add( sgn * 8 * MIN( len, 32 ) );
  return result;
}

static uint32 col, frstchar = 0, spafdo = 0, spaces = 0, spacecount = 0, words = 0, wordcount = 0, fails = 0, failz = 0,
                   failcount = 0;

//////////////////////////// wordModel /////////////////////////

// Model English text (words and columns/end of line)

static void wordModel( Mixer &m ) {
  static uint32 word0 = 0;
  static uint32 word1 = 0;
  static uint32 word2 = 0;
  static uint32 word3 = 0;
  static uint32 word4 = 0; // hashes
  static ContextMap cm( MEM * 64, 46 );
  static int nl1 = -3;
  static int nl = -2; // previous, current newline position
  static uint32 t1[256];
  static uint16 t2[0x10000];

  // Update word hashes
  if( bpos == 0 ) {
    uint32 c = b1;
    uint32 f = 0;

    if( ( spaces & 0x80000000 ) != 0U )
      --spacecount;
    if( ( words & 0x80000000 ) != 0U )
      --wordcount;
    spaces = spaces * 2;
    words = words * 2;

    if( ( c - 'a' ) <= ( 'z' - 'a' ) || c == 8 || c == 6 || ( c > 127 && b2 != 12 ) ) {
      ++words, ++wordcount;
      word0 = word0 * 263 * 8 + c;
    } else {
      if( c == 32 || c == 10 ) {
        ++spaces, ++spacecount;
        if( c == 10 )
          nl1 = nl, nl = pos - 1;
      }
      if( word0 != 0U ) {
        word4 = word3 * 43;
        word3 = word2 * 47;
        word2 = word1 * 53;
        word1 = word0 * 83;
        word0 = 0;
        if( c == '.' || c == 'O' || c == ( '}' - '{' + 'P' ) )
          f = 1, spafdo = 0;
        else {
          ++spafdo;
          spafdo = MIN( 63, spafdo );
        }
      }
    }

    uint32 h = word0 * 271 + c;
    cm.set( word0 );
    cm.set( h + word1 );
    cm.set( word0 * 91 + word1 * 89 );
    cm.set( h + word1 * 79 + word2 * 71 );

    cm.set( h + word2 );
    cm.set( h + word3 );
    cm.set( h + word4 );
    cm.set( h + word1 * 73 + word3 * 61 );
    cm.set( h + word2 * 67 + word3 * 59 );

    if( f != 0U ) {
      word4 = word3 * 31;
      word3 = word2 * 37;
      word2 = word1 * 41;
      word1 = '.';
    }

    cm.set( b3 | b4 << 8 );
    cm.set( spafdo * 8 * static_cast<unsigned int>( ( w4 & 3 ) == 1 ) );

    col = MIN( 31, pos - nl );
    if( col <= 2 ) {
      if( col == 2 )
        frstchar = MIN( c, 96 );
      else
        frstchar = 0;
    }
    if( frstchar == '[' && c == 32 ) {
      if( b3 == ']' || b4 == ']' )
        frstchar = 96;
    }
    cm.set( frstchar << 11 | c );

    int above = buf[nl1 + col]; // text column context

    // Text column models
    cm.set( col << 16 | c << 8 | above );
    cm.set( col << 8 | c );
    cm.set( col * static_cast<unsigned int>( c == 32 ) );

    h = wordcount * 64 + spacecount;
    cm.set( spaces & 0x7fff );
    cm.set( frstchar << 7 );
    cm.set( spaces & 0xff );
    cm.set( c * 64 + spacecount / 2 );
    cm.set( ( c << 13 ) + h );
    cm.set( h );

    uint32 d = c4 & 0xffff;
    h = w4 << 6;
    cm.set( c + ( h & 0xffffff00 ) );
    cm.set( c + ( h & 0x00ffff00 ) );
    cm.set( c + ( h & 0x0000ff00 ) );
    h <<= 6;
    cm.set( d + ( h & 0xffff0000 ) );
    cm.set( d + ( h & 0x00ff0000 ) );
    h <<= 6, f = c4 & 0xffffff;
    cm.set( f + ( h & 0xff000000 ) );

    uint16 &r2 = t2[f >> 8];
    r2 = r2 << 8 | c;
    uint32 &r1 = t1[d >> 8];
    r1 = r1 << 8 | c;
    uint32 t = c | t1[c] << 8;
    cm.set( t & 0xffff );
    cm.set( t & 0xffffff );
    cm.set( t );
    cm.set( t & 0xff00 );
    t = d | t2[d] << 16;
    cm.set( t & 0xffffff );
    cm.set( t );

    cm.set( x4 & 0x00ff00ff );
    cm.set( x4 & 0xff0000ff );
    cm.set( x4 & 0x00ffff00 );
    cm.set( c4 & 0xff00ff00 );
    cm.set( c + b5 * 256 + ( 1 << 17 ) );
    cm.set( c + b6 * 256 + ( 2 << 17 ) );
    cm.set( b4 + b8 * 256 + ( 4 << 17 ) );

    cm.set( d );
    cm.set( w4 & 15 );
    cm.set( f4 );
    cm.set( ( w4 & 63 ) * 128 + ( 5 << 17 ) );
    cm.set( d << 9 | frstchar );
    cm.set( ( f4 & 0xffff ) << 11 | frstchar );
  }
  cm.mix( m );
}

//////////////////////////// recordModel ///////////////////////

// Model 2-D data with fixed record length.  Also order 1-2 models
// that include the distance to the last match.

static void recordModel( Mixer &m ) {
  static int cpos1[256];     //, cpos2[256], cpos3[256], cpos4[256]; //buf(1)->last 3 pos
  static int wpos1[0x10000]; // buf(1..2) -> last position
  static ContextMap cm( 32768 / 4, 2 );
  static ContextMap cn( 32768 / 2, 5 );
  static ContextMap co( 32768, 4 );
  static ContextMap cp( 32768 * 2, 3 );
  static ContextMap cq( 32768 * 4, 3 );

  // Find record length
  if( bpos == 0 ) {
    int c = b1;
    int w = ( b2 << 8 ) + c;
    int d = w & 0xf0ff;
    int e = c4 & 0xffffff;
    cm.set( c << 8 | ( MIN( 255, pos - cpos1[c] ) / 4 ) );
    cm.set( w << 9 | llog( pos - wpos1[w] ) >> 2 );
    ///    cm.set(rlen|buf(rlen)<<10|buf(rlen*2)<<18);
    cn.set( w );
    cn.set( d << 8 );
    cn.set( c << 16 );
    cn.set( ( f4 & 0xffff ) << 3 );
    int col = pos & 3;
    cn.set( col | 2 << 12 );

    co.set( c );
    co.set( w << 8 );
    co.set( w5 & 0x3ffff );
    co.set( e << 3 );

    cp.set( d );
    cp.set( c << 8 );
    cp.set( w << 16 );

    cq.set( w << 3 );
    cq.set( c << 19 );
    cq.set( e );

    // update last context positions
    cpos1[c] = pos;
    wpos1[w] = pos;
  }
  co.mix( m );
  cp.mix( m );
  cxtfl = 0;
  cm.mix( m );
  cn.mix( m );
  cq.mix( m );
  cxtfl = 3;
}

//////////////////////////// sparseModel ///////////////////////

// Model order 1-2 contexts with gaps.

static void sparseModel( Mixer &m ) {
  static ContextMap cn( MEM * 2, 5 );
  static SmallStationaryContextMap scm1( 0x20000, 17 );
  static SmallStationaryContextMap scm2( 0x20000, 12 );
  static SmallStationaryContextMap scm3( 0x20000, 12 );
  static SmallStationaryContextMap scm4( 0x20000, 13 );
  static SmallStationaryContextMap scm5( 0x10000, 12 );
  static SmallStationaryContextMap scm6( 0x20000, 12 );
  static SmallStationaryContextMap scm7( 0x2000, 12 );
  static SmallStationaryContextMap scm8( 0x8000, 13 );
  static SmallStationaryContextMap scm9( 0x1000, 12 );
  static SmallStationaryContextMap scma( 0x10000, 16 );

  if( bpos == 0 ) {
    cn.set( words & 0x1ffff );
    cn.set( ( f4 & 0x000fffff ) * 7 );
    cn.set( ( x4 & 0xf8f8f8f8 ) + 3 );
    cn.set( ( tt & 0x00000fff ) * 9 );
    cn.set( ( x4 & 0x80f0f0ff ) + 6 );
    scm1.set( b1 );
    scm2.set( b2 );
    scm3.set( b3 );
    scm4.set( b4 );
    scm5.set( words & 127 );
    scm6.set( ( words & 12 ) * 16 + ( w4 & 12 ) * 4 + ( b1 >> 4 ) );
    scm7.set( w4 & 15 );
    scm8.set( spafdo * static_cast<unsigned int>( ( w4 & 3 ) == 1 ) );
    scm9.set( col * static_cast<unsigned int>( b1 == 32 ) );
    scma.set( frstchar );
  }
  cn.mix( m );
  scm1.mix( m );
  scm2.mix( m );
  scm3.mix( m );
  scm4.mix( m );
  scm5.mix( m );
  scm6.mix( m );
  scm7.mix( m );
  scm8.mix( m );
  scm9.mix( m );
  scma.mix( m );
}

static int primes[] = {0, 257, 251, 241, 239, 233, 229, 227, 223, 211, 199, 197, 193, 191};
static uint32 WRT_mpw[16] = {3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0}, tri[4] = {0, 4, 3, 7},
              trj[4] = {0, 6, 6, 12};
static uint32 WRT_mtt[16] = {0, 0, 1, 2, 3, 4, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7};

//////////////////////////// contextModel //////////////////////

// file types (order is important: the last will be sorted by filetype detection as the first)

// This combines all the context models with a Mixer.

static int contextModel2() {
  static ContextMap cm( MEM * 32, 7 );
  static RunContextMap rcm7( MEM / 4, 14 );
  static RunContextMap rcm9( MEM / 4, 18 );
  static RunContextMap rcm10( MEM / 2, 20 );
  static Mixer m( 456, 128 * ( 16 + 14 + 14 + 12 + 14 + 16 ), 6, 512 );
  static uint32 cxt[16]; // order 0-11 contexts
  static Filetype filetype = DEFAULT;
  static int size = 0; // bytes remaining in block

  // Parse filetype and size
  if( bpos == 0 ) {
    --size;
    if( size == -1 )
      filetype = ( Filetype ) b1;
    if( size == -5 ) {
      size = c4;
    }
  }

  m.update();
  m.add( 64 );

  // Test for special file types
  int ismatch = matchModel( m ); // Length of longest matching context

  // Normal model
  if( bpos == 0 ) {
    int i = 0;
    int f2 = buf( 2 );

    if( f2 == '.' || f2 == 'O' || f2 == 'M' || f2 == '!' || f2 == ')' || f2 == ( '}' - '{' + 'P' ) ) {
      if( b1 != f2 && buf( 3 ) != f2 )
        i = 13, x4 = x4 * 256 + f2;
    }

    for( ; i > 0; --i ) // update order 0-11 context hashes
      cxt[i] = cxt[i - 1] * primes[i];

    for( i = 13; i > 0; --i ) // update order 0-11 context hashes
      cxt[i] = cxt[i - 1] * primes[i] + b1;

    cm.set( cxt[3] );
    cm.set( cxt[4] );
    cm.set( cxt[5] );
    cm.set( cxt[6] );
    cm.set( cxt[8] );
    cm.set( cxt[13] );
    cm.set( 0 );

    rcm7.set( cxt[7] );
    rcm9.set( cxt[9] );
    rcm10.set( cxt[11] );

    x4 = x4 * 256 + b1;
  }
  rcm7.mix( m );
  rcm9.mix( m );
  rcm10.mix( m );
  int qq = m.nx;
  order = cm.mix( m ) - 1;
  if( order < 0 )
    order = 0;
  int zz = ( m.nx - qq ) / 7;

  m.nx = qq + zz * 3;
  for( qq = zz * 2; qq != 0; --qq )
    m.mul( 5 );
  for( qq = zz; qq != 0; --qq )
    m.mul( 6 );
  for( qq = zz; qq != 0; --qq )
    m.mul( 9 );

  if( level >= 4 ) {
    wordModel( m );
    sparseModel( m );
    recordModel( m );
  }

  uint32 c1 = b1;
  uint32 c2 = b2;
  uint32 c;
  if( c1 == 9 || c1 == 10 || c1 == 32 )
    c1 = 16;
  if( c2 == 9 || c2 == 10 || c2 == 32 )
    c2 = 16;

  m.set( 256 * order + ( w4 & 240 ) + ( c2 >> 4 ), 256 * 7 );

  c = ( words >> 1 ) & 63;
  m.set( ( w4 & 3 ) * 64 + c + order * 256, 256 * 7 );

  c = ( w4 & 255 ) + 256 * bpos;
  m.set( c, 256 * 8 );

  if( bpos != 0 ) {
    c = c0 << ( 8 - bpos );
    if( bpos == 1 )
      c += b3 / 2;
    c = ( MIN( bpos, 5 ) ) * 256 + ( tt & 63 ) + ( c & 192 );
  } else
    c = ( words & 12 ) * 16 + ( tt & 63 );
  m.set( c, 1536 );

  c = bpos;
  c2 = ( c0 << ( 8 - bpos ) ) | ( c1 >> bpos );
  m.set( order * 256 + c + ( c2 & 248 ), 256 * 7 );

  c = c * 256 + ( ( c0 << ( 8 - bpos ) ) & 255 );
  c1 = ( words << bpos ) & 255;
  m.set( c + ( c1 >> bpos ), 2048 );

  return m.p();
}

//////////////////////////// Predictor /////////////////////////

// A Predictor estimates the probability that the next bit of
// uncompressed data is 1.  Methods:
// p() returns P(1) as a 12 bit number (0-4095).
// update(y) trains the predictor with the actual bit (0 or 1).

class Predictor {
  int pr{ 2048 }; // next prediction
public:
  Predictor();
  int p() const {
    assert( pr >= 0 && pr < 4096 );
    return pr;
  }
  void update();
};

Predictor::Predictor()  {}

void Predictor::update() {
  static APM a1( 256 );
  static APM a2( 0x8000 );
  static APM a3( 0x8000 );
  static APM a4( 0x20000 );
  static APM a5( 0x10000 );
  static APM a6( 0x10000 );

  // Update global context: pos, bpos, c0, c4, buf
  c0 += c0 + y;
  if( c0 >= 256 ) {
    buf[pos++] = c0;
    c0 -= 256;
    if( pos <= 1024 * 1024 ) {
      if( pos == 1024 * 1024 )
        sm_shft = 9, sm_add = 65535 + 511;
      if( pos == 512 * 1024 )
        sm_shft = 8, sm_add = 65535 + 255;
      sm_add_y = sm_add & ( -y );
    }
    int i = WRT_mpw[c0 >> 4];
    w4 = w4 * 4 + i;
    if( b1 == 12 )
      i = 2;
    w5 = w5 * 4 + i;
    b8 = b7, b7 = b6, b6 = b5, b5 = b4, b4 = b3;
    b3 = b2;
    b2 = b1;
    b1 = c0;
    if( c0 == '.' || c0 == 'O' || c0 == 'M' || c0 == '!' || c0 == ')' || c0 == ( '}' - '{' + 'P' ) ) {
      w5 = ( w5 << 8 ) | 0x3ff, x5 = ( x5 << 8 ) + c0, f4 = ( f4 & 0xfffffff0 ) + 2;
      if( c0 != '!' && c0 != 'O' )
        w4 |= 12;
      if( c0 != '!' )
        b2 = '.', tt = ( tt & 0xfffffff8 ) + 1;
    }
    c4 = ( c4 << 8 ) + c0;
    x5 = ( x5 << 8 ) + c0;
    if( c0 == 32 )
      --c0;
    f4 = f4 * 16 + ( c0 >> 4 );
    tt = tt * 8 + WRT_mtt[c0 >> 4];
    c0 = 1;
  }
  bpos = ( bpos + 1 ) & 7;

  if( ( fails & 0x00000080 ) != 0U )
    --failcount;
  fails = fails * 2;
  failz = failz * 2;
  if( y != 0 )
    pr ^= 4095;
  if( pr >= 1820 )
    ++fails, ++failcount;
  if( pr >= 848 )
    ++failz;

  // Filter the context model with APMs
  pr = contextModel2();

  int rate = 6 + static_cast<int>( pos > 14 * 256 * 1024 ) + static_cast<int>( pos > 28 * 512 * 1024 );
  int pt;
  int pu = ( a1.p( pr, c0, 3 ) + 7 * pr + 4 ) >> 3;
  int pv;
  int pz = failcount + 1;
  pz += tri[( fails >> 5 ) & 3];
  pz += trj[( fails >> 3 ) & 3];
  pz += trj[( fails >> 1 ) & 3];
  if( ( fails & 1 ) != 0U )
    pz += 8;
  pz = pz / 2;

  pu = a4.p( pu, ( c0 * 2 ) ^ ( hash( b1, ( x5 >> 8 ) & 255, ( x5 >> 16 ) & 0x80ff ) & 0x1ffff ), rate );
  pv = a2.p( pr, ( c0 * 8 ) ^ ( hash( 29, failz & 2047 ) & 0x7fff ), rate + 1 );
  pv = a5.p( pv, hash( c0, w5 & 0xfffff ) & 0xffff, rate );
  pt = a3.p( pr, ( c0 * 32 ) ^ ( hash( 19, x5 & 0x80ffff ) & 0x7fff ), rate );
  pz = a6.p( pu, ( c0 * 4 ) ^ ( hash( MIN( 9, pz ), x5 & 0x80ff ) & 0xffff ), rate );

  if( ( fails & 255 ) != 0U )
    pr = ( pt * 6 + pu + pv * 11 + pz * 14 + 16 ) >> 5;
  else
    pr = ( pt * 4 + pu * 5 + pv * 12 + pz * 11 + 16 ) >> 5;
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

typedef enum { COMPRESS, DECOMPRESS } Mode;
class Encoder {
private:
  Predictor predictor;
  const Mode mode; // Compress or decompress?
  FILE *archive;   // Compressed data file
  uint32 x1, x2;   // Range, initially [0, 1), scaled by 2^32
  uint32 x;        // Decompress mode: last 4 input bytes of archive
  FILE *alt;       // decompress() source in COMPRESS mode

  // Compress bit y or return decompressed bit
  int code( int i = 0 ) {
    int p = predictor.p();
    assert( p >= 0 && p < 4096 );
    p += static_cast<int>( p < 2048 );
    uint32 xmid = x1 + ( ( x2 - x1 ) >> 12 ) * p + ( ( x2 - x1 & 0xfff ) * p >> 12 );
    assert( xmid >= x1 && xmid < x2 );
    if( mode == DECOMPRESS )
      y = static_cast<int>( x <= xmid );
    else
      y = i;
    y != 0 ? ( x2 = xmid, p = sm_add ) : ( x1 = xmid + 1, p = 0 );
    sm_add_y = p;
    predictor.update();
    while( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 ) { // pass equal leading bytes of range
      if( mode == COMPRESS )
        putc( x2 >> 24, archive );
      x1 <<= 8;
      x2 = ( x2 << 8 ) + 255;
      if( mode == DECOMPRESS )
        x = ( x << 8 ) + ( getc( archive ) & 255 ); // EOF is OK
    }
    return y;
  }

public:
  Encoder( Mode m, FILE *f );

  // Compress one byte
  void compress( int c ) {
    assert( mode == COMPRESS );
    if( level == 0 ) {
      putc( c, archive );
      return;
    }
    if( c >= '{' && c < 127 )
      c += 'P' - '{';
    else if( c >= 'P' && c < 'T' )
      c -= 'P' - '{';
    else if( ( c >= ':' && c <= '?' ) || ( c >= 'J' && c <= 'O' ) )
      c ^= 0x70;
    if( c == 'X' || c == '`' )
      c ^= 'X' ^ '`';
    for( int i = 7; i >= 0; --i )
      code( ( c >> i ) & 1 );
  }

  // Decompress and return one byte
  int decompress() {
    if( mode == COMPRESS ) {
      assert( alt );
      return getc( alt );
    } if( level == 0 )
      return getc( archive );
    else {
      int c = 0;
      for( int i = 8; i != 0; --i )
        c += c + code();
      if( c >= '{' && c < 127 )
        c += 'P' - '{';
      else if( c >= 'P' && c < 'T' )
        c -= 'P' - '{';
      else if( ( c >= ':' && c <= '?' ) || ( c >= 'J' && c <= 'O' ) )
        c ^= 0x70;
      if( c == 'X' || c == '`' )
        c ^= 'X' ^ '`';
      return c;
    }
  }

  void flush() {
    if( mode == COMPRESS && level > 0 )
      putc( x1 >> 24, archive ); // Flush first unequal byte of range
  }
};

Encoder::Encoder( Mode m, FILE *f ) : mode( m ), archive( f ), x1( 0 ), x2( 0xffffffff ), x( 0 ), alt( 0 ) {
  if( level > 0 && mode == DECOMPRESS ) { // x = first 4 bytes of archive
    for( int i = 4; i != 0; --i )
      x = ( x << 8 ) + ( getc( archive ) & 255 );
  }
}

///////////////////////////// Filter /////////////////////////////////

// A Filter is an abstract class which should be implemented to perform
// various transforms to improve compression for various file types.
// A function makeFilter(filename, filetype, encoder) will create an
// appropriate derived object either by examining filename and its
// contents (for compression) and set filetype, or as specified by
// filetype (for decompression).
//
// An implementation of Filter must provide the following 4 functions:
//
// 1. protected: void encode(FILE* f, int n) const;  f will be open for
// reading in binary mode and positioned at the beginning of the file,
// which has length n.  The function should read all n bytes of f and
// write transformed data to a temporary file, FILE* tmp, a protected
// member of Filter, which will be open in "wb+" mode (as returned
// by tmpfile()).  f and tmp should be left open.
// encode() should not modify any other data members than tmp that might
// change the behaior of decode().  It is const to prevent some errors but
// there is nothing to prevent it from modifying global variables or
// objects through pointers that might be used by decode().
//
// 2. protected: int decode();  should perform the inverse translation of
// encode() one byte at a time.  decode() will be called once for each byte
// of f.  The n'th call to decode() should return the n'th byte of f (0-255).
// decode() should get its input by calling protected member
// int read(), which returns one byte of transformed data (as stored
// in tmp).  decode() should not read tmp directly.  (tmp may not be open).
//
// 3. A public constructor taking an Encoder reference and passing it to the
// Filter constructor, e.g.
//
//   class Myfilter: public Filter {
//     public: MyFilter(Encoder& en): Filter(en){} // initialize for decode()
//
// 4. A public destructor to free any resources (memory) allocated by the
// constructor using 'new'.  A destructor is not necessary if all
// memory is allocated by creating Arrays.  Remember that a new
// Filter is created for each file in the archive.
//
// In addition an implementation should modify:
//
// 5. public: static Filter* make(const char* filename, Encoder& en);
//
// to return a pointer to a new Filter of the derived type when an
// appropriate file type is detected.  A file type might be detected by
// the filename extension or by examining the file contents.  If the
// file is opened, then it should be closed before returning.
//
//
// Filter implements the following:
//
// protected: int read() tests whether tmp is NULL.  If so, it
// decompresses a byte from the Encoder en and returns it.  Otherwise
// it reads a byte from tmp.
//
// The following are public:
//
// void decompress(FILE* f, int n);  decompresses to f (open in "wb"
// mode) by calling decode() n times and writing the bytes to f.
//
// void compare(FILE* f, int n);  decompresses n bytes by calling
// decode() n times and compares with the first n bytes of f (open
// in "rb" mode).  Prints either "identical" or a message indicating
// where the files first differ.
//
// void skip(int n);  calls decode() n times, discarding the results.
//
// void compress(FILE* f, int n);  compresses f, which is
// open in "rb" mode and has length n bytes.  It first opens tmp using
// tmpfile(), then calls encode(f) to write the transformed data to tmp.
// Then it tests the transform by rewinding tmp and f, then calling decode()
// n times and comparing with n bytes of f.  If the comparison is identical
// then filetype (global, one byte) is compressed (with filetype 0
// compression) to Encoder en, tmp is rewound again and compressed to en
// with appropriate filetype (which affects the model).  Otherwise a
// warning is written, filetype is set to 0, a 0 is compressed to en,
// f is rewound and n bytes of f are compressed to en.  Then in either case,
// tmp is closed (which deletes the file).  In addition,
// decode() must read exactly every byte of tmp and not the EOF, or else
// the transform is abandoned with a warning.
//
// A derived class defaultFilter does a 1-1 transform, equivalent
// to filetype 0.

class Filter {
private:
  Encoder *en;

protected:
  FILE *tmp;                                 // temporary file
  virtual void encode( FILE *f, int n ) = 0; // user suplied
  virtual int decode() = 0;                  // user supplied
  Filter( const Filter * );                  // copying not allowed
  Filter &operator=( const Filter & );       // assignment not allowed
  static void printStatus( int n ) {         // print progress
    if( n > 0 && ( ( n & 0x3fff ) == 0 ) )
      printf( "%10d \b\b\b\b\b\b\b\b\b\b\b", n ), fflush( stdout );
  }

public:
  Filter( Encoder *e ) : en( e ), tmp( 0 ), reads( 0 ) {}
  virtual ~Filter() {}
  void decompress( FILE *f, int n );
  void compare( FILE *f, int n );
  void skip( int n );
  void compress( FILE *f, int n );
  static Filter *make( const char *filename, Encoder *e );
  int read() {
    return ++reads, tmp != nullptr ? getc( tmp ) : en->decompress();
  }
  int reads; // number of calls to read()
};

void Filter::decompress( FILE *f, int n ) {
  for( int i = 0; i < n; ++i ) {
    putc( decode(), f );
    printStatus( i );
  }
  printf( "extracted  \n" );
}

void Filter::compare( FILE *f, int n ) {
  bool found = false; // mismatch?
  for( int i = 0; i < n; ++i ) {
    printStatus( i );
    int c1 = found ? EOF : getc( f );
    int c2 = decode();
    if( c1 != c2 && !found ) {
      printf( "differ at %d: file=%d archive=%d\n", i, c1, c2 );
      found = true;
    }
  }
  if( !found && getc( f ) != EOF )
    printf( "file is longer\n" );
  else if( !found )
    printf( "identical  \n" );
}

void Filter::skip( int n ) {
  for( int i = 0; i < n; ++i ) {
    decode();
    printStatus( i );
  }
  printf( "skipped    \n" );
}

// Compress n bytes of f. If filetype > 0 then transform
void Filter::compress( FILE *f, int n ) {
  // No transform
  if( filetype == 0 ) {
    en->compress( 0 );
    for( int i = 0; i < n; ++i ) {
      printStatus( i );
      en->compress( getc( f ) );
    }
    return;
  }

  // Try transform
  tmp = tmpfile();
  if( tmp == nullptr )
    perror( "tmpfile" ), exit( 1 );
  encode( f, n );

  // Test transform.  decode() should produce copy of input and read
  // exactly all of tmp but not EOF.
  int tmpn = ftell( tmp );
  rewind( tmp );
  rewind( f );

  bool found = false; // mismatch?
#ifndef NO_UNWRT_CHECK
  for( int i = 0; i < n; ++i ) {
    int c1 = getc( f );
    int c2 = decode();
    if( c1 != c2 && !found ) {
      found = true;
      printf( "filter %d failed at %d: %d -> %d, skipping\n", filetype, i, c1, c2 );
      break;
    }
  }

  if( !found && tmpn != reads )
    printf( "filter %d reads %d/%d bytes, skipping\n", filetype, reads, tmpn );
  if( found || tmpn != reads ) { // bug in Filter, code untransformed
    rewind( f );
    filetype = 0;
    en->compress( 0 );
    for( int i = 0; i < n; ++i ) {
      printStatus( i );
      en->compress( getc( f ) );
    }
  } else
#endif
  { // transformed
    rewind( tmp );
    ///if (filetype==EXE) printf("-> %d (x86) ", tmpn);
    ///else
    if( filetype == TEXT )
      printf( "-> %d (text) ", tmpn );
    else if( filetype == BINTEXT )
      printf( "-> %d (binary+text) ", tmpn );

    int t = filetype;
    filetype = 0;
    en->compress( t );
    filetype = t;
    for( int i = 0; i < tmpn; ++i ) {
      printStatus( i );
      en->compress( getc( tmp ) );
    }
  }
  fclose( tmp );
}

///////////////// DefaultFilter ////////////

// DefaultFilter does no translation (for testing)
class DefaultFilter : public Filter {
public:
  DefaultFilter( Encoder *e ) : Filter( e ) {}

protected:
  void encode( FILE *f, int n ) { // not executed if filetype is 0
    while( ( n-- ) != 0 )
      putc( getc( f ), tmp );
  }
  int decode() {
    return read();
  }
};

#define POWERED_BY_PAQ

#pragma warning( disable : 4786 )
#include "vector"

Filter *WRTd_filter = NULL;
#include "textfilter.hpp"
WRT wrt;

class TextFilter : public Filter {
private:
  FILE *dtmp;
  bool first;

public:
  TextFilter( Encoder *e ) : Filter( e ), dtmp( NULL ) {
    reset();
    WRTd_filter = this;
  }
  virtual ~TextFilter() {
    reset();
    tmp = NULL;
  };
  void encode( FILE *f, int n );
  int decode();
  void reset() {
    first = true;
    wrt.WRT_prepare_decoding();
    if( dtmp != nullptr )
      fclose( dtmp );
  };
};

void TextFilter::encode( FILE *f, int n ) {
  //	wrt.defaultSettings(0,NULL);
  wrt.WRT_start_encoding( f, tmp, n, true ); // wrt.WRT_getFileType() called in make()
}

int TextFilter::decode() {
  if( first ) {
    first = false;
    if( tmp == nullptr ) {
      if( dtmp != nullptr )
        fclose( dtmp );

      dtmp = tmpfile();
      if( dtmp == nullptr )
        perror( "WRT tmpfile" ), exit( 1 );

      unsigned int size = 0;
      unsigned int i;
      for( i = 4; i != 0; --i ) {
        int c = read();
        size = size * 256 + c;
        putc( c, dtmp );
      }

      size -= 4;
      for( ; i < size; i++ ) {
        putc( read(), dtmp );
        printStatus( i );
      }

      fseek( dtmp, 0, SEEK_SET );
      tmp = dtmp;
    }
  }

  return wrt.WRT_decode_char( tmp, NULL, 0 );
}

////////////////// Filter::make ////////////

// Create a new Filter of an appropriate type, either by examining
// filename (and maybe its contents) and setting filetype (for compression)
// or if filename is NULL (decompression) then use the supplied value
// of filetype.

Filter *Filter::make( const char *filename, Encoder *e ) {
  if( filename != nullptr ) {
    filetype = 0;
    const char *ext = strrchr( filename, '.' );

    FILE *file = fopen( filename, "rbe" );
    if( file != nullptr ) {
      if( fgetc( file ) == 'M' && fgetc( file ) == 'Z' )
        filetype = EXE;
      else
      ///if (!ext || (!equals(ext, ".dbf") && !equals(ext, ".mdb") && !equals(ext, ".tar")
      ///	&& !equals(ext, ".c") && !equals(ext, ".cpp") && !equals(ext, ".h")
      ///	&& !equals(ext, ".hpp") && !equals(ext, ".ps") && !equals(ext, ".hlp")
      ///	&& !equals(ext, ".ini") && !equals(ext, ".inf") ))
      {
        // fseek(file, 0, SEEK_SET ); <- unnecessary for WRT
        wrt.defaultSettings( 0, NULL );
        int recordLen;                                   // unused in PAQ
        if( wrt.WRT_getFileType( file, recordLen ) > 0 ) // 0 = binary or not known
        {
          if( IF_OPTION( OPTION_NORMAL_TEXT_FILTER ) )
            filetype = TEXT;
          else
            filetype = BINTEXT;
        }
      }
      fclose( file );
    }
  }
  if( e != nullptr ) {
    ///if (filetype==EXE)
    ///return new ExeFilter(e);
    ///else
    if( filetype == TEXT || filetype == BINTEXT )
      return new TextFilter( e );
    
      return new DefaultFilter( e );
  }
  return NULL;
}

//////////////////////////// main program ////////////////////////////

// Read one line, return NULL at EOF or ^Z.  f may be opened ascii or binary.
// Trailing \r\n is dropped.  Lines longer than MAXLINE-1=511 are truncated.

static char *getline( FILE *f = stdin ) {
  const int MAXLINE = 512;
  static char s[MAXLINE];
  int len = 0;
  int c;
  while( ( c = getc( f ) ) != EOF && c != 26 && c != '\n' && len < MAXLINE - 1 ) {
    if( c != '\r' )
      s[len++] = c;
  }
  s[len] = 0;
  if( c == EOF || c == 26 )
    return 0;
  return s;
}

// Test if files exist and get their sizes, store in archive header
static void store_in_header( FILE *f, char *filename, long &total_size ) {
  FILE *fi = fopen( filename, "rbe" );
  if( fi != nullptr ) {
    fseek( fi, 0, SEEK_END ); // get size
    long size = ftell( fi );
    total_size += size;
    if( ( ( size & ~0x7fffffffL ) != 0 ) || ( ( total_size & ~0x7fffffffL ) != 0 ) ) {
      fprintf( stderr, "File sizes must total less than 2 gigabytes\n" );
      fprintf( f, "-1\tError: over 2 GB\r\n" );
      exit( 1 );
    }
    fclose( fi );
    if( size != -1 ) {
      fprintf( f, "%ld\t%s\r\n", size, filename );
      return;
    }
  }
  perror( filename );
}

// Compress/decompress files.  Usage: paq8h archive files...
// If archive does not exist, it is created and the named files are
// compressed.  If there are no file name arguments after the archive,
// then file names are read from input up to a blank line or EOF.
// If archive already exists, then the files in the archive are either
// extracted, or compared if the file already exists.  The files
// are extracted to the names listed on the command line in the
// order they were stored, defaulting to the stored names.

int main( int argc, char **argv ) {
  clock_t start_time = clock(); // start timer
  long total_size = 0;          // uncompressed size of all files
  FILE *f;

  // Print help message
  if( argc < 2 ) {
#ifndef SHORTEN_CODE
    printf( PROGNAME " archiver (C) 2006, Matt Mahoney.\n"
                     "Free under GPL, http://www.gnu.org/licenses/gpl.txt\n\n"
                     "To compress:\n"
                     "  " PROGNAME " -level archive files... (archive will be created)\n"
                     "or (Windows):\n"
                     "  dir/b | " PROGNAME " archive  (file names read from input)\n"
                     "\n"
                     "level: -0 = store, -1 -2 -3 = faster (uses 21, 28, 42 MB)\n"
                     "-4 -5 -6 -7 -8 = smaller (uses 120, 225, 450, 900, 1800 MB)\n"
                     "level -%d is default\n"
                     "\n"
                     "To extract or compare:\n"
                     "  " PROGNAME " archive  (extracts to stored names)\n"
                     "\n"
                     "To view contents: more < archive\n"
                     "\n",
            DEFAULT_OPTION );
#endif
    return 0;
  }

  // Get option
  int option = '4';
  if( argv[1][0] == '-' )
    option = argv[1][1], ++argv, --argc;
#ifndef SHORTEN_CODE
  if( option < 32 )
    option = 32;
  if( option < '0' || option > '9' )
    fprintf( stderr, "Bad option -%c (use -0 to -9)\n", option ), exit( 1 );
#endif

  // Test for archive.  If none, create one and write a header.
  // The first line is PROGNAME.  This is followed by a list of
  // file sizes (as decimal numbers) and names, separated by a tab
  // and ending with \r\n.  The last entry is followed by ^Z
  Mode mode = DECOMPRESS;

  f = fopen( argv[1], "rbe" );
  if( f == nullptr ) {
    mode = COMPRESS;

    f = fopen( argv[1], "wbe" );
    if( f == nullptr )
      perror( argv[1] ), exit( 1 );
    fprintf( f, "%s -%c\r\n", PROGNAME, option );

    // Get filenames
#ifndef SHORTEN_CODE
    if( argc == 2 )
      printf( "Enter names of files to compress, followed by blank line\n" );
#endif
    int i = 2;
    std::multimap<int, std::string> filetypes;
    std::multimap<int, std::string>::iterator it;
    std::vector<std::string> filenames;
    char *filename;

    for( ;; ) {
#ifndef SHORTEN_CODE
      if( argc == 2 ) {
        filename = getline();
        if( ( filename == nullptr ) || ( filename[0] == 0 ) )
          break;
      } else
#endif
      {
        if( i == argc )
          break;
        filename = argv[i++];
      }
      filenames.emplace_back(filename );
    } // for

    for( i = 0; i < filenames.size(); i++ ) {
      Filter::make( filenames[i].c_str(), NULL );
      std::pair<int, std::string> p( ( 1 << ( 31 - 1 ) )
                                         - ( filetype * 8 * 16 * 2048 + preprocFlag + ( wrt.longDict + 1 ) * 16 * 2048
                                             + ( wrt.shortDict + 1 ) * 2048 ),
                                     filenames[i] );
      PRINT_DICT( ( " %s\n", filenames[i].c_str() ) );
      filetypes.insert( p );
    }

    for( it = filetypes.begin(); it != filetypes.end(); it++ ) // multimap is sorted by filetype
      store_in_header( f, ( char * ) it->second.c_str(), total_size );

    fputc( 26, f ); // EOF
    fclose( f );
    f = fopen( argv[1], "r+be" );
    if( f == nullptr )
      perror( argv[1] ), exit( 1 );
  }

  // Read existing archive. Two pointers (header and body) track the
  // current filename and current position in the compressed data.
  long header;
  long body;             // file positions in header, body
  char *filename = getline( f ); // check header
  if( ( filename == nullptr ) || ( strncmp( filename, PROGNAME " -", strlen( PROGNAME ) + 2 ) != 0 ) )
    fprintf( stderr, "%s: not a " PROGNAME " file\n", argv[1] ), exit( 1 );
  option = filename[strlen( filename ) - 1];
  level = option - '0';
  if( level < 0 || level > 9 )
    level = DEFAULT_OPTION;

  {
    buf.setsize( MEM * 8 );
    FILE *dictfile = fopen( "./to_train_models.dic", "rbe" );
    FILE *tmpfi = fopen( "./tmp_tmp_tmp_tmp.dic", "wbe" );
    filetype = 0;
    Encoder en( COMPRESS, tmpfi );
    en.compress( 0 );
    for( int i = 0; i < 465211; ++i )
      en.compress( getc( dictfile ) );
    en.flush();
    fclose( tmpfi );
  }

  header = ftell( f );

  // Initialize encoder at end of header
  if( mode == COMPRESS )
    fseek( f, 0, SEEK_END );
  else { // body starts after ^Z in file
    int c;
    while( ( c = getc( f ) ) != EOF && c != 26 )
      ;
    if( c != 26 )
      fprintf( stderr, "Archive %s is incomplete\n", argv[1] ), exit( 1 );
  }
  Encoder en( mode, f );
  body = ftell( f );

  // Compress/decompress files listed on command line, or header if absent.
  int filenum = 1; // command line index
  total_size = 0;
  for( ;; ) {
    fseek( f, header, SEEK_SET );
    if( ( filename = getline( f ) ) == 0 )
      break;
    size = atol( filename ); // parse size and filename, separated by tab
    total_size += size;
    while( ( *filename != 0 ) && *filename != '\t' )
      ++filename;
    if( *filename == '\t' )
      ++filename;
    printf( "%ld\t%s: ", size, filename );
    fsize = ( int ) size;
    /*   if (mode==DECOMPRESS && ++filenum<argc  // doesn't work with sorting depend on type of file
    && strcmp(argv[filenum], filename)) {
    printf(" -> %s", argv[filenum]);
    filename=argv[filenum];
    } */
    if( size < 0 || total_size < 0 )
      break;
    header = ftell( f );
    fseek( f, body, SEEK_SET );

    // If file exists in COMPRESS mode, compare, else compress/decompress
    FILE *fi = fopen( filename, "rbe" );
    filetype = 0;
    if( mode == COMPRESS ) {
      if( fi == nullptr )
        perror( filename ), exit( 1 );
      Filter *fp = Filter::make( filename, &en ); // sets filetype
      fp->compress( fi, size );
      printf( " -> %4ld   \n", ftell( f ) - body );
      delete fp;
    } else { // DECOMPRESS, first byte determines filter type
      filetype = en.decompress();
      Filter *fp = Filter::make( 0, &en );
      if( fi != nullptr )
        fp->compare( fi, size );
      else { // extract
        fi = fopen( filename, "wbe" );
        if( fi != nullptr )
          fp->decompress( fi, size );
        else {
          perror( filename );
          fp->skip( size );
        }
      }
      delete fp;
    }
    if( fi != nullptr )
      fclose( fi );
    body = ftell( f );
  }
  fseek( f, body, SEEK_SET );
  en.flush();

  // Print stats
  if( f != nullptr ) {
    if( mode == DECOMPRESS && filenum < argc - 1 )
      printf( "No more files to extract\n" );
    long compressed_size = ftell( f );
    if( mode == COMPRESS )
      printf( "%ld -> %ld", total_size, compressed_size );
    else
      printf( "%ld -> %ld", compressed_size, total_size );
    start_time = clock() - start_time;
    if( compressed_size > 0 && total_size > 0 && start_time > 0 ) {
      printf( " (%1.4f bpc) in %1.2f sec (%1.3f KB/sec), %d Kb\n", 8.0 * compressed_size / total_size,
              ( double ) start_time / CLOCKS_PER_SEC, 0.001 * total_size * CLOCKS_PER_SEC / start_time,
              programChecker.maxmem / 1024 );
    }
  }
  return 0;
}
