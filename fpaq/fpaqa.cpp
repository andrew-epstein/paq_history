/* fpaqa - Stationary order 0 file compressor.
(C) 2007, Matt Mahoney under GPL ver. 3.
Released Dec. 15, 2007.

    LICENSE

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details at
    Visit <http://www.gnu.org/copyleft/gpl.html>.

This program is a simple order-0 model implementing Jarek Duda's asymmetric
binary coder.  The coder uses the same interface as the arithmetic coder
in better compresssors such as PAQ, LPAQ, BBB, or SR2, so it could be
used in those programs as well.

To compress:   fpaqa c input output
To decompress: fpaqa d input output

To compile:

  g++ -O2 -Os -march=pentiumpro -fomit-frame-pointer -s fpaqa.cpp -o fpaqa

To use arithmetic coder (instead of Duda's asymmetric coder), compile
with -DARITH

The asymmetric coder has parameter settings N=10, R=7, B=500000.  These
could be increased to the maximum N=12, R=7 to improve compression at
the cost of speed and memory.  B could be increased to improve compression at
the cost of memory.  Memory usage is pow(2,N+R+2) + 5*B/4 bytes for
compression and pow(2,N+R+1) bytes for decompression.  These parameters
are set in class Encoder (second version).

*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define NDEBUG // remove for debugging
#include <assert.h>

// Create an array p of n elements of type T
template <class T>
void alloc( T *&p, int n ) {
  p = ( T * ) calloc( n, sizeof( T ) );
  if( !p )
    printf( "Out of memory" ), exit( 1 );
}

///////////////////////////// Squash //////////////////////////////

// return p = 1/(1 + exp(-d)), d scaled by 8 bits, p scaled by 12 bits
int squash( int d ) {
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

// Inverse of squash. stretch(d) returns ln(p/(1-p)), d scaled by 8 bits,
// p by 12 bits.  d has range -2047 to 2047 representing -8 to 8.
// p has range 0 to 4095 representing 0 to 1.

class Stretch {
  short t[4096];

public:
  Stretch();
  int operator()( int p ) const {
    assert( p >= 0 && p < 4096 );
    return t[p];
  }
} stretch;

Stretch::Stretch() {
  int pi = 0;
  for( int x = -2047; x <= 2047; ++x ) { // invert squash()
    int i = squash( x );
    for( int j = pi; j <= i; ++j )
      t[j] = x;
    pi = i + 1;
  }
  t[4095] = 2047;
}

//////////////////////////// Predictor /////////////////////////

// Adaptive order-0 model, derived from Ilia Muraviev's fpaq0p

class Predictor {
private:
  int cxt{1};          // Context: last 0-8 bits with a leading 1
  unsigned int t[512]; // Probability of 1

public:
  Predictor() {
    for( unsigned int &i: t )
      i = 32768;
  }

  // Assume a stationary order 0 stream of 9-bit symbols
  int p() const {
    return t[cxt] >> 4;
  }

  void update( int y ) {
    if( y != 0 )
      t[cxt] += ( 65536 - t[cxt] ) >> 5;
    else
      t[cxt] -= t[cxt] >> 5;
    if( ( cxt += cxt + y ) >= 512 )
      cxt = 1;
  }
};

//////////////////////////// Encoder ////////////////////////////

/*
An Encoder does arithmetic encoding for data compression.  Methods:
- Encoder(COMPRESS, f) creates encoder for compression to archive f, which
  must be open past any header for writing in binary mode
- Encoder(DECOMPRESS, f) creates encoder for decompression from archive f,
  which must be open past any header for reading in binary mode
- encode(bit) in COMPRESS mode compresses bit to file f.
- decode() in DECOMPRESS mode returns the next decompressed bit from file f.
- flush() should be called when there is no more to compress.

An Encoder expects a Predictor to be present with the following methods:
- Predictor::p() returns q, the probability that the next bit will be 1 as a
  12 bit number in the range 0..4095.
- Predictor::update(d) updates the model with bit d (0 or 1), where d was
  predicted by p().  Predictions depend only on the sequence of d.

This version of the encoder (if compiled with -DARITH) uses arithmetic coding.
It is also used in most version of PAQ, LPAQ, SR2, and BBB.
*/

#ifdef ARITH

