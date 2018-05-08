@echo off

goto setup


Tested on WinXP Home SP2 and Vista Home Premium SP1
rugxulo _AT_ gmail _DOT_ com

DJDEV204, GCC423B, GPP423B, /current/BNU217B, UPX 3.03, NASM 2.02
(avoid /beta/'s stupid BNU217B versions that are 686+ only !!)
http://short.stop.home.att.net/lists/djgpp.htm

Also used OpenWatcom 1.7a-RC1 (for Win32 target)
http://www.openwatcom.org/index.php/Alternative_Open_Watcom_distribution
http://www.ibiblio.org/pub/micro/pc-stuff/freedos/files/devel/c/openwatcom/


:setup
for %%a in (clea clen clan clena claen clane) do if "%1"=="%%a" %0 clean
if not "%WATCOM%"=="" if "%1"=="tiny" goto fino
if not "%DJGPP%"=="" if "%1"=="dos" goto fino
if "%1"=="nopack" set NOPACK=-nowfse
if "%1"=="nopack" shift
if "%1"=="debug" set DEBUG=2
if "%1"=="debug" set NOPACK=-nowfse
if "%1"=="debug" shift
if "%1"=="hdpmi" set HDPMI=1
if "%1"=="hdpmi" shift
if "%1"=="help" goto help
if "%1"=="Help" goto help
if "%1"=="/help" goto help
if "%1"=="--help" goto help
if "%1"=="h" goto help
if "%1"=="H" goto help
if "%1"=="-h" goto help
if "%1"=="-H" goto help
if "%1"=="/h" goto help
if "%1"=="/H" goto help
set S1=tonystub
set P=paq8o8z
if not "%1"=="dos" if not "%1"=="clean" if "%WATCOM%\H"=="%INCLUDE%" %0 dos

for %%a in (%P%*.exe) do if exist %%a del %%a
if "%1"=="clean" goto bye

if "%DOS4G%"=="" set DOS4G=quiet

if not "%WATCOM%"=="" goto watcom
if not "%DJGPP%"=="" goto djgpp
echo.
echo No compiler found. You need either DJGPP or OpenWatcom.
echo.
goto bye

:djgpp
set GCCBAK=%GCCOPT%
if not "%GCCBAK%"=="" echo %%GCCOPT%%=%GCCOPT%
echo.
echo Compiling via DJGPP ...
set NASMENV=-fcoff --prefix _
if exist fast-paq.asm nasm fast-paq.asm
set NASMENV=

if not "%DEBUG%"=="" echo.
if not "%DEBUG%"=="" echo Making *slower*-running debug (fast compile) build!
if not "%DEBUG%"=="" goto djfast
REM Bah, 4.3.0 is majorly whiny in C++ now due to -Wall using -Wparentheses !
set GCCOPT=-O3 -Wall -Wno-parentheses

:djfast
if not "%1"=="tiny" goto dj
REM N.B. this disables wildcard globbing !!
set GCCOPT=%GCCOPT% -s -Os -DTINY
shift
echo.
echo Making *slower*-running tiny build! (Not for typical use!)

:dj
if not exist fast-paq.o echo.
if not exist fast-paq.o echo Where's the NASM file ??
set FPOBJ=-DNOASM
if exist fast-paq.o set FPOBJ=fast-paq.o
echo on
redir -t gpp %GCCOPT% -mtune=i686 %1 %2 %3 %FPOBJ% %P%.cpp -o %P%.exe
@echo off
REM Use WDOSX so it can use VCPI (ring 0) under EMM386 to enable SSE (in CR4)
REM (only necessary for real DOS w/o HDPMI32)
set GCCOPT=%GCCBAK%

:upx
echo.
REM if exist %P%*.exe upx -qq --ultra-brute %P%*.exe
REM if exist %P%*.exe upx -qq --best --lzma --all-filters %P%*.exe
if "%NOPACK%"=="" if exist %P%*.exe upx -qq --best %P%*.exe
if exist %P%.exe if "%HDPMI%"=="" if "%WATCOM%"=="" stubit %NOPACK% %P%.exe >NUL
if exist %P%.exe if not "%HDPMI%"=="" stubedit %P%.exe dpmi=hdpmi32.exe
if exist %P%.bak del %P%.bak
if "%DJGPP%"=="" goto end
if not "%DJGPP%"=="" goto debug

:watcom
if "%OS%"=="DRDOS" set TIME=%HOUR%:%MINUTE%:%SECOND%%AM_PM%
set CCTIME=Started compile at %TIME% and ended at
if not "%_CWD%"=="" time /t

set WATBAK=%WATOPT%
if not "%WATBAK%"=="" echo %%WATOPT%%=%WATOPT%
if not "%WCL386%"=="" echo %%WCL386%%=%WCL386%
echo.
echo Compiling via OpenWatcom ...
set WATOPT=-od
if "%DEBUG%"=="" set WATOPT=-ox

:wat
if not "%DEBUG%"=="" echo.
if not "%DEBUG%"=="" echo Making *slower*-running debug (fast compile) build!
set NASMENV=-fobj
if "%1"=="dos" goto watdos
if exist %S1%.c wcl -bcl=dos -q -os -s %S1%.c
if exist fast-paq.asm nasm -DTWEAK fast-paq.asm
REM These still run on a 386, supposedly, despite "-6s" etc (a la "-mtune").
if not exist fast-paq.obj echo.
if not exist fast-paq.obj echo Where's the NASM file ??
set FPOBJ=-dNOASM
if exist fast-paq.obj set FPOBJ=fast-paq.obj
REM FAST-PAQ.OBJ must be used/linked before PAQ8O8Z.CPP (else SSE2 error) !!

REM wcl386 %WATOPT% -xs -q -6s -fe=%P%w -bt=nt -"op stub=%S1%" %1 %2 %3 %FPOBJ% %P%.cpp
echo on
wpp386 %WATOPT% -xs -q -6s -bt=nt %1 %2 %3 %P%.cpp
@echo off
if exist %S1%.exe wlink op q sys nt file %FPOBJ%,%P% op stub=%S1% name %P%w
if not exist %S1%.exe wlink op q sys nt file %FPOBJ%,%P% name %P%w

REM N.B. You can even run this Win32 .EXE under pure DOS via HXRT emulation!
REM http://www.japheth.de/HX.html
REM
REM DOS version (CWSTUB.EXE 4.02 from Dec. 18 2004, *not* OpenWatcom's 3.60)
REM ftp://ftp.devoresoftware.com/downloads/cw402.zip   (66k)
REM N.B. I did "upx --ultra-brute --8086" it & replaced \BINW\CWSTUB.EXE
REM (this is why it's smaller than typical Causeway stub, if you're curious)
REM
REM DOS/32A would work too, but again, not OpenWatcom's 7.2, only 9.1.x
REM http://download.narechk.net/dos32a-912-bin.zip     (210k)
REM (smaller than Causeway but no VM swapping, bah!)
:watdos
if "%1"=="dos" shift
if exist fast-paq.asm nasm fast-paq.asm -o fp.obj
set NASMENV=
if not exist fp.obj echo.
if not exist fp.obj echo Where's the NASM file ??
set FPOBJ=-dNOASM
if exist fp.obj set FPOBJ=fp.obj

set WATOPT=%WATOPT% -xs -q -6s
echo on
wcl386 %WATOPT% -fe=%P%c %1 %2 %3 -bt=dos -l=causeway %FPOBJ% %P%.cpp
@echo off

set WATOPT=%WATBAK%
if "%OS%"=="DRDOS" set TIME=%HOUR%:%MINUTE%:%SECOND%%AM_PM%
set CCTIME=%CCTIME% %TIME%

goto upx

:help
echo.
if exist %0 echo %0
if exist %0.bat echo %0.bat
echo.
echo %%DJGPP%% == '%DJGPP%', %%WATCOM%% == '%WATCOM%'
echo %%WCL%% == '%WCL%', %%WCL386%% == '%WCL386%'
echo %%GCCOPT%% == '%GCCOPT%', %%WATOPT%% == '%WATOPT%'
echo.
echo '%0 help'
echo '%0 clean'    (delete unnecessary files)
echo '%0 nopack'   (no compression of .EXEs)
echo '%0 debug'    (compile faster, then test compression)
echo '%0 dos'      (Watcom only)  DOS/Causeway target only
echo '%0 tiny'     (DJGPP only )  make tiny .EXE
echo '%0 hdpmi'    (DJGPP only )  use HDPMI32 instead of WDOSX
echo.
goto fino

:end
if not "%OS%"=="" echo.
if not "%OS%"=="" echo %CCTIME%
if not "%OS%"=="" echo.
if not "%_CWD%"=="" time /t

:debug
if "%DEBUG%"=="" goto bye
if exist %P%.exe paq8o8z doydoy paq8o8z.cpp
if exist %P%.exe goto bye
if "%OS%"=="Windows_NT" if exist %P%w.exe %P%w doydoy %P%.cpp
if not exist %P%w.exe if exist %P%c.exe %P%c doydoy %P%.cpp

:bye
for %%a in (lnk err obj o) do if exist *.%%a del *.%%a
if exist doydoy*.paq del doydoy*.paq
if exist %S1%.exe del %S1%.exe
set GCCBAK=
set WATBAK=
set CCTIME=
set NOPACK=
set HDPMI=
set FPOBJ=
set TIME=
set S1=
set P=
if "%DEBUG%"=="2" set DEBUG=
if "%DOS4G%"=="quiet" set DOS4G=

:fino
