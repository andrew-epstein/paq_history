#define CREATION_DATE "18.01.2005"
#define LONG_PRG_NAME "PAsQDa (PAQ+Dictionary) v1.0"
#define PRG_NAME LONG_PRG_NAME " by M. Mahoney & P. Skibinski (" CREATION_DATE ")"

#define CHAR_CR_LF 5
#define CHAR_NOSPACE 6     // for punctuation marks modeling
#define CHAR_PUNCTUATION 6 // for punctuation marks modeling
#define CHAR_ESCAPE 7      // for encode reserved chars (CHAR_ESCAPE,CHAR_FIRSTUPPER,...)
#define CHAR_FIRSTUPPER 8  // for encode lower word with first capital letter
#define CHAR_UPPERWORD 11  // for encode upper word
#define CHAR_LOWERWORD 12  // for encode lower word with few capital letter
#define NGRAM_FIRST 'A'
#define NGRAM_LAST 'Z'
#define BINARY_FIRST 128
#define BINARY_LAST 255

#define OPTION_SPACELESS_WORDS 2
#define OPTION_CAPTIAL_CONVERSION 4
#define OPTION_SPACE_AFTER_CC_FLAG 8
#define OPTION_USE_DICTIONARY 16
#define OPTION_TRY_SHORTER_WORD 32
#define OPTION_WORD_SURROROUNDING_MODELING 64
#define OPTION_NORMAL_TEXT_FILTER 128
#define OPTION_EOL_CODING 256
#define OPTION_ADD_SYMBOLS_0_4 512
#define OPTION_ADD_SYMBOLS_14_31 1024
#define OPTION_ADD_SYMBOLS_A_Z 2048
#define OPTION_ADD_SYMBOLS_MISC 4096
#define OPTION_USE_NGRAMS 8192

#define AUTO_SWITCH 10 // param for !OPTION_NORMAL_TEXT_FILTER
#define WORD_MIN_SIZE 1
#define FUNCTION_CHECK_ERRORS
#define WRT_HEADER "WRT4"

//#define min(a,b) (((a)>(b))?(b):(a))
#define TOLOWER( c ) ( ( c >= 'A' && c <= 'Z' ) ? ( c + 32 ) : ( c ) )
#define TOUPPER( c ) ( ( c >= 'a' && c <= 'z' ) ? ( c - 32 ) : ( c ) )
#define IF_OPTION( option ) ( ( preprocFlag & option ) != 0 )
#define TURN_OFF( option )                                                                                             \
  {                                                                                                                    \
    if( preprocFlag & option )                                                                                         \
      preprocFlag -= option;                                                                                           \
  }
#define TURN_ON( option )                                                                                              \
  {                                                                                                                    \
    if( ( preprocFlag & option ) == 0 )                                                                                \
      preprocFlag += option;                                                                                           \
  }
#define RESET_OPTIONS ( preprocFlag = 0 )

enum EPreprocessType { BWT, PPM, PAQ };
enum EWordType { LOWERWORD, FIRSTUPPER, UPPERWORD };
enum EEOLType { UNDEFINED, CRLF, LF };
enum EUpperType { UFALSE, UTRUE, FORCE };
enum ESpaceType { NONE, SPACE, EOL };

#define COND_BIN_FILTER( c ) ( ( ( c < 32 ) ? ( c != 10 && c != 13 ) : ( 0 ) ) || ( c >= BINARY_FIRST ) )

#define PRINT_CHARS( data ) ;     //printf data
#define PRINT_CODEWORDS( data ) ; // printf data
#define PRINT_DICT( data ) ;      //printf data
//#define LOG_ARITHMETIC_ENCODER

bool initOrder, forceNormalTextFilter, forceWordSurroroundModeling, forceEOLcoding;

EPreprocessType preprocType;
ESpaceType spaceBefore = NONE;
int preprocFlag = 0, tryShorterBound = 0;

#define POWERED_BY_PAQ
#ifdef POWERED_BY_PAQ

