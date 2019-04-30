/* PAQ6D - File archiver and compressor.
(C) 2003, Matt Mahoney, mmahoney@cs.fit.edu

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation at
http://www.gnu.org/licenses/gpl.txt or (at your option) any later version.
This program is distributed without any warranty.


USAGE

To compress:      PAQ6A -m archive file file...  (1 or more file names), or
  or (MSDOS):     dir/b | PAQ6A -m archive       (read file names from input)
  or (UNIX):      ls    | PAQ6A -m archive
To decompress:    PAQ6A archive
To list contents: more < archive

Compression:  The files listed are compressed and stored in the archive,
which is created.  The archive must not already exist.  File names may
specify a path, which is stored.  If there are no file names on the command
line, then PAQ6A prompts for them, reading until the first blank line or
end of file.

Decompression:  No file names are specified.  The archive must exist.
If a path is stored, the file is extracted to the appropriate directory,
which must exist.  PAQ6A does not create directories.  If the file to be
extracted already exists, it is not replaced; rather it is compared with
the archived file, and the offset of the first difference is reported.

It is not possible to add, remove, or update files in an existing archive.
If you want to do this, extract the files, delete the archive, and
create a new archive with just the files you want.

The -m is optional and ranges from -0 to -9.  Higher numbers mean
compress better but slower and use more memory.  The default is -5
which uses about 184 MB.  Increasing by 1 doubles memory usage.
The option is not used for decompression.  Rather it is read from
the archive.  Decompression requires the same amount of memory.


TO COMPILE

gxx -O PAQ6A.cpp        DJGPP 2.95.2  (gives fastest executable)
bcc32 -O2 PAQ6A.cpp     Borland 5.5.1
sc -o PAQ6A.cpp         Digital Mars 8.35n


PAQ6D is still in development.  When done it will be called PAQ6.
The description below is for PAQ6A.  I modified it by adding an extra
bit to the SSE context to distinguish text and binary files, added
case insensitive contexts to the word model, added a CCITT (calgary/pic)
model, added an extra context to the record model (2 most likely record
sizes), and some minor improvements.  It compresses the Calgary corpus
to 657,970 bytes in 616 sec. (750 MHz PC) using the default memory
size (-5).


PAQ6A DESCRIPTION

1. OVERVIEW

A PAQ6A archive has a header, listing the names and lengths of the files
it contains in human-readable format, followed by the compressed data.
The first line of the header is "PAQ6A -m" where -m is the memory option.
The data is compressed as if all the files were concatenated into one
long string.

PAQ6A uses predictive arithmetic coding.  The string, y, is compressed
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

To simplify coding, PAQ6A uses a binary string alphabet.  Thus, the
output of a model is an estimate of P(y_i = 1 | context) (henceforth p),
where y_i is the i'th bit, and the context is the previous i - 1 bits of
uncompressed data.


2.  PAQ6A MODEL

The PAQ6A model consists of a weighted mix of independent submodels which
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

  Fig. 2.  PAQ6A Model details for compression.  The model is identical for
  decompression, but the encoder is replaced with a decoder.

In Sections 2-6, the description applies to the default memory option
(-5, or MEM = 5).  For smaller values of MEM, some components are
omitted and the number of contexts is less.


3.  SUBMODELS

Individual submodels output a prediction in the form of 2 counts (n0, n1)
representing counts of past 0's and 1's that have occurred previously
in the same context.  After the result of the prediction becomes known
the appropriate count is updated.  A pair of counts represents a
probability p:

  p = n1 / n = n1 / (n0 + n1)                                           (3)

with confidence n = n0 + n1.

A pair of counts is associated with each context.  By context, we mean
a partition of all previous input, i.e. y_1 y_2 ... y_i-1 when predicting
the i'th bit y_i.  For example, one might choose the last k bits to be
the context.  In this case, there would be 2^k possible contexts, with
one pair of counters associated with each.

After the i'th bit is encoded and becomes known to the model to be a
0 or 1, we update the appropriate counter associated with the current
context.  However, the counts are decayed in order to favor newer data
over old by the following algorithm:

  n_y := n+y + 1
  if n_1-y > 2 then n_1-y := 1 + n_1-y / 2                              (4)

In other words, when we observe a 1, we discard half of the 0 count
over 2.  This favors newer data, but also gives greater weight to
long runs of 0s or 1s.  For example:

  Input             n0  n1   p    Confidence = n  Stationary model
  0000000000        10   0   0    10              0/10
  00000000001        6   1   1/7   7              1/11
  000000000011       4   2   2/6   6              2/12
  0000000000111      3   3   3/6   6              3/13
  00000000001111     2   4   4/6   6              4/14
  000000000011111    2   5   5/7   7              5/15
  0000000000111111   2   6   6/8   8              6/16

  Table. 1.  Nonstationary counter example.  For comparison, the predictions
  of a stationary model are also shown.

This technique allows nonstationary sources to be modeled.  Had we
assumed that the input are independent (as some models do), we would
have estimated the 1 bits incorrectly.

Because both n0 and n1 cannot be large, it is possible to represent a
pair of counts compactly.  PAQ6A represents both counts as a single
8-bit state.  Large counts are approximated with probabilistic increments.
The allowable values are 0-20, 24, 28, 32, 48, 64, 128,
256, and 512.  For example, if n0 = 20 and we update with y = 0, then
instead of assigning n = 21, we assign n = 24 with 25% probability.
Also, for n0 > 15, n1 cannot exceed 1, and vice versa.

A second type of model considers only runs of identical byte values
within a context.  If a byte c occurs within a context m times in a row,
then we add m to either n0 or n1 depending on the corresponding bit within
c that we wish to predict.  This run length model requires 2 bytes to
represent, one for c and one for the 8-bit counter m, which is limited
to 255.


4.  SUBMODEL DETAILS

Submodels differ only in their contexts.  These are as follows:

a. DefaultModel.  (n0, n1) = (1, 1) regardless of context.

b. CharModel (n-gram model).  A context consists of the last n whole
bytes, plus the 0 to 7 bits of the partially read current byte.  There
are 8 models with n ranging from 0 to 7.  For n > 2, the context is
hashed and the counters are stored in a hash table of the following size:

  n  Number of contexts  (2^k means 2 to the power of k)
  -  ------------------
  0  2^8
  1  2^16
  2  2^20
  3  2^22
  4  2^23
  5  2^23
  6  2^23
  7  2^23
  8  2^23
  9  2^23

  Table 2.  Maximum number of contexts of length n bytes.

In addition, run length models are stored for 2 <= n <= 7.  There are
1/8 as many contexts for run length models as for nonstationary counters.

c.  MatchModel (long context).  A context is the last n whole bytes
(plus extra bits) where n >=8.  Up to 4 matching contexts are found by
indexing into a rotating input buffer of length 2^24 bytes.  The index
is a hash table of 2^22 pointers (32-bit, although 24-bit would be
sufficient) without collision detection.  Hashes are indexed using a
hash of the last 8 bytes, except that in 1/16 of the cases the last
32 bytes are hashed in order to find very long contexts.  For each
context match of length n bytes, the next bit (n0 or n1) is given
a weight of n^2/4 (maximum 511).

For mixing, the following models are added together: n = 2 and 3, 4 and 5,
6 and 7.  Thus, the output appears as 5 submodel predictions.

d.  RecordModel.  This models data with fixed length records, such as
tables.  The model attempts to find the record length by searching for
characters that repeat in the pattern x..x..x..x where the interval
between 4 successive occurrences is identical and at least 3.  If a
pattern is found, the record length remains fixed until another is found.
Two contexts are modeled:

  1. The two bytes above the current bit.
  2. The byte above and the previous byte (to the left).

Both contexts also include the record length and the current 0-7 bits,
which are all hashed together.  For each context, there are 2^22
pairs of counters plus 2^19 run length counts.  The counts from the
two contexts are added prior to mixing.

e.  SparseModel.  This models contexts with gaps.  It considers the
following contexts, where x denotes the bytes considered and ? denotes
the bit being predicted (plus preceding bits, which are included in
the context).

       xx?        x.x?
     x..x?      x...x?
   x....x?    x.....x?
 x......x?        xx.?
     x.x.?      x..x.?

  Table 3.  Sparse models.

For example, the first model considers the last and third to last bytes.
There are three outputs fed to the mixer, combined by adding the two
counter pairs in each of the three rows.  There are 2^20 counter pairs
and 2^17 run length counts associated with each of the 10 contexts.

e.  AnalogModel.  This is intended to model 16-bit audio (mono or stereo),
24-bit color images, and 8-bit data (such as grayscale images).  Contexts
drop the low order bits, and include the position within the file
modulo 2, 3, or 4.  There are 6 models, combined into 3 by addition
before mixing.  An x represents those bits which are part of the context.

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

  Table 4.  Analog models.

Each of the 6 contexts is modeled by 2^18 counter pairs and 2^15 run
length counters.

f.  WordModel.  This is intended to model text files.  The string
is parsed into words by splitting on white space, which is considered
to be any character with an ASCII code of 32 or less.  The remaining
characters are hashed and form the context.  There is one sparse model
which skips a word.  In the table below, x now represents a whole
word.  The context also includes all of the bits of the current word
so far.  There are 3 contexts grouped into 2 prior to mixing:

    ?  (unigram model, 2^24 counter pairs and 2^21 run lengths)
   x?  (bigram model, 2^23 counter pairs and 2^20 run lengths)
  x.?  (sparse bigram model, 2^23 counter pairs and 2^20 run lengths,
        grouped with bigram model)

  Table 5.  Word models.


5.  MIXER

The mixers compute a probability by a weighted summation of the N = 18
counter pairs from the submodels.

      SUM_i=1..N w_i n1_i                                               (5)
  p = -------------------
      SUM_i=1..N w_i n_i

where w_i is the i'th weight, n1_i is the 1 bit count from the i'th
submodel, and n_i = n0_i + n1_i.

The weights w_i are adjusted after each bit of uncompressed data becomes
known in order to reduce the cost (code length) of that bit.  The
adjustment is along the gradient of the cost in weight space, which is

  w_i := w_i + e[ny_i/(SUM_j (w_j+wo) ny_j) - n_i/(SUM_j (w_j+wo) n_j)]

where e and wo are small constants, and ny_i is the count for bit y
(either n0 or n1) from the i'th submodel when the actual bit is y.
The weight offset wo prevents the gradient from going to infinity as the
weights go to 0.

There are two mixers, whose outputs are averaged together before being
input to the SSE stage.  Each mixer maintains a set of weights which
is selected by a context.  Mixer 1 maintains 8 weight vectors, selected
by the 3 high order bits of the previous byte.  Mixer 2 maintains 16
weight vectors, selected by the 2 high order bits of each of the
previous 2 bytes.


6.  SSE

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

SSE is context sensitive.  There are 1024 separately maintained SSE
functions, selected by the 0-7 bits of the current (partial) byte and
the 2 high order bits of the previous byte.

The final output to the encoder is a weighted average of the SSE
input and output, with the output receiving 3/4 of the weight:

  p := (3 SSE(p) + p) / 4.                                              (6)


7.  MEMORY USAGE

The -m option (MEM = 0 through 9) controls model and memory usage.  Smaller
numbers compress faster and use less memory, while higher numbers compress
better.

For MEM < 5, only one mixer is used.  For MEM < 4, bit counts are stored
in nonstationary counters, but no run length is stored (decreasing
memory by 20%).  For MEM < 1, SSE is not used.  For MEM < 5, the record,
sparse, and analog models are not used.  For MEM < 4, the word model is
not used.  The order of the char model ranges from 4 to 9 depending on
MEM for MEM as shown in Table 6.

              Run        Memory used by...                        Total
 MEM   Mixers Len  Order Char Match Record Sparse Analog Word SSE Memory (MB)
 ---   ------ ---  ----- ---- ----- ------ ------ ------ ---- --- -----------
  0      1    no   4      .5    1                                      1.5
  1      1    no   5       1    2                             .06      3
  2      1    no   5       2    4                             .06      6
  3      1    no   7       8    8                             .06     16
  4      1   yes   7      20   16       6                 20  .06     58
  5      2   yes   9      66   32      12    12      2    40  .06    165
  6      2   yes   9     132   64      25    25      4    80  .06    330
  7      2   yes   9     264  128      50    50      7.5 160  .06    660
  8      2   yes   9     528  256     100   100     15   320  .06   1320
  9      2   yes   9    1056  512     200   200     30   640  .06   2640

  Table 6.  Memory usage depending on MEM (-0 to -9 option).
  

8.  EXPERIMENTAL RESULTS

Results on the Calgary corpos are shown below for some top data compressors
as of Dec. 2003.  Options are set for maximum compression.  When possible,
the files are all compressed into a single archive.

  Original size   Options        3,141,622  Time   Author
  -------------   -------        ---------  ----   ------
  gzip 1.2.4      -9             1,017,624     2   Jean Loup Gailly
  epm r9          c                668,115    49   Serge Osnach
  rkc             a -M80m -td+     661,102    91   Malcolm Taylor
  slim 19         a                659,358   153   Serge Voskoboynikov
  compressia 1.0 beta              650,398    66   Yaakov Gringeler
  durilca v.03a (as in README)     647,028    30   Dmitry Shkarin
  PAQ5                             661,811   361   Matt Mahoney
  WRT11 + PAQ5                     638,635   258   Przemyslaw Skibinski +
  PAQ6A           -0               863,871    48   Berto Destasio +
                  -1               786,181    61
                  -2               730,653    73
                  -3               714,302    90
                  -4               696,235   148
                  -5               661,611   457

  Table 7.  Compressed size of the Calgary corpus.

WRT11 is a word reducing transform written by Przemyslaw Skibinski.  It
uses an external English dictionary to replace words with 1-3 byte
symbols to improve compression.  rkc, compressia, and durilca use a
similar approach.  Among dictionary compressors, WRT + PAQ5 gives the
best compression.  Among non-dictionary compressors, slim slightly
outperforms PAQ5.

PAQ5 was designed for maximum compression rather than speed, and is
the slowest compressor listed.  Compression is in seconds and was timed
on a 750 MHz Duron with 256 MB memory under Windows Me.  To obtain this
compression, it was often necessary to sacrifice 25% speed and memory
to gain 1% of compression.


8.  ACKNOWLEDGMENTS

Thanks to Serge Osnach for introducing me to SSE (in PAQ1SSE/PAQ2) and
the sparse models (PAQ3N).  Also, credit to Eugene Shelwein,
Dmitry Shkarin for suggestions on using multiple character SSE contexts.
Credit to Eugene, Serge, and Jason Schmidt for developing faster and
smaller executables of previous versions.  Credit to Werner Bergmans
and Berto Destasio for testing and evaluating them, including modifications
that improve compression at the cost of more memory.  Credit to
Alexander Ratushnyak who found a bug in PAQ4 decompression (now fixed).

Thanks to Berto for writing PAQ5-EMILCONT-DEUTERIUM from which this
program is derived (which he derived from PAQ5).  His improvements to
PAQ5 include a new Counter state table and additional contexts for
CharModel and SparseModel.  The only significant change I made was to add
the -m option.

I expect there will be better versions in the future.  If you make any
changes, please change the name of the program (e.g. PAQ7), including
the string in the archive header by redefining PROGNAME below.
This will prevent any confusion about versions or archive compatibility.
Also, give yourself credit in the help message.


PAQ6D

Still in development.  
*/

