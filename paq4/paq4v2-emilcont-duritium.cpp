/* PAQ4 v2 - File archiver and compressor.
(C) 2003, Matt Mahoney, mmahoney@cs.fit.edu

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation at
http://www.gnu.org/licenses/gpl.txt or (at your option) any later version.
This program is distributed without any warranty.


USAGE

To compress: PAQ4 archive file file... (1 or more file names), or
  or (MSDOS): dir/b | PAQ4 archive (read file names from input)
  or (UNIX): ls | PAQ4 archive
To decompress: PAQ4 archive
To list contents: more < archive

Compression: The files listed are compressed and stored in the archive,
which is created. The archive must not already exist. File names may
specify a path, which is stored. If there are no file names on the command
line, then PAQ4 prompts for them, reading until the first blank line or
end of file.

Decompression: No file names are specified. The archive must exist.
If a path is stored, the file is extracted to the appropriate directory,
which must exist. PAQ4 does not create directories. If the file to be
extracted already exists, it is not replaced; rather it is compared with
the archived file, and the offset of the first difference is reported.

It is not possible to add, remove, or update files in an existing archive.
If you want to do this, extract the files, delete the archive, and
create a new archive with just the files you want.


TO COMPILE

gxx -O paq4.cpp DJGPP 2.95.2 (gives fastest executable)
bcc32 -O2 paq4.cpp Borland 5.5.1
sc -o paq4.cpp Digital Mars 8.35n


FILE FORMAT

A PAQ4 archive has the following format. The first 6 bytes are
"PAQ4\r\n" followed by a list of file sizes and names in
"%ld\t%s\r\n" format, e.g. 768771<TAB>book1<CR><LF>. The last filename
is followed by "\x1A\f\0" (<MSDOS-EOF><FORMFEED><NUL>) and the
compressed data.

The compressor uses predictive arithmetic coding. The compressed data
is a base 256 represenation of a number x (e.g. the first byte has
weight 1/256, second is 1/65536, etc.) such that x has as few digits as
possible and satisfies P(s > y) <= x < P(s >= y), where y is the concatenated
files, s is a random string with probability distribution P, and (s > y)
means that s follows y in a lexicographical ordering over strings.
As y is read in, the bounds of possible x narrow, allowing the leading
digits to be written as they become known. Computation of x
is approximated using integer arithmetic, which expands the
data less than 0.001% over the Shannon limit of log2 1/P(y) bits.

Prediction (estimation of P(y)) is independent of coding. The model
by which P is estimated is built from the previous uncompressed data
regardless of whether it is being input or output, so the model
is identical for compression and decompression. The model consists
of an adaptive weighting of submodels, followed by context sensitive
probability adjustment using SSE (secondary symbol estimation). It
differs from PAQ3 and PAQ3N that the weighting of models is adaptive
rather than fixed. The cyclic model for fixed length records has also
been improved.

There are N = 19 submodels, which consist of counts of 0 and 1 bits that
appear in different contexts. These are combined to a probability P(1)
that the next bit of input will be a 1 as follows:

  P(1) = SUM(i=1..N) wi (n1i / ni)

where wi is the i'th weight (determined adaptively), n1i is the number
of 1 bits observed in the i'th context, and ni = n0i + n1i is the
total number of times the i'th context was observed.

The weights wi are adjusted after each bit of uncompressed data to
reduce the cost (code length) of that bit. The adjustment is along
the gradient of the cost in weight space, which is

  wi := wi + e[nyi/(SUM(j) (wj+wo) nyj) - ni/(SUM(j) (wj+wo) nj)]

where e and wo are small constants, and nyi is the count for bit y (0 or 1)
in the i'th context. The weight offset wo prevents the gradient from
going to infinity as the weights go to 0.

The weight vector is context sensitive. There are 8 vectors which
are trained and used depending on the context of the 3 high order
bits of the previous whole byte.

In order to favor recent data over older data, counts are discarded
in a way that gives higher weights to probabilities near 0 or 1.
After bit y is observed, the count ny is incremented, but half of the
opposite count over 2 is discarded. For example, if n0=3, n1=10, then
after a 0 is observed, then half of the 8 bits over 2 in n1 are discarded,
so n0=4, n1=6.

The 19 submodels are as follows. The current 0-7 bits of the partially
compressed byte is always included in the context except in the fixed
model.
- Fixed (n0 = n1 = 1)
- Order 0. The context is the 0-7 bits of the current byte.
- Order 1. The context is the last byte plus the current 0-7 bits.
- Order 2. The context is the last 2 bytes plus the current 0-7 bits.
- Order 3.
- Order 4.
- Order 5.
- Order 6.
- Order 7.
- Word. The context is the last whole word, defined as the last 0 to 8
  contiguous bytes with values of 33 or more, plus the current bits.
- Sparse order 2, skipping the next to last byte (x.x)
- Sparse order 2, skipping 2 bytes (x..x), the 4th to last and last bytes.
- Sparse order 2, the 2nd and 4th last bytes (x.x.)
- Sparse order 2, the 4th and 8th last bytes (x...x...)
- Sparse order 2, the 2nd and 3rd last (xx.)
- Sparse order 2, the 3rd and 4th last (xx..)
- A long string model consisting of the last match of 8 or more bytes.
- A fixed record model consisting of the two bytes above (as a table)
- A fixed record model consisting of the byte above and to the left.

Counts for the order 2-7 and word models share a 32M element hash table,
where both counts are represented by an 8 bit state. Hash collisions
are detected (usually) by using an 8 bit checksum of the context,
with linear search over 3 buckets. If all 3 are full, then the element
with the lowest priority is replaced, where priority = n0 + n1.
The order 0 and 1 models use unhashed tables. The sparse order 2 models
share a 4M hash table. The two fixed record models share a 2M hash table

The long string model uses a 1M hash table to index a 4MB rotating buffer
with 8 and 32 bute hashes to find the most recent context match of
8 bytes or more. The count is set to nu = the length of the
match, where y is the bit that occured in that context, and the other
count to 0. For example, given "abcdefghij1...abcdefghij" then
n0=0, n1=10.

The fixed record model determines the record size (table width) by
searching for 4 consecutive appearances of the same character equally
spaced with an interval of at least 3. For instance, if the
string "a..a..a..a" is observed, then the record length is set to 3.
It remains set until another sequence is observed.

The probability P(1) calculated by the weighted sum of models is
further adjusted by SSE in the context of the current
0-7 bits of the partially compressed byte and the 2 high order bits of
the previous byte (1024 contexts). In each SSE context, P(1) is divided
into 32 bins, and the actual counts of 0s and 1s observed in each bin
are used to compute the new probability. When P(1) falls between bins,
the counts in the bins on both sides are incremented, and the output
probability is interpolated between the bins. The bin spacing is
closer together for output probabilities near 0 or 1. The counts are
not discounted to favor recent data as in the 19 submodels. Instead,
when the counters (8 bits) overflow, both n0 and n1 are halved. The
SSE mapping is initialized to SSE(P) = P. The final probability Pf
is then averaged to Pf = (3 SSE(P) + P) / 4.

Thanks to Serge Osnach for introducing me to SSE (in PAQ1SSE/PAQ2) and
the sparse models (PAQ3N). Also, credit to Eugene Shelwein,
Dmitry Shkarin for suggestions on using multiple character SSE contexts.
Credit to Eugene, Serge, and Jason Schmidt for developing faster and
smaller executables of previous versions. Credit to Werner Bergmans
and Berto Destasio for testing and evaluating them, including modifications
that improve compression at the cost of more memory.

I expect there will be better versions in the future. If you make any
changes, please change the name of the program (e.g. PAQ5), including
the string in the archive header by redefining PROGNAME below.
This will prevent any confusion about versions or archive compatibility.
Also, give yourself credit in the help message.
*/