#  define DECODE_GETC( c, file )                                                                                       \
    { c = e.decode(); }

#  define ENCODE_PUTC( c, file )                                                                                       \
    { e.encode( c ); }

#else

#  define DECODE_GETC( c, file )                                                                                       \
    { c = getc( file ); }

#  define ENCODE_PUTC( c, file ) putc( c, file )

#endif

#include <stdio.h>
#include "RangeCoder.hpp"
RangeCoder coder;
#include "Order1.hpp"
#include "Dictionary.hpp"
#include "Detect.hpp"

int bufferedChar = -1;
EEOLType EOLType = UNDEFINED;
int lastEOL = 0;
int lastChar = 0;
int fftell = 0;

bool swapCase = false;

#define SWAP_CASE( c )                                                                                                 \
  {                                                                                                                    \
    if( swapCase ) {                                                                                                   \
      if( c != ' ' && c != '\r' && c != '\n' && c != '\'' && c != '"' && c != '\t' )                                   \
        swapCase = false;                                                                                              \
                                                                                                                       \
      if( c >= 'a' && c <= 'z' ) {                                                                                     \
        c -= 32; /* toupper(c); */                                                                                     \
      } else if( c >= 'A' && c <= 'Z' ) {                                                                              \
        c += 32; /* tolower(c); */                                                                                     \
      }                                                                                                                \
    } else if( c == '.' || c == '!' || c == '?' )                                                                      \
      swapCase = true;                                                                                                 \
  }

#define ENCODE_GETC( c, file )                                                                                         \
  {                                                                                                                    \
    c = getc( file );                                                                                                  \
    fftell++;                                                                                                          \
  }

#define DECODE_PUTC( c, file )                                                                                         \
  {                                                                                                                    \
    fftell++;                                                                                                          \
    putc( c, file );                                                                                                   \
  }

char WRT_get_options( FILE *file );

