/*	lpaq9m.cpp file compressor, February 20, 2009.

(C) 2007	Matt Mahoney, matmahoney@yahoo.com
(C) 2007-2009	Alexander Ratushnyak, pqr@rogers.com

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

To compress:   N input output  (N=0..9, uses 6+3*2^N MB memory)
To decompress: d input output  (requires same memory)

For example:

  lpaq9m 9 foo foo.lpq

compresses foo to foo.lpq using 1.5 GB memory.

  lpaq9m d foo.lpq foo

decompresses to foo, also using 1.5 GB memory.
Option 0 uses 9 MB memory.  The maximum file size is 2 GB.


DESCRIPTION OF LPAQ1:

lpaq1 is a "lite" version of PAQ, about 35 times faster than paq8l
at the cost of some compression (but similar to high end BWT and
PPM compressors).  It is a context-mixing compressor combining 7
contexts: orders 1, 2, 3, 4, 6, a lowercase unigram word context
(for ASCII text), and a "match" order, which predicts the next
bit in the last matching context.  The independent bit predictions of
the 7 models are combined using one of 80 neural networks (selected by
a small context), then adjusted using 2 SSE stages (order 0 and 1)
and arithmetic coded.

Prediction is bitwise.  This means that an order-n context consists
of the last n whole bytes plus any of the 0 to 7 previously coded
bits of the current byte starting with the most significant bit.
The unigram word context consists of a hash of the last (at most) 11
consecutive letters (A-Z, a-z) folded to lower case.  The context
does not include any nonalphabetic characters nor any characters
preceding the last nonalphabetic character.

The first 6 contexts (orders 1..4, 6, word) are used to index a
hash table to look up a bit-history represented by an 8-bit state.
The representable states are the same as in paq8l.  A state can
either represent all histories up to 4 bits long, or a pair of
0,1 counts plus a flag to indicate the most recent bit.  The counts
are bounded by (41,0), (40,1), (12,2), (5,3), (4,4) and likewise
for 1,0.  When a count is exceeded, the opposite count is reduced to
approximately preserve the count ratio.  The last bit flag is present
only for states whose total count is less than 16.  There are 253
possible states.

A bit history is mapped to a probability using an adaptive table
(StateMap).  This differs from paq8l in that each table entry includes
a count so that adaptation is rapid at first.  Each table entry
has a 22-bit probability (initially p = 0.5) and 10-bit count (initially
n = 0) packed into 32 bits.  After bit y is predicted, n is incremented
up to the limit (1023) and the probability is adjusted by
p := p + (y - p)/(n + 0.5).  This model is stationary:
p = (n1 + 0.5)/(n + 1), where n1 is the number of times y = 1 out of n.

The "match" model (MatchModel) looks up the current context in a
hash table, first using a longer context, then a shorter one.  If
a match is found, then the following bits are predicted until there is
a misprediction.  The prediction is computed by mapping the predicted
bit, the length of the match (1..15 or quantized by 4 in 16..62, max 62),
and the last whole byte as context into a StateMap.  If no match is found,
then the order 0 context (last 0..7 bits of the current byte) is used
as context to the StateMap.

The 7 predictions are combined using a neural network (Mixer) as in
paq8l, except it is a single level network without MMX code.  The
inputs p_i, i=0..6 are first stretched: t_i = log(p_i/(1 - p_i)),
then the output is computed: p = squash(SUM_i t_i * w_i), where
squash(x) = 1/(1 + exp(-x)) is the inverse of stretch().  The weights
are adjusted to reduce the error: w_i := w_i + L * t_i * (y - p) where
(y - p) is the prediction error and L ~ 0.002 is the learning rate.
This is a standard single layer backpropagation network modified to
minimize coding cost rather than RMS prediction error (thus dropping
the factors p * (1 - p) from learning).

One of 80 neural networks are selected by a context that depends on
the 3 high order bits of the last whole byte plus the context order
(quantized to 0, 1, 2, 3, 4, 6, 8, 12, 16, 32).  The order is
determined by the number of nonzero bit histories and the length of
the match from MatchModel.

The Mixer output is adjusted by 2 SSE stages (called APM for adaptive
probability map).  An APM is a StateMap that accepts both a discrte
context and an input probability, pr.  pr is stetched and quantized
to 24 levels.  The output is interpolated between the 2 nearest
table entries, and then only the nearest entry is updated.  The entries
are initialized to p = pr and n = 6 (to slow down initial adaptation)
with a limit n <= 255.  The APM differs from paq8l in that it uses
the new StateMap rapid initial adaptation, does not update both
adjacent table entries, and uses 24 levels instead of 33.  The two
stages use a discrete order 0 context (last 0..7 bits) and a hashed order-1
context (14 bits).  Each output is averaged with its input weighted
by 1/4.

The output is arithmetic coded.  The code for a string s with probability
p(s) is a number between Q and Q+p(x) where Q is the total probability
of all strings lexicographically preceding s.  The number is coded as
a big-endian base-256 fraction.  A header is prepended as follows:

- "pQ" 2 bytes must be present or decompression gives an error.
- 1 (0x01) version number (other values give an error).
- memory option N as one byte '0'..'9' (0x30..0x39).
- file size as a 4 byte big-endian number.
- arithmetic coded data.

Two thirds of the memory (2 * 2^N MB) is used for a hash table mapping
the 6 regular contexts (orders 1-4, 6, word) to bit histories.  A lookup
occurs every 4 bits.  The input is a byte-oriented context plus possibly
the first nibble of the next byte.  The output is an array of 15 bit
histories (1 byte each) for all possible contexts formed by appending
0..3 more bits.  The table entries have the format:

 {checksum, "", 0, 1, 00, 10, 01, 11, 000, 100, 010, 110, 001, 101, 011, 111}

The second byte is the bit history for the context ending on a nibble
boundary.  It also serves as a priority for replacement.  The states
are ordered by increasing total count, where state 0 represents the
initial state (no history).  When a context is looked up, the 8 bit
checksum (part of the hash) is compared with 3 adjacent entries, and
if there is no match, the entry with the lowest priority is cleared
and the new checksum is stored.

The hash table is aligned on 64 byte cache lines.  A table lookup never
crosses a 64 byte address boundary.  Given a 32-bit hash h of the context,
8 bits are used for the checksum and 17 + N bits are used for the
index i.  Then the entries i, i XOR 1, and i XOR 2 are tried.  The hash h
is actually a collision-free permuation, consisting of multiplying the
input by a large odd number mod 2^32, a 16-bit rotate, and another multiply.

The order-1 context is mapped to a bit history using a 64K direct
lookup table, not a hash table.

One third of memory is used by MatchModel, divided equally between
a rotating input buffer of 2^(N+19) bytes and an index (hash table)
with 2^(N+17) entries.  Two context hashes are maintained, a long one,
h1, of length ceil((N+17)/3) bytes and a shorter one, h2, of length
ceil((N+17)/5) bytes, where ceil() is the ceiling function.  The index
does not use collision detection.  At each byte boundary, if there is
not currently a match, then the bytes before the current byte are
compared with the location indexed by h1.  If less than 2 bytes match,
then h2 is tried.  If a match of length 1 or more is found, the
match is maintained until the next bit mismatches the predicted bit.
The table is updated at h1 and h2 after every byte.

To compile (g++ 3.4.5, upx 3.00w):
  g++ -Wall lpaq1.cpp -O2 -Os -march=pentiumpro -fomit-frame-pointer
      -s -o lpaq1.exe
  upx -qqq lpaq1.exe

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#define NDEBUG // remove for debugging
#include <assert.h>

// 8, 16, 32 bit unsigned types (adjust as appropriate)
using U8 = unsigned char;
using U16 = unsigned short;
using U32 = unsigned int;

#define FB_SIZE 8192
#define TOLIMIT_10 64
#define TOLIMIT_11 832
#define TOLIMIT_12 1023
#define TOLIMIT_13 1023
#define TOLIMIT_14 1023
#define TOLIMIT_15 6
#define TOLIMIT_16 296
#define TOLIMIT_2a 1023
#define TOLIMIT_2b 1023
#define SQUARD 4
#define MINLEN 3
#define MXR_UPD_7 9
#define MXR_UPD_6 26
#define MXR_UPD_5 27
#define MXR_UPD_4 20
#define MXR_UPD_3 24
#define MXR_UPD_2 28
#define MXR_UPD_1 35
#define MXR_UPD_0 46

#define upd1_cp                                                                                                        \
  int zq = 2 + ( c0 & 3 ) * 2;                                                                                         \
  cp[1] = t4b.get1( 0, zq + hh[1] );                                                                                   \
  cp[5] = t4b.get1( 0, zq + hh[5] );                                                                                   \
  cp[2] = t4a.get1( 128, zq + hh[2] );                                                                                 \
  zq *= 2;                                                                                                             \
  cp[3] = t4a.get3a( 0, zq + hh[3] );                                                                                  \
  cp[4] = t4b.get3a( 128, zq + hh[4] );

#define upd3_cp                                                                                                        \
  hh[4] = hash7( c0 - c4 * 654321893 + ( c8 & 0x00ffffff ) * ( 2039 * 765432197 ) );                                   \
  hh[3] = hash9( c0 - c4 * 991 - ( c8 & 0x80ff ) * 65519 );                                                            \
  hh[5] = hash0( c0 - hh[5] );                                                                                         \
  hh[2] = hash8( c0 - hh[2] );                                                                                         \
  hh[1] = hash6( c0 - hh[1] );                                                                                         \
  cp[1] = t4b.get1( 160, hh[1] );                                                                                      \
  cp[5] = t4b.get1( 160, hh[5] );                                                                                      \
  cp[2] = t4a.get1( 32, hh[2] );                                                                                       \
  cp[3] = t4a.get3b( 160, hh[3] );                                                                                     \
  cp[4] = t4b.get3b( 32, hh[4] );

#define upd5_cp                                                                                                        \
  int zq = 2 + ( c0 & 3 ) * 2;                                                                                         \
  cp[1] = t4b.get1( 192, zq + hh[1] );                                                                                 \
  cp[5] = t4b.get1( 192, zq + hh[5] );                                                                                 \
  cp[2] = t4a.get1( 64, zq + hh[2] );                                                                                  \
  zq *= 2;                                                                                                             \
  cp[3] = t4a.get3b( 192, zq + hh[3] );                                                                                \
  cp[4] = t4b.get3b( 64, zq + hh[4] );

#define upd7_cp                                                                                                        \
  hh[5] = d;                                                                                                           \
  hh[4] = hash4( c4 * 0xfbffefdf - ( ( c8 & 0x00feffff ) + *( U32 * ) &WRT_mxr[( c8 >> 24 ) * 4] ) * 543210973 );      \
  hh[3] = hash3( c4 * ( 479 * 0xfffefdfb ) + ( c8 & 0xc0ff ) * ( 16381 * 0xfffefdfb ) );                               \
  hh[2] = hash2( ( c4 & 0xffffff ) + *( U32 * ) &WRT_mxr[( c4 >> 24 ) * 4] );                                          \
  hh[1] = hash1( c4 & 0x80ffff );                                                                                      \
  cp[1] = t4b.get1( 224, hh[1] );                                                                                      \
  cp[5] = t4b.get1( 224, hh[5] );                                                                                      \
  cp[2] = t4a.get1( 96, hh[2] );                                                                                       \
  cp[3] = t4a.get3a( 224, hh[3] );                                                                                     \
  cp[4] = t4b.get3a( 96, hh[4] );

#define upd7_h123                                                                                                      \
  t0c1 = t0 + c * 256;                                                                                                 \
  h1 = hash1a( c8 * ( 65407 * 234567891 ) - cc * ( 1021 * 234567891 ) + c4 * ( 79 * 234567891 ) ) & HN;

typedef enum { DEFAULT, TEXT, EXE };
U32 method = DEFAULT;

U32 mem_usage = 0;
U32 fb_done = 0;
U8 file_buf[FB_SIZE + 4], *fb_pos = &file_buf[0], *fb_stop = &file_buf[FB_SIZE + 4], *fb_len;
FILE *uncompressed;

int DP_SHIFT = 14;

inline int do_getc() {
  if( fb_pos < fb_stop )
    return *fb_pos++;
  if( fb_stop != &file_buf[FB_SIZE] || fb_stop == fb_len )
    return EOF;

  fb_pos = &file_buf[0];
  *( U32 * ) fb_pos = *( U32 * ) fb_stop;

  fb_stop = fb_len - FB_SIZE;
  if( fb_stop == &file_buf[4] ) {
    fb_len = fb_stop + fread( &file_buf[4], 1, FB_SIZE, uncompressed );
    fb_stop = fb_len;
    if( fb_stop > &file_buf[FB_SIZE] )
      fb_stop = &file_buf[FB_SIZE];
  }
  return *fb_pos++;
}

inline void do_putc( U8 c ) {
  if( fb_pos >= fb_stop ) {
    fwrite( &file_buf[0], 1, FB_SIZE, uncompressed );
    fb_pos = &file_buf[4];
    *( U32 * ) ( fb_pos - 4 ) = *( U32 * ) ( fb_stop - 4 );
  }
  *fb_pos++ = c;
}

inline void do_putc_end() {
  int LEN = fb_pos - &file_buf[0];
  fwrite( &file_buf[0], 1, LEN, uncompressed );
}

// Error handler: print message if any, and exit
void quit( const char *message = 0 ) {
  if( message != nullptr )
    printf( "%s\n", message );
  exit( 1 );
}

// Create an array p of n elements of type T
template <class T>
void alloc( T *&p, int n ) {
  p = ( T * ) calloc( n, sizeof( T ) );
  mem_usage += n * sizeof( T );
  if( !p )
    quit( "out of memory" );
}

///////////////////////////// Squash //////////////////////////////

int squash_t[4096]; //initialized when Encoder is created

#define squash( x ) squash_t[( x ) + 2047]

// return p = 1/(1 + exp(-d)), d scaled by 8 bits, p scaled by 12 bits
int squash_init( int d ) {
  if( d >= 2047 )
    return 4095;
  if( d <= -2047 )
    return 0;
  double k = 4096 / ( double( 1 + exp( -( double( d ) / 256 ) ) ) );
  return int( k );
}

//////////////////////////// Stretch ///////////////////////////////

// Inverse of squash. stretch(d) returns ln(p/(1-p)), d scaled by 8 bits,
// p by 12 bits.  d has range -2047 to 2047 representing -8 to 8.
// p has range 0 to 4095 representing 0 to 1.

static int stretch_t[4096]; //initialized when Encoder is created
static int stretch_t2[4096];
static int dt[1024];           // i -> 16K/(i+3)
static int dta[1024];          // i ->  8K/(i+3)
static U8 calcfails[8192 * 8]; //as above, initialized when Encoder is created

static int y20;        // y<<20
static int c0 = 1;     // last 0-7 bits with leading 1
static int bcount = 7; // bit count
static U8 *buf;        // input buffer

U32 smt[256 * 67], cxt0 = 0, cxt1 = 0, cxt2 = 0, cxt3 = 0, cxt4 = 0, cxt5 = 0, cxtm = 0;
U8 t0[0x10000];                                                // order 1 cxt -> state
U8 *t0c1 = t0, *cp[6] = {t0, t0, t0, t0, t0, t0};              // pointer to bit history
U32 h5 = 0, hh[6], bc4cp0 = 0, pw = 0, c4 = 0, c8 = 0, cc = 0; // last 4, 8, 12 bytes
int pos = 0;                                                   // number of bytes in buf
int c1 = 0, c2 = 0;                                            // last two higher 4-bit nibbles
int MEM = 0;                                                   // Global memory usage = 3*MEM bytes (1<<20 .. 1<<29)
U8 fails = 0;

///////////////////////// state table ////////////////////////

// State table:
//   nex(state, 0) = next state if bit y is 0, 0 <= state < 256
//   nex(state, 1) = next state if bit y is 1
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

static const U8 State_table[512 * 6] = {
    1,   3,   4,   7,   8,   9,   11,  15,  16,  17,  18,  20,  21,  22,  26,  31,  32,  32,  32,  32,  34,  34,
    34,  34,  34,  34,  36,  36,  36,  36,  38,  41,  42,  42,  44,  44,  46,  46,  48,  48,  50,  53,  54,  54,
    56,  56,  58,  58,  60,  60,  62,  62,  50,  67,  68,  68,  70,  70,  72,  72,  74,  74,  76,  76,  62,  62,
    64,  83,  84,  84,  86,  86,  44,  44,  58,  58,  60,  60,  76,  76,  78,  78,  80,  93,  94,  94,  96,  96,
    48,  48,  88,  88,  80,  103, 104, 104, 106, 106, 62,  62,  88,  88,  80,  113, 114, 114, 116, 116, 62,  62,
    88,  88,  90,  123, 124, 124, 126, 126, 62,  62,  98,  98,  90,  133, 134, 134, 136, 136, 62,  62,  98,  98,
    90,  143, 144, 144, 68,  68,  62,  62,  98,  98,  100, 149, 150, 150, 108, 108, 100, 153, 154, 108, 100, 157,
    158, 108, 100, 161, 162, 108, 110, 165, 166, 118, 110, 169, 170, 118, 110, 173, 174, 118, 110, 177, 178, 118,
    110, 181, 182, 118, 120, 185, 186, 128, 120, 189, 190, 128, 120, 193, 194, 128, 120, 197, 198, 128, 120, 201,
    202, 128, 120, 205, 206, 128, 120, 209, 210, 128, 130, 213, 214, 138, 130, 217, 218, 138, 130, 221, 222, 138,
    130, 225, 226, 138, 130, 229, 230, 138, 130, 233, 234, 138, 130, 237, 238, 138, 130, 241, 242, 138, 130, 245,
    246, 138, 140, 249, 250, 80,  140, 253, 254, 80,  140, 253, 254, 80,

    2,   2,   6,   5,   9,   13,  14,  11,  17,  25,  21,  27,  19,  29,  30,  23,  33,  49,  37,  51,  41,  53,
    45,  55,  35,  57,  43,  59,  39,  61,  62,  47,  65,  97,  69,  99,  73,  101, 77,  103, 81,  105, 85,  107,
    89,  109, 93,  111, 67,  113, 75,  115, 83,  117, 91,  119, 71,  121, 87,  123, 79,  125, 126, 95,  65,  97,
    69,  99,  73,  101, 77,  103, 81,  105, 85,  107, 89,  109, 93,  111, 65,  97,  69,  99,  73,  101, 77,  103,
    81,  105, 85,  107, 89,  109, 93,  111, 67,  113, 75,  115, 83,  117, 91,  119, 67,  113, 75,  115, 83,  117,
    91,  119, 71,  121, 87,  123, 71,  121, 87,  123, 79,  125, 79,  125, 79,  130, 128, 95,  132, 95,  134, 79,
    136, 95,  138, 79,  140, 95,  142, 79,  144, 95,  146, 79,  148, 95,  150, 79,  152, 95,  154, 79,  156, 95,
    158, 79,  156, 95,  160, 79,  162, 103, 164, 103, 166, 103, 168, 103, 170, 103, 172, 103, 174, 103, 176, 103,
    178, 103, 180, 103, 182, 103, 184, 103, 186, 103, 188, 103, 190, 103, 192, 103, 194, 103, 196, 103, 198, 103,
    200, 103, 202, 115, 204, 115, 206, 115, 208, 115, 210, 115, 212, 115, 214, 115, 216, 115, 218, 115, 220, 115,
    222, 115, 224, 115, 226, 115, 228, 115, 230, 115, 232, 115, 234, 115, 236, 115, 238, 115, 240, 115, 242, 115,
    244, 115, 246, 115, 248, 115, 250, 115, 252, 115, 254, 115, 254, 115,

    1,   3,   5,   7,   9,   11,  13,  15,  17,  19,  21,  23,  25,  27,  29,  31,  33,  35,  37,  39,  41,  43,
    45,  47,  49,  51,  53,  55,  57,  59,  61,  63,  65,  67,  69,  71,  73,  75,  77,  79,  81,  83,  85,  87,
    89,  91,  93,  95,  97,  99,  101, 103, 105, 107, 109, 111, 113, 115, 117, 119, 121, 123, 125, 127, 129, 131,
    133, 135, 137, 139, 141, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175,
    177, 179, 181, 183, 185, 187, 189, 191, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219,
    221, 223, 225, 227, 229, 231, 233, 235, 237, 239, 241, 243, 245, 247, 249, 251, 253, 127, 129, 131, 133, 135,
    137, 139, 141, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 177, 179,
    181, 183, 185, 187, 189, 191, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221, 223,
    225, 227, 229, 231, 233, 235, 237, 239, 241, 243, 245, 247, 249, 251, 189, 255, 129, 131, 133, 135, 137, 139,
    141, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 177, 179, 181, 183,
    185, 187, 189, 191, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221, 223, 225, 227,
    229, 231, 233, 235, 237, 239, 241, 243, 245, 247, 249, 251, 253, 255,

    1,   3,   5,   6,   9,   8,   10,  13,  12,  14,  15,  18,  17,  20,  19,  21,  24,  23,  26,  25,  27,  28,
    31,  30,  33,  32,  35,  34,  36,  39,  38,  41,  40,  43,  42,  44,  45,  48,  47,  50,  49,  52,  51,  54,
    53,  55,  58,  57,  60,  59,  62,  61,  64,  63,  65,  66,  69,  68,  71,  70,  73,  72,  75,  74,  77,  76,
    78,  81,  80,  83,  82,  85,  84,  87,  86,  89,  88,  90,  91,  94,  93,  96,  95,  98,  97,  100, 99,  102,
    101, 104, 103, 105, 108, 107, 110, 109, 112, 111, 114, 113, 116, 115, 118, 117, 119, 105, 121, 120, 123, 122,
    125, 124, 127, 126, 129, 128, 131, 130, 133, 132, 134, 137, 38,  139, 138, 141, 140, 143, 142, 145, 144, 147,
    146, 148, 149, 39,  47,  152, 151, 154, 153, 156, 155, 158, 157, 160, 159, 162, 161, 163, 48,  59,  166, 49,
    168, 167, 170, 169, 172, 171, 174, 173, 175, 176, 48,  59,  179, 178, 181, 180, 183, 182, 185, 184, 187, 186,
    188, 58,  72,  191, 190, 193, 192, 195, 194, 197, 196, 198, 199, 58,  72,  202, 201, 204, 203, 206, 205, 208,
    207, 209, 69,  86,  212, 211, 214, 213, 216, 215, 217, 218, 69,  86,  221, 220, 223, 222, 225, 224, 226, 81,
    101, 229, 228, 231, 230, 232, 233, 81,  101, 236, 235, 238, 237, 239, 94,  117, 242, 241, 243, 244, 94,  117,
    247, 246, 248, 108, 132, 250, 251, 108, 246, 253, 121, 255, 121, 255,

    1,   4,   3,   6,   8,   7,   11,  10,  13,  12,  15,  17,  16,  19,  18,  22,  21,  24,  23,  26,  25,  28,
    30,  29,  32,  31,  34,  33,  37,  36,  39,  38,  41,  40,  43,  42,  45,  47,  46,  49,  48,  51,  50,  53,
    52,  56,  55,  58,  57,  60,  59,  62,  61,  64,  63,  66,  68,  67,  70,  69,  72,  71,  74,  73,  76,  75,
    79,  78,  81,  80,  83,  82,  85,  84,  87,  86,  89,  88,  91,  93,  92,  95,  94,  97,  96,  99,  98,  101,
    100, 103, 102, 106, 105, 108, 107, 110, 109, 112, 111, 114, 113, 116, 115, 118, 117, 120, 122, 121, 124, 123,
    126, 125, 128, 127, 130, 129, 132, 131, 118, 133, 135, 134, 137, 136, 139, 138, 141, 140, 143, 142, 41,  144,
    147, 146, 149, 151, 150, 153, 152, 155, 154, 157, 156, 159, 158, 74,  160, 161, 52,  164, 163, 166, 165, 168,
    167, 170, 169, 172, 171, 85,  173, 174, 52,  176, 178, 177, 180, 179, 182, 181, 184, 183, 97,  185, 186, 63,
    189, 188, 191, 190, 193, 192, 195, 194, 97,  196, 197, 63,  199, 201, 200, 203, 202, 205, 204, 110, 206, 207,
    75,  210, 209, 212, 211, 214, 213, 124, 215, 216, 75,  218, 220, 219, 222, 221, 124, 223, 224, 88,  227, 226,
    229, 228, 137, 230, 231, 88,  233, 235, 234, 151, 236, 237, 102, 240, 239, 151, 241, 242, 102, 244, 164, 245,
    246, 117, 176, 248, 249, 117, 244, 251, 133, 253, 133, 255, 148, 255,

    10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  12,  14,  68,  70,  70,  70,  10,  10,  10,  10,  10,  10,
    10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
    10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
    10,  10,  68,  70,  70,  70,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
    10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
    10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
    10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
    10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
    10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
    10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
    10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
    10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,

    2,   5,   6,   10,  12,  13,  14,  19,  23,  24,  25,  27,  28,  29,  30,  33,  35,  35,  35,  35,  37,  37,
    37,  37,  37,  37,  39,  39,  39,  39,  40,  43,  45,  45,  47,  47,  49,  49,  51,  51,  52,  43,  57,  57,
    59,  59,  61,  61,  63,  63,  65,  65,  66,  55,  57,  57,  73,  73,  75,  75,  77,  77,  79,  79,  81,  81,
    82,  69,  71,  71,  73,  73,  59,  59,  61,  61,  49,  49,  89,  89,  91,  91,  92,  69,  87,  87,  45,  45,
    99,  99,  101, 101, 102, 69,  87,  87,  57,  57,  109, 109, 111, 111, 112, 85,  87,  87,  57,  57,  119, 119,
    121, 121, 122, 85,  97,  97,  57,  57,  129, 129, 131, 131, 132, 85,  97,  97,  57,  57,  139, 139, 141, 141,
    142, 95,  97,  97,  57,  57,  81,  81,  147, 147, 148, 95,  107, 107, 151, 151, 152, 95,  107, 155, 156, 95,
    107, 159, 160, 105, 107, 163, 164, 105, 117, 167, 168, 105, 117, 171, 172, 105, 117, 175, 176, 105, 117, 179,
    180, 115, 117, 183, 184, 115, 127, 187, 188, 115, 127, 191, 192, 115, 127, 195, 196, 115, 127, 199, 200, 115,
    127, 203, 204, 115, 127, 207, 208, 125, 127, 211, 212, 125, 137, 215, 216, 125, 137, 219, 220, 125, 137, 223,
    224, 125, 137, 227, 228, 125, 137, 231, 232, 125, 137, 235, 236, 125, 137, 239, 240, 125, 137, 243, 244, 135,
    137, 247, 248, 135, 69,  251, 252, 135, 69,  255, 252, 135, 69,  255,

    3,   3,   4,   7,   12,  10,  8,   15,  24,  18,  26,  22,  28,  20,  16,  31,  48,  34,  50,  38,  52,  42,
    54,  46,  56,  36,  58,  44,  60,  40,  32,  63,  96,  66,  98,  70,  100, 74,  102, 78,  104, 82,  106, 86,
    108, 90,  110, 94,  112, 68,  114, 76,  116, 84,  118, 92,  120, 72,  122, 88,  124, 80,  64,  127, 96,  66,
    98,  70,  100, 74,  102, 78,  104, 82,  106, 86,  108, 90,  110, 94,  96,  66,  98,  70,  100, 74,  102, 78,
    104, 82,  106, 86,  108, 90,  110, 94,  112, 68,  114, 76,  116, 84,  118, 92,  112, 68,  114, 76,  116, 84,
    118, 92,  120, 72,  122, 88,  120, 72,  122, 88,  124, 80,  124, 80,  131, 80,  64,  129, 64,  133, 80,  135,
    64,  137, 80,  139, 64,  141, 80,  143, 64,  145, 80,  147, 64,  149, 80,  151, 64,  153, 80,  155, 64,  157,
    80,  159, 64,  157, 80,  161, 104, 163, 104, 165, 104, 167, 104, 169, 104, 171, 104, 173, 104, 175, 104, 177,
    104, 179, 104, 181, 104, 183, 104, 185, 104, 187, 104, 189, 104, 191, 104, 193, 104, 195, 104, 197, 104, 199,
    104, 201, 116, 203, 116, 205, 116, 207, 116, 209, 116, 211, 116, 213, 116, 215, 116, 217, 116, 219, 116, 221,
    116, 223, 116, 225, 116, 227, 116, 229, 116, 231, 116, 233, 116, 235, 116, 237, 116, 239, 116, 241, 116, 243,
    116, 245, 116, 247, 116, 249, 116, 251, 116, 253, 116, 255, 116, 255,

    2,   4,   6,   8,   10,  12,  14,  16,  18,  20,  22,  24,  26,  28,  30,  32,  34,  36,  38,  40,  42,  44,
    46,  48,  50,  52,  54,  56,  58,  60,  62,  64,  66,  68,  70,  72,  74,  76,  78,  80,  82,  84,  86,  88,
    90,  92,  94,  96,  98,  100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132,
    134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176,
    178, 180, 182, 184, 186, 188, 190, 192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220,
    222, 224, 226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 254, 128, 130, 132, 134, 136,
    138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180,
    182, 184, 186, 188, 190, 192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222, 224,
    226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 190, 192, 130, 132, 134, 136, 138, 140,
    142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184,
    186, 188, 190, 192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222, 224, 226, 228,
    230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 254, 192,

    2,   5,   4,   8,   7,   9,   12,  11,  14,  13,  17,  16,  19,  18,  20,  23,  22,  25,  24,  27,  26,  30,
    29,  32,  31,  34,  33,  35,  38,  37,  40,  39,  42,  41,  44,  43,  47,  46,  49,  48,  51,  50,  53,  52,
    54,  57,  56,  59,  58,  61,  60,  63,  62,  65,  64,  68,  67,  70,  69,  72,  71,  74,  73,  76,  75,  77,
    80,  79,  82,  81,  84,  83,  86,  85,  88,  87,  90,  89,  93,  92,  95,  94,  97,  96,  99,  98,  101, 100,
    103, 102, 104, 107, 106, 109, 108, 111, 110, 113, 112, 115, 114, 117, 116, 119, 118, 120, 106, 122, 121, 124,
    123, 126, 125, 128, 127, 130, 129, 132, 131, 133, 136, 135, 138, 39,  140, 139, 142, 141, 144, 143, 146, 145,
    148, 147, 38,  150, 151, 48,  153, 152, 155, 154, 157, 156, 159, 158, 161, 160, 162, 47,  164, 165, 60,  167,
    50,  169, 168, 171, 170, 173, 172, 175, 174, 47,  177, 178, 60,  180, 179, 182, 181, 184, 183, 186, 185, 187,
    57,  189, 190, 73,  192, 191, 194, 193, 196, 195, 198, 197, 57,  200, 201, 73,  203, 202, 205, 204, 207, 206,
    208, 68,  210, 211, 87,  213, 212, 215, 214, 217, 216, 68,  219, 220, 87,  222, 221, 224, 223, 225, 80,  227,
    228, 102, 230, 229, 232, 231, 80,  234, 235, 102, 237, 236, 238, 93,  240, 241, 118, 243, 242, 93,  245, 246,
    118, 247, 107, 249, 250, 133, 107, 252, 247, 120, 254, 120, 254, 134,

    2,   3,   5,   7,   6,   9,   10,  12,  11,  14,  16,  15,  18,  17,  20,  21,  23,  22,  25,  24,  27,  29,
    28,  31,  30,  33,  32,  35,  36,  38,  37,  40,  39,  42,  41,  44,  46,  45,  48,  47,  50,  49,  52,  51,
    54,  55,  57,  56,  59,  58,  61,  60,  63,  62,  65,  67,  66,  69,  68,  71,  70,  73,  72,  75,  74,  77,
    78,  80,  79,  82,  81,  84,  83,  86,  85,  88,  87,  90,  92,  91,  94,  93,  96,  95,  98,  97,  100, 99,
    102, 101, 104, 105, 107, 106, 109, 108, 111, 110, 113, 112, 115, 114, 117, 116, 119, 121, 120, 123, 122, 125,
    124, 127, 126, 129, 128, 131, 130, 133, 132, 119, 134, 136, 135, 138, 137, 140, 139, 142, 141, 144, 143, 42,
    145, 148, 150, 149, 152, 151, 154, 153, 156, 155, 158, 157, 160, 159, 75,  51,  162, 163, 165, 164, 167, 166,
    169, 168, 171, 170, 173, 172, 86,  51,  175, 177, 176, 179, 178, 181, 180, 183, 182, 185, 184, 98,  62,  187,
    188, 190, 189, 192, 191, 194, 193, 196, 195, 98,  62,  198, 200, 199, 202, 201, 204, 203, 206, 205, 111, 74,
    208, 209, 211, 210, 213, 212, 215, 214, 125, 74,  217, 219, 218, 221, 220, 223, 222, 125, 87,  225, 226, 228,
    227, 230, 229, 138, 87,  232, 234, 233, 236, 235, 152, 101, 238, 239, 241, 240, 152, 101, 243, 245, 244, 165,
    116, 247, 248, 177, 116, 250, 245, 132, 252, 132, 254, 147, 254, 147,

    11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  13,  15,  69,  69,  69,  71,  11,  11,  11,  11,  11,  11,
    11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
    11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
    11,  11,  69,  69,  69,  71,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
    11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
    11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
    11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
    11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
    11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
    11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
    11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
    11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
};
#define nex( state, sel ) *( p + state + sel * 256 )
#define neq( state, sel ) *( q + state + sel * 256 )

//////////////////////////// StateMap, APM //////////////////////////

// A StateMap maps a context to a probability.  Methods:
//
// Statemap sm(n) creates a StateMap with n contexts using 4*n bytes memory.
// sm.p(y, cx, limit) converts state cx (0..n-1) to a probability (0..4095).
//	that the next y=1, updating the previous prediction with y (0..1).
//	limit (1..1023, default 1023) is the maximum count for computing a
//	prediction.  Larger values are better for stationary sources.

U8 WRT_mtt[256] = {
    0,      0,      0,      0,      0,      0,      3 * 32, 7 * 32, 0,      2 * 32, 4 * 32, 0,      1 * 32, 0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      1 * 32, 0,      0,      0,
    0,      0,      0,      0,      2 * 32, 4 * 32, 3 * 32, 3 * 32, 4 * 32, 6 * 32, 6 * 32, 7 * 32, 5 * 32, 4 * 32,
    0,      4 * 32, 3 * 32, 4 * 32, 0,      1 * 32, 32,     32,     32,     32,     32,     32,     32,     32,
    32,     32,     7 * 32, 3 * 32, 3 * 32, 4 * 32, 3 * 32, 4 * 32,

    0,      5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32,
    5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 6 * 32,
    4 * 32, 3 * 32, 4 * 32, 4 * 32,

    4 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32,
    5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 5 * 32, 4 * 32,
    4 * 32, 4 * 32, 4 * 32, 4 * 32,

    6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32,
    6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32,
    6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32,
    6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32,
    6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32,
    6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32, 6 * 32,

    7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32,
    7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32,
    7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32,
    7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32, 7 * 32,
};

#define zz 0, 0, 0, 2 *

U8 WRT_mxr[256 * 4] = {
    zz 0,  zz 0,  zz 0, zz 0,  zz 0,  zz 0,  zz 5,  zz 10, zz 0,  zz 2,  zz 14, zz 0,  zz 0,  zz 0,
    zz 0,  zz 0,  zz 0, zz 0,  zz 0,  zz 0,  zz 0,  zz 0,  zz 0,  zz 0,  zz 5,  zz 0,  zz 0,  zz 0,
    zz 0,  zz 0,  zz 0, zz 0,  zz 2,  zz 10, zz 13, zz 10, zz 11, zz 14, zz 13, zz 11, zz 15, zz 3,
    zz 14, zz 15, zz 3, zz 15, zz 12, zz 15, // _!"#$%&' ()*+,-./
    zz 1,  zz 1,  zz 1, zz 1,  zz 1,  zz 1,  zz 1,  zz 1,  zz 1,  zz 1,  zz 10, zz 15, zz 13, zz 11,
    zz 0,  zz 15, // 01234567 89:;<=>?

    zz 9,  zz 5,  zz 5, zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,
    zz 5,  zz 5, //  ABCDEFG HIJKLMNO
    zz 5,  zz 5,  zz 5, zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 4,  zz 10, zz 3,
    zz 14, zz 15, // PQRSTUVW XYZ[\]^_

    zz 10, zz 5,  zz 5, zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,
    zz 5,  zz 5, //  abcdefg hijklmno
    zz 5,  zz 5,  zz 5, zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 5,  zz 15, zz 10, zz 11,
    zz 15, zz 10, // pqrstuvw xyz{|}~

    zz 6,  zz 6,  zz 6, zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,
    zz 6,  zz 6,  zz 6, zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,
    zz 6,  zz 6,  zz 6, zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,
    zz 6,  zz 6,  zz 6, zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,
    zz 6,  zz 6,  zz 6, zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,
    zz 6,  zz 6,  zz 6, zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,  zz 6,

    zz 7,  zz 7,  zz 7, zz 7,  zz 7,  zz 7,  zz 7,  zz 7,  zz 7,  zz 7,  zz 7,  zz 7,  zz 7,  zz 7,
    zz 7,  zz 7,  zz 7, zz 7,  zz 7,  zz 7,  zz 7,  zz 7,  zz 7,  zz 7,  zz 7,  zz 7,  zz 7,  zz 7,
    zz 7,  zz 7,  zz 7, zz 7,  zz 8,  zz 8,  zz 8,  zz 8,  zz 8,  zz 8,  zz 8,  zz 8,  zz 8,  zz 8,
    zz 8,  zz 8,  zz 8, zz 8,  zz 8,  zz 8,
};

U8 WRT_wrd[256] = {
    0,   0,   0,   0,   0,   0,   192, 0,   0,   0,   64,  0,   64,  0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   192, 0,   0,   0,   0,   0,   0,   0,   0,   0,   192, 0,   0,   0,   0,   0,   0,   0,   0,   0,
    128, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   128, 0,   128, 0,

    0,   64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,
    64,  64,  64,  64,  64,  0,   192, 0,   0,   0,

    0,   64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,
    64,  64,  64,  64,  64,  0,   0,   0,   0,   0,

    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,

    192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
    192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
    192, 192, 192, 192,
};

U8 WRT_chc1[64] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x01, 0x12, 0x13, 0x14, 0x05, 0x16, 0x17, 0x18, 0x09, 0x1a, 0x1b, 0x1c, 0x0d, 0x1e, 0x1f,
    0x20, 0x11, 0x22, 0x23, 0x24, 0x15, 0x26, 0x27, 0x28, 0x19, 0x2a, 0x2b, 0x2c, 0x1d, 0x2e, 0x2f,
    0x30, 0x11, 0x32, 0x33, 0x34, 0x15, 0x36, 0x37, 0x38, 0x19, 0x3a, 0x3b, 0x3c, 0x1d, 0x3e, 0x3f,
};

int limits_15a[256] = {
    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 24, 24, 16, 16, 16, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 2, 176, 176, 2, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

int limits_15b[256] = {
    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 18, 18, 12, 12, 12, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 1, 168, 168, 1, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,   0,   0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// update bit y (0..1), predict next bit in context cx

#define smw( cxt, adda, sh, ml, mxrn )                                                                                 \
  {                                                                                                                    \
    assert( y >> 1 == 0 );                                                                                             \
    assert( cx >= 0 && cx < N );                                                                                       \
    assert( cxt >= 0 && cxt < N );                                                                                     \
    int p0 = smt[adda + cxt];                                                                                          \
    p0 += ( y20 - p0 ) * ml >> sh;                                                                                     \
    smt[adda + cxt] = p0;                                                                                              \
    cxt = cx;                                                                                                          \
    mxr_tx[mxrn] = stretch_t[smt[adda + cx] >> 8];                                                                     \
  }

#define smzr( rv, cxt, adda, sh, ml, finaladd )                                                                        \
  {                                                                                                                    \
    U32 *g = &smt[adda];                                                                                               \
    {                                                                                                                  \
      int p0 = *( g + cxt );                                                                                           \
      p0 += ( y2o - p0 ) * ml >> sh;                                                                                   \
      *( g + cxt ) = p0;                                                                                               \
    }                                                                                                                  \
    cxt = *cp[rv];                                                                                                     \
    mxr_tx[rv] = stretch_t[*( g + finaladd + cxt ) >> 8];                                                              \
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

#define APMw 24
#define APMmul ( ( 4096 - 0.0001 ) / ( APMw - 1 ) )

class APM {
protected:
  U32 *tcxt; // Context of last prediction
  U32 *t;    // cxt -> prediction in high 22 bits, count in low 10 bits
public:
  APM( int n, int s ) {
    alloc( t, n );
    tcxt = t;
    int i;
    for( i = 0; i < APMw; ++i ) {
      int p = ( int ) ( i * APMmul - 2048 );
      t[i] = ( U32( squash_init( p ) ) << 20 ) + s;
    }
    for( ; i < n; ++i ) {
      t[i] = t[i - APMw];
    }
  }

  inline U32 p1( U32 pr, int cx ) { //, int limit=1023)
    {
      U32 p0 = *tcxt;
      U32 i = p0 & 1023, pd = p0 >> 12; // count, prediction
      p0 += static_cast<unsigned int>( i < TOLIMIT_2a );
      p0 += ( ( y20 - ( int ) pd ) * dt[i] + 0x380 ) & 0xfffffc00;
      *tcxt = p0;
    }
    pr *= APMw - 1;
    int wt = pr & 0xfff; // interpolation weight of next element
    U32 *tcx = t + cx * APMw + ( pr >> 12 ), v0;
    v0 = *tcx;
    int v1 = *( tcx + 1 );
    pr = v0 >> 12;
    v1 = ( ( U32 ) v1 >> 12 ) - pr;
    pr = ( v1 * wt + v0 ) >> 20;
    tcxt = tcx + ( wt >> 11 );
    return pr;
  }

  inline U32 p2( U32 pr, int cx ) { //, int limit=1023)
    assert( y >> 1 == 0 );
    assert( cx >= 0 && cx < N / 24 );
    assert( cxt >= 0 && cxt < N );
    {
      U32 p0 = *tcxt;
      U32 i = p0 & 1023, pr = p0 >> 12; // count, prediction
      p0 += static_cast<unsigned int>( i < TOLIMIT_2b );
      p0 += ( ( y20 - ( int ) pr ) * dta[i] + 0x180 ) & 0xfffffc00;
      *tcxt = p0;
    }
    int wt = pr & 0xfff; // interpolation weight of next element
    U32 *tcx = t + cx * APMw + ( pr >> 12 ), v0;
    v0 = *tcx;
    int v1 = *( tcx + 1 );
    pr = v0 >> 12;
    v1 = ( ( U32 ) v1 >> 12 ) - pr;
    pr = ( v1 * wt + v0 ) >> 20;
    tcxt = tcx + ( wt >> 11 );
    return pr;
  }
};

APM a1( APMw * 0x10000, 18 ), a2( APMw * 0x800, 40 );

//////////////////////////// Mixer /////////////////////////////

// Mixer m(MI, MC) combines models using MC neural networks with
//	MI inputs each using 4*MC*MI bytes of memory.  It is used as follows:
// m.update(y) trains the network where the expected output is the
//	last bit, y.
// m.add(stretch(p)) inputs prediction from one of MI models.  The
//	prediction should be positive to predict a 1 bit, negative for 0,
//	nominally -2K to 2K.
// m.set(cxt) selects cxt (0..MC-1) as one of MC neural networks to use.
// m.p() returns the output prediction that the next bit is 1 as a
//	12 bit number (0 to 4095).  The normal sequence per prediction is:
//
// - m.add(x) called MI times with input x=(-2047..2047)
// - m.set(cxt) called once with cxt=(0..MC-1)
// - m.p() called once to predict the next bit, returns 0..4095
// - m.update(y) called once for actual bit y=(0..1).

#define MI 8
#define MC 1280
int mxr_tx[MI];    // MI inputs
int *mxr_wx;       // MI*MC weights
int *mxr_cxt;      // context
int mxr_pr = 2048; // last result (scaled 12 bits)
U32 mmask = 0xffffffff;

#if 0 // ATTENTION !  CHANGE this to 1 if you start to use                                                             \
      //		<mixer max inputs>!=8 in your versions.
inline void train(int err) {
  int *w=mxr_cxt;
  assert(err>=-32768 && err<32768);
  for (int i=0; i<MI; ++i)
    w[i]+=mxr_tx[i]*err+0x4000>>15;
}

inline int dot_product() {
  int *w=mxr_cxt;
  int sum=0;
  for (int i=0; i<MI; ++i)
    sum+=mxr_tx[i]*w[i];
  sum>>=DP_SHIFT;
  if (sum<-2047) sum=-2047;
  if (sum> 2047) sum= 2047;
  return sum;
}

#else

#  define train( err, MS0, MS1, MS2, MS3, MS4, MS5, MS6, MS7 )                                                         \
    {                                                                                                                  \
      int *w = mxr_cxt;                                                                                                \
      assert( err >= -32768 && err < 32768 );                                                                          \
      w[0] += mxr_tx[0] * err + ( 1 << ( MS0 - 1 ) ) >> MS0;                                                           \
      w[1] += mxr_tx[1] * err + ( 1 << ( MS1 - 1 ) ) >> MS1;                                                           \
      w[2] += mxr_tx[2] * err + ( 1 << ( MS2 - 1 ) ) >> MS2;                                                           \
      w[3] += mxr_tx[3] * err + ( 1 << ( MS3 - 1 ) ) >> MS3;                                                           \
      w[4] += mxr_tx[4] * err + ( 1 << ( MS4 - 1 ) ) >> MS4;                                                           \
      w[5] += mxr_tx[5] * err + ( 1 << ( MS5 - 1 ) ) >> MS5;                                                           \
      w[6] += mxr_tx[6] * err + ( 1 << ( MS6 - 1 ) ) >> MS6;                                                           \
      w[7] += mxr_tx[7] * err + ( 1 << ( MS7 - 1 ) ) >> MS7;                                                           \
    }

inline int dot_product() {
  int *w = mxr_cxt;
  int sum = mxr_tx[0] * w[0];
  sum += mxr_tx[1] * w[1];
  sum += mxr_tx[2] * w[2];
  sum += mxr_tx[3] * w[3];
  sum += mxr_tx[4] * w[4];
  sum += mxr_tx[5] * w[5];
  sum += mxr_tx[6] * w[6];
  sum += mxr_tx[7] * w[7];
  sum >>= DP_SHIFT;
  if( sum < -2047 )
    sum = -2047;
  if( sum > 2047 )
    sum = 2047;
  return sum;
}
#endif

///class Mixer {
///public:
///  Mixer(int m);

// Adjust weights to minimize coding cost of last prediction
#define m_update( y, bitcount, MU, MS0, MS1, MS2, MS3, MS4, MS5, MS6, MS7 )                                            \
  {                                                                                                                    \
    int err = y * 0xfff - mxr_pr;                                                                                      \
    fails <<= 1;                                                                                                       \
    if( err < MU && err > -MU ) {                                                                                      \
    } else {                                                                                                           \
      fails |= calcfails[err + 4096 + 8192 * bitcount];                                                                \
      train( err, MS0, MS1, MS2, MS3, MS4, MS5, MS6, MS7 );                                                            \
    }                                                                                                                  \
  }

// Input x (call up to MI times)

//#define m_add(a,b) { assert((a)<MI); mxr_tx[a]=stretch_t[b]; }

// predict next bit
#define m_p dot_product()

///};

inline void smpm( int cx ) smw( cxtm, 12 * 256, 3, 1, 0 ) inline void sm7m( int cx )
    smw( cxtm, 12 * 256, 2, 1, 0 ) inline void smp0n( int cx ) smw( cxt0, 0 * 256, 6, 1, 6 ) inline void smp7n( int cx )
        smw( cxt0, 0 * 256, 6, 1, 6 ) inline void smp0w( int cx )
            smw( cxt0, 0 * 256, 7, 1, 6 ) inline void smp7w( int cx ) smw( cxt0, 0 * 256, 11, 1, 6 )

    //////////////////////////// HashTable /////////////////////////

    // A HashTable maps a 32-bit index to an array of B bytes.
    // The first byte is a checksum using the upper 8 bits of the
    // index.  The second byte is a priority (0 = empty) for hash
    // replacement.  The index need not be a hash.

    // HashTable<B> h(n) - create using n bytes  n and B must be
    //     powers of 2 with n >= B*4, and B >= 2.
    // h[i] returns array [1..B-1] of bytes indexed by i, creating and
    //     replacing another element if needed.  Element 0 is the
    //     checksum and should not be modified.

    int NB; // size in bytes

template <int B>
class HashTable {
public:
  U8 *t; // table: 1 element = B bytes: checksum,priority,data,data,...
  U8 *get1( U32 o, U32 i );
  U8 *get3a( U32 o, U32 i );
  U8 *get3b( U32 o, U32 i );
  void ht_init();
};

template <int B>
void HashTable<B>::ht_init() {
  int n = MEM;
  NB = n / B - 1;
  assert( B >= 2 && ( B & B - 1 ) == 0 );
  //sert(n>=B*4 && (n&n-1)==0);
  alloc( t, n + 512 + B * 2 );
  t = ( U8 * ) ( ( ( long ) t + 511 ) & -512 ) + 1; // align on cache line boundary
}

#define RORi( x, y ) x << ( 32 - y ) | x >> y

inline U32 hash1( U32 i ) {
  i *= 654321893;
  i = RORi( i, 9 );
  return ( i * 0xfffefdfb );
}
inline U32 hash2( U32 i ) {
  i *= 0xfffefdfb;
  i = RORi( i, 12 );
  return ( i * 654321893 );
}
inline U32 hash3( U32 i ) {
  i = RORi( i, 10 );
  return ( i * 543210973 );
}
inline U32 hash4( U32 i ) {
  i = RORi( i, 9 );
  return ( i * 0xfffefdfb );
}
inline U32 hash5( U32 i ) {
  i *= 0xfeff77ff;
  i = RORi( i, 14 );
  return ( i * 987654323 );
}
inline U32 hash6( U32 i ) {
  i *= 876543211;
  i = RORi( i, 14 );
  return ( i * 345678941 );
}
inline U32 hash7( U32 i ) {
  i = RORi( i, 12 );
  return ( i * 0xfbffefdf );
}
inline U32 hash8( U32 i ) {
  i *= 345678941;
  i = RORi( i, 15 );
  return ( i * 345678941 );
}
inline U32 hash9( U32 i ) {
  i *= 0xfefdff7f;
  i = RORi( i, 11 );
  return ( i * 543210973 );
}
inline U32 hash0( U32 i ) {
  i *= 543210973;
  i = RORi( i, 8 );
  return ( i * 0xfdf7bfff );
}
inline U32 hash1a( U32 i ) {
  i = RORi( i, 7 );
  return ( i );
}
inline U32 hash3a( U32 i ) {
  i *= 234567891;
  i = RORi( i, 7 );
  return ( i );
}

template <int B>
inline U8 *HashTable<B>::get1( U32 o, U32 i ) {
  U8 *p = t + ( i & NB ) * B, *q, f;
  i >>= 27;
  i |= o;
  f = *( p - 1 );
  if( f == U8( i ) )
    return p;
  q = ( U8 * ) ( ( long ) p ^ B );
  f = *( q - 1 );
  if( f == U8( i ) )
    return q;
  if( *p > *q )
    p = q;
  *( U32 * ) ( p - 1 ) = i; // This is NOT big-endian-compatible
  return p;
}

template <int B>
inline U8 *HashTable<B>::get3a( U32 o, U32 i ) {
  U8 *p = t + ( i & NB ) * B, *q, *r, f;
  i >>= 27;
  i |= o;
  f = *( p - 1 );
  if( f == U8( i ) )
    return p;
  r = ( U8 * ) ( ( long ) p ^ B * 3 );
  f = *( r - 1 );
  if( f == U8( i ) )
    return r;
  q = ( U8 * ) ( ( long ) p ^ B * 2 );
  f = *( q - 1 );
  if( f == U8( i ) )
    return q;
  {
    U8 *s;
    s = ( U8 * ) ( ( long ) p ^ B );
    f = *( s - 1 );
    if( f == U8( i ) )
      return s;
  }
  {
    U8 g;
    f = *r;
    g = *p;
    if( g > f )
      p = r;
    f = *q;
    g = *p;
    if( g > f )
      p = q;
    q = ( U8 * ) ( ( long ) q ^ B * 3 );
    f = *q;
    g = *p;
    if( g > f )
      p = q;
  }
  *( U32 * ) ( p - 1 ) = i; // This is NOT big-endian-compatible
  return p;
}

template <int B>
inline U8 *HashTable<B>::get3b( U32 o, U32 i ) {
  U8 *p = t + ( i & NB ) * B, *q, *r, f;
  i >>= 27;
  i |= o;
  f = *( p - 1 );
  if( f == U8( i ) )
    return p;
  r = ( U8 * ) ( ( long ) p ^ B * 2 );
  f = *( r - 1 );
  if( f == U8( i ) )
    return r;
  q = ( U8 * ) ( ( long ) p ^ B * 3 );
  f = *( q - 1 );
  if( f == U8( i ) )
    return q;
  {
    U8 *s;
    s = ( U8 * ) ( ( long ) p ^ B );
    f = *( s - 1 );
    if( f == U8( i ) )
      return s;
  }
  {
    U8 g;
    f = *r;
    g = *p;
    if( g > f )
      p = r;
    f = *q;
    g = *p;
    if( g > f )
      p = q;
    q = ( U8 * ) ( ( long ) q ^ B * 2 );
    f = *q;
    g = *p;
    if( g > f )
      p = q;
  }
  *( U32 * ) ( p - 1 ) = i; // This is NOT big-endian-compatible
  return p;
}

//////////////////////////// MatchModel ////////////////////////

// MatchModel(n) predicts next bit using most recent context match.
//     using n bytes of memory.  n must be a power of 2 at least 8.
// MatchModel::p(y, m) updates the model with bit y (0..1) and writes
//     a prediction of the next bit to Mixer m.  It returns the length of
//     context matched (0..62).

U32 h1 = 0;        // context hashes
U32 buf_match = 0; // buf[match]+256
int N, HN;         // last hash table index, n/8-1
int *ht;           // context hash -> next byte in buf
int match;         // pointer to current byte in matched context in buf
int len;           // length of match

enum { MAXLEN = 62 }; // maximum match length, at most 62
U32 len2cxt[MAXLEN * 4 + 4], len2order[MAXLEN + 1], len2order7[MAXLEN + 1];

U8 len2cxt0[] = {0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
                 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 22, 22, 23,
                 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 27};

#define SEARCH                                                                                                         \
  {                                                                                                                    \
    l = 1;                                                                                                             \
    if( m != pos ) {                                                                                                   \
      while( buf[m - l & N] == buf[pos - l & N] && l < MAXLEN + 1 )                                                    \
        ++l;                                                                                                           \
    }                                                                                                                  \
    --l;                                                                                                               \
  }
#define SEARCH2                                                                                                        \
  {                                                                                                                    \
    l = 0;                                                                                                             \
    U8 *p = buf + pos - 1;                                                                                             \
    if( m >= MAXLEN ) {                                                                                                \
      int d = m - pos;                                                                                                 \
      do {                                                                                                             \
        if( *( p + d ) != *p )                                                                                         \
          break;                                                                                                       \
        ++l;                                                                                                           \
        if( *( p + d - 1 ) != *( p - 1 ) )                                                                             \
          break;                                                                                                       \
        ++l;                                                                                                           \
        p -= 2;                                                                                                        \
      } while( l < MAXLEN );                                                                                           \
    } else if( m ) {                                                                                                   \
      int i = m - 1;                                                                                                   \
      while( buf[i - l & N] == *p && l < MAXLEN )                                                                      \
        --p, ++l;                                                                                                      \
    }                                                                                                                  \
  }

inline void mm_upd() {
  int l = len, m;

  // find or extend match
  if( l > MINLEN ) {
    m = match;
    ++m;
    m &= N;
    if( l < MAXLEN )
      ++l;
  } else {
    m = ht[h1];
    if( pos >= MAXLEN )
      SEARCH2
    else
      SEARCH
    if( l < 7 ) {
      l = 0;
      if( ( pos & mmask ) == 0 )
        goto no_upd;
    }
  }
  ht[h1] = pos;
  match = m;
  buf_match = buf[m];
no_upd:
  len = l;
}

inline int mm_p() {
  int cxt = c0, l = len;
  if( l > 0 ) {
    int b = buf_match >> bcount;
    if( ( b >> 1 ) == cxt ) {
      b = ( b & 1 );
      mxr_tx[7] = len2cxt[126 + b + l * 2];
      cxt = len2cxt[b + l * 2] + c1;
      l = len2order[l];
      goto mm_e;
    }
    l = 0, len = l;
  }
  if( *cp[2] != 0U ) {
    l = MI * 8;
    if( *cp[3] != 0U ) {
      l = 2 * MI * 8;
      if( *cp[4] != 0U )
        l = 3 * MI * 8;
    }
  }
mm_e:
  smpm( cxt );
  return l;
}

inline int mm_p7() {
  int cxt, l = len;
  if( l > 0 ) {
    int b = buf_match;
    buf_match = b + 256;
    b = ( b >> 7 );
    mxr_tx[7] = len2cxt[124 + b + l * 2];
    cxt = len2cxt[b + l * 2] + c1;
    l = len2order7[l];
    goto mm_e;
  }
  cxt = c0;
  if( *cp[2] != 0U ) {
    l = MI * 8;
    if( *cp[3] != 0U ) {
      l = 2 * MI * 8;
      if( *cp[4] != 0U )
        l = 3 * MI * 8;
    }
  }
mm_e:
  sm7m( cxt );
  return l;
}

//////////////////////////// Predictor /////////////////////////

// A Predictor estimates the probability that the next bit of
// uncompressed data is 1.  Methods:
// Predictor(n) creates with 3*n bytes of memory.
// p() returns P(1) as a 12 bit number (0-4095).
// update(y) trains the predictor with the actual bit (0 or 1).

//////////////////////////// Encoder ////////////////////////////

// An Encoder does arithmetic encoding.  Methods:
// Encoder(COMPRESS, f) creates encoder for compression to archive f, which
//     must be open past any header for writing in binary mode.
// Encoder(DECOMPRESS, f) creates encoder for decompression from archive f,
//     which must be open past any header for reading in binary mode.
// code(i) in COMPRESS mode compresses bit i (0 or 1) to file f.
// code() in DECOMPRESS mode returns the next decompressed bit from file f.
// compress(c) in COMPRESS mode compresses one byte.
// decompress() in DECOMPRESS mode decompresses and returns one byte.
// flush() should be called exactly once after compression is done and
//     before closing f.  It does nothing in DECOMPRESS mode.

HashTable<4> t4a, t4b; // cxt -> state

FILE *archive;               // Compressed data file
U32 x1 = 0, x2 = 0xffffffff; // Range, initially [0, 1), scaled by 2^32
U32 saved_x = 0;             // Decompress mode: last 4 input bytes of archive
U32 pre = 2048;
int *add2order;

typedef enum { COMPRESS, DECOMPRESS } Mode;
class Encoder {
public:
#define cp_update( m0, m1, m2, m3, m4, m5 )                                                                            \
  assert( y == 0 || y == 1 );                                                                                          \
  y20 = y << 20;                                                                                                       \
  {                                                                                                                    \
    U8 *p = ( U8 * ) &State_table[0], *q = p;                                                                          \
    int j;                                                                                                             \
    {                                                                                                                  \
      int c = c0;                                                                                                      \
      c0 = c * 2 + y;                                                                                                  \
      if( y )                                                                                                          \
        p += 256 * 6;                                                                                                  \
      j = c & 1;                                                                                                       \
      if( j )                                                                                                          \
        q += 256 * 6;                                                                                                  \
      {                                                                                                                \
        U8 *a = t0c1;                                                                                                  \
        *( a + c ) = nex( *( a + c ), m0 );                                                                            \
        c = c >> 1;                                                                                                    \
        *( a + c ) = neq( *( a + c ), m0 );                                                                            \
      }                                                                                                                \
    }                                                                                                                  \
    j = j ^ 0xffffffff;                                                                                                \
    U8 *a = cp[1];                                                                                                     \
    *a = nex( *a, m1 );                                                                                                \
    *( a + j ) = neq( *( a + j ), m1 );                                                                                \
    a = cp[2];                                                                                                         \
    *a = nex( *a, m2 );                                                                                                \
    *( a + j ) = neq( *( a + j ), m2 );                                                                                \
    a = cp[3];                                                                                                         \
    *a = nex( *a, m3 );                                                                                                \
    *( a + j ) = neq( *( a + j ), m3 );                                                                                \
    a = cp[4];                                                                                                         \
    *a = nex( *a, m4 );                                                                                                \
    *( a + j ) = neq( *( a + j ), m4 );                                                                                \
    a = cp[5];                                                                                                         \
    *a = nex( *a, m5 );                                                                                                \
    *( a + j ) = neq( *( a + j ), m5 );                                                                                \
  }

#define upd0( bc )                                                                                                     \
  assert( y == 0 || y == 1 );                                                                                          \
  y20 = y << 20;                                                                                                       \
  c0 += c0 + y;                                                                                                        \
  bcount = bc;                                                                                                         \
  add2order += MI;                                                                                                     \
  y = y + 1;                                                                                                           \
  cp[1] += y;                                                                                                          \
  cp[2] += y;                                                                                                          \
  cp[3] += y;                                                                                                          \
  cp[4] += y;                                                                                                          \
  cp[5] += y;

#define upd1( m0, m1, m2, m3, m4, m5, bc )                                                                             \
  cp_update( m0, m1, m2, m3, m4, m5 ) bcount = bc;                                                                     \
  add2order += MI;                                                                                                     \
  upd1_cp

#define upd3( m0, m1, m2, m3, m4, m5, bc )                                                                             \
  cp_update( m0, m1, m2, m3, m4, m5 ) bcount = bc;                                                                     \
  add2order += MI;                                                                                                     \
  upd3_cp

#define upd5( m0, m1, m2, m3, m4, m5, bc )                                                                             \
  cp_update( m0, m1, m2, m3, m4, m5 ) bcount = bc;                                                                     \
  add2order += MI;                                                                                                     \
  upd5_cp

#define upd7                                                                                                           \
  {                                                                                                                    \
    int c = c0 - 256, d = WRT_mxr[3 + c * 4];                                                                          \
    add2order = mxr_wx + MI * 10 * 4 * d;                                                                              \
    d = WRT_mtt[c];                                                                                                    \
    if( ( pw & 255 ) == 0 )                                                                                            \
      c1 = 33 + ( d >> 3 );                                                                                            \
    else {                                                                                                             \
      c1 = ( pw & 31 ) + d;                                                                                            \
      if( d < 33 )                                                                                                     \
        c1 = WRT_chc1[c1];                                                                                             \
    }                                                                                                                  \
    c2 = c1 * 256;                                                                                                     \
    buf[pos] = c;                                                                                                      \
    pos = ( pos + 1 ) & N;                                                                                             \
    cc = c8 >> 24;                                                                                                     \
    c8 = ( c8 << 8 ) + ( c4 >> 24 );                                                                                   \
    c4 = c4 << 8 | c;                                                                                                  \
    upd7_h123 c0 = 1;                                                                                                  \
    d = WRT_wrd[c];                                                                                                    \
    {                                                                                                                  \
      int e = pw * 2;                                                                                                  \
      if( bc4cp0 = ( d << 2 ) ) {                                                                                      \
        ++e, h5 = h5 * 724 + c;                                                                                        \
        d -= 128;                                                                                                      \
        if( d )                                                                                                        \
          d = hash5( h5 );                                                                                             \
      } else                                                                                                           \
        h5 = d;                                                                                                        \
      pw = e;                                                                                                          \
    }                                                                                                                  \
    upd7_cp mm_upd();                                                                                                  \
    if( c == 32 )                                                                                                      \
      p = Predict_was32s();                                                                                            \
    else                                                                                                               \
      p = Predict_not32s();                                                                                            \
  }

  U32 Predict_was32() {
    int y2o = y20;
    smzr( 5, cxt5, 5 * 256, 10, limits_15b[cxt5], 0 ) y2o += 768;
    smzr( 1, cxt1, 4 * 256, 14, 1, 0 )

        smzr( 2, cxt2, 7 * 256, 10, 1, 0 ) smzr( 3, cxt3, 9 * 256, 11, 1, 0 ) smzr( 4, cxt4, 11 * 256, 10, 1, 0 )

            int len = mm_p(),
                pr;
    mxr_cxt = add2order + len;
    smp0w( *( t0c1 + c0 ) + 256 );
    pr = m_p + 2047;
    int xx = a1.p1( pr, c2 + c0 );
    mxr_pr = xx, pr = squash( pr - 2047 ) + 7 * xx >> 3;
    pr = ( pr * 3 + 5 * a2.p2( stretch_t2[xx], fails * 8 + bcount ) + 4 ) >> 3;
    return pr + static_cast<int>( pr < 2048 );
  }

  U32 Predict_not32() {
    int y2o = y20;
    smzr( 5, cxt5, 5 * 256, 9, limits_15a[cxt5], 0 ) y2o += 384;
    smzr( 1, cxt1, 4 * 256, 9, 1, 0 )

        smzr( 2, cxt2, 6 * 256, 9, 1, 0 ) smzr( 3, cxt3, 8 * 256, 10, 1, 0 ) smzr( 4, cxt4, 10 * 256, 10, 1, 0 )

            int len = mm_p(),
                pr;
    mxr_cxt = add2order + len;
    smp0n( *( t0c1 + c0 ) + bc4cp0 );
    pr = m_p + 2047;
    int xx = a1.p1( pr, c2 + c0 );
    mxr_pr = squash( pr - 2047 ) + 15 * xx >> 4;
    pr = ( mxr_pr + 7 * a2.p2( stretch_t2[xx], fails * 8 + bcount ) + 4 ) >> 3;
    return pr + static_cast<int>( pr < 2048 );
  }

  U32 Predict_was32s() {
    int y2o = y20;
    smzr( 5, cxt5, 5 * 256, 14, limits_15b[cxt5], 0 ) y2o += 6144;
    smzr( 1, cxt1, 4 * 256, 14, 1, 0 ) if( ( c4 & 0xff00 ) == 0x2000 ) {
      smzr( 2, cxt2, 7 * 256, 13, 1, 0 ) smzr( 3, cxt3, 9 * 256, 14, 1, 0 ) smzr( 4, cxt4, 11 * 256, 13, 1, 0 )
    }
    else {
      smzr( 2, cxt2, 6 * 256, 13, 1, 256 ) smzr( 3, cxt3, 8 * 256, 14, 1, 256 ) smzr( 4, cxt4, 10 * 256, 13, 1, 256 )
    }
    int len = mm_p7(), pr;
    mxr_cxt = add2order + len;
    smp7w( *( t0c1 + 1 ) + 256 );
    pr = m_p + 2047;
    int xx = a1.p1( pr, c2 );
    mxr_pr = squash( pr - 2047 ) + 7 * xx >> 3;
    pr = a2.p2( stretch_t2[xx], fails * 8 + 7 );
    pr = pr * 2 + pr;
    pr = ( mxr_pr + pr ) >> 2;
    return pr + static_cast<int>( pr < 2048 );
  }

  U32 Predict_not32s() {
    int y2o = y20;
    smzr( 5, cxt5, 5 * 256, 9, limits_15a[cxt5], 0 ) smzr( 1, cxt1, 4 * 256, 9, 1, 0 ) if( ( c4 & 0xff00 ) == 0x2000 ) {
      y2o += 768;
      smzr( 2, cxt2, 7 * 256, 10, 1, -256 ) smzr( 3, cxt3, 9 * 256, 11, 1, -256 ) smzr( 4, cxt4, 11 * 256, 11, 1, -256 )
    }
    else {
      y2o += 384;
      smzr( 2, cxt2, 6 * 256, 9, 1, 0 ) smzr( 3, cxt3, 8 * 256, 10, 1, 0 ) smzr( 4, cxt4, 10 * 256, 9, 1, 0 )
    }
    int len = mm_p7(), pr;
    mxr_cxt = add2order + len;
    smp7n( *( t0c1 + 1 ) + bc4cp0 );
    pr = m_p + 2047;
    int xx = a1.p1( pr, c2 );
    mxr_pr = squash( pr - 2047 ) + 7 * xx >> 3;
    pr = a2.p2( stretch_t2[xx], fails * 8 + 7 );
    pr = pr * 2 + pr;
    pr = ( mxr_pr + pr ) >> 2;
    return pr + static_cast<int>( pr < 2048 );
  }

  void pass_bytes_enc( void ) {
    do { /* pass equal leading bytes of range */
      putc( x2 >> 24, archive );
      x1 <<= 8;
      x2 = ( x2 << 8 ) + 255;
    } while( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 );
  }

  U32 pass_bytes_dec( U32 x ) {
    do {                                          /* pass equal leading bytes of range */
      x = ( x << 8 ) + ( getc( archive ) & 255 ); /* EOF is OK */
      x1 <<= 8;
      x2 = ( x2 << 8 ) + 255;
    } while( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 );
    return x;
  }

  // Compress bit y
