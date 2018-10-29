// fpaq0b - Stationary order 0 file compressor.
// (C) 2004, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt
// To compile: g++ -O fpaq0.cpp
// 10/01/2006 32 bit encoder modified, Fabio Buffoni

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cassert>
namespace std {} // namespace std
using namespace std;

typedef unsigned char U8;
typedef unsigned int U32; // 32 bit type

#define Top_value U32( 0XFFFFFFFF ) /* Largest code value */
/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */
#define First_qtr U32( Top_value / 4 + 1 ) /* Point after first quarter    */
#define Half U32( 2 * First_qtr )          /* Point after first half  */
#define Third_qtr U32( 3 * First_qtr )     /* Point after third quarter */

//////////////////////////// Predictor /////////////////////////

/* A Predictor estimates the probability that the next bit of
   uncompressed data is 1.  Methods:
   p() returns P(1) as a 12 bit number (0-4095).
   update(y) trains the predictor with the actual bit (0 or 1).
*/

class Predictor {
  int cxt;        // Context: last 0-8 bits with a leading 1
  int ct[512][2]; // 0 and 1 counts in context cxt
public:
  Predictor() : cxt( 1 ) {
    memset( ct, 0, sizeof( ct ) );
  }

  // Assume a stationary order 0 stream of 9-bit symbols
  int p() const {
    return 4096 * ( ct[cxt][1] + 1 ) / ( ct[cxt][0] + ct[cxt][1] + 2 );
  }

  void update( int y ) {
    if( ++ct[cxt][y] > 65534 ) {
      ct[cxt][0] >>= 1;
      ct[cxt][1] >>= 1;
    }
    if( ( cxt += cxt + y ) >= 512 )
      cxt = 1;
  }
};

//////////////////////////// Encoder ////////////////////////////

/* An Encoder does arithmetic encoding.  Methods:
   Encoder(COMPRESS, f) creates encoder for compression to archive f, which
     must be open past any header for writing in binary mode
   Encoder(DECOMPRESS, f) creates encoder for decompression from archive f,
     which must be open past any header for reading in binary mode
   encode(bit) in COMPRESS mode compresses bit to file f.
   decode() in DECOMPRESS mode returns the next decompressed bit from file f.
   flush() should be called when there is no more to compress.
*/

typedef enum { COMPRESS, DECOMPRESS } Mode;
class Encoder {
private:
  Predictor predictor;
  const Mode mode; // Compress or decompress?
  FILE *archive;   // Compressed data file
  U32 x1, x2;      // Range, initially [0, 1), scaled by 2^32
  U32 x;           // Last 4 input bytes of archive.
  U32 bits_to_follow;
  U8 bptr, bout, bptrin;
  int bin;

public:
  Encoder( Mode m, FILE *f );
  void encode( int y ); // Compress bit y
  int decode();         // Uncompress and return bit y
  void flush();         // Call when done compressing
  void bit_plus_follow( int bit );
  int input_bit( void );
};

inline void Encoder::bit_plus_follow( int bit ) {
  bits_to_follow++;
  for( int notb = bit ^ 1; bits_to_follow > 0; bits_to_follow--, bit = notb ) {
    if( bit != 0 )
      bout |= bptr;
    if( ( bptr >>= 1 ) == 0u ) {
      putc( bout, archive );
      bptr = 128;
      bout = 0;
    }
  }
}
inline int Encoder::input_bit( void ) {
  if( ( bptrin >>= 1 ) == 0u ) {
    bin = getc( archive );
    if( bin == EOF )
      bin = 0;
    bptrin = 128;
  }
  return static_cast<int>( ( bin & bptrin ) != 0 );
}

// Constructor
Encoder::Encoder( Mode m, FILE *f ) :
    predictor(),
    mode( m ),
    archive( f ),
    x1( 0 ),
    x2( 0xffffffff ),
    x( 0 ),
    bits_to_follow( 0 ),
    bptr( 128 ),
    bout( 0 ),
    bptrin( 1 ) {
  // In DECOMPRESS mode, initialize x to the first 4 bytes of the archive
  if( mode == DECOMPRESS ) {
    x = 1;
    for( ; x < Half; )
      x += x + input_bit();
    x += x + input_bit();
  }
}

/* encode(y) -- Encode bit y by splitting the range [x1, x2] in proportion
to P(1) and P(0) as given by the predictor and narrowing to the appropriate
subrange.  Output leading bytes of the range as they become known. */

inline void Encoder::encode( int y ) {
  // Update the range
  const U32 xmid = x1 + ( ( x2 - x1 ) >> 12 ) * predictor.p();
  assert( xmid >= x1 && xmid < x2 );
  if( y != 0 )
    x2 = xmid;
  else
    x1 = xmid + 1;
  predictor.update( y );

  // Shift equal MSB's out
  for( ;; ) {
    if( x2 < Half ) {
      bit_plus_follow( 0 );
    } else if( x1 >= Half ) {
      bit_plus_follow( 1 );
    } else if( x1 >= First_qtr && x2 < Third_qtr ) {
      bits_to_follow++;
      x1 ^= First_qtr;
      x2 ^= First_qtr;
    } else {
      break;
    }
    x1 += x1;
    x2 += x2 + 1;
  }
}

/* Decode one bit from the archive, splitting [x1, x2] as in the encoder
and returning 1 or 0 depending on which subrange the archive point x is in.
*/
inline int Encoder::decode() {
  // Update the range
  const U32 xmid = x1 + ( ( x2 - x1 ) >> 12 ) * predictor.p();
  assert( xmid >= x1 && xmid < x2 );
  int y = 0;
  if( x <= xmid ) {
    y = 1;
    x2 = xmid;
  } else
    x1 = xmid + 1;
  predictor.update( y );

  // Shift equal MSB's out
  for( ;; ) {
    if( x2 < Half ) {
    } else if( x1 >= Half ) { /* Output 1 if in high half. */
      x1 -= Half;
      x -= Half;
      x2 -= Half;                    /* Subtract offset to top.  */
    } else if( x1 >= First_qtr       /* Output an opposite bit   */
               && x2 < Third_qtr ) { /* later if in middle half. */
      x1 -= First_qtr;               /* Subtract offset to middle */
      x -= First_qtr;
      x2 -= First_qtr;
    } else {
      break; /* Otherwise exit loop.     */
    }
    x1 += x1;
    x += x + input_bit();
    x2 += x2 + 1; /* Scale up code range.     */
  }
  return y;
}

// Should be called when there is no more to compress
void Encoder::flush() {
  // In COMPRESS mode, write out the remaining bytes of x, x1 < x < x2
  if( mode == COMPRESS ) {
    bits_to_follow = 0; //FB 10/01/2006
    if( x1 == 0 )
      bit_plus_follow( 0 );
    else
      bit_plus_follow( 1 );
    if( bout != 0u )
      putc( bout, archive );
  }
}

//////////////////////////// main ////////////////////////////

int main( int argc, char **argv ) {
  // Chech arguments: fpaq0 c/d input output
  if( argc != 4 || ( argv[1][0] != 'c' && argv[1][0] != 'd' ) ) {
    printf( "To compress:   fpaq0 c input output\n"
            "To decompress: fpaq0 d input output\n" );
    exit( 1 );
  }

  // Start timer
  clock_t start = clock();

  // Open files
  FILE *in = fopen( argv[2], "rb" );
  if( in == nullptr )
    perror( argv[2] ), exit( 1 );
  FILE *out = fopen( argv[3], "wb" );
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