// preprocess the file
void WRT_encode( FILE *file, FILE *fileout, int fileLen ) {
  unsigned char s[1024];
  EWordType wordType;
  int preprocessing, last_c, c, next_c, s_size;

  //	printf("- encoding file (%d bytes) afterWRT=%d",fileLen,afterWRT);

  c = WRT_get_options( file );
  ENCODE_PUTC( c, fileout );

  //printf(" c=%d\n",c);

  preprocessing = 0;
  s_size = 0;
  last_c = 0;
  lastEOL = 0;
  EOLType = UNDEFINED;
  wordType = LOWERWORD;

  ENCODE_GETC( c, file );
  fftell = 0;

  initOrder = true;
  lastEOL = 0;

  printf( "      " );
  long step = ceil( ( double ) fileLen / 1000 );

  while( !feof( file ) ) {
    if( fftell % step == 0 )
      printf( "\b\b\b\b\b\b%5.1f%%", ( ( double ) fftell / fileLen ) * 100 );

    PRINT_CHARS( ( "c=%d (%c) spaceBefore=%d\n", c, c, spaceBefore ) );

    if( preprocessing > 0 ) {
      if( !IF_OPTION( OPTION_NORMAL_TEXT_FILTER ) && COND_BIN_FILTER( c ) ) {
        preprocessing = AUTO_SWITCH;
        PRINT_CHARS( ( "preprocessing=%d c=%c(%d)\n", preprocessing, c, c ) );
      } else {
        preprocessing--;
        PRINT_CHARS( ( "preprocessing=%d c=%c(%d)\n", preprocessing, c, c ) );
        if( preprocessing == 0 )
          initOrder = true;
      }

      ENCODE_PUTC( c, fileout );
      ENCODE_GETC( c, file );
      continue;
    }

    if( c >= 'a' && c <= 'z' ) {
      PRINT_CHARS( ( "a-z c=%d (%c) spaceBefore=%d\n", c, c, spaceBefore ) );

      if( s_size == 0 ) {
        wordType = LOWERWORD;
      } else {
        if( wordType == UPPERWORD ) {
          encodeWord( fileout, s, s_size, wordType );
          if( IF_OPTION( OPTION_CAPTIAL_CONVERSION ) )
            ENCODE_PUTC( CHAR_LOWERWORD, fileout );

          wordType = LOWERWORD;
          s_size = 1;
          s[0] = c;
          last_c = c;
          ENCODE_GETC( c, file );
          continue;
        }
      }

      s[s_size++] = c;
      if( s_size >= sizeof( s ) - 1 ) {
        encodeWord( fileout, s, s_size, wordType );
        s_size = 0;
      }
      last_c = c;
      ENCODE_GETC( c, file );
      continue;
    }

    if( c <= 'Z' && c >= 'A' ) {
      PRINT_CHARS( ( "A-Z c=%d (%c) spaceBefore=%d\n", c, c, spaceBefore ) );

      if( s_size == 0 ) {
        wordType = FIRSTUPPER;
      } else {
        if( wordType == FIRSTUPPER ) {
          if( s_size == 1 )
            wordType = UPPERWORD;
          else {
            encodeWord( fileout, s, s_size, wordType );

            wordType = FIRSTUPPER;
            s_size = 1;
            s[0] = c;
            last_c = c;
            ENCODE_GETC( c, file );
            continue;
          }
        } else if( wordType == LOWERWORD ) {
          encodeWord( fileout, s, s_size, wordType );

          wordType = FIRSTUPPER;
          s_size = 1;
          s[0] = c;
          last_c = c;
          ENCODE_GETC( c, file );
          continue;
        }
      }

      s[s_size++] = c;
      if( s_size >= sizeof( s ) - 1 ) {
        encodeWord( fileout, s, s_size, wordType );
        s_size = 0;
      }

      last_c = c;
      ENCODE_GETC( c, file );
      continue;
    }

    if( reservedSet[c] ) {
      PRINT_CHARS( ( "reservedSet[c] c=%d (%c)\n", c, c ) );

      encodeWord( fileout, s, s_size, wordType );
      s_size = 0;
      ENCODE_PUTC( CHAR_ESCAPE, fileout );
      ENCODE_PUTC( c, fileout );

      if( !IF_OPTION( OPTION_NORMAL_TEXT_FILTER ) && COND_BIN_FILTER( c ) )
        preprocessing = AUTO_SWITCH;

      last_c = c;
      ENCODE_GETC( c, file );
      continue;
    }

    encodeWord( fileout, s, s_size, wordType );

    s_size = 0;

    PRINT_CHARS( ( "other c=%d\n", c ) );

    ENCODE_GETC( next_c, file );

    if( IF_OPTION( OPTION_EOL_CODING ) && ( c == 32 || c == 10 || ( c == 13 && next_c == 10 ) ) ) {
      if( initOrder ) {
        initOrder = false;
        INIT_ORDER1;
      }

      if( c == 13 ) {
        if( EOLType == CRLF || EOLType == UNDEFINED ) {
          c = next_c;
          ENCODE_GETC( next_c, file );
          lastEOL++;

          last_c = ContextEncode( last_c, c, next_c, fftell - lastEOL + ( next_c < 0 ? 1 : 0 ) );

          if( EOLType == UNDEFINED && last_c != c ) {
            EOLType = CRLF;
            EncodeEOLformat( EOLType );
          }

          lastEOL = fftell;

          if( last_c == 10 ) {
            ENCODE_PUTC( CHAR_CR_LF, fileout );
          } else {
            if( ( IF_OPTION( OPTION_SPACELESS_WORDS ) ) && last_c == ' ' )
              spaceBefore = SPACE;
            else
              ENCODE_PUTC( last_c, fileout );
          }
        } else
          ENCODE_PUTC( c, fileout );
      } else {
        if( c == 10 && EOLType == CRLF )
          ENCODE_PUTC( c, fileout )
        else {
          if( IF_OPTION( OPTION_EOL_CODING ) ) {
            last_c = ContextEncode( last_c, c, next_c, fftell - lastEOL + ( next_c < 0 ? 1 : 0 ) );
          } else
            last_c = c;

          if( EOLType == UNDEFINED && last_c != c ) {
            EOLType = LF;
            EncodeEOLformat( EOLType );
          }

          if( ( IF_OPTION( OPTION_SPACELESS_WORDS ) ) && last_c == ' ' )
            spaceBefore = SPACE;
          else
            ENCODE_PUTC( last_c, fileout );
        }

        if( c == 10 ) {
          lastEOL = fftell;
        }
      }
    } else {
      if( c != EOF ) {
        if( ( IF_OPTION( OPTION_WORD_SURROROUNDING_MODELING ) ) && c > ' ' ) {
          if( ( last_c >= 'a' && last_c <= 'z' ) || ( last_c >= 'A' && last_c <= 'Z' ) ) {
            ENCODE_PUTC( ' ', fileout );
            ENCODE_PUTC( CHAR_PUNCTUATION, fileout );
            ENCODE_PUTC( c, fileout );

          } else if( next_c >= 'a' && next_c <= 'z' ) {
            ENCODE_PUTC( c, fileout );
            ENCODE_PUTC( CHAR_LOWERWORD, fileout );
            ENCODE_PUTC( ' ', fileout );
          } else {
            ENCODE_PUTC( c, fileout );
          }
        } else if( ( IF_OPTION( OPTION_SPACELESS_WORDS ) ) && c == ' ' )
          spaceBefore = SPACE;
        else
          ENCODE_PUTC( c, fileout );
      }
    }

    last_c = c;
    c = next_c;
  }

  encodeWord( fileout, s, s_size, wordType );
  s_size = 0;

  printf( "\b\b\b\b\b\b      \b\b\b\b\b\b" );

  //printf("- encoding finished (%d->%d bytes) afterWRT=%d c=%d\n",fileLen,ftell(fileout),afterWRT,c);
}