#define PROGNAME "PAQ4" // Please change this if you change the program

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

using std::max;
using std::min;
using std::set_new_handler;
using std::string;
using std::vector;

const int PSCALE = 2048; // Integer scale for representing probabilities
const int MEM = 8;       // 6 = 88 MB, increase by 1 to double it

template <class T>
inline int size( const T &t ) {
  return t.size();
}

// 8-32 bit unsigned types, adjust as appropriate
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

class U24 {      // 24-bit unsigned int
  U8 b0, b1, b2; // Low, mid, high byte
public:
  explicit U24( int x = 0 ) : b0( x ), b1( x >> 8 ), b2( x >> 16 ) {}
  operator int() const {
    return ( ( ( b2 << 8 ) | b1 ) << 8 ) | b0;
  }
};

// 32-bit random number generator based on r(i) = r(i-24) ^ r(i-55)
class Random {
  U32 table[55]; // Last 55 random values
  int i;         // Index of current random value in table
public:
  Random();
  U32 operator()() { // Return 32-bit random number
    if( ++i == 55 )
      i = 0;
    if( i >= 64 )
      return table[i] ^= table[i - 16]; //					emilcont
    else
      return table[i] ^= table[i + 63]; //					emilcont
  }
} rnd;

Random::Random() : i( 0 ) { // Seed the table
  table[0] = 123456789;
  table[1] = 987654321;
  for( int j = 2; j < 55; ++j )
    table[j] = table[j - 1] * 11 + table[j - 2] * 0.1; //				emilcont
}

/* 3 byte counter, shown for reference only. It implements a
nonstationary pair of counters of 0s and 1s such that preference is
given to recent history by discarding old bits. */

class Counter3 {
  U8 n[2]; // n[y] is the counts of ys (0s or 1s)
public:
  Counter3() {
    n[0] = n[1] = 0;
  }
  int get0() const {
    return n[0];
  } // Return count of 0s
  int get1() const {
    return n[1];
  } // Return count of 1s
  int priority() const {
    return get0() + get1();
  }                   // For hash replacement
  void add( int y ) { // Add y (0 or 1) to n[y] and age the opposite count
    if( n[y] < 255 )
      ++n[y];
    if( n[1 - y] > 2 )
      n[1 - y] /= 2, n[1 - y] += 1;
  }
};

/* Approximately equivalent 2 byte counter implementing the above.
The representable counts (n0, n1) are 0-10, 12, 14, 16, 20, 24, 28,
32, 48, 64, 128, 256, 512. Both counts are represented by a single
8-bit state. Counts larger than 10 are incremented probabilistically.
Although it uses 1/3 less memory, it is 8% slower and gives 0.05% worse
compression than the 3 byte counter. */

class Counter2 {
  U8 state;
  struct E {     // State table entry
    U16 n0, n1;  // Counts represented by state
    U8 s00, s01; // Next state on input 0 without/with probabilistic incr.
    U8 s10, s11; // Next state on input 1
    U32 p0, p1;  // Probability of increment x 2^32-1 on inputs 0, 1
  };
  static E table[150]; // State table
public:
  Counter2() : state( 0 ) {}
  int get0() const {
    return table[state].n0;
  }
  int get1() const {
    return table[state].n1;
  }
  int priority() const {
    return state;
  }
  void add( int y ) {
    if( y != 0 ) {
      if( state < 75 || rnd() < table[state].p1 ) //						emilcont
        state = table[state].s11;
      else
        state = table[state].s10;
    } else {
      if( state < 75 || rnd() < table[state].p0 ) //						emilcont
        state = table[state].s01;
      else
        state = table[state].s00;
    }
  }
};

