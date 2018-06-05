/*
PAQAR 4.0 - Experimental version by Alexander Ratushnyak - File archiver and compressor.
(C) 2004, Matt Mahoney, mmahoney@cs.fit.edu, Alexander Ratushnyak, artest@inbox.ru
          Fabio Buffoni, David Scott, Jason Schmidt, Berto Destasio

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation at
http://www.gnu.org/licenses/gpl.txt or (at your option) any later version.
This program is distributed without any warranty.

USAGE

To compress:      PAQAR -3 archive file file...  (1 or more file names), or
  or (MSDOS):     dir/b | PAQAR -3 archive       (read file names from input)
  or (UNIX):      ls    | PAQAR -3 archive
To decompress:    PAQAR archive                  (no option)
To list contents: more < archive

Compression:  The files listed are compressed and stored in the archive,
which is created.  The archive must not already exist.  File names may
specify a path, which is stored.  If there are no file names on the command
line, then PAQAR prompts for them, reading until the first blank line or
end of file.

The -3 is optional, and is used to trade off compression vs. speed
and memory.  Valid options are -0 to -9.  Higher numbers compress better
but run slower and use more memory.  -6 is the default, and gives a
reasonable tradeoff.  Recommended options are:

  -0 to -2 for fast but not very good compression, uses ~8-15 Mb memory
  -3 for reasonably fast and good compression,
  -4 better compression but slower,
  -5 slightly better compression,
  -6 the default, uses ~230 Mb,
  -7 to -9 use ~460 Mb, ~920 Mb, ~1840 Mb, about the same speed as -6

use '-N' if you are sure your data has no x86 code (e.g. Calgary Corpus),
         and save some bytes;
use '-Ne' if you don't know anything, or think x86 code can be
          present in your data (e.g. MaximumCompression files).

Decompression:  No file names are specified.  The archive must exist.
If a path is stored, the file is extracted to the appropriate directory,
which must exist.  PAQAR does not create directories.  If the file to be
extracted already exists, it is not replaced; rather it is compared with
the archived file, and the offset of the first difference is reported.
The decompressor requires as much memory as was used to compress.
There is no option.

It is not possible to add, remove, or update files in an existing archive.
If you want to do this, extract the files, delete the archive, and
create a new archive with just the files you want.


TO COMPILE

gxx -O PAQAR.cpp	DJGPP 2.95.2
bcc32 -O2 PAQAR.cpp	Borland 5.5.1
sc -o PAQAR.cpp		Digital Mars 8.35n

g++ -O produces the fastest executable among free compilers, followed
by Borland and Mars.  However Intel 8 will produce the fastest and smallest
Windows executable overall, followed by Microsoft VC++ .net 7.1 /O2 /G7


PAQ6 DESCRIPTION    // to be updated for PAQAR

1. OVERVIEW

A PAQAR archive has a header, listing the names and lengths of the files
it contains in human-readable format, followed by the compressed data.
The first line of the header is "PAQAR -m" where -m is the memory option.
The data is compressed as if all the files were concatenated into one
long string.

PAQAR uses predictive arithmetic coding.  The string, y, is compressed
by representing it as a base 256 number, x, such that:

  P(s < y) <= x < P(s <= y)                                             (1)

where s is chosen randomly from the probability distribution P, and x
has the minimum number of digits (bytes) needed to satisfy (1).
Such coding is within 1 byte of the Shannon limit, log 1/P(y), so
compression depends almost entirely on the goodness of the model, P,
i.e. how well it estimates the probability distribution of strings that
might be input to the compressor.

Coding and decoding are illustrated in Fig. 1.  An encoder, given P and
y, outputs x.  A decoder, given P and x, outputs y.  Note that given
P in equation (1), that you can find either x from y or y from x.
Note also that both computations can be done incrementally.  As the
leading characters of y are known, the range of possible x narrows, so
the leading digits can be output as they become known.  For decompression,
as the digits of x are read, the set of possible y satisfying (1)
is restricted to an increasingly narrow lexicographical range containing y.
All of the strings in this range will share a growing prefix.  Each time
the prefix grows, we can output a character.

            y
          +--------------------------+
  Uncomp- |                          V
  ressed  |    +---------+  p   +----------+  x   Compressed
  Data  --+--->|  Model  |----->| Encoder  |----+ Data
               +---------+      +----------+    |
                                                |
                                     +----------+
                                     V
            y  +---------+  p   +----------+  y       Uncompressed
          +--->|  Model  |----->| Decoder  |----+---> Data
          |    +---------+      +----------+    |
          |                                     |
          +-------------------------------------+

  Fig. 1.  Predictive arithmetic compression and decompression


Note that the model, which estimates P, is identical for compression
and decompression.  Modeling can be expressed incrementally by the
chain rule:

  P(y) = P(y_1) P(y_2|y_1) P(y_3|y_1 y_2) ... P(y_n|y_1 y_2 ... y_n-1)  (2)

where y_i means the i'th character of the string y.  The output of the
model is a distribution over the next character, y_i, given the context
of characters seen so far, y_1 ... y_i-1.

To simplify coding, PAQAR uses a binary string alphabet.  Thus, the
output of a model is an estimate of P(y_i = 1 | context) (henceforth p),
where y_i is the i'th bit, and the context is the previous i - 1 bits of
uncompressed data.


2.  PAQAR MODEL

The PAQAR model consists of a weighted mix of independent submodels which
make predictions based on different contexts.  The submodels are weighted
adaptively to favor those making the best predictions.  The output of
two independent mixers (which use sets of weights selected by different
contexts) are averaged.  This estimate is then adjusted by secondary
symbol estimation (SSE), which maps the probability to a new probability
based on previous experience and the current context.  This final
estimate is then fed to the encoder as illustrated in Fig. 2.


  Uncompressed input
  -----+--------------------+-------------+-------------+
       |                    |             |             |
       V                    V             |             |
  +---------+  n0, n1  +----------+       |             |
  | Model 1 |--------->| Mixer 1  |\ p    |             |
  +---------+ \      / |          | \     V             V
               \    /  +----------+  \ +-----+    +------------+
  +---------+   \  /                  \|     | p  |            |    Comp-
  | Model 2 |    \/                  + | SSE |--->| Arithmetic |--> ressed
  +---------+    /\                    |     |    | Encoder    |    output
      ...       /  \                  /|     |    |            |
               /    \  +----------+  / +-----+    +------------+
  +---------+ /      \ | Mixer 2  | /
  | Model N |--------->|          |/ p
  +---------+          +----------+

  Fig. 2.  PAQAR Model details for compression.  The model is identical for
  decompression, but the encoder is replaced with a decoder.

In Sections 2-6, the description applies to the default memory option
(-5, or MEM = 5).  For smaller values of MEM, some components are
omitted and the number of contexts is less.


3.  MIXER

The mixers compute a probability by a weighted summation of the N
models.  Each model outputs two numbers, n0 and n1 represeting the
relative probability of a 0 or 1, respectively.  These are
combined using weighted summations to estimate the probability p
that the next bit will be a 1:

      SUM_i=1..N w_i n1_i                                               (3)
  p = -------------------,  n_i = n0_i + n1_i
      SUM_i=1..N w_i n_i

The weights w_i are adjusted after each bit of uncompressed data becomes
known in order to reduce the cost (code length) of that bit.  The cost
of a 1 bit is -log(p), and the cost of a 0 is -log(1-p).  We find the
gradient of the weight space by taking the partial derivatives of the
cost with respect to w_i, then adjusting w_i in the direction
of the gradient to reduce the cost.  This adjustment is:

  w_i := w_i + e[ny_i/(SUM_j (w_j+wo) ny_j) - n_i/(SUM_j (w_j+wo) n_j)]

where e and wo are small constants, and ny_i means n0_i if the actual
bit is a 0, or n1_i if the bit is a 1.  The weight offset wo prevents
the gradient from going to infinity as the weights go to 0.  e is set
to around .004, trading off between faster adaptation (larger e)
and less noise for better compression of stationary data (smaller e).

There are two mixers, whose outputs are averaged together before being
input to the SSE stage.  Each mixer maintains a set of weights which
is selected by a context.  Mixer 1 maintains 16 weight vectors, selected
by the 3 high order bits of the previous byte and on whether the data
is text or binary.  Mixer 2 maintains 16 weight vectors, selected by the
2 high order bits of each of the previous 2 bytes.

To distinguish text from binary data, we use the heuristic that space
characters are more common in text than NUL bytes, while NULs are more
common in binary data.  We compare the position of the 4th from last
space with the position of the 4th from last 0 byte.


4.  CONTEXT MODELS

Individual submodels output a prediction in the form of two numbers,
n0 and n1, representing relative probabilities of 0 and 1.  Generally
this is done by storing a pair of counters (c0,c1) in a hash table
indexed by context.  When a 0 or 1 is encountered in a context, the
appropriate count is increased by 1.  Also, in order to favor newer
data over old, the opposite count is decreased by the following
heuristic:

  If the count > 25 then replace with sqrt(count) + 6 (rounding down)
  Else if the count > 1 then replace with count / 2 (rounding down)

The outputs are derived from the counts in a way that favors highly
predictive contexts, i.e. those where one count is large and the
other is small.  For the case of c1 >= c0 the following heuristic
is used.

  If c0 = 0 then n0 = 0, n1 = 4 c0
  Else n0 = 1, n1 = c1 / c0

For the case of c1 < c0 we use the same heuristic swapping 0 and 1.

In the following example, we encounter a long string of zeros followed
by a string of ones and show the model output.  Note how n0 and n1 predict
the relative outcome of 0 and 1 respectively, favoring the most recent
data, with weight n = n0 + n1

  Input                 c0  c1  n0  n1
  -----                 --  --  --  --
  0000000000            10   0  40   0
  00000000001            5   1   5   1
  000000000011           2   2   1   1
  0000000000111          1   3   1   3
  00000000001111         1   4   1   4

  Table 1.  Example of counter state (c0,c1) and outputs (n0,n1)

In order to represent (c0,c1) as an 8-bit state, counts are restricted
to the values 0-40, 44, 48, 56, 64, 96, 128, 160, 192, 224, or 255.
Large counts are incremented probabilistically.  For example, if
c0 = 40 and a 0 is encountered, then c0 is set to 44 with
probability 1/4.  Decreases in counter values are deterministic,
and are rounded down to the nearest representable state.

Counters are stored in a hash table indexed by contexts starting
on byte boundaries and ending on nibble (4-bit) boundaries.  Each
hash element contains 15 counter states, representing the 15 possible
values for the 0-3 remaining bits of the context after the last nibble
boundary.  Hash collisions are detected by storing an 8-bit checksum of
the context.

Each bucket contains 4 elements in a move-to-front queue.  When a
new element is to be inserted, the priority of the two least recently
accessed elements are compared by using n (n0+n1) of the initial
counter as the priority, and the lower priority element is discarded.
Hash buckets are aligned on 64 byte addresses to minimize cache misses.


5.  RUN LENGTH MODELS

A second type of model is used to efficiently represent runs of
up to 255 identical bytes within a context.  For example, given the
sequence "abc...abc...abc..." then a run length model would map
"ab" -> ("c", 3) using a hash table indexed by "ab".  If a new
value is seen, e.g. "abd", then the state is updated to the new
character and a count of 1, i.e. "ab" -> ("d", 1).

A run length context is accessed 8 times, once for each bit.  If the
bits seen so far are consistent with the modeled character, then the output
of a run length model is (n0,n1) = (0,n) if the next bit is a 1,
or (n,0) if the next bit is a 0, where n is the count (1 to 255).
If the bits seen so far are not consistent with the predicted byte,
then the output is (0,0).  These counts are added to the counter state
counts to produce the model output.

Run lengths are stored in a hash table without collision detection,
so an element occupies 2 bytes.  Generally, most models store one run
length for every 8 counter pairs, so 20% of the memory is allocated to
them.  Run lengths are used only for memory option (-MEM) of 5 or higher.


6.  SUBMODEL DETAILS

Submodels differ mainly in their contexts.  These are as follows:

a. DefaultModel.  (n0,n1) = (1,1) regardless of context.

b. CharModel (N-gram model).  A context consists of the last 0 to N whole
bytes, plus the 0 to 7 bits of the partially read current byte.
The maximum N depends on the -MEM option as shown in the table below.
The order 0 and 1 contexts use a counter state lookup table rather
than a hash table.

  Order  Counters               Run lengths
  -----  --------               -----------
   0     2^8
   1     2^16
   2     2^(MEM+15)             2^(MEM+12), MEM >= 5
   3     2^(MEM+17)             2^(MEM+14), MEM >= 5
   4     2^(MEM+18)             2^(MEM+15), MEM >= 5
   5     2^(MEM+18), MEM >= 1   2^(MEM+15), MEM >= 5
   6     2^(MEM+18), MEM >= 3   2^(MEM+15), MEM >= 5
   7     2^(MEM+18), MEM >= 3   2^(MEM+15), MEM >= 5
   8     2^20, MEM = 5          2^17, MEM = 5
         2^(MEM+14), MEM >= 6   2^(MEM+14), MEM >= 6
   9     2^20, MEM = 5          2^17, MEM = 5
         2^(MEM+14), MEM >= 6   2^(MEM+14), MEM >= 6

  Table 2.  Number of modeled contexts of length 0-9

c.  MatchModel (long context).  A context is the last n whole bytes
(plus extra bits) where n >=8.  Up to 4 matching contexts are found by
indexing into a rotating input buffer whose size depends on MEM.  The
index is a hash table of 32-bit pointers with 1/4 as many elements as
the buffer (and therefore occupying an equal amount of memory).  The
table is indexed by a hashes of 8 byte contexts.  No collision detection
is used.  In order to detect very long matches at a long distance
(for example, versions of a file compressed together), 1/16 of the
pointers (chosen randomly) are indexed by a hash of a 32 byte context.

For each match found, the output is (n0,n1) = (w,0) or (0,w) (depending on
the next bit) with a weight of w = length^2 / 4 (maximum 511), depending
on the length of the context in bytes.  The four outputs are added together.

d.  RecordModel.  This models data with fixed length records, such as
tables.  The model attempts to find the record length by searching for
characters that repeat in the pattern x..x..x..x where the interval
between 4 successive occurrences is identical and at least 2.  Because
of uncertainty in this method, the two most recent values (which must
be different) are used.  The following 5 contexts are modeled;

  1. The two bytes above the current bit for each repeat length.
  2. The byte above and the previous byte (to the left) for each repeat
     length.
  3. The byte above and the current position modulo the repeat length,
     for the longer of the two lengths only.

e.  SparseModel.  This models contexts with gaps.  It considers the
following contexts, where x denotes the bytes considered and ? denotes
the bit being predicted (plus preceding bits, which are included in
the context).

       x.x?  (first and third byte back)
      x..x?
     x...x?
    x....x?
       xx.?
      x.x.?
      xx..?
  c ...  c?, gap length
  c ... xc?, gap length

  Table 3.  Sparse model contexts

The last two examples model variable gap lengths between the last byte
and its previous occurrence.  The length of the gap (up to 255) is part
of the context.

e.  AnalogModel.  This is intended to model 16-bit audio (mono or stereo),
24-bit color images, 8-bit data (such as grayscale images).  Contexts drop
the low order bits, and include the position within the file modulo
2, 3, or 4.  There are 8 models, combined into 4 by addition before
mixing.  An x represents those bits which are part of the context.

  16 bit audio:
    xxxxxx.. ........ xxxxxx.. ........ ?  (position mod 2)
    xxxx.... ........ xxxxxx.. ........ ?  (position mod 2)
    xxxxxx.. ........ ........ ........ xxxxxx.. ........ xxxxxx.. ........ ?
      (position mod 4 for stereo audio)

  24 bit color:
    xxxx.... ........ ........ xxxxxxxx ........ ........ ? (position mod 3)
    xxxxxx.. xxxx.... xxxx.... ? (position mod 3)

  8 bit data:
    xxx..... xxxxx... xxxxxxx. ?

  CCITT images (1 bit per pixel, 216 bytes wide, e.g. calgary/pic)
    xxxxxxxx (skip 215 bytes...) xxxxxxxx (skip 215 bytes...) ?

  Table 4.  Analog models.

f.  WordModel.  This is intended to model text files.  There are
3 contexts:

  1.  The current word
  2.  The previous and current words
  3.  The second to last and current words (skipping a word)

A word is defined in two different ways, resulting in a total of 6
different contexts:

  1.  Any sequence of characters with ASCII code > 32 (not white space).
      Upper case characters are converted to lower case.
  2.  Any sequence of A-Z and a-z (case sensitive).


7.  SSE

The purpose of the SSE stage is to further adjust the probability
output from the mixers to agree with actual experience.  Ideally this
should not be necessary, but in reality this can improve compression.
For example, when "compressing" random data, the output probability
should be 0.5 regardless of what the models say.  SSE will learn this
by mapping all input probabilities to 0.5.


    | Output   __
    | p      /
    |       /
    |    __/
    |   /
    |  /
    |  |
    | /
    |/   Input p
    +-------------

  Fig. 3.  Example of an SSE mapping.

SSE maps the probability p back to p using a piecewise linear function
with 32 segments.  Each vertex is represented by a pair of 8-bit counters
(n0, n1) except that now the counters use a stationary model.  When the
input is p and a 0 or 1 is observed, then the corresponding count (n0
or n1) of the two vertices on either side of p are incremented.  When
a count exceeds the maximum of 255, both counts are halved.  The output
probability is a linear interpolation of n1/n between the vertices on
either side.

The vertices are scaled to be longer in the middle of the graph and short
near the ends.  The intial counts are set so that p maps to itself.

SSE is context sensitive.  There are 2048 separately maintained SSE
functions, selected by the 0-7 bits of the current (partial) byte and
the 2 high order bits of the previous byte, and on whether the data
is text or binary, using the same heuristic as for selecting the mixer
context.

The final output to the encoder is a weighted average of the SSE
input and output, with the output receiving 3/4 of the weight:

  p := (3 SSE(p) + p) / 4.                                              (4)


8.  MEMORY USAGE

    Run executable without arguments and see help screen.

9.  EXPERIMENTAL RESULTS

Results on the Calgary corpos are shown below for some top data compressors
as of Dec. 30, 2003.  Options are set for maximum compression.  When
possible, the files are all compressed into a single archive.  Run times
are on a 705 MHz Duron with 256 Mb memory, and include 3 seconds to run
WRT when applicable.  PAQAR was compiled with DJGPP (g++) 2.95.2 -O.

  Original size   Options        3,141,622  Time   Author
  -------------   -------        ---------  ----   ------
  gzip 1.2.4      -9             1,017,624     2   Jean Loup Gailly
  epm r9          c                668,115    49   Serge Osnach
  rkc             a -M80m -td+     661,102    91   Malcolm Taylor
  slim 20         a                659,213   159   Serge Voskoboynikov
  compressia 1.0 beta              650,398    66   Yaakov Gringeler
  durilca v.03a (as in README)     647,028    30   Dmitry Shkarin
  PAQ5                             661,811   361   Matt Mahoney
  WRT11 + PAQ5                     638,635   258   Przemyslaw Skibinski +
  PAQ6            -0               858,954    52
                  -1               750,031    66
                  -2               725,798    76
                  -3               709,806    97
                  -4               655,694   354
                  -5               648,951   625
                  -6               648,892   636
  WRT11 + PAQ6    -6               626,395   446
  WRT20 + PAQ6    -6               617,734   439
  PAQAR 1.0       -6               608,607 ~1200   Alexander Ratushnyak

  Table 6.  Compressed size of the Calgary corpus.

WRT11 is a word reducing transform written by Przemyslaw Skibinski.  It
uses an external English dictionary to replace words with 1-3 byte
symbols to improve compression.  rkc, compressia, and durilca use a
similar approach.  WRT20 is a newer version of WRT11.


10.  ACKNOWLEDGMENTS

Thanks to Serge Osnach for introducing me to SSE (in PAQ1SSE/PAQ2) and
the sparse models (PAQ3N).  Also, credit to Eugene Shelwein,
Dmitry Shkarin for suggestions on using multiple character SSE contexts.
Credit to Eugene, Serge, and Jason Schmidt for developing faster and
smaller executables of previous versions.  Credit to Werner Bergmans
and Berto Destasio for testing and evaluating them, including modifications
that improve compression at the cost of more memory.  Credit to
Alexander Ratushnyak who found a bug in PAQ4 decompression, and also
in PAQ6 decompression for very small files (both fixed).

Thanks to Berto for writing PAQ5-EMILCONT-DEUTERIUM from which this
program is derived (which he derived from PAQ5).  His improvements to
PAQ5 include a new Counter state table and additional contexts for
CharModel and SparseModel.  I refined the state table by adding
more representable states and modified the return counts to give greater
weight when there is a large difference between the two counts.

I expect there will be better versions in the future.  If you make any
changes, please change the name of the program (e.g. PAQ7), including
the string in the archive header by redefining PROGNAME below.
This will prevent any confusion about versions or archive compatibility.
Also, give yourself credit in the help message.
*/