int stringCachePtr = 0;
char stringCache[40];

inline void hook_putc( int c, FILE *file ) {
  if( bufferedChar < 0 && c == ' ' ) {
    bufferedChar = c;
    return;
  }

  if( bufferedChar >= 0 ) {
    if( IF_OPTION( OPTION_EOL_CODING ) ) {
      if( initOrder ) {
        initOrder = false;
        INIT_ORDER1;
      }

      bufferedChar = ContextDecode( lastChar, c, fftell - lastEOL );

      if( bufferedChar == 10 ) {
        if( EOLType == UNDEFINED )
          EOLType = DecodeEOLformat();
        if( EOLType == CRLF ) {
          lastChar = 13;
          DECODE_PUTC( lastChar, file );
        }

        lastEOL = fftell;
      }
    }

    DECODE_PUTC( bufferedChar, file );

    if( c == ' ' ) {
      lastChar = bufferedChar;
      bufferedChar = c;
      return;
    }

    bufferedChar = -1;
  }

  if( c == 10 )
    lastEOL = fftell;

  lastChar = c;

  stringCache[stringCachePtr] = c;
  stringCachePtr++;
  if( stringCachePtr >= 20 )
    stringCachePtr = 0;

  if( c == EOF )
    return;

  DECODE_PUTC( c, file );
  return;
}

void readEOLstream( FILE *file ) {
  unsigned int fileEnd;
  unsigned int EOLstream_len = ( unsigned int ) ( -1 );
  int fileLen;

  fseek( file, -4, SEEK_END );
  fileLen = ftell( file ) + 4;

  fread( ( void * ) &fileEnd, sizeof( fileEnd ), 1, file );
  EOLstream_len -= fileEnd;

  //	printf("End-of-Line (EOL) stream %d\n",EOLstream_len);

  if( EOLstream_len < fileLen ) {
    fseek( file, -( ( int ) EOLstream_len ) - 4, SEEK_END );

    coder.StartDecode( file, EOLstream_len );
  }

  fseek( file, 0, SEEK_SET );
}

