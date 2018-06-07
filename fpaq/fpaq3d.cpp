// fpaq3d - Stationary order 0X bits file compressor.
// (C) 2002, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt
// To compile: g++ -O fpaq3d.cpp
// 28/12/2006 modified by Nania Francesco Antonio (Italia - Merì (ME))
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cassert>
namespace std {}  // for MARS compiler
using namespace std;
typedef unsigned int U32;  // 32 bit type
unsigned long filesize,pos,control,maxsize,scanned,memo;
unsigned long rc[2],cc[2],r2,r1,conta,sce;  // Context: last 0-8 bits with a leading 1
unsigned char epix,mbit;  // 0 and 1 state count in context
unsigned short cuf[256][256];
unsigned short duf[256];
unsigned char *buffer;
unsigned char *buf;
  // Assume a stationary order 3 stream of 9-bit symbols
  unsigned long len(unsigned long number)
{
  unsigned long nbit;
  nbit=1;
  bscan:
  if ((number>>1)>0)
    {
   number=number>>1;
   nbit++;
   goto bscan;
  }
  return (nbit);
}
  unsigned char Peekb(unsigned char *memory)
  {
  unsigned char result;
  memcpy(&result,memory,1);
  return (result);
  }
void Pokeb(unsigned char *memory,unsigned char value)
  {
  memcpy(memory,&value,1);
  }
  int p()
  {
  return 4096*(Peekb(buf+rc[1])+1)/(Peekb(buf+rc[1])+Peekb(buf+rc[0])+2);
  }