#define PROGNAME "PAQAR"
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#include <cctype>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cassert>
#define au 0xffffffff
#define bu 0x03ffffff
#define cu 0x01ffffff
#define du 0x007fffff
using std::string;
using std::vector;
using std::swap;

const int PSCALE=4096;  // Integer scale for representing probabilities
int MEM=6;
int exe=0;

template <class T> inline int size(const T& t) {return t.size();}

// 8-32 bit unsigned types, adjust as appropriate
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;

#define Top_value U32(0XFFFFFFFF)	/* Largest code value */
/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */
#define First_qtr U32(Top_value/4+1)  /* Point after first quarter    */
#define Half	  U32(2*First_qtr)    /* Point after first half  */
#define Third_qtr U32(3*First_qtr)    /* Point after third quarter */

class ProgramChecker {
  clock_t start;
public:
  ProgramChecker() {
    start=clock();

    // Test the compiler for common but not guaranteed assumptions
    assert(sizeof(U8)==1);
    assert(sizeof(U16)==2);
    assert(sizeof(U32)==4);
    assert(sizeof(int)==4);
  }
  clock_t start_time() const {return start;}  // When the program started
} programChecker;

void handler(){
printf("Out of memory\n");exit(1);
}
int wlen,clen,fsize,tf,po,bp,lon,z=0;
U8 l8=0;