void writeEOLstream( FILE *fileout ) {
  unsigned int EOLstream_len;
  unsigned int enc;

  EOLstream_len = ( unsigned int ) ( -1 );

  enc = coder.FinishEncode( fileout );
  EOLstream_len -= enc;

  printf( "%-24s%10ld -> %d\n", "End-of-Line (EOL) stream", 0, enc );

  fwrite( ( void * ) &EOLstream_len, sizeof( EOLstream_len ), 1, fileout );
}

void WRT_set_options( char c );

int bufferedDecodeGetC = -1;

void WRT_decode( FILE *file, FILE *fileout, int decomprFileLen ) {
  bool upper = false;
  EUpperType upperWord = UFALSE;
  int i, c;
  int preprocessing = 0;
  unsigned char s[1024];
  int s_size = 0;

  //	printf("- decoding file (%d bytes) afterWRT=%d\n",decomprFileLen,afterWRT);

  bufferedChar = -1;
  EOLType = UNDEFINED;
  lastChar = 0;
  fftell = 0;

  if( bufferedDecodeGetC >= 0 )
    c = bufferedDecodeGetC;
  else
    DECODE_GETC( c, file );

  WRT_set_options( c );

  //	printf("- decoding file (%d bytes) c=%d\n",decomprFileLen,c);

  if( IF_OPTION( OPTION_SPACELESS_WORDS ) )
    spaceBefore = SPACE;
  else
    spaceBefore = NONE;

  initOrder = true;

  //	printf("- decoding file (%d bytes->%d bytes), EOL stream=%d bytes\n",fileLen,fileEnd);

  DECODE_GETC( c, file );
  lastEOL = -1;
  EOLType = UNDEFINED;

  printf( "      " );
  long step = ceil( ( double ) decomprFileLen / 1000 );

  while( fftell < decomprFileLen ) {
    if( fftell % step == 0 )
      printf( "\b\b\b\b\b\b%5.1f%%", ( ( double ) fftell / decomprFileLen ) * 100 );

    PRINT_CHARS( ( "c=%d (%c) spaceBefore=%d\n", c, c, spaceBefore ) );

    if( c < 0 )
      break;

    if( preprocessing > 0 ) {
      DECODE_PUTC( c, fileout );

      if( !IF_OPTION( OPTION_NORMAL_TEXT_FILTER ) && COND_BIN_FILTER( c ) ) {
        preprocessing = AUTO_SWITCH;
      } else {
        preprocessing--;
        PRINT_CHARS( ( "preprocessing=%d c=%c(%d)\n", preprocessing, c, c ) );
        if( preprocessing == 0 )
          initOrder = true;
      }

      DECODE_GETC( c, file );
      continue;
    }

    if( addSymbols[c] && IF_OPTION( OPTION_USE_DICTIONARY ) ) {
      if( spaceBefore == SPACE ) {
        if( upperWord == FORCE )
          upperWord = UTRUE;
        else
          upperWord = UFALSE;
      }

      PRINT_CHARS( ( "addSymbols[c] && IF_OPTION(OPTION_USE_DICTIONARY) upperWord=%d\n", upperWord ) );

      s_size = decodeCodeWord( file, s, c );

      if( upper ) {
        upper = false;
        s[0] = TOUPPER( s[0] );
      }

      if( upperWord != UFALSE )
        toUpper( s, s_size );

      if( IF_OPTION( OPTION_SPACELESS_WORDS ) ) {
        if( spaceBefore == SPACE )
          hook_putc( ' ', fileout );
        else
          spaceBefore = SPACE;
      }

      PRINT_CHARS( ( "word=" ) );
      for( i = 0; i < s_size; i++ )
        PRINT_CHARS( ( "%c", s[i] ) );
      PRINT_CHARS( ( " upperWord=%d\n", upperWord ) );

      for( i = 0; i < s_size; i++ ) {
        //				ORIGINAL_CHARSET(s[i]);
        hook_putc( s[i], fileout );
      }
      continue;
    }

    if( reservedSet[c] ) {
      if( c == CHAR_ESCAPE ) {
        upper = false;
        upperWord = UFALSE;

        DECODE_GETC( c, file );
        PRINT_CHARS( ( "c==CHAR_ESCAPE, next=%x\n", c ) );
        hook_putc( c, fileout );

        if( !IF_OPTION( OPTION_NORMAL_TEXT_FILTER ) && COND_BIN_FILTER( c ) )
          preprocessing = AUTO_SWITCH;

        DECODE_GETC( c, file );
        continue;
      }

      if( c == CHAR_NOSPACE && IF_OPTION( OPTION_SPACELESS_WORDS ) ) {
        PRINT_CHARS( ( "c==CHAR_NOSPACE\n" ) );

        if( upperWord == FORCE )
          upperWord = UTRUE;

        DECODE_GETC( c, file );
        spaceBefore = NONE;
        continue;
      }

      if( c == CHAR_PUNCTUATION ) {
        if( bufferedChar == 32 )
          bufferedChar = -1;
        DECODE_GETC( c, file );
        continue;
      }

      if( c == CHAR_CR_LF ) {
        PRINT_CHARS( ( "c==CHAR_CR_LF\n" ) );

        hook_putc( '\r', fileout );
        c = '\n';
      }

      if( c == CHAR_FIRSTUPPER ) {
        PRINT_CHARS( ( "c==CHAR_FIRSTUPPER\n" ) );

        if( IF_OPTION( OPTION_SPACE_AFTER_CC_FLAG ) ) {
          DECODE_GETC( c, file ); // skip space
        }
        upper = true;
        upperWord = UFALSE;
        DECODE_GETC( c, file );
        continue;
      }

      if( c == CHAR_UPPERWORD ) {
        PRINT_CHARS( ( "c==CHAR_UPPERWORD\n" ) );

        if( IF_OPTION( OPTION_SPACE_AFTER_CC_FLAG ) ) {
          DECODE_GETC( c, file ); // skip space
        }
        upperWord = FORCE;
        DECODE_GETC( c, file );
        continue;
      }

      if( c == CHAR_LOWERWORD ) {
        PRINT_CHARS( ( "c==CHAR_LOWERWORD\n" ) );

        upper = false;
        upperWord = UFALSE;
        DECODE_GETC( c, file );
        if( c == ' ' ) // skip space
        {
          DECODE_GETC( c, file );
        }
        continue;
      }
    }

    PRINT_CHARS( ( "other c=%d\n", c ) );

    if( upperWord != UFALSE ) {
      if( upperWord == FORCE )
        upperWord = UTRUE;

      if( c >= 'a' && c <= 'z' )
        c = TOUPPER( c );
      else
        upperWord = UFALSE;
    } else if( upper == true ) {
      upper = false;
      c = TOUPPER( c );
    }

    hook_putc( c, fileout );

    DECODE_GETC( c, file );
    continue;
  }

  //	printf("- decoding finished (%d bytes) afterWRT=%d bufferedDecodeGetC=%d\n",ftell(fileout),afterWRT,c);

  hook_putc( EOF, fileout );

  bufferedDecodeGetC = c;

  printf( "\b\b\b\b\b\b      \b\b\b\b\b\b" );
}