#define PROGNAME "PAQ6D" // Please change this if you change the program

#define hash ___hash // To avoid Digital MARS name collision
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <ctime>
#include <cassert>
#include <new>
#include <string>
#include <vector>
#include <algorithm>
#undef hash

using std::set_new_handler;
using std::string;
using std::swap;
using std::vector;

const int PSCALE = 2048; // Integer scale for representing probabilities
int MEM = 5;             // Use about 6 MB * 2^MEM bytes of memory

#define min( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define max( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )

template <class T>
inline int size( const T &t ) {
  return t.size();
}

// 8-32 bit unsigned types, adjust as appropriate
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

// Fail if out of memory
void handler() {
  printf( "Out of memory\n" );
  exit( 1 );
}

// A ProgramChecker verifies some environmental assumptions and sets the
// out of memory handler.  It also gets the program starting time.
// The global object programChecker should be initialized before any
// other global objects.

class ProgramChecker {
  clock_t start;

public:
  ProgramChecker() {
    start = clock();
    set_new_handler( handler );

    // Test the compiler for common but not guaranteed assumptions
    assert( sizeof( U8 ) == 1 );
    assert( sizeof( U16 ) == 2 );
    assert( sizeof( U32 ) == 4 );
    assert( sizeof( int ) == 4 );
  }
  clock_t start_time() const {
    return start;
  } // When the program started
} programChecker;

//////////////////////////// rnd ////////////////////////////

// 32-bit random number generator based on r(i) = r(i-24) ^ r(i-55)

class Random {
  U32 table[55]; // Last 55 random values
  int i{ 0 };         // Index of current random value in table
public:
  Random();
  U32 operator()() { // Return 32-bit random number
    if( ++i == 55 )
      i = 0;
    if( i >= 24 )
      return table[i] ^= table[i - 24];
    else
      return table[i] ^= table[i + 31];
  }
} rnd;

Random::Random()  { // Seed the table
  table[0] = 123456789;
  table[1] = 987654321;
  for( int j = 2; j < 55; ++j )
    table[j] = table[j - 1] * 11 + table[j - 2] * 19 / 16;
}

//////////////////////////// hash ////////////////////////////

// Hash functoid, returns 32 bit hash of 1-4 chars

class Hash {
  U32 table[8][256]; // Random number table
public:
  Hash() {
    for( int i = 7; i >= 0; --i )
      for( int j = 0; j < 256; ++j )
        table[i][j] = rnd();
    assert( table[0][255] == 3610026313LU );
  }
  U32 operator()( U8 i0 ) const {
    return table[0][i0];
  }
  U32 operator()( U8 i0, U8 i1 ) const {
    return table[0][i0] + table[1][i1];
  }
  U32 operator()( U8 i0, U8 i1, U8 i2 ) const {
    return table[0][i0] + table[1][i1] + table[2][i2];
  }
  U32 operator()( U8 i0, U8 i1, U8 i2, U8 i3 ) const {
    return table[0][i0] + table[1][i1] + table[2][i2] + table[3][i3];
  }
} hash;

//////////////////////////// Counter ////////////////////////////

/* A Counter represents a pair (n0, n1) of counts of 0 and 1 bits
in a context.

  get0() -- returns n0
  get1() -- returns n1
  add(y) -- increments ny, where y is 0 or 1
  priority() -- Returns a priority for hash replacement such that
    higher numbers should be favored.

A counter uses a nonstationary model favoring newer data.  When
ny (n0 or n1) is incremented and the opposite count is > 2, then
the excess over 2 is halved (rounding down).  For instance:

  Input         n0  n1
  -----         --  --
  00000000       8   0
  000000001      5   1
  0000000011     3   2
  00000000111    2   3
  000000001111   2   4
  0000000011111  2   5

To represent both counts using only 8 bits, the representable counts
(n0, n1) are limited to 0-10, 12, 14, 16, 20, 24, 28, 32, 48, 64, 128,
256, 512.  For counts above 10, incrementing is probabilistic and the
count is approximate.  The state represening (n0, n1) is updated using
a state table generated by stategen.cpp.  */

