#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void main(int argc,char* argv[])
{
  FILE *sf, *tf;
  int c, i;

  if (argc<5)	printf("USAGE: Split.exe inputfile outputfile <bytes_to_skip> <bytes_to_move>"), exit(0);
  sf = fopen(argv[1],"rb");
  tf = fopen(argv[2],"wb");
  if ( sf==0 || tf==0 )		printf("can't open file"), exit(0);

  fseek( sf, atoi(argv[3]), SEEK_SET );
  for (i=atoi(argv[4]); i!=0; i--) { c=getc(sf); if (c==EOF) break; putc(c,tf); }

  fclose( sf );
  fclose( tf );
}