int defaultSettings( int argc, char *argv[] ) {
  static bool firstTime = true;

  RESET_OPTIONS;

  TURN_ON( OPTION_EOL_CODING );
  TURN_ON( OPTION_TRY_SHORTER_WORD );
  TURN_ON( OPTION_USE_DICTIONARY );
  TURN_ON( OPTION_NORMAL_TEXT_FILTER );
  TURN_ON( OPTION_CAPTIAL_CONVERSION );
  //	TURN_ON(OPTION_ADD_SYMBOLS_A_Z);

  switch( preprocType ) {
    case BWT:
      TURN_ON( OPTION_SPACELESS_WORDS );
      TURN_ON( OPTION_ADD_SYMBOLS_0_4 );
      TURN_ON( OPTION_ADD_SYMBOLS_14_31 );
      TURN_ON( OPTION_ADD_SYMBOLS_MISC );
      break;
    case PAQ:
    case PPM:
      tryShorterBound = 4;
      //	TURN_ON(OPTION_USE_NGRAMS);
      TURN_ON( OPTION_WORD_SURROROUNDING_MODELING );
      //	TURN_ON(OPTION_SPACE_AFTER_CC_FLAG)
      break;
  }

  return 0;
}

void WRT_reset_options() {
  forceNormalTextFilter = false;
  forceWordSurroroundModeling = false;
  forceEOLcoding = false;

  preprocType = PAQ;

  defaultSettings( 0, NULL );
}