class Counter {
  U8 state{ 0 };
  struct E {     // State table entry
    U16 n0, n1;  // Counts represented by state
    U8 s00, s01; // Next state on input 0 without/with probabilistic incr.
    U8 s10, s11; // Next state on input 1
    U32 p0, p1;  // Probability of increment x 2^32-1 on inputs 0, 1
  };
  static E table[150]; // State table // emilcont
public:
  Counter()  {}
  int get0() const {
    return table[state].n0;
  }
  int get1() const {
    return table[state].n1;
  }
  int priority() const {
    return get0() + get1();
  }
  void add( int y ) {
    if( y != 0 ) {
      if( state < 108 || rnd() < table[state].p1 )
        state = table[state].s11;
      else
        state = table[state].s10;
    } else {
      if( state < 108 || rnd() < table[state].p0 )
        state = table[state].s01;
      else
        state = table[state].s00;
    }
  }
};

// State table generated by stategen-emilcont.cpp
Counter::E Counter::table[] = {
    //    n0  n1 s00 s01 s10 s11          p0          p1
    {0, 0, 0, 2, 0, 1, 4294967295U, 4294967295U},        // 0 emilcont
    {0, 1, 1, 4, 1, 3, 4294967295U, 4294967295U},        // 1 ALL THE TABLE REGENERATED BY EMILCONT
    {1, 0, 2, 5, 2, 4, 4294967295U, 4294967295U},        // 2
    {0, 2, 1, 4, 3, 6, 4294967295U, 4294967295U},        // 3
    {1, 1, 4, 8, 4, 7, 4294967295U, 4294967295U},        // 4
    {2, 0, 5, 9, 2, 4, 4294967295U, 4294967295U},        // 5
    {0, 3, 1, 4, 6, 10, 4294967295U, 4294967295U},       // 6
    {1, 2, 4, 8, 7, 11, 4294967295U, 4294967295U},       // 7
    {2, 1, 8, 13, 4, 7, 4294967295U, 4294967295U},       // 8
    {3, 0, 9, 14, 2, 4, 4294967295U, 4294967295U},       // 9
    {0, 4, 3, 7, 10, 15, 4294967295U, 4294967295U},      // 10
    {1, 3, 4, 8, 11, 16, 4294967295U, 4294967295U},      // 11
    {2, 2, 8, 13, 7, 11, 4294967295U, 4294967295U},      // 12
    {3, 1, 13, 19, 4, 7, 4294967295U, 4294967295U},      // 13
    {4, 0, 14, 20, 5, 8, 4294967295U, 4294967295U},      // 14
    {0, 5, 3, 7, 15, 21, 4294967295U, 4294967295U},      // 15
    {1, 4, 7, 12, 16, 22, 4294967295U, 4294967295U},     // 16
    {2, 3, 8, 13, 11, 16, 4294967295U, 4294967295U},     // 17
    {3, 2, 13, 19, 7, 11, 4294967295U, 4294967295U},     // 18
    {4, 1, 19, 26, 8, 12, 4294967295U, 4294967295U},     // 19
    {5, 0, 20, 27, 5, 8, 4294967295U, 4294967295U},      // 20
    {0, 6, 6, 11, 21, 28, 4294967295U, 4294967295U},     // 21
    {1, 5, 7, 12, 22, 29, 4294967295U, 4294967295U},     // 22
    {2, 4, 12, 18, 16, 22, 4294967295U, 4294967295U},    // 23
    {3, 3, 13, 19, 11, 16, 4294967295U, 4294967295U},    // 24
    {4, 2, 19, 26, 12, 17, 4294967295U, 4294967295U},    // 25
    {5, 1, 26, 34, 8, 12, 4294967295U, 4294967295U},     // 26
    {6, 0, 27, 35, 9, 13, 4294967295U, 4294967295U},     // 27
    {0, 7, 6, 11, 28, 36, 4294967295U, 4294967295U},     // 28
    {1, 6, 11, 17, 29, 37, 4294967295U, 4294967295U},    // 29
    {2, 5, 12, 18, 22, 29, 4294967295U, 4294967295U},    // 30
    {3, 4, 18, 25, 16, 22, 4294967295U, 4294967295U},    // 31
    {4, 3, 19, 26, 17, 23, 4294967295U, 4294967295U},    // 32
    {5, 2, 26, 34, 12, 17, 4294967295U, 4294967295U},    // 33
    {6, 1, 34, 42, 13, 18, 4294967295U, 4294967295U},    // 34
    {7, 0, 35, 43, 9, 13, 4294967295U, 4294967295U},     // 35
    {0, 8, 6, 11, 36, 44, 4294967295U, 4294967295U},     // 36
    {1, 7, 11, 17, 37, 45, 4294967295U, 4294967295U},    // 37
    {2, 6, 17, 24, 29, 37, 4294967295U, 4294967295U},    // 38
    {3, 5, 18, 25, 22, 29, 4294967295U, 4294967295U},    // 39
    {5, 3, 26, 34, 17, 23, 4294967295U, 4294967295U},    // 40
    {6, 2, 34, 42, 18, 24, 4294967295U, 4294967295U},    // 41
    {7, 1, 42, 50, 13, 18, 4294967295U, 4294967295U},    // 42
    {8, 0, 43, 51, 9, 13, 4294967295U, 4294967295U},     // 43
    {0, 9, 10, 16, 44, 60, 4294967295U, 4294967295U},    // 44
    {1, 8, 11, 17, 45, 52, 4294967295U, 4294967295U},    // 45
    {2, 7, 17, 24, 37, 45, 4294967295U, 4294967295U},    // 46
    {3, 6, 24, 32, 29, 37, 4294967295U, 4294967295U},    // 47
    {6, 3, 34, 42, 24, 31, 4294967295U, 4294967295U},    // 48
    {7, 2, 42, 50, 18, 24, 4294967295U, 4294967295U},    // 49
    {8, 1, 50, 57, 13, 18, 4294967295U, 4294967295U},    // 50
    {9, 0, 51, 61, 14, 19, 4294967295U, 4294967295U},    // 51
    {1, 9, 16, 23, 52, 63, 4294967295U, 4294967295U},    // 52
    {2, 8, 17, 24, 45, 52, 4294967295U, 4294967295U},    // 53
    {3, 7, 24, 32, 37, 45, 4294967295U, 4294967295U},    // 54
    {7, 3, 42, 50, 24, 31, 4294967295U, 4294967295U},    // 55
    {8, 2, 50, 57, 18, 24, 4294967295U, 4294967295U},    // 56
    {9, 1, 57, 64, 19, 25, 4294967295U, 4294967295U},    // 57
    {2, 9, 23, 31, 52, 63, 4294967295U, 4294967295U},    // 58
    {9, 2, 57, 64, 25, 32, 4294967295U, 4294967295U},    // 59
    {0, 10, 10, 16, 60, 62, 4294967295U, 4294967295U},   // 60
    {10, 0, 61, 65, 14, 19, 4294967295U, 4294967295U},   // 61
    {0, 11, 15, 22, 62, 66, 4294967295U, 4294967295U},   // 62
    {1, 10, 16, 23, 63, 67, 4294967295U, 4294967295U},   // 63
    {10, 1, 64, 70, 19, 25, 4294967295U, 4294967295U},   // 64
    {11, 0, 65, 71, 20, 26, 4294967295U, 4294967295U},   // 65
    {0, 12, 15, 22, 66, 72, 4294967295U, 4294967295U},   // 66
    {1, 11, 22, 30, 67, 73, 4294967295U, 4294967295U},   // 67
    {2, 10, 23, 31, 63, 67, 4294967295U, 4294967295U},   // 68
    {10, 2, 64, 70, 25, 32, 4294967295U, 4294967295U},   // 69
    {11, 1, 70, 76, 26, 33, 4294967295U, 4294967295U},   // 70
    {12, 0, 71, 77, 20, 26, 4294967295U, 4294967295U},   // 71
    {0, 13, 21, 29, 72, 78, 4294967295U, 4294967295U},   // 72
    {1, 12, 22, 30, 73, 79, 4294967295U, 4294967295U},   // 73
    {2, 11, 30, 39, 67, 73, 4294967295U, 4294967295U},   // 74
    {11, 2, 70, 76, 33, 40, 4294967295U, 4294967295U},   // 75
    {12, 1, 76, 82, 26, 33, 4294967295U, 4294967295U},   // 76
    {13, 0, 77, 83, 27, 34, 4294967295U, 4294967295U},   // 77
    {0, 14, 21, 29, 78, 84, 4294967295U, 4294967295U},   // 78
    {1, 13, 29, 38, 79, 85, 4294967295U, 4294967295U},   // 79
    {2, 12, 30, 39, 73, 79, 4294967295U, 4294967295U},   // 80
    {12, 2, 76, 82, 33, 40, 4294967295U, 4294967295U},   // 81
    {13, 1, 82, 88, 34, 41, 4294967295U, 4294967295U},   // 82
    {14, 0, 83, 89, 27, 34, 4294967295U, 4294967295U},   // 83
    {0, 15, 28, 37, 84, 90, 4294967295U, 4294967295U},   // 84
    {1, 14, 29, 38, 85, 91, 4294967295U, 4294967295U},   // 85
    {2, 13, 38, 47, 79, 85, 4294967295U, 4294967295U},   // 86
    {13, 2, 82, 88, 41, 48, 4294967295U, 4294967295U},   // 87
    {14, 1, 88, 92, 34, 41, 4294967295U, 4294967295U},   // 88
    {15, 0, 89, 93, 35, 42, 4294967295U, 4294967295U},   // 89
    {0, 16, 28, 37, 90, 94, 4294967295U, 4294967295U},   // 90
    {1, 15, 37, 46, 91, 95, 4294967295U, 4294967295U},   // 91
    {15, 1, 92, 98, 42, 49, 4294967295U, 4294967295U},   // 92
    {16, 0, 93, 99, 35, 42, 4294967295U, 4294967295U},   // 93
    {0, 17, 36, 45, 94, 100, 4294967295U, 4294967295U},  // 94
    {1, 16, 37, 46, 95, 101, 4294967295U, 4294967295U},  // 95
    {2, 15, 46, 54, 91, 95, 4294967295U, 4294967295U},   // 96
    {15, 2, 92, 98, 49, 55, 4294967295U, 4294967295U},   // 97
    {16, 1, 98, 102, 42, 49, 4294967295U, 4294967295U},  // 98
    {17, 0, 99, 103, 43, 50, 4294967295U, 4294967295U},  // 99
    {0, 18, 36, 45, 100, 104, 4294967295U, 4294967295U}, // 100
    {1, 17, 45, 53, 101, 105, 4294967295U, 4294967295U}, // 101
    {17, 1, 102, 106, 50, 56, 4294967295U, 4294967295U}, // 102
    {18, 0, 103, 107, 43, 50, 4294967295U, 4294967295U}, // 103
    {0, 19, 44, 52, 104, 108, 4294967295U, 4294967295U}, // 104
    {1, 18, 45, 53, 105, 109, 4294967295U, 4294967295U}, // 105
    {18, 1, 106, 110, 50, 56, 4294967295U, 4294967295U}, // 106
    {19, 0, 107, 111, 51, 57, 4294967295U, 4294967295U}, // 107
    {0, 20, 44, 52, 108, 114, 4294967295U, 1073741823U}, // 108
    {1, 19, 52, 58, 109, 112, 4294967295U, 4294967295U}, // 109
    {19, 1, 110, 113, 57, 59, 4294967295U, 4294967295U}, // 110
    {20, 0, 111, 115, 51, 57, 1073741823U, 4294967295U}, // 111
    {1, 20, 52, 58, 112, 116, 4294967295U, 1073741823U}, // 112
    {20, 1, 113, 117, 57, 59, 1073741823U, 4294967295U}, // 113
    {0, 24, 60, 63, 114, 118, 4294967295U, 1073741823U}, // 114
    {24, 0, 115, 119, 61, 64, 1073741823U, 4294967295U}, // 115
    {1, 24, 63, 68, 116, 120, 4294967295U, 1073741823U}, // 116
    {24, 1, 117, 121, 64, 69, 1073741823U, 4294967295U}, // 117
    {0, 28, 60, 63, 118, 122, 4294967295U, 1073741823U}, // 118
    {28, 0, 119, 123, 61, 64, 1073741823U, 4294967295U}, // 119
    {1, 28, 63, 68, 120, 124, 4294967295U, 1073741823U}, // 120
    {28, 1, 121, 125, 64, 69, 1073741823U, 4294967295U}, // 121
    {0, 32, 62, 67, 122, 126, 4294967295U, 268435455U},  // 122
    {32, 0, 123, 127, 65, 70, 268435455U, 4294967295U},  // 123
    {1, 32, 67, 74, 124, 128, 4294967295U, 268435455U},  // 124
    {32, 1, 125, 129, 70, 75, 268435455U, 4294967295U},  // 125
    {0, 48, 62, 67, 126, 130, 4294967295U, 268435455U},  // 126
    {48, 0, 127, 131, 65, 70, 268435455U, 4294967295U},  // 127
    {1, 48, 67, 74, 128, 132, 4294967295U, 268435455U},  // 128
    {48, 1, 129, 133, 70, 75, 268435455U, 4294967295U},  // 129
    {0, 64, 66, 73, 130, 134, 4294967295U, 134217727U},  // 130
    {64, 0, 131, 135, 71, 76, 134217727U, 4294967295U},  // 131
    {1, 64, 73, 80, 132, 136, 4294967295U, 134217727U},  // 132
    {64, 1, 133, 137, 76, 81, 134217727U, 4294967295U},  // 133
    {0, 96, 66, 73, 134, 138, 4294967295U, 134217727U},  // 134
    {96, 0, 135, 139, 71, 76, 134217727U, 4294967295U},  // 135
    {1, 96, 73, 80, 136, 140, 4294967295U, 134217727U},  // 136
    {96, 1, 137, 141, 76, 81, 134217727U, 4294967295U},  // 137
    {0, 128, 72, 79, 138, 142, 4294967295U, 33554431U},  // 138
    {128, 0, 139, 143, 77, 82, 33554431U, 4294967295U},  // 139
    {1, 128, 79, 86, 140, 144, 4294967295U, 33554431U},  // 140
    {128, 1, 141, 145, 82, 87, 33554431U, 4294967295U},  // 141
    {0, 256, 72, 79, 142, 146, 4294967295U, 16777215U},  // 142
    {256, 0, 143, 147, 77, 82, 16777215U, 4294967295U},  // 143
    {1, 256, 79, 86, 144, 148, 4294967295U, 16777215U},  // 144
    {256, 1, 145, 149, 82, 87, 16777215U, 4294967295U},  // 145
    {0, 512, 84, 91, 146, 146, 4294967295U, 0U},         // 146
    {512, 0, 147, 147, 89, 92, 0U, 4294967295U},         // 147
    {1, 512, 91, 96, 148, 148, 4294967295U, 0U},         // 148
    {512, 1, 149, 149, 92, 97, 0U, 4294967295U},         // 149
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
class Ch {
  U32 N{ 0 };                    // Buffer size
  U8 *buf{ 0 };                  // [N] last N bytes
  U32 p{ 0 };                    // pos()
  U32 bp{ 0 };                   // bpos()
  U32 hi_nibble{ 0 }, lo_nibble{ 1 }; // hi(), lo()
  U32 lpos[256][4];         // pos(c, i)
public:
  Ch()  {
    memset( lpos, 0, 256 * 4 * sizeof( U32 ) );
  }
  void init() {
    N = 1 << ( 19 + MEM );
    buf = ( U8 * ) calloc( N, 1 );
    if( buf == nullptr )
      handler();
    buf[0] = 1;
  }
  U32 operator()( int i ) const {
    return buf[( p - i ) & ( N - 1 )];
  }
  U32 operator()() const {
    return buf[p & ( N - 1 )];
  }
  void update( int y ) {
    U8 &r = buf[p & ( N - 1 )];
    r += r + y;
    if( ++bp == 8 ) {
      lpos[r][3] = lpos[r][2];
      lpos[r][2] = lpos[r][1];
      lpos[r][1] = lpos[r][0];
      lpos[r][0] = p;
      bp = 0;
      ++p;
      buf[p & ( N - 1 )] = 1;
    }
    if( ( lo_nibble += lo_nibble + y ) >= 16 ) {
      hi_nibble = lo_nibble - 16;
      lo_nibble = 1;
    }
  }
  U32 pos() const {
    return p;
  }
  U32 pos( U8 c, int i = 0 ) const {
    return lpos[c][i & 3];
  }
  U32 bpos() const {
    return bp;
  }
  U32 operator[]( int i ) const {
    return buf[i & ( N - 1 )];
  }
  U32 hi() const {
    return hi_nibble;
  }
  U32 lo() const {
    return lo_nibble;
  }
} ch; // Global object

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

template <class T>
class Hashtable {
private:
  const U32 N; // log2 size in bytes
  struct HashElement {
    U8 checksum{ 0 }; // Checksum of context, used to detect collisions
    T c[15];     // 1-byte counters in minor context c
    HashElement()  {}
  };
  HashElement *table; // [2^(N-4)]
  U32 cxt;            // major context
public:
  Hashtable( U32 n );

  // Set major context to h, a 32 bit hash.  Create a new element if needed.
  void set( U32 h ) {
    // Search 4 elements for h within a 64-byte cache line
    const U8 checksum = ( h >> 24 ) ^ h;
    const U32 lo = ( h >> ( 32 - N ) ) & -4;
    const U32 hi = lo + 4;
    U32 i;
    for( i = lo; i < hi; ++i ) {
      U32 pri = table[i].c[0].priority();
      if( table[i].checksum == checksum ) { // found
        cxt = i;
        break;
      } else if( pri == 0 ) { // empty bucket
        table[i].checksum = checksum;
        cxt = i;
        break;
      }
    }

    // Put new element in front, pushing the lower priority of the two
    // oldest off the back
    if( i == hi ) {
      cxt = lo;
      if( table[lo + 2].c[0].priority() < table[lo + 3].c[0].priority() )
        memmove( table + lo + 1, table + lo, 32 );
      else
        memmove( table + lo + 1, table + lo, 48 );
      memset( table + lo, 0, 16 );
      table[cxt].checksum = checksum;
    }

    // Move newest to front
    else if( cxt != lo ) {
      HashElement he = table[cxt];
      memmove( table + lo + 1, table + lo, ( cxt - lo ) * 16 );
      table[lo] = he;
      cxt = lo;
    }
  }

  // Get element c (1-15) of bucket cxt
  T &operator()( U32 c ) {
    --c;
    assert( c < 15 );
    return table[cxt].c[c];
  }
};

template <class T>
Hashtable<T>::Hashtable( U32 n ) : N( n > 4 ? n - 4 : 1 ), table( 0 ), cxt( 0 ) {
  assert( sizeof( HashElement ) == 16 );
  assert( sizeof( char ) == 1 );

  // Align the hash table on a 64 byte cache page boundary
  char *p = ( char * ) calloc( ( 16 << N ) + 64, 1 );
  if( p == nullptr )
    handler();
  p += 64 - ( ( ( long ) p ) & 63 ); // Aligned
  table = ( HashElement * ) p;
}

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
class Mixer {
  enum { N = 32 }; // Max writes before update
  const int C;
  U32 *bc0, *bc1; // 0,1 counts for N models
  U32 ( *wt )[N]; // wt[c][n] is n'th weight in context c
  int n;          // number of bit count pairs written
  int c;          // weight set context
public:
  Mixer( int C_ );
  ~Mixer();
  U32 getN() const {
    return N;
  }
  U32 getC() const {
    return C;
  }

  // Store next counts n0, n1 from model
  void write( int n0, int n1 ) {
    bc0[n] = n0;
    bc1[n] = n1;
    ++n;
  }

  // Add to the last write
  void add( int n0, int n1 ) {
    bc0[n - 1] += n0;
    bc1[n - 1] += n1;
  }
  int predict( int c_ );
  void update( int y );
};

// Return weighted average of models in context c_
int Mixer::predict( int c_ ) {
  assert( n > 0 && n <= N );
  assert( c_ >= 0 && c_ < C );
  c = c_;
  int n0 = 1, n1 = n0;
  for( int j = 0; j < n; ++j ) {
    U32 w = wt[c][j];
    n0 += bc0[j] * w;
    n1 += bc1[j] * w;
  }
  int sum = n0 + n1;
  assert( sum > 0 );
  while( sum > 2000000000 / PSCALE )
    sum /= 4, n1 /= 4;
  return int( ( PSCALE - 1 ) * n1 / sum );
}

// Adjust the weights by gradient descent to reduce cost of bit y
void Mixer::update( int y ) {
  U32 s0 = 0, s1 = 0;
  for( int i = 0; i < n; ++i ) {
    s0 += ( wt[c][i] + 48 ) * bc0[i];
    s1 += ( wt[c][i] + 48 ) * bc1[i];
  }
  if( s0 > 0 && s1 > 0 ) {
    const U32 s = s0 + s1;
    const U32 sy = y != 0 ? s1 : s0;
    const U32 sy1 = ( 0xffffffff / sy + ( rnd() & 1023 ) ) >> 10;
    const U32 s1 = ( 0xffffffff / s + ( rnd() & 1023 ) ) >> 10;
    for( int i = 0; i < n; ++i ) {
      const int dw = int( ( y != 0 ? bc1[i] : bc0[i] ) * sy1 - ( bc0[i] + bc1[i] ) * s1 + ( rnd() & 255 ) ) >> 8;
      wt[c][i] = min( 65535, max( 1, int( wt[c][i] + dw ) ) );
    }
  }
  n = 0;
}

Mixer::Mixer( int C_ ) : C( C_ ), bc0( new U32[N] ), bc1( new U32[N] ), wt( new U32[C_][N] ), n( 0 ), c( 0 ) {
  for( int i = 0; i < C; ++i ) {
    for( int j = 0; j < N; ++j )
      wt[i][j] = 1;
  }
  for( int i = 0; i < N; ++i )
    bc0[i] = bc1[i] = 0;
}

Mixer::~Mixer() {
  /*
  // Uncomment this to print the weights.  This is useful for testing
  // new models or weight vector contexts.
  if (n==0)
    return;
  printf("  ");
  for (int i=0; i<n; ++i)
    printf("%4d", i);
  printf("\n");
  fflush(stdout);
  for (int i=0; i<C && i<16; ++i) {
    printf("%2d", i);
    for (int j=0; j<n; ++j)
      printf("%4d", wt[i][j]/10);
    printf("\n");
    fflush(stdout);
  } */
}

// A MultiMixer averages the output of 2 mixers using different contexts
class MultiMixer {
  enum { MINMEM = 5 }; // Lowest MEM to use 2 mixers and all weights
  Mixer m1, m2;

public:
  MultiMixer() : m1( 8 ), m2( 16 ) {}
  void write( int n0, int n1 ) {
    m1.write( n0, n1 );
    if( MEM >= MINMEM )
      m2.write( n0, n1 );
  }
  void add( int n0, int n1 ) {
    if( MEM >= MINMEM ) {
      m1.add( n0, n1 );
      m2.add( n0, n1 );
    } else
      m1.add( n0, n1 );
  }
  int predict() {
    U32 p1 = m1.predict( ch( 1 ) * m1.getC() / 256 );
    if( MEM >= MINMEM ) {
      U32 p2 = m2.predict( ( ch( 1 ) >> 6 ) + 4 * ( ch( 2 ) >> 6 ) );
      return ( p1 + p2 ) / 2;
    } else
      return p1;
  }
  void update( int y ) {
    m1.update( y );
    if( MEM >= MINMEM )
      m2.update( y );
  }
  U32 getC() const {
    return 256;
  }
  U32 getN() const {
    return m1.getN();
  }
};

MultiMixer mixer;

//////////////////////////// CounterMap ////////////////////////////

/* CounterMap maintains a model and one context

  Countermap cm(N); -- Create, size 2^N bytes
  cm.update(h);     -- Update model, then set next context hash to h
  cm.write();       -- Predict next bit and write counts to mixer
  cm.add();         -- Predict and add to previously written counts

There should be 8 calls to either write() or add() between each update(h).
h is a 32-bit hash of the context which should be set after a whole number
of bytes are read. */

// Stores only the most recent byte and its count per context
// in a hash table without collision detection
class CounterMap1 {
  const int N;
  struct S {
    U8 c; // char
    U8 n; // count
  };
  S *t; // cxt -> c repeated last n times
  U32 cxt;

public:
  CounterMap1( int n ) : N( n > 1 ? n - 1 : 1 ), cxt( 0 ) {
    assert( sizeof( S ) == 2 );
    t = ( S * ) calloc( 1 << N, 2 );
    if( t == nullptr )
      handler();
  }
  void update( U32 h ) {
    if( ch.bpos() == 0 ) {
      if( t[cxt].n == 0 ) {
        t[cxt].n = 1;
        t[cxt].c = ch( 1 );
      } else if( U32( t[cxt].c ) == ch( 1 ) ) {
        if( t[cxt].n < 255 )
          ++t[cxt].n;
      } else {
        t[cxt].c = ch( 1 );
        t[cxt].n = 1;
      }
    }
    cxt = h >> ( 32 - N );
  }
  void add() {
    if( ( U32 )( ( t[cxt].c + 256 ) >> ( 8 - ch.bpos() ) ) == ch() ) {
      if( ( ( t[cxt].c >> ( 7 - ch.bpos() ) ) & 1 ) != 0 )
        mixer.add( 0, t[cxt].n );
      else
        mixer.add( t[cxt].n, 0 );
    }
  }
  void write() {
    mixer.write( 0, 0 );
    add();
  }
};

// Uses a nibble-oriented hash table of contexts
class CounterMap2 {
private:
  const U32 N2;           // Size of ht2 in elements
  U32 cxt;                // Major context
  Hashtable<Counter> ht2; // Secondary hash table
  Counter *cp[8];         // Pointers into ht2 or 0 if not used
public:
  CounterMap2( int n ); // Use 2^n bytes memory
  void add();
  void update( U32 h );
  void write() {
    mixer.write( 0, 0 );
    add();
  }
};

CounterMap2::CounterMap2( int n ) : N2( n ), cxt( 0 ), ht2( N2 ) {
  for( int i = 0; i < 8; ++i )
    cp[i] = 0;
}

// Predict the next bit given the bits so far in ch()
void CounterMap2::add() {
  const U32 bcount = ch.bpos();
  if( bcount == 4 ) {
    cxt ^= hash( ch.hi(), cxt );
    ht2.set( cxt );
  }
  cp[bcount] = &ht2( ch.lo() );
  mixer.add( cp[bcount]->get0(), cp[bcount]->get1() );
}

// After 8 predictions, update the models with the last input char, ch(1),
// then set the new context hash to h
void CounterMap2::update( U32 h ) {
  const U32 c = ch( 1 );

  // Update the secondary context
  for( int i = 0; i < 8; ++i ) {
    if( cp[i] != nullptr ) {
      cp[i]->add( ( c >> ( 7 - i ) ) & 1 );
      cp[i] = 0;
    }
  }
  cxt = h;
  ht2.set( cxt );
}

// Combines 1 and 2 above.
class CounterMap3 {
  enum { MINMEM = 4 }; // Smallest MEM to use cm1
  CounterMap1 cm1;
  CounterMap2 cm2;

public:
  CounterMap3( int n ) : cm1( MEM >= MINMEM ? n - 2 : 0 ), cm2( n ) {}
  void update( U32 h ) {
    if( MEM >= MINMEM )
      cm1.update( h );
    cm2.update( h );
  }
  void write() {
    cm2.write();
    if( MEM >= MINMEM )
      cm1.add();
  }
  void add() {
    cm2.add();
    if( MEM >= MINMEM )
      cm1.add();
  }
};

#define CounterMap CounterMap3

//////////////////////////// Model ////////////////////////////

// All models have a function model() which updates the model with the
// last bit of input (in ch) then writes probabilities for the following
// bit into mixer.
class Model {
public:
  virtual void model() = 0;
  virtual ~Model() {}
};

//////////////////////////// defaultModel ////////////////////////////

// DefaultModel predicts P(1) = 0.5

class DefaultModel : public Model {
public:
  void model() {
    mixer.write( 1, 1 );
  }
};

//////////////////////////// charModel ////////////////////////////

// A CharModel contains n-gram models from 0 to 9

class CharModel : public Model {
  enum { N = 8 };                    // Number of models
  Counter *t0, *t1;                  // Model orders 0, 1 [256], [65536]
  CounterMap t2, t3, t4, t5, t6, t7; // Model orders 2-9
  U32 *cxt;                          // Context hashes [N]
  Counter *cp0, *cp1;                // Pointers to counters in t0, t1
public:
  CharModel() :
      t0( new Counter[256] ),
      t1( new Counter[65536] ),
      t2( MEM + 16 ),
      t3( MEM + 18 ),
      t4( MEM + 20 ),
      t5( static_cast<int>( MEM >= 1 ) * ( MEM + 20 ) ),
      t6( static_cast<int>( MEM >= 3 ) * ( MEM + 20 ) ),
      t7( static_cast<int>( MEM >= 3 ) * ( MEM + 20 ) ),

      cxt( new U32[N] ) {
    cp0 = &t0[0];
    cp1 = &t1[0];
    memset( cxt, 0, N * sizeof( U32 ) );
    memset( t0, 0, 256 * sizeof( Counter ) );
    memset( t1, 0, 65536 * sizeof( Counter ) );
  }
  void model(); // Update and predict
};

// Update with bit y, put array of 0 counts in n0 and 1 counts in n1
inline void CharModel::model() {
  // Update models
  int y = ch( static_cast<int>( ch.bpos() == 0 ) ) & 1; // last input bit
  cp0->add( y );
  cp1->add( y );

  // Update context
  if( ch.bpos() == 0 ) { // Start new byte
    for( int i = N - 1; i > 0; --i )
      cxt[i] = cxt[i - 1] ^ hash( ch( 1 ), i );
    t2.update( cxt[2] );
    t3.update( cxt[3] );
    t4.update( cxt[4] );
    if( MEM >= 1 )
      t5.update( cxt[5] );
    if( MEM >= 3 ) {
      t6.update( cxt[6] );
      t7.update( cxt[7] );
    }
  }
  cp0 = &t0[ch()];
  cp1 = &t1[ch() + 256 * ch( 1 )];

  // Write predictions to the mixer
  mixer.write( cp0->get0(), cp0->get1() );
  mixer.write( cp1->get0(), cp1->get1() );
  t2.write();
  t3.add();
  t4.write();
  if( MEM >= 1 )
    t5.add();
  if( MEM >= 3 ) {
    t6.write();
    t7.add();
  }
}

//////////////////////////// matchModel ////////////////////////////

/* A MatchModel looks for a match of length n >= 8 bytes between
the current context and previous input, and predicts the next bit
in the previous context with weight n.  If the next bit is 1, then
the mixer is assigned (0, n), else (n, 0).  Matchies are found using
an index (a hash table of pointers into ch). */

class MatchModel : public Model {
  enum { M = 4 }; // Number of strings to match
  U32 hash[2];    // Hashes of current context up to pos-1
  U32 begin[M];   // Points to first matching byte
  U32 end[M];     // Points to last matching byte + 1, 0 if no match
  U32 *ptr;       // Hash table of pointers [2^(MEM+17)]
public:
  MatchModel() : ptr( new U32[1 << ( 17 + MEM )] ) {
    memset( ptr, 0, ( 1 << ( 17 + MEM ) ) * sizeof( U32 ) );
    hash[0] = hash[1] = 0;
    for( int i = 0; i < M; ++i )
      begin[i] = end[i] = 0;
  }
  void model();
};

inline void MatchModel::model() {
  if( ch.bpos() == 0 ) { // New byte
    hash[0] = hash[0] * ( 16 * 56797157 ) + ch( 1 ) + 1;
    // Hash of last 8 bytes if MEM>=3 or else last 5 bytes
    hash[1] = hash[1] * ( 2 * 45684217 ) + ch( 1 ) + 1; // Hash last 32 bytes
    U32 h = hash[0] >> ( 15 - MEM );
    if( ( hash[0] >> 28 ) == 0 )
      h = hash[1] >> ( 15 - MEM ); // 1/16 of 8-contexts are hashed to 32 bytes
    for( int i = 0; i < M; ++i ) {
      if( ( end[i] != 0U ) && ch( 1 ) == ch[end[i]] )
        ++end[i];
    }
    for( int i = 0; i < M; ++i ) {
      if( end[i] == 0U ) { // Search for a matching context
        int j;
        for( j = 0; j < M; ++j ) // Search for duplicate match
          if( ptr[h] == end[j] )
            break;
        if( j != M ) // Context already matched?
          break;
        end[i] = ptr[h];
        if( end[i] > 0 ) {
          begin[i] = end[i];
          U32 p = ch.pos();
          while( begin[i] > 0 && p > 0 && begin[i] != p + 1 && ch[begin[i] - 1] == ch[p - 1] ) {
            --begin[i];
            --p;
          }
        }
        if( end[i] == begin[i] ) // No match found
          begin[i] = end[i] = 0;
        break;
      }
    }
    ptr[h] = ch.pos();
  }

  // Test whether the current context is valid in the last 0-7 bits
  for( int i = 0; i < M; ++i ) {
    if( ( end[i] != 0U ) && ( ( ch[end[i]] + 256 ) >> ( 8 - ch.bpos() ) ) != ch() )
      begin[i] = end[i] = 0;
  }

  // Predict the bit found in the matching contexts
  int n0 = 0, n1 = 0;
  for( int i = 0; i < M; ++i ) {
    if( end[i] != 0U ) {
      U32 wt = ( end[i] - begin[i] );
      wt = wt * wt / 4;
      if( wt > 511 )
        wt = 511;
      int y = ( ch[end[i]] >> ( 7 - ch.bpos() ) ) & 1;
      if( y != 0 )
        n1 += wt;
      else
        n0 += wt;
    }
  }
  mixer.write( n0, n1 );
}

//////////////////////////// recordModel ////////////////////////////

/* A RecordModel finds fixed length records and models bits in the context
of the two bytes above (the same position in the two previous records)
and in the context of the byte above and to the left (the previous byte).
The record length is assumed to be the interval in the most recent
occurrence of a byte occuring 4 times in a row equally spaced, e.g.
"x..x..x..x" would imply a record size of 3.  There are models for
the 2 most recent, different record lengths of at least 2. */

class RecordModel : public Model {
  const int SIZE;
  enum { N = 5 };                // Number of models
  CounterMap t0, t1, t2, t3, t4; // Model
  int repeat1{ 2 }, repeat2{ 3 };          // 2 last cycle lengths
public:
  RecordModel() :
      SIZE( static_cast<int>( MEM >= 4 ) * ( 17 + MEM ) ),
      t0( SIZE ),
      t1( SIZE ),
      t2( SIZE ),
      t3( SIZE ),
      t4( SIZE )
      {}
  void model();
};

// Update the model with bit y, then put predictions of the next update
// as 0 counts in n0[0..N-1] and 1 counts in n1[0..N-1]
inline void RecordModel::model() {
  if( ch.bpos() == 0 ) {
    // Check for a repeating pattern of interval 3 or more
    const int c = ch( 1 );
    const int d1 = ch.pos( c, 0 ) - ch.pos( c, 1 );
    const int d2 = ch.pos( c, 1 ) - ch.pos( c, 2 );
    const int d3 = ch.pos( c, 2 ) - ch.pos( c, 3 );
    if( d1 > 1 && d1 == d2 && d2 == d3 ) {
      if( d1 == repeat1 )
        swap( repeat1, repeat2 );
      else if( d1 != repeat2 ) {
        repeat1 = repeat2;
        repeat2 = d1;
      }
    }

    // Compute context hashes
    int r1 = repeat1, r2 = repeat2;
    if( r1 > r2 )
      swap( r1, r2 );
    t0.update( hash( ch( r1 ), ch( r1 * 2 ), r1 ) ); // 2 above (shorter repeat)
    t1.update( hash( ch( 1 ), ch( r1 ), r1 ) );      // above and left
    t2.update( hash( ch( r1 ), ch.pos() % r1 ) );    // above and pos
    t3.update( hash( ch( r2 ), ch( r2 * 2 ), r2 ) ); // 2 above (longer repeat)
    t4.update( hash( ch( 1 ), ch( r2 ), r2 ) );      // above and left
  }
  t0.write();
  t1.write();
  t2.write();
  t3.write();
  t4.write();
}

//////////////////////////// sparseModel //////////////////////////// EMILCONT ALL

// A SparseModel models several order-2 contexts with gaps

class SparseModel : public Model {
  const int SIZE;
  enum { N = 10 };                                   // Number of models
  CounterMap t0, t1, t2, t3, t4, t5, t6, t7, t8, t9; // Sparse models
public:
  SparseModel() :
      SIZE( static_cast<int>( MEM >= 5 ) * ( MEM + 17 ) ),
      t0( SIZE ),
      t1( SIZE ),
      t2( SIZE ),
      t3( SIZE ),
      t4( SIZE ),
      t5( SIZE ),
      t6( SIZE ),
      t7( SIZE ),
      t8( SIZE ),
      t9( SIZE ) {}
  void model(); // Update and predict
};

inline void SparseModel::model() {
  // Update context
  if( ch.bpos() == 0 ) {
    t0.update( hash( ch( 1 ), ch( 3 ) ) );
    t1.update( hash( ch( 1 ), ch( 4 ) ) );
    t2.update( hash( ch( 2 ), ch( 4 ) ) );
    t3.update( hash( ch( 4 ), ch( 8 ) ) );
    t4.update( hash( ch( 2 ), ch( 3 ) ) );
    t5.update( hash( ch( 3 ), ch( 4 ) ) );
    t6.update( hash( ch( 1 ), ch( 2 ) ) );
    t7.update( hash( ch( 1 ), ch( 5 ) ) );
    t8.update( hash( ch( 1 ), ch( 6 ) ) );
    t9.update( hash( ch( 1 ), ch( 7 ) ) );
  }

  // Predict
  t0.write();
  t1.add();
  t2.write();
  t3.add();
  t4.write();
  t5.add();
  t6.write();
  t7.add();
  t8.write();
  t9.add();
}

//////////////////////////// analogModel ////////////////////////////

// An AnalogModel is intended for 16-bit mono or stereo (WAV files)
// 24-bit images (BMP files), and 8 bit analog data (such as grayscale
// images), and CCITT images.

class AnalogModel : public Model {
  const int SIZE;
  enum { N = 7 };
  CounterMap t0, t1, t2, t3, t4, t5, t6;
  int pos3{ 0 }; // pos % 3
public:
  AnalogModel() :
      SIZE( static_cast<int>( MEM >= 5 ) * ( MEM + 17 ) ),
      t0( SIZE ),
      t1( SIZE ),
      t2( SIZE ),
      t3( SIZE ),
      t4( SIZE ),
      t5( SIZE ),
      t6( SIZE )
      {}
  void model() {
    if( ch.bpos() == 0 ) {
      if( ++pos3 == 3 )
        pos3 = 0;
      t0.update( hash( ch( 2 ) / 4, ch( 4 ) / 4, ch.pos() % 2 ) ); // 16 bit mono model
      t1.update( hash( ch( 2 ) / 16, ch( 4 ) / 16, ch.pos() % 2 ) );
      t2.update( hash( ch( 2 ) / 4, ch( 4 ) / 4, ch( 8 ) / 4, ch.pos() % 4 ) ); // Stereo
      t3.update( hash( ch( 3 ), ch( 6 ) / 4, pos3 ) );                          // 24 bit image models
      t4.update( hash( ch( 1 ) / 16, ch( 2 ) / 16, ch( 3 ) / 4, pos3 ) );
      t5.update( hash( ch( 1 ) / 2, ch( 2 ) / 8, ch( 3 ) / 32 ) ); // 8-bit data model
      t6.update( hash( ch( 216 ), ch( 432 ) ) );                   // CCITT images
    }
    t0.write();
    t1.add();
    t2.add();
    t3.write();
    t4.add();
    t5.write();
    t6.write();
  }
};

//////////////////////////// wordModel ////////////////////////////

// A WordModel models words, which are any characters > 32 separated
// by whitespace ( <= 32).  There is a unigram, bigram and sparse
// bigram model (skipping 1 word).

class WordModel : public Model {
  const int SIZE;
  enum { N = 6 };
  CounterMap t0, t1, t2, t3, t4, t5;
  U32 cxt[N];  // Hashes of last N words broken on whitespace
  U32 word[N]; // Hashes of last N words of letters only
public:
  WordModel() :
      SIZE( static_cast<int>( MEM >= 4 ) * ( MEM + 18 ) ),
      t0( 24 ),
      t1( SIZE ),
      t2( SIZE ),
      t3( SIZE ),
      t4( SIZE ),
      t5( SIZE ) {
    for( int i = 0; i < N; ++i )
      cxt[i] = word[i] = 0;
  }
  void model() {
    if( ch.bpos() == 0 ) {
      int c = ch( 1 );
      if( c > 32 ) {
        cxt[0] ^= hash( cxt[0], c );
      } else if( cxt[0] != 0U ) {
        for( int i = N - 1; i > 0; --i )
          cxt[i] = cxt[i - 1];
        cxt[0] = 0;
      }
      if( isalpha( c ) != 0 )
        word[0] ^= hash( word[0], tolower( c ), 1 );
      else {
        for( int i = N - 1; i > 0; --i )
          word[i] = word[i - 1];
        word[0] = 0;
      }
      t0.update( cxt[0] );
      t1.update( cxt[1] + cxt[0] );
      t2.update( cxt[2] + cxt[0] );
      t3.update( word[0] );
      t4.update( word[1] + word[0] );
      t5.update( word[2] + word[0] );
    }
    t0.write();
    t1.add();
    t2.add();
    t3.write();
    t4.add();
    t5.add();
  }
};

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

class Predictor {
  // Models
  DefaultModel defaultModel;
  CharModel charModel;
  MatchModel matchModel;
  RecordModel recordModel;
  SparseModel sparseModel;
  AnalogModel analogModel;
  WordModel wordModel;

  enum {
    SSE1 = 256 * 4 * 2,
    SSE2 = 32, // SSE dimensions (contexts, probability bins)
    SSESCALE = 1024 / SSE2
  }; // Number of mapped probabilities between bins

  // Scale probability p into a context in the range 0 to 1K-1 by
  // stretching the ends of the range.
  class SSEMap {
    U16 table[PSCALE];

  public:
    int operator()( int p ) const {
      return table[p];
    }
    SSEMap();
  } ssemap; // functoid

  // Secondary source encoder element
  struct SSEContext {
    U8 c1{ 0 }, n{ 0 }; // Count of 1's, count of bits
    int p() const {
      return PSCALE * ( c1 * 64 + 1 ) / ( n * 64 + 2 );
    }
    void update( int y ) {
      if( y != 0 )
        ++c1;
      if( ++n > 254 ) { // Roll over count overflows
        c1 /= 2;
        n /= 2;
      }
    }
    SSEContext()  {}
  };

  SSEContext ( *sse )[SSE2 + 1]{ 0 }; // [SSE1][SSE2+1] context, mapped probability
  U32 nextp;                     // p()
  U32 ssep{ 512 };                      // Output of sse
  U32 context{ 0 };                   // SSE context
public:
  Predictor();
  int p() const {
    return nextp;
  }                     // Returns pr(y = 1) * PSCALE
  void update( int y ); // Update model with bit y = 0 or 1
};

Predictor::SSEMap::SSEMap() {
  for( int i = 0; i < PSCALE; ++i ) {
    int p = int( 64 * log( ( i + 0.5 ) / ( PSCALE - 0.5 - i ) ) + 512 );
    if( p > 1023 )
      p = 1023;
    if( p < 0 )
      p = 0;
    table[i] = p;
  }
}

Predictor::Predictor() :  nextp( PSCALE / 2 ) {
  ch.init();

  // Initialize to sse[context][ssemap(p)] = p
  if( MEM >= 1 ) {
    sse = ( SSEContext( * )[SSE2 + 1] ) new SSEContext[SSE1][SSE2 + 1];
    int N = PSCALE;
    int oldp = SSE2 + 1;
    for( int i = N - 1; i >= 0; --i ) {
      int p = ( ssemap( i * PSCALE / N ) + SSESCALE / 2 ) / SSESCALE;
      int n = 1 + N * N / ( ( i + 1 ) * ( N - i ) );
      if( n > 254 )
        n = 254;
      int c1 = ( i * n + N / 2 ) / N;
      for( int j = oldp - 1; j >= p; --j ) {
        for( int k = 0; k < SSE1; ++k ) {
          sse[k][j].n = n;
          sse[k][j].c1 = c1;
        }
      }
      oldp = p;
    }
  }
}

inline void Predictor::update( int y ) {
  // Update the bins below and above the last input probability, ssep
  if( MEM >= 1 ) {
    sse[context][ssep / SSESCALE].update( y );
    sse[context][ssep / SSESCALE + 1].update( y );
  }

  // Adjust model mixing weights
  mixer.update( y );

  // Update individual models
  ch.update( y );
  defaultModel.model();
  charModel.model();
  if( MEM >= 2 )
    matchModel.model();
  if( MEM >= 5 ) {
    sparseModel.model();
    analogModel.model();
  }
  if( MEM >= 4 ) {
    recordModel.model();
    wordModel.model();
  }

  // Combine probabilities
  nextp = mixer.predict();

  // Get final probability, interpolate SSE and average with original
  if( MEM >= 1 ) {
    context =
        ( ch( 0 ) * 4 + ch( 1 ) / 64 ) * 2 + static_cast<unsigned int>( ch.pos( 0, 3 ) < ch.pos( 32, 3 ) ); // for SSE
    ssep = ssemap( nextp );
    U32 wt = ssep % SSESCALE;
    U32 i = ssep / SSESCALE;
    nextp =
        ( ( ( sse[context][i].p() * ( SSESCALE - wt ) + sse[context][i + 1].p() * wt ) / SSESCALE ) * 3 + nextp ) / 4;
  }
}

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

typedef enum { COMPRESS, DECOMPRESS } Mode;
class Encoder {
private:
  Predictor predictor;
  const Mode mode; // Compress or decompress?
  FILE *archive;   // Compressed data file
  U32 x1, x2;      // Range, initially [0, 1), scaled by 2^32
  U32 x;           // Last 4 input bytes of archive.
public:
  Encoder( Mode m, FILE *f );
  void encode( int y ); // Compress bit y
  int decode();         // Uncompress and return bit y
  void flush();         // Call when done compressing
};

// Constructor
Encoder::Encoder( Mode m, FILE *f ) : predictor(), mode( m ), archive( f ), x1( 0 ), x2( 0xffffffff ), x( 0 ) {
  // In DECOMPRESS mode, initialize x to the first 4 bytes of the archive
  if( mode == DECOMPRESS ) {
    for( int i = 0; i < 4; ++i ) {
      int c = getc( archive );
      x = ( x << 8 ) + ( c & 0xff );
    }
  }
}

/* encode(y) -- Encode bit y by splitting the range [x1, x2] in proportion
to P(1) and P(0) as given by the predictor and narrowing to the appropriate
subrange.  Output leading bytes of the range as they become known. */

inline void Encoder::encode( int y ) {
  // Split the range
  const U32 p = predictor.p() * ( 4096 / PSCALE ) + 2048 / PSCALE; // P(1) * 4K
  assert( p < 4096 );
  const U32 xdiff = x2 - x1;
  U32 xmid = x1; // = x1+p*(x2-x1) multiply without overflow, round down
  if( xdiff >= 0x4000000 )
    xmid += ( xdiff >> 12 ) * p;
  else if( xdiff >= 0x100000 )
    xmid += ( ( xdiff >> 6 ) * p ) >> 6;
  else
    xmid += ( xdiff * p ) >> 12;

  // Update the range
  if( y != 0 )
    x2 = xmid;
  else
    x1 = xmid + 1;
  predictor.update( y );

  // Shift equal MSB's out
  while( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 ) {
    putc( x2 >> 24, archive );
    x1 <<= 8;
    x2 = ( x2 << 8 ) + 255;
  }
}

/* Decode one bit from the archive, splitting [x1, x2] as in the encoder
and returning 1 or 0 depending on which subrange the archive point x is in.
*/
inline int Encoder::decode() {
  // Split the range
  const U32 p = predictor.p() * ( 4096 / PSCALE ) + 2048 / PSCALE; // P(1) * 4K
  assert( p < 4096 );
  const U32 xdiff = x2 - x1;
  U32 xmid = x1; // = x1+p*(x2-x1) multiply without overflow, round down
  if( xdiff >= 0x4000000 )
    xmid += ( xdiff >> 12 ) * p;
  else if( xdiff >= 0x100000 )
    xmid += ( ( xdiff >> 6 ) * p ) >> 6;
  else
    xmid += ( xdiff * p ) >> 12;

  // Update the range
  int y = 0;
  if( x <= xmid ) {
    y = 1;
    x2 = xmid;
  } else
    x1 = xmid + 1;
  predictor.update( y );

  // Shift equal MSB's out
  while( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 ) {
    x1 <<= 8;
    x2 = ( x2 << 8 ) + 255;
    int c = getc( archive );
    if( c == EOF )
      c = 0;
    x = ( x << 8 ) + c;
  }
  return y;
}

// Should be called when there is no more to compress
void Encoder::flush() {
  // In COMPRESS mode, write out the remaining bytes of x, x1 < x < x2
  if( mode == COMPRESS ) {
    while( ( ( x1 ^ x2 ) & 0xff000000 ) == 0 ) {
      putc( x2 >> 24, archive );
      x1 <<= 8;
      x2 = ( x2 << 8 ) + 255;
    }
    putc( x2 >> 24, archive ); // First unequal byte
  }
}

//////////////////////////// Transformer ////////////////////////////

/* A transformer filters input data into a form easier to compress
   and then encodes it.

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
  enum { DATA, TEXT, EXE } filetype;
  void compress( int c ) { // Compress transformed byte c
    for( int i = 7; i >= 0; --i )
      e.encode( ( c >> i ) & 1 );
  }
  int decompress() { // Uncompress a transformed byte and return it
    int c = 0;
    for( int i = 0; i < 8; ++i )
      c = c + c + e.decode();
    return c;
  }

public:
  Transformer( Mode mode, FILE *f ) : e( mode, f ), filetype( DATA ) {}
  void encode( int c ) {
    if( filetype == TEXT ) {
      // Convert uppercase to U + lowercase
      if( isupper( c ) != 0 ) {
        compress( 'U' );
        c = tolower( c );
      }

      // Transform LF to LF SPACE
      compress( c );
      if( c == '\n' )
        compress( ' ' );
    } else {
      compress( c );
    }
  }
  int decode() {
    int c = decompress();
    if( filetype == TEXT ) {
      if( c == '\n' )
        decompress(); // discard space
      else if( c == 'U' ) {
        c = toupper( decompress() );
      }
    }
    return c;
  }
  void flush() {
    e.flush();
  }
};

//////////////////////////// main ////////////////////////////

// Read and return a line of input from FILE f (default stdin) up to
// first control character except tab.  Skips CR in CR LF.
string getline( FILE *f = stdin ) {
  int c;
  string result = "";
  while( ( c = getc( f ) ) != EOF && ( c >= 32 || c == '\t' ) )
    result += char( c );
  if( c == '\r' )
    ( void ) getc( f );
  return result;
}

// User interface
int main( int argc, char **argv ) {
  // Check arguments
  if( argc < 2 ) {
    printf( PROGNAME " file compressor/archiver, (C) 2003, Matt Mahoney, mmahoney@cs.fit.edu\n"
                     "This program is free software distributed without warranty under the terms\n"
                     "of the GNU General Public License, see http://www.gnu.org/licenses/gpl.txt\n"
                     "\n"
                     "To compress:         " PROGNAME " -5 archive filenames...  (archive will be created)\n"
                     "  or (MSDOS):        dir/b | " PROGNAME " -5 archive  (reads file names from input)\n"
                     "To extract/compare:  " PROGNAME " archive  (does not clobber existing files)\n"
                     "To view contents:    more < archive\n"
                     "\n"
                     "Compression option: -5 may be -0 through -9.  Higher numbers compress\n"
                     "better but slower and each increment doubles memory used.  Decompression\n"
                     "requires just as much memory.  Default is -5 (186 MB)\n" );
    return 1;
  }

  // Read and remove -m option
  if( argc > 1 && argv[1][0] == '-' ) {
    if( isdigit( argv[1][1] ) != 0 ) {
      MEM = argv[1][1] - '0';
      printf( "Memory usage option -%d\n", MEM );
    } else
      printf( "Option %s ignored\n", argv[1] );
    argc--;
    argv++;
  }

  // File names and sizes from input or archive
  vector<string> filename;                          // List of names
  vector<long> filesize;                            // Size or -1 if error
  int uncompressed_bytes = 0, compressed_bytes = 0; // Input, output sizes

  // Extract files
  FILE *archive = fopen( argv[1], "rbe" );
  if( archive != nullptr ) {
    if( argc > 2 ) {
      printf( "File %s already exists\n", argv[1] );
      return 1;
    }

    // Read PROGNAME " -m\r\n" at start of archive
    string s = getline( archive );
    if( s.substr( 0, string( PROGNAME ).size() ) != PROGNAME ) {
      printf( "Archive file %s not in " PROGNAME " format\n", argv[1] );
      return 1;
    }

    // Get option -m where m is a digit
    if( s.size() > 2 && s[s.size() - 2] == '-' ) {
      int c = s[s.size() - 1];
      if( c >= '0' && c <= '9' )
        MEM = c - '0';
    }
    printf( "Extracting archive " PROGNAME " -%d %s ...\n", MEM, argv[1] );

    // Read "size filename" in "%d\t%s\r\n" format
    while( true ) {
      string s = getline( archive );
      if( s.size() > 1 ) {
        filesize.push_back( atol( s.c_str() ) );
        string::iterator tab = find( s.begin(), s.end(), '\t' );
        if( tab != s.end() )
          filename.emplace_back( tab + 1, s.end() );
        else
          filename.emplace_back("" );
      } else
        break;
    }

    // Test end of header for "\f\0"
    {
      int c1 = 0, c2 = 0;
      if( ( c1 = getc( archive ) ) != '\f' || ( c2 = getc( archive ) ) != 0 ) {
        printf( "%s: Bad " PROGNAME " header format %d %d\n", argv[1], c1, c2 );
        return 1;
      }
    }

    // Extract files from archive data
    Transformer e( DECOMPRESS, archive );
    for( int i = 0; i < int( filename.size() ); ++i ) {
      printf( "%10ld %s: ", filesize[i], filename[i].c_str() );

      // Compare with existing file
      FILE *f = fopen( filename[i].c_str(), "rbe" );
      const long size = filesize[i];
      uncompressed_bytes += size;
      if( f != nullptr ) {
        bool different = false;
        for( long j = 0; j < size; ++j ) {
          int c1 = e.decode();
          int c2 = getc( f );
          if( !different && c1 != c2 ) {
            printf( "differ at offset %ld, archive=%d file=%d\n", j, c1, c2 );
            different = true;
          }
        }
        if( !different )
          printf( "identical\n" );
        fclose( f );
      }

      // Extract to new file
      else {
        f = fopen( filename[i].c_str(), "wbe" );
        if( f == nullptr )
          printf( "cannot create, skipping...\n" );
        for( long j = 0; j < size; ++j ) {
          int c = e.decode();
          if( f != nullptr )
            putc( c, f );
        }
        if( f != nullptr ) {
          printf( "extracted\n" );
          fclose( f );
        }
      }
    }
    compressed_bytes = ftell( archive );
    fclose( archive );
  }

  // Compress files
  else {
    // Read file names from command line or input
    if( argc > 2 )
      for( int i = 2; i < argc; ++i )
        filename.emplace_back(argv[i] );
    else {
      printf( "Enter names of files to compress, followed by blank line or EOF.\n" );
      while( true ) {
        string s = getline( stdin );
        if( s == "" )
          break;
        else
          filename.push_back( s );
      }
    }

    // Get file sizes
    for( int i = 0; i < int( filename.size() ); ++i ) {
      FILE *f = fopen( filename[i].c_str(), "rbe" );
      if( f == nullptr ) {
        printf( "File not found, skipping: %s\n", filename[i].c_str() );
        filesize.push_back( -1 );
      } else {
        fseek( f, 0L, SEEK_END );
        filesize.push_back( ftell( f ) );
        fclose( f );
      }
    }
    if( filesize.empty() || *max_element( filesize.begin(), filesize.end() ) < 0 ) {
      printf( "No files to compress, no archive created.\n" );
      return 1;
    }

    // Write header
    archive = fopen( argv[1], "wbe" );
    if( archive == nullptr ) {
      printf( "Cannot create archive: %s\n", argv[1] );
      return 1;
    }
    fprintf( archive, PROGNAME );
    if( MEM != 5 )
      fprintf( archive, " -%d", MEM );
    fprintf( archive, "\r\n" );
    for( int i = 0; i < int( filename.size() ); ++i ) {
      if( filesize[i] >= 0 )
        fprintf( archive, "%ld\t%s\r\n", filesize[i], filename[i].c_str() );
    }
    putc( 032, archive ); // MSDOS EOF
    putc( '\f', archive );
    putc( 0, archive );

    // Write data
    Transformer e( COMPRESS, archive );
    long file_start = ftell( archive );
    for( int i = 0; i < int( filename.size() ); ++i ) {
      const long size = filesize[i];
      if( size >= 0 ) {
        uncompressed_bytes += size;
        printf( "%-23s %10ld -> ", filename[i].c_str(), size );
        FILE *f = fopen( filename[i].c_str(), "rbe" );
        int c;
        for( long j = 0; j < size; ++j ) {
          if( f != nullptr )
            c = getc( f );
          else
            c = 0;
          e.encode( c );
        }
        if( f != nullptr )
          fclose( f );
        printf( "%ld\n", ftell( archive ) - file_start );
        file_start = ftell( archive );
      }
    }
    e.flush();
    compressed_bytes = ftell( archive );
    fclose( archive );
  }

  // Report statistics
  const double elapsed_time = double( clock() - programChecker.start_time() ) / CLOCKS_PER_SEC;
  printf( "%d/%d in %1.2f sec.", compressed_bytes, uncompressed_bytes, elapsed_time );
  if( uncompressed_bytes > 0 && elapsed_time > 0 ) {
    printf( " (%1.4f bpc, %1.2f%% at %1.0f KB/s)", compressed_bytes * 8.0 / uncompressed_bytes,
            compressed_bytes * 100.0 / uncompressed_bytes, uncompressed_bytes / ( elapsed_time * 1000.0 ) );
  }
  printf( "\n" );
  return 0;
}
