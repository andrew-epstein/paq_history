#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#define CONSTA 292*1024+512
void te8e9(char*, int, int, int*);

void main(int argc,char* argv[])
{
  FILE *SourceFile, *TargetFile;
  int m,flen,data2write[4];
  char c, *st0, *st;
  char Usage[]="Usage:\nery c sourcefile compressedfile\nery d compressedfile decompressedfile\n";

  if ( argc<4 ) printf(Usage), exit(0);
	c=argv[1][0]; 
	SourceFile= fopen(argv[2],"rb");
	TargetFile= fopen(argv[3],"wb");
  if ( SourceFile==0 || TargetFile==0 || (c!='c' && c!='d')) printf(Usage), exit(0);

	fseek(SourceFile, 0L, SEEK_END),
	flen= ftell(SourceFile),
	fseek(SourceFile, 0L, 0);
    if(c=='c') m=3; else m=4;

    if ((st0=malloc(CONSTA+2*flen+256))==NULL) printf("Can't allocate memory\n"), exit(1);
    st=st0+256-((int)st0&255);	//te8e9() needs 256-byte-alignment
    flen=fread(st+CONSTA-32768,1,flen,SourceFile);

	if(flen) te8e9(st,m,flen,&data2write[0]);
      if (data2write[0])      fwrite((char *)data2write[1],1,data2write[0],TargetFile);
      if (data2write[2]&&m<4) fwrite((char *)data2write[3],1,data2write[2],TargetFile);
}
