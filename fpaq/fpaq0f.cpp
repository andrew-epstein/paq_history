/* fpaq0f - Adaptive order 0 file compressor.
(C) 2008, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt

To compile:    g++ -O2 -s -fomit-frame-pointer -march=pentiumpro fpaq0f.cpp
To compress:   fpaq0f c input output
To decompress: fpaq0f d input output

fpaq0f is an order-0 file compressor and arithmetic coder.  Each bit is
modeled in the context of the previous bits in the current byte, plus
the bit history (as an 8 bit state) observed in this context.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 8, 16, 32 bit unsigned types (adjust as appropriate)
using U8 = unsigned char;
using U16 = unsigned short;
using U32 = unsigned int;

// Create an array p of n elements of type T
template <class T>
void alloc( T *&p, int n ) {
  p = ( T * ) calloc( n, sizeof( T ) );
  if( !p )
    fprintf( stderr, "out of memory\n" ), exit( 1 );
}

///////////////////////// state table ////////////////////////

// State table (from paq8 and lpaq1):
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

static const U8 State_table[256][2] = {
    {1, 2},     {3, 5},     {4, 6},     {7, 10},    {8, 12},    {9, 13},    {11, 14},   // 0
    {15, 19},   {16, 23},   {17, 24},   {18, 25},   {20, 27},   {21, 28},   {22, 29},   // 7
    {26, 30},   {31, 33},   {32, 35},   {32, 35},   {32, 35},   {32, 35},   {34, 37},   // 14
    {34, 37},   {34, 37},   {34, 37},   {34, 37},   {34, 37},   {36, 39},   {36, 39},   // 21
    {36, 39},   {36, 39},   {38, 40},   {41, 43},   {42, 45},   {42, 45},   {44, 47},   // 28
    {44, 47},   {46, 49},   {46, 49},   {48, 51},   {48, 51},   {50, 52},   {53, 43},   // 35
    {54, 57},   {54, 57},   {56, 59},   {56, 59},   {58, 61},   {58, 61},   {60, 63},   // 42
    {60, 63},   {62, 65},   {62, 65},   {50, 66},   {67, 55},   {68, 57},   {68, 57},   // 49
    {70, 73},   {70, 73},   {72, 75},   {72, 75},   {74, 77},   {74, 77},   {76, 79},   // 56
    {76, 79},   {62, 81},   {62, 81},   {64, 82},   {83, 69},   {84, 71},   {84, 71},   // 63
    {86, 73},   {86, 73},   {44, 59},   {44, 59},   {58, 61},   {58, 61},   {60, 49},   // 70
    {60, 49},   {76, 89},   {76, 89},   {78, 91},   {78, 91},   {80, 92},   {93, 69},   // 77
    {94, 87},   {94, 87},   {96, 45},   {96, 45},   {48, 99},   {48, 99},   {88, 101},  // 84
    {88, 101},  {80, 102},  {103, 69},  {104, 87},  {104, 87},  {106, 57},  {106, 57},  // 91
    {62, 109},  {62, 109},  {88, 111},  {88, 111},  {80, 112},  {113, 85},  {114, 87},  // 98
    {114, 87},  {116, 57},  {116, 57},  {62, 119},  {62, 119},  {88, 121},  {88, 121},  // 105
    {90, 122},  {123, 85},  {124, 97},  {124, 97},  {126, 57},  {126, 57},  {62, 129},  // 112
    {62, 129},  {98, 131},  {98, 131},  {90, 132},  {133, 85},  {134, 97},  {134, 97},  // 119
    {136, 57},  {136, 57},  {62, 139},  {62, 139},  {98, 141},  {98, 141},  {90, 142},  // 126
    {143, 95},  {144, 97},  {144, 97},  {68, 57},   {68, 57},   {62, 81},   {62, 81},   // 133
    {98, 147},  {98, 147},  {100, 148}, {149, 95},  {150, 107}, {150, 107}, {108, 151}, // 140
    {108, 151}, {100, 152}, {153, 95},  {154, 107}, {108, 155}, {100, 156}, {157, 95},  // 147
    {158, 107}, {108, 159}, {100, 160}, {161, 105}, {162, 107}, {108, 163}, {110, 164}, // 154
    {165, 105}, {166, 117}, {118, 167}, {110, 168}, {169, 105}, {170, 117}, {118, 171}, // 161
    {110, 172}, {173, 105}, {174, 117}, {118, 175}, {110, 176}, {177, 105}, {178, 117}, // 168
    {118, 179}, {110, 180}, {181, 115}, {182, 117}, {118, 183}, {120, 184}, {185, 115}, // 175
    {186, 127}, {128, 187}, {120, 188}, {189, 115}, {190, 127}, {128, 191}, {120, 192}, // 182
    {193, 115}, {194, 127}, {128, 195}, {120, 196}, {197, 115}, {198, 127}, {128, 199}, // 189
    {120, 200}, {201, 115}, {202, 127}, {128, 203}, {120, 204}, {205, 115}, {206, 127}, // 196
    {128, 207}, {120, 208}, {209, 125}, {210, 127}, {128, 211}, {130, 212}, {213, 125}, // 203
    {214, 137}, {138, 215}, {130, 216}, {217, 125}, {218, 137}, {138, 219}, {130, 220}, // 210
    {221, 125}, {222, 137}, {138, 223}, {130, 224}, {225, 125}, {226, 137}, {138, 227}, // 217
    {130, 228}, {229, 125}, {230, 137}, {138, 231}, {130, 232}, {233, 125}, {234, 137}, // 224
    {138, 235}, {130, 236}, {237, 125}, {238, 137}, {138, 239}, {130, 240}, {241, 125}, // 231
    {242, 137}, {138, 243}, {130, 244}, {245, 135}, {246, 137}, {138, 247}, {140, 248}, // 238
    {249, 135}, {250, 69},  {80, 251},  {140, 252}, {249, 135}, {250, 69},  {80, 251},  // 245
    {140, 252}, {0, 0},     {0, 0},     {0, 0}};                                        // 252
#define nex( state, sel ) State_table[state][sel]

//////////////////////////// StateMap //////////////////////////

// A StateMap maps a context to a probability.  After a bit prediction, the
// map is updated in the direction of the actual value to improve future
// predictions in the same context.

class StateMap {
protected:
  const int N;        // Number of contexts
  int cxt{ 0 };            // Context of last prediction
  U32 *t;             // cxt -> prediction in high 23 bits, count in low 9 bits
  static int dt[512]; // reciprocal table: i -> 16K/(i+2.5)
public:
  StateMap( int n = 256 ); // create allowing n contexts

  // Predict next bit to be updated in context cx (0..n-1).
  // Return prediction as a 16 bit number (0..65535) that next bit is 1.
  int p( int cx ) {
    assert( cx >= 0 && cx < N );
    return t[cxt = cx] >> 16;
  }

  // Update the model with bit y (0..1).
  // limit (1..511) controls the rate of adaptation (higher = slower)
  void update( int y, int limit = 511 ) {
    assert( cxt >= 0 && cxt < N );
    assert( y == 0 || y == 1 );
    assert( limit >= 0 && limit < 512 );
    int n = t[cxt] & 511, p = t[cxt] >> 14; // count, prediction
    if( n < limit )
      ++t[cxt];
    t[cxt] += ( ( y << 18 ) - p ) * dt[n] & 0xfffffe00;
  }
};

int StateMap::dt[512] = {0};

StateMap::StateMap( int n ) : N( n ) {
  alloc( t, N );
  for( int i = 0; i < N; ++i )
    t[i] = 1 << 31;
  if( dt[0] == 0 )
    for( int i = 0; i < 512; ++i )
      dt[i] = 32768 / ( i + i + 5 );
}

//////////////////////////// Predictor /////////////////////////

/* A Predictor estimates the probability that the next bit of
   uncompressed data is 1.  Methods:
   p() returns P(1) as a 16 bit number (0..65535).
   update(y) trains the predictor with the actual bit (0 or 1).
*/