void WRT_read_dict( bool encoding ) {
  int dictPathLen;
  unsigned char dictPath[256];
  unsigned char s[256];
  s[0] = 0;

  printf( PRG_NAME "\n" );

  WRT_reset_options();

#ifdef WIN32
  dictPathLen = getSourcePath( ( char * ) dictPath, sizeof( dictPath ) );
#else
  dictPath[0] = 0;
  dictPathLen = 0;
#endif

  if( dictPathLen > 0 ) {
    dictPath[dictPathLen] = 0;
    strcpy( ( char * ) s, "pasqda.dic" );
    strcat( ( char * ) dictPath, ( char * ) s );
    strcpy( ( char * ) s, ( char * ) dictPath );
  }

  if( !initialize( ( unsigned char * ) s, NULL, encoding ) )
    exit( 0 );
}

void WRT_set_options( char c ) {
  WRT_reset_options();

  if( ( c & 64 ) == 0 )
    TURN_OFF( OPTION_USE_DICTIONARY )
  else
    TURN_ON( OPTION_USE_DICTIONARY );

  if( ( c & 32 ) == 0 )
    TURN_OFF( OPTION_EOL_CODING )
  else
    TURN_ON( OPTION_EOL_CODING );

  if( ( c & 16 ) == 0 )
    TURN_OFF( OPTION_WORD_SURROROUNDING_MODELING )
  else
    TURN_ON( OPTION_WORD_SURROROUNDING_MODELING );

  if( ( c & 8 ) == 0 )
    TURN_OFF( OPTION_NORMAL_TEXT_FILTER )
  else
    TURN_ON( OPTION_NORMAL_TEXT_FILTER );

  if( ( IF_OPTION( OPTION_WORD_SURROROUNDING_MODELING ) ) && ( IF_OPTION( OPTION_SPACELESS_WORDS ) ) ) {
    printf( "OPTION_WORD_SURROROUNDING_MODELING and OPTION_SPACELESS_WORDS collision\n" );
    return;
  }

  if( !IF_OPTION( OPTION_NORMAL_TEXT_FILTER ) )
    TURN_OFF( OPTION_CAPTIAL_CONVERSION );

  if( !IF_OPTION( OPTION_USE_DICTIONARY ) )
    TURN_OFF( OPTION_USE_NGRAMS );

  WRT_initializeCodeWords();
}

char WRT_get_options( FILE *file ) {
  WRT_reset_options();
  preprocess( file, 10240, 5 );

  if( ( IF_OPTION( OPTION_WORD_SURROROUNDING_MODELING ) ) && ( IF_OPTION( OPTION_SPACELESS_WORDS ) ) ) {
    printf( "OPTION_WORD_SURROROUNDING_MODELING and OPTION_SPACELESS_WORDS collision\n" );
    exit( 0 );
  }

  if( !IF_OPTION( OPTION_NORMAL_TEXT_FILTER ) )
    TURN_OFF( OPTION_CAPTIAL_CONVERSION );

  if( !IF_OPTION( OPTION_USE_DICTIONARY ) )
    TURN_OFF( OPTION_USE_NGRAMS );

  char c = 0;

  if( IF_OPTION( OPTION_USE_DICTIONARY ) )
    c = c + 64;
  if( IF_OPTION( OPTION_EOL_CODING ) )
    c = c + 32;
  if( IF_OPTION( OPTION_WORD_SURROROUNDING_MODELING ) )
    c = c + 16;
  if( IF_OPTION( OPTION_NORMAL_TEXT_FILTER ) )
    c = c + 8;

  WRT_initializeCodeWords(); // depend on OPTIONs

  return c;
}