//////////////////////////// rnd ////////////////////////////

// 32-bit random number generator based on r(i) = r(i-24) ^ r(i-55)

class Random{
U32 table[55];
int i;
public:
Random(){
table[0]=123456789;
table[1]=987654321;
for(int j=0;j<53;j++)
table[j+2]=table[j+1]*11+table[j]*23/16;
i=0;
}
U32 operator()(){
if(++i>54)
i=0;
if(i<24)
return table[i]^=table[i+31];
return table[i]^=table[i-24];
}
}rnd;

//////////////////////////// hash ////////////////////////////

// Hash functoid, returns 32 bit hash of 1-4 chars

class Hash{
U32 table[8][256];
public:
Hash(){
for(int i=7;i>=0;i--)
for(int j=0;j<256;j++)
table[i][j]=rnd();
}
U32 operator()(U8 a){
return table[0][a];
}
U32 operator()(U8 a,U8 b){
return table[0][a]+table[1][b];
}
U32 operator()(U8 a,U8 b,U8 c){
return table[0][a]+table[1][b]+table[2][c];
}
U32 operator()(U8 a,U8 b,U8 c,U8 s){
return table[0][a]+table[1][b]+table[2][c]+table[3][s];
}
}hash;

//////////////////////////// Counter ////////////////////////////

/* A Counter represents a pair (n0, n1) of counts of 0 and 1 bits
in a context.

  get0() -- returns p(0) with weight n = get0()+get1()
  get1() -- returns p(1) with weight n
  add(y) -- increments n_y, where y is 0 or 1 and decreases n_1-y
  priority() -- Returns a priority (n) for hash replacement such that
    higher numbers should be favored.
*/

class Counter{
U8 state;
struct E{
U32 n0,n1;
U8 s00,s01,s10,s11;
U32 p0,p1;
};
static E table[];
public:
Counter(){}
int get0(){
return table[state].n0;
}
int get1(){
return table[state].n1;
}
int priority(){
int a=table[state].n0;
int b=table[state].n1;
int c=2048*(a+b)+state;
if(a*b==0) c*=256;
return c;
}
void add(int y){
if(y){
if(state<214||rnd()<=table[state].p1)
state=table[state].s11;
else
state=table[state].s10;
}
else
{
if(state<214||rnd()<=table[state].p0)
state=table[state].s01;
else
state=table[state].s00;
}
}
};
Counter::E Counter::table[]={
#include"statable.dat"
};

//////////////////////////// ch ////////////////////////////

/* ch is a global object that provides common services to models.
It stores all the input so far in a rotating buffer of the last N bytes

  ch -- Global object
  ch.init() -- Initialize (after MEM is set)
  ch(i) -- Returns i'th byte from end
  ch(0) -- Returns the 0-7 bits of the partially read byte with a leading 1
  ch()  -- ch(0)
  ch.update(y) -- Appends bit y to the buffer
  ch.pos() -- The number of whole bytes appended, possibly > N
  ch.bpos() -- The number of bits (0-7) of the current partial byte at (0)
  ch[i] -- ch(pos()-i)
  ch.lo() -- Low order nibble so far (1-15 with leading 1)
  ch.hi() -- Previous nibble, 0-15 (no leading 1 bit)
  ch.pos(c) -- Position of the last occurrence of byte c (0-255)
  ch.pos(c, i) -- Position of the i'th to last occurrence, i = 0 to 3
*/

class Ch{
U8 *buf,lidx[256];
U32 N;
U32 lpos[4][256];
public:
Ch(){}
void init(){
lon=1;
po=bp=0;
N=1<<(16+MEM+(MEM>6));
buf=(U8*)calloc(N--,1);
if(!buf)
handler();
buf[0]=1;
memset(lpos,0,4096);
}
U32 operator[](int i){
return buf[i&N];
}
U32 operator()(int i){
return buf[(po-i)&N];
}
void upd(int y){
U8 &r=buf[po&N];
r=r*2+y;
l8=l8*2+y;
++bp;
if((lon=lon*2+y)>15){
lon=1;
if(!(bp&=7)){
lpos[++lidx[r]&=3][r]=po++;
buf[po&N]=1;
z+=(l8>127);
}
}
}
U32 pos(U8 c,int i){
return lpos[(lidx[c]-i)&3][c];
}
U32 hi(){
if(bp>3)
return buf[po&N]&15;
else
return buf[(po-1)&N]/16;
}
}ch;

//////////////////////////// Hashtable ////////////////////////////

/* A Hashtable stores Counters.  It is organized to minimize cache
misses for 64-byte cache lines.  The size is fixed at 2^n bytes.  It
uses LRU replacement for buckets of size 4, except that the next to
oldest element is replaced if it has lower priority than the oldest.
Each bucket represents 15 counters for a context on a half-byte boundary.

  Hashtable<Counter> ht(n) -- Create hash table of 2^n bytes (15/16 of
    these are 1-byte Counters).
  ht.set(h) -- Set major context to h, a 32 bit hash of a context ending on a
    nibble (4-bit) boundary.
  ht(c) -- Retrieve a reference to counter associated with partial nibble c
    (1-15) in context h.

Normally there should be 4 calls to ht(c) after each ht.set(h).
*/

class Hashtable{
U32 N;
struct HashElement{
U8 cs[2];
Counter c[15];
};
HashElement *table;
Counter *Tcxt;
public:
Hashtable(U32 n):N(n){
U8 *p=(U8*)calloc((17<<N)+64,1);
if(!p)
handler();
p+=64-((int)p&63);
table=(HashElement*)p;
Tcxt=&(table[0].c[-1]);
}
Counter& operator()(){
return Tcxt[lon];
}
void set(U32 h){
short cs=h;
U32 lo=(h>>(32-N))&-16,i=lo;
for(;i<lo+16;i++)
if(*((short*)&table[i].cs[0])==cs)
break;
else
if(!table[i].c[0].priority()){
*((short*)&table[i].cs[0])=cs;
break;
}
if(i==lo+16){
if(table[lo+14].c[0].priority()<table[lo+15].c[0].priority())
memmove(table+lo+1,table+lo,17*14);
else
memmove(table+lo+1,table+lo,17*15);
memset(table+lo,0,17);
*((short*)&table[lo].cs[0])=cs;
}
else
if(i!=lo){
HashElement he=table[i];
memmove(table+lo+1,table+lo,17*(i-lo));
table[lo]=he;
}
Tcxt=&(table[lo].c[-1]);
}
};

int n=-1, mp=-1;
U32 bc0[65],bc1[65],mxrs[64],mxwt[8][64];
struct SSEContext{
U8 c,n;
int p(){
return 4096*c/n;
}
void upd(int y){
c+=y;
if(++n>254)
c/=2,n/=2;
}
};
class SSEMap{
U32 table[4096];
public:
int operator()(int p){
return table[p];
}
SSEMap(){
for(int i=0;i<4096;i++)
table[i]=int(64*log((i+0.5)/(4096-0.5-i))+512);
table[4095]=1023;
table[0]=0;
}
}ssemap;