typedef enum { COMPRESS, DECOMPRESS } Mode;
class Encoder {
private:
  Predictor predictor;
  const Mode mode;     // Compress or decompress?
  FILE *archive;       // Compressed data file
  unsigned int x1, x2; // Range, initially [0, 1), scaled by 2^32
  unsigned int x;      // Last 4 input bytes of archive.
public:
  Encoder( Mode m, FILE *f );
  void encode( int y ); // Compress bit y
  int decode();         // Uncompress and return bit y
  void flush();         // Call when done compressing
};

// Constructor
Encoder::Encoder( Mode m, FILE *f ) : predictor(), mode( m ), archive( f ), x1( 0 ), x2( 0xffffffff ), x( 0 ) {
  // In DECOMPRESS mode, initialize x to the first 4 bytes of the archive
  if( mode == DECOMPRESS ) {
    for( int i = 0; i < 4; ++i ) {
      int c = getc( archive );
      if( c == EOF )
        c = 0;
      x = ( x << 8 ) + c;
    }
  }
}

/* encode(y) -- Encode bit y by splitting the range [x1, x2] in proportion
to P(1) and P(0) as given by the predictor and narrowing to the appropriate
subrange.  Output leading bytes of the range as they become known. */

inline void Encoder::encode( int y ) {
  // Update the range
  const unsigned int xmid = x1 + ( ( x2 - x1 ) >> 12 ) * predictor.p();
  assert( xmid >= x1 && xmid < x2 );
  if( y )
    x2 = xmid;
  else
    x1 = xmid + 1;
  predictor.update( y );

  // Shift equal MSB's out
  while( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 ) {
    putc( x2 >> 24, archive );
    x1 <<= 8;
    x2 = ( x2 << 8 ) + 255;
  }
}

/* Decode one bit from the archive, splitting [x1, x2] as in the encoder
and returning 1 or 0 depending on which subrange the archive point x is in.
*/
inline int Encoder::decode() {
  // Update the range
  const unsigned int xmid = x1 + ( ( x2 - x1 ) >> 12 ) * predictor.p();
  assert( xmid >= x1 && xmid < x2 );
  int y = 0;
  if( x <= xmid ) {
    y = 1;
    x2 = xmid;
  } else
    x1 = xmid + 1;
  predictor.update( y );

  // Shift equal MSB's out
  while( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 ) {
    x1 <<= 8;
    x2 = ( x2 << 8 ) + 255;
    int c = getc( archive );
    if( c == EOF )
      c = 0;
    x = ( x << 8 ) + c;
  }
  return y;
}

// Should be called when there is no more to compress
void Encoder::flush() {
  if( mode == COMPRESS )
    putc( x2 >> 24, archive ); // First unequal byte
}

#else

