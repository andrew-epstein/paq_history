// fpaq0r - Stationary order 0 file compressor.
// (C) 2004, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt
// To compile: g++ -O fpaq0r.cpp
// 15 April 2007 -- modified predictor to improve speed, Ilia Muraviev
// 9 January 2008 -- speed and compression improved, Alexander Ratushnyak

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cassert>
namespace std {} // namespace std
using namespace std;

using U32 = unsigned int; // 32 bit type

#define PSCALE 13

#ifdef SLOWER
#  define CALC_XMID x1 + ( ( x2 - x1 ) >> PSCALE ) * p + ( ( x2 - x1 & ( ( 1 << PSCALE ) - 1 ) ) * p >> PSCALE )
#else
#  define CALC_XMID x1 + ( ( x2 - x1 ) >> PSCALE ) * p
#endif

//////////////////////////// Predictor /////////////////////////

class Predictor {
private:
  int p[256]; // Probability of 1

public:
  int cxt{ 1 }; // Context: last 0-7 bits with a leading 1

  Predictor()  {
    for( int i = 0; i < 256; i++ )
      p[i] = 1 << ( PSCALE - 1 );
  }

  // Assume a stationary order 0 stream of 8-bit symbols
  int P() const {
    return p[cxt];
  }

  inline void update( int y ) {
    if( y != 0 ) {
      p[cxt] += ( ( ( 3 << PSCALE ) - 24 - p[cxt] * 3 ) >> 7 );
      cxt += cxt + 1;
    } else {
      p[cxt] -= ( ( p[cxt] * 3 + 48 ) >> 7 );
      cxt += cxt;
    }
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
  const Mode mode; // Compress or decompress?
  FILE *archive;   // Compressed data file
  U32 x1, x2;      // Range, initially [0, 1), scaled by 2^32
  U32 x;           // Last 4 input bytes of archive.
public:
  Predictor predictor;
  Encoder( Mode m, FILE *f );
  void encode( int y ); // Compress bit y
  int decode();         // Uncompress and return bit y
  void flush();         // Call when done compressing
};

// Constructor
Encoder::Encoder( Mode m, FILE *f ) : mode( m ), archive( f ), x1( 0 ), x2( 0xffffffff ), x( 0 ), predictor() {
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
  int p = predictor.P();
  const U32 xmid = CALC_XMID;
  assert( xmid >= x1 && xmid < x2 );
  if( y != 0 )
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
  int p = predictor.P();
  const U32 xmid = CALC_XMID;
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

//////////////////////////// main ////////////////////////////

int main( int argc, char **argv ) {
  // Check arguments: fpaq0r c/d input output
  if( argc != 4 || ( argv[1][0] != 'c' && argv[1][0] != 'd' ) ) {
    printf( "To compress:   fpaq0r c input output\n"
            "To decompress: fpaq0r d input output\n" );
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
  unsigned long len;

  // Compress
  if( argv[1][0] == 'c' ) {
    fseek( in, 0, SEEK_END );
    len = ftell( in );
    fseek( in, 0, SEEK_SET );
    fwrite( &len, sizeof( len ), 1, out );

    Encoder e( COMPRESS, out );
    int c;
    while( ( c = getc( in ) ) != EOF ) {
      for( int i = 7; i >= 0; --i )
        e.encode( ( c >> i ) & 1 );
      e.predictor.cxt = 1;
    }
    e.flush();
  }

  // Decompress
  else {
    fread( &len, sizeof( len ), 1, in );
    Encoder e( DECOMPRESS, in );
    while( ( len-- ) != 0U ) {
      int c = 1;
      while( c < 256 )
        c += c + e.decode();
      e.predictor.cxt = 1;
      putc( c - 256, out );
    }
  }

  // Print results
  printf( "%s (%ld bytes) -> %s (%ld bytes) in %1.2f s.\n", argv[2], ftell( in ), argv[3], ftell( out ),
          ( ( double ) clock() - start ) / CLOCKS_PER_SEC );
  return 0;
}