//////////////////////////// mixer ////////////////////////////

/* A Mixer combines a weighted set of probabilities (expressed as 0 and
1 counts) into a single probability P(1) that the next bit will be a 1.

  Mixer m(C);      -- Create Mixer with C sets of N weights (N is fixed)
  m.write(n0, n1); -- Store a prediction P(1) = n1/(n0+n1), with confidence
                      0 <= n0+n1 < 1024.  There should be at most N calls
                      to write() followed by predict() and update().
                      Write order should be consistent.
  m.add(n0, n1);   -- Adds to a previous write.
  m.predict(c);    -- Return P(1)*PSCALE (range 0 to PSCALE-1) for
                      weight set c (0 to C-1).
  m.update(y);     -- Tune the N internal weights for set c such that
                      predict(c) would return a result closer to y*PSCALE,
                      y = 0 or 1.
*/

class Mixer{
protected:
SSEContext (*sse)[33];
U32 (*wt)[65];
U32 swt,ssep,c,b0,b1;
public:
Mixer(int n):wt(new U32[n][65]),sse(new SSEContext[2048][33]){
for(int i=0;i<n;i++)
for(int j=0;j<65;j++)
wt[i][j]=(j>3&&j<12)?8:16;
int oldp=33;
for(i=4095;i>=0;i--){
int p=(ssemap(i)+16)/32;
int m=1+4096*4096/((i+1)*(4096-i));
if(m>254)m=254;
int c=(i*m+512)/4096;
for(int j=p;j<oldp;j++)
for(int k=0;k<2048;k++){
sse[k][j].c=c;
sse[k][j].n=m;
}
oldp=p;
}
}

void predict(int y){
c=y;
{
int _b0=0;
int _b1=0;
for(int j=n+1;j!=0;--j){
U32 w=wt[c][j-1];
_b0+=bc0[j-1]*w;
_b1+=bc1[j-1]*w;
}
b0=_b0;
b1=_b1;
}
if(fsize>=0)
b0+=n*3,b1+=n*3;
int p=(int)((double)b1*4096/(b0+b1));
if(fsize<0)
b0+=n*3,b1+=n*3;
b0+=n*3,b1+=n*3;
ssep=ssemap(p);
swt=ssep&31;
ssep/=32;
U32 wt=sse[c][ssep].p()*(32-swt)+sse[c][ssep+1].p()*swt;
if(fsize>300000){
if(exe)
p=(wt*28/32+p*4)/32;
else
p=(wt/32+p)/2;
}
else
if(z!=0&&fsize!=0)
p=(wt*27/32+p*5)/32;
mxrs[++mp]=p;
mxrs[16+mp]=4096-p;
}

void upd(int y,U32 d,U32 e){
sse[c][ssep].upd(y);
if(swt)
sse[c][ssep+1].upd(y);
U32 s=b0+b1;
U32 sy=y?b1:b0;
U32 rn=rnd();
U32 s1=(au/s+((rn>>(d-7))&127))/128+e*32;
U32 syd=(au*(1.0/sy-1.0/s)+(rn&127))/128;
int m0=y?-s1:syd;
int m1=y?syd:-s1;
rn=(rn>>d)&32767;
{
U32 *wl=&wt[c][0];
for(int i=n+1;i!=0;i--){
int dw=int(m0*bc0[i-1]+m1*bc1[i-1]+rn)>>15;
wl[i-1]=max(int(wl[i-1]+dw),1);
}
}
}
};

// A MultiMixer averages the output of N mixers using different contexts

class MultiMixer{
Mixer m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,ma,mb;
int b0,b1;
public:
MultiMixer():m0(256),m1(256),m2(256),m3(256),m4(2048),m5(2048),m6(2048),m7(2048),m8(2048),m9(2048),ma(2048),mb(2048){
for(int i=0;i<8;i++)
for(int j=0;j<64;j++)
mxwt[i][j]=3;
}

void wri(int n0,int n1){
n++;
bc0[n]=n0;
bc1[n]=n1;
}
void add(int n0,int n1){
bc0[n]+=n0;
bc1[n]+=n1;
}
void mul(int m){
bc0[n]=bc0[n]*m/2;
bc1[n]=bc1[n]*m/2;
}

int predict(){
int c=ch(2)/64;
if(bp>1)
c=ch(0)>>(bp-2);
c=ch(1)*8+c;
m6.predict(c);
m7.predict(c);
c=ch(2)/16+tf*16*(ch(1)/32+8*(wlen==clen));
m0.predict(c);
m2.predict(c);
c=ch(0)*8+(ch(1)/64)*2+tf*(clen>2);
ma.predict(c);
mb.predict(c);
c=ch(1)/32+8*(ch(2)/32)+(ch(3)&192);
m3.predict(c);
m1.predict(c);
c=ch(3);
if(bp>1)
c=ch(0)<<(8-bp);
c=bp*256+ch(1)/32+8*(ch(2)/32)+(c&192);
m8.predict(c);
m9.predict(c);
c=bp*256+((ch(0)<<(8-bp))+(ch(1)>>bp)&255);
m4.predict(c);
m5.predict(c);
b0=0,b1=0;
for(int i=mp;i>=0;i--){
int m=mxwt[bp][i];
b1+=mxrs[i]*m;
b0+=m;
}
b0=4096*b0-b1;
b0+=32;
b1+=32;
return (int)((double)b1*4096/(b0+b1));
}

void upd(int y){
if(mp>0){
int i=0,j=16;
if(fsize<0){
i=3,j=64;
}
if(exe){
i=8,j=160;
}
m0.upd(y,15,i);
m1.upd(y,15,j);
m3.upd(y,15,i);
m2.upd(y,15,j);
m4.upd(y,15,i);
m5.upd(y,15,j);
m6.upd(y,15,i);
m7.upd(y,15,j);
m8.upd(y,15,i);
m9.upd(y,15,j);
ma.upd(y,15,i);
mb.upd(y,15,j);
U32 s=b0+b1;
U32 sy=y?b1:b0;
U32 rn=rnd()&32767;
U32 s1=(au/s+32)/ 64;
U32 syd=(au/sy-au/s+ 32)/ 64;
int m0=y?-s1:syd;
int m1=y?syd:-s1;
U32 bp1=(bp-1)&7;
int mi=8;
if(z!=0&&fsize!=0)
mi=2;
for(i=mp;i>=0;i--){
int dw=int(rn+mxrs[i]*m1+mxrs[16+i]*m0)>>15;
mxwt[bp1][i]=max(int(mxwt[bp1][i]+dw),mi);
}
}

mp=-1;
n=-1;
wri(1,0);
wri(2,1);
wri(4,1);
wri(8,1);
wri(0,1);
wri(1,1);
wri(16,16);
wri(1,2);
wri(1,4);
wri(1,8);
}
}mixer;

//////////////////////////// CounterMap ////////////////////////////

/* CounterMap maintains a model and one context
  Countermap cm(N); -- Create, size 2^N bytes
  cm.update(h);     -- Update model, then set next context hash to h
  cm.write();       -- Predict next bit and write counts to mixer
  cm.add();         -- Predict and add to previously written counts

There should be 8 calls to either write() or add() between each update(h).
h is a 32-bit hash of the context which should be set after a whole number
of bytes are read. */

// Stores only the most recent byte and its count per context (run length)
// in a hash table without collision detection

class CounterMap1{
int N;
struct S{
U8 c,n;
U16 csum;
};
S *t,*cxt;
public:
CounterMap1(int n):N(n+MEM-10){
if(!(t=(S*)calloc(1<<N,4)))
handler();
cxt=t;
}
void upd(U32 h){
if(cxt->c!=ch(1))
cxt->n=0;
cxt->c=ch(1);
if(cxt->n<255)
++(cxt->n);
U32 lo=(h>>(32-N))&-16,i=lo;
U16 cs=h&65535;
for(;i<lo+16;i++){
cxt=t+i;
if(cxt->csum==cs)
break;
else
if(cxt->n==0){
cxt->csum=cs;
break;
}
}
if(i==lo+16){
int k,l;
cxt=t+lo+14; k=cxt->n;
cxt=t+lo+15; l=cxt->n;
if(k<l)
memmove(t+lo+1,t+lo,4*14);
else
memmove(t+lo+1,t+lo,4*15);
memset(t+lo,0,4);
cxt=t+lo;
cxt->csum=cs;
}
else
if(i!=lo){
S he=t[i];
memmove(t+lo+1,t+lo,4*(i-lo));
t[lo]=he;
}
cxt=t+lo;
}
void add(){
U32 d=(cxt->c+256)>>(7-bp);
U32 c=2*cxt->n;
if(c>6)
c=c/2+3;
if(!(d^=ch(0)*2))
mixer.add(c,0);
else
if(d==1)
mixer.add(0,c);
}
};

// Uses a nibble-oriented hash table of contexts (counter state)
class CounterMap2{
U32 cxt;
Hashtable ht2;
Counter *cp[8];
public:
CounterMap2(int n):ht2(n+MEM-10){
for(int i=0;i<8;i++)
cp[i]=0;
cxt=0;
}
// After 8 predictions, update the models with the last input char, ch(1),
// then set the new context hash to h
void upd(U32 h){
int c=ch(1);
for(int i=0;i<8;i++)
if(cp[i]){
cp[i]->add((c>>(7-i))&1),cp[i]=0;
}
ht2.set(cxt=h);
}
void add(){
if(bp==4){
ht2.set(cxt^=hash(ch.hi()));
}
cp[bp]=&ht2();
mixer.add(cp[bp]->get0(),cp[bp]->get1());
}
void wri(){
if(bp==4){
ht2.set(cxt^=hash(ch.hi()));
}
cp[bp]=&ht2();
mixer.wri(cp[bp]->get0(),cp[bp]->get1());
}
void mul(int n){
wri();
mixer.mul(n);
}
};

