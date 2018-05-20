// fpaq0s4 - Stationary order 0 file compressor.
// (C) 2004, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt
// To compile: gcc -O fpaq0s.cpp -o fpaq0s.exe
// 10/01/2006 32 bit encoder modified, Fabio Buffoni
// jan 16 2006 improved compression, David Scott
// 11/10/2006 modified encoder (o0+o1+o2)+LZ+PPM ,(fast) Nania Francesco Antonio (Italy)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cassert>
namespace std {}  // for MARS compiler
using namespace std;

typedef unsigned char U8;
typedef unsigned long U32;  // 32 bit type

#define Top_value U32(0XFFFFFFFF)    /* Largest code value */
/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */
#define First_qtr U32(Top_value/4+1)  /* Point after first quarter    */
#define Half      U32(2*First_qtr)    /* Point after first half  */
#define Third_qtr U32(3*First_qtr)    /* Point after third quarter */

int EOS = 0; /* for terminating compression */
U32 rc,rc1,rc2,rc3,r1,r2,r3,r4,ubi,xmid,cobit,filesize;
unsigned short buf[16777216];
//////////////////////////// Predictor /////////////////////////

/* A Predictor estimates the probability that the next bit of
   uncompressed data is 1.  Methods:
   p() returns P(1) as a 12 bit number (0-4095).
   update(y) trains the predictor with the actual bit (0 or 1).
*/

  unsigned char cbuf[65793];
  int cxt,bcxt;  // Context: last 0-8 bits with a leading 1
  unsigned short ct[65793][256][2];  // 0 and 1 counts in context cxt
  unsigned short bct[65536][256][2];
  // Assume a stationary order 0 stream of 9-bit symbols
  int p() {
    return 4096*(ct[rc][cxt][1]+1)/(ct[rc][cxt][0]+ct[rc][cxt][1]+2);
  }

  void update(int y) {
    if (++ct[rc][cxt][y] > 65534)
    {
      ct[rc][cxt][0] >>= 1;
      ct[rc][cxt][1] >>= 1;
    }

    if (rc!=0)
    {
    if (++ct[rc2][cxt][y] > 65534)
    {
      ct[rc2][cxt][0] >>= 1;
      ct[rc2][cxt][1] >>= 1;
    }
    }

    if (rc!=rc1)
    {
    if (++ct[rc2][cxt][y] > 65534)
    {
      ct[rc2][cxt][0] >>= 1;
      ct[rc2][cxt][1] >>= 1;
    }
    }

    if (rc!=rc2)
    {
    if (++ct[rc2][cxt][y] > 65534)
    {
      ct[rc2][cxt][0] >>= 1;
      ct[rc2][cxt][1] >>= 1;
    }
    }


    if ((cxt+=cxt+y) > 255)
      cxt=1;
  }
  int bp() {
    return
4096*(bct[rc3][bcxt][1]+1)/(bct[rc3][bcxt][0]+bct[rc3][bcxt][1]+2);
  }

  void bupdate(int y) {
    if (++bct[rc3][bcxt][y] > 65534) {
      bct[rc3][bcxt][0] >>= 1;
      bct[rc3][bcxt][1] >>= 1;
    }
    if ((bcxt+=bcxt+y) > 255)
      bcxt=1;
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

  const Mode mode;       // Compress or decompress?
  FILE* archive;         // Compressed data file
  U32 x1, x2;            // Range, initially [0, 1), scaled by 2^32
  U32 x;                 // Last 4 input bytes of archive.
  U32 bits_to_follow;
  U8 bptr,bout,bptrin;
  int bin;
public:
  Encoder(Mode m, FILE* f);
  void encode(int y);    // Compress bit y
  int decode();          // Uncompress and return bit y
  void flush();          // Call when done compressing
  void bit_plus_follow(int bit);
  int input_bit(void);
};


