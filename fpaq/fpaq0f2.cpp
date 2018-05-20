/* fpaq0f2 - Adaptive order 0 file compressor.
(C) 2008, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt

To compile:    g++ -O2 -s -fomit-frame-pointer -march=pentiumpro fpaq0f2.cpp
To compress:   fpaq0f2 c input output
To decompress: fpaq0f2 d input output

fpaq0f2 is an order-0 file compressor and arithmetic coder.  Each bit is
modeled in the context of the previous bits in the current byte, plus
the bit history (last 8 bits) observed in this context.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define NDEBUG  // remove for debugging
#include <assert.h>

// 8, 16, 32 bit unsigned types (adjust as appropriate)
typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int   U32;

// Create an array p of n elements of type T
template <class T> void alloc(T*&p, int n) {
  p=(T*)calloc(n, sizeof(T));
  if (!p) fprintf(stderr, "out of memory\n"), exit(1);
}

//////////////////////////// StateMap //////////////////////////

// A StateMap maps a context to a probability.  After a bit prediction, the
// map is updated in the direction of the actual value to improve future
// predictions in the same context.

class StateMap {
protected:
  const int N;  // Number of contexts
  int cxt;      // Context of last prediction
  U32 *t;       // cxt -> prediction in high 24 bits, count in low 8 bits
  static int dt[256];  // reciprocal table: i -> 16K/(i+1.5)
public:
  StateMap(int n=256);  // create allowing n contexts

  // Predict next bit to be updated in context cx (0..n-1).
  // Return prediction as a 16 bit number (0..65535) that next bit is 1.
  int p(int cx) {
    assert(cx>=0 && cx<N);
    return t[cxt=cx]>>16;
  }

  // Update the model with bit y (0..1).
  // limit (1..255) controls the rate of adaptation (higher = slower)
  void update(int y, int limit=255) {
    assert(cxt>=0 && cxt<N);
    assert(y==0 || y==1);
    assert(limit>=0 && limit<255);
    int n=t[cxt]&255, p=t[cxt]>>14;  // count, prediction
    if (n<limit) ++t[cxt];
    t[cxt]+=((y<<18)-p)*dt[n]&0xffffff00;
  }
};

int StateMap::dt[256]={0};

// Initialize assuming low 8 bits of context is a bit history.
StateMap::StateMap(int n): N(n), cxt(0) {
  alloc(t, N);
  for (int i=0; i<N; ++i) {
    // Count 1 bits to determine initial probability.
    U32 n=(i&1)*2+(i&2)+(i>>2&1)+(i>>3&1)+(i>>4&1)+(i>>5&1)+(i>>6&1)+(i>>7&1)+3;
    t[i]=n<<28|6;
  }
  if (dt[0]==0)
    for (int i=0; i<256; ++i)
      dt[i]=32768/(i+i+3);
}

//////////////////////////// Predictor /////////////////////////

/* A Predictor estimates the probability that the next bit of
   uncompressed data is 1.  Methods:
   p() returns P(1) as a 16 bit number (0..65535).
   update(y) trains the predictor with the actual bit (0 or 1).
*/

class Predictor {
  int cxt;  // Context: 0=not EOF, 1..255=last 0-7 bits with a leading 1
  StateMap sm;
  int state[256];
public:
  Predictor();

  // Assume order 0 stream of 9-bit symbols
  int p() {
    return sm.p(cxt<<8|state[cxt]);
  }

  void update(int y) {
    sm.update(y, 90);
    int& st=state[cxt];
    (st+=st+y)&=255;
    if ((cxt+=cxt+y) >= 256)
      cxt=0;
  }
};