// Combines 1 and 2 above.
class CounterMap3{
CounterMap1 cm1;
CounterMap2 cm2;
public:
CounterMap3(int n):cm1(n),cm2(n){}
void upd(U32 h){
cm1.upd(h);
cm2.upd(h);
}
void mul(int n){
wri();
mixer.mul(n);
}
void wri(){
cm2.wri();
cm1.add();
}
void add(){
cm2.add();
cm1.add();
}
};

//////////////////////////// charModel ////////////////////////////

// A CharModel contains n-gram models from 0 to 9

class CharModel{
CounterMap2 t2,t3,t4,t5,t6,t7,t8,t9,ta;
Counter *t0,*cp0,*cp1;
U32 cxt[11];
public:
CharModel():t0(new Counter[256*257]),t2(21),t3(23),t4(23),t5(24),t6(24),t7(24),t8(24),t9(23),ta(23){
cp0=t0;
cp1=t0+256;
memset(t0,0,256*257);
memset(cxt,0,44);
}
void model(){
int y=ch(!bp)&1;
cp0->add(y);
cp1->add(y);
if(!bp){
for(int i=10;i!=0;i--)
cxt[i]=cxt[i-1]^hash(ch(1),i,tf*(wlen==i));
t2.upd(cxt[2]);
t3.upd(cxt[3]);
t4.upd(cxt[4]);
t5.upd(cxt[5]);
t6.upd(cxt[6]);
t7.upd(cxt[7]);
t8.upd(cxt[8]);
t9.upd(cxt[9]);
ta.upd(cxt[10]);
}
cp0=t0+ch(0);
cp1=t0+256+ch(1)*256+ch(0);
mixer.wri(cp0->get0(),cp0->get1());
if(tf)
mixer.mul(7);
mixer.wri(cp1->get0(),cp1->get1());
if(tf)
mixer.mul(4);
t2.wri();
t3.wri();
t4.wri();
t5.wri();
if(fsize!=0){
t6.add();
t7.wri();
t8.add();
t9.add();
ta.add();
}
}
};

//////////////////////////// matchModel ////////////////////////////

/* A MatchModel looks for a match of length n >= 8 bytes between
the current context and previous input, and predicts the next bit
in the previous context with weight n.  If the next bit is 1, then
the mixer is assigned (0, n), else (n, 0).  Matchies are found using
an index (a hash table of pointers into ch). */

class MatchModel{
U32 hash[2];
U32 begin[4];
U32 end[4];
U32 *ptr;
public:
MatchModel():ptr(new U32[1<<21]){
memset(ptr,0,1<<23);
hash[0]=hash[1]=0;
for(int i=0;i<4;i++)
begin[i]=end[i]=0;
}
void model(){
if(!bp){
hash[0]=hash[0]*908754512+ch(1)+1;
hash[1]=hash[1]*91368434+ch(1)+1;
U32 h=hash[0]>>11;
if(!(h>>17))
h=hash[1]>>11;
for(int i=0;i<4;i++)
if(end[i]&&ch(1)==ch[end[i]])
++end[i];
for(i=0;i<4;i++){
if(!end[i]){
int j;
for(j=0;j<4;j++)
if(end[j]==ptr[h])
break;
if(j<4)
break;
end[i]=ptr[h];
if(end[i]){
U32 p=po;
begin[i]=end[i];
while(begin[i]&&p&&begin[i]!=p+1&&ch[begin[i]-1]==ch[--p])
--begin[i];
}
if(end[i]==begin[i])
begin[i]=end[i]=0;
break;
}
}
ptr[h]=po;
}
int n0=0,n1=0;
for(int i=0;i<4;i++)
if(end[i]){
U32 d=(ch[end[i]]+256)>>(7-bp);
if((d>>1)!=ch(0))
begin[i]=end[i]=0;
else
{
U32 wt=end[i]-begin[i];
wt=min(int(wt*wt/4),1020);
if(d&1)
n1+=wt;
else
n0+=wt;
}
}
mixer.wri(n0,n1);
}
};

//////////////////////////// recordModel ////////////////////////////

/* A RecordModel finds fixed length records and models bits in the context
of the two bytes above (the same position in the two previous records)
and in the context of the byte above and to the left (the previous byte).
The record length is assumed to be the interval in the most recent
occurrence of a byte occuring 4 times in a row equally spaced, e.g.
"x..x..x..x" would imply a record size of 3.  There are models for
the 2 most recent, different record lengths of at least 2. */

class RecordModel{
enum{SIZE=20};
CounterMap3 t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,ta,tb;
int r1,r2,r3,r4,c1,c2;
public:
RecordModel():t0(17-MEM),tb(17-MEM),t1(19-MEM),t2(20-MEM),t3(SIZE),t4(SIZE),t5(SIZE),t6(SIZE),t7(SIZE),t8(SIZE),t9(SIZE),ta(SIZE),r1(2),r2(3),r3(2),r4(3),c1(0),c2(0){}
void model(){
if(!bp){
int c=ch(1);
int d=ch.pos(c,0)-ch.pos(c,1);
if(d>2&&d==ch.pos(c,1)-ch.pos(c,2)&&d==ch.pos(c,2)-ch.pos(c,3))
if(d==r1){
if(++c1>=c2){
swap(c1,c2);
swap(r1,r2);
}
}
else
if(d==r2)
++c2;
else
{
r1=d;
c1=0;
}
if(d>2){
if(d==r3)
swap(r3,r4);
if(d!=r4)
r3=d;
}
t0.upd(hash(ch(1),0));
tb.upd(hash(ch(1),9));
t1.upd(hash(ch(1),ch(2),1));
t2.upd(hash(ch(1),ch(2),ch(3),2));
t3.upd(hash(ch(r2),ch(r2*2),r2));
t4.upd(hash(ch(1),ch(r2),ch(r2+1),r2));
t5.upd(hash(ch(r2),po%r2));
t6.upd(hash(ch(r1),ch(r1*2),r1));
t7.upd(hash(ch(1),ch(r1),ch(r1+1),r1));
t8.upd(hash(ch(r4),ch(r4*2),r4));
t9.upd(hash(ch(1),ch(r4),ch(r4+1),r4));
ta.upd(hash(ch(r4),r4));
}
if(fsize!=0){
if(tf){
t0.wri();
tb.add();
t1.mul(5);
t2.mul(6);
}
else
{
t0.wri();
tb.add();
t1.mul(4);
t2.mul(3);
}
t8.wri();
t9.wri();
ta.wri();
t4.wri();
}
t3.wri();
t5.wri();
t6.wri();
t7.wri();
}
};

//////////////////////////// sparseModel ////////////////////////////

// A SparseModel models several order-2 contexts with gaps

class SparseModel{
enum{SIZE=20};
CounterMap3 t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,ta;
public:
SparseModel():t0(SIZE),t1(SIZE),t2(SIZE),t3(SIZE),t4(SIZE),t5(SIZE),t6(SIZE),t7(SIZE),t8(SIZE),t9(SIZE),ta(SIZE){}
void model(){
if(!bp){
int i=min(int(po-ch.pos(ch(1),1)),192)/4;
int j=min(int(po-ch.pos(ch(2),1)),192)/4;
if(j>8)
j=j/4+7;
if(i>8)
i=i/4+7;
U8 k=tf*(wlen+1);
t0.upd(hash(ch(1),ch(3),++k));
t1.upd(hash(ch(1),ch(4),++k));
t2.upd(hash(ch(1),ch(5),++k));
t3.upd(hash(ch(1),ch(6),++k));
t4.upd(hash(ch(1),ch(3),ch(5),++k));
t5.upd(hash(ch(2),ch(3),++k));
t6.upd(hash(ch(2),ch(4),++k));
t7.upd(hash(ch(3),ch(4),++k));
t8.upd(hash(ch(2),ch(4),ch(6),++k));
t9.upd(hash(ch(1),ch(2),i,++k));
ta.upd(hash(ch(1),ch(2),j+64,++k));
}
t0.wri();
t1.wri();
t2.wri();
t3.wri();
t4.wri();
t5.wri();
t6.wri();
t7.wri();
t8.wri();
t9.wri();
ta.wri();
}
};

class SparseModel2{
enum{SIZE=21};
CounterMap3 t0,t1,t2,t3,t4;
public:
SparseModel2():t0(SIZE),t1(SIZE),t2(SIZE),t3(SIZE),t4(SIZE){}
void model(){
if(!bp){
t0.upd(hash(ch(1),ch(7),ch(9),ch(13)));
t1.upd(hash(ch(2),ch(5),ch(10),ch(14)));
t2.upd(hash(ch(3),ch(8),ch(12),ch(15)));
t3.upd(hash(ch(4),ch(6),ch(11),ch(16)));
t4.upd(hash(ch(5),ch(7),ch(12),ch(18)));
}
t0.mul(2);
t1.mul(3);
t2.mul(3);
t3.add();
t4.add();
}
};

//////////////////////////// analogModel ////////////////////////////

// An AnalogModel is intended for 16-bit mono or stereo (WAV files)
// 24-bit images (BMP files), and 8 bit analog data (such as grayscale
// images), and CCITT images.

