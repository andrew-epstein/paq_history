// fpaq0x1b - Stationary order 0 file compressor.
// (C) 2004, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt
// 06.10.2006 Modified by Francesco Antonio Nania ( Italia - Sicilia - Merì (ME) )
// To compile: g++ -O fpaq0.cpp

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
namespace std {} // namespace std
using namespace std;

using U32 = unsigned int; // 32 bit type
U32 rc, r1, r2, r3, rc1, rc2, rc3, buf[16843009], cont, limi, ruc, c;
unsigned char cbuf[512000];
int cxt;                           // Context: last 0-8 bits with a leading 1
unsigned short ct[512000][512][2]; // 0 and 1 counts in context cxt
                                   //////////////////////////// Predictor /////////////////////////

/* A Predictor estimates the probability that the next bit of
   uncompressed data is 1.  Methods:
   p() returns P(1) as a 12 bit number (0-4095).
   update(y) trains the predictor with the actual bit (0 or 1).
*/

void counter() {
  for( ;; ) {
    cont++;
    if( cont > limi - 1 )
      cont = 0;
    if( cbuf[cont] == 0 )
      break;
  }
}
// Assume a stationary order 0 stream of 9-bit symbols
int p() {
  ruc = buf[rc];
  return 4096 * ( ct[ruc][cxt][1] + 1 ) / ( ct[ruc][cxt][0] + ct[ruc][cxt][1] + 2 );
}
void control( unsigned long ruc, unsigned char y ) {
  ct[ruc][cxt][1 - y] >>= 1; // predictor modified //
  if( ++ct[ruc][cxt][y] > 65535 ) {
    ct[ruc][cxt][y] >>= 1;
  }
}

void update( int y ) {
  ruc = buf[rc]; // oX model in action //
  control( buf[rc], y );

  if( rc != rc2 ) // o2 bytes model //
  {
    control( buf[rc2], y );
  }

  if( rc != rc3 ) // o3 bytes model //
  {
    control( buf[rc3], y );
  }

  if( ( cxt += cxt + y ) >= 512 )
    cxt = 1;
}

/// change frequency ////
void change( int c ) {
  r3 = r2;
  r2 = r1;
  r1 = c;
  rc = 0; // detect o0 model //
  rc1 = 1 + r1;
  rc2 = 257 + r1 + ( r2 << 8 );
  rc3 = 65793 + r1 + ( r2 << 8 ) + ( r3 << 16 );
  if( buf[rc1] == 0 ) {
    counter();
    for( int i = 1; i < 513; i++ ) {
      ct[cont][i][0] = ct[buf[0]][i][0]; // clone model //
      ct[cont][i][1] = ct[buf[0]][i][1]; // clone model //
    }
    buf[rc1] = cont;
    cbuf[cont] = 1;
  } else {
    rc = rc1; // detect o1 model //
  }
  if( buf[rc2] == 0 ) {
    counter();
    for( int i = 1; i < 513; i++ ) {
      ct[cont][i][0] = ct[buf[rc1]][i][0]; // clone model //
      ct[cont][i][1] = ct[buf[rc1]][i][1]; // clone model //
    }
    buf[rc2] = cont;
  } else {
    rc = rc2; // detect o2 model //
  }

  if( buf[rc3] == 0 ) {
    counter();
    for( int i = 1; i < 513; i++ ) {
      ct[cont][i][0] = ct[buf[rc2]][i][0]; // clone model //
      ct[cont][i][1] = ct[buf[rc2]][i][1]; // clone model //
    }
    buf[rc3] = cont;
    cbuf[buf[rc2]]++;
    if( cbuf[buf[rc2]] == 256 )
      cbuf[buf[rc2]] = 0;
  } else {
    rc = rc3; // detect o3 model //
  }
}

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
    printf( "To compress:   fpaq0 c input output\n"
            "To decompress: fpaq0 d input output\n" );
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
  limi = 512000;
  cxt = 1;
  memset( ct, 0, sizeof( ct ) );
  memset( buf, 0, sizeof( buf ) );
  memset( cbuf, 0, sizeof( cbuf ) );
  cont = 0;
  cbuf[0] = 1;
  // Compress
  if( argv[1][0] == 'c' ) {
    Encoder e( COMPRESS, out );
    while( ( c = getc( in ) ) != EOF ) {
      e.encode( 0 );
      for( int i = 7; i >= 0; --i )
        e.encode( ( c >> i ) & 1 );
      change( c ); // cahange model o0 - o1 -o2 - o3//
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
      putc( c - 256, out ); // cahange model o0 - o1 -o2 - o3//
      change( c - 256 );
    }
  }

  // Print results
  printf( "%s (%ld bytes) -> %s (%ld bytes) in %1.2f s.\n", argv[2], ftell( in ), argv[3], ftell( out ),
          ( ( double ) clock() - start ) / CLOCKS_PER_SEC );
  return 0;
}