// State table generated by stategen.cpp
Counter2::E Counter2::table[150] = {
    // n0 n1 s00 s01 s10 s11 p0 p1 state

    {0, 0, 0, 2, 0, 1, 4294967295u, 4294967295u},        // 0					emilcont
    {0, 1, 1, 4, 1, 3, 4294967295u, 4294967295u},        // 1			ALL THE TABLE REGENERATED BY EMILCONT
    {1, 0, 2, 5, 2, 4, 4294967295u, 4294967295u},        // 2
    {0, 2, 1, 4, 3, 6, 4294967295u, 4294967295u},        // 3
    {1, 1, 4, 8, 4, 7, 4294967295u, 4294967295u},        // 4
    {2, 0, 5, 9, 2, 4, 4294967295u, 4294967295u},        // 5
    {0, 3, 1, 4, 6, 10, 4294967295u, 4294967295u},       // 6
    {1, 2, 4, 8, 7, 11, 4294967295u, 4294967295u},       // 7
    {2, 1, 8, 13, 4, 7, 4294967295u, 4294967295u},       // 8
    {3, 0, 9, 14, 2, 4, 4294967295u, 4294967295u},       // 9
    {0, 4, 3, 7, 10, 15, 4294967295u, 4294967295u},      // 10
    {1, 3, 4, 8, 11, 16, 4294967295u, 4294967295u},      // 11
    {2, 2, 8, 13, 7, 11, 4294967295u, 4294967295u},      // 12
    {3, 1, 13, 19, 4, 7, 4294967295u, 4294967295u},      // 13
    {4, 0, 14, 20, 5, 8, 4294967295u, 4294967295u},      // 14
    {0, 5, 3, 7, 15, 21, 4294967295u, 4294967295u},      // 15
    {1, 4, 7, 12, 16, 22, 4294967295u, 4294967295u},     // 16
    {2, 3, 8, 13, 11, 16, 4294967295u, 4294967295u},     // 17
    {3, 2, 13, 19, 7, 11, 4294967295u, 4294967295u},     // 18
    {4, 1, 19, 26, 8, 12, 4294967295u, 4294967295u},     // 19
    {5, 0, 20, 27, 5, 8, 4294967295u, 4294967295u},      // 20
    {0, 6, 6, 11, 21, 28, 4294967295u, 4294967295u},     // 21
    {1, 5, 7, 12, 22, 29, 4294967295u, 4294967295u},     // 22
    {2, 4, 12, 18, 16, 22, 4294967295u, 4294967295u},    // 23
    {3, 3, 13, 19, 11, 16, 4294967295u, 4294967295u},    // 24
    {4, 2, 19, 26, 12, 17, 4294967295u, 4294967295u},    // 25
    {5, 1, 26, 34, 8, 12, 4294967295u, 4294967295u},     // 26
    {6, 0, 27, 35, 9, 13, 4294967295u, 4294967295u},     // 27
    {0, 7, 6, 11, 28, 36, 4294967295u, 4294967295u},     // 28
    {1, 6, 11, 17, 29, 37, 4294967295u, 4294967295u},    // 29
    {2, 5, 12, 18, 22, 29, 4294967295u, 4294967295u},    // 30
    {3, 4, 18, 25, 16, 22, 4294967295u, 4294967295u},    // 31
    {4, 3, 19, 26, 17, 23, 4294967295u, 4294967295u},    // 32
    {5, 2, 26, 34, 12, 17, 4294967295u, 4294967295u},    // 33
    {6, 1, 34, 42, 13, 18, 4294967295u, 4294967295u},    // 34
    {7, 0, 35, 43, 9, 13, 4294967295u, 4294967295u},     // 35
    {0, 8, 6, 11, 36, 44, 4294967295u, 4294967295u},     // 36
    {1, 7, 11, 17, 37, 45, 4294967295u, 4294967295u},    // 37
    {2, 6, 17, 24, 29, 37, 4294967295u, 4294967295u},    // 38
    {3, 5, 18, 25, 22, 29, 4294967295u, 4294967295u},    // 39
    {5, 3, 26, 34, 17, 23, 4294967295u, 4294967295u},    // 40
    {6, 2, 34, 42, 18, 24, 4294967295u, 4294967295u},    // 41
    {7, 1, 42, 50, 13, 18, 4294967295u, 4294967295u},    // 42
    {8, 0, 43, 51, 9, 13, 4294967295u, 4294967295u},     // 43
    {0, 9, 10, 16, 44, 60, 4294967295u, 4294967295u},    // 44
    {1, 8, 11, 17, 45, 52, 4294967295u, 4294967295u},    // 45
    {2, 7, 17, 24, 37, 45, 4294967295u, 4294967295u},    // 46
    {3, 6, 24, 32, 29, 37, 4294967295u, 4294967295u},    // 47
    {6, 3, 34, 42, 24, 31, 4294967295u, 4294967295u},    // 48
    {7, 2, 42, 50, 18, 24, 4294967295u, 4294967295u},    // 49
    {8, 1, 50, 57, 13, 18, 4294967295u, 4294967295u},    // 50
    {9, 0, 51, 61, 14, 19, 4294967295u, 4294967295u},    // 51
    {1, 9, 16, 23, 52, 63, 4294967295u, 4294967295u},    // 52
    {2, 8, 17, 24, 45, 52, 4294967295u, 4294967295u},    // 53
    {3, 7, 24, 32, 37, 45, 4294967295u, 4294967295u},    // 54
    {7, 3, 42, 50, 24, 31, 4294967295u, 4294967295u},    // 55
    {8, 2, 50, 57, 18, 24, 4294967295u, 4294967295u},    // 56
    {9, 1, 57, 64, 19, 25, 4294967295u, 4294967295u},    // 57
    {2, 9, 23, 31, 52, 63, 4294967295u, 4294967295u},    // 58
    {9, 2, 57, 64, 25, 32, 4294967295u, 4294967295u},    // 59
    {0, 10, 10, 16, 60, 62, 4294967295u, 4294967295u},   // 60
    {10, 0, 61, 65, 14, 19, 4294967295u, 4294967295u},   // 61
    {0, 11, 15, 22, 62, 66, 4294967295u, 4294967295u},   // 62
    {1, 10, 16, 23, 63, 67, 4294967295u, 4294967295u},   // 63
    {10, 1, 64, 70, 19, 25, 4294967295u, 4294967295u},   // 64
    {11, 0, 65, 71, 20, 26, 4294967295u, 4294967295u},   // 65
    {0, 12, 15, 22, 66, 72, 4294967295u, 4294967295u},   // 66
    {1, 11, 22, 30, 67, 73, 4294967295u, 4294967295u},   // 67
    {2, 10, 23, 31, 63, 67, 4294967295u, 4294967295u},   // 68
    {10, 2, 64, 70, 25, 32, 4294967295u, 4294967295u},   // 69
    {11, 1, 70, 76, 26, 33, 4294967295u, 4294967295u},   // 70
    {12, 0, 71, 77, 20, 26, 4294967295u, 4294967295u},   // 71
    {0, 13, 21, 29, 72, 78, 4294967295u, 4294967295u},   // 72
    {1, 12, 22, 30, 73, 79, 4294967295u, 4294967295u},   // 73
    {2, 11, 30, 39, 67, 73, 4294967295u, 4294967295u},   // 74
    {11, 2, 70, 76, 33, 40, 4294967295u, 4294967295u},   // 75
    {12, 1, 76, 82, 26, 33, 4294967295u, 4294967295u},   // 76
    {13, 0, 77, 83, 27, 34, 4294967295u, 4294967295u},   // 77
    {0, 14, 21, 29, 78, 84, 4294967295u, 4294967295u},   // 78
    {1, 13, 29, 38, 79, 85, 4294967295u, 4294967295u},   // 79
    {2, 12, 30, 39, 73, 79, 4294967295u, 4294967295u},   // 80
    {12, 2, 76, 82, 33, 40, 4294967295u, 4294967295u},   // 81
    {13, 1, 82, 88, 34, 41, 4294967295u, 4294967295u},   // 82
    {14, 0, 83, 89, 27, 34, 4294967295u, 4294967295u},   // 83
    {0, 15, 28, 37, 84, 90, 4294967295u, 4294967295u},   // 84
    {1, 14, 29, 38, 85, 91, 4294967295u, 4294967295u},   // 85
    {2, 13, 38, 47, 79, 85, 4294967295u, 4294967295u},   // 86
    {13, 2, 82, 88, 41, 48, 4294967295u, 4294967295u},   // 87
    {14, 1, 88, 92, 34, 41, 4294967295u, 4294967295u},   // 88
    {15, 0, 89, 93, 35, 42, 4294967295u, 4294967295u},   // 89
    {0, 16, 28, 37, 90, 94, 4294967295u, 4294967295u},   // 90
    {1, 15, 37, 46, 91, 95, 4294967295u, 4294967295u},   // 91
    {15, 1, 92, 98, 42, 49, 4294967295u, 4294967295u},   // 92
    {16, 0, 93, 99, 35, 42, 4294967295u, 4294967295u},   // 93
    {0, 17, 36, 45, 94, 100, 4294967295u, 4294967295u},  // 94
    {1, 16, 37, 46, 95, 101, 4294967295u, 4294967295u},  // 95
    {2, 15, 46, 54, 91, 95, 4294967295u, 4294967295u},   // 96
    {15, 2, 92, 98, 49, 55, 4294967295u, 4294967295u},   // 97
    {16, 1, 98, 102, 42, 49, 4294967295u, 4294967295u},  // 98
    {17, 0, 99, 103, 43, 50, 4294967295u, 4294967295u},  // 99
    {0, 18, 36, 45, 100, 104, 4294967295u, 4294967295u}, // 100
    {1, 17, 45, 53, 101, 105, 4294967295u, 4294967295u}, // 101
    {17, 1, 102, 106, 50, 56, 4294967295u, 4294967295u}, // 102
    {18, 0, 103, 107, 43, 50, 4294967295u, 4294967295u}, // 103
    {0, 19, 44, 52, 104, 108, 4294967295u, 4294967295u}, // 104
    {1, 18, 45, 53, 105, 109, 4294967295u, 4294967295u}, // 105
    {18, 1, 106, 110, 50, 56, 4294967295u, 4294967295u}, // 106
    {19, 0, 107, 111, 51, 57, 4294967295u, 4294967295u}, // 107
    {0, 20, 44, 52, 108, 114, 4294967295u, 1073741823u}, // 108
    {1, 19, 52, 58, 109, 112, 4294967295u, 4294967295u}, // 109
    {19, 1, 110, 113, 57, 59, 4294967295u, 4294967295u}, // 110
    {20, 0, 111, 115, 51, 57, 1073741823u, 4294967295u}, // 111
    {1, 20, 52, 58, 112, 116, 4294967295u, 1073741823u}, // 112
    {20, 1, 113, 117, 57, 59, 1073741823u, 4294967295u}, // 113
    {0, 24, 60, 63, 114, 118, 4294967295u, 1073741823u}, // 114
    {24, 0, 115, 119, 61, 64, 1073741823u, 4294967295u}, // 115
    {1, 24, 63, 68, 116, 120, 4294967295u, 1073741823u}, // 116
    {24, 1, 117, 121, 64, 69, 1073741823u, 4294967295u}, // 117
    {0, 28, 60, 63, 118, 122, 4294967295u, 1073741823u}, // 118
    {28, 0, 119, 123, 61, 64, 1073741823u, 4294967295u}, // 119
    {1, 28, 63, 68, 120, 124, 4294967295u, 1073741823u}, // 120
    {28, 1, 121, 125, 64, 69, 1073741823u, 4294967295u}, // 121
    {0, 32, 62, 67, 122, 126, 4294967295u, 268435455u},  // 122
    {32, 0, 123, 127, 65, 70, 268435455u, 4294967295u},  // 123
    {1, 32, 67, 74, 124, 128, 4294967295u, 268435455u},  // 124
    {32, 1, 125, 129, 70, 75, 268435455u, 4294967295u},  // 125
    {0, 48, 62, 67, 126, 130, 4294967295u, 268435455u},  // 126
    {48, 0, 127, 131, 65, 70, 268435455u, 4294967295u},  // 127
    {1, 48, 67, 74, 128, 132, 4294967295u, 268435455u},  // 128
    {48, 1, 129, 133, 70, 75, 268435455u, 4294967295u},  // 129
    {0, 64, 66, 73, 130, 134, 4294967295u, 134217727u},  // 130
    {64, 0, 131, 135, 71, 76, 134217727u, 4294967295u},  // 131
    {1, 64, 73, 80, 132, 136, 4294967295u, 134217727u},  // 132
    {64, 1, 133, 137, 76, 81, 134217727u, 4294967295u},  // 133
    {0, 96, 66, 73, 134, 138, 4294967295u, 134217727u},  // 134
    {96, 0, 135, 139, 71, 76, 134217727u, 4294967295u},  // 135
    {1, 96, 73, 80, 136, 140, 4294967295u, 134217727u},  // 136
    {96, 1, 137, 141, 76, 81, 134217727u, 4294967295u},  // 137
    {0, 128, 72, 79, 138, 142, 4294967295u, 33554431u},  // 138
    {128, 0, 139, 143, 77, 82, 33554431u, 4294967295u},  // 139
    {1, 128, 79, 86, 140, 144, 4294967295u, 33554431u},  // 140
    {128, 1, 141, 145, 82, 87, 33554431u, 4294967295u},  // 141
    {0, 256, 72, 79, 142, 146, 4294967295u, 16777215u},  // 142
    {256, 0, 143, 147, 77, 82, 16777215u, 4294967295u},  // 143
    {1, 256, 79, 86, 144, 148, 4294967295u, 16777215u},  // 144
    {256, 1, 145, 149, 82, 87, 16777215u, 4294967295u},  // 145
    {0, 512, 84, 91, 146, 146, 4294967295u, 0u},         // 146
    {512, 0, 147, 147, 89, 92, 0u, 4294967295u},         // 147
    {1, 512, 91, 96, 148, 148, 4294967295u, 0u},         // 148
    {512, 1, 149, 149, 92, 97, 0u, 4294967295u},         // 149
};