inline void Encoder::bit_plus_follow(int bit)
{
    bits_to_follow++;
    for (int notb=bit^1; bits_to_follow > 0; bits_to_follow--, bit=notb) {
        if (bit) bout|=bptr;
        if (!(bptr>>=1)) {
            putc(bout,archive);
            bptr=128;
            bout=0;
        }
    }
}
inline int Encoder::input_bit(void)
{
    if (!(bptrin>>=1)) {
        bin=getc(archive);
        if (bin==EOF) {
                   bin=0;
                   EOS++;
                }
        bptrin=128;
    }
    return ((bin&bptrin)!=0);
}

// Constructor
Encoder::Encoder(Mode m, FILE* f): mode(m), archive(f), x1(0),
                                   x2(0xffffffff), x(0), bits_to_follow(0),
bptr(128),
bout(0), bptrin(1) {
  // In DECOMPRESS mode, initialize x to the first 4 bytes of the archive
  if (mode==DECOMPRESS) {
    x = 1;
    for (;x < Half;) x += x + input_bit();
    x += x + input_bit();
  }
}

/* encode(y) -- Encode bit y by splitting the range [x1, x2] in proportion
to P(1) and P(0) as given by the predictor and narrowing to the appropriate
subrange.  Output leading bytes of the range as they become known. */