/* This version of the encoder (if compiled without -DARITH) implements
Jarek Duda's asymmetric binary system, as described in

Asymmetric binary encoding is described in chapter 3 of Jarek Duda,
"Optimal encoding on discrete lattice with translational invariant constrains
using statistical algorithms", 2007.
http://uk.arxiv.org/PS_cache/arxiv/pdf/0710/0710.3861v1.pdf

The interface is the same as the arithmetic version (see above).

The coding method has an advantage over arithmetic coding in that it
uses only one state variable instead of two (range bounds), so the
coder can be implemented with lookup tables.  However, the coding method
as described is unsuitable for predictive methods (PPM, PAQ, LZMA, etc)
because the bit stream must be decoded in the reverse order that it was
encoded, which means that the model would see different contexts during
compression and decompression.  This was not a problem in the paper because
the application described, the bit predictions were fixed in advance.

This implementation solves the general problem by saving predictions in
a stack and then coding them in reverse order to a second stack, which
is then popped and written to disk in blocks.  Decoding is straightforward
and fast.  The asymmetric coder can be used as a drop-in replacement for
an arithmetic coder.

Asymmetric binary encoding encodes bit d in {0,1}, having probability q
in (0,1), as an integer state x.  If w is the previous state, then d is
encoded:

  if d is 0 then x = ceil((w + 1)/(1 - q)) - 1
  if d is 1 then x = floor(w/q)

To decode, given state x and prediction q:

  d = ceil((x + 1)*q) - ceil(x*q)

which is 1 if the fractional part of x*q >= 1 - q, else 0.  Then the prior
state w is obtained from x:

  if d is 0 then w = x - ceil(x*q)
  if d is 1 then w = ceil(x*q)

The operations may be carried out in N bit arithmetic.  Let Q = pow(2,N).
Then we restrict q to the range [1..Q-1] and x to the range [Q..2*Q-1].

Define x = encode(w,q,d) as:

  if d is 0 then x = ((w + 1)*Q - 1)/(Q - q)
  if d is 1 then x = (w*Q)/q

Then the complete encoding operation is

  read d
  while encode(x,q,d) >= 2*Q do
    write bit x mod 2
    x := floor(x/2)
  x := encode(x,q,d)

Define w,d = decode(x,q) as follows:

  d = floor(((x+1)*q - 1)/Q) - floor((x*q - 1)/Q)
  xq = floor((x*q - 1)/Q) + 1
  if d = 0 then w = x - xq
  if d = 1 then w = xq

The complete decoding operation is

  w,d := decode(x,q)
  write d
  while w < Q do
    read bit b
    w := w*2 + b
  x := w

Both reading and writing during compression must be in the reverse
order as decompression.  During compression, the input is divided into
blocks of size B bits.  When d is encoded with probability q, the
encoder just saves d and q on a stack of size B until full.  When the
stack is ready to be written, either because it is full or the end
of input is reached, then x is reset to state x = Q, the d and q
are popped and encoded to a second stack of bits, then the final state
x and the size of the stack, n <= B, are written to the output file, and 
then the bits are popped and written.

The decoder does not use any stacks.  To decode, the decoder reads
x and n, then decodes n bits of output.  As it counts output bits down
to 0, it expects to find x = Q, then read new values of x and n.

The decoder expects to read x as a 2 byte big-endian integer, and
n as a 3 byte big-endian integer.  Bits should be read in LSB to MSB
order.  The first byte consists of a 1 bit followed 0 to 7 data bits.
Subsequent bytes contain 8 bits each.  For example, if there are 2 data
bytes, 00010011 11000111, then 12 bits are read in the order 1100 11100011.

The encoder and decoder are implemented as lookup tables.  To reduce
the table sizes, q is quantized to R = 7 bits as follows:

  r = min(7.875, max(-7.875, round(stretch(q)*8)))
  q = squash(r)

where

  stretch(q) = ln(q/(1 - q)
  squash(r) = 1/(1 + exp(-r))

are inverses of each other.  This quantizes r nonuniformly with
closer spacing near 0 and 1.  Experimentally, this increases compressed
size about 0.02% over a 12 bit uniform quantization.  A 6 bit quantization
would increase size about 0.1%.

Compression algorithm:

  read d
  q -> r (table lookup)
  push r,d on input stack
  if input stack is full or end of input then
    n = 0
    w = Q
    ni = input stack size
    while input stack is not empty
      pop r,d
      r,d,w -> k,x  (table lookup)
      write k low bits of w to output stack
      n := n + k
      w := x
    write w,ni
    repeat n times
      pop b from output stack
      write b

  In the input stack, r and d are packed into 1 byte with d in the low bit.
  In the encoding table, enc[rd][w], the output is a 16 bit value with
  k in the high 4 bits and x in the low N bits.  Bit N is an implied 1
  and is not set in w or x.

  Writing the output stack is a simple block write of floor(n/8)+1 bytes.
  The first byte written consists of leading zeros, then a 1, then 0 to 7
  valid bits, which should be read LSB to MSB.  Remaining bytes are 8 bits,
  which should be read LSB to MSB.

Decompression:

  if ni = 0 then read x,ni
  q -> r (table lookup)
  r,x -> d,x,k (table lookup)
  read k low bits into x
  ni := ni - 1
  write d

  In the decoding table, dec[r][x], bit N of x (always 1) is dropped.
  The table output is a 16 bit signed value with d in bit 15 and new x in the
  low bits.  k is implied by the number of left shifts until x >= Q.
*/