class AnalogModel{
enum{SIZE=18};
CounterMap3 t0,t1,t2,t3,t4,t5,t6,t7,t8;
public:
AnalogModel():t7(18-MEM),t0(23-MEM),t1(SIZE-1),t2(SIZE-1),t3(SIZE),t4(23-MEM),t5(23-MEM),t6(SIZE),t8(SIZE){}
void model(){
if(!bp){
U8 c1,c2,c3,c4;
c1=     ch(1)/64;
c1=c1*4+ch(2)/64;
c1=c1*4+ch(3)/64;
c1=c1*4+ch(4)/64;
c2=     ch(5)/64;
c2=c2*4+ch(6)/64;
c2=c2*4+ch(7)/64;
c2=c2*4+ch(8)/64;
c3=     ch(9)/64;
c3=c3*4+ch(10)/64;
c3=c3*4+ch(11)/64;
c3=c3*4+ch(12)/64;
t7.upd(hash(c1));
t0.upd(hash(c1,c2));
t1.upd(hash(c1,c2,c3));
c1=     ch(1)/32;
c1=c1*8+ch(2)/32;
c2=     ch(3)/32;
c2=c2*8+ch(4)/32;
c3=     ch(5)/32;
c3=c3*8+ch(6)/32;
c4=     ch(7)/32;
c4=c4*8+ch(8)/32;
t4.upd(hash(c1,c2));
t5.upd(hash(c1,c2,c3));
t6.upd(hash(c1,c2,c3,c4));
c4=c4*4+ch(9)/64;
c3=c3*4+ch(10)/64;
if(ch(9)&32) c2+=128;
if(ch(10)&32) c1+=128;
t8.upd(hash(c1,c2,c3,c4));
t2.upd(hash(ch(1)/2,ch(2)/8,ch(3)/32,5));
t3.upd(hash(ch(1),ch(2)/4,ch(3)/16,6));
}
t7.wri();
t0.wri();
t1.wri();
t2.wri();
t3.wri();
t4.mul(1);
t5.wri();
t6.wri();
t8.mul(4);
}
};

class PicModel{
enum{SIZE=21};
CounterMap3 t0,t1,ta,tb,tc,td,te,tf,tg,th;
CounterMap2 t2,t3,t4,t5,t6,t7,t8,t9,ti,tj,tk,tl,tm,tn,to,tp,tq,tr,ts,tt,tu,tv,tw;
public:
PicModel():t0(SIZE),t1(SIZE),t2(SIZE),t3(SIZE),t4(SIZE),t5(SIZE),t6(SIZE),t7(SIZE),t8(SIZE),t9(SIZE),ta(SIZE),tb(SIZE),tc(SIZE),td(SIZE),te(SIZE),tf(SIZE),tg(SIZE),th(SIZE),ti(SIZE),tj(SIZE),tk(SIZE),tl(SIZE),tm(SIZE),tn(SIZE),to(SIZE),tp(SIZE),tq(SIZE),tr(SIZE),ts(SIZE),tt(SIZE),tu(SIZE),tv(SIZE),tw(SIZE){}
void model(){
if(!bp){
int i=0,j=63;
t0.upd(hash(ch(1),ch(2),ch(216),i++));
t1.upd(hash(ch(1),ch(2),ch(217),i++));
t2.upd(hash(ch(216),i++));
t3.upd(hash(ch(216),ch(1),i++));
t4.upd(hash(ch(216),ch(217),i++));
t5.upd(hash(ch(216),ch(215),i++));
t6.upd(hash(ch(216),ch(432),i++));
t7.upd(hash(ch(216),ch(217),ch(1),i++));
t8.upd(hash(ch(216),ch(215),ch(1),i++));
t9.upd(hash(ch(216),ch(432),ch(1),i++));
ta.upd(hash(ch(216),ch(217),ch(215),i++));
tb.upd(hash(ch(216),ch(432),ch(217),i++));
tc.upd(hash(ch(216),ch(432),ch(215),i++));
td.upd(hash(ch(216),ch(432),ch(648),i++));
te.upd(hash(ch(1),ch(216),ch(2),ch(215)));
tf.upd(hash(ch(1),ch(216),ch(2),ch(217)));
tg.upd(hash(ch(1),ch(216),ch(215),ch(217)));
th.upd(hash(ch(1),ch(216),ch(432),ch(217)));
ti.upd(hash(ch(1)&j,ch(216)&j,ch(2)&j,i++));
tj.upd(hash(ch(1)&j,ch(216)&j,ch(217)&j,i++));
tk.upd(hash(ch(1)&j,ch(216)&j,ch(215)&j,i++));
tl.upd(hash(ch(1)&j,ch(216)&j,ch(432)&j,i++));
tm.upd(hash(ch(216)&j,i++));j/=4;
tn.upd(hash(ch(1)&j,ch(216)&j,ch(2)&j,i++));
to.upd(hash(ch(1)&j,ch(216)&j,ch(217)&j,i++));
tp.upd(hash(ch(1)&j,ch(216)&j,ch(215)&j,i++));
tq.upd(hash(ch(1)&j,ch(216)&j,ch(432)&j,i++));
tr.upd(hash(ch(216)&j,i++));j/=4;
ts.upd(hash(ch(1)&j,ch(216)&j,ch(2)&j,i++));
tt.upd(hash(ch(1)&j,ch(216)&j,ch(217)&j,i++));
tu.upd(hash(ch(1)&j,ch(216)&j,ch(215)&j,i++));
tv.upd(hash(ch(1)&j,ch(216)&j,ch(432)&j,i++));
tw.upd(hash(ch(216)&j,i++));
}
t0.wri();
t1.add();
t2.wri();
t3.wri();
t4.wri();
t5.wri();
t6.wri();
t7.wri();
t8.wri();
t9.wri();
ta.wri();
tb.wri();
tc.wri();
td.add();
te.add();
tf.add();
tg.add();
th.add();
ti.wri();
tj.wri();
tk.wri();
tl.wri();
tm.wri();
tn.wri();
to.wri();
tp.wri();
tq.wri();
tr.wri();
ts.wri();
tt.wri();
tu.wri();
tv.wri();
tw.wri();
}
};

//////////////////////////// wordModel ////////////////////////////

// A WordModel models words, which are any characters > 32 separated
// by whitespace ( <= 32).  There is a unigram, bigram and sparse
// bigram model (skipping 1 word).

class WordModel{
enum{SIZE=23};
CounterMap2 t0;
CounterMap3 t1,t2,t3,t4,t5,t6,t7,t8,t9,ta,tb,tc;

U32 cxt[6];     // Hashes of last N words broken on whitespace
U32 wxt[6];     // Hashes of last N words of letters only, lower case
public:
WordModel():t0(SIZE-2),t1(SIZE),t2(SIZE),t3(SIZE),t4(SIZE-1),t5(SIZE-1),t6(SIZE-1),t7(SIZE-2),t8(SIZE-2),t9(SIZE-2),ta(SIZE-1),tb(SIZE),tc(SIZE-1){
wlen=0;
for(int i=5;i>=0;i--)
wxt[i]=cxt[i]=0;
clen=0;
}
void model(){
if(!bp){
int c=ch(1);
if(c>32)
cxt[0]^=hash(cxt[0],cxt[0]>>8,c,clen++);
else
if(cxt[0]){
for(int i=5;i!=0;i--)
cxt[i]=cxt[i-1];
cxt[0]=0;
clen=0;
}
if(isalpha(c)){
wxt[0]^=hash(wxt[0],tolower(c),wlen++);
}
else
{
wlen=0;
for(int i=5;i!=0;i--)
wxt[i]=wxt[i-1];
wxt[0]=0;
}
t0.upd(-cxt[0]);
t1.upd(cxt[1]-cxt[0]);
t5.upd(cxt[2]-cxt[0]);
t2.upd(cxt[1]+cxt[2]-cxt[0]);
t4.upd(cxt[3]+cxt[2]-cxt[0]);
t3.upd(cxt[3]+cxt[1]-cxt[0]);
t6.upd(cxt[3]-cxt[0]);
t7.upd(cxt[4]-cxt[0]);
t8.upd(cxt[5]-cxt[0]);
t9.upd(-wxt[0]);
ta.upd(wxt[1]-wxt[0]);
tb.upd(wxt[1]+wxt[2]-wxt[0]);
tc.upd(wxt[2]-wxt[0]);
}
if(tf){
t0.mul(6);
t1.mul(6);
t2.mul(10);
t3.mul(10);
t4.mul(8);
t5.mul(6);
t6.mul(4);
t7.mul(6);
t8.mul(6);
t9.mul(4);
ta.mul(4);
tb.mul(4);
tc.add();
}
else
{
t0.wri();
t1.wri();
t2.wri();
t3.wri();
t4.wri();
t5.wri();
t6.wri();
t7.mul(1);
t8.mul(1);
t9.mul(1);
ta.mul(1);
mixer.wri(0,0);
}
}
};
SSEContext sse[2048][33],ss2[2048][33],ss3[2048][33],ss4[2048][33],ss5[2048][33];

//////////////////////////// Predictor ////////////////////////////

/* A Predictor adjusts the model probability using SSE and passes it
to the encoder.  An SSE model is a table of counters, sse[SSE1][SSE2]
which maps a context and a probability into a new, more accurate
probability.  The context, SSE1, consists of the 0-7 bits of the current
byte and the 2 leading bits of the previous byte.  The probability
to be mapped, SSE2 is first stretched near 0 and 1 using SSEMap, then
quantized into SSE2=32 intervals.  Each SSE element is a pair of 0
and 1 counters of the bits seen so far in the current context and
probability range.  Both the bin below and above the current probability
is updated by adding 1 to the appropriate count (n0 or n1).  The
output probability for an SSE element is n1/(n0+n1) interpolated between
the bins below and above the input probability.  This is averaged
with the original probability with 25% weight to give the final
probability passed to the encoder. */

