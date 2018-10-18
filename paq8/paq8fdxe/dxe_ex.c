/* DXE_EX.C -- stupid DXE example for DJGPP 2.03p2 and DJGPP 2.04
	rugxulo _AT_ gmail _DOT_ com
	http://rugxulo.googlepages.com
  @echo off
  gcc -DYO -c dxe_ex.c -o yo.o
  redir -eo ls /dev/env/DJDIR/bin/dxe3gen.exe >NUL
  if errorlevel 1 redir -eo dxegen yo.dxe _yo yo.o >NUL
  if not exist yo.dxe redir -eo dxe3gen -o yo.dxe yo.o
  if not exist yo.dxe redir -eo dxegen yo.dxe _yo yo.o >NUL
  gcc dxe_ex.c -o dxe_ex.exe
  if exist dxe_ex.exe dxe_ex.exe
  for %%a in (yo.dxe yo.o dxe_ex.exe) do if exist %%a del %%a
*/
#if !defined( DXE1 ) && !defined( DXE3 )
#  if __DJGPP_MINOR__ == 3
#    define DXE1
#  endif
#endif
#ifdef YO
yo() {
  asm( ".intel_syntax noprefix" );
  asm( "mov al,0x48" ); /* "H" */
  asm( "int 0x29" );    /* output char in AL to screen */
  asm( "mov al,0x49" ); /* "I" */
  asm( "int 0x29" );
  asm( ".att_syntax prefix" );
}
#else
#  ifdef DXE1
#    include <sys/dxe.h>
#  else
#    include <dlfcn.h>
#  endif
#  include <stdio.h>

int main() {
  static void ( *yo )(), *blah;

#  ifdef DXE1
  if( yo = _dxe_load( "yo.dxe" ) )
    yo();
#  else
  if( blah = dlopen( "yo.dxe", RTLD_GLOBAL ) ) {
    yo = dlsym( blah, "_yo" );
    ( *yo )();
  }
#  endif
  else
    puts( "\nCannot load YO.DXE" );
  return 0;
}
#endif