class Predictor {
  int cxt{ 0 }; // Context: last 0-8 bits with a leading 1
  StateMap sm;
  U8 *state;

public:
  Predictor() :  sm( 0x10000 ) {
    alloc( state, 0x10000 );
  }

  // Assume a stationary order 0 stream of 9-bit symbols
  int p() {
    return sm.p( cxt * 256 + state[cxt] );
  }

  void update( int y ) {
    sm.update( y, 127 );
    state[cxt] = nex( state[cxt], y );
    if( ( cxt += cxt + y ) >= 256 )
      cxt = 0;
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
  Predictor predictor; // Computes next bit probability (0..65535)
  const Mode mode;     // Compress or decompress?
  FILE *archive;       // Compressed data file
  U32 x1, x2;          // Range, initially [0, 1), scaled by 2^32
  U32 x;               // Last 4 input bytes of archive.
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
      x = ( x << 8 ) + ( c & 0xff );
    }
  }
}

// encode(y) -- Encode bit y by splitting the range [x1, x2] in proportion
// to P(1) and P(0) as given by the predictor and narrowing to the appropriate
// subrange.  Output leading bytes of the range as they become known.

inline void Encoder::encode( int y ) {
  // Update the range
  const U32 p = predictor.p();
  assert( p <= 0xffff );
  assert( y == 0 || y == 1 );
  const U32 xmid = x1 + ( ( x2 - x1 ) >> 16 ) * p + ( ( x2 - x1 & 0xffff ) * p >> 16 );
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

// Decode one bit from the archive, splitting [x1, x2] as in the encoder
// and returning 1 or 0 depending on which subrange the archive point x is in.

inline int Encoder::decode() {
  // Update the range
  const U32 p = predictor.p();
  assert( p <= 0xffff );
  const U32 xmid = x1 + ( ( x2 - x1 ) >> 16 ) * p + ( ( x2 - x1 & 0xffff ) * p >> 16 );
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

// Should be called when there is no more to compress.
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
    printf( "To compress:   fpaq0f c input output\n"
            "To decompress: fpaq0f d input output\n" );
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

  // Compress each byte as 9 bits as 1xxxxxxxx, then EOF as 0.
  if( argv[1][0] == 'c' ) {
    Encoder e( COMPRESS, out );
    while( ( c = getc( in ) ) != EOF ) {
      e.encode( 1 );
      for( int i = 7; i >= 0; --i )
        e.encode( ( c >> i ) & 1 );
    }
    e.encode( 0 ); // EOF code
    e.flush();
  }

  // Decompress
  else {
    Encoder e( DECOMPRESS, in );
    while( e.decode() != 0 ) {
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
