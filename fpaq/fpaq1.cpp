// fpaq1 - Stationary order 0 file compressor using 64 bit arithmetic coding.
// (C) 2006, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt
// To compile: g++ -O2 fpaq1.cpp

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
namespace std {} // namespace std
using namespace std;

using U32 = unsigned int; // 32, 64 bit unsigned integer types
using U64 = unsigned long long;

//////////////////////////// Predictor /////////////////////////

/* A Predictor estimates the probability that the next bit of
   uncompressed data is 1.  Methods:
   p() returns P(1) * 2^32.
   update(y) trains the predictor with the actual bit (0 or 1).
*/

class Predictor {
  int cxt{ 1 };   // Context: last 0-8 bits with a leading 1
  int ct[512][2]; // 0 and 1 counts in context cxt
public:
  Predictor() {
    memset( ct, 0, sizeof( ct ) );
  }

  // Assume a stationary order 0 stream of 9-bit symbols
  U32 p() const {
    return ( U64( ct[cxt][1] + 1 ) << 32 ) / ( ct[cxt][0] + ct[cxt][1] + 2 );
  }

  void update( int y ) {
    if( ++ct[cxt][y] > 2000000000 ) {
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
   code(bit) in COMPRESS mode compresses bit to file f.
   code() in DECOMPRESS mode returns the next decompressed bit from file f.
   flush() should be called when there is no more to compress.
*/

typedef enum { COMPRESS, DECOMPRESS } Mode;
class Encoder {
private:
  Predictor predictor;
  const Mode mode;  // Compress or decompress?
  FILE *archive;    // Compressed data file
  U64 low, high, x; // Range, initially [0, 1), and archive
public:
  Encoder( Mode m, FILE *f );
  int code( int y ); // Compress bit y or return decompressed bit
  void flush();      // Call when done compressing
};

// Constructor
Encoder::Encoder( Mode m, FILE *f ) :

    mode( m ), archive( f ), low( 0 ), high( 0xffffffffffffffffLL ), x( 0 ) {
  // In DECOMPRESS mode, initialize x to the first 6 bytes of the archive
  if( mode == DECOMPRESS ) {
    for( int i = 0; i < 8; ++i ) {
      int c = getc( archive );
      if( c == EOF )
        c = 0;
      x = x << 8 | c;
    }
  }
}

/* encode(y) -- Encode bit y by splitting the range [x1, x2] in proportion
to P(1) and P(0) as given by the predictor and narrowing to the appropriate
subrange.  Output leading bytes of the range as they become known. */

inline int Encoder::code( int y = 0 ) {
  // Update the range
  U32 p = predictor.p();
  U64 mid = low + ( ( high - low ) >> 32 ) * p;
  assert( mid >= low && mid < high );
  if( mode == DECOMPRESS )
    y = static_cast<int>( x <= mid );
  if( y != 0 )
    high = mid;
  else
    low = mid + 1;
  predictor.update( y );

  // Shift equal MSB's out
  while( ( ( low ^ high ) & 0xff00000000000000LL ) == 0 ) {
    if( mode == COMPRESS )
      putc( low >> 56, archive );
    low = low << 8;
    high = high << 8 | 0xff;
    if( mode == DECOMPRESS ) {
      int c = getc( archive );
      if( c == EOF )
        c = 0;
      x = x << 8 | c;
    }
  }
  return y;
}

// Should be called when there is no more to compress
void Encoder::flush() {
  putc( high >> 56, archive ); // First unequal byte
}

//////////////////////////// main ////////////////////////////

int main( int argc, char **argv ) {
  // Chech arguments: fpaq1 c/d input output
  if( argc != 4 || ( argv[1][0] != 'c' && argv[1][0] != 'd' ) ) {
    printf( "To compress:   fpaq1 c input output\n"
            "To decompress: fpaq1 d input output\n" );
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
      e.code( 0 );
      for( int i = 7; i >= 0; --i )
        e.code( ( c >> i ) & 1 );
    }
    e.code( 1 ); // EOF code
    e.flush();
  }

  // Decompress
  else {
    Encoder e( DECOMPRESS, in );
    while( e.code() == 0 ) {
      int c = 1;
      while( c < 256 )
        c += c + e.code();
      putc( c - 256, out );
    }
  }

  // Print results
  printf( "%s (%ld bytes) -> %s (%ld bytes) in %1.2f s.\n", argv[2], ftell( in ), argv[3], ftell( out ),
          ( ( double ) clock() - start ) / CLOCKS_PER_SEC );
  return 0;
}
