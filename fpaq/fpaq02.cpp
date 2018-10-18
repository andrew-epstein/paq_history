// fpaq0b - Stationary order 0 file compressor.
// (C) 2004, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt
// To compile: gcc -O fpaq0s.cpp -o fpaq0s.exe
// 10/01/2006 32 bit encoder modified, Fabio Buffoni
// jan 16 2006 improved compression, David Scott

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cassert>
namespace std {} // namespace std
using namespace std;

typedef unsigned char U8;
typedef unsigned int U32;
typedef unsigned long long U64;

#define LOWEST 0x0000000000000000LL
#define FIRST_QUARTER 0x4000000000000000LL
#define HALF 0x8000000000000000LL
#define THIRD_QUARTER 0xC000000000000000LL
#define HIGHEST 0xFFFFFFFFFFFFFFFFLL

int EOS = 0; /* for terminating compression */

//////////////////////////// Predictor /////////////////////////

/* A Predictor estimates the probability that the next bit of
   uncompressed data is 1 using the fraction numerator/denominator
*/

class Predictor {
  U32 cxt;        // Context: last 0-8 bits with a leading 1
  U32 ct[256][2]; // 0 and 1 counts in context cxt
public:
  Predictor() : cxt( 1 ) {
    memset( ct, 0, sizeof( ct ) );
  }

  // Assume a stationary order 0 stream of 9-bit symbols
  void p( U32 &numerator, U32 &denominator ) const {
    numerator = ct[cxt][1] + 1;
    denominator = ct[cxt][0] + ct[cxt][1] + 2;
  }

  void update( int y ) {
    if( ++ct[cxt][y] == 0xFFFFFFFFL ) {
      ct[cxt][0] >>= 1;
      ct[cxt][1] >>= 1;
    }
    cxt = ( cxt << 1 ) | y;
    if( cxt > 255 )
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
  const Mode mode;  // Compress or decompress?
  FILE *archive;    // Compressed data file
  U64 lower, upper; // Range, initially [0, 1)
  U64 x;            // Last 8 input bytes of archive.
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
    if( bit )
      bout |= bptr;
    if( !( bptr >>= 1 ) ) {
      putc( bout, archive );
      bptr = 128;
      bout = 0;
    }
  }
}
inline int Encoder::input_bit( void ) {
  if( !( bptrin >>= 1 ) ) {
    bin = getc( archive );
    if( bin == EOF ) {
      bin = 0;
      EOS++;
    }
    bptrin = 128;
  }
  return ( ( bin & bptrin ) != 0 );
}

// Constructor
Encoder::Encoder( Mode m, FILE *f ) :
    predictor(),
    mode( m ),
    archive( f ),
    lower( LOWEST ),
    upper( HIGHEST ),
    x( LOWEST ),
    bits_to_follow( 0 ),
    bptr( 128 ),
    bout( 0 ),
    bptrin( 1 ) {
  // In DECOMPRESS mode, initialize x to the first 8 bytes of the archive
  if( mode == DECOMPRESS ) {
    for( int i = 0; i < 64; i++ ) {
      x = ( x << 1 ) | input_bit();
    }
  }
}

/* encode(y) -- Encode bit y by splitting the range [lower, upper] in proportion
to P(1) and P(0) as given by the predictor and narrowing to the appropriate
subrange.  Output leading bytes of the range as they become known. */

inline void Encoder::encode( int y ) {
  U32 numerator, denominator;
  predictor.p( numerator, denominator );

  // Update the range. Compute the fraction so that it's exact to 64 bits
  U64 width = upper - lower;
  U64 quotient = width / denominator;
  U32 remainder = width % denominator;
  U64 xmid = lower + quotient * numerator;
  quotient = remainder * numerator;
  xmid += quotient / denominator;

  assert( xmid >= lower && xmid < upper );
  if( y )
    upper = xmid;
  else
    lower = xmid + 1;
  predictor.update( y );

  // Shift equal MSB's out
  for( ;; ) {
    if( upper < HALF ) {
      bit_plus_follow( 0 );
    } else if( lower >= HALF ) {
      bit_plus_follow( 1 );
    } else if( lower >= FIRST_QUARTER && upper < THIRD_QUARTER ) {
      bits_to_follow++;
      upper -= FIRST_QUARTER;
      lower -= FIRST_QUARTER;
    } else {
      break;
    }
    upper = ( upper << 1 ) + 1;
    lower = ( lower << 1 );
  }
}

