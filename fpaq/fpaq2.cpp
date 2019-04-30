// fpaq2 - Stationary order 0 file compressor.
// (C) 2004, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt
// To compile: gcc -O fpaq0s.cpp -o fpaq0s.exe
// 10/01/2006 32 bit encoder modified, Fabio Buffoni
// jan 16 2006 improved compression, David Scott
// 20.10.2006 modified by Nania Francesco Antonio (Italia - Merì (ME))
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

int EOS = 0; /* for terminating compression */

unsigned long filesize, pos, control, i, lbit, posix, limbuf;

unsigned long posi, rposi, scota, conta, rp[2]; // Context: last 0-8 bits with a leading 1
unsigned char *buffer;
unsigned char ry[256], pr[2], numax[2]; // 0 and 1 counts in context cxt

unsigned long Peekl( unsigned char *memory ) {
  unsigned long result;
  memcpy( &result, memory, 4 );
  return ( result );
}

void Pokel( unsigned char *memory, unsigned long value ) {
  memcpy( memory, &value, 4 );
}

unsigned char Peekb( unsigned char *memory ) {
  unsigned char result;
  memcpy( &result, memory, 1 );
  return ( result );
}
void Pokeb( unsigned char *memory, unsigned char value ) {
  memcpy( memory, &value, 1 );
}

// Assume a stationary order 0 stream of 9-bit symbols
int p() {
  return 4096 * ( pr[1] + 1 ) / ( pr[0] + pr[1] + 2 );
}

void update( int y ) {
  if( Peekb( buffer + rp[y] ) + 1 < 255 )
    Pokeb( buffer + rp[y], Peekb( buffer + rp[y] ) + 1 );

  Pokeb( buffer + rp[1 - y], Peekb( buffer + rp[1 - y] ) >> 1 );

  lbit = 128;
  scota = 0;
  pr[0] = 1;
  pr[1] = 1;
  numax[0] = 0;
  numax[1] = 0;

  for( i = 0; i < lbit; ++i ) {
    posi = scota * 10;
    posix = posi + ( ry[i] << 2 );
    if( Peekl( buffer + posix ) == 0 ) {
      if( ( conta + 2 ) * 10 > limbuf )
        break;
      conta++;
      Pokel( buffer + posix, conta );
      Pokeb( buffer + posi + 8, pr[0] );
      Pokeb( buffer + posi + 9, pr[1] );
      break;
    }
    scota = Peekl( buffer + posix );
    pr[0] = Peekb( buffer + posi + 8 );
    pr[1] = Peekb( buffer + posi + 9 );

    if( pr[y] > pr[1 - y] )
      numax[y]++;
    else
      numax[y] = 0;

    if( numax[y] > 23 )
      break;
  }

  for( i = ( lbit - 1 ); i >= 1; --i )
    ry[i] = ry[i - 1];
  ry[0] = y;

  scota = 0;
  pr[0] = 1;
  pr[1] = 1;
  for( i = 0; i < lbit; ++i ) {
    posi = scota * 10;
    if( Peekl( buffer + posi + ( ry[i] << 2 ) ) == 0 ) {
      break;
    }
    scota = Peekl( buffer + posi + ( ry[i] << 2 ) );
    rposi = posi;
  }
  pr[0] = Peekb( buffer + rposi + 8 );
  pr[1] = Peekb( buffer + rposi + 9 );
  rp[0] = rposi + 8;
  rp[1] = rposi + 9;
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
    pos++;
    bin = getc( archive );
    if( bin == EOF ) {
      bin = 0;
      EOS++;
    }
    bptrin = 128;
  }
  return static_cast<int>( ( bin & bptrin ) != 0 );
}

// Constructor
Encoder::Encoder( Mode m, FILE *f ) :
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
  unsigned int xdiff = x2 - x1;
  unsigned int xmid = x1;

  if( xdiff >= 0x4000000 )
    xmid += ( xdiff >> 12 ) * p();
  else if( xdiff >= 0x100000 )
    xmid += ( ( xdiff >> 6 ) * p() ) >> 6;
  else
    xmid += ( xdiff * p() ) >> 12;

  assert( xmid >= x1 && xmid < x2 );
  if( y != 0 )
    x2 = xmid;
  else
    x1 = xmid + 1;
  update( y );

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

  unsigned int xdiff = x2 - x1;
  unsigned int xmid = x1;

  if( xdiff >= 0x4000000 )
    xmid += ( xdiff >> 12 ) * p();
  else if( xdiff >= 0x100000 )
    xmid += ( ( xdiff >> 6 ) * p() ) >> 6;
  else
    xmid += ( xdiff * p() ) >> 12;

  assert( xmid >= x1 && xmid < x2 );
  int y = 0;
  if( x <= xmid ) {
    y = 1;
    x2 = xmid;
  } else
    x1 = xmid + 1;
  update( y );

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
    if( EOS > 6 || ( EOS > 0 && ( x << 2 ) == 0 ) ) {
      EOS = 100;
      if( x == Half || x == 0 )
        EOS = 10;
    }
  }
  return y;
}

// Should be called when there is no more to compress
void Encoder::flush() {
  // Update the range
  const U32 xmid = x1 + ( ( x2 - x1 ) >> 12 ) * p();
  assert( xmid >= x1 && xmid < x2 );
  if( xmid < Half )
    x2 = xmid;
  else
    x1 = xmid + 1;
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
  if( x1 <= First_qtr ) {
    bit_plus_follow( 0 );
    bit_plus_follow( 1 );
  } else {
    bit_plus_follow( 1 );
    bit_plus_follow( 1 );
  }
  if( bout != 0u )
    putc( bout, archive );
}

//////////////////////////// main ////////////////////////////

int main( int argc, char **argv ) {
  // Chech arguments: fpaq0 c/d input output
  if( argc != 4 || ( argv[1][0] != 'c' && argv[1][0] != 'd' ) ) {
    printf( "To compress:   fpaq0s c input output\n"
            "To decompress: fpaq0s d input output\n" );
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
  buffer = ( unsigned char * ) malloc( 1 << 27 );
  fseek( in, 0, SEEK_END );
  filesize = ftell( in );
  rewind( in );
  limbuf = ( 1 << 27 ) - 1;
  // Compress
  if( argv[1][0] == 'c' ) {
    Encoder e( COMPRESS, out );
    while( ( c = getc( in ) ) != EOF ) {
      for( int i = 7; i >= 0; --i )
        e.encode( ( c >> i ) & 1 );
      pos++;
      if( pos > control * ( filesize / 79 ) ) {
        printf( ">" );
        control++;
      }
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
        if( EOS > 5 )
          break;
      }
      if( EOS > 5 )
        break;
      putc( c - 256, out );

      if( pos > control * ( filesize / 79 ) ) {
        printf( ">" );
        control++;
      }
    }
    if( EOS != 100 || c > 3 ) {
      printf( " *** bad input file on decompress *** \n" );
      putc( 5, out ); /* just to mark file bad can be dropped */
      putc( 5, out ); /* just to mark file bad can be dropped */
    } else
      printf( " ** successful most likely error free decompression ** \n" );
  }

  // Print results
  printf( "%s (%ld bytes) -> %s (%ld bytes) in %1.2f s.\n", argv[2], ftell( in ), argv[3], ftell( out ),
          ( ( double ) clock() - start ) / CLOCKS_PER_SEC );
  return 0;
}