inline void Encoder::encode(int y) {

  // Update the range
  if (ubi==0)
  xmid = x1 + ((x2-x1) >> 12) * p();
  else
  xmid = x1 + ((x2-x1) >> 12) * bp();
  assert(xmid >= x1 && xmid < x2);
  if (y)
    x2=xmid;
  else
    x1=xmid+1;
  if (ubi==0)
  update(y);
  else
  bupdate(y);

  cobit--;
  if (cobit==0) cobit=8;

  // Shift equal MSB's out
  for (;;) {
    if ( x2 < Half ) {
      bit_plus_follow(0);
    } else if (x1 >= Half) {
      bit_plus_follow(1);
    } else if (x1 >= First_qtr && x2 < Third_qtr) {
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
  if (ubi==0)
  xmid = x1 + ((x2-x1) >> 12) * p();
  else
  xmid = x1 + ((x2-x1) >> 12) * bp();
  assert(xmid >= x1 && xmid < x2);
  int y=0;
  if (x<=xmid) {
    y=1;
    x2=xmid;
  }
  else
    x1=xmid+1;
  if (ubi==0)
  update(y);
  else
  bupdate(y);

  // Shift equal MSB's out
  for (;;) {
      if ( x2 < Half ) {
      } else if (x1 >= Half) {    /* Output 1 if in high half. */
     x1 -= Half;
     x -= Half;
     x2 -= Half;        /* Subtract offset to top.  */
      } else if (x1 >= First_qtr    /* Output an opposite bit   */
         && x2 < Third_qtr) {    /* later if in middle half. */
     x1 -= First_qtr;    /* Subtract offset to middle */
     x -=  First_qtr;
     x2 -= First_qtr;
      } else {
     break;            /* Otherwise exit loop.     */
      }
      x1 += x1;
      x += x + input_bit();
      x2 += x2 + 1;    /* Scale up code range.     */
      if ( EOS > 6 || ( EOS > 0 && ( x << 2) == 0 )) {
            EOS = 100;
            if ( x == Half || x == 0 ) EOS = 10;
      }
   }
  return y;
}

// Should be called when there is no more to compress
void Encoder::flush() {
  // Update the range
    if (ubi==0)
  xmid = x1 + ((x2-x1) >> 12) * p();
  else
  xmid = x1 + ((x2-x1) >> 12) * bp();
  assert(xmid >= x1 && xmid < x2);
  if ( xmid < Half )
    x2=xmid;
  else
    x1=xmid+1;
  // Shift equal MSB's out
  for (;;) {
    if ( x2 < Half ) {
      bit_plus_follow(0);
    } else if (x1 >= Half) {
      bit_plus_follow(1);
    } else if (x1 >= First_qtr && x2 < Third_qtr) {
      bits_to_follow++;
      x1 ^= First_qtr;
      x2 ^= First_qtr;
    } else {
      break;
    }
    x1 += x1;
    x2 += x2 + 1;
  }
   if ( x1 <=  First_qtr ) {
     bit_plus_follow(0);
     bit_plus_follow(1);
   } else {
     bit_plus_follow(1);
     bit_plus_follow(1);
   }
   if (bout) putc(bout,archive);
}

//////////////////////////// main ////////////////////////////

int main(int argc, char** argv) {

  // Chech arguments: fpaq0 c/d input output
  if (argc!=4 || (argv[1][0]!='c' && argv[1][0]!='d')) {
    printf("To compress:   fpaq0s c input output\n"
           "To decompress: fpaq0s d input output\n");
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
  int ok;
  cobit=8;
    cxt=1;
  memset(ct, 0, sizeof(ct));
      bcxt=1;
  memset(bct, 0, sizeof(bct));



  // Compress
  if (argv[1][0]=='c') {
    fseek (in , 0 , SEEK_END);
    filesize = ftell (in);
    rewind (in);
    fseek(out,0,SEEK_SET);
    fwrite (&filesize , 1 , 4 , out);
    Encoder e(COMPRESS, out);
    while ((c=getc(in))!=EOF) {
      ok=0;
      if (buf[r3+(r2<<8)+(r1<<16)]>0 && (buf[r3+(r2<<8)+(r1<<16)]-1)==c) ok=1; // LZ active
      ubi=1;
      if (buf[r3+(r2<<8)+(r1<<16)]>0) e.encode(ok); // it comes compressed in base to an other analysis of frequency "bUdate"
      if (ok==0) // no LZ
      {
      ubi=0;
      for (int i=7; i>=0; --i)
      e.encode((c>>i)&1); // simple o0-o1-o2 // it comes compressed in base to an other analysis of frequency "Udate"
      }
        r4=r3;
        r3=r2;
        r2=r1;
        r1=c;

        rc=0; // PPM - o0
        rc1=1+r1;
        rc2=257+r1+(r2<<8);
        rc3=r1+(r2<<8);
        if (cbuf[rc1]>0) // PPM - o1
        rc=rc1;
        else
        cbuf[rc1]=1;

        if (cbuf[rc2]>0)  // PPM - o2
        rc=rc2;
        else
        cbuf[rc2]=1;

        buf[r4+(r3<<8)+(r2<<16)]=r1+1; // LZ
    }

    e.flush();
  }

  // Decompress
  else {
    fseek(in,0,SEEK_SET);
    fread (&filesize , 1 , 4 , in);
    Encoder e(DECOMPRESS, in);
    int c=1;
    while (filesize>0)
    {
      ubi=1;
      ok=0;
      if (buf[r3+(r2<<8)+(r1<<16)]>0) ok=e.decode();
      if (ok==0)
      {
      ubi=0;
      c = 1;
      while (c<256)
      {
        c+=c+e.decode();
        if ( EOS > 5 ) break;
      }
      if ( EOS > 5 ) break;
      putc(c-256, out);
      }
      else
      {
      putc(buf[r3+(r2<<8)+(r1<<16)]-1, out);
      c=(buf[r3+(r2<<8)+(r1<<16)]-1)+256;
      }
        r4=r3;
        r3=r2;
        r2=r1;
        r1=c-256;
        rc=0;
        rc1=1+r1;
        rc2=257+r1+(r2<<8);
        rc3=r1+(r2<<8);
        if (cbuf[rc1]>0)
        rc=rc1;
        else
        cbuf[rc1]=1;

        if (cbuf[rc2]>0)
        rc=rc2;
        else
        cbuf[rc2]=1;

        buf[r4+(r3<<8)+(r2<<16)]=r1+1;
        filesize--;
    }
       printf(" ** successful most likely error free decompression ** \n");
  }

  // Print results
  printf("%s (%ld bytes) -> %s (%ld bytes) in %1.2f s.\n",
    argv[2], ftell(in), argv[3], ftell(out),
    ((double)clock()-start)/CLOCKS_PER_SEC);
  return 0;
}