// Counter2 (less memory) or Counter3 (faster and better compression)
typedef Counter2 Counter;

/* Hashtable<T, N> is a hash table of 2^N elements of type T
with linear search of M=3 elements in case of collision. If all elements
collide, then the one with the lowest T.priority() is replaced. 
Hashtable[i] returns a T& indexed by the lower N bits of i whose
checksum matches bits 31-24 of i, creating or replacing if needed.
The user must supply T::priority() and a 32-bit hash. */

template <class T, int N, int M = 3>
class Hashtable {
private:
  struct HashElement {
    U8 checksum;
    T value;
    HashElement( int ch = 0 ) : checksum( ch ), value() {}
  };
  vector<HashElement> table; // Array of 2^N+M elements
public:
  Hashtable() : table( ( 1 << N ) + M ) {}
  inline T &operator[]( U32 i );
};

template <class T, int N, int M>
T &Hashtable<T, N, M>::operator[]( U32 i ) {
  const U32 lb = i & ( ( 1 << N ) - 1 ); // Lower bound of search range
  const U32 ub = lb + M;                 // Upper bound + 1
  const int checksum = i >> 24;
  int bj = lb;
  int bget = 1000000000;
  for( U32 j = lb; j < ub; ++j ) {
    HashElement &c = table[j];
    int g = c.value.priority();
    if( g == 0 ) {
      c = HashElement( checksum );
      return c.value;
    }
    if( checksum == c.checksum )
      return c.value;
    if( g < bget ) {
      bget = g;
      bj = j;
    }
  }
  table[bj] = HashElement( checksum );
  return table[bj].value;
}
/* MatchModel, RecordModel and CharModel all have a function
model(y, n0, n1) which updates the model with bit y (0 or 1), then
returns an array of counts in n0[] and n1[]. Each pair of elements
represents a probability n1[i]/n with weight n that the next value of
y will be 1, where n = n0[i]+n1[i]. The number of elements written
depends on the model, and is usually defined as N in the class.

A MatchModel looks for a match of length n >= 8 bytes between
the current context and previous input, and predicts the next bit
in the previous context with weight n. If the next bit is 1, then
(n0[0], n1[0]) is assigned (0, n), else (n, 0). Matchies are found in
a 4MB rotating buffer using a 1M hash table of pointers. */