#define enc1( k, mu, MS0, MS1, MS2, MS3, MS4, MS5, MS6, MS7 )                                                          \
  int y = ( c >> k ) & 1;                                                                                              \
  U32 xmid = x1 + ( x2 - x1 >> 12 ) * p + ( ( x2 - x1 & 0xfff ) * p >> 12 );                                           \
  assert( xmid >= x1 && xmid < x2 );                                                                                   \
  y ? ( x2 = xmid ) : ( x1 = xmid + 1 );                                                                               \
  m_update( y, ( ( k - 1 ) & 7 ), mu, MS0, MS1, MS2, MS3, MS4, MS5, MS6, MS7 );

#define enc2                                                                                                           \
  if( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 )                                                                              \
    pass_bytes_enc();

  // Return decompressed bit
#define dec1( k, mu, MS0, MS1, MS2, MS3, MS4, MS5, MS6, MS7 )                                                          \
  U32 xmid = x1 + ( x2 - x1 >> 12 ) * p + ( ( x2 - x1 & 0xfff ) * p >> 12 );                                           \
  assert( xmid >= x1 && xmid < x2 );                                                                                   \
  int y;                                                                                                               \
  if( x <= xmid )                                                                                                      \
    x2 = xmid, y = 1;                                                                                                  \
  else                                                                                                                 \
    x1 = xmid + 1, y = 0;                                                                                              \
  m_update( y, ( ( k - 1 ) & 7 ), mu, MS0, MS1, MS2, MS3, MS4, MS5, MS6, MS7 );