Predictor::Predictor(): cxt(0), sm(0x10000) {
  for (int i=0; i<0x100; ++i)
    state[i]=0x66;
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

typedef enum {COMPRESS, DECOMPRESS} Mode;
class Encoder {
private:
  Predictor predictor;   // Computes next bit probability (0..65535)
  const Mode mode;       // Compress or decompress?
  FILE* archive;         // Compressed data file
  U32 x1, x2;            // Range, initially [0, 1), scaled by 2^32
  U32 x;                 // Last 4 input bytes of archive.
public:
  Encoder(Mode m, FILE* f);
  void encode(int y);    // Compress bit y
  int decode();          // Uncompress and return bit y
  void flush();          // Call when done compressing
};

// Constructor
Encoder::Encoder(Mode m, FILE* f): predictor(), mode(m), archive(f), x1(0),
                                   x2(0xffffffff), x(0) {

  // In DECOMPRESS mode, initialize x to the first 4 bytes of the archive
  if (mode==DECOMPRESS) {
    for (int i=0; i<4; ++i) {
      int c=getc(archive);
      if (c==EOF) c=0;
      x=(x<<8)+(c&0xff);
    }
  }
}

// encode(y) -- Encode bit y by splitting the range [x1, x2] in proportion
// to P(1) and P(0) as given by the predictor and narrowing to the appropriate
// subrange.  Output leading bytes of the range as they become known.

inline void Encoder::encode(int y) {

  // Update the range
  const U32 p=predictor.p();
  assert(p<=0xffff);
  assert(y==0 || y==1);
  const U32 xmid = x1 + (x2-x1>>16)*p + ((x2-x1&0xffff)*p>>16);
  assert(xmid >= x1 && xmid < x2);
  if (y)
    x2=xmid;
  else
    x1=xmid+1;
  predictor.update(y);

  // Shift equal MSB's out
  while (((x1^x2)&0xff000000)==0) {
    putc(x2>>24, archive);
    x1<<=8;
    x2=(x2<<8)+255;
  }
}

// Decode one bit from the archive, splitting [x1, x2] as in the encoder
// and returning 1 or 0 depending on which subrange the archive point x is in.

inline int Encoder::decode() {

  // Update the range
  const U32 p=predictor.p();
  assert(p<=0xffff);
  const U32 xmid = x1 + (x2-x1>>16)*p + ((x2-x1&0xffff)*p>>16);
  assert(xmid >= x1 && xmid < x2);
  int y=0;
  if (x<=xmid) {
    y=1;
    x2=xmid;
  }
  else
    x1=xmid+1;
  predictor.update(y);

  // Shift equal MSB's out
  while (((x1^x2)&0xff000000)==0) {
    x1<<=8;
    x2=(x2<<8)+255;
    int c=getc(archive);
    if (c==EOF) c=0;
    x=(x<<8)+c;
  }
  return y;
}

// Should be called when there is no more to compress.
void Encoder::flush() {

  // In COMPRESS mode, write out the remaining bytes of x, x1 < x < x2
  if (mode==COMPRESS) {
    while (((x1^x2)&0xff000000)==0) {
      putc(x2>>24, archive);
      x1<<=8;
      x2=(x2<<8)+255;
    }
    putc(x2>>24, archive);  // First unequal byte
  }
}

//////////////////////////// main ////////////////////////////

int main(int argc, char** argv) {

  // Chech arguments: fpaq0 c/d input output
  if (argc!=4 || (argv[1][0]!='c' && argv[1][0]!='d')) {
    printf("To compress:   fpaq0f2 c input output\n"
           "To decompress: fpaq0f2 d input output\n");
    exit(1);
  }

  // Start timer
  clock_t start = clock();

  // Open files
  FILE *in=fopen(argv[2], "rb");
  if (!in) perror(argv[2]), exit(1);
  FILE *out=fopen(argv[3], "wb");
  if (!out) perror(argv[3]), exit(1);
  int c;

  // Compress each byte as 9 bits as 1xxxxxxxx, then EOF as 0.
  if (argv[1][0]=='c') {
    Encoder e(COMPRESS, out);
    while ((c=getc(in))!=EOF) {
      e.encode(1);
      for (int i=7; i>=0; --i)
        e.encode((c>>i)&1);
    }
    e.encode(0);  // EOF code
    e.flush();
  }

  // Decompress
  else {
    Encoder e(DECOMPRESS, in);
    while (e.decode()) {
      int c=1;
      while (c<256)
        c+=c+e.decode();
      putc(c-256, out);
    }
  }

  // Print results
  printf("%s (%ld bytes) -> %s (%ld bytes) in %1.2f s.\n",
    argv[2], ftell(in), argv[3], ftell(out),
    ((double)clock()-start)/CLOCKS_PER_SEC);
  return 0;
}