class MatchModel {
  enum { N = 1 };  // Number of submodels
  vector<U8> buf;  // Input buffer, wraps at end
  vector<U24> ptr; // Hash table of pointers
  U32 hash[2];     // Hashes of current context up to pos-1
  int pos;         // Element of buf where next bit will be stored
  int bpos;        // Number of bits (0-7) stored at buf[pos]
  int begin;       // Points to first matching byte (does not wrap)
  int end;         // Points to last matching byte + 1, 0 if no match
public:
  MatchModel() : buf( 0x10000 << MEM ), ptr( 0x4000 << MEM ), pos( 0 ), bpos( 0 ), begin( 0 ), end( 0 ) {
    hash[0] = hash[1] = 0;
  }
  void model( int y, int *n0, int *n1 );
} matchModel;

void MatchModel::model( int y, int *n0, int *n1 ) {
  buf[pos] += buf[pos] + y; // Store bit
  ++bpos;
  if( ( end != 0 ) && ( buf[end] >> ( 8 - bpos ) ) != buf[pos] ) // Does it match?
    begin = end = 0;                                             // no
  if( bpos == 8 ) {                                              // New byte
    bpos = 0;
    hash[0] = hash[0] * ( 16 * 56797157 ) + buf[pos] + 1; // Hash last 8 bytes
    hash[1] = hash[1] * ( 2 * 45684217 ) + buf[pos] + 1;  // Hash last 32 bytes
    U32 h = hash[0] >> ( 10 );                            //						emilcont
    if( ( hash[0] >> 28 ) == 0 )
      h = hash[1] >> ( 10 ); // 1/16 of 8-contexts are hashed to 32 bytes			emilcont
    if( ++pos == int( buf.size() ) )
      pos = 0;
    if( end != 0 )
      ++end;
    else { // Search for a matching context
      end = ptr[h];
      if( end > 0 ) {
        begin = end;
        int p = pos;
        while( begin > 0 && p > 0 && begin != p + 1 && buf[begin - 1] == buf[p - 1] ) {
          --begin;
          --p;
        }
      }
      if( end == begin ) // No match found
        begin = end = 0;
    }
    ptr[h] = U24( pos );
  }
  *n0 = *n1 = 0;
  if( end != 0 ) {
    int wt = end - begin;
    if( wt > 255 )
      wt = 255;
    int y = ( buf[end] >> ( 7 - bpos ) ) & 1;
    if( y != 0 )
      *n1 = wt;
    else
      *n0 = wt;
  }
}