typedef enum { COMPRESS, DECOMPRESS } Mode;
class Encoder {
private:
  Predictor predictor;
  const Mode mode;     // Compress or decompress?
  enum { N = 10 };     // Number of bits in state x, max 12
  enum { R = 7 };      // Number of bits in r (quantized q), max 7
  enum { B = 500000 }; // Input stack size (compression)
  enum {
    BO = B * 2 + 128
  };                               // Output stack size in bits, a multiple of 8, max
                                   // 0xfffff8 (compression).  It should reserve enough
                                   // space > B in case the compressor expands the input.
  FILE *archive;                   // Compressed data file
  int x;                           // Encoder state (decompression)
  int n;                           // Number of bits in input stack (compression)
                                   // or remaining in block (decompression)
  unsigned char *qr;               // q -> r quantization
  unsigned short ( *enc )[1 << N]; // rd,w -> k,x (compression)
  short ( *dec )[1 << N];          // r,x -> d,x>>k (decompression)
  unsigned char *ins;              // Input stack (compression)
  unsigned char *outs;             // Output stack (compression)
  int ch;                          // Input byte (decompression)
public:
  Encoder( Mode m, FILE *f );
  void encode( int d ); // Compress bit y
  int decode();         // Uncompress and return bit y
  void flush();         // Call when done compressing
};

// Initialize tables
Encoder::Encoder( Mode m, FILE *f ) : mode( m ), archive( f ), x( 0 ), n( 0 ), enc( 0 ), dec( 0 ), ins( 0 ), ch( 0 ) {
  alloc( qr, 4096 );
  if( mode == COMPRESS )
    alloc( enc, 2 << ( N + R ) );
  else
    alloc( dec, 1 << ( N + R ) );

  // Init qr[q] = r (r*2 in COMPRESS mode)
  int qinv[256]; // r -> q
  for( int i = 0; i < 256; ++i ) {
    int mn = 1 << ( 12 - N ); // min distance from 0 or 4096
    qinv[i] = squash( ( i << ( 12 - R ) ) - 2048 + ( 1 << ( 11 - R ) ) ) & -mn;
    if( qinv[i] < mn )
      qinv[i] = mn;
    if( qinv[i] > 4096 - mn )
      qinv[i] = 4096 - mn;
  }
  for( int i = 0; i < 4096; ++i ) {
    qr[i] = ( stretch( i ) + 2048 ) >> ( 12 - R );
    if( mode == COMPRESS )
      qr[i] *= 2;
  }

  // Init enc[rd][w] = k,x (k in bits 12..15, x in bits 0..N-1, bit N implied)
  if( enc != nullptr ) {
    alloc( ins, B );
    alloc( outs, ( BO + 7 ) / 8 );
    for( int i = 0; i < 2 << R; ++i ) {
      for( int j = 0; j < 1 << N; ++j ) {
        int r = i >> 1;
        int d = i & 1;
        int q = qinv[r] >> ( 12 - N );
        int k = 0;
        int w = j + ( 1 << N );
        int x1 = 0;
        while( k < 15 ) {
          assert( q > 0 );
          assert( ( 1 << N ) - q > 0 );
          if( d != 0 )
            x1 = ( w << N ) / q;
          else
            x1 = ( ( ( w + 1 ) << N ) - 1 ) / ( ( 1 << N ) - q );
          if( x1 < ( 2 << N ) )
            break;

          w >>= 1, ++k;
        }
        enc[i][j] = k << 12 | x1 - ( 1 << N );
      }
    }
  }

  // Init dec[r][x-(1<<N)] = d,x (d in bit 15, x in bits 0..N)
  if( dec != nullptr ) {
    for( int i = 0; i < 1 << R; ++i ) {
      int q = qinv[i] >> ( 12 - N );
      for( int j = 0; j < 1 << N; ++j ) {
        int x1 = j + ( 1 << N );
        int d = ( ( ( x1 + 1 ) * q - 1 ) >> N ) - ( ( x1 * q - 1 ) >> N );
        assert( d == 0 || d == 1 );
        int xq = ( ( x1 * q - 1 ) >> N ) + 1;
        assert( xq >= 0 && xq <= x1 );
        dec[i][j] = d != 0 ? 0x8000 + xq : x1 - xq;
      }
    }
  }
  /*
  // Check that enc and dec are inverses (for testing, normally skipped.
  // To enable test, allocate both enc and dec above and #undef NDEBUG.
  if (enc && dec) {
    for (int w=1<<N; w<2<<N; ++w) {  // check w,r,d -> enc -> dec -> w,r,d
      for (int r=0; r<1<<R; ++r) {
        for (int d=0; d<2; ++d) {
          int kx=enc[r*2+d][w-(1<<N)];
          int k=kx>>12;
          int x=(kx&0xfff)+(1<<N);
          int wr=w&(1<<k)-1;  // written bits
          int dx=dec[r][x-(1<<N)];
          int d1=dx>>15&1;
          dx&=0x7fff;
          dx<<=k;
          dx+=wr;
          assert(d1==d);
          assert(dx==w);
        }
      }
    }
  }
*/
}

