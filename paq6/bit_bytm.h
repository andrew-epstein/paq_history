/*
 * modifed in December 2003 by David Scott for less expansion in compression
 * decompression programs same abount of data in bit file byte file could
 * change a max of one byte. Still a complete bijective transform but pseudo
 * random bijective transform. modified for matt 32bit max
 */

/*
 * modifed in Qctober 2002 by David Scott for delayed -2 in write state wz()
 * and wzc() routines
 */
struct bit_byts {
public:
  bit_byts() {
    xx();
  }
  void ir( FILE *fr ) { /* open file for bit read FOF assumed */
    CHK();
    inuse = 0x01;
    f = fr;
    bn = getc( f );
    if( bn == EOF ) {
      fprintf( stderr, " empty file in bit_byts \n" );
      abort();
    }
  }
  void irc( FILE *fr ) { /* open file for 01 read FOF assumed */
    CHK();
    inuse = 0x01;
    f = fr;
    bn = getc( f );
    if( ( bn != ( int ) '1' ) && ( bn != ( int ) '0' ) ) {
      fprintf( stderr, " empty file in bit_byts \n" );
      abort();
    }
  }
  int irr( FILE *frr ) { /* open and read first bit */
    ir( frr );
    return r();
  }
  int r(); /* get next bit */

  int rc();             /* get next ASCII 1 or 0 */
  void iw( FILE *fw ) { /* open file for bit write FOF
                      * assumed */
    CHK();
    inuse = 0x02;
    f = fw;
  }
  int iww( FILE *fww, int b ) { /* open and write first bit */
    iw( fww );
    return w( b );
  }
  int w( int );  /* write next bit */
  int wc( int ); /* write next ASCII 1 or 0 */
  int status() {
    return inuse;
  }

  /* on read 0 if normal -1 if last bit -2 there after */
  /* on write -1 if current is last -2 if call after last one */
  int wz( int c ) {
    if( c == -2 )
      return w( -2 );
    if( c == 0 ) {
      bn++;
      return 0;
    } else {
      for( ; bn > 0; bn-- )
        w( 0 );
      return w( c );
    }
  }
  int wzc( int c ) {
    if( c == -2 )
      return wc( -2 );
    if( c == 0 ) {
      bn++;
      return 0;
    } else {
      for( ; bn > 0; bn-- )
        wc( 0 );
      return wc( c );
    }
  }

private:
  void CHK() {
    if( inuse != 0x69 ) {
      fprintf( stderr, " all read in use bit_byts use error %x \n", inuse );
      abort();
    }
  }
  void xx() {
    inuse = 0x69;
    M = 0x80;
    l = 0;
    f = NULL;
    bn = 0;
    bo = 0;
    zerf = 0;
    onef = 0;
    zc = 0;
  }
  int inuse;
  int zerf;
  int onef;
  int bn;
  int bo;
  int l;
  int M;
  long zc;
  FILE *f;
};
