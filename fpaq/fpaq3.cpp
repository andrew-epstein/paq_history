// fpaq3 - Stationary order 3 (27 bits) file compressor.
// (C) 2004, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt
// To compile: gcc -O fpaq3.cpp -o fpaq3.exe
// 10/01/2006 32 bit encoder modified, Fabio Buffoni
// jan 16 2006 improved compression, David Scott
// 20.11.2006 modified by Nania Francesco Antonio (Italia - Mer� (ME))
//
//  SFC TEST [MAXIMUM COMPRESSION]:
//  ________________________________
//  WORLD95.TXT   =    889.750 Bytes
//  FP.LOG        =  1.017.226 Bytes
//  ENGLISH.DIC   =    753.106 Bytes
//  ACRORD32.EXE  =  2.107.418 Bytes
//  MSO97.DLL     =  2.444.729 Bytes
//  RAFALE.BMP    =    969.524 Bytes
//  A10.JPG       =    845.419 Bytes
//  VCFIU.HLP     =  1.016.269 Bytes
//  OHS.DOC       =  1.031.211 Bytes
//  FLASHMX.PDF   =  3.849.108 Bytes
//  ________________________________
//  TOTAL         = 14.923.760 Bytes
///////////////////////////////////////////////////////////////////////////
//
//  Canterbury corpus concatenated (ISO) =   691.514 Bytes
//  Calgary corpus concatenated    (ISO) = 1.055.013 Bytes
///////////////////////////////////////////////////////////////////////////
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cassert>
namespace std {} // namespace std
using namespace std;

using U8 = unsigned char;
using U32 = unsigned int; // 32 bit type

#define Top_value U32( 0XFFFFFFFF ) /* Largest code value */
/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */
#define First_qtr U32( Top_value / 4 + 1 ) /* Point after first quarter    */
#define Half U32( 2 * First_qtr )          /* Point after first half  */
#define Third_qtr U32( 3 * First_qtr )     /* Point after third quarter */

int EOS = 0; /* for terminating compression */

unsigned long filesize, pos, control, rc0, rc1;
unsigned long rc;              // Context: last 0-8 bits with a leading 1
unsigned char buf[1 << 27][2]; // 0 and 1 counts in context

// Assume a stationary order 0 stream of 9-bit symbols
int p() {
  return 4096 * ( buf[rc][1] + 1 ) / ( buf[rc][1] + buf[rc][0] + 2 );
}

void update( int y ) {
  buf[rc][y == 0] >>= 1; // aggressive mode //
  if( ++buf[rc][y] > 254 ) {
    buf[rc][y] >>= 1;
    buf[rc][y == 0] >>= 1;
  }

  rc >>= 1;                  //
  rc += ( y * ( 1 << 26 ) ); // old 27 bits [0001...................01] swicth

  rc0 = rc;
  rc0 >>= 1;
  rc1 = rc0;
  rc1 += ( 1 << 26 );

  if( buf[rc0][0] == 0 && buf[rc0][1] == 0 ) //  verification and increases the probability of 1
    if( buf[rc1][0] > 0 || buf[rc1][1] > 0 )
      buf[rc][1] <<= 1; // [probalibity of 1 ]*2

  if( buf[rc1][0] == 0 && buf[rc1][1] == 0 ) // verification and increases the probability of 0
    if( buf[rc0][0] > 0 || buf[rc0][1] > 0 )
      buf[rc][0] <<= 1; // [probalibity of 0 ]*2
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
    if( ( bptr >>= 1 ) == 0U ) {
      putc( bout, archive );
      bptr = 128;
      bout = 0;
    }
  }
}
inline int Encoder::input_bit( void ) {
  if( ( bptrin >>= 1 ) == 0U ) {
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
  U32 xmid;
  xmid = x1 + ( ( x2 - x1 ) >> 12 ) * p();
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

  U32 xmid;
  xmid = x1 + ( ( x2 - x1 ) >> 12 ) * p();

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
  U32 xmid;
  xmid = x1 + ( ( x2 - x1 ) >> 12 ) * p();
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
  if( bout != 0U )
    putc( bout, archive );
}

//////////////////////////// main ////////////////////////////

int main( int argc, char **argv ) {
  // Chech arguments: fpaq3 c/d input output
  if( argc != 4 || ( argv[1][0] != 'c' && argv[1][0] != 'd' ) ) {
    printf( "To compress:   fpaq3 c input output\n"
            "To decompress: fpaq3 d input output\n" );
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

  fseek( in, 0, SEEK_END );
  filesize = ftell( in );
  rewind( in );
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
