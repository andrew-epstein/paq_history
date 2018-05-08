#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void main(int argc,char* argv[])
{
  FILE *sf, *tf;
  int cc, i, ts, tr;

  if (argc<4)	printf("USAGE: inputfile outputfile <bytes_to_skip> <bytes_to_move>"), exit(0);

  sf = fopen(argv[1],"rb");
  tf = fopen(argv[2],"wb");
  if ( sf==0 || tf==0 )	printf("can't open file"), exit(0);

  ts = atoi(argv[3]);
  tr = atoi(argv[4]);

  for (i=0; i<ts; i++) cc=getc(sf);
  for (i=0; i<tr; i++) { cc=getc(sf); if (cc==EOF) break; putc(cc,tf); }

  fclose( sf );
  fclose( tf );
}
