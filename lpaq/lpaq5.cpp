/*	lpaq5.cpp file compressor, September 29, 2007.
(C) 2007, Matt Mahoney, matmahoney@yahoo.com
	  Alexander Ratushnyak, artest@inbox.ru

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

To compress:   N input output  (N=0..9, uses 3 + 3*2^N MB memory)
To decompress: d input output  (requires same memory)

For example:

  lpaq5 9 foo foo.lpq

compresses foo to foo.lpq using 1.5 GB memory.

  lpaq5 d foo.lpq foo

decompresses to foo, also using 1.5 GB memory.  Option 0 uses
6 MB memory.  The maximum file size is 2 GB.

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
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

int DP_SHIFT = 14;

#ifdef WIKI
#  define TOLIMIT_1 1023
#  define TOLIMIT_2 1023
#  define SQUARD 4
#  define add2order0 ( c >> 4 )
#  define upd3_cp                                                                                                      \
    cp[2] = t1a.get( hash7( c0 * 23 - ( c4 & 0xc0ffffff ) * 241 ) );             /*  3.25 */                           \
    cp[3] = t2a.get( hash8( c0 - c4 * 19 + ( c8 & 0xc0ff ) * 19991 ) );          /*  5.25 */                           \
    cp[4] = t3b.get( hash9( c0 * 31 - c4 * 59 - ( c8 & 0xfcffffff ) * 59999 ) ); /*  7.75 */
#  define upd7_cp                                                                                                      \
    cp[2] = t1b.get( hash2( ( c4 & 0xffffff ) * 251 ) );              /* order 3   */                                  \
    cp[3] = t2b.get( hash3( c4 * 127 + ( ( c8 >> 4 ) & 15 ) ) );      /* order 4.5 */                                  \
    cp[4] = t3a.get( hash4( c4 * 197 - ( c8 & 0xfcffff ) * 63331 ) ); /* order 6.75*/
#  define upd7_h123                                                                                                    \
    h2 = hash1( c4 * 59 + c8 * 59999 ) & HN;                                                                           \
    h1 = hash2( c4 * 73 - c8 * 101 + ( cc & 0xffffff ) * 421 ) & HN;

#else
#  define TOLIMIT_1 159
#  define TOLIMIT_2 175
#  define SQUARD 2
#  define add2order0 ( ( c >> 6 ) + ( ( c4 >> 4 ) & 12 ) )
int calcprevfail[256];
#  define upd3_cp                                                                                                      \
    cp[2] = t1a.get( hash7( c0 * 23 - ( c4 & 0xffffff ) * 251 ) );                                                     \
    cp[3] = t2a.get( hash8( c0 - c4 * 19 ) );                                                                          \
    cp[4] = t3b.get( hash9( c0 * 31 - c4 * 197 + ( c8 & 0xffff ) * 63331 ) );
#  define upd7_cp                                                                                                      \
    cp[2] = t1b.get( hash2( ( c4 & 0xffffff ) * 251 ) );                                                               \
    cp[3] = t2b.get( hash3( c4 * 127 ) );                                                                              \
    cp[4] = t3a.get( hash4( c4 * 197 - ( c8 & 0xffff ) * 63331 ) );
#  define upd7_h123                                                                                                    \
    t0c1 = t0 + c1 * 256;                                                                                              \
    prevfail = calcprevfail[fails];                                                                                    \
    fails = 1;                                                                                                         \
    h1 = h1 * ( 3 << 1 ) + c + 1 & HN;                                                                                 \
    h2 = hash2( c4 * 73 - c8 * 101 + cc * 421 ) & HN;                                                                  \
    h3 = hash1( c4 * 59 + c8 * 59999 ) & HN;
#endif

// Error handler: print message if any, and exit
void quit( const char *message = 0 ) {
  if( message != nullptr )
    printf( "%s\n", message );
  exit( 1 );
}

U32 mem_usage = 0;

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
  if( d == 2047 )
    return 4095;
  if( d == -2047 )
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
static int dt[1024];        // i -> 16K/(i+3)
static int calcfails[8192]; //as above, initialized when Encoder is created

static int TextFlag = 0;
static int y22;        // y<<22
static int c0 = 1;     // last 0-7 bits with leading 1
static int c4 = 0;     // last 4 bytes
static int c1 = 0;     // last two higher 4-bit nibbles
static int bcount = 0; // bit count
static U8 *buf;        // input buffer
static int pos;        // number of bytes in buf

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

static const U8 State_table[512 * 3] = {
#include "State_tables.dat"
};
#define nex( state, sel ) *( p + state + sel * 256 )

//////////////////////////// StateMap, APM //////////////////////////