/* A RecordModel finds fixed length records and models bits in the context
of the two bytes above (the same position in the two previous records)
and in the context of the byte above and to the left (the previous byte).
The record length is assumed to be the interval in the most recent
occurrence of a byte occuring 4 times in a row equally spaced, e.g.
"x..x..x..x" would imply a record size of 3 (the mimimum). */

class RecordModel {
  enum { N = 2 };           // Number of models
  static int lpos[256][4];  // Position of last 4 bytes
  vector<U8> buf;           // Rotating history [4K]
  Hashtable<Counter, 23> t; // Model							emilcont
  int pos;                  // Byte counter
  int c0;                   // Leading 1 and 0-7 bits
  int repeat;               // Cycle length
  int hash[N];              // of c0, context and repeat
  Counter *cp[N];           // Pointer to current counter
public:
  RecordModel();
  void model( int y, int *n0, int *n1 ); // Update and predict
} recordModel;
int RecordModel::lpos[256][4] = {{0}};

RecordModel::RecordModel() : buf( 4096 ), pos( 0 ), c0( 1 ), repeat( 1 ) {
  hash[0] = hash[1] = 0;
  cp[0] = cp[1] = &t[0];
}

// Update the model with bit y, then put predictions of the next update
// as 0 counts in n0[0..N-1] and 1 counts in n1[0..N-1]
void RecordModel::model( int y, int *n0, int *n1 ) {
  // Update the counters with bit y
  cp[0]->add( y );
  cp[1]->add( y );
  c0 += c0 + y;
  if( c0 >= 256 ) {
    c0 -= 256;

    // Save positions of last 4 occurrences of current byte c0
    int( &lp )[4] = lpos[c0];
    for( int j = 3; j > 0; --j )
      lp[j] = lp[j - 1];
    lp[0] = pos;

    // Check for a repeating pattern of interval 3 or more
    if( lp[2] - lp[3] == lp[1] - lp[2] && lp[1] - lp[2] == lp[0] - lp[1] && lp[0] - lp[1] > 2 )
      repeat = lp[0] - lp[1];

    // Save c0
    buf[pos & ( buf.size() - 1 )] = c0;
    ++pos;

    // Compute context hashes
    hash[0] = buf[( pos - repeat ) & ( buf.size() - 1 )] * 578997481
              + buf[( pos - repeat * 2 ) & ( buf.size() - 1 )] * 878996291
              + repeat * 378997811; // Context is 2 bytes above
    hash[1] = buf[( pos - repeat ) & ( buf.size() - 1 )] * 236399113 + buf[( pos - 1 ) & ( buf.size() - 1 )] * 736390141
              + repeat * 984388129; // Context is the byte above and to the left
    c0 = 1;
  }

  // Set up counter pointers
  cp[0] = &t[hash[0] + ( c0 << 24 ) + ( ( c0 * 3 ) & 255 )];
  cp[1] = &t[hash[1] + ( c0 << 24 ) + ( ( c0 * 3 ) & 255 )];

  // Predict the next value of y
  n0[0] = cp[0]->get0();
  n1[0] = cp[0]->get1();
  n0[1] = cp[1]->get0();
  n1[1] = cp[1]->get1();
}

/* A CharModel contains a fixed model, n-gram models from 0 to 7,
and several order-2 sparse models which skip over parts of the context. */

class CharModel {
  enum { N = 16 };                // Number of models
  vector<Counter> t0;             // Order 0 counts [256]
  vector<Counter> t1;             // Order 1 counts [64K]
  Hashtable<Counter, 25> t2;      // Sparse models						//	emilcont
  Hashtable<Counter, 27> t;       // Order 2-7 models						//	emilcont
  int c0;                         // Current partial byte with leading 1 bit
  int c1, c2, c3, c4, c5, c6, c7; // Previous bytes
  vector<U32> cxt;                // Context hashes [N]
  vector<Counter *> cp;           // Pointers to current counts [N]
public:
  CharModel();
  void model( const int y, int *n0, int *n1 ); // Update and predict
  int getc0() const {
    return c0;
  }
  int getc1() const {
    return c1;
  }
  int getc2() const {
    return c2;
  }
} charModel;

CharModel::CharModel() :
    t0( 256 ),
    t1( 65536 ),
    t(),
    c0( 1 ),
    c1( 0 ),
    c2( 0 ),
    c3( 0 ),
    c4( 0 ),
    c5( 0 ),
    c6( 0 ),
    c7( 0 ),
    cxt( N ),
    cp( N ) {
  for( int i = 0; i < N; ++i )
    cp[i] = &t[0];
}

// Update with bit y, put array of 0 counts in n0 and 1 counts in n1
void CharModel::model( const int y, int *n0, int *n1 ) {
  // Update models
  for( int i = 1; i < N; ++i )
    cp[i]->add( y );

  // Update context
  c0 += c0 + y;
  if( c0 >= 256 ) { // Start new byte
    for( int i = 8; i > 1; --i )
      cxt[i] = ( cxt[i - 1] + c0 ) * 987660757;
    c0 -= 256;
    if( c0 > 32 )
      cxt[9] = ( cxt[9] + c0 ) * 34609027 * 4; // Last whole word (8 chars max)
    else
      cxt[9] = 0;
    cxt[10] = c0 * 837503159 + c2 * 537496093; // Sparse models
    cxt[11] = c0 * 840101893 + c3 * 850090301;
    cxt[12] = c1 * 769377353 + c3 * 653589317;
    cxt[13] = c3 * 368910977 + c7 * 568909763;
    cxt[14] = c1 * 950087393 + c2 * 970092001;
    cxt[15] = c2 * 629495183 + c3 * 649492307;
    c7 = c6;
    c6 = c5;
    c5 = c4;
    c4 = c3;
    c3 = c2;
    c2 = c1;
    c1 = c0;
    c0 = 1;
  }

  // Set up pointers to next counters
  cp[1] = &t0[c0];
  cp[2] = &t1[c0 + c1 * 256];
  for( int i = 3; i < 10; ++i )
    cp[i] = &t[cxt[i] + ( c0 << 24 ) + ( ( c0 * 3 ) & 255 )];
  for( int i = 10; i < N; ++i )
    cp[i] = &t2[cxt[i] + ( c0 << 24 ) + ( ( c0 * 3 ) & 255 )];

  // Return bit counts
  n0[0] = n1[0] = 1;
  for( int i = 1; i < N; ++i ) {
    n0[i] = cp[i]->get0();
    n1[i] = cp[i]->get1();
  }
}