class Predictor{
U32 wt,ssep,c1,c2,c3,c4,c5,nextp;
CharModel charModel;
MatchModel matchModel;
RecordModel recordModel;
SparseModel sparseModel;
SparseModel2 sparseModel2;
AnalogModel analogModel;
WordModel wordModel;
PicModel picModel;
public:
Predictor(){
ch.init();
nextp=2048;
int oldp=33;
for(int i=4095;i>=0;i--){
int p=(ssemap(i)+16)/32;
int m=1+4096*4096/((i+1)*(4096-i));
if(m>254)m=254;
int c=(i*m+2048)/4096;
for(int j=p;j<oldp;j++)
for(int k=0;k<2048;k++){
sse[k][j].c=ss2[k][j].c=ss3[k][j].c=ss4[k][j].c=ss5[k][j].c=c;
sse[k][j].n=ss2[k][j].n=ss3[k][j].n=ss4[k][j].n=ss5[k][j].n=m;
}
oldp=p;
}
}
int p(){
return nextp;
}
void upd(int y){
if(n>0){
sse[c1][ssep].upd(y);
ss2[c2][ssep].upd(y);
ss3[c3][ssep].upd(y);
ss4[c4][ssep].upd(y);
ss5[c5][ssep].upd(y);
if(wt){
ssep++;
sse[c1][ssep].upd(y);
ss2[c2][ssep].upd(y);
ss3[c3][ssep].upd(y);
ss4[c4][ssep].upd(y);
ss5[c5][ssep].upd(y);
}
}
ch.upd(y);
tf=ch.pos(0,0)<ch.pos(32,3)&&ch.pos(255,0)<ch.pos(32,3);
mixer.upd(y);
charModel.model();
recordModel.model();
sparseModel.model();
if(fsize!=0){
mixer.wri(18,12);
sparseModel2.model();
analogModel.model();
matchModel.model();
wordModel.model();
}
else
picModel.model();
nextp=mixer.predict();
ssep=ssemap(nextp);
wt=ssep&31;
ssep/=32;
c4=ch(1)*8+ch(2)/32;
c1=ch(0)*8+(ch(1)/64)*2+tf*(wlen==clen);
c2=l8*8+bp;
c3=bp*256+ch(1)/32+8*(ch(2)/32)+(ch(3)&192);
c5=l8*8+ch(1)/32;
U32 p1=sse[c1][ssep].p()*(32-wt)+sse[c1][ssep+1].p()*wt;
U32 p2=ss2[c2][ssep].p()*(32-wt)+ss2[c2][ssep+1].p()*wt;
U32 p3=ss3[c3][ssep].p()*(32-wt)+ss3[c3][ssep+1].p()*wt;
U32 p4=ss4[c4][ssep].p()*(32-wt)+ss4[c4][ssep+1].p()*wt;
U32 p5=ss5[c5][ssep].p()*(32-wt)+ss5[c5][ssep+1].p()*wt;
if(tf)
nextp=((3*p3+6*p1+6*p2+2*p4+2*p5)*10/32+9+nextp*19)/(19*11);
else
nextp=((3*p3+3*p1+4*p2+2*p4+2*p5)*13/32+7+nextp*14)/(14*14);
}
};




//////////////////////////// Encoder ////////////////////////////

/* An Encoder does arithmetic encoding.  Methods:
   Encoder(COMPRESS, f) creates encoder for compression to archive f, which
     must be open past the header for writing in binary mode
   Encoder(DECOMPRESS, f) creates encoder for decompression from archive f,
     which must be open past the header for reading in binary mode
   encode(bit) in COMPRESS mode compresses bit to file f.
   decode() in DECOMPRESS mode returns the next decompressed bit from file f.
   flush() should be called when there is no more to compress
*/