// A StateMap maps a context to a probability.  Methods:
//
// Statemap sm(n) creates a StateMap with n contexts using 4*n bytes memory.
// sm.p(y, cx, limit) converts state cx (0..n-1) to a probability (0..4095).
//	that the next y=1, updating the previous prediction with y (0..1).
//	limit (1..1023, default 1023) is the maximum count for computing a
//	prediction.  Larger values are better for stationary sources.

class StateMap {
public:
  U32 *t_cxt; // Context of last prediction
  U32 *t;     // cxt -> prediction in high 22 bits, count in low 10 bits
  StateMap( int n = 512 );

  // update bit y (0..1), predict next bit in context cx
  inline int p( int cx ) { //, int limit=1023)
    assert( y >> 1 == 0 );
    assert( cx >= 0 && cx < N );
    assert( cxt >= 0 && cxt < N );
    U32 p0 = *t_cxt;
    U32 i = p0 & 1023, pr = p0 >> 10; // count, prediction
    p0 += static_cast<unsigned int>( i < TOLIMIT_1 );
    p0 += ( ( ( y22 - ( int ) pr ) >> 3 ) * dt[i] ) & 0xfffffc00;
    *t_cxt = p0;
    t_cxt = t + cx;
    return ( *t_cxt ) >> 20;
  }
};