/* A MixModel mixes an array of counts to guess the next bit. The counts
are adaptively weighted to minimize cost. model(y, pr) updates all the
models with y, gets their bit counts, weights them, and returns the
probability P(1) * PSCALE in pr. The weights are adjusted to minimize
the cost of encoding y. There are C sets of weights, selected by
a context (the 3 upper bits of the last whole byte). */

class MixModel {
  enum { N = 19, C = 8 }; // Number of models, number of weight contexts
  vector<int> wt;         // Context weights [C, N]
  int bc0[N], bc1[N];     // Bit counts concatenated from various models
  int cxt2;               // Secondary context to select a weight vector
public:
  MixModel();
  ~MixModel();
  void model( int y, int &pr ); // Update with bit y, then put next guess in pr
} mixModel;

MixModel::MixModel() : wt( C * N ), cxt2( 0 ) {
  for( int i = 0; i < C * N; ++i )
    wt[i] = 1;
  for( int i = 0; i < N; ++i )
    bc0[i] = bc1[i] = 0;
}

MixModel::~MixModel() {
  /*
  // Uncomment this to print the weights. This is useful for testing
  // new models or weight vector contexts.
  printf(" ");
  for (int i=0; i<N; ++i)
    printf("%4d", i);
  printf("\n");
  for (int i=0; i<C; ++i) {
    printf("%2d", i);
    for (int j=0; j<N; ++j)
      printf("%4d", wt[i*N+j]/10);
    printf("\n");
  } */
}

void MixModel::model( int y, int &pr ) {
  // Adjust the weights by gradient descent to reduce cost
  {
    const int cn = cxt2 * N;
    int s0 = 0, s1 = 0;
    for( int i = 0; i < N; ++i ) {
      s0 += ( wt[cn + i] + 48 ) * bc0[i];
      s1 += ( wt[cn + i] + 48 ) * bc1[i];
    }
    if( s0 > 0 && s1 > 0 ) {
      const int s = s0 + s1;
      const int sy = y != 0 ? s1 : s0;
      const int sy1 = ( 0xffffffff / sy + ( rnd() & 1023 ) ) >> 10;
      const int s1 = ( 0xffffffff / s + ( rnd() & 1023 ) ) >> 10;
      for( int i = 0; i < N; ++i ) {
        const int dw = int( ( y != 0 ? bc1[i] : bc0[i] ) * sy1 - ( bc0[i] + bc1[i] ) * s1 + ( rnd() & 255 ) ) >> 8;
        wt[cn + i] = min( 65535, max( 1, wt[cn + i] + dw ) );
      }
    }
  }

  // Get counts
  charModel.model( y, bc0, bc1 );
  recordModel.model( y, bc0 + 16, bc1 + 16 );
  matchModel.model( y, bc0 + 18, bc1 + 18 );

  // Update secondary context
  cxt2 = charModel.getc1() / ( 256 / C );

  // Predict next bit
  int n0 = 1, n1 = n0;
  for( int j = 0; j < N; ++j ) {
    int w = wt[cxt2 * N + j];
    n0 += bc0[j] * w;
    n1 += bc1[j] * w;
  }
  int n = n0 + n1;
  assert( n > 0 );
  while( n > 2000000000 / PSCALE )
    n /= 4, n1 /= 4;
  pr = int( ( PSCALE - 1 ) * n1 / n );
}

/* A Predictor adjusts the model probability using SSE and passes it
to the encoder. An SSE model is a table of counters, sse[SSE1][SSE2]
which maps a context and a probability into a new, more accurate
probability. The context, SSE1, consists of the 0-7 bits of the current
byte and the 2 leading bits of the previous byte. The probability
to be mapped, SSE2 is first stretched near 0 and 1 using SSEMap, then
quantized into SSE2=32 intervals. Each SSE element is a pair of 0
and 1 counters of the bits seen so far in the current context and
probability range. Both the bin below and above the current probability
is updated by adding 1 to the appropriate count (n0 or n1). The
output probability for an SSE element is n1/(n0+n1) interpolated between
the bins below and above the input probability. This is averaged
with the original probability with 25% weight to give the final
probability passed to the encoder. */

class Predictor {
  enum {
    SSE1 = 256 * 4,
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
    U8 c1, n; // Count of 1's, count of bits
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
    SSEContext() : c1( 0 ), n( 0 ) {}
  };

