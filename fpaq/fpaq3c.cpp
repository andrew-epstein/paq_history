// fpaq3c - Stationary order 028b file compressor.
// (C) 2004, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt
// To compile: g++ -O fpaq0.cpp
// 21/12/2006 modified by Nania Francesco Antonio (Italia - Merì (ME))
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cassert>
namespace std {} // namespace std
using namespace std;

typedef unsigned int U32; // 32 bit type

unsigned long filesize, pos, control, maxsize, scanned;
unsigned long rc[2], cc[2];       // Context: last 0-8 bits with a leading 1
unsigned char buf[1 << 28], epix; // 0 and 1 state count in context
unsigned char cuf[1 << 23];
unsigned char *buffer;
// Assume a stationary order 3 stream of 9-bit symbols
unsigned char Peekb( unsigned char *memory ) {
  unsigned char result;
  memcpy( &result, memory, 1 );
  return ( result );
}
void Pokeb( unsigned char *memory, unsigned char value ) {
  memcpy( memory, &value, 1 );
}
int p() {
  if( epix == 7 )
    return 4096 * ( cuf[cc[1]] + 1 ) / ( cuf[cc[1]] + cuf[cc[0]] + 2 );
  else
    return 4096 * ( buf[rc[1]] + 1 ) / ( buf[rc[1]] + buf[rc[0]] + 2 );
}

void update( int y ) {
  if( epix < 7 ) {
    buf[rc[y]] += 8;
    if( buf[rc[y]] > 247 ) {
      buf[rc[y]] >>= 1;
      buf[rc[y == 0]] >>= 1;
    }
    rc[y] >>= 1;          //
    rc[y] += ( 1 << 27 ); // old 28 state of     y [0001...................01] count
    rc[y == 0] >>= 1;     // old 28 state of not y [1110...................10] count
  } else {
    cuf[cc[y]] += 8;
    if( cuf[cc[y]] > 247 ) {
      cuf[cc[y]] >>= 1;
      cuf[cc[y == 0]] >>= 1;
    }
    cc[y] >>= 1;          //
    cc[y] += ( 1 << 22 ); // old 28 state of     y [0001...................01] count
    cc[y == 0] >>= 1;     // old 28 state of not y [1110...................10] count
  }
}

//////////////////////////// Encoder ////////////////////////////

/* An Encoder does arithmetic encoding.  Methods:
   Encoder(COMPRESS, f) creates encoder for compression to archive f, which
     must be open past any header for writing in binary mode
   Encoder(DECOMPRESS, f) creates encoder for decompression from archive f,
     which must be open past any header for reading in binary mode
   encode(bit) in COMPRESS mode compresses bit to file f.
   decode() in DECOMPRESS mode returns the next decompressed bit from file
f.
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
  Encoder( Mode m, FILE *f );
  void encode( int y ); // Compress bit y
  int decode();         // Uncompress and return bit y
  void flush();         // Call when done compressing
};

// Constructor
Encoder::Encoder( Mode m, FILE *f ) : mode( m ), archive( f ), x1( 0 ), x2( 0xffffffff ), x( 0 ) {
  // In DECOMPRESS mode, initialize x to the first 4 bytes of the archive
  if( mode == DECOMPRESS ) {
    for( int i = 0; i < 4; ++i ) {
      int c = getc( archive );
      if( c == EOF )
        c = 0;
      x = ( x << 8 ) + ( c & 0xff );
    }
  }
}

/* encode(y) -- Encode bit y by splitting the range [x1, x2] in proportion
to P(1) and P(0) as given by the predictor and narrowing to the appropriate
subrange.  Output leading bytes of the range as they become known. */

inline void Encoder::encode( int y ) {
  // Update the range
  const U32 xmid = x1 + ( ( x2 - x1 ) >> 12 ) * p();
  assert( xmid >= x1 && xmid < x2 );
  if( y != 0 )
    x2 = xmid;
  else
    x1 = xmid + 1;

  update( y );

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
  const U32 xmid = x1 + ( ( x2 - x1 ) >> 12 ) * p();
  assert( xmid >= x1 && xmid < x2 );
  int y = 0;
  if( x <= xmid ) {
    y = 1;
    x2 = xmid;
  } else
    x1 = xmid + 1;

  update( y );

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
  // In COMPRESS mode, write out the remaining bytes of x, x1 < x < x2
  if( mode == COMPRESS ) {
    while( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 ) {
      putc( x2 >> 24, archive );
      x1 <<= 8;
      x2 = ( x2 << 8 ) + 255;
    }
    putc( x2 >> 24, archive ); // First unequal byte
  }
}

//////////////////////////// main ////////////////////////////

int main( int argc, char **argv ) {
  // Chech arguments: fpaq0 c/d input output
  if( argc != 4 || ( argv[1][0] != 'c' && argv[1][0] != 'd' ) ) {
    printf( "To compress:   fpaq3c c input output\n"
            "To decompress: fpaq3c d input output\n" );
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
  maxsize = 1 << 20;
  buffer = ( unsigned char * ) malloc( maxsize );
  memset( buf, 0, sizeof( buf ) );
  // Compress
  if( argv[1][0] == 'c' ) {
    fseek( in, 0, SEEK_END );
    filesize = ftell( in );
    rewind( in );
    fseek( out, 0, SEEK_SET );
    fwrite( &filesize, 1, 4, out );
    Encoder e( COMPRESS, out );
  newscan:
    if( scanned + maxsize > filesize )
      maxsize = filesize - scanned;
    pos = 0;
    fread( buffer, 1, maxsize, in );
    while( pos < maxsize ) {
      c = Peekb( buffer + pos );
      for( int i = 7; i >= 0; --i ) {
        epix = i;
        e.encode( ( c >> i ) & 1 );
      }
      pos++;
      if( scanned + pos > control * ( filesize / 79 ) ) {
        printf( ">" );
        control++;
      }
    }
    scanned += maxsize;
    if( scanned < filesize )
      goto newscan;
    e.flush();
  }

  // Decompress
  else {
    fseek( in, 0, SEEK_SET );
    fread( &filesize, 1, 4, in );
    fseek( in, 4, SEEK_SET );
    Encoder e( DECOMPRESS, in );
  newdscan:
    if( scanned + maxsize > filesize )
      maxsize = filesize - scanned;
    pos = 0;
    while( pos < maxsize ) {
      c = 0;
      for( int i = 7; i >= 0; --i ) {
        epix = i;
        c += e.decode() * ( 1 << i );
      }
      Pokeb( buffer + pos, c );
      if( scanned + pos > control * ( filesize / 79 ) ) {
        printf( ">" );
        control++;
      }
      pos++;
    }
    fwrite( buffer, 1, maxsize, out );
    scanned += maxsize;
    if( scanned < filesize )
      goto newdscan;
  }

  // Print results
  printf( "%s (%ld bytes) -> %s (%ld bytes) in %1.2f s.\n", argv[2], ftell( in ), argv[3], ftell( out ),
          ( ( double ) clock() - start ) / CLOCKS_PER_SEC );
  return 0;
}