void update(int y)
{
    if (Peekb(buf+rc[y])+8<255)
    Pokeb(buf+rc[y],Peekb(buf+rc[y])+8) ;
    else
    {
    Pokeb(buf+rc[ y],Peekb(buf+rc[ y])>>1) ;
    Pokeb(buf+rc[!y],Peekb(buf+rc[!y])>>1) ;
    }
    rc[ y]>>=1;          //
    rc[ y]+=(1<<mbit);  // old 28 state of     y [0001...................01] count
    rc[!y]>>=1;          // old 28 state of not y [1110...................10] count

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

typedef enum {COMPRESS, DECOMPRESS} Mode;
class Encoder {
private:

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
Encoder::Encoder(Mode m, FILE* f):  mode(m), archive(f), x1(0),
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

/* encode(y) -- Encode bit y by splitting the range [x1, x2] in proportion
to P(1) and P(0) as given by the predictor and narrowing to the appropriate
subrange.  Output leading bytes of the range as they become known. */

inline void Encoder::encode(int y) {

  // Update the range
  const U32 xmid = x1 + ((x2-x1) >> 12) * p();
  assert(xmid >= x1 && xmid < x2);
  if (y)
    x2=xmid;
  else
    x1=xmid+1;

  update(y);

  // Shift equal MSB's out
  while (((x1^x2)&0xff000000)==0) {
    putc(x2>>24, archive);
    x1<<=8;
    x2=(x2<<8)+255;
  }
}

/* Decode one bit from the archive, splitting [x1, x2] as in the encoder
and returning 1 or 0 depending on which subrange the archive point x is in.
*/
inline int Encoder::decode() {

  // Update the range
  const U32 xmid = x1 + ((x2-x1) >> 12) * p();
  assert(xmid >= x1 && xmid < x2);
  int y=0;
  if (x<=xmid) {
    y=1;
    x2=xmid;
  }
  else
    x1=xmid+1;

  update(y);

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

// Should be called when there is no more to compress
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
void endx()
{
   printf( "Fpaq3d file compressor/archiver, (C) 2006 By Nania Francesco Antonio (Italy)\n"
           "based on Fpaq0 (C) 2002 Matt Mahoney, mmahoney@cs.fit.edu\n"
           "This program is free software distributed without warranty under the terms\n"
           "of the GNU General Public License, see http://www.gnu.org/licenses/gpl.txt\n"
           "                              INSTRUCTION                                  \n"
           "To compress:   fpaq3d c [memory option] input output \n"
           "To decompress: fpaq3d d input output\n"
           "Compression option  Memory needed to compress/decompress\n"
           "------------------  ------------------------------------\n"
           " 0                   16 MB (fastest)\n"
           " 1                   32 MB\n"
           " 2                   64 MB\n"
           " 3                  128 MB\n"
           " 4                  256 MB\n"
           " 5                  512 MB\n"
           " 6                    1 GB\n"
           " 7                    2 GB (slowest)\n"
           );
exit(1);
}
//////////////////////////// main ////////////////////////////

int main(int argc, char** argv) {

  // Chech arguments: fpaq3d c/d option input output
  if (  argc<4 || argc>5 )
  {
  endx();
  return 1;
  }
  if (  argv[1][0]!='c' && argv[1][0]!='d')
  {
  endx();
  return 1;
  }

  if (argv[1][0]=='c' && (argv[2][0]!='0' && argv[2][0]!='1'&&
argv[2][0]!='2' && argv[2][0]!='3' && argv[2][0]!='4' && argv[2][0]!='5'&&
argv[2][0]!='6' && argv[2][0]!='7') )
  {
  endx();
  }
   if (argv[1][0]=='c' && argc!=5 )
  {
    endx();
  }
    if (argv[1][0]=='d' && argc!=4 )
  {
    endx();
  }

  // Start timer
  clock_t start = clock();
  FILE *in;
  FILE *out;
  // Open files
  if (argv[1][0]=='c')
  {
  if (argv[2][0]=='0') {mbit=23;printf(" Memory : 16 Mb\n");}
  if (argv[2][0]=='1') {mbit=24;printf(" Memory : 32 Mb\n");}
  if (argv[2][0]=='2') {mbit=25;printf(" Memory : 64 Mb\n");}
  if (argv[2][0]=='3') {mbit=26;printf(" Memory :128 Mb\n");}
  if (argv[2][0]=='4') {mbit=27;printf(" Memory :256 Mb\n");}
  if (argv[2][0]=='5') {mbit=28;printf(" Memory :512 Mb\n");}
  if (argv[2][0]=='6') {mbit=29;printf(" Memory :  1 Gb\n");}
  if (argv[2][0]=='7') {mbit=30;printf(" Memory :  2 Gb\n");}
  in=fopen(argv[3], "rb");
  if (!in) perror(argv[3]), exit(1);
  out=fopen(argv[4], "wb");
  if (!out) perror(argv[4]), exit(1);
  }
  else
  {
  in=fopen(argv[2], "rb");
  if (!in) perror(argv[2]), exit(1);
  out=fopen(argv[3], "wb");
  if (!out) perror(argv[3]), exit(1);
  }

  int c;
  maxsize=1<<20;
  buffer = (unsigned char *)malloc(maxsize);
  memset(cuf, 0, sizeof(cuf));
  memset(duf, 0, sizeof(duf));
  // Compress
  if (argv[1][0]=='c')
    {
    memo=1ULL<<(mbit+1);
    buf = (unsigned char *)malloc(memo);
    memset(buf, 0, sizeof(buf));
    while ((c=getc(in))!=EOF) // reduction of bits to encode
    {
    if (cuf[r1][c]==0)
    {cuf[r1][c]=1;duf[r1]++;}
    r1=c;
    }
    rewind (in);
    fseek (in , 0 , SEEK_END);
    filesize = ftell (in);
    rewind (in);
    fseek(out,0,SEEK_SET);
    fwrite (&filesize , 1 , 4 , out);
    fwrite (&mbit , 1 , 1 , out);
    Encoder e(COMPRESS, out);
    for (r1=0; r1<256; r1++)
    {
    if (duf[r1]>0)
    {
    e.encode(1);
    conta=0;
    for (r2=0; r2<256; r2++)
    {
    e.encode(cuf[r1][r2]);
    if(cuf[r1][r2]==1){cuf[r1][r2]=conta;conta++;}
    }
     }
     else
     e.encode(0);
    }
    memset(buf, 0, sizeof(buf));
    r1=0;
    newscan:
    if (scanned+maxsize>filesize) maxsize=filesize-scanned;
    pos=0;
    fread (buffer , 1 , maxsize , in);
    while (pos<maxsize)
    {
    c=Peekb(buffer+pos);
    for (int i=len(duf[r1]-1)-1; i>=0; --i)
    e.encode((cuf[r1][c]>>i)&1);
    r1=c;
    pos++;
    if (scanned+pos>control*(filesize/79))
    {
    printf(">");
    control++;
    }
    }
    scanned+=maxsize;
    if (scanned<filesize) goto newscan;
    e.flush();
  }

  // Decompress
  else {
    fseek(in,0,SEEK_SET);
    fread (&filesize , 1 , 4 , in);
    fread (&mbit , 1 , 1 , in);
    memo=1<<(mbit+1);
    buf = (unsigned char *)malloc(memo);
    memset(buf, 0, sizeof(buf));
    Encoder e(DECOMPRESS, in);
    for (r1=0; r1<256; r1++)
    {
    if (e.decode())
    for (r2=0; r2<256; r2++)
    {
    sce=e.decode();
    if (sce==1)
     {
     cuf[r1][duf[r1]]=r2;
     duf[r1]++;
     }
    }
    }
    memset(buf, 0, sizeof(buf));
    r1=0;
    newdscan:
    if (scanned+maxsize>filesize) maxsize=filesize-scanned;
    pos=0;
    while (pos<maxsize)
    {
     c=0;
     for (int i=len(duf[r1]-1)-1; i>=0; --i)
     c+=e.decode()*(1<<i);
     Pokeb(buffer+pos,cuf[r1][c]);
     r1=cuf[r1][c];
    if (scanned+pos>control*(filesize/79))
    {
    printf(">");
    control++;
    }
     pos++;
    }
    fwrite (buffer , 1 , maxsize , out);
    scanned+=maxsize;
    if (scanned<filesize) goto newdscan;
  }

  // Print results
  if (argv[1][0]=='c')
  {
  printf("%s (%ld bytes) -> %s (%ld bytes) in %1.2f s.\n",
  argv[3], ftell(in), argv[4], ftell(out)
  ,((double)clock()-start)/CLOCKS_PER_SEC);
  }
  else
    {
  printf("%s (%ld bytes) -> %s (%ld bytes) in %1.2f s.\n"
  ,argv[2], ftell(in), argv[3], ftell(out)
  ,((double)clock()-start)/CLOCKS_PER_SEC);
  }
  return 0;
}