// Return an uncompressed bit
inline int Encoder::decode() {
  // Read block header
  while( n <= 0 ) {
    if( x != 0 || n < 0 ) {
      printf( "Archive error: x=%03X n=%d at %ld\n", x, n, ftell( archive ) );
      exit( 1 );
    }
    x = getc( archive );
    x = x * 256 + getc( archive );
    n = getc( archive );
    n = n * 256 + getc( archive );
    n = n * 256 + getc( archive );
    if( x < 1 << N || x >= 2 << N || n < 0 || n > BO ) {
      printf( "Archive error: x=%d (%d-%d) n=%d (0-%d) at %ld\n", x, 1 << N, ( 2 << N ) - 1, n, BO, ftell( archive ) );
      exit( 1 );
    }
    x -= 1 << N;
    ch = getc( archive );
  }

  // Decode
  int r = qr[predictor.p()];
  x = dec[r][x];
  int d = static_cast<int>( x < 0 );
  predictor.update( d );
  x &= 0x7fff;
  while( x < 1 << N ) {
    if( ch == 1 )
      ch = getc( archive ) + 256;
    x += x + ( ch & 1 );
    ch >>= 1;
  }
  --n;
  x -= 1 << N;
  return d;
}

// Compress bit d
inline void Encoder::encode( int d ) {
  if( n > B - 17 )
    flush();
  ins[n++] = qr[predictor.p()] + d;
  predictor.update( d );
}

// Compress pending data in input stack
void Encoder::flush() {
  int no = 0;                       // output stack size
  int w = 0;                        // encoder state
  unsigned char *p = &outs[BO / 8]; // output stack pointer

  // Encode input stack to output stack
  int ni = n;
  while( n != 0 ) {
    --n;
    int kx = enc[ins[n]][w];
    for( ; kx >= 4096; kx -= 4096 ) {
      if( ( no & 7 ) == 0 )
        *--p = 1;
      assert( p >= outs + 5 && p < outs + BO / 8 );
      *p += *p + ( w & 1 );
      w >>= 1;
      ++no;
    }
    w = kx;
  }
  if( ( no & 7 ) == 0 )
    *--p = 1;

  // Append w, ni to output stack and write
  w += 1 << N;
  *--p = ni;
  *--p = ni >> 8;
  *--p = ni >> 16;
  *--p = w;
  *--p = w >> 8;
  assert( p + no / 8 + 6 == outs + BO / 8 );
  fwrite( p, 1, no / 8 + 6, archive );
}

#endif

//////////////////////////// main ////////////////////////////

int main( int argc, char **argv ) {
  // Check arguments: fpaqa c/d input output
  if( argc != 4 || ( argv[1][0] != 'c' && argv[1][0] != 'd' ) ) {
    printf( "To compress:   fpaqa c input output\n"
            "To decompress: fpaqa d input output\n" );
    exit( 1 );
  }

  // Start timer
  clock_t start = clock();

  // Open files
  FILE *in = fopen( argv[2], "rbe" );
  if( in == nullptr )
    perror( argv[2] ), exit( 1 );
  FILE *out = fopen( argv[3], "wbe" );
  if( out == nullptr )
    perror( argv[3] ), exit( 1 );
  int c;

  // Compress
  if( argv[1][0] == 'c' ) {
    Encoder e( COMPRESS, out );
    while( ( c = getc( in ) ) != EOF ) {
      e.encode( 0 );
      for( int i = 7; i >= 0; --i )
        e.encode( ( c >> i ) & 1 );
    }
    e.encode( 1 ); // EOF code
    e.flush();
  }

  // Decompress
  else {
    Encoder e( DECOMPRESS, in );
    while( e.decode() == 0 ) {
      int c = 1;
      while( c < 256 )
        c += c + e.decode();
      putc( c - 256, out );
    }
  }

  // Print results
  printf( "%s (%ld bytes) -> %s (%ld bytes) in %1.2f s.\n", argv[2], ftell( in ), argv[3], ftell( out ),
          ( ( double ) clock() - start ) / CLOCKS_PER_SEC );
  return 0;
}