typedef enum {COMPRESS, DECOMPRESS} Mode;
class Encoder {
private:
  Predictor predictor;
  const Mode mode;       // Compress or decompress?
  FILE* archive;         // Compressed data file
  U32 x1, x2;            // Range, initially [0, 1), scaled by 2^32
  U32 x;                 // Last 4 input bytes of archive.
  int  LPS;
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


void Encoder::bit_plus_follow(int bit)
{ 
	if (bit) bout|=bptr;
	if (!(bptr>>=1)) {
		putc(bout,archive);
		bptr=128;
		bout=0;
	}
	bit^=1;
	for (; bits_to_follow > 0; bits_to_follow--) {
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
		if (bin==EOF) bin=0;
		bptrin=128;
	}
	return ((bin&bptrin)!=0);
}


// Constructor
Encoder::Encoder(Mode m, FILE* f): predictor(), mode(m), archive(f), x1(0),
                                   x2(0xffffffff), x(0), bits_to_follow(0), bptr(128), bout(0), bptrin(1) {

  // In DECOMPRESS mode, initialize x to the first 4 bytes of the archive
  if (mode==DECOMPRESS) {
    x = 1;
    for (;x < Half;) x = 2 * x + input_bit();
    x = 2 * x + input_bit();
  }
}

/* encode(y) -- Encode bit y by splitting the range [x1, x2] in proportion
to P(1) and P(0) as given by the predictor and narrowing to the appropriate
subrange.  Output leading bytes of the range as they become known. */


inline void Encoder::encode(int y) {

  // Split the range
  U32 p=predictor.p()*(4096/PSCALE); // P(1) * 4K
  assert(p<4096);
  const U32 xdiff=x2-x1;
  U32 a,b,c;
  U32 xmid;  // = x1+p*(x2-x1) multiply without overflow, round down

  c = 2*p + 1;
  a = xdiff >> 13;
  b = xdiff & 0x1fff;
  xmid = x1 + a * c + (( c * b ) >> 13);

  // Update the range
  if (y)
    x2=xmid;
  else
    x1=xmid+1;
  predictor.upd(y);

  // Shift equal MSB's out
  for (;;) {
    if ( x2 < Half ) {
	 bit_plus_follow(0);	/* Output 0 if in low half. */
      } else if (x1 >= Half) {	/* Output 1 if in high half. */
	 bit_plus_follow(1);
//	 x1 -= Half;
//	 x2 -= Half;		/* Subtract offset to top.  */
      } else if (x1 >= First_qtr	/* Output an opposite bit   */
		 && x2 < Third_qtr) {	/* later if in middle half. */
	 bits_to_follow ++;
	 x1 ^= First_qtr;	/* Subtract offset to middle */
	 x2 ^= First_qtr;
      } else {
	 break;			/* Otherwise exit loop.     */
      }
      x1 = 2 * x1;
      x2 = 2 * x2 + 1;	/* Scale up code range.     */
   }

}

/* Decode one bit from the archive, splitting [x1, x2] as in the encoder
and returning 1 or 0 depending on which subrange the archive point x is in.
*/
inline int Encoder::decode() {

  // Split the range
  const U32 p=predictor.p()*(4096/PSCALE); // P(1) * 4K
  assert(p<4096);
  const U32 xdiff=x2-x1;
  U32 a,b,c;
  U32 xmid;  // = x1+p*(x2-x1) multiply without overflow, round down

  c = 2*p + 1;
  a = xdiff >> 13;
  b = xdiff & 0x1fff;
  xmid = x1 + a * c + (( c * b ) >> 13);

  // Update the range
  int y=0;
  if (x<=xmid) {
    y=1;
    x2=xmid;
  }
  else
    x1=xmid+1;
  predictor.upd(y);

  // Shift equal MSB's out
  for (;;) {
      if ( x2 < Half ) {
      } else if (x1 >= Half) {	/* Output 1 if in high half. */
	 x1 -= Half;
	 x -= Half;
	 x2 -= Half;		/* Subtract offset to top.  */
      } else if (x1 >= First_qtr	/* Output an opposite bit   */
		 && x2 < Third_qtr) {	/* later if in middle half. */
	 x1 -= First_qtr;	/* Subtract offset to middle */
	 x -=  First_qtr;
	 x2 -= First_qtr;
      } else {
	 break;			/* Otherwise exit loop.     */
      }
      x1 = 2 * x1;
      x = 2 * x + input_bit();
      x2 = 2 * x2 + 1;	/* Scale up code range.     */
   }
  return y;
}
// Should be called when there is no more to compress
void Encoder::flush() {
  // In COMPRESS mode, write out the remaining bytes of x, x1 < x < x2
  if (mode==COMPRESS) {
  if ( x1 == 0 ) 
     bit_plus_follow(0);
   else 
     bit_plus_follow(1); 
	if (bout) putc(bout,archive);

   }
}

//////////////////////////// Transformer ////////////////////////////

/* A transformer compresses 1 byte at a time.  It also provides a
   place to insert transforms or filters in the future.

  Transformer tf(COMPRESS, f) -- Initialize for compression to archive f
    which must be open in "wb" mode with the header already written
  Transformer tf(DECOMPRESS, f) -- Initialize for decompression from f which
    must be open in "rb" mode past the header
  tf.encode(c) -- Compress byte c
  c = tf.decode() -- Decompress byte c
  tf.flush() -- Should be called when compression is finished
*/

class Transformer {
  Encoder e;
public:
  Transformer(Mode mode, FILE* f): e(mode, f) {}
  void encode(int c) {
    if ((z==0 || tf!=0) && c/32==1 && ((c&15)==11 || (c&15)==15)) c^=16;
    for (int i=7; i>=0; --i)
      e.encode((c>>i)&1);
  }
  U32 decode() {
    U32 c=0;
    for (int i=0; i<8; ++i)
      c=c+c+e.decode();
    if ((z==0 || tf!=0) && c/32==1 && ((c&15)==11 || (c&15)==15)) c^=16;
    return c;
  }
  void flush() {
    e.flush();
  }
};

#define CONSTA 292*1024+512
char *st0, *st;
extern "C" {
void te8e9(char*, int, int, int*);
}

//////////////////////////// main ////////////////////////////

// Read and return a line of input from FILE f (default stdin) up to
// first control character except tab.  Skips CR in CR LF.
string getline(FILE* f=stdin) {
  int c;
  string result="";
  while ((c=getc(f))!=EOF && (c>=32 || c=='\t'))
    result+=char(c);
  if (c=='\r')
    (void) getc(f);
  return result;
}


// User interface
int main(int argc, char** argv) {

  // Check arguments
  if (argc<2) {
    printf(
      PROGNAME " v4.0 file compressor/archiver, (C)2004, Matt Mahoney, mmahoney@cs.fit.edu\n"
          "                                        & Alexander Ratushnyak, artest@inbox.ru\n"
      "This program is free software distributed without warranty under the terms\n"
      "of the GNU General Public License, see http://www.gnu.org/licenses/gpl.txt\n"
      "\n"
      "To compress:         " PROGNAME " -6 archive filenames...  (archive will be created)\n"
      "  or (MSDOS):        dir/b | " PROGNAME " -6 archive  (reads file names from input)\n"
      "To extract/compare:  " PROGNAME " archive  (does not clobber existing files)\n"
      "To view contents:    more < archive\n"
      "\n"
      "Compression option  Memory needed to compress/decompress\n"
      "------------------  ------------------------------------\n"
      " -0                  ~8 Mb (fastest)\n"
      " -1                 ~12 Mb\n"
      " -2                 ~15 Mb\n"
      " -3                 ~30 Mb\n"
      " -4                 <60 Mb\n"
      " -5                <120 Mb\n"
      " -6                <230 Mb (default)\n"
      " -7                <460 Mb\n"
      " -8                <920 Mb\n"
      " -9               <1840 Mb (best compression, slowest)\n");
    return 1;
  }

  // Read and remove -MEM option
  if (argc>1 && argv[1][0]=='-') {
    if (isdigit(argv[1][1])) {
      MEM=argv[1][1]-'0';
      if (argv[1][2]=='e') exe=1;
    }
    else
      printf("Option %s ignored\n", argv[1]);
    argc--;
    argv++;
  }

  // File names and sizes from input or archive
  vector<string> filename; // List of names
  vector<long> filesize;   // Size or -1 if error
  int uncompressed_bytes=0, compressed_bytes=0;  // Input, output sizes
  FILE* archive=fopen(argv[1], "rb");


  // Extract files
  if (archive) {
    if (argc>2) {
      printf("File %s already exists\n", argv[1]);
      return 1;
    }

    // Read PROGNAME " -m\r\n" at start of archive
    string s=getline(archive);
    if (s.substr(0, string(PROGNAME).size()) != PROGNAME) {
      printf("Archive file %s not in " PROGNAME " format\n", argv[1]);
      return 1;
    }

    exe=0;
    // Get option -m where m is a digit
    if (s.size()>2) {
      if (s[s.size()-2]=='-') {
        int c=s[s.size()-1];
        if (c>='0' && c<='9')
          MEM=c-'0';
      }
      else
      if (s[s.size()-3]=='-') {
        int c=s[s.size()-2];
        if (c>='0' && c<='9')
          MEM=c-'0';
        c=s[s.size()-1];
	if (c=='e') exe=1;
      }
    }
    if (exe)
    printf("Extracting archive " PROGNAME " -%de %s ...\n", MEM, argv[1]);
    else
    printf("Extracting archive " PROGNAME " -%d %s ...\n", MEM, argv[1]);

    // Read "size filename" in "%d\t%s\r\n" format
    while (true) {
      string s=getline(archive);
      if (s.size()>1) {
        filesize.push_back(atol(s.c_str()));
        string::iterator tab=find(s.begin(), s.end(), '\t');
        if (tab!=s.end())
          filename.push_back(string(tab+1, s.end()));
        else
          filename.push_back("");
      }
      else
        break;
    }

    // Test end of header for "\f\0"
/*  {
      int c1=0, c2=0;
      if ((c1=getc(archive))!='\f' || (c2=getc(archive))!=0) {
        printf("%s: Bad " PROGNAME " header format %d %d\n", argv[1],
          c1, c2);
        return 1;
      }
    }
*/

    // Extract files from archive data
    Transformer e(DECOMPRESS, archive);
    for (int i=0; i<int(filename.size()); ++i) {
      printf("%10ld %s: ", filesize[i], filename[i].c_str());

      // Compare with existing file
      FILE* f=fopen(filename[i].c_str(), "rb");
      long size=filesize[i];
      uncompressed_bytes+=size;
      if (f) {
        bool different=false;
        fsize=size-513216;
        for (long j=0; j<size; ++j) {
          int c1=e.decode();
          int c2=getc(f);
          if (!different && c1!=c2) {
            printf("differ at offset %ld, archive=%d file=%d\n",
              j, c1, c2);
            different=true;
          }
        }
        if (!different)
          printf("identical\n");
        fclose(f);
      }

      // Extract to new file
      else {
        f=fopen(filename[i].c_str(), "wb");
        if (!f)
          printf("cannot create, skipping...\n");
        fsize=size-513216;
        if (exe && size>0) {
          int c=e.decode();
          if (f) putc(c, f);
          if (c) {
		c-=23;
		if ( (c<0 && ((c+8)&1)) || (c>=0 && ((c+2)&1)) )
		{
          	  int c=e.decode();
          	  if (f) putc(c, f);
          	  int d=e.decode();
          	  if (f) putc(d, f);
		  d=d*256+c;
	        	for (long j=0; j<d*3; ++j) {
        		  int c=e.decode();
                          if (f) putc(c, f);
        		}
		}
	  }
	}
        for (long j=0; j<size; ++j) {
          int c=e.decode();
          if (f) putc(c,f);
        }
        if (f) {
          printf("extracted\n");
		if (exe && size>0)
		{
		int data2write[4]={0,0,0,0};

			fclose(f);
			f=fopen(filename[i].c_str(), "rb");
			fseek(f, 0L, SEEK_END);
			long flen= ftell(f);
			fseek(f, 0L, 0);

		if ((st0=(char*)malloc(CONSTA+2*flen+256))==NULL) handler();
		st=st0+256-((int)st0&255);	// 256-byte-alignment
		flen=fread(st+CONSTA-32768,1,flen,f);
			fclose(f);
			f=fopen(filename[i].c_str(), "wb");

		if (flen) te8e9(st,4,flen,&data2write[0]);
		if (data2write[0])
		  fwrite((char *)data2write[1],1,data2write[0],f);

		free(st0);
		}
          fclose(f);
        }
      }
    }
    compressed_bytes=ftell(archive);
  }


  // Compress files
  else {

    // Read file names from command line or input
    if (argc>2)
      for (int i=2; i<argc; ++i)
        filename.push_back(argv[i]);
    else {
      printf(
        "Enter names of files to compress, followed by blank line or EOF.\n");
      while (true) {
        string s=getline(stdin);
        if (s=="")
          break;
        else
          filename.push_back(s);
      }
    }

    // Get file sizes
    int i;
    for (i=0; i<int(filename.size()); ++i) {
      FILE* f=fopen(filename[i].c_str(), "rb");
      if (!f) {
        printf("File not found, skipping: %s\n",
          filename[i].c_str());
        filesize.push_back(-1);
      }
      else {
        fseek(f, 0L, SEEK_END);
        filesize.push_back(ftell(f));
        fclose(f);
      }
    }
    if (filesize.empty() || *max_element(filesize.begin(), filesize.end())<0){
      printf("No files to compress, no archive created.\n");
      return 1;
    }

    // Write header
    archive=fopen(argv[1], "wb");
    if (!archive) {
      printf("Cannot create archive: %s\n", argv[1]);
      return 1;
    }
    fprintf(archive, PROGNAME " -%d", MEM);
    if (exe)
    fprintf(archive, "e");
    fprintf(archive, "\r\n");
    for ( i=0; i<int(filename.size()); ++i) {
      if (filesize[i]>=0)
        fprintf(archive, "%ld\t%s\r\n", filesize[i], filename[i].c_str());
    }
    putc(032, archive);  // MSDOS EOF
//    putc('\f', archive);
//    putc(0, archive);


    // Write data
    Transformer e(COMPRESS, archive);
    long file_start=ftell(archive);
    for ( i=0; i<int(filename.size()); ++i) {
      long size=filesize[i];
      if (size>=0) {
        uncompressed_bytes+=size;
        printf("%-23s %10ld -> ", filename[i].c_str(), size);
        FILE* f=fopen(filename[i].c_str(), "rb");
        fsize=size-513216;

		if (f && exe && size>0)
		{
		int data2write[4]={0,0,0,0};
			FILE* fw=fopen("paqar11tmp.tmp", "wb");

			fseek(f, 0L, SEEK_END);
			long flen= ftell(f);
			fseek(f, 0L, 0);

		if ((st0=(char*)malloc(CONSTA+2*flen+256))==NULL) handler();
		st=st0+256-((int)st0&255);	// 256-byte-alignment
		flen=fread(st+CONSTA-32768,1,flen,f);

		if (flen) te8e9(st,3,flen,&data2write[0]);
 		size=0;
		if (data2write[0])
		  size+=data2write[0],
		  fwrite((char *)data2write[1],1,data2write[0],fw);
		if (data2write[2])
		  size+=data2write[2],
		  fwrite((char *)data2write[3],1,data2write[2],fw);
		free(st0);

		fclose(fw);
		fclose(f);
		f=fopen("paqar11tmp.tmp", "rb");
		}

        int c;
        for (long j=0; j<size; ++j) {
          if (f)
            c=getc(f);
          else
            c=0;
          e.encode(c); 
        }
        if (f)
          fclose(f);
		if (f && exe && size>0) remove("paqar11tmp.tmp");
        printf("%ld\n", ftell(archive)-file_start);
        file_start=ftell(archive);
      }
    }
    e.flush();
    compressed_bytes=ftell(archive);
    fclose(archive);
  }

  // Report statistics
  const double elapsed_time =
    double(clock()-programChecker.start_time())/CLOCKS_PER_SEC;
  printf("%d/%d in %1.2f sec.", compressed_bytes, uncompressed_bytes,
    elapsed_time);
  if (uncompressed_bytes>0 && elapsed_time>0) {
    printf(" (%1.4f bpb, %1.2f%% at %1.0f KB/s)",
      compressed_bytes*8.0/uncompressed_bytes,
      compressed_bytes*100.0/uncompressed_bytes,
      uncompressed_bytes/(elapsed_time*1000.0));
  }
  printf("\n");
  return 0;
}
