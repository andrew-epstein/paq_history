// Order1.hpp - part of Word Replacing Transformation by P.Skibinski

#define MAX_FREQ_ORDER1 2520
#define ORDER1_STEP 4

int mZero[MAX_FREQ_ORDER1];
int mOne[MAX_FREQ_ORDER1];
int EOLcount = 0;

#define UPDATE_ORDER1( prev, value ) UpdateOrder1( prev, value, ORDER1_STEP )
#define ENCODE_ORDER1( prev, value ) EncodeOrder1( prev, value )
#define DECODE_ORDER1( prev ) DecodeOrder1( prev )
#define INIT_ORDER1 InitOrder1( MAX_FREQ_ORDER1 )

inline void InitOrder1( int size );

#include "RangeCoder.hpp"
RangeCoder coder;

inline void UpdateOrder1( int prev, int c, int step ) {
  if( c == 0 )
    mZero[prev] += step;
  else
    mOne[prev] += step;

  if( mZero[prev] + mOne[prev] >= 1 << 15 ) {
    mZero[prev] = ( mZero[prev] + 1 ) / 2;
    mOne[prev] = ( mOne[prev] + 1 ) / 2;
  }
}

inline void EncodeOrder1( int prev, int c ) {
  if( c == 0 )
    coder.Encode( 0, mZero[prev], mZero[prev] + mOne[prev] );
  else
    coder.Encode( mZero[prev], mOne[prev], mZero[prev] + mOne[prev] );
}

inline int DecodeOrder1( int prev ) {
  int c = coder.GetFreq( mZero[prev] + mOne[prev] );

  if( c < mZero[prev] )
    c = 0;
  else
    c = 1;

  if( c == 0 )
    coder.Decode( 0, mZero[prev], mZero[prev] + mOne[prev] );
  else
    coder.Decode( mZero[prev], mOne[prev], mZero[prev] + mOne[prev] );

  return c;
}

inline void InitOrder1( int size ) {
  static int i;

#ifdef LOG_ARITHMETIC_ENCODER
  printf( "init=%d\n", size );
#endif

  for( i = 0; i < size; i++ ) {
    mZero[i] = 1;
    mOne[i] = 1;
  }
}

inline int ContextEncode( int leftChar, int c, int rightChar, int distance ) {
  unsigned int prev, result;

  if( fsize == FILE_BOOK1 ) {
    if( rightChar >= 'A' && rightChar <= 'Z' )
      rightChar += 32;

    if( leftChar >= 'A' && leftChar <= 'Z' )
      leftChar += 32;
  }

  if( leftChar < 'a' || leftChar > 'z' || rightChar < 'a' || rightChar > 'z' )
    if( ( leftChar != ',' && leftChar != '.' && leftChar != '\'' ) || rightChar < 'a' || rightChar > 'z' )
      return c;

  EOLcount++;

#ifdef LOG_ARITHMETIC_ENCODER
  printf( "%c (%d) %c (%d) dist=%d\n", leftChar, leftChar, rightChar, rightChar, distance );
#endif

  if( c == 32 )
    result = 0;
  else
    result = 1;

  if( leftChar == ',' )
    leftChar = 'z' + 1;
  if( leftChar == '.' || leftChar == '\'' )
    leftChar = 'z' + 3;

  leftChar -= 'a'; // leftChar (0-25+3)

  if( rightChar > 127 )
    rightChar = 127;
  else if( rightChar == 10 )
    rightChar = 32;
  else if( rightChar < 32 )
    rightChar = 128;
  rightChar -= 32; // c (0-96)

  if( distance > 80 )
    distance = 80;

  prev = 5 * 16 * ( distance / 5 + 1 ) + 5 * ( leftChar / 2 + 1 ) + ( rightChar / 32 );

  ENCODE_ORDER1( prev, result );
  UPDATE_ORDER1( prev, result );

  return 32;
}

inline int ContextDecode( int leftChar, int rightChar, int distance ) {
  unsigned int prev, result;

  if( fsize == FILE_BOOK1 ) {
    if( rightChar >= 'A' && rightChar <= 'Z' )
      rightChar += 32;

    if( leftChar >= 'A' && leftChar <= 'Z' )
      leftChar += 32;
  }

  if( leftChar < 'a' || leftChar > 'z' || rightChar < 'a' || rightChar > 'z' )
    if( ( leftChar != ',' && leftChar != '.' && leftChar != '\'' ) || rightChar < 'a' || rightChar > 'z' )
      return 32;

  EOLcount++;

#ifdef LOG_ARITHMETIC_ENCODER
  printf( "%c (%d) %c (%d) dist=%d\n", leftChar, leftChar, rightChar, rightChar, distance );
#endif

  if( leftChar == ',' )
    leftChar = 'z' + 1;
  if( leftChar == '.' || leftChar == '\'' )
    leftChar = 'z' + 3;
  leftChar -= 'a'; // leftChar (0-26)

  if( rightChar > 127 )
    rightChar = 127;
  else if( rightChar == 10 )
    rightChar = 32;
  else if( rightChar < 32 )
    rightChar = 128;
  rightChar -= 32; // c (0-96)

  if( distance > 80 )
    distance = 80;

  prev = 5 * 16 * ( distance / 5 + 1 ) + 5 * ( leftChar / 2 + 1 ) + ( rightChar / 32 );

  result = DECODE_ORDER1( prev );
  UPDATE_ORDER1( prev, result );

  if( result == 0 )
    return 32;

  return 10;
}

inline void EncodeEOLformat( EEOLType EOLType ) {
#ifdef LOG_ARITHMETIC_ENCODER
  printf( "EncodeEOLformat %d\n", EOLType );
#endif
  EOLcount++;

  if( EOLType == CRLF )
    coder.Encode( 0, 1, 2 );
  else
    coder.Encode( 1, 1, 2 );
}

inline EEOLType DecodeEOLformat() {
  int c = coder.GetFreq( 2 );

#ifdef LOG_ARITHMETIC_ENCODER
  printf( "DecodeEOLformat %d\n", ( c < 1 ) ? CRLF : LF );
#endif

  if( c < 1 ) {
    coder.Decode( 0, 1, 2 );
    return CRLF;
  } else
    coder.Decode( 1, 1, 2 );

  return LF;
}