#define dec2                                                                                                           \
  if( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 )                                                                              \
    x = pass_bytes_dec( x );

  Encoder( Mode m, FILE *f );
  void flush(); // call this when compression is finished

#define eight_bits( part1, part2 )                                                                                     \
  if( ( c4 & 255 ) == 32 ) {                                                                                           \
    {                                                                                                                  \
      part1( 7, ( MXR_UPD_7 ), 14, 14, 15, 14, 14, 15, 13, 14 );                                                       \
      upd0( 6 );                                                                                                       \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_was32();                                                                                               \
    {                                                                                                                  \
      part1( 6, ( MXR_UPD_6 ), 14, 14, 15, 14, 14, 15, 14, 14 );                                                       \
      upd1( 2, 1, 4, 3, 4, 5, 5 );                                                                                     \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_was32();                                                                                               \
    {                                                                                                                  \
      part1( 5, ( MXR_UPD_5 ), 14, 14, 15, 14, 14, 15, 14, 14 );                                                       \
      upd0( 4 );                                                                                                       \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_was32();                                                                                               \
    {                                                                                                                  \
      part1( 4, ( MXR_UPD_4 ), 14, 14, 15, 14, 14, 15, 14, 14 );                                                       \
      upd3( 2, 1, 4, 3, 4, 5, 3 );                                                                                     \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_was32();                                                                                               \
    {                                                                                                                  \
      part1( 3, ( MXR_UPD_3 ), 14, 14, 15, 14, 14, 15, 14, 14 );                                                       \
      upd0( 2 );                                                                                                       \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_was32();                                                                                               \
    {                                                                                                                  \
      part1( 2, ( MXR_UPD_2 ), 14, 14, 15, 14, 14, 15, 14, 14 );                                                       \
      upd5( 2, 1, 4, 3, 4, 5, 1 );                                                                                     \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_was32();                                                                                               \
    {                                                                                                                  \
      part1( 1, ( MXR_UPD_1 ), 14, 14, 16, 14, 14, 15, 14, 14 );                                                       \
      upd0( 0 );                                                                                                       \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_was32();                                                                                               \
    {                                                                                                                  \
      part1( 0, ( MXR_UPD_0 ), 14, 15, 16, 15, 14, 16, 15, 14 );                                                       \
      cp_update( 2, 1, 4, 3, 4, 5 );                                                                                   \
    }                                                                                                                  \
  } else {                                                                                                             \
    {                                                                                                                  \
      part1( 7, ( MXR_UPD_7 ), 14, 14, 13, 13, 12, 13, 13, 12 );                                                       \
      upd0( 6 );                                                                                                       \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_not32();                                                                                               \
    {                                                                                                                  \
      part1( 6, ( MXR_UPD_6 ), 14, 14, 13, 13, 13, 13, 13, 12 );                                                       \
      upd1( 2, 1, 0, 3, 4, 5, 5 );                                                                                     \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_not32();                                                                                               \
    {                                                                                                                  \
      part1( 5, ( MXR_UPD_5 ), 14, 14, 13, 13, 13, 13, 13, 12 );                                                       \
      upd0( 4 );                                                                                                       \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_not32();                                                                                               \
    {                                                                                                                  \
      part1( 4, ( MXR_UPD_4 ), 14, 14, 14, 13, 13, 13, 13, 12 );                                                       \
      upd3( 2, 1, 0, 3, 4, 5, 3 );                                                                                     \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_not32();                                                                                               \
    {                                                                                                                  \
      part1( 3, ( MXR_UPD_3 ), 14, 14, 14, 13, 13, 13, 13, 13 );                                                       \
      upd0( 2 );                                                                                                       \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_not32();                                                                                               \
    {                                                                                                                  \
      part1( 2, ( MXR_UPD_2 ), 14, 14, 14, 13, 13, 13, 13, 13 );                                                       \
      upd5( 2, 1, 0, 3, 4, 5, 1 );                                                                                     \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_not32();                                                                                               \
    {                                                                                                                  \
      part1( 1, ( MXR_UPD_1 ), 14, 14, 14, 13, 13, 13, 13, 13 );                                                       \
      upd0( 0 );                                                                                                       \
    }                                                                                                                  \
    part2;                                                                                                             \
    p = Predict_not32();                                                                                               \
    {                                                                                                                  \
      part1( 0, ( MXR_UPD_0 ), 14, 14, 14, 13, 13, 13, 13, 13 );                                                       \
      cp_update( 2, 1, 0, 3, 4, 5 );                                                                                   \
    }                                                                                                                  \
  }                                                                                                                    \
  part2;                                                                                                               \
  upd7;

  // Compress one byte
  inline void compress( int c ) {
    ///assert(mode==COMPRESS);
    U32 p = pre;
    eight_bits( enc1, enc2 ) pre = p;
  }

  // Decompress and return one byte
  inline int decompress() {
    U32 p = pre, x = saved_x;
    eight_bits( dec1, dec2 ) pre = p, saved_x = x;
    int c = c4 & 255;
    return c;
  }
};

Encoder::Encoder( Mode m, FILE *f ) {
  t4a.ht_init();
  t4b.ht_init();
  archive = f;
  if( m == DECOMPRESS ) { // x = first 4 bytes of archive
    for( int i = 0; i < 4; ++i )
      saved_x = ( saved_x << 8 ) + ( getc( archive ) & 255 );
  }

  int i, c, pi = 0;
  for( int x = -2047; x <= 2047; ++x ) { // invert squash()
    int i = squash_init( x );
    squash( x ) = i + SQUARD; //rounding,  needed at the end of Predictor::update()
    for( int j = pi; j <= i; ++j )
      stretch_t[j] = x, stretch_t2[j] = ( x + 2047 ) * ( APMw - 1 );
    pi = i + 1;
  }
  stretch_t2[4095] = 4094 * ( APMw - 1 );
  for( i = 5; i != 0; --i )
    stretch_t[i] = -1696, stretch_t[4090 + i] = 1696;
  stretch_t[0] = -1696;

  pi = 41 * 256, c = 42 * 256;
  if( MEM > ( 1 << 22 ) )
    pi = 48 * 128, c = 32 * 128;
  for( i = 1023; i >= 0; --i ) {
    dt[i] = pi / ( i + 4 ), dta[i] = c / ( i + 4 );
  }

  for( i = -4096; i < 4096; ++i ) {
    int e = i, v;
    if( e < 0 )
      e = -e;
    v = 0;
    if( e > ( 1440 + 160 ) )
      v = 1;
    if( e > 2880 + 224 )
      v = 3;
    calcfails[i + 4096 + 8192 * 0] = v;
    v = 0;
    if( e > ( 1440 - 192 ) )
      v = 1;
    if( e > 2592 - 32 )
      v = 3;
    calcfails[i + 4096 + 8192 * 1] = v;
    v = 0;
    if( e > ( 1440 + 64 ) )
      v = 1;
    calcfails[i + 4096 + 8192 * 2] = v;
    v = 0;
    if( e > ( 96 ) )
      v = 1;
    if( e > 2048 - 416 )
      v = 3;
    calcfails[i + 4096 + 8192 * 3] = v;
    v = 0;
    if( e > ( 1440 + 224 ) )
      v = 1;
    if( e > 4096 - 768 )
      v = 3;
    calcfails[i + 4096 + 8192 * 4] = v;
    v = 0;
    if( e > ( 256 + 704 ) )
      v = 1;
    if( e > 2592 + 544 )
      v = 3;
    calcfails[i + 4096 + 8192 * 5] = v;
    v = 1;
    if( e > 352 )
      v = 3;
    calcfails[i + 4096 + 8192 * 6] = v;
    v = 0;
    if( e > ( 1440 ) )
      v = 1;
    if( e > 2592 - 64 )
      v = 3;
    calcfails[i + 4096 + 8192 * 7] = v;
  }

  for( i = 1; i <= MAXLEN; i++ ) {
    int j, k, c = len2cxt0[i];
    k = 0x80000 + ( 0x7ffff * c / 27 );
    c *= 512;
    len2cxt[i * 2] = c;
    len2cxt[i * 2 + 1] = c - 256;
    j = i;
    if( j > 32 )
      j = 16 + j / 2;
    len2cxt[i * 2 + 126] = -j * 20;
    len2cxt[i * 2 + 127] = j * 20;

    if( c != 0 )
      for( j = 255; j >= 0; --j )
        smt[256 * 11 + c + j] = k, smt[256 * 12 + c + j] = 0xfffff ^ k;
    k = ( 4 + static_cast<int>( i >= 11 ) + static_cast<int>( i >= 13 ) + static_cast<int>( i >= 16 )
          + static_cast<int>( i > 51 ) )
        * MI * 8;
    len2order[i] = k + static_cast<int>( i >= 23 ) * MI * 8;
    len2order7[i] = k + static_cast<int>( i >= 29 ) * MI * 8;
  }

  i = MEM / 2;
  N = i - 1;
  HN = i / 4 - 1;
  len = 0;
  alloc( buf, i );
  alloc( ht, i / 4 );

  alloc( mxr_wx, MI * MC );
  pi = 0;
  for( i = 0; i < MI * MC; ++i ) {
    c = 0xa40;
    if( ( i & 7 ) == 7 )
      c = 0xa00;
    if( ( i & 7 ) == 0 ) {
      if( pi >= 8 * 8 )
        c = 0xd00;
      ++pi;
      if( pi == 80 )
        pi = 0;
    }
    mxr_wx[i] = c;
  }
  mxr_cxt = mxr_wx;
  add2order = mxr_wx;

  for( i = 256 * 13 - 1; i >= 0; --i )
    smt[i] = 0x7ffff;
  {
    int i, z, o, p1, p2, p3, p4, p5, p6;
    U8 *p = ( U8 * ) &State_table[0], *q = p + 256 * 6;
    for( i = 0; i < 6; ++i ) {
      U32 *j = &smt[( ( 0x578046 >> ( i * 4 ) ) & 15 ) * 256];
      p1 = p2 = nex( 0, i );
      p3 = p4 = neq( 0, i );
      p1 = nex( p1, i );
      *( j + p1 ) = 0xfffff * ( 1 ) / 4;
      p2 = neq( p2, i );
      *( j + p2 ) = 0xfffff * ( 2 ) / 4;
      p3 = nex( p3, i );
      *( j + p3 ) = 0xfffff * ( 2 ) / 4;
      p4 = neq( p4, i );
      *( j + p4 ) = 0xfffff * ( 3 ) / 4;
      p5 = p1;
      p6 = p4;
      for( z = 5; z < 70; ++z ) {
        o = p1, p1 = nex( p1, i );
        if( p1 != o )
          *( j + p1 ) = 0xfffff * ( 1 ) / z;
        o = p2, p2 = neq( p2, i );
        if( p2 != o )
          *( j + p2 ) = 0xfffff * ( z - 2 ) / z;
        o = p3, p3 = nex( p3, i );
        if( p3 != o )
          *( j + p3 ) = 0xfffff * ( 2 ) / z;
        o = p4, p4 = neq( p4, i );
        if( p4 != o )
          *( j + p4 ) = 0xfffff * ( z - 1 ) / z;

        o = p5, p5 = neq( p5, i );
        if( p5 < o )
          p5 = o;
        if( p5 != o )
          *( j + p5 ) = 0xfffff * ( z - 3 ) / z; // 00 111
        o = p6, p6 = nex( p6, i );
        if( p6 < o )
          p6 = o;
        if( p6 != o )
          *( j + p6 ) = 0xfffff * ( 3 ) / z; // 11 000
      }
    }
    for( z = 255; z >= 0; --z )
      o = smt[z], smt[256 + z] = o, smt[512 + z] = o, smt[768 + z] = o, o = smt[2048 + z], smt[2304 + z] = o,
      o = smt[1792 + z], smt[2560 + z] = o, smt[2816 + z] = o;
  }
}

void Encoder::flush() {
  ///if (mode==COMPRESS)
  putc( x1 >> 24, archive ); // Flush first unequal byte of range
}

//////////////////////////// User Interface ////////////////////////////

int main( int argc, char **argv ) {
  // Check arguments
  if( argc != 4 || ( ( isdigit( argv[1][0] ) == 0 ) && argv[1][0] != 'd' ) ) {
    printf( "lpaq9m file compressor (C) 2007-2009, Matt Mahoney, Alexander Ratushnyak\n"
            "Licensed under GPL, http://www.gnu.org/copyleft/gpl.html\n"
            "\n"
            "To   compress: lpaq9m N input output  (N=0..9, uses 6+3*2^N MB)\n"
            "To decompress: lpaq9m d input output  (needs same memory)\n" );
    return 1;
  }

  // Get start time
  clock_t start = clock();

  // Open input file
  FILE *in = fopen( argv[2], "rbe" ), *out = 0;
  if( in == nullptr )
    perror( argv[2] ), exit( 1 );

  // Compress
  if( isdigit( argv[1][0] ) != 0 ) {
#ifndef WITHOUT_COMPRESSOR
    MEM = 1 << ( argv[1][0] - '0' + 20 );

    fseek( in, 0, SEEK_END );
    long size = ftell( in );
    if( size < 0 || size >= 0x7FFFFFFF )
      quit( "input file too big" );
    fseek( in, 0, SEEK_SET );

    uncompressed = in;
    { // a better data detection algorithm will be here in future
      int i = fread( file_buf, 1, FB_SIZE + 4, in );
      fb_len = &file_buf[i];
      fb_stop = fb_len;
      if( fb_stop > &file_buf[FB_SIZE] )
        fb_stop = &file_buf[FB_SIZE];
    }

    // Encode header: version 17, memory option, file size
    out = fopen( argv[3], "wbe" );
    if( out == nullptr )
      perror( argv[3] ), exit( 1 );
    fprintf( out, "pQ%c%c%ld%ld%ld%ld%c", 17, argv[1][0], size >> 24, size >> 16, size >> 8, size, method );

    // Compress
    Encoder e( COMPRESS, out );
    int c;
    while( ( c = do_getc() ) != EOF ) {
      e.compress( c );
      if( ( pos & ( 256 * 1024 - 1 ) ) == 0 ) {
        if( ( pos == 25 * 256 * 1024 && DP_SHIFT == 16 ) || ( pos == 1024 * 1024 && DP_SHIFT == 15 ) || DP_SHIFT == 14 )
          for( DP_SHIFT++, c = 0; c < MI * MC; ++c )
            mxr_wx[c] *= 2;
        if( pos == 24 * 1024 * 1024 )
          mmask = 3;
        if( pos == 48 * 1024 * 1024 )
          mmask = 1;
      }
    }
    e.flush();
#endif
  }

  // Decompress
  else {
#ifndef WITHOUT_DECOMPRESSOR
    // Check header version, get memory option, file size
    if( getc( in ) != 'p' || getc( in ) != 'Q' || getc( in ) != 17 )
      quit( "Not a lpaq9m file" );
    MEM = getc( in );
    if( MEM < '0' || MEM > '9' )
      quit( "Bad memory option (not 0..9)" );
    MEM = 1 << ( MEM - '0' + 20 );
    long size = getc( in ) << 24;
    size |= getc( in ) << 16;
    size |= getc( in ) << 8;
    size |= getc( in );
    if( size < 0 )
      quit( "Bad file size" );
    method = getc( in );

    // Decompress
    out = fopen( argv[3], "wbe" );
    if( out == nullptr )
      perror( argv[3] ), exit( 1 );
    Encoder e( DECOMPRESS, in );

    uncompressed = out;
    {
      U8 *p = &file_buf[0];
      long s = FB_SIZE + 4;
      if( s > size )
        s = size;
      size -= s;
      while( s-- > 0 )
        *p++ = e.decompress();
      fb_pos = p;
    }

    while( size-- > 0 ) {
      int c;
      do_putc( e.decompress() );
      if( ( pos & ( 256 * 1024 - 1 ) ) == 0 ) {
        if( ( pos == 25 * 256 * 1024 && DP_SHIFT == 16 ) || ( pos == 1024 * 1024 && DP_SHIFT == 15 ) || DP_SHIFT == 14 )
          for( DP_SHIFT++, c = 0; c < MI * MC; ++c )
            mxr_wx[c] *= 2;
        if( pos == 24 * 1024 * 1024 )
          mmask = 3;
        if( pos == 48 * 1024 * 1024 )
          mmask = 1;
      }
    }
    do_putc_end();
#endif
  }

  // Report result
  assert( in );
  assert( out );
  printf( "%ld -> %ld in %1.3f sec. using %d MB memory\n", ftell( in ), ftell( out ),
          double( clock() - start ) / CLOCKS_PER_SEC, mem_usage >> 20 );

  return 0;
}
