#include <emmintrin.h>

int dot_product( const short *const t, const short *const w, int n ) {
  __m128i sum = _mm_setzero_si128();

  while( ( n -= 8 ) >= 0 ) {
    __m128i tmp = _mm_madd_epi16( *( __m128i * ) &t[n], *( __m128i * ) &w[n] );
    tmp = _mm_srai_epi32( tmp, 8 );
    sum = _mm_add_epi32( sum, tmp );
  }

  sum = _mm_add_epi32( sum, _mm_srli_si128( sum, 8 ) );
  sum = _mm_add_epi32( sum, _mm_srli_si128( sum, 4 ) );
  return _mm_cvtsi128_si32( sum );
}

void train( const short *const t, short *const w, int n, const int e ) {
  const __m128i one = _mm_set1_epi16( 1 );
  const __m128i err = _mm_set1_epi16( short( e ) );

  while( ( n -= 8 ) >= 0 ) {
    __m128i tmp = _mm_adds_epi16( *( __m128i * ) &t[n], *( __m128i * ) &t[n] );
    tmp = _mm_mulhi_epi16( tmp, err );
    tmp = _mm_adds_epi16( tmp, one );
    tmp = _mm_srai_epi16( tmp, 1 );
    tmp = _mm_adds_epi16( tmp, *( __m128i * ) &w[n] );
    *( __m128i * ) &w[n] = tmp;
  }
}