StateMap::StateMap( int n ) {
  alloc( t, n );
  t_cxt = t;
  for( int i = 0; i < n; ++i )
    t[i] = 1 << 31;
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

class APM {
protected:
  int cxt; // Context of last prediction
  U32 *t;  // cxt -> prediction in high 22 bits, count in low 10 bits
public:
  APM( int n );
  inline int pp( int pr, int cx ) { //, int limit=1023)
    assert( y >> 1 == 0 );
    assert( cx >= 0 && cx < N / 24 );
    assert( cxt >= 0 && cxt < N );
    {
      U32 *p = &t[cxt], p0 = p[0];
      U32 i = p0 & 1023, pr = p0 >> 10; // count, prediction
      p0 += static_cast<unsigned int>( i < TOLIMIT_2 );
      p0 += ( ( ( y22 - ( int ) pr ) >> 3 ) * dt[i] + 0x200 ) & 0xfffffc00;
      p[0] = p0;
    }
    int wt = pr & 0xfff; // interpolation weight of next element
    cx = cx * 24 + ( pr >> 12 );
    cxt = cx + ( wt >> 11 );
    pr = ( ( t[cx] >> 13 ) * ( 0x1000 - wt ) + ( t[cx + 1] >> 13 ) * wt ) >> 19;
    return pr;
  }
};

APM::APM( int n ) : cxt( 0 ) {
  alloc( t, n );
  for( int i = 0; i < n; ++i ) {
    int p = ( ( i % 24 * 2 + 1 ) * 4096 ) / 48 - 2048;
    t[i] = ( U32( squash_init( p ) ) << 20 ) + 8;
  }
}

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
inline void train( int err ) {
  int *w = mxr_cxt;
  assert( err >= -32768 && err < 32768 );
  w[0] += ( mxr_tx[0] * err + 0x2000 ) >> 14;
  w[1] += ( mxr_tx[1] * err + 0x2000 ) >> 14;
  w[2] += ( mxr_tx[2] * err + 0x2000 ) >> 14;
  w[3] += ( mxr_tx[3] * err + 0x2000 ) >> 14;
  w[4] += ( mxr_tx[4] * err + 0x2000 ) >> 14;
  w[5] += ( mxr_tx[5] * err + 0x2000 ) >> 14;
  w[6] += ( mxr_tx[6] * err + 0x2000 ) >> 14;
  w[7] += ( err + 0x20 ) >> 6;
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
  sum += w[7] << 8;
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
#define m_update( y )                                                                                                  \
  {                                                                                                                    \
    int err = y * 0xfff - mxr_pr;                                                                                      \
    fails <<= 1;                                                                                                       \
    fails |= calcfails[err + 4096];                                                                                    \
    train( err );                                                                                                      \
  }

// Input x (call up to MI times)

#define m_add( a, b )                                                                                                  \
  {                                                                                                                    \
    assert( ( a ) < MI );                                                                                              \
    mxr_tx[a] = stretch_t[b];                                                                                          \
  }

// predict next bit
#define m_p dot_product();

///};

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

template <int B>
class HashTable {
  U8 *t;        // table: 1 element = B bytes: checksum,priority,data,data,...
  const int NB; // size in bytes
public:
  HashTable( int n );
  U8 *get( U32 i );
};

template <int B>
HashTable<B>::HashTable( int n ) : NB( n - B ) {
  assert( B >= 2 && ( B & B - 1 ) == 0 );
  assert( n >= B * 4 && ( n & n - 1 ) == 0 );
  alloc( t, n + 512 );
  t = ( U8 * ) ( ( ( uintptr_t ) t + 63 ) & ~( uintptr_t ) 63 ); // align on cache line boundary
}

inline U32 hash1( U32 i ) {
  i *= 234567891;
  i = i << 21 | i >> 11;
  return ( i * 765432197 );
}
inline U32 hash2( U32 i ) {
  i *= 234567891;
  i = i << 19 | i >> 13;
  return ( i * 654321893 );
}
inline U32 hash3( U32 i ) {
  i *= 234567891;
  i = i << 21 | i >> 11;
  return ( i * 543210973 );
}
inline U32 hash4( U32 i ) {
  i *= 234567891;
  i = i << 19 | i >> 13;
  return ( i * 432109879 );
}
inline U32 hash5( U32 i ) {
  i *= 234567891;
  i = i << 21 | i >> 11;
  return ( i * 987654323 );
}

inline U32 hash6( U32 i ) {
  i *= 345678941;
  i = i << 19 | i >> 13;
  return ( i * 876543211 );
}
inline U32 hash7( U32 i ) {
  i *= 345678941;
  i = i << 21 | i >> 11;
  return ( i * 765432197 );
}
inline U32 hash8( U32 i ) {
  i *= 345678941;
  i = i << 19 | i >> 13;
  return ( i * 654321893 );
}
inline U32 hash9( U32 i ) {
  i *= 345678941;
  i = i << 21 | i >> 11;
  return ( i * 543210973 );
}
inline U32 hash0( U32 i ) {
  i *= 345678941;
  i = i << 19 | i >> 13;
  return ( i * 432109879 );
}

template <int B>
inline U8 *HashTable<B>::get( U32 i ) {
  U8 *p = t + ( i * B & NB ), *q, *r;
  i >>= 24;
  U8 c = i;
  if( *( p - 1 ) == c )
    return p;
  q = ( U8 * ) ( ( uintptr_t ) p ^ B );
  if( *( q - 1 ) == c )
    return q;
  r = ( U8 * ) ( ( uintptr_t ) p ^ B * 2 );
  if( *( r - 1 ) == c )
    return r;
  if( *p > *q )
    p = q;
  if( *p > *r )
    p = r;
#if 0 // ATTENTION !	CHANGE this to 1 if you start to use                                                              \
      //		HashTable with B!=16 in your versions.
  memset(p-1, 0, B);
  *(p-1)=i;		// This is big-endian-compatible
#else
  *( U32 * ) ( p - 1 ) = i; // This is NOT big-endian-compatible
  *( U32 * ) ( p + 3 ) = 0;
  *( U32 * ) ( p + 7 ) = 0;
  *( U32 * ) ( p + 11 ) = 0;
#endif
  return p;
}

//////////////////////////// MatchModel ////////////////////////

// MatchModel(n) predicts next bit using most recent context match.
//     using n bytes of memory.  n must be a power of 2 at least 8.
// MatchModel::p(y, m) updates the model with bit y (0..1) and writes
//     a prediction of the next bit to Mixer m.  It returns the length of
//     context matched (0..62).

U32 h1, h2, h3; // context hashes
int N, HN;      // last hash table index, n/8-1

enum { MAXLEN = 62 }; // maximum match length, at most 62
U32 len2cxt[MAXLEN * 2 + 1];
U32 len2order[MAXLEN + 1];

class MatchModel {
  int *ht;       // context hash -> next byte in buf
  int match;     // pointer to current byte in matched context in buf
  int buf_match; // buf[match]+256
  int len;       // length of match
  StateMap sm;   // len, bit, last byte -> prediction
public:
  MatchModel( int n ); // n must be a power of 2 at least 8.
  int p();             // predict next bit to m
  void upd();          // update bit y (0..1)
};

MatchModel::MatchModel( int n ) : sm( 55 << 8 ) {
  N = n / 2 - 1;
  HN = n / 8 - 1;
  pos = h1 = h2 = h3 = match = len = 0;
  assert( n >= 8 && ( n & n - 1 ) == 0 );
  alloc( buf, N + 1 );
  alloc( ht, HN + 1 );
}

#define SEARCH( hsh )                                                                                                  \
  {                                                                                                                    \
    len = 1;                                                                                                           \
    match = ht[hsh];                                                                                                   \
    if( match != pos ) {                                                                                               \
      while( len < MAXLEN + 1 && buf[match - len & N] == buf[pos - len & N] )                                          \
        ++len;                                                                                                         \
    }                                                                                                                  \
  }
#define SEARCH2( hsh )                                                                                                 \
  {                                                                                                                    \
    len = 1;                                                                                                           \
    match = ht[hsh];                                                                                                   \
    if( match != pos ) {                                                                                               \
      p = p1;                                                                                                          \
      while( len < MAXLEN + 1 && buf[match - len & N] == *p )                                                          \
        --p, ++len;                                                                                                    \
    }                                                                                                                  \
  }

void MatchModel::upd() {
  // find or extend match
  if( len > 2 ) {
    ++match;
    match &= N;
    if( len < MAXLEN )
      ++len;
  } else {
    if( pos >= MAXLEN ) {
      U8 *p1 = buf + pos - 1, *p;
      SEARCH2( h1 )
      if( len < 3 )
        SEARCH2( h2 )
#ifndef WIKI
      if( len < 3 )
        SEARCH2( h3 )
#endif
    } else {
      SEARCH( h1 )
      if( len < 3 )
        SEARCH( h2 )
#ifndef WIKI
      if( len < 3 )
        SEARCH( h3 )
#endif
    }

    --len;
  }
  buf_match = buf[match] + 256;

  // update index
#ifdef WIKI
  if( ( pos & 1 ) || ( len > 11 ) )
#endif
    ht[h1] = pos;
  ht[h2] = pos;
#ifndef WIKI
  ht[h3] = pos;
#endif
}

int MatchModel::p() {
  int cxt = c0;
  if( len > 0 ) {
    int b = buf_match;
    if( ( b >> ( 8 - bcount ) ) == cxt ) {
      b = b >> ( 7 - bcount ) & 1; // next bit
      cxt = len2cxt[len * 2 - b] + c1;
    } else
      len = 0;
  }

  m_add( 0, sm.p( cxt ) );
  return len;
}

//////////////////////////// Predictor /////////////////////////

// A Predictor estimates the probability that the next bit of
// uncompressed data is 1.  Methods:
// Predictor(n) creates with 3*n bytes of memory.
// p() returns P(1) as a 12 bit number (0-4095).
// update(y) trains the predictor with the actual bit (0 or 1).

int MEM = 0; // Global memory usage = 3*MEM bytes (1<<20 .. 1<<29)

U8 t0[0x10000];                                   // order 1 cxt -> state
U8 *t0c1 = t0, *cp[6] = {t0, t0, t0, t0, t0, t0}; // pointer to bit history
U32 h[6], pw = 0, c8 = 0, cc = 0, prevfail = 0;
U8 fails = 0;
StateMap sm[6];
APM a1( 24 * 0x10000 ), a2( 24 * 0x800 );

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

typedef enum { COMPRESS, DECOMPRESS } Mode;
class Encoder {
public:
#ifdef WIKI
  HashTable<16> t4a; // cxt -> state
  HashTable<16> t4b; // cxt -> state
#  define t1a t4a
#  define t2a t4a
#  define t3a t4a
#  define t1b t4b
#  define t2b t4b
#  define t3b t4b
#else
  HashTable<16> t1; // cxt -> state
  HashTable<16> t2; // cxt -> state
  HashTable<16> t3; // cxt -> state
#  define t1a t1
#  define t2a t2
#  define t3a t3
#  define t1b t1
#  define t2b t2
#  define t3b t3
#endif
  MatchModel mm; // predicts next bit by matching context
  FILE *archive; // Compressed data file
  U32 x1, x2;    // Range, initially [0, 1), scaled by 2^32
  U32 x;         // Decompress mode: last 4 input bytes of archive
  U32 p;
  int *add2order;

#define cp_update( m0, m1, m2, m3, m4, m5 )                                                                            \
  assert( y == 0 || y == 1 );                                                                                          \
  y22 = y << 22;                                                                                                       \
  {                                                                                                                    \
    const U8 *p = &State_table[0];                                                                                     \
    if( y )                                                                                                            \
      p += 256 * 3;                                                                                                    \
    *cp[0] = nex( *cp[0], m0 );                                                                                        \
    *cp[1] = nex( *cp[1], m1 );                                                                                        \
    *cp[2] = nex( *cp[2], m2 );                                                                                        \
    *cp[3] = nex( *cp[3], m3 );                                                                                        \
    *cp[4] = nex( *cp[4], m4 );                                                                                        \
    *cp[5] = nex( *cp[5], m5 );                                                                                        \
  }                                                                                                                    \
  c0 += c0 + y;

#define upd0( m0, m1, m2, m3, m4, m5, bc, sh )                                                                         \
  assert( y == 0 || y == 1 );                                                                                          \
  y22 = y << 22;                                                                                                       \
  bcount = bc;                                                                                                         \
  add2order += MI * 10;                                                                                                \
  {                                                                                                                    \
    const U8 *p = &State_table[0];                                                                                     \
    if( y )                                                                                                            \
      p += 256 * 3;                                                                                                    \
    int j = y + 1 << sh;                                                                                               \
    *cp[1] = nex( *cp[1], m1 );                                                                                        \
    cp[1] += j;                                                                                                        \
    *cp[2] = nex( *cp[2], m2 );                                                                                        \
    cp[2] += j;                                                                                                        \
    *cp[3] = nex( *cp[3], m3 );                                                                                        \
    cp[3] += j;                                                                                                        \
    *cp[4] = nex( *cp[4], m4 );                                                                                        \
    cp[4] += j;                                                                                                        \
    *cp[5] = nex( *cp[5], m5 );                                                                                        \
    cp[5] += j;                                                                                                        \
    *cp[0] = nex( *cp[0], m0 );                                                                                        \
  }                                                                                                                    \
  c0 += c0 + y;

#define upd3( m0, m1, m2, m3, m4, m5, bc )                                                                             \
  cp_update( m0, m1, m2, m3, m4, m5 ) bcount = bc;                                                                     \
  add2order += MI * 10;                                                                                                \
  cp[1] = t1b.get( hash6( c0 - h[1] ) );                                                                               \
  upd3_cp cp[5] = t2b.get( hash0( c0 * 37 - h[5] ) );

#define upd7( m0, m1, m2, m3, m4, m5 )                                                                                 \
  cp_update( m0, m1, m2, m3, m4, m5 ) {                                                                                \
    int c = c0 - 256;                                                                                                  \
    add2order = mxr_wx + MI * 10 * add2order0 * 8;                                                                     \
    c1 = ( c1 >> 4 ) + ( c & 240 );                                                                                    \
    buf[pos++] = c;                                                                                                    \
    pos &= N;                                                                                                          \
    cc = ( cc << 8 ) + ( c8 >> 24 );                                                                                   \
    c8 = ( c8 << 8 ) + ( c4 >> 24 );                                                                                   \
    c4 = c4 << 8 | c;                                                                                                  \
    upd7_h123 h[0] = c << 8;       /* order 1 */                                                                       \
    h[1] = ( c4 & 0xffff ) * 8191; /* order 2 */                                                                       \
    cp[1] = t1b.get( hash1( h[1] ) );                                                                                  \
    upd7_cp if( c >= 65 && c <= 90 ) c += 32; /* lowercase unigram word order */                                       \
    if( c >= 97 && c <= 122 )                                                                                          \
      h[5] = ( h[5] + c ) * ( 191 << 1 );                                                                              \
    else                                                                                                               \
      pw = h[5] * 241, h[5] = 0;                                                                                       \
    cp[5] = t2a.get( hash5( h[5] - pw ) );                                                                             \
    c0 = 1;                                                                                                            \
    bcount = 0;                                                                                                        \
    mm.upd();                                                                                                          \
  }

  U32 Predictor_upd( int y ) {
    m_update( y );

    // predict
    int len = mm.p(), pr;
    if( len == 0 )
      len = ( static_cast<int>( *cp[1] != 0 ) + static_cast<int>( *cp[2] != 0 ) + static_cast<int>( *cp[3] != 0 )
              + static_cast<int>( *cp[4] != 0 ) )
            * MI;
    else
      len = len2order[len];
    mxr_cxt = add2order + len;
    m_add( 1, sm[1].p( *cp[1] ) );
    m_add( 2, sm[2].p( *cp[2] ) );
    m_add( 3, sm[3].p( *cp[3] ) );
    m_add( 4, sm[4].p( *cp[4] ) );
    m_add( 5, sm[5].p( *cp[5] ) );

#ifdef WIKI
    int h0c0 = h[0] + c0;
    cp[0] = t0 + h0c0;
    m_add( 6, sm[0].p( *cp[0] ) );
    pr = m_p;
    pr = squash( pr ) + 7 * a1.pp( ( pr + 2047 ) * 23, h0c0 ) >> 3;
    mxr_pr = pr;
    pr = pr + 3 * a2.pp( stretch_t2[pr], fails * 8 + bcount ) + 2 >> 2;
#else
    cp[0] = t0c1 + c0;
    m_add( 6, sm[0].p( *cp[0] ) );
    pr = m_p;
    pr = squash( pr ) + 3 * a1.pp( ( pr + 2047 ) * 23, h[0] + c0 ) >> 2;
    mxr_pr = pr;
    pr = ( pr * 3 + 5 * a2.pp( stretch_t2[pr], fails + prevfail ) + 4 ) >> 3;
#endif
    return pr + static_cast<int>( pr < 2048 );
  }

  // Compress bit y
#define enc1( k )                                                                                                      \
  int y = ( c >> k ) & 1;                                                                                              \
  U32 xmid = x1 + ( x2 - x1 >> 12 ) * p + ( ( x2 - x1 & 0xfff ) * p >> 12 );                                           \
  assert( xmid >= x1 && xmid < x2 );                                                                                   \
  y ? ( x2 = xmid ) : ( x1 = xmid + 1 );

#define enc2                                                                                                           \
  while( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 ) { /* pass equal leading bytes of range */                                 \
    putc( x2 >> 24, archive );                                                                                         \
    x1 <<= 8;                                                                                                          \
    x2 = ( x2 << 8 ) + 255;                                                                                            \
  }

  // Return decompressed bit
#define dec1( k )                                                                                                      \
  U32 xmid = x1 + ( x2 - x1 >> 12 ) * p + ( ( x2 - x1 & 0xfff ) * p >> 12 );                                           \
  assert( xmid >= x1 && xmid < x2 );                                                                                   \
  int y = x <= xmid;                                                                                                   \
  y ? ( x2 = xmid ) : ( x1 = xmid + 1 );

#define dec2                                                                                                           \
  while( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 ) {  /* pass equal leading bytes of range */                                \
    x = ( x << 8 ) + ( getc( archive ) & 255 ); /* EOF is OK */                                                        \
    x1 <<= 8;                                                                                                          \
    x2 = ( x2 << 8 ) + 255;                                                                                            \
  }

  Encoder( Mode m, FILE *f );
  void flush(); // call this when compression is finished

#ifdef WIKI
#  define eight_bits( part1, part2 )                                                                                   \
    sm[0].t += 256;                                                                                                    \
    {                                                                                                                  \
      part1( 7 );                                                                                                      \
      upd0( 0, 1, 1, 0, 0, 0, 1, 0 );                                                                                  \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    {                                                                                                                  \
      part1( 6 );                                                                                                      \
      upd0( 1, 1, 1, 0, 0, 0, 2, 1 );                                                                                  \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    sm[1].t += 256;                                                                                                    \
    {                                                                                                                  \
      part1( 5 );                                                                                                      \
      upd0( 1, 1, 1, 0, 0, 0, 3, 2 );                                                                                  \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    sm[2].t += 256;                                                                                                    \
    sm[5].t += 256;                                                                                                    \
    {                                                                                                                  \
      part1( 4 );                                                                                                      \
      upd3( 1, 1, 1, 0, 0, 0, 4 );                                                                                     \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    sm[4].t += 256;                                                                                                    \
    {                                                                                                                  \
      part1( 3 );                                                                                                      \
      upd0( 1, 1, 0, 0, 0, 1, 5, 0 );                                                                                  \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    {                                                                                                                  \
      part1( 2 );                                                                                                      \
      upd0( 1, 1, 0, 0, 2, 1, 6, 1 );                                                                                  \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    {                                                                                                                  \
      part1( 1 );                                                                                                      \
      upd0( 1, 1, 0, 0, 2, 1, 7, 2 );                                                                                  \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    sm[0].t -= 256;                                                                                                    \
    sm[1].t -= 256;                                                                                                    \
    sm[2].t -= 256;                                                                                                    \
    sm[4].t -= 256;                                                                                                    \
    sm[5].t -= 256;                                                                                                    \
    {                                                                                                                  \
      part1( 0 );                                                                                                      \
      upd7( 1, 1, 0, 0, 2, 1 );                                                                                        \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;
#else
#  define eight_bits( part1, part2 )                                                                                   \
    {                                                                                                                  \
      part1( 7 );                                                                                                      \
      upd0( 1, 0, 0, 2, 0, 1, 1, 0 );                                                                                  \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    sm[4].t += 256;                                                                                                    \
    sm[5].t += 256;                                                                                                    \
    {                                                                                                                  \
      part1( 6 );                                                                                                      \
      upd0( 1, 0, 0, 2, 0, 1, 2, 1 );                                                                                  \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    sm[2].t += 256;                                                                                                    \
    {                                                                                                                  \
      part1( 5 );                                                                                                      \
      upd0( 1, 0, 0, 2, 0, 1, 3, 2 );                                                                                  \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    sm[1].t += 256;                                                                                                    \
    {                                                                                                                  \
      part1( 4 );                                                                                                      \
      upd3( 1, 0, 1, 2, 0, 1, 4 );                                                                                     \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    {                                                                                                                  \
      part1( 3 );                                                                                                      \
      upd0( 1, 0, 1, 2, 0, 1, 5, 0 );                                                                                  \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    {                                                                                                                  \
      part1( 2 );                                                                                                      \
      upd0( 1, 0, 1, 2, 0, 1, 6, 1 );                                                                                  \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    {                                                                                                                  \
      part1( 1 );                                                                                                      \
      upd0( 1, 0, 1, 2, 0, 1, 7, 2 );                                                                                  \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;                                                                                                             \
    sm[1].t -= 256;                                                                                                    \
    sm[2].t -= 256;                                                                                                    \
    sm[4].t -= 256;                                                                                                    \
    sm[5].t -= 256;                                                                                                    \
    {                                                                                                                  \
      part1( 0 );                                                                                                      \
      upd7( 1, 0, 1, 2, 0, 1 );                                                                                        \
      p = Predictor_upd( y );                                                                                          \
    }                                                                                                                  \
    part2;
#endif

  // Compress one byte
  inline void compress( int c ) {
    ///assert(mode==COMPRESS);
    if( TextFlag != 0 )
      if( c == 0x20 || c == 0x1f )
        c ^= 0x3f;
    eight_bits( enc1, enc2 )
  }

  // Decompress and return one byte
  inline int decompress() {
    eight_bits( dec1, dec2 ) int c = c4 & 255;
    if( TextFlag != 0 )
      if( c == 0x20 || c == 0x1f )
        c ^= 0x3f;
    return c;
  }
};

Encoder::Encoder( Mode m, FILE *f ) :
#ifdef WIKI
    t4a( MEM ),
    t4b( MEM ),
#else
    t1( MEM / 2 ),
    t2( MEM ),
    t3( MEM / 2 ),
#endif
    mm( MEM ),
    archive( f ),
    x1( 0 ),
    x2( 0xffffffff ),
    x( 0 ),
    p( 2048 ) {

  if( m == DECOMPRESS ) { // x = first 4 bytes of archive
    for( int i = 0; i < 4; ++i )
      x = ( x << 8 ) + ( getc( archive ) & 255 );
  }

  int i, pi = 0;
  for( int x = -2047; x <= 2047; ++x ) { // invert squash()
    int i = squash_init( x );
    squash( x ) = i + SQUARD; //rounding,  needed at the end of Predictor::update()
    for( int j = pi; j <= i; ++j )
      stretch_t[j] = x, stretch_t2[j] = ( x + 2047 ) * 23;
    pi = i + 1;
  }
  stretch_t[4095] = 2047;
  stretch_t2[4095] = 4094 * 23;

  for( i = 0; i < 1024; ++i )
    dt[i] = 16384 / ( i + i + 3 );

#ifndef WIKI
  for( i = 0; i < 256; ++i ) {
    pi = ( i & 1 ) << 10;
    if( ( i & 6 ) != 0 )
      pi += 512;
    if( ( i & 248 ) != 0 )
      pi += 256;
    calcprevfail[i] = pi;
  }
#endif

  for( i = -4096; i < 4096; ++i ) {
    int e = i, v = 0;
    if( e < 0 )
      e = -e;
    if( e > 1024 )
      v = 1;
    if( e > 2624 )
      v = 3;
    calcfails[i + 4096] = v;
  }

  for( i = 2; i <= MAXLEN * 2; i += 2 ) {
    int c;
    if( i < 32 )
      c = i;
    else
      c = ( i >> 3 ) * 2 + 24;
    c *= 256;
    len2cxt[i] = c;
    len2cxt[i - 1] = c - 256;
    c = i >> 1;
    len2order[c] = ( 5 + static_cast<int>( c >= 8 ) + static_cast<int>( c >= 12 ) + static_cast<int>( c >= 16 )
                     + static_cast<int>( c >= 32 ) )
                   * MI;
  }

  alloc( mxr_wx, MI * MC );
  for( i = 0; i < MI * MC; ++i )
    mxr_wx[i] = ( 1 << ( DP_SHIFT - 2 ) );
  mxr_cxt = mxr_wx;
  add2order = mxr_wx;
}

void Encoder::flush() {
  ///if (mode==COMPRESS)
  putc( x1 >> 24, archive ); // Flush first unequal byte of range
}

//////////////////////////// User Interface ////////////////////////////

int main( int argc, char **argv ) {
  // Check arguments
  if( argc != 4 || ( ( isdigit( argv[1][0] ) == 0 ) && argv[1][0] != 'd' ) ) {
    printf( "lpaq5 file compressor (C) 2007, Matt Mahoney\n"
            "Licensed under GPL, http://www.gnu.org/copyleft/gpl.html\n"
            "\n"
            "To compress:   lpaq5 N input output  (N=0..9, uses 6+3*2^N MB)\n"
            "To decompress: lpaq5 d input output  (needs same memory)\n" );
    return 1;
  }

  // Get start time
  clock_t start = clock();

  // Open input file
  FILE *in = fopen( argv[2], "rb" ), *out = 0;
  if( in == nullptr )
    perror( argv[2] ), exit( 1 );

  // Compress
  if( isdigit( argv[1][0] ) != 0 ) {
#ifndef WITHOUT_COMPRESSOR
    MEM = 1 << ( argv[1][0] - '0' + 20 );

    { // a better data detection algorithm will be here in future
      U8 buf[4096];
      int i = fread( buf, 1, 4096, in ), k = 0;
      while( i-- > 0 ) {
        if( buf[i] > 0x7f )
          ++k;
      }
      if( k == 0 )
        TextFlag = 1;
    }

    // Encode header: version 5, memory option, file size
    fseek( in, 0, SEEK_END );
    long size = ftell( in );
    if( size < 0 || size >= 0x7FFFFFFF )
      quit( "input file too big" );
    fseek( in, 0, SEEK_SET );

    out = fopen( argv[3], "wb" );
    if( out == nullptr )
      perror( argv[3] ), exit( 1 );
    fprintf( out, "pQ%c%c%ld%ld%ld%ld", 5, argv[1][0], size >> 24, size >> 16, size >> 8, size );

    // Compress
    Encoder e( COMPRESS, out );
    int c;
    while( ( c = getc( in ) ) != EOF ) {
      e.compress( c );
      if( ( pos & ( 256 * 1024 - 1 ) ) == 0 )
#  ifndef WIKI
        if( TextFlag != 0 )
          if( ( pos == 7 * 1024 * 1024 && DP_SHIFT == 16 ) || ( pos == 1024 * 1024 && DP_SHIFT == 15 )
              || DP_SHIFT == 14 )
#  else
        if( ( pos == 7 * 1024 * 1024 && DP_SHIFT == 16 ) || ( pos == 1280 * 1024 && DP_SHIFT == 15 ) || DP_SHIFT == 14 )
#  endif
            for( DP_SHIFT++, c = 0; c < MI * MC; ++c )
              mxr_wx[c] *= 2;
    }
    e.flush();
#endif
  }

  // Decompress
  else {
#ifndef WITHOUT_DECOMPRESSOR
    // Check header version, get memory option, file size
    if( getc( in ) != 'p' || getc( in ) != 'Q' || getc( in ) != 5 )
      quit( "Not a lpaq5 file" );
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

    // Decompress
    out = fopen( argv[3], "wb" );
    if( out == nullptr )
      perror( argv[3] ), exit( 1 );
    Encoder e( DECOMPRESS, in );

    { // this is because we don't save TextFlag in the compressed file
      U8 buf[4096], *p = &buf[0], c;
      long s = 4096, ss, k = 0;
      if( s > size )
        s = size;
      size -= s;
      ss = s;
      while( s-- > 0 ) {
        c = e.decompress();
        *p++ = c;
        if( c > 0x7f )
          ++k;
      }
      if( k == 0 )
        for( s = 0, TextFlag = 1; s < ss; ++s )
          if( buf[s] == 0x1f || buf[s] == 0x20 )
            buf[s] ^= 0x3f;
      fwrite( buf, 1, ss, out ), k = 0;
    }

    while( size-- > 0 ) {
      int c;
      putc( e.decompress(), out );
      if( ( pos & ( 256 * 1024 - 1 ) ) == 0 )
#  ifndef WIKI
        if( TextFlag != 0 )
          if( ( pos == 7 * 1024 * 1024 && DP_SHIFT == 16 ) || ( pos == 1024 * 1024 && DP_SHIFT == 15 )
              || DP_SHIFT == 14 )
#  else
        if( ( pos == 7 * 1024 * 1024 && DP_SHIFT == 16 ) || ( pos == 1280 * 1024 && DP_SHIFT == 15 ) || DP_SHIFT == 14 )
#  endif
            for( DP_SHIFT++, c = 0; c < MI * MC; ++c )
              mxr_wx[c] *= 2;
    }
#endif
  }

  // Report result
  assert( in );
  assert( out );
  printf( "%ld -> %ld in %1.3f sec. using %d MB memory\n", ftell( in ), ftell( out ),
          double( clock() - start ) / CLOCKS_PER_SEC, mem_usage >> 20 );

  return 0;
}