void main2( int argc, char *argv[] ) {
  FILE *file, *fileout;
  bool encoding;
  int optCount;

  printf( PRG_NAME "\n" );

  optCount = defaultSettings( argc, argv );

  if( argc - optCount < 3 || strcmp( argv[optCount + 1], argv[optCount + 2] ) == 0 ) {
    printf( "\nusage: WRT.exe [options] <input_file> <output_file>\n" );
    printf( "  general options (which also set default additional options):\n" );
    printf( "    -1 = preprocessing optimized for further BWT-based compresion\n" );
    printf( "    -2 = preprocessing optimized for further PPM-based compresion (default)\n" );
    printf( "    -3 = preprocessing optimized for further PAQ-based compresion\n" );
    printf( "  additional options:\n" );
    printf( "    -b = Turn off binary data filter option (ignore auto detection)\n" );
    printf( "    +b = Turn on binary data filter option (ignore auto detection)\n" );
    printf( "    -d = Turn off usage of the dictionary and n-gram replacement (-n)\n" );
    printf( "    -e = Turn off EOL-coding (ignore auto detection)\n" );
    printf( "    +e = Turn on EOL-coding (ignore auto detection)\n" );
    printf( "    -n = Turn off n-gram replacement\n" );
    printf( "    -p = Turn off punctuation marks modeling (ignore auto detection)\n" );
    printf( "    +p = Turn on punctuation marks modeling (ignore auto detection)\n" );
    printf( "    -t = Turn off \"try shorter word\" option\n" );
    return;
  };

  file = fopen( ( const char * ) argv[optCount + 1], "rb" );
  if( file == NULL ) {
    printf( "Can't open file %s\n", argv[optCount + 1] );
    return;
  }

  encoding = true;
  if( getc( file ) == WRT_HEADER[0] && getc( file ) == WRT_HEADER[1] && getc( file ) == WRT_HEADER[2]
      && getc( file ) == WRT_HEADER[3] ) {
    encoding = false;

    printf( "- decoding %s to %s\n", argv[optCount + 1], argv[optCount + 2] );
  } else {
    printf( "- encoding %s to %s\n", argv[optCount + 1], argv[optCount + 2] );
  }

  fileout = fopen( ( const char * ) argv[optCount + 2], "wb" );
  if( fileout == NULL ) {
    printf( "Can't open file %s\n", argv[optCount + 2] );
    fclose( file );
    return;
  }

  WRT_read_dict( encoding );

  if( encoding ) {
    putc( WRT_HEADER[0], fileout );
    putc( WRT_HEADER[1], fileout );
    putc( WRT_HEADER[2], fileout );
    putc( WRT_HEADER[3], fileout );

    unsigned int fileLen = flen( file );
    fwrite( ( void * ) &fileLen, sizeof( fileLen ), 1, fileout );

    coder.StartEncode();
#ifdef POWERED_BY_PAQ
    e.start( COMPRESS, fileout );
#endif
    WRT_encode( file, fileout, fileLen );
#ifdef POWERED_BY_PAQ
    e.flush();
#endif
    writeEOLstream( fileout );
  } else {
    readEOLstream( file );

    unsigned int fileLen;
    fseek( file, 4, SEEK_SET );                               // skip "WRTx" header
    fread( ( void * ) &fileLen, sizeof( fileLen ), 1, file ); // decompressedfileLen

    printf( "decode fileLen=%d\n", fileLen );

#ifdef POWERED_BY_PAQ
    e.start( DECOMPRESS, file );
#endif
    WRT_decode( file, fileout, fileLen );
  }

  fclose( fileout );
  fclose( file );

  WRT_deinitialize();
}