/* Decode one bit from the archive, splitting [lower, upper] as in the encoder
and returning 1 or 0 depending on which subrange the archive point x is in.
*/
inline int Encoder::decode() {
  U32 numerator, denominator;
  predictor.p( numerator, denominator );

  // Update the range. Compute the fraction so that it's exact to 64 bits
  U64 width = upper - lower;
  U64 quotient = width / denominator;
  U32 remainder = width % denominator;
  U64 xmid = lower + quotient * numerator;
  quotient = remainder * numerator;
  xmid += quotient / denominator;

  assert( xmid >= lower && xmid < upper );
  int y = 0;
  if( x <= xmid ) {
    y = 1;
    upper = xmid;
  } else
    lower = xmid + 1;
  predictor.update( y );

  // Shift equal MSB's out
  for( ;; ) {
    if( upper < HALF ) {
    } else if( lower >= HALF ) {
    } else if( lower >= FIRST_QUARTER && upper < THIRD_QUARTER ) {
      upper -= FIRST_QUARTER;
      x -= FIRST_QUARTER;
      lower -= FIRST_QUARTER;
    } else {
      break; /* Otherwise exit loop.     */
    }
    upper = ( upper << 1 ) | 1; /* Scale up code range.     */
    x = ( x << 1 ) | input_bit();
    lower = ( lower << 1 );
    /*
      if( EOS > 0 ) {
        printf( "decode() EOS=%d x=0x%08x%08x\n", 
          EOS,
          (unsigned int)(x >> 32),
          (unsigned int)(x & 0xFFFFFFFF) );
      }
      */
    // keep reading until the last two bits.
    if( EOS > 0 && ( ( x << 2 ) == 0 ) ) {
      EOS = 100;
      if( x == HALF || x == 0 )
        EOS = 14; // stop compression but signal error
    }
  }
  return y;
}

// Should be called when there is no more to compress
void Encoder::flush() {
  U32 numerator, denominator;
  predictor.p( numerator, denominator );

  // Update the range. Compute the fraction so that it's exact to 64 bits
  U64 width = upper - lower;
  U64 quotient = width / denominator;
  U32 remainder = width % denominator;
  U64 xmid = lower + quotient * numerator;
  quotient = remainder * numerator;
  xmid += quotient / denominator;

  assert( xmid >= lower && xmid < upper );
  if( xmid < HALF )
    upper = xmid;
  else
    lower = xmid + 1;
  // Shift equal MSB's out
  for( ;; ) {
    if( upper < HALF ) {
      bit_plus_follow( 0 );
    } else if( lower >= HALF ) {
      bit_plus_follow( 1 );
    } else if( lower >= FIRST_QUARTER && upper < THIRD_QUARTER ) {
      bits_to_follow++;
      upper -= FIRST_QUARTER;
      lower -= FIRST_QUARTER;
    } else {
      break;
    }
    upper = ( upper << 1 ) | 1;
    lower = ( lower << 1 );
  }
  if( lower <= FIRST_QUARTER ) {
    bit_plus_follow( 0 );
    bit_plus_follow( 1 );
  } else {
    bit_plus_follow( 1 );
    bit_plus_follow( 1 );
  }
  if( bout )
    putc( bout, archive );
}

//////////////////////////// main ////////////////////////////

int main( int argc, char **argv ) {
  // Chech arguments: fpaq0 c/d input output
  if( argc != 4 || ( argv[1][0] != 'c' && argv[1][0] != 'd' ) ) {
    printf( "To compress:   %s c input output\n"
            "To decompress: %s d input output\n",
            argv[0], argv[0] );
    exit( 1 );
  }

  // Start timer
  clock_t start = clock();

  // Open files
  FILE *in = fopen( argv[2], "rb" );
  if( !in )
    perror( argv[2] ), exit( 1 );
  FILE *out = fopen( argv[3], "wb" );
  if( !out )
    perror( argv[3] ), exit( 1 );
  int c;

  // Compress
  if( argv[1][0] == 'c' ) {
    Encoder e( COMPRESS, out );
    while( ( c = getc( in ) ) != EOF ) {
      for( int i = 7; i >= 0; --i )
        e.encode( ( c >> i ) & 1 );
    }
    e.flush();
  }

  // Decompress
  else {
    Encoder e( DECOMPRESS, in );
    int c = 1;
    for( ;; ) {
      c = 1;
      while( c < 256 ) {
        c += c + e.decode();
        //once the EOF is reached, one EOS++ for that, and 7 more null byte reads until
        //the 64-bit buffer is down to it's last byte. If EOS is 9 then the buffer is
        //totally empty
        if( EOS > 8 )
          break;
      }
      if( EOS > 8 )
        break;
      putc( c - 256, out );
    }
    if( EOS != 100 || c > 3 ) {
      printf( " *** bad input file on decompress *** (EOS=%d, c=%d)\n", EOS, c );
      //putc ( 5 , out ); /* just to mark file bad can be dropped */
      //putc ( 5 , out ); /* just to mark file bad can be dropped */
    } else
      printf( " ** successful most likely error free decompression ** \n" );
  }

  // Print results
  printf( "%s (%ld bytes) -> %s (%ld bytes) in %1.2f s.\n", argv[2], ftell( in ), argv[3], ftell( out ),
          ( ( double ) clock() - start ) / CLOCKS_PER_SEC );
  return 0;
}
