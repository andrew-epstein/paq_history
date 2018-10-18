// RangeCoder.hpp - part of Word Replacing Transformation by P.Skibinski
//
// based on files Shindlet.cdr and io_ramd.inc from
// ARIDEMO v1.04 (c) 2000-2001  Dmitry Shkarin, Eugene Shelwien

#include <memory.h>
#include <stdlib.h>

// types definitions

typedef unsigned int uint;
typedef unsigned char uc;

// filesize() function

uint flen( FILE *f ) {
  fseek( f, 0, SEEK_END );
  uint len = ftell( f );
  fseek( f, 0, SEEK_SET );
  return len;
}

// Input/Output using dynamic memory allocation

#define OUTPUT_BUFFER_MIN_SIZE 10240

class MemoryBuffer {
  uc *SourceBuf;
  uint SrcPtr, SrcLen;
  uc *TargetBuf;
  uint TgtPtr, TgtLen;

public:
  MemoryBuffer() {
    SrcPtr = 0;
    TgtPtr = 0;
  };
  inline void OutTgtByte( uc c ) {
    *( TargetBuf + ( TgtPtr++ ) ) = c;
    if( TgtPtr > TgtLen - 1 )
      ReallocTgtBuf( TgtLen * 2 );
  }

  inline uc InpSrcByte( void ) {
#ifdef FUNCTION_CHECK_ERRORS
    if( SrcPtr >= SrcLen ) {
      printf( "File is corrupted! (RangeCoder)\n" );
      exit( 1 );
    }
#endif
    return *( SourceBuf + ( SrcPtr++ ) );
  }

  inline void AllocSrcBuf( FILE *SourceFile, uint len ) {
    if( len == 0 )
      SrcLen = flen( SourceFile );
    else
      SrcLen = len;
    SourceBuf = new uc[SrcLen];
    fread( SourceBuf, SrcLen, 1, SourceFile );
  }

  inline void AllocTgtBuf( uint len = OUTPUT_BUFFER_MIN_SIZE ) {
    TgtLen = len;
    TargetBuf = new uc[len];
  }

  inline void ReallocTgtBuf( uint len ) {
    uc *NewTargetBuf = new uc[len];
    memcpy( NewTargetBuf, TargetBuf, min( TgtPtr, len ) );
    TgtLen = len;
    delete( TargetBuf );
    TargetBuf = NewTargetBuf;
  }

  inline uint FlushTgtBuf( FILE *TargetFile ) {
    fwrite( TargetBuf, TgtPtr, 1, TargetFile );
    return TgtPtr;
  }
};

// class RangeCoder - arithmetic coder

#define DO( n ) for( int _ = 0; _ < n; _++ )
#define TOP ( 1 << 24 )

class RangeCoder {
  uint code, range, FFNum, Cache;
  __int64 low;
  MemoryBuffer buffer;

public:
  // inline void ShiftLow( void )
#define ShiftLow()                                                                                                     \
  {                                                                                                                    \
    if( ( low ^ 0xFF000000 ) > 0xFFFFFF ) {                                                                            \
      buffer.OutTgtByte( Cache + ( low >> 32 ) );                                                                      \
      int c = 0xFF + ( low >> 32 );                                                                                    \
      while( FFNum )                                                                                                   \
        buffer.OutTgtByte( c ), FFNum--;                                                                               \
      Cache = uint( low ) >> 24;                                                                                       \
    } else                                                                                                             \
      FFNum++;                                                                                                         \
    low = uint( low ) << 8;                                                                                            \
  }

  void StartEncode() {
    low = FFNum = Cache = 0;
    range = ( uint ) -1;
    buffer.AllocTgtBuf();
  }

  void StartDecode( FILE *InputFile, uint len ) {
    buffer.AllocSrcBuf( InputFile, len );
    code = 0;
    range = ( uint ) -1;
    DO( 5 ) code = ( code << 8 ) | buffer.InpSrcByte();
  }

  uint FinishEncode( FILE *OutputFile ) {
    DO( 5 ) ShiftLow();
    return buffer.FlushTgtBuf( OutputFile );
  }

  void FinishDecode( void ) {}

  void Encode( uint cumFreq, uint freq, uint totFreq ) {
    low += cumFreq * ( range /= totFreq );
    range *= freq;
    while( range < TOP ) {
      ShiftLow();
      range <<= 8;
    }
  }

  uint GetFreq( uint totFreq ) {
    return code / ( range /= totFreq );
  }

  void Decode( uint cumFreq, uint freq, uint totFreq ) {
    code -= cumFreq * range;
    range *= freq;
    while( range < TOP )
      code = ( code << 8 ) | buffer.InpSrcByte(), range <<= 8;
  }
};