  vector<vector<SSEContext>> sse; // [SSE1][SSE2+1] context, mapped prob
  int nextp;                      // p()
  int ssep;                       // Output of sse
  int context;                    // SSE context
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

Predictor::Predictor() : sse( SSE1 ), nextp( PSCALE / 2 ), ssep( 512 ), context( 0 ) {
  // Initialize to sse[context][ssemap(p)] = p
  for( int i = 0; i < SSE1; ++i )
    sse[i].resize( SSE2 + 1 );
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

inline void Predictor::update( int y ) {
  // Update the bins below and above the last input probability, ssep
  sse[context][ssep / SSESCALE].update( y );
  sse[context][ssep / SSESCALE + 1].update( y );

  // Update/predict all the models
  mixModel.model( y, nextp );

  // Get the SSE context
  context = charModel.getc0() * 4 + charModel.getc1() / 64;

  // Get final probability, interpolate and average with original
  ssep = ssemap( nextp );
  int wt = ssep % SSESCALE;
  int i = ssep / SSESCALE;
  nextp = ( ( ( sse[context][i].p() * ( SSESCALE - wt ) + sse[context][i + 1].p() * wt ) / SSESCALE ) * 3 + nextp ) / 4;
}

/* An Encoder does arithmetic encoding. Methods:
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
subrange. Output leading bytes of the range as they become known. */

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
      c = 0; // Fix for version 2
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

// Read one byte from encoder e
int decompress( Encoder &e ) { // Decompress 8 bits, MSB first
  int c = 0;
  for( int i = 0; i < 8; ++i )
    c = c + c + e.decode();
  return c;
}

// Write one byte c to encoder e
void compress( Encoder &e, int c ) {
  for( int i = 7; i >= 0; --i )
    e.encode( ( c >> i ) & 1 );
}

// Fail if out of memory
void handler() {
  printf( "Out of memory\n" );
  exit( 1 );
}

// Read and return a line of input from FILE f (default stdin) up to
// first control character except tab. Skips CR in CR LF.
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
  set_new_handler( handler );

  // Test the compiler
  assert( sizeof( U8 ) == 1 );
  assert( sizeof( U16 ) == 2 );
  assert( sizeof( U24 ) == 3 );
  assert( sizeof( U32 ) == 4 );
  assert( sizeof( int ) == 4 );

  // Check arguments
  if( argc < 2 ) {
    printf( PROGNAME " v2 file compressor/archiver, (C) 2003, Matt Mahoney, mmahoney@cs.fit.edu\n"
                     "This program is free software distributed without warranty under the terms\n"
                     "of the GNU General Public License, see http://www.gnu.org/licenses/gpl.txt\n"
                     "\n"
                     "To compress: " PROGNAME " archive filenames... (archive will be created)\n"
                     " or (MSDOS): dir/b | " PROGNAME " archive (reads file names from input)\n"
                     "To extract/compare: " PROGNAME " archive (does not clobber existing files)\n"
                     "To view contents: more < archive\n" );
    return 1;
  }

  // File names and sizes from input or archive
  vector<string> filename; // List of names
  vector<long> filesize;   // Size or -1 if error
  int start_time = clock();
  int uncompressed_bytes = 0, compressed_bytes = 0; // Input, output sizes

  // Extract files
  FILE *archive = fopen( argv[1], "rb" );
  if( archive != nullptr ) {
    if( argc > 2 ) {
      printf( "File %s already exists\n", argv[1] );
      return 1;
    }
    printf( "Extracting archive %s ...\n", argv[1] );

    // Read PROGNAME "\r\n" at start of archive
    if( getline( archive ) != PROGNAME ) {
      printf( "Archive file %s not in " PROGNAME " format\n", argv[1] );
      return 1;
    }

    // Read "size filename" in "%d\t%s\r\n" format
    while( true ) {
      string s = getline( archive );
      if( s.size() > 1 ) {
        filesize.push_back( atol( s.c_str() ) );
        string::iterator tab = find( s.begin(), s.end(), '\t' );
        if( tab != s.end() )
          filename.push_back( string( tab + 1, s.end() ) );
        else
          filename.push_back( "" );
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
    Encoder e( DECOMPRESS, archive );
    for( int i = 0; i < int( filename.size() ); ++i ) {
      printf( "%10ld %s: ", filesize[i], filename[i].c_str() );

      // Compare with existing file
      FILE *f = fopen( filename[i].c_str(), "rb" );
      const long size = filesize[i];
      uncompressed_bytes += size;
      if( f != nullptr ) {
        bool different = false;
        for( long j = 0; j < size; ++j ) {
          int c1 = decompress( e );
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
        f = fopen( filename[i].c_str(), "wb" );
        if( f == nullptr )
          printf( "cannot create, skipping...\n" );
        for( long j = 0; j < size; ++j ) {
          int c = decompress( e );
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
        filename.push_back( argv[i] );
    else {
      printf( "Enter names of files to compress, followed by blank line or EOF.\n" );
      while( true ) {
        string s = getline( stdin );
        if( s.empty() )
          break;
        else
          filename.push_back( s );
      }
    }

    // Get file sizes
    for( int i = 0; i < int( filename.size() ); ++i ) {
      FILE *f = fopen( filename[i].c_str(), "rb" );
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
    archive = fopen( argv[1], "wb" );
    if( archive == nullptr ) {
      printf( "Cannot create archive: %s\n", argv[1] );
      return 1;
    }
    fprintf( archive, PROGNAME "\r\n" );
    for( int i = 0; i < int( filename.size() ); ++i ) {
      if( filesize[i] >= 0 )
        fprintf( archive, "%ld\t%s\r\n", filesize[i], filename[i].c_str() );
    }
    putc( 032, archive ); // MSDOS EOF
    putc( '\f', archive );
    putc( 0, archive );

    // Write data
    Encoder e( COMPRESS, archive );
    long file_start = ftell( archive );
    for( int i = 0; i < int( filename.size() ); ++i ) {
      const long size = filesize[i];
      if( size >= 0 ) {
        uncompressed_bytes += size;
        printf( "%-23s %10ld -> ", filename[i].c_str(), size );
        FILE *f = fopen( filename[i].c_str(), "rb" );
        int c;
        for( long j = 0; j < size; ++j ) {
          if( f != nullptr )
            c = getc( f );
          else
            c = 0;
          compress( e, c );
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
  const double elapsed_time = double( clock() - start_time ) / CLOCKS_PER_SEC;
  printf( "%d/%d in %1.2f sec.", compressed_bytes, uncompressed_bytes, elapsed_time );
  if( uncompressed_bytes > 0 && elapsed_time > 0 ) {
    printf( " (%1.4f bpc, %1.2f%% at %1.0f KB/s)", compressed_bytes * 8.0 / uncompressed_bytes,
            compressed_bytes * 100.0 / uncompressed_bytes, uncompressed_bytes / ( elapsed_time * 1000.0 ) );
  }
  printf( "\n" );
  return 0;
}
