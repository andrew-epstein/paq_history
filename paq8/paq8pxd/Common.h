#if !defined common_h_included
#  define common_h_included

#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <vector>
#  include "MemBuffer.h"

class XWRT_Common {
public:
  XWRT_Common( int fileBufferSize = 17 ); // 128 kb
  ~XWRT_Common();

  void defaultSettings();
  unsigned int flen( FILE *&f );

  CContainers cont;

  EPreprocessType preprocType;
  int preprocFlag;

protected:
  inline void stringHash( const unsigned char *ptr, int len, int &hash );
  int addWord( unsigned char *&mem, int &i );
  unsigned char *loadDynamicDictionary( unsigned char *mem, unsigned char *mem_end );
  //unsigned char* loadDictionary(FILE* file,unsigned char* mem,int word_count);
  //void loadCharset(FILE* file);
  void tryAddSymbol( int c );
  void initializeLetterSet();
  void initializeCodeWords( int word_count, bool initMem = true );
  bool initialize( bool encoding );
  void WRT_deinitialize();

  //int getSourcePath(char* buf, int buf_size);
  void WRT_print_options();
  int minSpacesFreq();

  int *word_hash;
  EPreprocessType codewordType;
  EWordType subWordType;
  bool decoding, fileCorrupted, detect, firstWarn;
  int maxDynDictBuf, minWordFreq, compLevel, maxDictSize, additionalParam, firstPassBlock;
  int tryShorterBound, spaces, fileLenMB, beforeWord;
  int spacesCodeword[256];
  int spacesCont[256];
  std::vector<std::string> sortedDict;

  ELetterType letterType;
  ELetterType letterSet[256];

  int sizeDict, sizeDynDict;
  unsigned char *dictmem;
  unsigned char *dictmem_end;
  unsigned char *mem;

  std::vector<std::string> stack;

  int addSymbols[256];  // reserved symbols in output alphabet
  int reservedSet[256]; // reserved symbols in input alphabet
  int outputSet[256];
  int reservedFlags[256];
  int wordSet[256];
  int startTagSet[256];
  int urlSet[256];
  int whiteSpaceSet[256];
  int sym2codeword[256];
  int codeword2sym[256];
  int value[256];
  int detectedSymbols[32];
  unsigned char num[16];
  size_t mFileBufferSize;

  int dictionary, dict1size, dict2size, dict3size, dict4size, dict1plus2plus3, dict1plus2;
  int bound4, bound3, dict123size, dict12size, collision, quoteOpen, quoteClose, detectedSym;
  int maxMemSize;
  int sortedDictSize;

public:
};

size_t fread_fast( unsigned char *dst, int len, FILE *file );
size_t fwrite_fast( unsigned char *dst, int len, FILE *file );

extern unsigned char **dict;
extern int *dictfreq;
extern unsigned char *dictlen;

#endif
