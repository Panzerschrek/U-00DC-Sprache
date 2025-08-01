#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#ifdef __GNUC__
#define ENABLE_128BIT_INT_TESTS
#endif

#define TEST_ASSERT(x) { if( !(x) ) { std::cerr << "Line " << __LINE__ << " assertion failed: " << #x << std::endl; std::abort(); } }

// std::tuple isn't POD, so, use our own replacement.
template<typename T0> struct Tuple1
{
	T0 v0;
	bool operator==( const Tuple1& other ) const
	{
		return this->v0 == other.v0;
	}
};

template<typename T0, typename T1> struct Tuple2
{
	T0 v0;
	T1 v1;
	bool operator==( const Tuple2& other ) const
	{
		return this->v0 == other.v0 && this->v1 == other.v1;
	}
};

template<typename T0, typename T1, typename T2> struct Tuple3
{
	T0 v0;
	T1 v1;
	T2 v2;
	bool operator==( const Tuple3& other ) const
	{
		return this->v0 == other.v0 && this->v1 == other.v1 && this->v2 == other.v2;
	}
};

template<typename T0, typename T1, typename T2, typename T3> struct Tuple4
{
	T0 v0;
	T1 v1;
	T2 v2;
	T3 v3;
	bool operator==( const Tuple4& other ) const
	{
		return this->v0 == other.v0 && this->v1 == other.v1 && this->v2 == other.v2 && this->v3 == other.v3;
	}
};

// Use explicit template instantiations here, since it helps to silence nasty compilation warnings/errors about C++-class return types of "extern C" functions.
template class std::array<int8_t, 1>;
template class std::array<int8_t, 2>;
template class std::array<int8_t, 3>;
template class std::array<int8_t, 4>;
template class std::array<int8_t, 5>;
template class std::array<int8_t, 6>;
template class std::array<int8_t, 7>;
template class std::array<int8_t, 8>;
template class std::array<int8_t, 9>;
template class std::array<int8_t, 10>;
template class std::array<int8_t, 11>;
template class std::array<int8_t, 12>;
template class std::array<int8_t, 13>;
template class std::array<int8_t, 14>;
template class std::array<int8_t, 15>;
template class std::array<int8_t, 16>;
template class std::array<int8_t, 17>;
template class std::array<int8_t, 35>;
template class std::array<uint16_t, 1>;
template class std::array<uint16_t, 2>;
template class std::array<uint16_t, 3>;
template class std::array<uint16_t, 4>;
template class std::array<uint16_t, 5>;
template class std::array<uint16_t, 6>;
template class std::array<uint16_t, 7>;
template class std::array<uint16_t, 8>;
template class std::array<uint16_t, 9>;
template class std::array<uint16_t, 15>;
template class std::array<uint16_t, 21>;
template class std::array<int32_t, 1>;
template class std::array<int32_t, 2>;
template class std::array<int32_t, 3>;
template class std::array<int32_t, 4>;
template class std::array<int32_t, 5>;
template class std::array<int32_t, 6>;
template class std::array<int32_t, 7>;
template class std::array<int32_t, 8>;
template class std::array<int32_t, 9>;
template class std::array<int32_t, 18>;
#ifdef ENABLE_128BIT_INT_TESTS
template class std::array<__int128_t, 1>;
template class std::array<__int128_t, 2>;
template class std::array<__int128_t, 3>;
#endif
template class std::array<float, 19>;

template struct Tuple2<int8_t, int8_t>;
template struct Tuple2<int8_t, uint16_t>;
template struct Tuple2<int8_t, int32_t>;
template struct Tuple2<int8_t, uint64_t>;
#ifdef ENABLE_128BIT_INT_TESTS
template struct Tuple2<int8_t, __int128_t>;
#endif
template struct Tuple2<uint16_t, uint8_t>;
template struct Tuple2<uint16_t, int16_t>;
template struct Tuple2<uint16_t, uint32_t>;
template struct Tuple2<uint16_t, int64_t>;
#ifdef ENABLE_128BIT_INT_TESTS
template struct Tuple2<uint16_t, __uint128_t>;
#endif
template struct Tuple2<int32_t, int8_t>;
template struct Tuple2<int32_t, uint16_t>;
template struct Tuple2<int32_t, int32_t>;
template struct Tuple2<int32_t, uint64_t>;
#ifdef ENABLE_128BIT_INT_TESTS
template struct Tuple2<int32_t, __int128_t>;
#endif
template struct Tuple2<uint64_t, uint8_t>;
template struct Tuple2<uint64_t, int16_t>;
template struct Tuple2<uint64_t, uint32_t>;
template struct Tuple2<uint64_t, int64_t>;
#ifdef ENABLE_128BIT_INT_TESTS
template struct Tuple2<uint64_t, __uint128_t>;
#endif
template struct Tuple2<float, int8_t>;
template struct Tuple2<float, uint16_t>;
template struct Tuple2<float, int32_t>;
template struct Tuple2<float, uint64_t>;
#ifdef ENABLE_128BIT_INT_TESTS
template struct Tuple2<float, __int128_t>;
#endif
template struct Tuple2<double, uint8_t>;
template struct Tuple2<double, int16_t>;
template struct Tuple2<double, uint32_t>;
template struct Tuple2<double, int64_t>;
#ifdef ENABLE_128BIT_INT_TESTS
template struct Tuple2<double, __uint128_t>;
#endif

extern "C"
{

void Pass_bool_Test0( const bool x ) { TEST_ASSERT( x == false ); }
void Pass_bool_Test1( const bool x ) { TEST_ASSERT( x == true ); }
void Pass_i8_Test0( const int8_t x ) { TEST_ASSERT( x == 27 ); }
void Pass_i8_Test1( const int8_t x ) { TEST_ASSERT( x == -58 ); }
void Pass_i8_Test2(
	const int8_t x0, const int8_t x1, const int8_t x2, const int8_t x3,
	const int8_t x4, const int8_t x5, const int8_t x6, const int8_t x7,
	const int8_t x8, const int8_t x9, const int8_t xa, const int8_t xb,
	const int8_t xc, const int8_t xd, const int8_t xe, const int8_t xf )
{
	TEST_ASSERT( x0 == 64 ); TEST_ASSERT( x1 == 31 ); TEST_ASSERT( x2 == 78 ); TEST_ASSERT( x3 == 120 );
	TEST_ASSERT( x4 == -36 ); TEST_ASSERT( x5 == 67 ); TEST_ASSERT( x6 == 13 ); TEST_ASSERT( x7 == 58 );
	TEST_ASSERT( x8 == 0 ); TEST_ASSERT( x9 == 45 ); TEST_ASSERT( xa == 99 ); TEST_ASSERT( xb == 105 );
	TEST_ASSERT( xc == -128 ); TEST_ASSERT( xd == 127 ); TEST_ASSERT( xe == 33 ); TEST_ASSERT( xf == -88 );
}
void Pass_u8_Test0( const uint8_t x ) { TEST_ASSERT( x == 117 ); }
void Pass_u8_Test1( const uint8_t x ) { TEST_ASSERT( x == 134 ); }
void Pass_u8_Test2( const uint8_t x ) { TEST_ASSERT( x == 249 ); }
void Pass_i16_Test0( const int16_t x ) { TEST_ASSERT( x == 27346 ); }
void Pass_i16_Test1( const int16_t x ) { TEST_ASSERT( x == -15343 ); }
void Pass_u16_Test0( const uint16_t x ) { TEST_ASSERT( x == 17 ); }
void Pass_u16_Test1( const uint16_t x ) { TEST_ASSERT( x == 15642 ); }
void Pass_u16_Test2( const uint16_t x ) { TEST_ASSERT( x == 30651 ); }
void Pass_u16_Test3( const uint16_t x ) { TEST_ASSERT( x == 52188 ); }
void Pass_u16_Test4(
	const uint16_t x0, const uint16_t x1, const uint16_t x2, const uint16_t x3,
	const uint16_t x4, const uint16_t x5, const uint16_t x6, const uint16_t x7,
	const uint16_t x8, const uint16_t x9, const uint16_t xa, const uint16_t xb,
	const uint16_t xc, const uint16_t xd, const uint16_t xe, const uint16_t xf )
{
	TEST_ASSERT( x0 == 7655 ); TEST_ASSERT( x1 == 32768 ); TEST_ASSERT( x2 == 6582 ); TEST_ASSERT( x3 == 49 );
	TEST_ASSERT( x4 == 0 ); TEST_ASSERT( x5 == 256 ); TEST_ASSERT( x6 == 9821 ); TEST_ASSERT( x7 == 65535 );
	TEST_ASSERT( x8 == 25843 ); TEST_ASSERT( x9 == 58441 ); TEST_ASSERT( xa == 864 ); TEST_ASSERT( xb == 8962 );
	TEST_ASSERT( xc == 123 ); TEST_ASSERT( xd == 645 ); TEST_ASSERT( xe == 32767 ); TEST_ASSERT( xf == 33 );
}
void Pass_i32_Test0( const int32_t x ) { TEST_ASSERT( x == 274383 ); }
void Pass_i32_Test1( const int32_t x ) { TEST_ASSERT( x == -7456 ); }
void Pass_i32_Test2( const int32_t x ) { TEST_ASSERT( x == 0x78ABCDEF ); }
void Pass_i32_Test3( const int32_t x ) { TEST_ASSERT( x == -674348993 ); }
void Pass_i32_Test4(
	const int32_t x0, const int32_t x1, const int32_t x2, const int32_t x3,
	const int32_t x4, const int32_t x5, const int32_t x6, const int32_t x7,
	const int32_t x8, const int32_t x9, const int32_t xa, const int32_t xb,
	const int32_t xc, const int32_t xd, const int32_t xe, const int32_t xf )
{
	TEST_ASSERT( x0 == 6531 ); TEST_ASSERT( x1 == -75247554 ); TEST_ASSERT( x2 == 456424 ); TEST_ASSERT( x3 == 8565523 );
	TEST_ASSERT( x4 == 0 ); TEST_ASSERT( x5 == 0x7FFFFFFF ); TEST_ASSERT( x6 == 54 ); TEST_ASSERT( x7 == int32_t(-0x80000000ll) );
	TEST_ASSERT( x8 == 643 ); TEST_ASSERT( x9 == 7621375 ); TEST_ASSERT( xa == 7567863 ); TEST_ASSERT( xb == -24782 );
	TEST_ASSERT( xc == 786234786 ); TEST_ASSERT( xd == 12308562 ); TEST_ASSERT( xe == -8624557 ); TEST_ASSERT( xf == 867245 );
}
void Pass_u32_Test0( const uint32_t x ) { TEST_ASSERT( x == 78u ); }
void Pass_u32_Test1( const uint32_t x ) { TEST_ASSERT( x == 45677u ); }
void Pass_u32_Test2( const uint32_t x ) { TEST_ASSERT( x == 6633477u ); }
void Pass_u32_Test3( const uint32_t x ) { TEST_ASSERT( x == 0xFEDCBA98u ); }
void Pass_i64_Test0( const int64_t x ) { TEST_ASSERT( x == 636746ll ); }
void Pass_i64_Test1( const int64_t x ) { TEST_ASSERT( x == -36ll ); }
void Pass_i64_Test2( const int64_t x ) { TEST_ASSERT( x == -6433763852258913ll ); }
void Pass_i64_Test3( const int64_t x ) { TEST_ASSERT( x == 7434744889515923245ll ); }
void Pass_u64_Test0( const uint64_t x ) { TEST_ASSERT( x == 7612ull ); }
void Pass_u64_Test1( const uint64_t x ) { TEST_ASSERT( x == 2147521472ull ); }
void Pass_u64_Test2( const uint64_t x ) { TEST_ASSERT( x == 7445889504678477554ull ); }
void Pass_u64_Test3( const uint64_t x ) { TEST_ASSERT( x == 18246784073809531617ull ); }
void Pass_u64_Test4(
	const uint64_t x0, const uint64_t x1, const uint64_t x2, const uint64_t x3,
	const uint64_t x4, const uint64_t x5, const uint64_t x6, const uint64_t x7,
	const uint64_t x8, const uint64_t x9, const uint64_t xa, const uint64_t xb,
	const uint64_t xc, const uint64_t xd, const uint64_t xe, const uint64_t xf )
{
	TEST_ASSERT( x0 == 36774676ull ); TEST_ASSERT( x1 == 78ull ); TEST_ASSERT( x2 == 478543737834754785ull ); TEST_ASSERT( x3 == 0ull );
	TEST_ASSERT( x4 == 21ull ); TEST_ASSERT( x5 == 78542136ull ); TEST_ASSERT( x6 == 8847838ull ); TEST_ASSERT( x7 == 47547247472367861ull );
	TEST_ASSERT( x8 == 7623758235ull ); TEST_ASSERT( x9 == 88524ull ); TEST_ASSERT( xa == 76521ull ); TEST_ASSERT( xb == 0xFF00112233445566ull );
	TEST_ASSERT( xc == 77852ull ); TEST_ASSERT( xd == 651374ull ); TEST_ASSERT( xe == 86ull ); TEST_ASSERT( xf == 3741356ull );
}
#ifdef ENABLE_128BIT_INT_TESTS
void Pass_i128_Test0( const __int128_t x ) { TEST_ASSERT( x == ( ( __int128_t( 0x0123456789ABCDEFll ) << 64u ) | __int128_t(0xFEDCBA9876543210ll) ) ); }
void Pass_u128_Test0( const __uint128_t x ) { TEST_ASSERT( x == ( ( __uint128_t( 0xFEDCBA9876543210ull ) << 64u ) | __uint128_t(0x0123456789ABCDEFull) ) ); }
#endif
// Use "unsigned char" for represent Ü "char8", since in C++ regular char signess is inplementation defined.
void Pass_char8_Test0( const unsigned char x ) { TEST_ASSERT( x == 'Q' ); }
void Pass_char8_Test1( const unsigned char x ) { TEST_ASSERT( x == '!' ); }
void Pass_char8_Test2( const unsigned char x ) { TEST_ASSERT( x == ' ' ); }
void Pass_char8_Test3( const unsigned char x ) { TEST_ASSERT( x == 240 ); }
void Pass_char16_Test0( const char16_t x ) { TEST_ASSERT( x == u'Ж' ); }
void Pass_char16_Test1( const char16_t x ) { TEST_ASSERT( x == u'Ꙥ' ); }
void Pass_char32_Test0( const char32_t x ) { TEST_ASSERT( x == U'😀' ); }
void Pass_f32_Test0( const float x ) { TEST_ASSERT( x == 0.0f ); }
void Pass_f32_Test1( const float x ) { TEST_ASSERT( x == 0.125f ); }
void Pass_f32_Test2( const float x ) { TEST_ASSERT( x == 6743.5f ); }
void Pass_f32_Test3( const float x ) { TEST_ASSERT( x == -7689543378437.0f ); }
void Pass_f32_Test4( const float x ) { TEST_ASSERT( std::isinf(x) ); }
void Pass_f32_Test5( const float x ) { TEST_ASSERT( std::isnan(x) ); }
void Pass_f32_Test6(
	const float x0, const float x1, const float x2, const float x3,
	const float x4, const float x5, const float x6, const float x7,
	const float x8, const float x9, const float xa, const float xb,
	const float xc, const float xd, const float xe, const float xf )
{
	TEST_ASSERT( x0 == 1786.5f ); TEST_ASSERT( x1 == -643.4f ); TEST_ASSERT( x2 == 754.0f ); TEST_ASSERT( x3 == 353347.0f );
	TEST_ASSERT( x4 == 3000000.0f ); TEST_ASSERT( x5 == -4454.25f ); TEST_ASSERT( x6 == 0.0f ); TEST_ASSERT( x7 == 66434.0f );
	TEST_ASSERT( x8 == 3643.3f ); TEST_ASSERT( x9 == 367341.5f ); TEST_ASSERT( xa == 67436.125f ); TEST_ASSERT( xb == 378436.0f );
	TEST_ASSERT( xc == 42.75f ); TEST_ASSERT( xd == -7542.2f ); TEST_ASSERT( xe == 6564.0f ); TEST_ASSERT( xf == 7854300000000.0f );
}
void Pass_f64_Test0( const double x ) { TEST_ASSERT( x == 0.0 ); }
void Pass_f64_Test1( const double x ) { TEST_ASSERT( x == 0.0625 ); }
void Pass_f64_Test2( const double x ) { TEST_ASSERT( x == 173.25 ); }
void Pass_f64_Test3( const double x ) { TEST_ASSERT( x == -569907695478437.0 ); }
void Pass_f64_Test4( const double x ) { TEST_ASSERT( std::isinf(x) ); }
void Pass_f64_Test5( const double x ) { TEST_ASSERT( std::isnan(x) ); }
void Pass_f64_Test6(
	const double x0, const double x1, const double x2, const double x3,
	const double x4, const double x5, const double x6, const double x7,
	const double x8, const double x9, const double xa, const double xb,
	const double xc, const double xd, const double xe, const double xf )
{
	TEST_ASSERT( x0 == 364341.5 ); TEST_ASSERT( x1 == 1786.5 ); TEST_ASSERT( x2 == -643.4 ); TEST_ASSERT( x3 == 353347.0 );
	TEST_ASSERT( x4 == 70000000.0 ); TEST_ASSERT( x5 == -4454.25 ); TEST_ASSERT( x6 == 7854320000000.0 ); TEST_ASSERT( x7 == 0.0 );
	TEST_ASSERT( x8 == 66434.0 ); TEST_ASSERT( x9 == 3643.3 ); TEST_ASSERT( xa == 67436.125 ); TEST_ASSERT( xb == 754.0 );
	TEST_ASSERT( xc == 378436.0 ); TEST_ASSERT( xd == -42.75 ); TEST_ASSERT( xe == -6552.4 ); TEST_ASSERT( xf == 6564.0 );
}
void Pass_u8_x1_Test0( const std::array<uint8_t, 1> x )
{
	TEST_ASSERT( x[0] == 0xB4 );
}
void Pass_u8_x2_Test0( const std::array<uint8_t, 2> x )
{
	TEST_ASSERT( x[0] == 0xAB ); TEST_ASSERT( x[1] == 0x7C );
}
void Pass_u8_x3_Test0( const std::array<uint8_t, 3> x )
{
	TEST_ASSERT( x[0] == 0x16 ); TEST_ASSERT( x[1] == 0xF7 ); TEST_ASSERT( x[2] == 0x75 );
}
void Pass_u8_x4_Test0( const std::array<uint8_t, 4> x )
{
	TEST_ASSERT( x[0] == 0x01 ); TEST_ASSERT( x[1] == 0x23 ); TEST_ASSERT( x[2] == 0x45 ); TEST_ASSERT( x[3] == 0x67 );
}
void Pass_u8_x5_Test0( const std::array<uint8_t, 5> x )
{
	TEST_ASSERT( x[0] == 0x89 ); TEST_ASSERT( x[1] == 0xAB ); TEST_ASSERT( x[2] == 0xCD ); TEST_ASSERT( x[3] == 0xEF );
	TEST_ASSERT( x[4] == 0x76 );
}
void Pass_u8_x6_Test0( const std::array<uint8_t, 6> x )
{
	TEST_ASSERT( x[0] == 0x11 ); TEST_ASSERT( x[1] == 0x22 ); TEST_ASSERT( x[2] == 0x33 ); TEST_ASSERT( x[3] == 0x44 );
	TEST_ASSERT( x[4] == 0x55 ); TEST_ASSERT( x[5] == 0x66 );
}
void Pass_u8_x7_Test0( const std::array<uint8_t, 7> x )
{
	TEST_ASSERT( x[0] == 0x77 ); TEST_ASSERT( x[1] == 0x88 ); TEST_ASSERT( x[2] == 0x99 ); TEST_ASSERT( x[3] == 0xAA );
	TEST_ASSERT( x[4] == 0xBB ); TEST_ASSERT( x[5] == 0xCC ); TEST_ASSERT( x[6] == 0xDD );
}
void Pass_u8_x8_Test0( const std::array<uint8_t, 8> x )
{
	TEST_ASSERT( x[0] == 0xF0 ); TEST_ASSERT( x[1] == 0xE1 ); TEST_ASSERT( x[2] == 0xD2 ); TEST_ASSERT( x[3] == 0xC3 );
	TEST_ASSERT( x[4] == 0xB4 ); TEST_ASSERT( x[5] == 0xA5 ); TEST_ASSERT( x[6] == 0x96 ); TEST_ASSERT( x[7] == 0x87 );
}
void Pass_u8_x9_Test0( const std::array<uint8_t, 9> x )
{
	TEST_ASSERT( x[0] == 0x01 ); TEST_ASSERT( x[1] == 0x12 ); TEST_ASSERT( x[2] == 0x23 ); TEST_ASSERT( x[3] == 0x34 );
	TEST_ASSERT( x[4] == 0x45 ); TEST_ASSERT( x[5] == 0x56 ); TEST_ASSERT( x[6] == 0x67 ); TEST_ASSERT( x[7] == 0x78 );
	TEST_ASSERT( x[8] == 0x89 );
}
void Pass_u8_x10_Test0( const std::array<uint8_t, 10> x )
{
	TEST_ASSERT( x[0] == 0x9A ); TEST_ASSERT( x[1] == 0xAB ); TEST_ASSERT( x[2] == 0xBC ); TEST_ASSERT( x[3] == 0xCD );
	TEST_ASSERT( x[4] == 0xDE ); TEST_ASSERT( x[5] == 0xEF ); TEST_ASSERT( x[6] == 0xF0 ); TEST_ASSERT( x[7] == 0x01 );
	TEST_ASSERT( x[8] == 0x12 ); TEST_ASSERT( x[9] == 0x23 );
}
void Pass_u8_x11_Test0( const std::array<uint8_t, 11> x )
{
	TEST_ASSERT( x[0] == 0xF0 ); TEST_ASSERT( x[1] == 0x1E ); TEST_ASSERT( x[2] == 0xD2 ); TEST_ASSERT( x[3] == 0x3C );
	TEST_ASSERT( x[4] == 0xB4 ); TEST_ASSERT( x[5] == 0x5A ); TEST_ASSERT( x[6] == 0x96 ); TEST_ASSERT( x[7] == 0x78 );
	TEST_ASSERT( x[8] == 0x11 ); TEST_ASSERT( x[9] == 0x22 ); TEST_ASSERT( x[10] == 0x33 );
}
void Pass_u8_x12_Test0( const std::array<uint8_t, 12> x )
{
	TEST_ASSERT( x[0] == 0xF7 ); TEST_ASSERT( x[1] == 0x8B ); TEST_ASSERT( x[2] == 0xE6 ); TEST_ASSERT( x[3] == 0x72 );
	TEST_ASSERT( x[4] == 0x85 ); TEST_ASSERT( x[5] == 0x00 ); TEST_ASSERT( x[6] == 0x3C ); TEST_ASSERT( x[7] == 0xFE );
	TEST_ASSERT( x[8] == 0xD5 ); TEST_ASSERT( x[9] == 0x91 ); TEST_ASSERT( x[10] == 0x4E ); TEST_ASSERT( x[11] == 0x67 );
}
void Pass_u8_x13_Test0( const std::array<uint8_t, 13> x )
{
	for( uint32_t i= 0; i < 13; ++i )
	{
		TEST_ASSERT( x[i] == uint8_t( i * i + i * 7u ) );
	}
}
void Pass_u8_x14_Test0( const std::array<uint8_t, 14> x )
{
	for( uint32_t i= 0; i < 14; ++i )
	{
		TEST_ASSERT( x[i] == uint8_t( i * i - i * 5u + 3u ) );
	}
}
void Pass_u8_x15_Test0( const std::array<uint8_t, 15> x )
{
	for( uint32_t i= 0; i < 15; ++i )
	{
		TEST_ASSERT( x[i] == uint8_t( i * i + i * 3u - 7u ) );
	}
}
void Pass_u8_x16_Test0( const std::array<uint8_t, 16> x )
{
	for( uint32_t i= 0; i < 16; ++i )
	{
		TEST_ASSERT( x[i] == uint8_t( 3u * i * i + i * 7u - 2u ) );
	}
}
void Pass_u8_x17_Test0( const std::array<uint8_t, 17> x )
{
	for( uint32_t i= 0; i < 17; ++i )
	{
		TEST_ASSERT( x[i] == uint8_t( i * i + i * 13u - 3567u ) );
	}
}
void Pass_u8_x29_Test0( const std::array<uint8_t, 29> x )
{
	for( uint32_t i= 0; i < 29; ++i )
	{
		TEST_ASSERT( x[i] == uint8_t( ( i * i ) ^ ( i + 13u ) ) );
	}
}
void Pass_u8_x371_Test0( const std::array<uint8_t, 371> x )
{
	for( uint32_t i= 0; i < 371; ++i )
	{
		TEST_ASSERT( x[i] == uint8_t( i * 3u - i * i * 6u + 564u ) );
	}
}
void Pass_i16_x1_Test0( const std::array<int16_t, 1> x )
{
	TEST_ASSERT( x[0] == -27816 );
}
void Pass_i16_x2_Test0( const std::array<int16_t, 2> x )
{
	TEST_ASSERT( x[0] == 1754 ); TEST_ASSERT( x[1] == -6534 );
}
void Pass_i16_x3_Test0( const std::array<int16_t, 3> x )
{
	TEST_ASSERT( x[0] == -1234 ); TEST_ASSERT( x[1] == 30431 ); TEST_ASSERT( x[2] == 561 );
}
void Pass_i16_x4_Test0( const std::array<int16_t, 4> x )
{
	TEST_ASSERT( x[0] == 29554 ); TEST_ASSERT( x[1] == -63 ); TEST_ASSERT( x[2] == 2452 ); TEST_ASSERT( x[3] == -22543 );
}
void Pass_i16_x5_Test0( const std::array<int16_t, 5> x )
{
	TEST_ASSERT( x[0] == -3431 ); TEST_ASSERT( x[1] == 9655 ); TEST_ASSERT( x[2] == 15667 ); TEST_ASSERT( x[3] == 46 );
	TEST_ASSERT( x[4] == 19734 );
}
void Pass_i16_x6_Test0( const std::array<int16_t, 6> x )
{
	TEST_ASSERT( x[0] == 3451 ); TEST_ASSERT( x[1] == 29655 ); TEST_ASSERT( x[2] == 93 ); TEST_ASSERT( x[3] == -5667 );
	TEST_ASSERT( x[4] == 19734 ); TEST_ASSERT( x[5] == -4323 );
}
void Pass_i16_x7_Test0( const std::array<int16_t, 7> x )
{
	TEST_ASSERT( x[0] == 3351 ); TEST_ASSERT( x[1] == 2955 ); TEST_ASSERT( x[2] == 5393 ); TEST_ASSERT( x[3] == -5667 );
	TEST_ASSERT( x[4] == -4323 ); TEST_ASSERT( x[5] == 19234 ); TEST_ASSERT( x[6] == -3373 );
}
void Pass_i16_x8_Test0( const std::array<int16_t, 8> x )
{
	TEST_ASSERT( x[0] == -3373 ); TEST_ASSERT( x[1] == 3351 ); TEST_ASSERT( x[2] == -5953 ); TEST_ASSERT( x[3] == 15353 );
	TEST_ASSERT( x[4] == 5667 ); TEST_ASSERT( x[5] == 4323 ); TEST_ASSERT( x[6] == -29214 ); TEST_ASSERT( x[7] == 5342 );
}
void Pass_i16_x9_Test0( const std::array<int16_t, 9> x )
{
	TEST_ASSERT( x[0] == 7322 ); TEST_ASSERT( x[1] == -3373 ); TEST_ASSERT( x[2] == 3351 ); TEST_ASSERT( x[3] == -5953 );
	TEST_ASSERT( x[4] == 5667 ); TEST_ASSERT( x[5] == -29214 ); TEST_ASSERT( x[6] == 5342 ); TEST_ASSERT( x[7] == 25353 );
	TEST_ASSERT( x[8] == -6343 );
}
void Pass_i16_x15_Test0( const std::array<int16_t, 15> x )
{
	for( int32_t i= 0; i < 15; ++i )
	{
		TEST_ASSERT( x[ size_t(i) ] == int16_t( i * i * 7 - i * 37 + 3 ) );
	}
}
void Pass_i16_x83_Test0( const std::array<int16_t, 83> x )
{
	for( int32_t i= 0; i < 83; ++i )
	{
		TEST_ASSERT( x[ size_t(i) ] == int16_t( i * i * 5 - i * 43 + 11 ) );
	}
}
void Pass_u32_x1_Test0( const std::array<uint32_t, 1> x )
{
	TEST_ASSERT( x[0] == 0xFBA633ADu );
}
void Pass_u32_x2_Test0( const std::array<uint32_t, 2> x )
{
	TEST_ASSERT( x[0] == 0x5356A4D7u ); TEST_ASSERT( x[1] == 0x05AD74CBu );
}
void Pass_u32_x3_Test0( const std::array<uint32_t, 3> x )
{
	TEST_ASSERT( x[0] == 0x15A67FCBu ); TEST_ASSERT( x[1] == 0x5D56A437u ); TEST_ASSERT( x[2] == 0xAB4C8F12u );
}
void Pass_u32_x4_Test0( const std::array<uint32_t, 4> x )
{
	TEST_ASSERT( x[0] == 0x23A68FCAu ); TEST_ASSERT( x[1] == 0x1E5AA732u ); TEST_ASSERT( x[2] == 0xC34D8F12u ); TEST_ASSERT( x[3] == 0xF354AB3Eu );
}
void Pass_u32_x5_Test0( const std::array<uint32_t, 5> x )
{
	TEST_ASSERT( x[0] == 0x5E5AE732u ); TEST_ASSERT( x[1] == 0x33A68FCAu ); TEST_ASSERT( x[2] == 0xE34D8F12u ); TEST_ASSERT( x[3] == 0xD354AB3Eu );
	TEST_ASSERT( x[4] == 0x03AD63C3u );
}
void Pass_u32_x6_Test0( const std::array<uint32_t, 6> x )
{
	TEST_ASSERT( x[0] == 0x34A68FCEu ); TEST_ASSERT( x[1] == 0x5E53E732u ); TEST_ASSERT( x[2] == 0xE34D8A12u ); TEST_ASSERT( x[3] == 0x03ADE3C3u );
	TEST_ASSERT( x[4] == 0xD354CB3Eu ); TEST_ASSERT( x[5] == 0x42D4E6C8u );
}
void Pass_u32_x7_Test0( const std::array<uint32_t, 7> x )
{
	TEST_ASSERT( x[0] == 0x14A68FCEu ); TEST_ASSERT( x[1] == 0x5F53E732u ); TEST_ASSERT( x[2] == 0x42D4E6F8u ); TEST_ASSERT( x[3] == 0xE36D8A12u );
	TEST_ASSERT( x[4] == 0x63ADF3C3u ); TEST_ASSERT( x[5] == 0x7354CE3Eu ); TEST_ASSERT( x[6] == 0x63E7F7C5u );
}
void Pass_u32_x8_Test0( const std::array<uint32_t, 8> x )
{
	TEST_ASSERT( x[0] == 0xE3E7F731u ); TEST_ASSERT( x[1] == 0xE4A686CEu ); TEST_ASSERT( x[2] == 0xD2D4E638u ); TEST_ASSERT( x[3] == 0xF36D8A62u );
	TEST_ASSERT( x[4] == 0x63ADF4C3u ); TEST_ASSERT( x[5] == 0x7E54CE3Eu ); TEST_ASSERT( x[6] == 0xDF53E732u ); TEST_ASSERT( x[7] == 0xC3E47C15u );
}
void Pass_u32_x9_Test0( const std::array<uint32_t, 9> x )
{
	TEST_ASSERT( x[0] == 0xE3E1F731u ); TEST_ASSERT( x[1] == 0xE4A686CEu ); TEST_ASSERT( x[2] == 0x5E7CD38Fu ); TEST_ASSERT( x[3] == 0xD2D48638u );
	TEST_ASSERT( x[4] == 0xF36D8A62u ); TEST_ASSERT( x[5] == 0x63ADF5C3u ); TEST_ASSERT( x[6] == 0x7E54CE3Eu ); TEST_ASSERT( x[7] == 0xDF51E732u );
	TEST_ASSERT( x[8] == 0xE3E47C15u );
}
void Pass_u32_x17_Test0( const std::array<uint32_t, 17> x )
{
	for( uint32_t i= 0; i < 17u; ++i )
	{
		TEST_ASSERT( x[i] == i * i * i * 37u + i * i * 52u + i * 12u + 36747u );
	}
}
void Pass_u64_x1_Test0( const std::array<uint64_t, 1> x )
{
	TEST_ASSERT( x[0] == 0xFBA633ADE4A686CEull );
}
void Pass_u64_x2_Test0( const std::array<uint64_t, 2> x )
{
	TEST_ASSERT( x[0] == 0xEBA631ADE4968FC3ull ); TEST_ASSERT( x[1] == 0x5E7CD38FDF53E732ull );
}
void Pass_u64_x3_Test0( const std::array<uint64_t, 3> x )
{
	TEST_ASSERT( x[0] == 0x5E72D38FDF53E73Eull ); TEST_ASSERT( x[1] == 0xEBA631ACE4968FC4ull ); TEST_ASSERT( x[2] == 0xC3E47C1534A68FCEull );
}
void Pass_u64_x4_Test0( const std::array<uint64_t, 4> x )
{
	TEST_ASSERT( x[0] == 0xE3E7F731AB4C8F12ull ); TEST_ASSERT( x[1] == 0x1E72D38FDF52E73Eull ); TEST_ASSERT( x[2] == 0xEBA631FCE4968FC4ull ); TEST_ASSERT( x[3] == 0xC3E4741534A68FCEull );
}
void Pass_u64_x5_Test0( const std::array<uint64_t, 5> x )
{
	TEST_ASSERT( x[0] == 0x13E7F7313B4C8F12ull ); TEST_ASSERT( x[1] == 0x1E72D38FDC52E79Eull ); TEST_ASSERT( x[2] == 0x7353CE3ED2D48638ull ); TEST_ASSERT( x[3] == 0xFBA631FCE1968FC4ull );
	TEST_ASSERT( x[4] == 0xC3E1741534A68F7Eull );
}
void Pass_u64_x11_Test0( const std::array<uint64_t, 11> x )
{
	for( uint64_t i= 0; i < 11u; ++i )
	{
		TEST_ASSERT( x[ size_t(i) ] == i * i * i * 337547u + i * i * i * 563454548u + 34565224787u );
	}
}
#ifdef ENABLE_128BIT_INT_TESTS
void Pass_u128_x1_Test0( const std::array<__uint128_t, 1> x )
{
	TEST_ASSERT( x[0] == ( ( __uint128_t( 0xEBA631ADE4968FC3ull ) << 64u ) | 0x5E7CD38FDF53E732ull ) );
}
void Pass_u128_x2_Test0( const std::array<__uint128_t, 2> x )
{
	TEST_ASSERT( x[0] == ( ( __uint128_t( 0xEEA631ADE4968FC3ull ) << 64u ) | 0x5E7CD3CFDF53E732ull ) );
	TEST_ASSERT( x[1] == ( ( __uint128_t( 0x7353CE3ED2D48638ull ) << 64u ) | 0xC3E4741534A68FCEull ) );
}
void Pass_u128_x3_Test0( const std::array<__uint128_t, 3> x )
{
	TEST_ASSERT( x[0] == ( ( __uint128_t( 0x1EA63AADE4968FC3ull ) << 64u ) | 0x5E7CDECFDF53E738ull ) );
	TEST_ASSERT( x[1] == ( ( __uint128_t( 0x7353CE3ED2D48138ull ) << 64u ) | 0xC354741534A68FFEull ) );
	TEST_ASSERT( x[2] == ( ( __uint128_t( 0x7353CE3ED2D48638ull ) << 64u ) | 0x13E7F7313B4C8F12ull ) );
}
#endif
void Pass_f32_x1_Test0( const std::array<float, 1> x )
{
	TEST_ASSERT( x[0] == -7878.25f );
}
void Pass_f32_x2_Test0( const std::array<float, 2> x )
{
	TEST_ASSERT( x[0] == 7.5f ); TEST_ASSERT( x[1] == 0.0625f );
}
void Pass_f32_x3_Test0( const std::array<float, 3> x )
{
	TEST_ASSERT( x[0] == 72.15f ); TEST_ASSERT( x[1] == 0.0f ); TEST_ASSERT( x[2] == -0.125f );
}
void Pass_f32_x4_Test0( const std::array<float, 4> x )
{
	TEST_ASSERT( x[0] == 3712.2f ); TEST_ASSERT( x[1] == 663300.0f ); TEST_ASSERT( x[2] == -336.25f ); TEST_ASSERT( x[3] == 250000000.0f );
}
void Pass_f32_x5_Test0( const std::array<float, 5> x )
{
	TEST_ASSERT( x[0] == -536.25f ); TEST_ASSERT( x[1] == 4711.4f ); TEST_ASSERT( x[2] == 66330230.0f ); TEST_ASSERT( x[3] == 270000000.0f );
	TEST_ASSERT( x[4] == -5333566.0f );
}
void Pass_f32_x6_Test0( const std::array<float, 6> x )
{
	TEST_ASSERT( x[0] == -4333563.0f ); TEST_ASSERT( x[1] == -536.25f ); TEST_ASSERT( x[2] == 4712.8f ); TEST_ASSERT( x[3] == 26330231.0f );
	TEST_ASSERT( x[4] == 130000000.0f ); TEST_ASSERT( x[5] == 0.01f );
}
void Pass_f32_x7_Test0( const std::array<float, 7> x )
{
	TEST_ASSERT( x[0] == 3712.3f ); TEST_ASSERT( x[1] == -1536.25f ); TEST_ASSERT( x[2] == 2633031.0f ); TEST_ASSERT( x[3] == -4323565.0f );
	TEST_ASSERT( x[4] == 13005000.0f ); TEST_ASSERT( x[5] == 0.02f );  TEST_ASSERT( x[6] == -6434.75f );
}
void Pass_f32_x8_Test0( const std::array<float, 8> x )
{
	TEST_ASSERT( x[0] == 3742.5f ); TEST_ASSERT( x[1] == 13005010.0f ); TEST_ASSERT( x[2] == -1566.25f ); TEST_ASSERT( x[3] == 1643031.0f );
	TEST_ASSERT( x[4] == -432515.5f ); TEST_ASSERT( x[5] == 0.04f ); TEST_ASSERT( x[6] == -634.75f ); TEST_ASSERT( x[7] == 164363.0f );
}
void Pass_f32_x9_Test0( const std::array<float, 9> x )
{
	TEST_ASSERT( x[0] == 162363.0f ); TEST_ASSERT( x[1] == 3742.7f ); TEST_ASSERT( x[2] == -1563.25f ); TEST_ASSERT( x[3] == 1644031.0f );
	TEST_ASSERT( x[4] == -437515.5f ); TEST_ASSERT( x[5] == 0.08f ); TEST_ASSERT( x[6] == 13005210.0f ); TEST_ASSERT( x[7] == -534.75f );
	TEST_ASSERT( x[8] == 345423.0f );
}
void Pass_f32_x15_Test0( const std::array<float, 15> x )
{
	for( uint32_t i= 0; i < 15u; ++i )
	{
		TEST_ASSERT( x[i] == float(i) * float(i) * 13.5f + 153.25f );
	}
}
void Pass_f32_x47_Test0( const std::array<float, 47> x )
{
	for( uint32_t i= 0; i < 47u; ++i )
	{
		TEST_ASSERT( x[i] == float(i) * float(i) * 12.75f + 253.5f );
	}
}
void Pass_f64_x1_Test0( const std::array<double, 1> x )
{
	TEST_ASSERT( x[0] == -7878.25 );
}
void Pass_f64_x2_Test0( const std::array<double, 2> x )
{
	TEST_ASSERT( x[0] == 7.5 ); TEST_ASSERT( x[1] == 0.0625 );
}
void Pass_f64_x3_Test0( const std::array<double, 3> x )
{
	TEST_ASSERT( x[0] == 72.15 ); TEST_ASSERT( x[1] == 0.0 ); TEST_ASSERT( x[2] == -0.125 );
}
void Pass_f64_x4_Test0( const std::array<double, 4> x )
{
	TEST_ASSERT( x[0] == 3712.2 ); TEST_ASSERT( x[1] == 663300.0 ); TEST_ASSERT( x[2] == -336.25 ); TEST_ASSERT( x[3] == 250000000.0 );
}
void Pass_f64_x5_Test0( const std::array<double, 5> x )
{
	TEST_ASSERT( x[0] == -536.25 ); TEST_ASSERT( x[1] == 4711.4 ); TEST_ASSERT( x[2] == 66330230.0 ); TEST_ASSERT( x[3] == 270000000.0 );
	TEST_ASSERT( x[4] == -5333566.0 );
}
void Pass_f64_x6_Test0( const std::array<double, 6> x )
{
	TEST_ASSERT( x[0] == -4333563.0 ); TEST_ASSERT( x[1] == -536.25 ); TEST_ASSERT( x[2] == 4712.8 ); TEST_ASSERT( x[3] == 26330231.0 );
	TEST_ASSERT( x[4] == 130000000.0 ); TEST_ASSERT( x[5] == 0.01 );
}
void Pass_f64_x7_Test0( const std::array<double, 7> x )
{
	TEST_ASSERT( x[0] == 3712.3 ); TEST_ASSERT( x[1] == -1536.25 ); TEST_ASSERT( x[2] == 2633031.0 ); TEST_ASSERT( x[3] == -4323565.0 );
	TEST_ASSERT( x[4] == 13005000.0 ); TEST_ASSERT( x[5] == 0.02 ); TEST_ASSERT( x[6] == -6434.75 );
}
void Pass_f64_x8_Test0( const std::array<double, 8> x )
{
	TEST_ASSERT( x[0] == 3742.5 ); TEST_ASSERT( x[1] == 13005010.0 ); TEST_ASSERT( x[2] == -1566.25 ); TEST_ASSERT( x[3] == 1643031.0 );
	TEST_ASSERT( x[4] == -432515.5 ); TEST_ASSERT( x[5] == 0.04 ); TEST_ASSERT( x[6] == -634.75 ); TEST_ASSERT( x[7] == 164363.0 );
}
void Pass_f64_x9_Test0( const std::array<double, 9> x )
{
	TEST_ASSERT( x[0] == 162363.0 ); TEST_ASSERT( x[1] == 3742.7 ); TEST_ASSERT( x[2] == -1563.25 ); TEST_ASSERT( x[3] == 1644031.0 );
	TEST_ASSERT( x[4] == -437515.5 ); TEST_ASSERT( x[5] == 0.08 ); TEST_ASSERT( x[6] == 13005210.0 ); TEST_ASSERT( x[7] == -534.75 );
	TEST_ASSERT( x[8] == 345423.0 );
}
void Pass_f64_x15_Test0( const std::array<double, 15> x )
{
	for( uint32_t i= 0; i < 15u; ++i )
	{
		TEST_ASSERT( x[i] == double(i) * double(i) * 13.5 + 153.25 );
	}
}
void Pass_f64_x47_Test0( const std::array<double, 47> x )
{
	for( uint32_t i= 0; i < 47u; ++i )
	{
		TEST_ASSERT( x[i] == double(i) * double(i) * 12.75 + 253.5 );
	}
}
void Pass_char8_x1_Test0( const std::array<char, 1> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "H", x.size() ) == 0 );
}
void Pass_char8_x2_Test0( const std::array<char, 2> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "-8", x.size() ) == 0 );
}
void Pass_char8_x3_Test0( const std::array<char, 3> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "Kek", x.size() ) == 0 );
}
void Pass_char8_x4_Test0( const std::array<char, 4> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "SPQR", x.size() ) == 0 );
}
void Pass_char8_x5_Test0( const std::array<char, 5> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "ApplE", x.size() ) == 0 );
}
void Pass_char8_x6_Test0( const std::array<char, 6> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "56 to ", x.size() ) == 0 );
}
void Pass_char8_x7_Test0( const std::array<char, 7> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "@#--ABe", x.size() ) == 0 );
}
void Pass_char8_x8_Test0( const std::array<char, 8> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "S.P.Q.R.", x.size() ) == 0 );
}
void Pass_char8_x9_Test0( const std::array<char, 9> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "Жопа!", x.size() ) == 0 );
}
void Pass_char8_x10_Test0( const std::array<char, 10> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "Black Mesa", x.size() ) == 0 );
}
void Pass_char8_x11_Test0( const std::array<char, 11> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "qwertyuiop[", x.size() ) == 0 );
}
void Pass_char8_x12_Test0( const std::array<char, 12> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "][poiuytrewq", x.size() ) == 0 );
}
void Pass_char8_x13_Test0( const std::array<char, 13> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "Computerliebe", x.size() ) == 0 );
}
void Pass_char8_x14_Test0( const std::array<char, 14> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "11 + 22 = some", x.size() ) == 0 );
}
void Pass_char8_x15_Test0( const std::array<char, 15> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "ABCDEFGHIJKLMNO", x.size() ) == 0 );
}
void Pass_char8_x16_Test0( const std::array<char, 16> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "Er ist wieder da", x.size() ) == 0 );
}
void Pass_char8_x17_Test0( const std::array<char, 17> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "XY = 33 + 44 - 55", x.size() ) == 0 );
}
void Pass_char8_x32_Test0( const std::array<char, 32> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "0123456789abcdefFEDCBA9876543210", x.size() ) == 0 );
}
void Pass_char8_x39_Test0( const std::array<char, 39> x )
{
	TEST_ASSERT( std::memcmp( x.data(), "Ficket euch, ihr beleidigt meine Augen!", x.size() ) == 0 );
}
void Pass_tup_i8_u8_Test0( const Tuple2<int8_t, uint8_t> x )
{
	TEST_ASSERT( x.v0 == -76 ); TEST_ASSERT( x.v1 == 214 );
}
void Pass_tup_i8_u8_Test1( const Tuple2<int8_t, uint8_t> x )
{
	TEST_ASSERT( x.v0 == 127 ); TEST_ASSERT( x.v1 == 13 );
}
void Pass_tup_i8_i16_Test0( const Tuple2<int8_t, int16_t> x )
{
	TEST_ASSERT( x.v0 == -72 ); TEST_ASSERT( x.v1 == 31000 );
}
void Pass_tup_i8_i16_Test1( const Tuple2<int8_t, int16_t> x )
{
	TEST_ASSERT( x.v0 == 105 ); TEST_ASSERT( x.v1 == -27823 );
}
void Pass_tup_i8_u32_Test0( const Tuple2<int8_t, uint32_t> x )
{
	TEST_ASSERT( x.v0 == 98 ); TEST_ASSERT( x.v1 == 0xFA56DE4Fu );
}
void Pass_tup_i8_u32_Test1( const Tuple2<int8_t, uint32_t> x )
{
	TEST_ASSERT( x.v0 == -123 ); TEST_ASSERT( x.v1 == 0x3AC6DE1Fu );
}
void Pass_tup_i8_i64_Test0( const Tuple2<int8_t, int64_t> x )
{
	TEST_ASSERT( x.v0 == 72 ); TEST_ASSERT( x.v1 == 6336747347783754868 );
}
void Pass_tup_i8_i64_Test1( const Tuple2<int8_t, int64_t> x )
{
	TEST_ASSERT( x.v0 == -13 ); TEST_ASSERT( x.v1 == -642476347823222 );
}
#ifdef ENABLE_128BIT_INT_TESTS
void Pass_tup_i8_u128_Test0( const Tuple2<int8_t, __uint128_t> x )
{
	TEST_ASSERT( x.v0 == 71 );
	if( true ) return; // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	TEST_ASSERT( x.v1 == ( ( __uint128_t(0x0123456789ABCDEFull) << 64 ) | 0xFEDCBA9876543210ull ) );
}
void Pass_tup_i8_u128_Test1( const Tuple2<int8_t, __uint128_t> x )
{
	TEST_ASSERT( x.v0 == -88 )
	if( true ) return; // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	TEST_ASSERT( x.v1 == ( ( __uint128_t(0xFEDCBA9876543210ull) << 64 ) | 0x0123456789ABCDEFull ) );
}
#endif
void Pass_tup_i8_f32_Test0( const Tuple2<int8_t, float> x )
{
	TEST_ASSERT( x.v0 == 78 ); TEST_ASSERT( x.v1 == 6763.5f );
}
void Pass_tup_i8_f32_Test1( const Tuple2<int8_t, float> x )
{
	TEST_ASSERT( x.v0 == -124 ); TEST_ASSERT( x.v1 == -6346470.0f );
}
void Pass_tup_i8_f64_Test0( const Tuple2<int8_t, double> x )
{
	TEST_ASSERT( x.v0 == 53 ); TEST_ASSERT( x.v1 == -67163.25 );
}
void Pass_tup_i8_f64_Test1( const Tuple2<int8_t, double> x )
{
	TEST_ASSERT( x.v0 == -97 ); TEST_ASSERT( x.v1 == 251.0 );
}
void Pass_tup_u16_i8_Test0( const Tuple2<uint16_t, int8_t> x )
{
	TEST_ASSERT( x.v0 == 1245 ); TEST_ASSERT( x.v1 == 114 );
}
void Pass_tup_u16_i8_Test1( const Tuple2<uint16_t, int8_t> x )
{
	TEST_ASSERT( x.v0 == 48437 ); TEST_ASSERT( x.v1 == -13 );
}
void Pass_tup_u16_u16_Test0( const Tuple2<uint16_t, uint16_t> x )
{
	TEST_ASSERT( x.v0 == 43311 ); TEST_ASSERT( x.v1 == 31000 );
}
void Pass_tup_u16_u16_Test1( const Tuple2<uint16_t, uint16_t> x )
{
	TEST_ASSERT( x.v0 == 37 ); TEST_ASSERT( x.v1 == 57823 );
}
void Pass_tup_u16_i32_Test0( const Tuple2<uint16_t, int32_t> x )
{
	TEST_ASSERT( x.v0 == 1298 ); TEST_ASSERT( x.v1 == -533167754 );
}
void Pass_tup_u16_i32_Test1( const Tuple2<uint16_t, int32_t> x )
{
	TEST_ASSERT( x.v0 == 65530 ); TEST_ASSERT( x.v1 == 336637444 );
}
void Pass_tup_u16_u64_Test0( const Tuple2<uint16_t, uint64_t> x )
{
	TEST_ASSERT( x.v0 == 257 ); TEST_ASSERT( x.v1 == 0x08192A3B4C5D6E7Full );
}
void Pass_tup_u16_u64_Test1( const Tuple2<uint16_t, uint64_t> x )
{
	TEST_ASSERT( x.v0 == 56316 ); TEST_ASSERT( x.v1 == 0xF7E6D5C4B3A29180ull );
}
#ifdef ENABLE_128BIT_INT_TESTS
void Pass_tup_u16_i128_Test0( const Tuple2<uint16_t, __int128_t> x )
{
	TEST_ASSERT( x.v0 == 712 );
	if( true ) return; // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	TEST_ASSERT( x.v1 == ( ( __int128_t(0x0123456789ABCDEFll) << 64 ) | 0x7EDCBA9876543210ll ) );
}
void Pass_tup_u16_i128_Test1( const Tuple2<uint16_t, __int128_t> x )
{
	TEST_ASSERT( x.v0 == 8812 )
	if( true ) return; // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	TEST_ASSERT( x.v1 == ( ( __int128_t(0x1EDCBA9876543210ll) << 64 ) | 0x0123456789ABCDEFll ) );
}
#endif
void Pass_tup_u16_f32_Test0( const Tuple2<uint16_t, float> x )
{
	TEST_ASSERT( x.v0 == 2467 ); TEST_ASSERT( x.v1 == 6263.5f );
}
void Pass_tup_u16_f32_Test1( const Tuple2<uint16_t, float> x )
{
	TEST_ASSERT( x.v0 == 9850 ); TEST_ASSERT( x.v1 == -6356470.0f );
}
void Pass_tup_u16_f64_Test0( const Tuple2<uint16_t, double> x )
{
	TEST_ASSERT( x.v0 == 3126 ); TEST_ASSERT( x.v1 == -37163.125 );
}
void Pass_tup_u16_f64_Test1( const Tuple2<uint16_t, double> x )
{
	TEST_ASSERT( x.v0 == 65535 ); TEST_ASSERT( x.v1 == 253.0 );
}
void Pass_tup_i32_u8_Test0( const Tuple2<int32_t, uint8_t> x )
{
	TEST_ASSERT( x.v0 == -3347237 ); TEST_ASSERT( x.v1 == 214 );
}
void Pass_tup_i32_u8_Test1( const Tuple2<int32_t, uint8_t> x )
{
	TEST_ASSERT( x.v0 == 9553344 ); TEST_ASSERT( x.v1 == 13 );
}
void Pass_tup_i32_i16_Test0( const Tuple2<int32_t, int16_t> x )
{
	TEST_ASSERT( x.v0 == -346314 ); TEST_ASSERT( x.v1 == 31000 );
}
void Pass_tup_i32_i16_Test1( const Tuple2<int32_t, int16_t> x )
{
	TEST_ASSERT( x.v0 == 78656858 ); TEST_ASSERT( x.v1 == -27823 );
}
void Pass_tup_i32_u32_Test0( const Tuple2<int32_t, uint32_t> x )
{
	TEST_ASSERT( x.v0 == 7542475 ); TEST_ASSERT( x.v1 == 0xFA56DE4Fu );
}
void Pass_tup_i32_u32_Test1( const Tuple2<int32_t, uint32_t> x )
{
	TEST_ASSERT( x.v0 == -36131647 ); TEST_ASSERT( x.v1 == 0x3AC6DE1Fu );
}
void Pass_tup_i32_i64_Test0( const Tuple2<int32_t, int64_t> x )
{
	TEST_ASSERT( x.v0 == 847823478 ); TEST_ASSERT( x.v1 == 6336747347783754868 );
}
void Pass_tup_i32_i64_Test1( const Tuple2<int32_t, int64_t> x )
{
	TEST_ASSERT( x.v0 == -854647 ); TEST_ASSERT( x.v1 == -642476347823222 );
}
#ifdef ENABLE_128BIT_INT_TESTS
void Pass_tup_i32_u128_Test0( const Tuple2<int32_t, __uint128_t> x )
{
	TEST_ASSERT( x.v0 == -643647 );
	if( true ) return; // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	TEST_ASSERT( x.v1 == ( ( __uint128_t(0x0123456789ABCDEFull) << 64 ) | 0xFEDCBA9876543210ull ) );
}
void Pass_tup_i32_u128_Test1( const Tuple2<int32_t, __uint128_t> x )
{
	TEST_ASSERT( x.v0 == 856247 )
	if( true ) return; // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	TEST_ASSERT( x.v1 == ( ( __uint128_t(0xFEDCBA9876543210ull) << 64 ) | 0x0123456789ABCDEFull ) );
}
#endif
void Pass_tup_i32_f32_Test0( const Tuple2<int32_t, float> x )
{
	TEST_ASSERT( x.v0 == 7542347 ); TEST_ASSERT( x.v1 == 6763.5f );
}
void Pass_tup_i32_f32_Test1( const Tuple2<int32_t, float> x )
{
	TEST_ASSERT( x.v0 == -334642 ); TEST_ASSERT( x.v1 == -6346470.0f );
}
void Pass_tup_i32_f64_Test0( const Tuple2<int32_t, double> x )
{
	TEST_ASSERT( x.v0 == 6413647 ); TEST_ASSERT( x.v1 == -67163.25 );
}
void Pass_tup_i32_f64_Test1( const Tuple2<int32_t, double> x )
{
	TEST_ASSERT( x.v0 == -5674137 ); TEST_ASSERT( x.v1 == 251.0 );
}
void Pass_tup_u64_i8_Test0( const Tuple2<uint64_t, int8_t> x )
{
	TEST_ASSERT( x.v0 == 0xFEDCBA9876543210u ); TEST_ASSERT( x.v1 == 114 );
}
void Pass_tup_u64_i8_Test1( const Tuple2<uint64_t, int8_t> x )
{
	TEST_ASSERT( x.v0 == 0xFED7BA9876543E10u ); TEST_ASSERT( x.v1 == -13 );
}
void Pass_tup_u64_u16_Test0( const Tuple2<uint64_t, uint16_t> x )
{
	TEST_ASSERT( x.v0 == 0x3EDCBA987654321Cu ); TEST_ASSERT( x.v1 == 31000 );
}
void Pass_tup_u64_u16_Test1( const Tuple2<uint64_t, uint16_t> x )
{
	TEST_ASSERT( x.v0 == 0x3EDCBAE87654721Cu ); TEST_ASSERT( x.v1 == 57823 );
}
void Pass_tup_u64_i32_Test0( const Tuple2<uint64_t, int32_t> x )
{
	TEST_ASSERT( x.v0 == 0x9EDCBAEA7654721Cu ); TEST_ASSERT( x.v1 == -533167754 );
}
void Pass_tup_u64_i32_Test1( const Tuple2<uint64_t, int32_t> x )
{
	TEST_ASSERT( x.v0 == 0x91DCBAEA765472ECu ); TEST_ASSERT( x.v1 == 336637444 );
}
void Pass_tup_u64_u64_Test0( const Tuple2<uint64_t, uint64_t> x )
{
	TEST_ASSERT( x.v0 == 0x91DC6AEA765477ECu ); TEST_ASSERT( x.v1 == 0x08192A3B4C5D6E7Full );
}
void Pass_tup_u64_u64_Test1( const Tuple2<uint64_t, uint64_t> x )
{
	TEST_ASSERT( x.v0 == 0xD1DC6AEA7E5477ECu ); TEST_ASSERT( x.v1 == 0xF7E6D5C4B3A29180ull );
}
#ifdef ENABLE_128BIT_INT_TESTS
void Pass_tup_u64_i128_Test0( const Tuple2<uint64_t, __int128_t> x )
{
	TEST_ASSERT( x.v0 == 0xD1D16AEA7E54778Cu );
	if( true ) return; // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	TEST_ASSERT( x.v1 == ( ( __int128_t(0x0123456789ABCDEFll) << 64 ) | 0x7EDCBA9876543210ll ) );
}
void Pass_tup_u64_i128_Test1( const Tuple2<uint64_t, __int128_t> x )
{
	TEST_ASSERT( x.v0 == 0x11D16AEA7E54278Cu )
	if( true ) return; // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	TEST_ASSERT( x.v1 == ( ( __int128_t(0x1EDCBA9876543210ll) << 64 ) | 0x0123456789ABCDEFll ) );
}
#endif
void Pass_tup_u64_f32_Test0( const Tuple2<uint64_t, float> x )
{
	TEST_ASSERT( x.v0 == 0x1ED16AEA7E54278Cu ); TEST_ASSERT( x.v1 == 6263.5f );
}
void Pass_tup_u64_f32_Test1( const Tuple2<uint64_t, float> x )
{
	TEST_ASSERT( x.v0 == 0x1ED1AAEA7E54279Cu ); TEST_ASSERT( x.v1 == -6356470.0f );
}
void Pass_tup_u64_f64_Test0( const Tuple2<uint64_t, double> x )
{
	TEST_ASSERT( x.v0 == 0xCED164EA7E54278Cu ); TEST_ASSERT( x.v1 == -37163.125 );
}
void Pass_tup_u64_f64_Test1( const Tuple2<uint64_t, double> x )
{
	TEST_ASSERT( x.v0 == 0xCE1164EA7354278Cu ); TEST_ASSERT( x.v1 == 253.0 );
}
void Pass_tup_f32_u8_Test0( const Tuple2<float, uint8_t> x )
{
	TEST_ASSERT( x.v0 == -3347237.0f ); TEST_ASSERT( x.v1 == 214 );
}
void Pass_tup_f32_u8_Test1( const Tuple2<float, uint8_t> x )
{
	TEST_ASSERT( x.v0 == 3643.25f ); TEST_ASSERT( x.v1 == 13 );
}
void Pass_tup_f32_i16_Test0( const Tuple2<float, int16_t> x )
{
	TEST_ASSERT( x.v0 == -346314.0f ); TEST_ASSERT( x.v1 == 31000 );
}
void Pass_tup_f32_i16_Test1( const Tuple2<float, int16_t> x )
{
	TEST_ASSERT( x.v0 == 43440.1f ); TEST_ASSERT( x.v1 == -27823 );
}
void Pass_tup_f32_u32_Test0( const Tuple2<float, uint32_t> x )
{
	TEST_ASSERT( x.v0 == 5336.0f ); TEST_ASSERT( x.v1 == 0xFA56DE4Fu );
}
void Pass_tup_f32_u32_Test1( const Tuple2<float, uint32_t> x )
{
	TEST_ASSERT( x.v0 == -3346477.25f ); TEST_ASSERT( x.v1 == 0x3AC6DE1Fu );
}
void Pass_tup_f32_i64_Test0( const Tuple2<float, int64_t> x )
{
	TEST_ASSERT( x.v0 == 5366.5f ); TEST_ASSERT( x.v1 == 6336747347783754868 );
}
void Pass_tup_f32_i64_Test1( const Tuple2<float, int64_t> x )
{
	TEST_ASSERT( x.v0 == -0.0625f ); TEST_ASSERT( x.v1 == -642476347823222 );
}
#ifdef ENABLE_128BIT_INT_TESTS
void Pass_tup_f32_u128_Test0( const Tuple2<float, __uint128_t> x )
{
	TEST_ASSERT( x.v0 == -3366.75f );
	if( true ) return; // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	TEST_ASSERT( x.v1 == ( ( __uint128_t(0x0123456789ABCDEFull) << 64 ) | 0xFEDCBA9876543210ull ) );
}
void Pass_tup_f32_u128_Test1( const Tuple2<float, __uint128_t> x )
{
	TEST_ASSERT( x.v0 == 0.125f )
	if( true ) return; // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	TEST_ASSERT( x.v1 == ( ( __uint128_t(0xFEDCBA9876543210ull) << 64 ) | 0x0123456789ABCDEFull ) );
}
#endif
void Pass_tup_f32_f32_Test0( const Tuple2<float, float> x )
{
	TEST_ASSERT( x.v0 == 444666.0f ); TEST_ASSERT( x.v1 == 6763.5f );
}
void Pass_tup_f32_f32_Test1( const Tuple2<float, float> x )
{
	TEST_ASSERT( x.v0 == -15215.2f ); TEST_ASSERT( x.v1 == -6346470.0f );
}
void Pass_tup_f32_f64_Test0( const Tuple2<float, double> x )
{
	TEST_ASSERT( x.v0 == 634663660000.0f ); TEST_ASSERT( x.v1 == -67163.25 );
}
void Pass_tup_f32_f64_Test1( const Tuple2<float, double> x )
{
	TEST_ASSERT( x.v0 == -333636.5f ); TEST_ASSERT( x.v1 == 251.0 );
}
void Pass_tup_f64_i8_Test0( const Tuple2<double, int8_t> x )
{
	TEST_ASSERT( x.v0 == 44.25 ); TEST_ASSERT( x.v1 == 114 );
}
void Pass_tup_f64_i8_Test1( const Tuple2<double, int8_t> x )
{
	TEST_ASSERT( x.v0 == -3615.2 ); TEST_ASSERT( x.v1 == -13 );
}
void Pass_tup_f64_u16_Test0( const Tuple2<double, uint16_t> x )
{
	TEST_ASSERT( x.v0 == 66547.0 ); TEST_ASSERT( x.v1 == 31000 );
}
void Pass_tup_f64_u16_Test1( const Tuple2<double, uint16_t> x )
{
	TEST_ASSERT( x.v0 == 0.02 ); TEST_ASSERT( x.v1 == 57823 );
}
void Pass_tup_f64_i32_Test0( const Tuple2<double, int32_t> x )
{
	TEST_ASSERT( x.v0 == 54647.25 ); TEST_ASSERT( x.v1 == -533167754 );
}
void Pass_tup_f64_i32_Test1( const Tuple2<double, int32_t> x )
{
	TEST_ASSERT( x.v0 == -0.5 ); TEST_ASSERT( x.v1 == 336637444 );
}
void Pass_tup_f64_u64_Test0( const Tuple2<double, uint64_t> x )
{
	TEST_ASSERT( x.v0 == 447.2 ); TEST_ASSERT( x.v1 == 0x08192A3B4C5D6E7Full );
}
void Pass_tup_f64_u64_Test1( const Tuple2<double, uint64_t> x )
{
	TEST_ASSERT( x.v0 == 7372483800000.0 ); TEST_ASSERT( x.v1 == 0xF7E6D5C4B3A29180ull );
}
#ifdef ENABLE_128BIT_INT_TESTS
void Pass_tup_f64_i128_Test0( const Tuple2<double, __int128_t> x )
{
	TEST_ASSERT( x.v0 == -5362370.5 );
	if( true ) return; // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	TEST_ASSERT( x.v1 == ( ( __int128_t(0x0123456789ABCDEFll) << 64 ) | 0x7EDCBA9876543210ll ) );
}
void Pass_tup_f64_i128_Test1( const Tuple2<double, __int128_t> x )
{
	TEST_ASSERT( x.v0 == -37485856855542.0 )
	if( true ) return; // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	TEST_ASSERT( x.v1 == ( ( __int128_t(0x1EDCBA9876543210ll) << 64 ) | 0x0123456789ABCDEFll ) );
}
#endif
void Pass_tup_f64_f32_Test0( const Tuple2<double, float> x )
{
	TEST_ASSERT( x.v0 == 2632647.0 ); TEST_ASSERT( x.v1 == 6263.5f );
}
void Pass_tup_f64_f32_Test1( const Tuple2<double, float> x )
{
	TEST_ASSERT( x.v0 == 11.125 ); TEST_ASSERT( x.v1 == -6356470.0f );
}
void Pass_tup_f64_f64_Test0( const Tuple2<double, double> x )
{
	TEST_ASSERT( x.v0 == -1.75 ); TEST_ASSERT( x.v1 == -37163.125 );
}
void Pass_tup_f64_f64_Test1( const Tuple2<double, double> x )
{
	TEST_ASSERT( x.v0 == 0.0 ); TEST_ASSERT( x.v1 == 253.0 );
}
void Pass_tup_u32_u16_u8_Test0( const Tuple3<uint32_t, uint16_t, uint8_t> x )
{
	TEST_ASSERT( x.v0 == 0x01234567u ); TEST_ASSERT( x.v1 == 0x89ABu ); TEST_ASSERT( x.v2 == 0xCDu );
}
void Pass_tup_u32_u16_u16_Test0( const Tuple3<uint32_t, uint16_t, uint16_t> x )
{
	TEST_ASSERT( x.v0 == 0x01234567u ); TEST_ASSERT( x.v1 == 0x89ABu ); TEST_ASSERT( x.v2 == 0xCDEFu );
}
void Pass_tup_u8_u16_u32_Test0( const Tuple3<uint8_t, uint16_t, uint32_t> x )
{
	TEST_ASSERT( x.v0 == 0x01u ); TEST_ASSERT( x.v1 == 0x2345u ); TEST_ASSERT( x.v2 == 0x6789ABCDu );
}
void Pass_tup_u16_u16_u32_Test0( const Tuple3<uint16_t, uint16_t, uint32_t> x )
{
	TEST_ASSERT( x.v0 == 0x0123u ); TEST_ASSERT( x.v1 == 0x4567u ); TEST_ASSERT( x.v2 == 0x89ABCDEFu );
}
void Pass_tup_u64_u32_u16_u8_Test0( const Tuple4<uint64_t, uint32_t, uint16_t, uint8_t> x )
{
	TEST_ASSERT( x.v0 == 0xFEDCBA9876543210ull ); TEST_ASSERT( x.v1 == 0x01234567u ); TEST_ASSERT( x.v2 == 0x89ABu ); TEST_ASSERT( x.v3 == 0xCDu );
}
void Pass_tup_u64_u32_u16_u16_Test0( const Tuple4<uint64_t, uint32_t, uint16_t, uint16_t> x )
{
	TEST_ASSERT( x.v0 == 0xFEDCBA9876543210ull ); TEST_ASSERT( x.v1 == 0x01234567u ); TEST_ASSERT( x.v2 == 0x89ABu ); TEST_ASSERT( x.v3 == 0xCDEFu );
}
void Pass_tup_u8_u16_u32_u64_Test0( const Tuple4<uint8_t, uint16_t, uint32_t, uint64_t> x )
{
	TEST_ASSERT( x.v0 == 0x01u ); TEST_ASSERT( x.v1 == 0x2345u ); TEST_ASSERT( x.v2 == 0x6789ABCDu ); TEST_ASSERT( x.v3 == 0xFEDCBA9876543210ull );
}
void Pass_tup_u16_u16_u32_u64_Test0( const Tuple4<uint16_t, uint16_t, uint32_t, uint64_t> x )
{
	TEST_ASSERT( x.v0 == 0x0123u ); TEST_ASSERT( x.v1 == 0x4567u ); TEST_ASSERT( x.v2 == 0x89ABCDEFu ); TEST_ASSERT( x.v3 == 0xFEDCBA9876543210ull );
}
void Pass_tup_u8_u16_u8_Test0( const Tuple3<uint8_t, uint16_t, uint8_t> x )
{
	TEST_ASSERT( x.v0 == 0xFEu ); TEST_ASSERT( x.v1 == 0xDCBAu ); TEST_ASSERT( x.v2 == 0x98u );
}
void Pass_tup_u8_u32_u8_Test0( const Tuple3<uint8_t, uint32_t, uint8_t> x )
{
	TEST_ASSERT( x.v0 == 0xFEu ); TEST_ASSERT( x.v1 == 0xDCBA9876u ); TEST_ASSERT( x.v2 == 0x54u );
}
void Pass_tup_u8_u64_u8_Test0( const Tuple3<uint8_t, uint64_t, uint8_t> x )
{
	TEST_ASSERT( x.v0 == 0xABu ); TEST_ASSERT( x.v1 == 0x0123456789ABCDEFull ); TEST_ASSERT( x.v2 == 0x12u );
}
void Pass_tup_u16_u32_u16_Test0( const Tuple3<uint16_t, uint32_t, uint16_t> x )
{
	TEST_ASSERT( x.v0 == 0x0123u ); TEST_ASSERT( x.v1 == 0x456789ABu ); TEST_ASSERT( x.v2 == 0xCDEFu );
}
void Pass_tup_u16_u64_u16_Test0( const Tuple3<uint16_t, uint64_t, uint16_t> x )
{
	TEST_ASSERT( x.v0 == 0xFEDCu ); TEST_ASSERT( x.v1 == 0x17283A4B5C6D7E8Full ); TEST_ASSERT( x.v2 == 0x9876ull );
}
void Pass_tup_u32_u64_u32_Test0( const Tuple3<uint32_t, uint64_t, uint32_t> x )
{
	TEST_ASSERT( x.v0 == 0x01234567u ); TEST_ASSERT( x.v1 == 0x17283A4B5C6D7E8Full ); TEST_ASSERT( x.v2 == 0x89ABCEDFu );
}
void Pass_tup_f32_i32_i32_Test0( const Tuple3<float, int32_t, int32_t> x )
{
	TEST_ASSERT( x.v0 == 123.45f ); TEST_ASSERT( x.v1 == 266747477 ); TEST_ASSERT( x.v2 == -963237321 );
}
void Pass_tup_i32_f32_i32_Test0( const Tuple3<int32_t, float, int32_t> x )
{
	TEST_ASSERT( x.v0 == -196323732 ); TEST_ASSERT( x.v1 == 236.5f ); TEST_ASSERT( x.v2 == 266745477 );
}
void Pass_tup_i32_i32_f32_Test0( const Tuple3<int32_t, int32_t, float> x )
{
	TEST_ASSERT( x.v0 == 196323735 ); TEST_ASSERT( x.v1 == 166745427 ); TEST_ASSERT( x.v2 == -0.7f );
}
void Pass_tup_f32_u64_u64_Test0( const Tuple3<float, uint64_t, uint64_t> x )
{
	TEST_ASSERT( x.v0 == 323.25f ); TEST_ASSERT( x.v1 == 0x64AB3C5482367DE3ull ); TEST_ASSERT( x.v2 == 0x17283A4B5C6D7E8Full );
}
void Pass_tup_u64_f32_u64_Test0( const Tuple3<uint64_t, float, uint64_t> x )
{
	TEST_ASSERT( x.v0 == 0x7637347A36B4E218u ); TEST_ASSERT( x.v1 == 1336.5f ); TEST_ASSERT( x.v2 == 0x067374735AE7DFC13u );
}
void Pass_tup_u64_u64_f32_Test0( const Tuple3<uint64_t, uint64_t, float> x )
{
	TEST_ASSERT( x.v0 == 0x27283A4B5C637E8Fu ); TEST_ASSERT( x.v1 == 0xE637347436B47218u ); TEST_ASSERT( x.v2 == 4.7f );
}
void Pass_tup_f64_i32_i32_Test0( const Tuple3<double, int32_t, int32_t> x )
{
	TEST_ASSERT( x.v0 == 123.45 ); TEST_ASSERT( x.v1 == 266747477 ); TEST_ASSERT( x.v2 == -963237321 );
}
void Pass_tup_i32_f64_i32_Test0( const Tuple3<int32_t, double, int32_t> x )
{
	TEST_ASSERT( x.v0 == -196323732 ); TEST_ASSERT( x.v1 == 236.5 ); TEST_ASSERT( x.v2 == 266745477 );
}
void Pass_tup_i32_i32_f64_Test0( const Tuple3<int32_t, int32_t, double> x )
{
	TEST_ASSERT( x.v0 == 196323735 ); TEST_ASSERT( x.v1 == 166745427 ); TEST_ASSERT( x.v2 == -0.7 );
}
void Pass_tup_f64_u64_u64_Test0( const Tuple3<double, uint64_t, uint64_t> x )
{
	TEST_ASSERT( x.v0 == 323.25 ); TEST_ASSERT( x.v1 == 0x64AB3C5482367DE3ull ); TEST_ASSERT( x.v2 == 0x17283A4B5C6D7E8Full );
}
void Pass_tup_u64_f64_u64_Test0( const Tuple3<uint64_t, double, uint64_t> x )
{
	TEST_ASSERT( x.v0 == 0x7637347A36B4E218u ); TEST_ASSERT( x.v1 == 1336.5 ); TEST_ASSERT( x.v2 == 0x067374735AE7DFC13u );
}
void Pass_tup_u64_u64_f64_Test0( const Tuple3<uint64_t, uint64_t, double> x )
{
	TEST_ASSERT( x.v0 == 0x27283A4B5C637E8Fu ); TEST_ASSERT( x.v1 == 0xE637347436B47218u ); TEST_ASSERT( x.v2 == 4.7 );
}
void Pass_tup_f32_f32_f32_Test0( const Tuple3<float, float, float> x )
{
	TEST_ASSERT( x.v0 == 0.25f ); TEST_ASSERT( x.v1 == -363.2f ); TEST_ASSERT( x.v2 == 3773440.0f );
}
void Pass_tup_f32_f32_f64_Test0( const Tuple3<float, float, double> x )
{
	TEST_ASSERT( x.v0 == 0.25f ); TEST_ASSERT( x.v1 == -363.2f ); TEST_ASSERT( x.v2 == 3773440.0 );
}
void Pass_tup_f32_f64_f32_Test0( const Tuple3<float, double, float> x )
{
	TEST_ASSERT( x.v0 == 0.25f ); TEST_ASSERT( x.v1 == -363.2 ); TEST_ASSERT( x.v2 == 3773440.0f );
}
void Pass_tup_f32_f64_f64_Test0( const Tuple3<float, double, double> x )
{
	TEST_ASSERT( x.v0 == 0.25f ); TEST_ASSERT( x.v1 == -363.2 ); TEST_ASSERT( x.v2 == 3773440.0 );
}
void Pass_tup_f64_f32_f32_Test0( const Tuple3<double, float, float> x )
{
	TEST_ASSERT( x.v0 == 0.25 ); TEST_ASSERT( x.v1 == -363.2f ); TEST_ASSERT( x.v2 == 3773440.0f );
}
void Pass_tup_f64_f32_f64_Test0( const Tuple3<double, float, double> x )
{
	TEST_ASSERT( x.v0 == 0.25 ); TEST_ASSERT( x.v1 == -363.2f ); TEST_ASSERT( x.v2 == 3773440.0 );
}
void Pass_tup_f64_f64_f32_Test0( const Tuple3<double, double, float> x )
{
	TEST_ASSERT( x.v0 == 0.25 ); TEST_ASSERT( x.v1 == -363.2 ); TEST_ASSERT( x.v2 == 3773440.0f );
}
void Pass_tup_f64_f64_f64_Test0( const Tuple3<double, double, double> x )
{
	TEST_ASSERT( x.v0 == 0.25 ); TEST_ASSERT( x.v1 == -363.2 ); TEST_ASSERT( x.v2 == 3773440.0 );
}
void Pass_u32_u32_u32_u32_u32_tup_u64_f64( const uint32_t a, const uint32_t b, const uint32_t c, const uint32_t d, const uint32_t e, const Tuple2<uint64_t, double> f )
{
	TEST_ASSERT( a == 47588u ); TEST_ASSERT( b == 33677u ); TEST_ASSERT( c == 12u ); TEST_ASSERT( d == 3785427u ); TEST_ASSERT( e == 13748588u );
	TEST_ASSERT( f.v0 == 0x0123456789ABCDEFull ); TEST_ASSERT( f.v1 == 26376.25 );
}
#ifdef ENABLE_128BIT_INT_TESTS
void Pass_u128_u32_u32_u32_u32_tup_u64_f64( const __uint128_t a, const uint32_t b, const uint32_t c, const uint32_t d, const uint32_t e, const Tuple2<uint64_t, double> f )
{
	TEST_ASSERT( a == ( ( __uint128_t( 0x1122334455667788ull ) << 64u ) | 0x99AABBCCDDEEFFull ) );
	TEST_ASSERT( b == 33647u );
	TEST_ASSERT( c == 13u );
	TEST_ASSERT( d == 3785437u );
	TEST_ASSERT( e == 53748583u );
	TEST_ASSERT( f.v0 == 0x012D456789ABC3EFull ); TEST_ASSERT( f.v1 == -26336.75 );
}
#endif
void Pass_u32_u32_u32_u32_u32_tup_u64_u64( const uint32_t a, const uint32_t b, const uint32_t c, const uint32_t d, const uint32_t e, const Tuple2<uint64_t, uint64_t> f )
{
	TEST_ASSERT( a == 47288u ); TEST_ASSERT( b == 31677u ); TEST_ASSERT( c == 14u ); TEST_ASSERT( d == 3285427u ); TEST_ASSERT( e == 13748988u );
	TEST_ASSERT( f.v0 == 0x0123456789ABCDEFu ); TEST_ASSERT( f.v1 == 0xFEDCBA9876543210u );
}
void Pass_u32_u32_u32_u32_u32_u32_tup_u64_f64( const uint32_t a, const uint32_t b, const uint32_t c, const uint32_t d, const uint32_t e, const uint32_t f, const Tuple2<uint64_t, double> g )
{
	TEST_ASSERT( a == 41588u ); TEST_ASSERT( b == 633677u ); TEST_ASSERT( c == 7812u ); TEST_ASSERT( d == 5785427u ); TEST_ASSERT( e == 23748588u ); TEST_ASSERT( f == 788588u );
	TEST_ASSERT( g.v0 == 0xFEDCBA9876543210ull ); TEST_ASSERT( g.v1 == -16376.75 );
}
void Pass_f64_f64_f64_f64_f64_f64_f64_f64_tup_u64_f64( const double a, const double b, const double c, const double d, const double e, const double f, const double g, const double h, const Tuple2<uint64_t, double> i )
{
	TEST_ASSERT( a == 1.0 ); TEST_ASSERT( b == 774.3 ); TEST_ASSERT( c == -366.0 ); TEST_ASSERT( d == 0.125 );
	TEST_ASSERT( e == 6336.2 ); TEST_ASSERT( f == 6774.0 ); TEST_ASSERT( g == -126.25 ); TEST_ASSERT( h == 0.75 );
	TEST_ASSERT( i.v0 == 0xFED7BA98C6543210ull ); TEST_ASSERT( i.v1 == 163.2 );
}
void Pass_f64_f64_f64_f64_f64_f64_f64_tup_f64_f64( const double a, const double b, const double c, const double d, const double e, const double f, const double g, const Tuple2<double, double> h )
{
	TEST_ASSERT( a == 3.0 ); TEST_ASSERT( b == 724.1 ); TEST_ASSERT( c == -365.0 ); TEST_ASSERT( d == -0.125 );
	TEST_ASSERT( e == 6336.2 ); TEST_ASSERT( f == 6724.0 ); TEST_ASSERT( g == -126.85 );
	TEST_ASSERT( h.v0 == 631.3 ); TEST_ASSERT( h.v1 == 165.2 );
}

void U_Pass_bool_Test0( bool x );
void U_Pass_bool_Test1( bool x );
void U_Pass_i8_Test0( int8_t x );
void U_Pass_i8_Test1( int8_t x );
void U_Pass_i8_Test2( int8_t x0, int8_t x1, int8_t x2, int8_t x3, int8_t x4, int8_t x5, int8_t x6, int8_t x7, int8_t x8, int8_t x9, int8_t xa, int8_t xb, int8_t xc, int8_t xd, int8_t xe, int8_t xf );
void U_Pass_u8_Test0( uint8_t x );
void U_Pass_u8_Test1( uint8_t x );
void U_Pass_u8_Test2( uint8_t x );
void U_Pass_i16_Test0( int16_t x );
void U_Pass_i16_Test1( int16_t x );
void U_Pass_u16_Test0( uint16_t x );
void U_Pass_u16_Test1( uint16_t x );
void U_Pass_u16_Test2( uint16_t x );
void U_Pass_u16_Test3( uint16_t x );
void U_Pass_u16_Test4( uint16_t x0, uint16_t x1, uint16_t x2, uint16_t x3, uint16_t x4, uint16_t x5, uint16_t x6, uint16_t x7, uint16_t x8, uint16_t x9, uint16_t xa, uint16_t xb, uint16_t xc, uint16_t xd, uint16_t xe, uint16_t xf );
void U_Pass_i32_Test0( int32_t x );
void U_Pass_i32_Test1( int32_t x );
void U_Pass_i32_Test2( int32_t x );
void U_Pass_i32_Test3( int32_t x );
void U_Pass_i32_Test4( int32_t x0, int32_t x1, int32_t x2, int32_t x3, int32_t x4, int32_t x5, int32_t x6, int32_t x7, int32_t x8, int32_t x9, int32_t xa, int32_t xb, int32_t xc, int32_t xd, int32_t xe, int32_t xf );
void U_Pass_u32_Test0( uint32_t x );
void U_Pass_u32_Test1( uint32_t x );
void U_Pass_u32_Test2( uint32_t x );
void U_Pass_u32_Test3( uint32_t x );
void U_Pass_i64_Test0( int64_t x );
void U_Pass_i64_Test1( int64_t x );
void U_Pass_i64_Test2( int64_t x );
void U_Pass_i64_Test3( int64_t x );
void U_Pass_u64_Test0( uint64_t x );
void U_Pass_u64_Test1( uint64_t x );
void U_Pass_u64_Test2( uint64_t x );
void U_Pass_u64_Test3( uint64_t x );
void U_Pass_u64_Test4( uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4, uint64_t x5, uint64_t x6, uint64_t x7, uint64_t x8, uint64_t x9, uint64_t xa, uint64_t xb, uint64_t xc, uint64_t xd, uint64_t xe, uint64_t xf );
#ifdef ENABLE_128BIT_INT_TESTS
void U_Pass_i128_Test0( __int128_t x );
void U_Pass_u128_Test0( __uint128_t x );
#endif
// Use "unsigned char" for represent Ü "char8", since in C++ regular char signess is inplementation defined.
void U_Pass_char8_Test0( unsigned char x );
void U_Pass_char8_Test1( unsigned char x );
void U_Pass_char8_Test2( unsigned char x );
void U_Pass_char8_Test3( unsigned char x );
void U_Pass_char16_Test0( char16_t x );
void U_Pass_char16_Test1( char16_t x );
void U_Pass_char32_Test0( char32_t x );
void U_Pass_f32_Test0( float x );
void U_Pass_f32_Test1( float x );
void U_Pass_f32_Test2( float x );
void U_Pass_f32_Test3( float x );
void U_Pass_f32_Test4( float x );
void U_Pass_f32_Test5( float x );
void U_Pass_f32_Test6( float x0, float x1, float x2, float x3, float x4, float x5, float x6, float x7, float x8, float x9, float xa, float xb, float xc, float xd, float xe, float xf );
void U_Pass_f64_Test0( double x );
void U_Pass_f64_Test1( double x );
void U_Pass_f64_Test2( double x );
void U_Pass_f64_Test3( double x );
void U_Pass_f64_Test4( double x );
void U_Pass_f64_Test5( double x );
void U_Pass_f64_Test6( double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7, double x8, double x9, double xa, double xb, double xc, double xd, double xe, double xf );
void U_Pass_u8_x1_Test0( std::array<uint8_t, 1> x );
void U_Pass_u8_x2_Test0( std::array<uint8_t, 2> x );
void U_Pass_u8_x3_Test0( std::array<uint8_t, 3> x );
void U_Pass_u8_x4_Test0( std::array<uint8_t, 4> x );
void U_Pass_u8_x5_Test0( std::array<uint8_t, 5> x );
void U_Pass_u8_x6_Test0( std::array<uint8_t, 6> x );
void U_Pass_u8_x7_Test0( std::array<uint8_t, 7> x );
void U_Pass_u8_x8_Test0( std::array<uint8_t, 8> x );
void U_Pass_u8_x9_Test0( std::array<uint8_t, 9> x );
void U_Pass_u8_x10_Test0( std::array<uint8_t, 10> x );
void U_Pass_u8_x11_Test0( std::array<uint8_t, 11> x );
void U_Pass_u8_x12_Test0( std::array<uint8_t, 12> x );
void U_Pass_u8_x13_Test0( std::array<uint8_t, 13> x );
void U_Pass_u8_x14_Test0( std::array<uint8_t, 14> x );
void U_Pass_u8_x15_Test0( std::array<uint8_t, 15> x );
void U_Pass_u8_x16_Test0( std::array<uint8_t, 16> x );
void U_Pass_u8_x17_Test0( std::array<uint8_t, 17> x );
void U_Pass_u8_x29_Test0( std::array<uint8_t, 29> x );
void U_Pass_u8_x371_Test0( std::array<uint8_t, 371> x );
void U_Pass_i16_x1_Test0( std::array<int16_t, 1> x );
void U_Pass_i16_x2_Test0( std::array<int16_t, 2> x );
void U_Pass_i16_x3_Test0( std::array<int16_t, 3> x );
void U_Pass_i16_x4_Test0( std::array<int16_t, 4> x );
void U_Pass_i16_x5_Test0( std::array<int16_t, 5> x );
void U_Pass_i16_x6_Test0( std::array<int16_t, 6> x );
void U_Pass_i16_x7_Test0( std::array<int16_t, 7> x );
void U_Pass_i16_x8_Test0( std::array<int16_t, 8> x );
void U_Pass_i16_x9_Test0( std::array<int16_t, 9> x );
void U_Pass_i16_x15_Test0( std::array<int16_t, 15> x );
void U_Pass_i16_x83_Test0( std::array<int16_t, 83> x );
void U_Pass_u32_x1_Test0( std::array<uint32_t, 1> x );
void U_Pass_u32_x2_Test0( std::array<uint32_t, 2> x );
void U_Pass_u32_x3_Test0( std::array<uint32_t, 3> x );
void U_Pass_u32_x4_Test0( std::array<uint32_t, 4> x );
void U_Pass_u32_x5_Test0( std::array<uint32_t, 5> x );
void U_Pass_u32_x6_Test0( std::array<uint32_t, 6> x );
void U_Pass_u32_x7_Test0( std::array<uint32_t, 7> x );
void U_Pass_u32_x8_Test0( std::array<uint32_t, 8> x );
void U_Pass_u32_x9_Test0( std::array<uint32_t, 9> x );
void U_Pass_u32_x17_Test0( std::array<uint32_t, 17> x );
void U_Pass_u64_x1_Test0( std::array<uint64_t, 1> x );
void U_Pass_u64_x2_Test0( std::array<uint64_t, 2> x );
void U_Pass_u64_x3_Test0( std::array<uint64_t, 3> x );
void U_Pass_u64_x4_Test0( std::array<uint64_t, 4> x );
void U_Pass_u64_x5_Test0( std::array<uint64_t, 5> x );
void U_Pass_u64_x11_Test0( std::array<uint64_t, 11> x );
#ifdef ENABLE_128BIT_INT_TESTS
void U_Pass_u128_x1_Test0( std::array<__uint128_t, 1> x );
void U_Pass_u128_x2_Test0( std::array<__uint128_t, 2> x );
void U_Pass_u128_x3_Test0( std::array<__uint128_t, 3> x );
#endif
void U_Pass_f32_x1_Test0( std::array<float, 1> x );
void U_Pass_f32_x2_Test0( std::array<float, 2> x );
void U_Pass_f32_x3_Test0( std::array<float, 3> x );
void U_Pass_f32_x4_Test0( std::array<float, 4> x );
void U_Pass_f32_x5_Test0( std::array<float, 5> x );
void U_Pass_f32_x6_Test0( std::array<float, 6> x );
void U_Pass_f32_x7_Test0( std::array<float, 7> x );
void U_Pass_f32_x8_Test0( std::array<float, 8> x );
void U_Pass_f32_x9_Test0( std::array<float, 9> x );
void U_Pass_f32_x15_Test0( std::array<float, 15> x );
void U_Pass_f32_x47_Test0( std::array<float, 47> x );
void U_Pass_f64_x1_Test0( std::array<double, 1> x );
void U_Pass_f64_x2_Test0( std::array<double, 2> x );
void U_Pass_f64_x3_Test0( std::array<double, 3> x );
void U_Pass_f64_x4_Test0( std::array<double, 4> x );
void U_Pass_f64_x5_Test0( std::array<double, 5> x );
void U_Pass_f64_x6_Test0( std::array<double, 6> x );
void U_Pass_f64_x7_Test0( std::array<double, 7> x );
void U_Pass_f64_x8_Test0( std::array<double, 8> x );
void U_Pass_f64_x9_Test0( std::array<double, 9> x );
void U_Pass_f64_x15_Test0( std::array<double, 15> x );
void U_Pass_f64_x47_Test0( std::array<double, 47> x );
void U_Pass_char8_x1_Test0( std::array<char, 1> x );
void U_Pass_char8_x2_Test0( std::array<char, 2> x );
void U_Pass_char8_x3_Test0( std::array<char, 3> x );
void U_Pass_char8_x4_Test0( std::array<char, 4> x );
void U_Pass_char8_x5_Test0( std::array<char, 5> x );
void U_Pass_char8_x6_Test0( std::array<char, 6> x );
void U_Pass_char8_x7_Test0( std::array<char, 7> x );
void U_Pass_char8_x8_Test0( std::array<char, 8> x );
void U_Pass_char8_x9_Test0( std::array<char, 9> x );
void U_Pass_char8_x10_Test0( std::array<char, 10> x );
void U_Pass_char8_x11_Test0( std::array<char, 11> x );
void U_Pass_char8_x12_Test0( std::array<char, 12> x );
void U_Pass_char8_x13_Test0( std::array<char, 13> x );
void U_Pass_char8_x14_Test0( std::array<char, 14> x );
void U_Pass_char8_x15_Test0( std::array<char, 15> x );
void U_Pass_char8_x16_Test0( std::array<char, 16> x );
void U_Pass_char8_x17_Test0( std::array<char, 17> x );
void U_Pass_char8_x32_Test0( std::array<char, 32> x );
void U_Pass_char8_x39_Test0( std::array<char, 39> x );
void U_Pass_tup_i8_u8_Test0( Tuple2<int8_t, uint8_t> x );
void U_Pass_tup_i8_u8_Test1( Tuple2<int8_t, uint8_t> x );
void U_Pass_tup_i8_i16_Test0( Tuple2<int8_t, int16_t> x );
void U_Pass_tup_i8_i16_Test1( Tuple2<int8_t, int16_t> x );
void U_Pass_tup_i8_u32_Test0( Tuple2<int8_t, uint32_t> x );
void U_Pass_tup_i8_u32_Test1( Tuple2<int8_t, uint32_t> x );
void U_Pass_tup_i8_i64_Test0( Tuple2<int8_t, int64_t> x );
void U_Pass_tup_i8_i64_Test1( Tuple2<int8_t, int64_t> x );
#ifdef ENABLE_128BIT_INT_TESTS
void U_Pass_tup_i8_u128_Test0( Tuple2<int8_t, __uint128_t> x );
void U_Pass_tup_i8_u128_Test1( Tuple2<int8_t, __uint128_t> x );
#endif
void U_Pass_tup_i8_f32_Test0( Tuple2<int8_t, float> x );
void U_Pass_tup_i8_f32_Test1( Tuple2<int8_t, float> x );
void U_Pass_tup_i8_f64_Test0( Tuple2<int8_t, double> x );
void U_Pass_tup_i8_f64_Test1( Tuple2<int8_t, double> x );
void U_Pass_tup_u16_i8_Test0( Tuple2<uint16_t, int8_t> x );
void U_Pass_tup_u16_i8_Test1( Tuple2<uint16_t, int8_t> x );
void U_Pass_tup_u16_u16_Test0( Tuple2<uint16_t, uint16_t> x );
void U_Pass_tup_u16_u16_Test1( Tuple2<uint16_t, uint16_t> x );
void U_Pass_tup_u16_i32_Test0( Tuple2<uint16_t, int32_t> x );
void U_Pass_tup_u16_i32_Test1( Tuple2<uint16_t, int32_t> x );
void U_Pass_tup_u16_u64_Test0( Tuple2<uint16_t, uint64_t> x );
void U_Pass_tup_u16_u64_Test1( Tuple2<uint16_t, uint64_t> x );
#ifdef ENABLE_128BIT_INT_TESTS
void U_Pass_tup_u16_i128_Test0( Tuple2<uint16_t, __int128_t> x );
void U_Pass_tup_u16_i128_Test1( Tuple2<uint16_t, __int128_t> x );
#endif
void U_Pass_tup_u16_f32_Test0( Tuple2<uint16_t, float> x );
void U_Pass_tup_u16_f32_Test1( Tuple2<uint16_t, float> x );
void U_Pass_tup_u16_f64_Test0( Tuple2<uint16_t, double> x );
void U_Pass_tup_u16_f64_Test1( Tuple2<uint16_t, double> x );
void U_Pass_tup_i32_u8_Test0( Tuple2<int32_t, uint8_t> x );
void U_Pass_tup_i32_u8_Test1( Tuple2<int32_t, uint8_t> x );
void U_Pass_tup_i32_i16_Test0( Tuple2<int32_t, int16_t> x );
void U_Pass_tup_i32_i16_Test1( Tuple2<int32_t, int16_t> x );
void U_Pass_tup_i32_u32_Test0( Tuple2<int32_t, uint32_t> x );
void U_Pass_tup_i32_u32_Test1( Tuple2<int32_t, uint32_t> x );
void U_Pass_tup_i32_i64_Test0( Tuple2<int32_t, int64_t> x );
void U_Pass_tup_i32_i64_Test1( Tuple2<int32_t, int64_t> x );
#ifdef ENABLE_128BIT_INT_TESTS
void U_Pass_tup_i32_u128_Test0( Tuple2<int32_t, __uint128_t> x );
void U_Pass_tup_i32_u128_Test1( Tuple2<int32_t, __uint128_t> x );
#endif
void U_Pass_tup_i32_f32_Test0( Tuple2<int32_t, float> x );
void U_Pass_tup_i32_f32_Test1( Tuple2<int32_t, float> x );
void U_Pass_tup_i32_f64_Test0( Tuple2<int32_t, double> x );
void U_Pass_tup_i32_f64_Test1( Tuple2<int32_t, double> x );
void U_Pass_tup_u64_i8_Test0( Tuple2<uint64_t, int8_t> x );
void U_Pass_tup_u64_i8_Test1( Tuple2<uint64_t, int8_t> x );
void U_Pass_tup_u64_u16_Test0( Tuple2<uint64_t, uint16_t> x );
void U_Pass_tup_u64_u16_Test1( Tuple2<uint64_t, uint16_t> x );
void U_Pass_tup_u64_i32_Test0( Tuple2<uint64_t, int32_t> x );
void U_Pass_tup_u64_i32_Test1( Tuple2<uint64_t, int32_t> x );
void U_Pass_tup_u64_u64_Test0( Tuple2<uint64_t, uint64_t> x );
void U_Pass_tup_u64_u64_Test1( Tuple2<uint64_t, uint64_t> x );
#ifdef ENABLE_128BIT_INT_TESTS
void U_Pass_tup_u64_i128_Test0( Tuple2<uint64_t, __int128_t> x );
void U_Pass_tup_u64_i128_Test1( Tuple2<uint64_t, __int128_t> x );
#endif
void U_Pass_tup_u64_f32_Test0( Tuple2<uint64_t, float> x );
void U_Pass_tup_u64_f32_Test1( Tuple2<uint64_t, float> x );
void U_Pass_tup_u64_f64_Test0( Tuple2<uint64_t, double> x );
void U_Pass_tup_u64_f64_Test1( Tuple2<uint64_t, double> x );
void U_Pass_tup_f32_u8_Test0( Tuple2<float, uint8_t> x );
void U_Pass_tup_f32_u8_Test1( Tuple2<float, uint8_t> x );
void U_Pass_tup_f32_i16_Test0( Tuple2<float, int16_t> x );
void U_Pass_tup_f32_i16_Test1( Tuple2<float, int16_t> x );
void U_Pass_tup_f32_u32_Test0( Tuple2<float, uint32_t> x );
void U_Pass_tup_f32_u32_Test1( Tuple2<float, uint32_t> x );
void U_Pass_tup_f32_i64_Test0( Tuple2<float, int64_t> x );
void U_Pass_tup_f32_i64_Test1( Tuple2<float, int64_t> x );
#ifdef ENABLE_128BIT_INT_TESTS
void U_Pass_tup_f32_u128_Test0( Tuple2<float, __uint128_t> x );
void U_Pass_tup_f32_u128_Test1( Tuple2<float, __uint128_t> x );
#endif
void U_Pass_tup_f32_f32_Test0( Tuple2<float, float> x );
void U_Pass_tup_f32_f32_Test1( Tuple2<float, float> x );
void U_Pass_tup_f32_f64_Test0( Tuple2<float, double> x );
void U_Pass_tup_f32_f64_Test1( Tuple2<float, double> x );
void U_Pass_tup_f64_i8_Test0( Tuple2<double, int8_t> x );
void U_Pass_tup_f64_i8_Test1( Tuple2<double, int8_t> x );
void U_Pass_tup_f64_u16_Test0( Tuple2<double, uint16_t> x );
void U_Pass_tup_f64_u16_Test1( Tuple2<double, uint16_t> x );
void U_Pass_tup_f64_i32_Test0( Tuple2<double, int32_t> x );
void U_Pass_tup_f64_i32_Test1( Tuple2<double, int32_t> x );
void U_Pass_tup_f64_u64_Test0( Tuple2<double, uint64_t> x );
void U_Pass_tup_f64_u64_Test1( Tuple2<double, uint64_t> x );
#ifdef ENABLE_128BIT_INT_TESTS
void U_Pass_tup_f64_i128_Test0( Tuple2<double, __int128_t> x );
void U_Pass_tup_f64_i128_Test1( Tuple2<double, __int128_t> x );
#endif
void U_Pass_tup_f64_f32_Test0( Tuple2<double, float> x );
void U_Pass_tup_f64_f32_Test1( Tuple2<double, float> x );
void U_Pass_tup_f64_f64_Test0( Tuple2<double, double> x );
void U_Pass_tup_f64_f64_Test1( Tuple2<double, double> x );
void U_Pass_tup_u32_u16_u8_Test0( Tuple3<uint32_t, uint16_t, uint8_t> x );
void U_Pass_tup_u32_u16_u16_Test0( Tuple3<uint32_t, uint16_t, uint16_t> x );
void U_Pass_tup_u8_u16_u32_Test0( Tuple3<uint8_t, uint16_t, uint32_t> x );
void U_Pass_tup_u16_u16_u32_Test0( Tuple3<uint16_t, uint16_t, uint32_t> x );
void U_Pass_tup_u64_u32_u16_u8_Test0( Tuple4<uint64_t, uint32_t, uint16_t, uint8_t> x );
void U_Pass_tup_u64_u32_u16_u16_Test0( Tuple4<uint64_t, uint32_t, uint16_t, uint16_t> x );
void U_Pass_tup_u8_u16_u32_u64_Test0( Tuple4<uint8_t, uint16_t, uint32_t, uint64_t> x );
void U_Pass_tup_u16_u16_u32_u64_Test0( Tuple4<uint16_t, uint16_t, uint32_t, uint64_t> x );
void U_Pass_tup_u8_u16_u8_Test0( Tuple3<uint8_t, uint16_t, uint8_t> x );
void U_Pass_tup_u8_u32_u8_Test0( Tuple3<uint8_t, uint32_t, uint8_t> x );
void U_Pass_tup_u8_u64_u8_Test0( Tuple3<uint8_t, uint64_t, uint8_t> x );
void U_Pass_tup_u16_u32_u16_Test0( Tuple3<uint16_t, uint32_t, uint16_t> x );
void U_Pass_tup_u16_u64_u16_Test0( Tuple3<uint16_t, uint64_t, uint16_t> x );
void U_Pass_tup_u32_u64_u32_Test0( Tuple3<uint32_t, uint64_t, uint32_t> x );
void U_Pass_tup_f32_i32_i32_Test0( Tuple3<float, int32_t, int32_t> x );
void U_Pass_tup_i32_f32_i32_Test0( Tuple3<int32_t, float, int32_t> x );
void U_Pass_tup_i32_i32_f32_Test0( Tuple3<int32_t, int32_t, float> x );
void U_Pass_tup_f32_u64_u64_Test0( Tuple3<float, uint64_t, uint64_t> x );
void U_Pass_tup_u64_f32_u64_Test0( Tuple3<uint64_t, float, uint64_t> x );
void U_Pass_tup_u64_u64_f32_Test0( Tuple3<uint64_t, uint64_t, float> x );
void U_Pass_tup_f64_i32_i32_Test0( Tuple3<double, int32_t, int32_t> x );
void U_Pass_tup_i32_f64_i32_Test0( Tuple3<int32_t, double, int32_t> x );
void U_Pass_tup_i32_i32_f64_Test0( Tuple3<int32_t, int32_t, double> x );
void U_Pass_tup_f64_u64_u64_Test0( Tuple3<double, uint64_t, uint64_t> x );
void U_Pass_tup_u64_f64_u64_Test0( Tuple3<uint64_t, double, uint64_t> x );
void U_Pass_tup_u64_u64_f64_Test0( Tuple3<uint64_t, uint64_t, double> x );
void U_Pass_tup_f32_f32_f32_Test0( Tuple3<float, float, float> x );
void U_Pass_tup_f32_f32_f64_Test0( Tuple3<float, float, double> x );
void U_Pass_tup_f32_f64_f32_Test0( Tuple3<float, double, float> x );
void U_Pass_tup_f32_f64_f64_Test0( Tuple3<float, double, double> x );
void U_Pass_tup_f64_f32_f32_Test0( Tuple3<double, float, float> x );
void U_Pass_tup_f64_f32_f64_Test0( Tuple3<double, float, double> x );
void U_Pass_tup_f64_f64_f32_Test0( Tuple3<double, double, float> x );
void U_Pass_tup_f64_f64_f64_Test0( Tuple3<double, double, double> x );
// Tricky cases - x86_64 System V ABI uses 6 integer registers for passing integer values.
void U_Pass_u32_u32_u32_u32_u32_tup_u64_f64( uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, Tuple2<uint64_t, double> f );
#ifdef ENABLE_128BIT_INT_TESTS
void U_Pass_u128_u32_u32_u32_u32_tup_u64_f64( __uint128_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, Tuple2<uint64_t, double> f );
#endif
void U_Pass_u32_u32_u32_u32_u32_tup_u64_u64( uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, Tuple2<uint64_t, uint64_t> f );
void U_Pass_u32_u32_u32_u32_u32_u32_tup_u64_f64( uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, Tuple2<uint64_t, double> g );
// Tricky cases - x86_64 System V ABI uses 8 sse registers for passing floating-point values.
void U_Pass_f64_f64_f64_f64_f64_f64_f64_f64_tup_u64_f64( double a, double b, double c, double d, double e, double f, double g, double h, Tuple2<uint64_t, double> i );
void U_Pass_f64_f64_f64_f64_f64_f64_f64_tup_f64_f64( double a, double b, double c, double d, double e, double f, double g, Tuple2<double, double> h );

int8_t U_Get_i8_Test0();
int8_t U_Get_i8_Test1();
uint8_t U_Get_u8_Test0();
uint8_t U_Get_u8_Test1();
int16_t U_Get_i16_Test0();
int16_t U_Get_i16_Test1();
uint16_t U_Get_u16_Test0();
uint16_t U_Get_u16_Test1();
int32_t U_Get_i32_Test0();
int32_t U_Get_i32_Test1();
uint32_t U_Get_u32_Test0();
uint32_t U_Get_u32_Test1();
int64_t U_Get_i64_Test0();
int64_t U_Get_i64_Test1();
uint64_t U_Get_u64_Test0();
uint64_t U_Get_u64_Test1();
#ifdef ENABLE_128BIT_INT_TESTS
__int128_t U_Get_i128_Test0();
__uint128_t U_Get_u128_Test0();
#endif
// Use "unsigned char" for represent Ü "char8", since in C++ regular char signess is inplementation defined.
unsigned char U_Get_char8_Test0();
unsigned char U_Get_char8_Test1();
char16_t U_Get_char16_Test0();
char16_t U_Get_char16_Test1();
char32_t U_Get_char32_Test0();
float U_Get_f32_Test0();
float U_Get_f32_Test1();
double U_Get_f64_Test0();
double U_Get_f64_Test1();
std::array<int8_t, 1> U_Get_i8_x1_Test0();
std::array<int8_t, 2> U_Get_i8_x2_Test0();
std::array<int8_t, 3> U_Get_i8_x3_Test0();
std::array<int8_t, 4> U_Get_i8_x4_Test0();
std::array<int8_t, 5> U_Get_i8_x5_Test0();
std::array<int8_t, 6> U_Get_i8_x6_Test0();
std::array<int8_t, 7> U_Get_i8_x7_Test0();
std::array<int8_t, 8> U_Get_i8_x8_Test0();
std::array<int8_t, 9> U_Get_i8_x9_Test0();
std::array<int8_t, 10> U_Get_i8_x10_Test0();
std::array<int8_t, 11> U_Get_i8_x11_Test0();
std::array<int8_t, 12> U_Get_i8_x12_Test0();
std::array<int8_t, 13> U_Get_i8_x13_Test0();
std::array<int8_t, 14> U_Get_i8_x14_Test0();
std::array<int8_t, 15> U_Get_i8_x15_Test0();
std::array<int8_t, 16> U_Get_i8_x16_Test0();
std::array<int8_t, 17> U_Get_i8_x17_Test0();
std::array<int8_t, 35> U_Get_i8_x35_Test0();
std::array<uint16_t, 1> U_Get_u16_x1_Test0();
std::array<uint16_t, 2> U_Get_u16_x2_Test0();
std::array<uint16_t, 3> U_Get_u16_x3_Test0();
std::array<uint16_t, 4> U_Get_u16_x4_Test0();
std::array<uint16_t, 5> U_Get_u16_x5_Test0();
std::array<uint16_t, 6> U_Get_u16_x6_Test0();
std::array<uint16_t, 7> U_Get_u16_x7_Test0();
std::array<uint16_t, 8> U_Get_u16_x8_Test0();
std::array<uint16_t, 9> U_Get_u16_x9_Test0();
std::array<uint16_t, 15> U_Get_u16_x15_Test0();
std::array<uint16_t, 21> U_Get_u16_x21_Test0();
std::array<int32_t, 1> U_Get_i32_x1_Test0();
std::array<int32_t, 2> U_Get_i32_x2_Test0();
std::array<int32_t, 3> U_Get_i32_x3_Test0();
std::array<int32_t, 4> U_Get_i32_x4_Test0();
std::array<int32_t, 5> U_Get_i32_x5_Test0();
std::array<int32_t, 6> U_Get_i32_x6_Test0();
std::array<int32_t, 7> U_Get_i32_x7_Test0();
std::array<int32_t, 8> U_Get_i32_x8_Test0();
std::array<int32_t, 9> U_Get_i32_x9_Test0();
std::array<int32_t, 18> U_Get_i32_x18_Test0();
std::array<uint64_t, 1> U_Get_u64_x1_Test0();
std::array<uint64_t, 2> U_Get_u64_x2_Test0();
std::array<uint64_t, 3> U_Get_u64_x3_Test0();
std::array<uint64_t, 4> U_Get_u64_x4_Test0();
std::array<uint64_t, 5> U_Get_u64_x5_Test0();
#ifdef ENABLE_128BIT_INT_TESTS
std::array<__int128_t, 1> U_Get_i128_x1_Test0();
std::array<__int128_t, 2> U_Get_i128_x2_Test0();
std::array<__int128_t, 3> U_Get_i128_x3_Test0();
#endif
std::array<float, 1> U_Get_f32_x1_Test0();
std::array<float, 2> U_Get_f32_x2_Test0();
std::array<float, 3> U_Get_f32_x3_Test0();
std::array<float, 4> U_Get_f32_x4_Test0();
std::array<float, 5> U_Get_f32_x5_Test0();
std::array<float, 6> U_Get_f32_x6_Test0();
std::array<float, 7> U_Get_f32_x7_Test0();
std::array<float, 8> U_Get_f32_x8_Test0();
std::array<float, 9> U_Get_f32_x9_Test0();
std::array<float, 19> U_Get_f32_x19_Test0();
std::array<double, 1> U_Get_f64_x1_Test0();
std::array<double, 2> U_Get_f64_x2_Test0();
std::array<double, 3> U_Get_f64_x3_Test0();
std::array<double, 4> U_Get_f64_x4_Test0();
std::array<double, 5> U_Get_f64_x5_Test0();
std::array<char, 1> U_Get_char8_x1_Test0();
std::array<char, 2> U_Get_char8_x2_Test0();
std::array<char, 3> U_Get_char8_x3_Test0();
std::array<char, 4> U_Get_char8_x4_Test0();
std::array<char, 5> U_Get_char8_x5_Test0();
std::array<char, 6> U_Get_char8_x6_Test0();
std::array<char, 7> U_Get_char8_x7_Test0();
std::array<char, 8> U_Get_char8_x8_Test0();
std::array<char, 9> U_Get_char8_x9_Test0();
std::array<char, 10> U_Get_char8_x10_Test0();
std::array<char, 11> U_Get_char8_x11_Test0();
std::array<char, 12> U_Get_char8_x12_Test0();
std::array<char, 13> U_Get_char8_x13_Test0();
std::array<char, 14> U_Get_char8_x14_Test0();
std::array<char, 15> U_Get_char8_x15_Test0();
std::array<char, 16> U_Get_char8_x16_Test0();
std::array<char, 17> U_Get_char8_x17_Test0();
std::array<char, 32> U_Get_char8_x32_Test0();
std::array<char, 39> U_Get_char8_x39_Test0();
Tuple2<int8_t, int8_t> U_Get_tup_i8_i8_Test0();
Tuple2<int8_t, uint16_t> U_Get_tup_i8_u16_Test0();
Tuple2<int8_t, int32_t> U_Get_tup_i8_i32_Test0();
Tuple2<int8_t, uint64_t> U_Get_tup_i8_u64_Test0();
#ifdef ENABLE_128BIT_INT_TESTS
Tuple2<int8_t, __int128_t> U_Get_tup_i8_i128_Test0();
#endif
Tuple2<int8_t, float> U_Get_tup_i8_f32_Test0();
Tuple2<int8_t, double> U_Get_tup_i8_f64_Test0();
Tuple2<uint16_t, uint8_t> U_Get_tup_u16_u8_Test0();
Tuple2<uint16_t, int16_t> U_Get_tup_u16_i16_Test0();
Tuple2<uint16_t, uint32_t> U_Get_tup_u16_u32_Test0();
Tuple2<uint16_t, int64_t> U_Get_tup_u16_i64_Test0();
#ifdef ENABLE_128BIT_INT_TESTS
Tuple2<uint16_t, __uint128_t> U_Get_tup_u16_u128_Test0();
#endif
Tuple2<uint16_t, float> U_Get_tup_u16_f32_Test0();
Tuple2<uint16_t, double> U_Get_tup_u16_f64_Test0();
Tuple2<int32_t, int8_t> U_Get_tup_i32_i8_Test0();
Tuple2<int32_t, uint16_t> U_Get_tup_i32_u16_Test0();
Tuple2<int32_t, int32_t> U_Get_tup_i32_i32_Test0();
Tuple2<int32_t, uint64_t> U_Get_tup_i32_u64_Test0();
#ifdef ENABLE_128BIT_INT_TESTS
Tuple2<int32_t, __int128_t> U_Get_tup_i32_i128_Test0();
#endif
Tuple2<int32_t, float> U_Get_tup_i32_f32_Test0();
Tuple2<int32_t, double> U_Get_tup_i32_f64_Test0();
Tuple2<uint64_t, uint8_t> U_Get_tup_u64_u8_Test0();
Tuple2<uint64_t, int16_t> U_Get_tup_u64_i16_Test0();
Tuple2<uint64_t, uint32_t> U_Get_tup_u64_u32_Test0();
Tuple2<uint64_t, int64_t> U_Get_tup_u64_i64_Test0();
#ifdef ENABLE_128BIT_INT_TESTS
Tuple2<uint64_t, __uint128_t> U_Get_tup_u64_u128_Test0();
#endif
Tuple2<uint64_t, float> U_Get_tup_u64_f32_Test0();
Tuple2<uint64_t, double> U_Get_tup_u64_f64_Test0();
Tuple2<float, int8_t> U_Get_tup_f32_i8_Test0();
Tuple2<float, uint16_t> U_Get_tup_f32_u16_Test0();
Tuple2<float, int32_t> U_Get_tup_f32_i32_Test0();
Tuple2<float, uint64_t> U_Get_tup_f32_u64_Test0();
#ifdef ENABLE_128BIT_INT_TESTS
Tuple2<float, __int128_t> U_Get_tup_f32_i128_Test0();
#endif
Tuple2<float, float> U_Get_tup_f32_f32_Test0();
Tuple2<float, double> U_Get_tup_f32_f64_Test0();
Tuple2<double, uint8_t> U_Get_tup_f64_u8_Test0();
Tuple2<double, int16_t> U_Get_tup_f64_i16_Test0();
Tuple2<double, uint32_t> U_Get_tup_f64_u32_Test0();
Tuple2<double, int64_t> U_Get_tup_f64_i64_Test0();
#ifdef ENABLE_128BIT_INT_TESTS
Tuple2<double, __uint128_t> U_Get_tup_f64_u128_Test0();
#endif
Tuple2<double, float> U_Get_tup_f64_f32_Test0();
Tuple2<double, double> U_Get_tup_f64_f64_Test0();
Tuple3<uint32_t, uint16_t, uint8_t> U_Get_tup_u32_u16_u8_Test0();
Tuple3<uint32_t, uint16_t, uint16_t> U_Get_tup_u32_u16_u16_Test0();
Tuple3<uint8_t, uint16_t, uint32_t> U_Get_tup_u8_u16_u32_Test0();
Tuple3<uint16_t, uint16_t, uint32_t> U_Get_tup_u16_u16_u32_Test0();
Tuple4<uint64_t, uint32_t, uint16_t, uint8_t> U_Get_tup_u64_u32_u16_u8_Test0();
Tuple4<uint64_t, uint32_t, uint16_t, uint16_t> U_Get_tup_u64_u32_u16_u16_Test0();
Tuple4<uint8_t, uint16_t, uint32_t, uint64_t> U_Get_tup_u8_u16_u32_u64_Test0();
Tuple4<uint16_t, uint16_t, uint32_t, uint64_t> U_Get_tup_u16_u16_u32_u64_Test0();
Tuple3<uint8_t, uint16_t, uint8_t> U_Get_tup_u8_u16_u8_Test0();
Tuple3<uint8_t, uint32_t, uint8_t> U_Get_tup_u8_u32_u8_Test0();
Tuple3<uint8_t, uint64_t, uint8_t> U_Get_tup_u8_u64_u8_Test0();
Tuple3<uint16_t, uint32_t, uint16_t> U_Get_tup_u16_u32_u16_Test0();
Tuple3<uint16_t, uint64_t, uint16_t> U_Get_tup_u16_u64_u16_Test0();
Tuple3<uint32_t, uint64_t, uint32_t> U_Get_tup_u32_u64_u32_Test0();
Tuple3<float, int32_t, int32_t> U_Get_tup_f32_i32_i32_Test0();
Tuple3<int32_t, float, int32_t> U_Get_tup_i32_f32_i32_Test0();
Tuple3<int32_t, int32_t, float> U_Get_tup_i32_i32_f32_Test0();
Tuple3<float, uint64_t, uint64_t> U_Get_tup_f32_u64_u64_Test0();
Tuple3<uint64_t, float, uint64_t> U_Get_tup_u64_f32_u64_Test0();
Tuple3<uint64_t, uint64_t, float> U_Get_tup_u64_u64_f32_Test0();
Tuple3<double, int32_t, int32_t> U_Get_tup_f64_i32_i32_Test0();
Tuple3<int32_t, double, int32_t> U_Get_tup_i32_f64_i32_Test0();
Tuple3<int32_t, int32_t, double> U_Get_tup_i32_i32_f64_Test0();
Tuple3<double, uint64_t, uint64_t> U_Get_tup_f64_u64_u64_Test0();
Tuple3<uint64_t, double, uint64_t> U_Get_tup_u64_f64_u64_Test0();
Tuple3<uint64_t, uint64_t, double> U_Get_tup_u64_u64_f64_Test0();
Tuple3<float, float, float> U_Get_tup_f32_f32_f32_Test0();
Tuple3<float, float, double> U_Get_tup_f32_f32_f64_Test0();
Tuple3<float, double, float> U_Get_tup_f32_f64_f32_Test0();
Tuple3<float, double, double> U_Get_tup_f32_f64_f64_Test0();
Tuple3<double, float, float> U_Get_tup_f64_f32_f32_Test0();
Tuple3<double, float, double> U_Get_tup_f64_f32_f64_Test0();
Tuple3<double, double, float> U_Get_tup_f64_f64_f32_Test0();
Tuple3<double, double, double> U_Get_tup_f64_f64_f64_Test0();

void TestPassingValuesToUCode()
{
	U_Pass_bool_Test0( false );
	U_Pass_bool_Test1( true );
	U_Pass_i8_Test0( 27 );
	U_Pass_i8_Test1( -58 );
	U_Pass_i8_Test2( 64, 31, 78, 120, -36, 67, 13, 58, 0, 45, 99, 105, -128, 127, 33, -88 );
	U_Pass_u8_Test0( 117 );
	U_Pass_u8_Test1( 134 );
	U_Pass_u8_Test2( 249 );
	U_Pass_i16_Test0( 27346 );
	U_Pass_i16_Test1( -15343 );
	U_Pass_u16_Test0( 17 );
	U_Pass_u16_Test1( 15642 );
	U_Pass_u16_Test2( 30651 );
	U_Pass_u16_Test3( 52188 );
	U_Pass_u16_Test4( 7655, 32768, 6582, 49, 0, 256, 9821, 65535, 25843, 58441, 864, 8962, 123, 645, 32767, 33 );
	U_Pass_i32_Test0( 274383 );
	U_Pass_i32_Test1( -7456 );
	U_Pass_i32_Test2( 0x78ABCDEF );
	U_Pass_i32_Test3( -674348993 );
	U_Pass_i32_Test4( 6531, -75247554, 456424, 8565523, 0, 0x7FFFFFFF, 54, int32_t(-0x80000000ll), 643, 7621375, 7567863, -24782, 786234786, 12308562, -8624557, 867245 );
	U_Pass_u32_Test0( 78u );
	U_Pass_u32_Test1( 45677u );
	U_Pass_u32_Test2( 6633477u );
	U_Pass_u32_Test3( 0xFEDCBA98u );
	U_Pass_i64_Test0( 636746ll );
	U_Pass_i64_Test1( -36ll );
	U_Pass_i64_Test2( -6433763852258913ll );
	U_Pass_i64_Test3( 7434744889515923245ll );
	U_Pass_u64_Test0( 7612 );
	U_Pass_u64_Test1( 2147521472ull );
	U_Pass_u64_Test2( 7445889504678477554ull );
	U_Pass_u64_Test3( 18246784073809531617ull );
	U_Pass_u64_Test4( 36774676ull, 78ull, 478543737834754785ull, 0ull, 21ull, 78542136ull, 8847838ull, 47547247472367861ull, 7623758235ull, 88524ull, 76521ul, 0xFF00112233445566ull, 77852ull, 651374ull, 86ull, 3741356ull );
#ifdef ENABLE_128BIT_INT_TESTS
	U_Pass_i128_Test0( ( __int128_t( 0x0123456789ABCDEFll) << 64u ) | 0xFEDCBA9876543210ll );
	U_Pass_u128_Test0( ( __uint128_t(0xFEDCBA9876543210ull) << 64u ) | 0x0123456789ABCDEFull );
#endif
	U_Pass_char8_Test0( 'Q' );
	U_Pass_char8_Test1( '!' );
	U_Pass_char8_Test2( ' ' );
	U_Pass_char8_Test3( 240 );
	U_Pass_char16_Test0( u'Ж' );
	U_Pass_char16_Test1( u'Ꙥ' );
	U_Pass_char32_Test0( U'😀' );
	U_Pass_f32_Test0( 0.0f );
	U_Pass_f32_Test1( 0.125f );
	U_Pass_f32_Test2( 6743.5f );
	U_Pass_f32_Test3( -7689543378437.0f );
	U_Pass_f32_Test4( std::numeric_limits<float>::infinity() );
	U_Pass_f32_Test5( std::numeric_limits<float>::quiet_NaN() );
	U_Pass_f32_Test6( 1786.5f, -643.4f, 754.0f, 353347.0f, 3000000.0f, -4454.25f, 0.0f, 66434.0f, 3643.3f, 367341.5f, 67436.125f, 378436.0f, 42.75f, -7542.2f, 6564.0f, 7854300000000.0f );
	U_Pass_f64_Test0( 0.0 );
	U_Pass_f64_Test1( 0.0625 );
	U_Pass_f64_Test2( 173.25 );
	U_Pass_f64_Test3( -569907695478437.0 );
	U_Pass_f64_Test4( std::numeric_limits<double>::infinity() );
	U_Pass_f64_Test5( std::numeric_limits<double>::quiet_NaN() );
	U_Pass_f64_Test6( 364341.5, 1786.5, -643.4, 353347.0, 70000000.0, -4454.25, 7854320000000.0, 0.0, 66434.0, 3643.3, 67436.125, 754.0, 378436.0, -42.75, -6552.4, 6564.0 );
	U_Pass_u8_x1_Test0( { 0xB4 } );
	U_Pass_u8_x2_Test0( { 0xAB, 0x7C } );
	U_Pass_u8_x3_Test0( { 0x16, 0xF7, 0x75 } );
	U_Pass_u8_x4_Test0( { 0x01, 0x23, 0x45, 0x67 } );
	U_Pass_u8_x5_Test0( { 0x89, 0xAB, 0xCD, 0xEF, 0x76 } );
	U_Pass_u8_x6_Test0( { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } );
	U_Pass_u8_x7_Test0( { 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD } );
	U_Pass_u8_x8_Test0( { 0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87 } );
	U_Pass_u8_x9_Test0( { 0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89 } );
	U_Pass_u8_x10_Test0( { 0x9A, 0xAB, 0xBC, 0xCD, 0xDE, 0xEF, 0xF0, 0x01, 0x12, 0x23 } );
	U_Pass_u8_x11_Test0( { 0xF0, 0x1E, 0xD2, 0x3C, 0xB4, 0x5A, 0x96, 0x78, 0x11, 0x22, 0x33 } );
	U_Pass_u8_x12_Test0( { 0xF7, 0x8B, 0xE6, 0x72, 0x85, 0x00, 0x3C, 0xFE, 0xD5, 0x91, 0x4E, 0x67 } );
	{
		std::array<uint8_t, 13> arg;
		for( auto  i= 0u; i < 13u; ++i )
			arg[i]= uint8_t( i * i + i * 7u );
		U_Pass_u8_x13_Test0( arg );
	}
	{
		std::array<uint8_t, 14> arg;
		for( auto  i= 0u; i < 14u; ++i )
			arg[i]= uint8_t( i * i - i * 5u + 3u );
		U_Pass_u8_x14_Test0( arg );
	}
	{
		std::array<uint8_t, 15> arg;
		for( auto  i= 0u; i < 15u; ++i )
			arg[i]= uint8_t( i * i + i * 3u - 7u );
		U_Pass_u8_x15_Test0( arg );
	}
	{
		std::array<uint8_t, 16> arg;
		for( auto  i= 0u; i < 16u; ++i )
			arg[i]= uint8_t( 3u * i * i + i * 7u - 2u );
		U_Pass_u8_x16_Test0( arg );
	}
	{
		std::array<uint8_t, 17> arg;
		for( auto  i= 0u; i < 17u; ++i )
			arg[i]= uint8_t( i * i + i * 13u - 3567u );
		U_Pass_u8_x17_Test0( arg );
	}
	{
		std::array<uint8_t, 29> arg;
		for( auto  i= 0u; i < 29u; ++i )
			arg[i]= uint8_t( ( i * i ) ^ ( i + 13u ) );
		U_Pass_u8_x29_Test0( arg );
	}
	{
		std::array<uint8_t, 371> arg;
		for( auto  i= 0u; i < 371u; ++i )
			arg[i]= uint8_t( i * 3u - i * i * 6u + 564u );
		U_Pass_u8_x371_Test0( arg );
	}
	U_Pass_i16_x1_Test0( { -27816 } );
	U_Pass_i16_x2_Test0( { 1754, -6534 } );
	U_Pass_i16_x3_Test0( { -1234, 30431, 561 } );
	U_Pass_i16_x4_Test0( { 29554, -63, 2452, -22543 } );
	U_Pass_i16_x5_Test0( { -3431, 9655, 15667, 46, 19734 } );
	U_Pass_i16_x6_Test0( { 3451, 29655, 93, -5667, 19734, -4323 } );
	U_Pass_i16_x7_Test0( { 3351, 2955, 5393, -5667, -4323, 19234, -3373 } );
	U_Pass_i16_x8_Test0( { -3373, 3351, -5953, 15353, 5667, 4323, -29214, 5342 } );
	U_Pass_i16_x9_Test0( { 7322, -3373, 3351, -5953, 5667, -29214, 5342, 25353, -6343 } );
	{
		std::array<int16_t, 15> arg;
		for( int32_t i= 0; i < 15; ++i )
			arg[ size_t(i) ]= int16_t( i * i * 7 - i * 37 + 3 );
		U_Pass_i16_x15_Test0( arg );
	}
	{
		std::array<int16_t, 83> arg;
		for( int32_t i= 0; i < 83; ++i )
			arg[ size_t(i) ]= int16_t( i * i * 5 - i * 43 + 11 );
		U_Pass_i16_x83_Test0( arg );
	}
	U_Pass_u32_x1_Test0( { 0xFBA633ADu } );
	U_Pass_u32_x2_Test0( { 0x5356A4D7u, 0x05AD74CBu } );
	U_Pass_u32_x3_Test0( { 0x15A67FCBu, 0x5D56A437u, 0xAB4C8F12u } );
	U_Pass_u32_x4_Test0( { 0x23A68FCAu, 0x1E5AA732u, 0xC34D8F12u, 0xF354AB3Eu } );
	U_Pass_u32_x5_Test0( { 0x5E5AE732u, 0x33A68FCAu, 0xE34D8F12u, 0xD354AB3Eu, 0x03AD63C3u } );
	U_Pass_u32_x6_Test0( { 0x34A68FCEu, 0x5E53E732u, 0xE34D8A12u, 0x03ADE3C3u, 0xD354CB3Eu, 0x42D4E6C8u } );
	U_Pass_u32_x7_Test0( { 0x14A68FCEu, 0x5F53E732u, 0x42D4E6F8u, 0xE36D8A12u, 0x63ADF3C3u, 0x7354CE3Eu, 0x63E7F7C5u } );
	U_Pass_u32_x8_Test0( { 0xE3E7F731u, 0xE4A686CEu, 0xD2D4E638u, 0xF36D8A62u, 0x63ADF4C3u, 0x7E54CE3Eu, 0xDF53E732u, 0xC3E47C15u } );
	U_Pass_u32_x9_Test0( { 0xE3E1F731u, 0xE4A686CEu, 0x5E7CD38Fu, 0xD2D48638u, 0xF36D8A62u, 0x63ADF5C3u, 0x7E54CE3Eu, 0xDF51E732u, 0xE3E47C15u } );
	{
		std::array<uint32_t, 17> arg;
		for( uint32_t i= 0u; i < 17u; ++i )
			arg[i]= uint32_t( i * i * i * 37u + i * i * 52u + i * 12u + 36747u );
		U_Pass_u32_x17_Test0( arg );
	}
	U_Pass_u64_x1_Test0( { 0xFBA633ADE4A686CEull } );
	U_Pass_u64_x2_Test0( { 0xEBA631ADE4968FC3ull, 0x5E7CD38FDF53E732ull } );
	U_Pass_u64_x3_Test0( { 0x5E72D38FDF53E73Eull, 0xEBA631ACE4968FC4ull, 0xC3E47C1534A68FCEull } );
	U_Pass_u64_x4_Test0( { 0xE3E7F731AB4C8F12ull, 0x1E72D38FDF52E73Eull, 0xEBA631FCE4968FC4ull, 0xC3E4741534A68FCEull } );
	U_Pass_u64_x5_Test0( { 0x13E7F7313B4C8F12ull, 0x1E72D38FDC52E79Eull, 0x7353CE3ED2D48638ull, 0xFBA631FCE1968FC4ull, 0xC3E1741534A68F7Eull } );
	{
		std::array<uint64_t, 11> arg;
		for(uint64_t i= 0u; i < 11u; ++i )
			arg[ size_t(i) ]= i * i * i * 337547ull + i * i * i * 563454548ull + 34565224787ull;
		U_Pass_u64_x11_Test0( arg );
	}
#ifdef ENABLE_128BIT_INT_TESTS
	U_Pass_u128_x1_Test0( { ( __uint128_t( 0xEBA631ADE4968FC3ull ) << 64u ) | 0x5E7CD38FDF53E732ull } );
	U_Pass_u128_x2_Test0( {
			( __uint128_t( 0xEEA631ADE4968FC3ull ) << 64u ) | 0x5E7CD3CFDF53E732ull,
			( __uint128_t( 0x7353CE3ED2D48638ull ) << 64u ) | 0xC3E4741534A68FCEull,
		} );
	U_Pass_u128_x3_Test0( {
			( __uint128_t( 0x1EA63AADE4968FC3ull ) << 64u ) | 0x5E7CDECFDF53E738ull,
			( __uint128_t( 0x7353CE3ED2D48138ull ) << 64u ) | 0xC354741534A68FFEull,
			( __uint128_t( 0x7353CE3ED2D48638ull ) << 64u ) | 0x13E7F7313B4C8F12ull,
		} );
#endif
	U_Pass_f32_x1_Test0( { -7878.25f } );
	U_Pass_f32_x2_Test0( { 7.5f, 0.0625f } );
	U_Pass_f32_x3_Test0( { 72.15f, 0.0f, -0.125f } );
	U_Pass_f32_x4_Test0( { 3712.2f, 663300.0f, -336.25f, 250000000.0f } );
	U_Pass_f32_x5_Test0( { -536.25f, 4711.4f, 66330230.0f, 270000000.0f, -5333566.0f } );
	U_Pass_f32_x6_Test0( { -4333563.0f, -536.25f, 4712.8f, 26330231.0f, 130000000.0f, 0.01f } );
	U_Pass_f32_x7_Test0( { 3712.3f, -1536.25f, 2633031.0f, -4323565.0f, 13005000.0f, 0.02f, -6434.75f } );
	U_Pass_f32_x8_Test0( { 3742.5f, 13005010.0f, -1566.25f, 1643031.0f, -432515.5f, 0.04f, -634.75f, 164363.0f } );
	U_Pass_f32_x9_Test0( { 162363.0f, 3742.7f, -1563.25f, 1644031.0f, -437515.5f, 0.08f, 13005210.0f, -534.75f, 345423.0f } );
	{
		std::array<float, 15> arg;
		for( uint32_t i= 0u; i < 15u; ++i )
			arg[i]= float(i) * float(i) * 13.5f + 153.25f;
		U_Pass_f32_x15_Test0( arg );
	}
	{
		std::array<float, 47> arg;
		for( uint32_t i= 0u; i < 47u; ++i )
			arg[i]= float(i) * float(i) * 12.75f + 253.5f;
		U_Pass_f32_x47_Test0( arg );
	}
	U_Pass_f64_x1_Test0( { -7878.25 } );
	U_Pass_f64_x2_Test0( { 7.5, 0.0625 } );
	U_Pass_f64_x3_Test0( { 72.15, 0.0, -0.125 } );
	U_Pass_f64_x4_Test0( { 3712.2, 663300.0, -336.25, 250000000.0 } );
	U_Pass_f64_x5_Test0( { -536.25, 4711.4, 66330230.0, 270000000.0, -5333566.0 } );
	U_Pass_f64_x6_Test0( { -4333563.0, -536.25, 4712.8, 26330231.0, 130000000.0, 0.01 } );
	U_Pass_f64_x7_Test0( { 3712.3, -1536.25, 2633031.0, -4323565.0, 13005000.0, 0.02, -6434.75 } );
	U_Pass_f64_x8_Test0( { 3742.5, 13005010.0, -1566.25, 1643031.0, -432515.5, 0.04, -634.75, 164363.0 } );
	U_Pass_f64_x9_Test0( { 162363.0, 3742.7, -1563.25, 1644031.0, -437515.5, 0.08, 13005210.0, -534.75, 345423.0 } );
	{
		std::array<double, 15> arg;
		for( uint32_t i= 0u; i < 15u; ++i )
			arg[i]= double(i) * double(i) * 13.5 + 153.25;
		U_Pass_f64_x15_Test0( arg );
	}
	{
		std::array<double, 47> arg;
		for( uint32_t i= 0u; i < 47u; ++i )
			arg[i]= double(i) * double(i) * 12.75 + 253.5;
		U_Pass_f64_x47_Test0( arg );
	}
	U_Pass_char8_x1_Test0( { 'H' } );
	U_Pass_char8_x2_Test0( { '-', '8' } );
	U_Pass_char8_x3_Test0( { 'K', 'e', 'k' } );
	U_Pass_char8_x4_Test0( { 'S', 'P', 'Q', 'R' } );
	U_Pass_char8_x5_Test0( { 'A', 'p', 'p', 'l', 'E' } );
	U_Pass_char8_x6_Test0( { '5', '6', ' ', 't', 'o', ' ' } );
	U_Pass_char8_x7_Test0( { '@', '#', '-', '-', 'A', 'B', 'e' } );
	U_Pass_char8_x8_Test0( { 'S', '.', 'P', '.', 'Q', '.', 'R', '.' } );
	{
		std::array<char, 9> arg;
		std::memcpy( arg.data(), "Жопа!", 9 );
		U_Pass_char8_x9_Test0( arg );
	}
	U_Pass_char8_x10_Test0( { 'B', 'l', 'a', 'c', 'k', ' ', 'M', 'e', 's', 'a' } );
	U_Pass_char8_x11_Test0( { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[' } );
	U_Pass_char8_x12_Test0( { ']', '[', 'p', 'o', 'i', 'u', 'y', 't', 'r', 'e', 'w', 'q' } );
	U_Pass_char8_x13_Test0( { 'C', 'o', 'm', 'p', 'u', 't', 'e', 'r', 'l', 'i', 'e', 'b', 'e' } );
	U_Pass_char8_x14_Test0( { '1', '1', ' ', '+', ' ', '2', '2', ' ', '=', ' ', 's', 'o', 'm', 'e' } );
	U_Pass_char8_x15_Test0( { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O' } );
	U_Pass_char8_x16_Test0( { 'E', 'r', ' ', 'i', 's', 't', ' ', 'w', 'i', 'e', 'd', 'e', 'r', ' ', 'd', 'a' } );
	U_Pass_char8_x17_Test0( { 'X', 'Y', ' ', '=', ' ', '3', '3', ' ', '+', ' ', '4', '4', ' ', '-', ' ', '5', '5' } );
	U_Pass_char8_x32_Test0( { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'F', 'E', 'D', 'C', 'B', 'A', '9', '8', '7', '6', '5', '4', '3', '2', '1', '0' } );
	U_Pass_char8_x39_Test0( { 'F', 'i', 'c', 'k', 'e', 't', ' ', 'e', 'u', 'c', 'h', ',', ' ', 'i', 'h', 'r', ' ', 'b', 'e', 'l', 'e', 'i', 'd', 'i', 'g', 't', ' ', 'm', 'e', 'i', 'n', 'e', ' ', 'A', 'u', 'g', 'e', 'n', '!' } );
	U_Pass_tup_i8_u8_Test0( { -76, 214 } );
	U_Pass_tup_i8_u8_Test1( { 127, 13 } );
	U_Pass_tup_i8_i16_Test0( { -72, 31000 } );
	U_Pass_tup_i8_i16_Test1( { 105, -27823 } );
	U_Pass_tup_i8_u32_Test0( { 98, 0xFA56DE4Fu } );
	U_Pass_tup_i8_u32_Test1( { -123, 0x3AC6DE1Fu } );
	U_Pass_tup_i8_i64_Test0( { 72, 6336747347783754868 } );
	U_Pass_tup_i8_i64_Test1( { -13, -642476347823222 } );
#ifdef ENABLE_128BIT_INT_TESTS
	U_Pass_tup_i8_u128_Test0( { 71, ( __uint128_t( 0x0123456789ABCDEFull ) << 64u ) | 0xFEDCBA9876543210ull } );
	U_Pass_tup_i8_u128_Test1( { -88, ( __uint128_t( 0xFEDCBA9876543210ull ) << 64u  ) | 0x0123456789ABCDEFull } );
#endif
	U_Pass_tup_i8_f32_Test0( { 78, 6763.5f } );
	U_Pass_tup_i8_f32_Test1( { -124, -6346470.0f } );
	U_Pass_tup_i8_f64_Test0( { 53, -67163.25 } );
	U_Pass_tup_i8_f64_Test1( { -97, 251.0 } );
	U_Pass_tup_u16_i8_Test0( { 1245, 114 } );
	U_Pass_tup_u16_i8_Test1( { 48437, -13 } );
	U_Pass_tup_u16_u16_Test0( { 43311, 31000 } );
	U_Pass_tup_u16_u16_Test1( { 37, 57823 } );
	U_Pass_tup_u16_i32_Test0( { 1298, -533167754 } );
	U_Pass_tup_u16_i32_Test1( { 65530, 336637444 } );
	U_Pass_tup_u16_u64_Test0( { 257, 0x08192A3B4C5D6E7Full } );
	U_Pass_tup_u16_u64_Test1( { 56316, 0xF7E6D5C4B3A29180ull } );
#ifdef ENABLE_128BIT_INT_TESTS
	U_Pass_tup_u16_i128_Test0( { 712, ( __int128_t( 0x0123456789ABCDEFll ) << 64u ) | 0x7EDCBA9876543210ll } );
	U_Pass_tup_u16_i128_Test1( { 8812, ( __int128_t( 0x1EDCBA9876543210ll ) << 64u ) | 0x0123456789ABCDEFll } );
#endif
	U_Pass_tup_u16_f32_Test0( { 2467, 6263.5f } );
	U_Pass_tup_u16_f32_Test1( { 9850, -6356470.0f } );
	U_Pass_tup_u16_f64_Test0( { 3126, -37163.125 } );
	U_Pass_tup_u16_f64_Test1( { 65535, 253.0 } );
	U_Pass_tup_i32_u8_Test0( { -3347237, 214 } );
	U_Pass_tup_i32_u8_Test1( { 9553344, 13 } );
	U_Pass_tup_i32_i16_Test0( { -346314, 31000 } );
	U_Pass_tup_i32_i16_Test1( { 78656858, -27823 } );
	U_Pass_tup_i32_u32_Test0( { 7542475, 0xFA56DE4Fu } );
	U_Pass_tup_i32_u32_Test1( { -36131647, 0x3AC6DE1Fu } );
	U_Pass_tup_i32_i64_Test0( { 847823478, 6336747347783754868 } );
	U_Pass_tup_i32_i64_Test1( { -854647, -642476347823222 } );
#ifdef ENABLE_128BIT_INT_TESTS
	U_Pass_tup_i32_u128_Test0( { -643647, ( __uint128_t(0x0123456789ABCDEFull) << 64u ) | 0xFEDCBA9876543210ull } );
	U_Pass_tup_i32_u128_Test1( { 856247, ( __uint128_t(0xFEDCBA9876543210ull) << 64u ) | 0x0123456789ABCDEFull } );
#endif
	U_Pass_tup_i32_f32_Test0( { 7542347, 6763.5f } );
	U_Pass_tup_i32_f32_Test1( { -334642, -6346470.0f } );
	U_Pass_tup_i32_f64_Test0( { 6413647, -67163.25 } );
	U_Pass_tup_i32_f64_Test1( { -5674137, 251.0 } );
	U_Pass_tup_u64_i8_Test0( { 0xFEDCBA9876543210ull, 114 } );
	U_Pass_tup_u64_i8_Test1( { 0xFED7BA9876543E10ull, -13 } );
	U_Pass_tup_u64_u16_Test0( { 0x3EDCBA987654321Cull, 31000u } );
	U_Pass_tup_u64_u16_Test1( { 0x3EDCBAE87654721Cull, 57823u } );
	U_Pass_tup_u64_i32_Test0( { 0x9EDCBAEA7654721Cull, -533167754 } );
	U_Pass_tup_u64_i32_Test1( { 0x91DCBAEA765472ECull, 336637444 } );
	U_Pass_tup_u64_u64_Test0( { 0x91DC6AEA765477ECull, 0x08192A3B4C5D6E7Full } );
	U_Pass_tup_u64_u64_Test1( { 0xD1DC6AEA7E5477ECull, 0xF7E6D5C4B3A29180ull } );
#ifdef ENABLE_128BIT_INT_TESTS
	U_Pass_tup_u64_i128_Test0( { 0xD1D16AEA7E54778Cull, ( __int128_t( 0x0123456789ABCDEFll ) << 64u ) | 0x7EDCBA9876543210ll } );
	U_Pass_tup_u64_i128_Test1( { 0x11D16AEA7E54278Cull, ( __int128_t( 0x1EDCBA9876543210ll ) << 64u ) | 0x0123456789ABCDEFll } );
#endif
	U_Pass_tup_u64_f32_Test0( { 0x1ED16AEA7E54278Cull, 6263.5f } );
	U_Pass_tup_u64_f32_Test1( { 0x1ED1AAEA7E54279Cull, -6356470.0f } );
	U_Pass_tup_u64_f64_Test0( { 0xCED164EA7E54278Cull, -37163.125 } );
	U_Pass_tup_u64_f64_Test1( { 0xCE1164EA7354278Cull, 253.0 } );
	U_Pass_tup_f32_u8_Test0( { -3347237.0f, 214 } );
	U_Pass_tup_f32_u8_Test1( { 3643.25f, 13 } );
	U_Pass_tup_f32_i16_Test0( { -346314.0f, 31000 } );
	U_Pass_tup_f32_i16_Test1( { 43440.1f, -27823 } );
	U_Pass_tup_f32_u32_Test0( { 5336.0f, 0xFA56DE4Fu } );
	U_Pass_tup_f32_u32_Test1( { -3346477.25f, 0x3AC6DE1Fu } );
	U_Pass_tup_f32_i64_Test0( { 5366.5f, 6336747347783754868 } );
	U_Pass_tup_f32_i64_Test1( { -0.0625f, -642476347823222 } );
#ifdef ENABLE_128BIT_INT_TESTS
	U_Pass_tup_f32_u128_Test0( { -3366.75f, ( __uint128_t( 0x0123456789ABCDEFull ) << 64u ) | 0xFEDCBA9876543210ull } );
	U_Pass_tup_f32_u128_Test1( { 0.125f, ( __uint128_t( 0xFEDCBA9876543210ull ) << 64u ) | 0x0123456789ABCDEFull } );
#endif
	U_Pass_tup_f32_f32_Test0( { 444666.0f, 6763.5f } );
	U_Pass_tup_f32_f32_Test1( { -15215.2f, -6346470.0f } );
	U_Pass_tup_f32_f64_Test0( { 634663660000.0f, -67163.25 } );
	U_Pass_tup_f32_f64_Test1( { -333636.5f, 251.0 } );
	U_Pass_tup_f64_i8_Test0( { 44.25, 114 } );
	U_Pass_tup_f64_i8_Test1( { -3615.2, -13 } );
	U_Pass_tup_f64_u16_Test0( { 66547.0, 31000 } );
	U_Pass_tup_f64_u16_Test1( { 0.02, 57823 } );
	U_Pass_tup_f64_i32_Test0( { 54647.25, -533167754 } );
	U_Pass_tup_f64_i32_Test1( { -0.5, 336637444 } );
	U_Pass_tup_f64_u64_Test0( { 447.2, 0x08192A3B4C5D6E7Full } );
	U_Pass_tup_f64_u64_Test1( { 7372483800000.0, 0xF7E6D5C4B3A29180ull } );
#ifdef ENABLE_128BIT_INT_TESTS
	U_Pass_tup_f64_i128_Test0( { -5362370.5, ( __int128_t( 0x0123456789ABCDEFll ) << 64u ) | 0x7EDCBA9876543210ll } );
	U_Pass_tup_f64_i128_Test1( { -37485856855542.0, ( __int128_t( 0x1EDCBA9876543210ll ) << 64u ) | 0x0123456789ABCDEFll } );
#endif
	U_Pass_tup_f64_f32_Test0( { 2632647.0, 6263.5f } );
	U_Pass_tup_f64_f32_Test1( { 11.125, -6356470.0f } );
	U_Pass_tup_f64_f64_Test0( { -1.75, -37163.125 } );
	U_Pass_tup_f64_f64_Test1( { 0.0, 253.0 } );
	U_Pass_tup_u32_u16_u8_Test0( { 0x01234567, 0x89AB, 0xCD } );
	U_Pass_tup_u32_u16_u16_Test0( { 0x01234567, 0x89AB, 0xCDEF } );
	U_Pass_tup_u8_u16_u32_Test0( { 0x01, 0x2345, 0x6789ABCD } );
	U_Pass_tup_u16_u16_u32_Test0( { 0x0123, 0x4567, 0x89ABCDEF } );
	U_Pass_tup_u64_u32_u16_u8_Test0( { 0xFEDCBA9876543210, 0x01234567, 0x89AB, 0xCD } );
	U_Pass_tup_u64_u32_u16_u16_Test0( { 0xFEDCBA9876543210, 0x01234567, 0x89AB, 0xCDEF } );
	U_Pass_tup_u8_u16_u32_u64_Test0( { 0x01, 0x2345, 0x6789ABCD, 0xFEDCBA9876543210 } );
	U_Pass_tup_u16_u16_u32_u64_Test0( { 0x0123, 0x4567, 0x89ABCDEF, 0xFEDCBA9876543210 } );
	U_Pass_tup_u8_u16_u8_Test0( { 0xFE, 0xDCBA, 0x98 } );
	U_Pass_tup_u8_u32_u8_Test0( { 0xFE, 0xDCBA9876, 0x54 } );
	U_Pass_tup_u8_u64_u8_Test0( { 0xAB, 0x0123456789ABCDEF, 0x12 } );
	U_Pass_tup_u16_u32_u16_Test0( { 0x0123, 0x456789AB, 0xCDEF } );
	U_Pass_tup_u16_u64_u16_Test0( { 0xFEDC, 0x17283A4B5C6D7E8F, 0x9876 } );
	U_Pass_tup_u32_u64_u32_Test0( { 0x01234567, 0x17283A4B5C6D7E8F, 0x89ABCEDF } );
	U_Pass_tup_f32_i32_i32_Test0( { 123.45f, 266747477, -963237321 } );
	U_Pass_tup_i32_f32_i32_Test0( { -196323732, 236.5f, 266745477 } );
	U_Pass_tup_i32_i32_f32_Test0( { 196323735, 166745427, -0.7f } );
	U_Pass_tup_f32_u64_u64_Test0( { 323.25f, 0x64AB3C5482367DE3u, 0x17283A4B5C6D7E8Fu } );
	U_Pass_tup_u64_f32_u64_Test0( { 0x7637347A36B4E218u, 1336.5f, 0x067374735AE7DFC13u } );
	U_Pass_tup_u64_u64_f32_Test0( { 0x27283A4B5C637E8Fu, 0xE637347436B47218u, 4.7f } );
	U_Pass_tup_f64_i32_i32_Test0( { 123.45, 266747477, -963237321 } );
	U_Pass_tup_i32_f64_i32_Test0( { -196323732, 236.5, 266745477 } );
	U_Pass_tup_i32_i32_f64_Test0( { 196323735, 166745427, -0.7 } );
	U_Pass_tup_f64_u64_u64_Test0( { 323.25, 0x64AB3C5482367DE3u, 0x17283A4B5C6D7E8Fu } );
	U_Pass_tup_u64_f64_u64_Test0( { 0x7637347A36B4E218u, 1336.5, 0x067374735AE7DFC13u } );
	U_Pass_tup_u64_u64_f64_Test0( { 0x27283A4B5C637E8Fu, 0xE637347436B47218u, 4.7 } );
	U_Pass_tup_f32_f32_f32_Test0( { 0.25f, -363.2f, 3773440.0f } );
	U_Pass_tup_f32_f32_f64_Test0( { 0.25f, -363.2f, 3773440.0 } );
	U_Pass_tup_f32_f64_f32_Test0( { 0.25f, -363.2, 3773440.0f } );
	U_Pass_tup_f32_f64_f64_Test0( { 0.25f, -363.2, 3773440.0 } );
	U_Pass_tup_f64_f32_f32_Test0( { 0.25, -363.2f, 3773440.0f } );
	U_Pass_tup_f64_f32_f64_Test0( { 0.25, -363.2f, 3773440.0  } );
	U_Pass_tup_f64_f64_f32_Test0( { 0.25, -363.2, 3773440.0f } );
	U_Pass_tup_f64_f64_f64_Test0( { 0.25, -363.2, 3773440.0 } );
	U_Pass_u32_u32_u32_u32_u32_tup_u64_f64( 47588u, 33677u, 12u, 3785427u, 13748588u, { 0x0123456789ABCDEFu, 26376.25 } );
#ifdef ENABLE_128BIT_INT_TESTS
	U_Pass_u128_u32_u32_u32_u32_tup_u64_f64( ( __uint128_t(0x1122334455667788ull) << 64u ) | 0x99AABBCCDDEEFFull, 33647, 13, 378543, 53748583, { 0x012D456789ABC3EF, -26336.75 } );
#endif
	U_Pass_u32_u32_u32_u32_u32_tup_u64_u64( 47288u, 31677u, 14u, 3285427u, 13748988u, { 0x0123456789ABCDEFu, 0xFEDCBA9876543210u } );
	U_Pass_u32_u32_u32_u32_u32_u32_tup_u64_f64( 41588u, 633677u, 7812u, 5785427u, 23748588u, 788588u, { 0xFEDCBA9876543210u, -16376.75 } );
	U_Pass_f64_f64_f64_f64_f64_f64_f64_f64_tup_u64_f64( 1.0, 774.3, -366.0, 0.125, 6336.2, 6774.0, -126.25, 0.75, { 0xFED7BA98C6543210u, 163.2 } );
	U_Pass_f64_f64_f64_f64_f64_f64_f64_tup_f64_f64( 3.0, 724.1, -365.0, -0.125, 6336.2, 6724.0, -126.85, { 631.3, 165.2 } );

	TEST_ASSERT( U_Get_i8_Test0() == 117 );
	TEST_ASSERT( U_Get_i8_Test1() == -24 );
	TEST_ASSERT( U_Get_u8_Test0() == 104 );
	TEST_ASSERT( U_Get_u8_Test1() == 231 );
	TEST_ASSERT( U_Get_i16_Test0() == 12361 );
	TEST_ASSERT( U_Get_i16_Test1() == -25331 );
	TEST_ASSERT( U_Get_u16_Test0() == 22365 );
	TEST_ASSERT( U_Get_u16_Test1() == 43652 );
	TEST_ASSERT( U_Get_i32_Test0() == 534875478 );
	TEST_ASSERT( U_Get_i32_Test1() == -34745344 );
	TEST_ASSERT( U_Get_u32_Test0() == 0x35B6E36Fu );
	TEST_ASSERT( U_Get_u32_Test1() == 0xE54EC57Fu );
	TEST_ASSERT( U_Get_i64_Test0() == 474247578522 );
	TEST_ASSERT( U_Get_i64_Test1() == -65433774422444 );
	TEST_ASSERT( U_Get_u64_Test0() == 0x35B6E36F4E8FEC37u );
	TEST_ASSERT( U_Get_u64_Test1() == 0xE54EC57F1E070FC1u );
#ifdef ENABLE_128BIT_INT_TESTS
	TEST_ASSERT( U_Get_i128_Test0() == __int128_t( ( __uint128_t( 0x0123456789ABCDEFull) << 64u ) | 0xFEDCBA9876543210ull ) );
	TEST_ASSERT( U_Get_u128_Test0() == ( ( ( __uint128_t( 0xFEDCBA9876543210ull) << 64u ) | 0x0123456789ABCDEFull ) ) );
#endif
	TEST_ASSERT( U_Get_char8_Test0() == 'n' );
	TEST_ASSERT( U_Get_char8_Test1() == 145 );
	TEST_ASSERT( U_Get_char16_Test0() == u'Й' );
	TEST_ASSERT( U_Get_char16_Test1() == u'Ꙥ' );
	TEST_ASSERT( U_Get_char32_Test0() == U'😀' );
	TEST_ASSERT( U_Get_f32_Test0() == 642.05f );
	TEST_ASSERT( U_Get_f32_Test1() == -0.012f );
	TEST_ASSERT( U_Get_f64_Test0() == 544747366.75 );
	TEST_ASSERT( U_Get_f64_Test1() == -34.25 );
	{
		std::array<int8_t, 1> expected{ -95 };
		TEST_ASSERT( U_Get_i8_x1_Test0() == expected );
	}
	{
		std::array<int8_t, 2> expected{ 107, -34 };
		TEST_ASSERT( U_Get_i8_x2_Test0() == expected );
	}
	{
		std::array<int8_t, 3> expected{ -65, 77, 4 };
		TEST_ASSERT( U_Get_i8_x3_Test0() == expected );
	}
	{
		const std::array<int8_t, 4> expected{ 37, 0, 127, -25 };
		TEST_ASSERT( U_Get_i8_x4_Test0() == expected );
	}
	{
		const std::array<int8_t, 5> expected{ 67, 3, 127, -25, 88 };
		TEST_ASSERT( U_Get_i8_x5_Test0() == expected );
	}
	{
		const std::array<int8_t, 6> expected{ 37, -63, 127, -25, 125, -107 };
		TEST_ASSERT( U_Get_i8_x6_Test0() == expected );
	}
	{
		const std::array<int8_t, 7> expected{ 117, 27, -93, -21, 125, -107, 72 };
		TEST_ASSERT( U_Get_i8_x7_Test0() == expected );
	}
	{
		const std::array<int8_t, 8> expected{ 34, 112, 73, -21, 125, -107, 52, -94 };
		TEST_ASSERT( U_Get_i8_x8_Test0() == expected );
	}
	{
		const std::array<int8_t, 9> expected{ 74, 126, 67, 73, -21, 125, -107, 53, -92 };
		TEST_ASSERT( U_Get_i8_x9_Test0() == expected );
	}
	{
		const std::array<int8_t, 10> expected{ 24, 120, 54, 73, -21, 105, -89, -107, 53, -93 };
		TEST_ASSERT( U_Get_i8_x10_Test0() == expected );
	}
	{
		const std::array<int8_t, 11> expected{ 29, 99, 120, 54, 78, -121, 105, -85, -107, 53, -94 };
		TEST_ASSERT( U_Get_i8_x11_Test0() == expected );
	}
	{
		const std::array<int8_t, 12> expected{ 69, 95, 120, 34, 78, -121, 105, -87, -107, 48, 63, -98 };
		TEST_ASSERT( U_Get_i8_x12_Test0() == expected );
	}
	{
		const std::array<int8_t, 13> expected{ 62, 95, 120, 31, 78, -121, 76, 105, -87, 107, 48, 64, -58 };
		TEST_ASSERT( U_Get_i8_x13_Test0() == expected );
	}
	{
		const std::array<int8_t, 14> expected{ 78, 61, 94, 121, 31, 72, -121, 78, 125, -87, 107, 48, 14, -28 };
		TEST_ASSERT( U_Get_i8_x14_Test0() == expected );
	}
	{
		const std::array<int8_t, 15> expected{ 79, 61, 94, 121, 121, 52, -121, 78, -5, 125, -87, 107, 48, 24, -7 };
		TEST_ASSERT( U_Get_i8_x15_Test0() == expected );
	}
	{
		const std::array<int8_t, 16> expected{ 59, 61, 84, 121, 121, 55, -121, 78, 74, -51, 24, -87, 107, 48, 24, -73 };
		TEST_ASSERT( U_Get_i8_x16_Test0() == expected );
	}
	{
		const std::array<int8_t, 17> expected{ 69, 65, 82, 101, 121, 55, -121, 0, 78, 74, -56, 24, -87, 104, 58, 34, -93 };
		TEST_ASSERT( U_Get_i8_x17_Test0() == expected );
	}
	{
		const auto res= U_Get_i8_x35_Test0();
		for( int32_t i= 0; i < 35; ++i )
			TEST_ASSERT( res[ size_t(i) ] == int8_t( i * i - 5 * i - 2 ) );
	}
	{
		const std::array<uint16_t, 1> expected{ 0xABCD };
		TEST_ASSERT( U_Get_u16_x1_Test0() == expected );
	}
	{
		const std::array<uint16_t, 2> expected{ 0xEF01, 0x2345 };
		TEST_ASSERT( U_Get_u16_x2_Test0() == expected );
	}
	{
		const std::array<uint16_t, 3> expected{ 0x6789, 0xABCD, 0xEF01 };
		TEST_ASSERT( U_Get_u16_x3_Test0() == expected );
	}
	{
		const std::array<uint16_t, 4> expected{ 0x2345, 0x6789, 0xABCD, 0xEF01 };
		TEST_ASSERT( U_Get_u16_x4_Test0() == expected );
	}
	{
		const std::array<uint16_t, 5> expected{ 0x2233, 0x4455, 0x6677, 0x8899, 0xAABB };
		TEST_ASSERT( U_Get_u16_x5_Test0() == expected );
	}
	{
		const std::array<uint16_t, 6> expected{ 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666 };
		TEST_ASSERT( U_Get_u16_x6_Test0() == expected );
	}
	{
		const std::array<uint16_t, 7> expected{ 0x0123, 0x1234, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789 };
		TEST_ASSERT( U_Get_u16_x7_Test0() == expected );
	}
	{
		const std::array<uint16_t, 8> expected{ 0xFEDC, 0xBA98, 0x7654, 0x3210, 0xDEAD, 0xC0DE, 0xB00B, 0xEBA0 };
		TEST_ASSERT( U_Get_u16_x8_Test0() == expected );
	}
	{
		const std::array<uint16_t, 9> expected{ 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0xAAAA, 0xBBBB, 0xCCCC };
		TEST_ASSERT( U_Get_u16_x9_Test0() == expected );
	}
	{
		const std::array<uint16_t, 15> expected{ 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE, 0xFFFF };
		TEST_ASSERT( U_Get_u16_x15_Test0() == expected );
	}
	{
		const auto res= U_Get_u16_x21_Test0();
		for( uint32_t i = 0u; i < 21u; ++i )
			TEST_ASSERT( res[ size_t(i) ] == uint16_t( i * i * 15u + i * 23u + 17u ) );
	}
	{
		const std::array<int32_t, 1> expected{ 547718243 };
		TEST_ASSERT( U_Get_i32_x1_Test0() == expected );
	}
	{
		const std::array<int32_t, 2> expected{ -4744785, 1236317 };
		TEST_ASSERT( U_Get_i32_x2_Test0() == expected );
	}
	{
		const std::array<int32_t, 3> expected{ 78482236, 847587447, 289378855 };
		TEST_ASSERT( U_Get_i32_x3_Test0() == expected );
	}
	{
		const std::array<int32_t, 4> expected{ -66366, 2667377, -1674, 757233 };
		TEST_ASSERT( U_Get_i32_x4_Test0() == expected );
	}
	{
		const std::array<int32_t, 5> expected{ -6353, -61366, 2667371, -2674, 5757233 };
		TEST_ASSERT( U_Get_i32_x5_Test0() == expected );
	}
	{
		const std::array<int32_t, 6> expected{ 5757233, -6353, 616613, 23521, 2667371, -2677834 };
		TEST_ASSERT( U_Get_i32_x6_Test0() == expected );
	}
	{
		const std::array<int32_t, 7> expected{ 5757213, -63543, 616513, 231521, 266371, -26778234, 289378855 };
		TEST_ASSERT( U_Get_i32_x7_Test0() == expected );
	}
	{
		const std::array<int32_t, 8> expected{ 157573, -613543, 6113, 231561, 2661371, -26778234, 289237855, -2634 };
		TEST_ASSERT( U_Get_i32_x8_Test0() == expected );
	}
	{
		const std::array<int32_t, 9> expected{ 15773, -613543, 6116, 211561, 2651371, -3412, -26778234, 289737855, -263 };
		TEST_ASSERT( U_Get_i32_x9_Test0() == expected );
	}
	{
		const auto res= U_Get_i32_x18_Test0();
		for( int32_t i= 0; i < 18; ++i )
			TEST_ASSERT( res[ size_t(i) ] == i * i * 752 + 6447 * i + 6437784 );
	}
	{
		const std::array<uint64_t, 1> expected{ 0x0123456789ABCDEFu };
		TEST_ASSERT( U_Get_u64_x1_Test0() == expected );
	}
	{
		const std::array<uint64_t, 2> expected{ 73434784882478588u, 378824886678u };
		TEST_ASSERT( U_Get_u64_x2_Test0() == expected );
	}
	{
		const std::array<uint64_t, 3> expected{ 378824816678u, 73436784882478588u, 74784785858885u };
		TEST_ASSERT( U_Get_u64_x3_Test0() == expected );
	}
	{
		const std::array<uint64_t, 4> expected{ 378824816178u, 734367848822478588u, 74784785888885u, 75443787589u };
		TEST_ASSERT( U_Get_u64_x4_Test0() == expected );
	}
	{
		const std::array<uint64_t, 5> expected{ 378824816171u, 734367842822478588u, 74784785888883u, 754243787589u, 67437858898u };
		TEST_ASSERT( U_Get_u64_x5_Test0() == expected );
	}
#ifdef ENABLE_128BIT_INT_TESTS
	{
		const std::array<__int128_t, 1> expected{ ( __int128_t(0x0123456789ABCDEFll) << 64u ) | 6458589734899ll };
		TEST_ASSERT( U_Get_i128_x1_Test0() == expected );
	}
	{
		const std::array<__int128_t, 2> expected{ ( __int128_t(0x3123456789AeCDEFll) << 64u ) | 6428589734399ll, ( __int128_t(0x5123456789ABFDEFll) << 64u ) | 6453589734892ll };
		TEST_ASSERT( U_Get_i128_x2_Test0() == expected );
	}
	{
		const std::array<__int128_t, 3> expected{ ( __int128_t(0x3123256789AeCDEFll) << 64u ) | 6428589754399ll, ( __int128_t(0x51234567896BFDEFll) << 64u ) | 6453582734892ll, ( __int128_t(0x0123456789ABCDEFll) << 64u ) | 6458589734899ll };
		TEST_ASSERT( U_Get_i128_x3_Test0() == expected );
	}
#endif
	{
		const std::array<float, 1> expected{ 0.125f };
		TEST_ASSERT( U_Get_f32_x1_Test0() == expected );
	}
	{
		const std::array<float, 2> expected{ -2.125f, 3.5f };
		TEST_ASSERT( U_Get_f32_x2_Test0() == expected );
	}
	{
		const std::array<float, 3> expected{ -22.125f, 35.5f, 7336.0f };
		TEST_ASSERT( U_Get_f32_x3_Test0() == expected );
	}
	{
		const std::array<float, 4> expected{ 32.25f, -35.5f, 736.0f, 56367744.5f };
		TEST_ASSERT( U_Get_f32_x4_Test0() == expected );
	}
	{
		const std::array<float, 5> expected{ 322.25f, -352.5f, 7316.0f, 5636744.5f, -0.1f };
		TEST_ASSERT( U_Get_f32_x5_Test0() == expected );
	}
	{
		const std::array<float, 6> expected{ 322.75f, 0.0f, -322.5f, -7316.0f, 5632744.75f, -0.2f };
		TEST_ASSERT( U_Get_f32_x6_Test0() == expected );
	}
	{
		const std::array<float, 7> expected{ 1322.75f, -0.0f, -3222.5f, -736.0f, 56344.75f, -4.2f, 874.5f };
		TEST_ASSERT( U_Get_f32_x7_Test0() == expected );
	}
	{
		const std::array<float, 8> expected{ 1322.75f, -0.0f, -3226.5f, -7365.0f, 5634.75f, -43.2f, 8174.5f, 3743477800.0f };
		TEST_ASSERT( U_Get_f32_x8_Test0() == expected );
	}
	{
		const std::array<float, 9> expected{ 322.75f, -0.4f, 563.0f, -32226.5f, -735.0f, 56734.75f, -44.2f, 81274.5f, 3743477801.0f };
		TEST_ASSERT( U_Get_f32_x9_Test0() == expected );
	}
	{
		const auto res= U_Get_f32_x19_Test0();
		for( uint32_t i= 0u; i < 19u; ++i )
			TEST_ASSERT( res[i] == float(i) * float(i) * 13.5f - float(i) * 67.25f + 2647.0f );
	}
	{
		const std::array<double, 1> expected{ 0.0625 };
		TEST_ASSERT( U_Get_f64_x1_Test0() == expected );
	}
	{
		const std::array<double, 2> expected{ -2.0625, 5757.25 };
		TEST_ASSERT( U_Get_f64_x2_Test0() == expected );
	}
	{
		const std::array<double, 3> expected{ 34.0625, 1757.75, 6741663000000000000.0 };
		TEST_ASSERT( U_Get_f64_x3_Test0() == expected );
	}
	{
		const std::array<double, 4> expected{ -34.0625, 1726757.75, -67523676.25, 0.005 };
		TEST_ASSERT( U_Get_f64_x4_Test0() == expected );
	}
	{
		const std::array<double, 5> expected{ 34.0625, 172657.0, -675276.25, 3.005, 643677.2 };
		TEST_ASSERT( U_Get_f64_x5_Test0() == expected );
	}
	TEST_ASSERT( std::memcmp( U_Get_char8_x1_Test0().data(), "H", 1 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x2_Test0().data(), "-8", 2 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x3_Test0().data(), "Kek", 3 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x4_Test0().data(), "SPQR", 4 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x5_Test0().data(), "ApplE", 5 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x6_Test0().data(), "56 to ", 6 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x7_Test0().data(), "@#--ABe", 7 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x8_Test0().data(), "S.P.Q.R.", 8 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x9_Test0().data(), "Жопа!", 9 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x10_Test0().data(), "Black Mesa", 10 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x11_Test0().data(), "qwertyuiop[", 11 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x12_Test0().data(), "][poiuytrewq", 12 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x13_Test0().data(), "Computerliebe", 13 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x14_Test0().data(), "11 + 22 = some", 14 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x15_Test0().data(), "ABCDEFGHIJKLMNO", 15 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x16_Test0().data(), "Er ist wieder da", 16 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x17_Test0().data(), "XY = 33 + 44 - 55", 17 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x32_Test0().data(), "0123456789abcdefFEDCBA9876543210", 32 ) == 0 );
	TEST_ASSERT( std::memcmp( U_Get_char8_x39_Test0().data(), "Ficket euch, ihr beleidigt meine Augen!", 39 ) == 0 );
	{
		const Tuple2<int8_t, int8_t> expected{ 37, -98 };
		TEST_ASSERT( U_Get_tup_i8_i8_Test0() == expected );
	}
	{
		const Tuple2<int8_t, uint16_t> expected{ 97, 35712 };
		TEST_ASSERT( U_Get_tup_i8_u16_Test0() == expected );
	}
	{
		const Tuple2<int8_t, int32_t> expected{ -67, -543467432 };
		TEST_ASSERT( U_Get_tup_i8_i32_Test0() == expected );
	}
	{
		const Tuple2<int8_t, uint64_t> expected{ -67, 0x0022446688AACCEEu };
		TEST_ASSERT( U_Get_tup_i8_u64_Test0() == expected );
	}
#ifdef ENABLE_128BIT_INT_TESTS
	if( false ) // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	{
		const Tuple2<int8_t, __int128_t> expected{ 78, ( __int128_t(0x0022446688AACCEEll) << 64u ) | 0x1133557799BBDDFFll };
		TEST_ASSERT( U_Get_tup_i8_i128_Test0() == expected );
	}
#endif
	{
		const Tuple2<int8_t, float> expected{ 127, 89.5f };
		TEST_ASSERT( U_Get_tup_i8_f32_Test0() == expected );
	}
	{
		const Tuple2<int8_t, double> expected{ -128, -674730004400.0 };
		TEST_ASSERT( U_Get_tup_i8_f64_Test0() == expected );
	}
	{
		const Tuple2<uint16_t, uint8_t> expected{ 0xABCD, 251 };
		TEST_ASSERT( U_Get_tup_u16_u8_Test0() == expected );
	}
	{
		const Tuple2<uint16_t, int16_t> expected{ 0x5432, 30712 };
		TEST_ASSERT( U_Get_tup_u16_i16_Test0() == expected );
	}
	{
		const Tuple2<uint16_t, uint32_t> expected{ 0x0123, 543467432 };
		TEST_ASSERT( U_Get_tup_u16_u32_Test0() == expected );
	}
	{
		const Tuple2<uint16_t, int64_t> expected{ 0xFEDC, 0x0022446688AACCEE };
		TEST_ASSERT( U_Get_tup_u16_i64_Test0() == expected );
	}
#ifdef ENABLE_128BIT_INT_TESTS
	if( false ) // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	{
		const Tuple2<uint16_t, __uint128_t> expected{ 0x6732, ( __uint128_t(0x0022446688AACCEEull) << 64u ) | 0x1133557799BBDDFFull };
		TEST_ASSERT( U_Get_tup_u16_u128_Test0() == expected );
	}
#endif
	{
		const Tuple2<uint16_t, float> expected{ 0xFFF1, 89.5f };
		TEST_ASSERT( U_Get_tup_u16_f32_Test0() == expected );
	}
	{
		const Tuple2<uint16_t, double> expected{ 0x7F5E, -674730004400.0 };
		TEST_ASSERT( U_Get_tup_u16_f64_Test0() == expected );
	}
	{
		const Tuple2<int32_t, int8_t> expected{ 65247547, -98 };
		TEST_ASSERT( U_Get_tup_i32_i8_Test0() == expected );
	}
	{
		const Tuple2<int32_t, uint16_t> expected{ -33467428, 35712 };
		TEST_ASSERT( U_Get_tup_i32_u16_Test0() == expected );
	}
	{
		const Tuple2<int32_t, int32_t> expected{ 78423, -543467432 };
		TEST_ASSERT( U_Get_tup_i32_i32_Test0() == expected );
	}
	{
		const Tuple2<int32_t, uint64_t> expected{ -377848, 0x0022446688AACCEE };
		TEST_ASSERT( U_Get_tup_i32_u64_Test0() == expected );
	}
#ifdef ENABLE_128BIT_INT_TESTS
	if( false ) // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	{
		const Tuple2<int32_t, __int128_t> expected{ 6781134, ( __int128_t(0x0022446688AACCEEll) << 64u ) | 0x1133557799BBDDFFll };
		TEST_ASSERT( U_Get_tup_i32_i128_Test0() == expected );
	}
#endif
	{
		const Tuple2<int32_t, float> expected{ -7712378, 89.5f };
		TEST_ASSERT( U_Get_tup_i32_f32_Test0() == expected );
	}
	{
		const Tuple2<int32_t, double> expected{ 67844881, -674730004400.0 };
		TEST_ASSERT( U_Get_tup_i32_f64_Test0() == expected );
	}
	{
		const Tuple2<uint64_t, uint8_t> expected{ 0x1122334455667788, 251 };
		TEST_ASSERT( U_Get_tup_u64_u8_Test0() == expected );
	}
	{
		const Tuple2<uint64_t, int16_t> expected{ 0x9ABCDEF01234567, 30712 };
		TEST_ASSERT( U_Get_tup_u64_i16_Test0() == expected );
	}
	{
		const Tuple2<uint64_t, uint32_t> expected{ 0xFEDCBA9876543210, 543467432 };
		TEST_ASSERT( U_Get_tup_u64_u32_Test0() == expected );
	}
	{
		const Tuple2<uint64_t, int64_t> expected{ 0x08192A3B4C5D6E7F, 0x0022446688AACCEE };
		TEST_ASSERT( U_Get_tup_u64_i64_Test0() == expected );
	}
#ifdef ENABLE_128BIT_INT_TESTS
	if( false ) // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	{
		const Tuple2<uint64_t, __uint128_t> expected{ 0xF8E6D5C4B3A29180, ( __int128_t(0x0022446688AACCEEull) << 64u ) | 0x1133557799BBDDFFul };
		TEST_ASSERT( U_Get_tup_u64_u128_Test0() == expected );
	}
#endif
	{
		const Tuple2<uint64_t, float> expected{ 0x121234345656787A, 89.5f };
		TEST_ASSERT( U_Get_tup_u64_f32_Test0() == expected );
	}
	{
		const Tuple2<uint64_t, double> expected{ 0x9A9ABCBCDEDEF0F0, -674730004400.0 };
		TEST_ASSERT( U_Get_tup_u64_f64_Test0() == expected );
	}
	{
		const Tuple2<float, int8_t> expected{ 674.1f, -98 };
		TEST_ASSERT( U_Get_tup_f32_i8_Test0() == expected );
	}
	{
		const Tuple2<float, uint16_t> expected{ -0.75f, 35712 };
		TEST_ASSERT( U_Get_tup_f32_u16_Test0() == expected );
	}
	{
		const Tuple2<float, int32_t> expected{ 677.5f, -543467432 };
		TEST_ASSERT( U_Get_tup_f32_i32_Test0() == expected );
	}
	{
		const Tuple2<float, uint64_t> expected{ 12.25f, 0x0022446688AACCEE };
		TEST_ASSERT( U_Get_tup_f32_u64_Test0() == expected );
	}
#ifdef ENABLE_128BIT_INT_TESTS
	if( false ) // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	{
		const Tuple2<float, __int128_t> expected{ 7558.0f, ( __int128_t(0x0022446688AACCEEll) << 64u ) | 0x1133557799BBDDFFll };
		TEST_ASSERT( U_Get_tup_f32_i128_Test0() == expected );
	}
#endif
	{
		const Tuple2<float, float> expected{ -63674777.0f, 89.5f };
		TEST_ASSERT( U_Get_tup_f32_f32_Test0() == expected );
	}
	{
		const Tuple2<float, double> expected{ 2252.5f, -674730004400.0 };
		TEST_ASSERT( U_Get_tup_f32_f64_Test0() == expected );
	}
	{
		const Tuple2<double, uint8_t> expected{ 96.5, 251 };
		TEST_ASSERT( U_Get_tup_f64_u8_Test0() == expected );
	}
	{
		const Tuple2<double, int16_t> expected{ -0.0625, 30712 };
		TEST_ASSERT( U_Get_tup_f64_i16_Test0() == expected );
	}
	{
		const Tuple2<double, uint32_t> expected{ 73377700000.0, 543467432 };
		TEST_ASSERT( U_Get_tup_f64_u32_Test0() == expected );
	}
	{
		const Tuple2<double, int64_t> expected{ -23.2, 0x0022446688AACCEE };
		TEST_ASSERT( U_Get_tup_f64_i64_Test0() == expected );
	}
#ifdef ENABLE_128BIT_INT_TESTS
	if( false ) // Disabled for now. In C++ uint128_t is 16-byte aligned, but in Ü only 8-byte aligned. TODO - fix this.
	{
		const Tuple2<double, __uint128_t> expected{ 7477.5, ( __uint128_t(0x0022446688AACCEEull) << 64u ) | 0x1133557799BBDDFFull };
		TEST_ASSERT( U_Get_tup_f64_u128_Test0() == expected );
	}
#endif
	{
		const Tuple2<double, float> expected{ 733333300000000.0, 89.5f };
		TEST_ASSERT( U_Get_tup_f64_f32_Test0() == expected );
	}
	{
		const Tuple2<double, double> expected{ -6723667.5, -674730004400.0 };
		TEST_ASSERT( U_Get_tup_f64_f64_Test0() == expected );
	}
	{
		const Tuple3<uint32_t, uint16_t, uint8_t> expected{ 0x01234567, 0x89AB, 0xCD };
		TEST_ASSERT( U_Get_tup_u32_u16_u8_Test0() == expected );
	}
	{
		const Tuple3<uint32_t, uint16_t, uint16_t> expected{ 0x01234567, 0x89AB, 0xCDEF };
		TEST_ASSERT( U_Get_tup_u32_u16_u16_Test0() == expected );
	}
	{
		const Tuple3<uint8_t, uint16_t, uint32_t> expected{ 0x01, 0x2345, 0x6789ABCD };
		TEST_ASSERT( U_Get_tup_u8_u16_u32_Test0() == expected );
	}
	{
		const Tuple3<uint16_t, uint16_t, uint32_t> expected{ 0x0123, 0x4567, 0x89ABCDEF };
		TEST_ASSERT( U_Get_tup_u16_u16_u32_Test0() == expected );
	}
	{
		const Tuple4<uint64_t, uint32_t, uint16_t, uint8_t> expected{ 0xFEDCBA9876543210, 0x01234567, 0x89AB, 0xCD };
		TEST_ASSERT( U_Get_tup_u64_u32_u16_u8_Test0() == expected );
	}
	{
		const Tuple4<uint64_t, uint32_t, uint16_t, uint16_t> expected{ 0xFEDCBA9876543210, 0x01234567, 0x89AB, 0xCDEF };
		TEST_ASSERT( U_Get_tup_u64_u32_u16_u16_Test0() == expected );
	}
	{
		const Tuple4<uint8_t, uint16_t, uint32_t, uint64_t> expected{ 0x01, 0x2345, 0x6789ABCD, 0xFEDCBA9876543210 };
		TEST_ASSERT( U_Get_tup_u8_u16_u32_u64_Test0() == expected );
	}
	{
		const Tuple4<uint16_t, uint16_t, uint32_t, uint64_t> expected{ 0x0123, 0x4567, 0x89ABCDEF, 0xFEDCBA9876543210 };
		TEST_ASSERT( U_Get_tup_u16_u16_u32_u64_Test0() == expected );
	}
	{
		const Tuple3<uint8_t, uint16_t, uint8_t> expected{ 0xFE, 0xDCBA, 0x98 };
		TEST_ASSERT( U_Get_tup_u8_u16_u8_Test0() == expected );
	}
	{
		const Tuple3<uint8_t, uint32_t, uint8_t> expected{ 0xFE, 0xDCBA9876, 0x54 };
		TEST_ASSERT( U_Get_tup_u8_u32_u8_Test0() == expected );
	}
	{
		const Tuple3<uint8_t, uint64_t, uint8_t> expected{ 0xAB, 0x0123456789ABCDEFu, 0x12 };
		TEST_ASSERT( U_Get_tup_u8_u64_u8_Test0() == expected );
	}
	{
		const Tuple3<uint16_t, uint32_t, uint16_t> expected{ 0x0123, 0x456789AB, 0xCDEF };
		TEST_ASSERT( U_Get_tup_u16_u32_u16_Test0() == expected );
	}
	{
		const Tuple3<uint16_t, uint64_t, uint16_t> expected{ 0xFEDC, 0x17283A4B5C6D7E8F, 0x9876 };
		TEST_ASSERT( U_Get_tup_u16_u64_u16_Test0() == expected );
	}
	{
		const Tuple3<uint32_t, uint64_t, uint32_t> expected{ 0x01234567, 0x17283A4B5C6D7E8F, 0x89ABCEDF };
		TEST_ASSERT( U_Get_tup_u32_u64_u32_Test0() == expected );
	}
	{
		const Tuple3<float, int32_t, int32_t> expected{ 123.45f, 266747477, -963237321 };
		TEST_ASSERT( U_Get_tup_f32_i32_i32_Test0() == expected );
	}
	{
		const Tuple3<int32_t, float, int32_t> expected{ -196323732, 236.5f, 266745477 };
		TEST_ASSERT( U_Get_tup_i32_f32_i32_Test0() == expected );
	}
	{
		const Tuple3<int32_t, int32_t, float> expected{ 196323735, 166745427, -0.7f };
		TEST_ASSERT( U_Get_tup_i32_i32_f32_Test0() == expected );
	}
	{
		const Tuple3<float, uint64_t, uint64_t> expected{ 323.25f, 0x64AB3C5482367DE3u, 0x17283A4B5C6D7E8Fu };
		TEST_ASSERT( U_Get_tup_f32_u64_u64_Test0() == expected );
	}
	{
		const Tuple3<uint64_t, float, uint64_t> expected{ 0x7637347A36B4E218u, 1336.5f, 0x067374735AE7DFC13u };
		TEST_ASSERT( U_Get_tup_u64_f32_u64_Test0() == expected );
	}
	{
		const Tuple3<uint64_t, uint64_t, float> expected{ 0x27283A4B5C637E8Fu, 0xE637347436B47218u, 4.7f };
		TEST_ASSERT( U_Get_tup_u64_u64_f32_Test0() == expected );
	}
	{
		const Tuple3<double, int32_t, int32_t> expected{ 123.45, 266747477, -963237321 };
		TEST_ASSERT( U_Get_tup_f64_i32_i32_Test0() == expected );
	}
	{
		const Tuple3<int32_t, double, int32_t> expected{ -196323732, 236.5, 266745477 };
		TEST_ASSERT( U_Get_tup_i32_f64_i32_Test0() == expected );
	}
	{
		const Tuple3<int32_t, int32_t, double> expected{ 196323735, 166745427, -0.7 };
		TEST_ASSERT( U_Get_tup_i32_i32_f64_Test0() == expected );
	}
	{
		const Tuple3<double, uint64_t, uint64_t> expected{ 323.25, 0x64AB3C5482367DE3u, 0x17283A4B5C6D7E8Fu };
		TEST_ASSERT( U_Get_tup_f64_u64_u64_Test0() == expected );
	}
	{
		const Tuple3<uint64_t, double, uint64_t> expected{ 0x7637347A36B4E218u, 1336.5, 0x067374735AE7DFC13u };
		TEST_ASSERT( U_Get_tup_u64_f64_u64_Test0() == expected );
	}
	{
		const Tuple3<uint64_t, uint64_t, double> expected{ 0x27283A4B5C637E8Fu, 0xE637347436B47218u, 4.7 };
		TEST_ASSERT( U_Get_tup_u64_u64_f64_Test0() == expected );
	}
	{
		const Tuple3<float, float, float> expected{ 0.7f, 67567.5f, -256733770.0f };
		TEST_ASSERT( U_Get_tup_f32_f32_f32_Test0() == expected );
	}
	{
		const Tuple3<float, float, double> expected{ 0.7f, 67567.5f, -256733770.0 };
		TEST_ASSERT( U_Get_tup_f32_f32_f64_Test0() == expected );
	}
	{
		const Tuple3<float, double, float> expected{ 0.7f, 67567.5, -256733770.0f };
		TEST_ASSERT( U_Get_tup_f32_f64_f32_Test0() == expected );
	}
	{
		const Tuple3<float, double, double> expected{ 0.7f, 67567.5, -256733770.0 };
		TEST_ASSERT( U_Get_tup_f32_f64_f64_Test0() == expected );
	}
	{
		const Tuple3<double, float, float> expected{ 0.7, 67567.5f, -256733770.0f };
		TEST_ASSERT( U_Get_tup_f64_f32_f32_Test0() == expected );
	}
	{
		const Tuple3<double, float, double> expected{ 0.7, 67567.5f, -256733770.0 };
		TEST_ASSERT( U_Get_tup_f64_f32_f64_Test0() == expected );
	}
	{
		const Tuple3<double, double, float> expected{ 0.7, 67567.5, -256733770.0f };
		TEST_ASSERT( U_Get_tup_f64_f64_f32_Test0() == expected );
	}
	{
		const Tuple3<double, double, double> expected{ 0.7, 67567.5, -256733770.0 };
		TEST_ASSERT( U_Get_tup_f64_f64_f64_Test0() == expected );
	}
}

int8_t Get_i8_Test0() { return 117; }
int8_t Get_i8_Test1() { return -24; }
uint8_t Get_u8_Test0() { return 104; }
uint8_t Get_u8_Test1() { return 231; }
int16_t Get_i16_Test0() { return 12361; }
int16_t Get_i16_Test1() { return -25331; }
uint16_t Get_u16_Test0() { return 22365; }
uint16_t Get_u16_Test1() { return 43652; }
int32_t Get_i32_Test0() { return 534875478; }
int32_t Get_i32_Test1() { return -34745344; }
uint32_t Get_u32_Test0() { return 0x35B6E36Fu; }
uint32_t Get_u32_Test1() { return 0xE54EC57Fu; }
int64_t Get_i64_Test0() { return 474247578522; }
int64_t Get_i64_Test1() { return -65433774422444; }
uint64_t Get_u64_Test0() { return 0x35B6E36F4E8FEC37u; }
uint64_t Get_u64_Test1() { return 0xE54EC57F1E070FC1u; }
#ifdef ENABLE_128BIT_INT_TESTS
__int128_t Get_i128_Test0() { return ( __int128_t( 0x0123456789ABCDEFull) << 64u ) | 0xFEDCBA9876543210ull; }
__uint128_t Get_u128_Test0() { return ( __uint128_t(0xFEDCBA9876543210ull) << 64u ) | 0x0123456789ABCDEFull; }
#endif
// Use "unsigned char" for represent Ü "char8", since in C++ regular char signess is inplementation defined.
unsigned char Get_char8_Test0() { return 'n'; }
unsigned char Get_char8_Test1() { return 145; }
char16_t Get_char16_Test0() { return u'Й'; }
char16_t Get_char16_Test1() { return u'Ꙥ'; }
char32_t Get_char32_Test0() { return U'😀'; }
float Get_f32_Test0() { return 642.05f; }
float Get_f32_Test1() { return -0.012f; }
double Get_f64_Test0() { return 544747366.75; }
double Get_f64_Test1() { return -34.25; }
std::array<int8_t, 1> Get_i8_x1_Test0() { return { -95 }; }
std::array<int8_t, 2> Get_i8_x2_Test0() { return { 107, -34 }; }
std::array<int8_t, 3> Get_i8_x3_Test0() { return { -65, 77, 4 }; }
std::array<int8_t, 4> Get_i8_x4_Test0() { return { 37, 0, 127, -25 }; }
std::array<int8_t, 5> Get_i8_x5_Test0() { return { 67, 3, 127, -25, 88 }; }
std::array<int8_t, 6> Get_i8_x6_Test0() { return { 37, -63, 127, -25, 125, -107 }; }
std::array<int8_t, 7> Get_i8_x7_Test0() { return { 117, 27, -93, -21, 125, -107, 72 }; }
std::array<int8_t, 8> Get_i8_x8_Test0() { return { 34, 112, 73, -21, 125, -107, 52, -94 }; }
std::array<int8_t, 9> Get_i8_x9_Test0() { return { 74, 126, 67, 73, -21, 125, -107, 53, -92 }; }
std::array<int8_t, 10> Get_i8_x10_Test0() { return { 24, 120, 54, 73, -21, 105, -89, -107, 53, -93 }; }
std::array<int8_t, 11> Get_i8_x11_Test0() { return { 29, 99, 120, 54, 78, -121, 105, -85, -107, 53, -94 }; }
std::array<int8_t, 12> Get_i8_x12_Test0() { return { 69, 95, 120, 34, 78, -121, 105, -87, -107, 48, 63, -98 }; }
std::array<int8_t, 13> Get_i8_x13_Test0() { return { 62, 95, 120, 31, 78, -121, 76, 105, -87, 107, 48, 64, -58 }; }
std::array<int8_t, 14> Get_i8_x14_Test0() { return { 78, 61, 94, 121, 31, 72, -121, 78, 125, -87, 107, 48, 14, -28 }; }
std::array<int8_t, 15> Get_i8_x15_Test0() { return { 79, 61, 94, 121, 121, 52, -121, 78, -5, 125, -87, 107, 48, 24, -7 }; }
std::array<int8_t, 16> Get_i8_x16_Test0() { return { 59, 61, 84, 121, 121, 55, -121, 78, 74, -51, 24, -87, 107, 48, 24, -73 }; }
std::array<int8_t, 17> Get_i8_x17_Test0() { return { 69, 65, 82, 101, 121, 55, -121, 0, 78, 74, -56, 24, -87, 104, 58, 34, -93 }; }
std::array<int8_t, 35> Get_i8_x35_Test0()
{
	std::array<int8_t, 35> res;
	for( int32_t i= 0; i < 35; ++i )
		res[ size_t(i) ]= int8_t( i * i - 5 * i - 2 );
	return res;
}
std::array<uint16_t, 1> Get_u16_x1_Test0() { return { 0xABCD }; }
std::array<uint16_t, 2> Get_u16_x2_Test0() { return { 0xEF01, 0x2345 }; }
std::array<uint16_t, 3> Get_u16_x3_Test0() { return { 0x6789, 0xABCD, 0xEF01 }; }
std::array<uint16_t, 4> Get_u16_x4_Test0() { return { 0x2345, 0x6789, 0xABCD, 0xEF01 }; }
std::array<uint16_t, 5> Get_u16_x5_Test0() { return { 0x2233, 0x4455, 0x6677, 0x8899, 0xAABB }; }
std::array<uint16_t, 6> Get_u16_x6_Test0() { return { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666 }; }
std::array<uint16_t, 7> Get_u16_x7_Test0() { return { 0x0123, 0x1234, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789 }; }
std::array<uint16_t, 8> Get_u16_x8_Test0() { return { 0xFEDC, 0xBA98, 0x7654, 0x3210, 0xDEAD, 0xC0DE, 0xB00B, 0xEBA0 }; }
std::array<uint16_t, 9> Get_u16_x9_Test0() { return { 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0xAAAA, 0xBBBB, 0xCCCC }; }
std::array<uint16_t, 15> Get_u16_x15_Test0() { return { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE, 0xFFFF }; }
std::array<uint16_t, 21> Get_u16_x21_Test0()
{
	std::array<uint16_t, 21> res;
	for( uint32_t i= 0; i < 21; ++i )
		res[ size_t(i) ]= uint16_t( i * i * 15u + i * 23u + 17u );
	return res;
}
std::array<int32_t, 1> Get_i32_x1_Test0() { return { 547718243 }; }
std::array<int32_t, 2> Get_i32_x2_Test0() { return { -4744785, 1236317 }; }
std::array<int32_t, 3> Get_i32_x3_Test0() { return { 78482236, 847587447, 289378855 }; }
std::array<int32_t, 4> Get_i32_x4_Test0() { return { -66366, 2667377, -1674, 757233 }; }
std::array<int32_t, 5> Get_i32_x5_Test0() { return { -6353, -61366, 2667371, -2674, 5757233  }; }
std::array<int32_t, 6> Get_i32_x6_Test0() { return { 5757233, -6353, 616613, 23521, 2667371, -2677834 }; }
std::array<int32_t, 7> Get_i32_x7_Test0() { return { 5757213, -63543, 616513, 231521, 266371, -26778234, 289378855 }; }
std::array<int32_t, 8> Get_i32_x8_Test0() { return { 157573, -613543, 6113, 231561, 2661371, -26778234, 289237855, -2634 }; }
std::array<int32_t, 9> Get_i32_x9_Test0() { return { 15773, -613543, 6116, 211561, 2651371, -3412, -26778234, 289737855, -263 }; }
std::array<int32_t, 18> Get_i32_x18_Test0()
{
	std::array<int32_t, 18> res;
	for( int32_t i= 0; i < 18; ++i )
		res[ size_t(i) ]= i * i * 752 + 6447 * i + 6437784;
	return res;
}
std::array<uint64_t, 1> Get_u64_x1_Test0() { return { 0x0123456789ABCDEFu }; }
std::array<uint64_t, 2> Get_u64_x2_Test0() { return { 73434784882478588u, 378824886678u }; }
std::array<uint64_t, 3> Get_u64_x3_Test0() { return { 378824816678u, 73436784882478588u, 74784785858885u }; }
std::array<uint64_t, 4> Get_u64_x4_Test0() { return { 378824816178u, 734367848822478588u, 74784785888885u, 75443787589u }; }
std::array<uint64_t, 5> Get_u64_x5_Test0() { return { 378824816171u, 734367842822478588u, 74784785888883u, 754243787589u, 67437858898u }; }
#ifdef ENABLE_128BIT_INT_TESTS
std::array<__int128_t, 1> Get_i128_x1_Test0() { return { ( __int128_t(0x0123456789ABCDEFll) << 64u ) | 6458589734899ll }; }
std::array<__int128_t, 2> Get_i128_x2_Test0() { return { ( __int128_t(0x3123456789AeCDEFll) << 64u ) | 6428589734399ll, ( __int128_t(0x5123456789ABFDEFll) << 64u ) | 6453589734892ll }; }
std::array<__int128_t, 3> Get_i128_x3_Test0() { return { ( __int128_t(0x3123256789AeCDEFll) << 64u ) | 6428589754399ll, ( __int128_t(0x51234567896BFDEFll) << 64u ) | 6453582734892ll, ( __int128_t(0x0123456789ABCDEFll) << 64u ) | 6458589734899ll }; }
#endif
std::array<float, 1> Get_f32_x1_Test0() { return { 0.125f }; }
std::array<float, 2> Get_f32_x2_Test0() { return { -2.125f, 3.5f }; }
std::array<float, 3> Get_f32_x3_Test0() { return { -22.125f, 35.5f, 7336.0f  }; }
std::array<float, 4> Get_f32_x4_Test0() { return { 32.25f, -35.5f, 736.0f, 56367744.5f }; }
std::array<float, 5> Get_f32_x5_Test0() { return { 322.25f, -352.5f, 7316.0f, 5636744.5f, -0.1f }; }
std::array<float, 6> Get_f32_x6_Test0() { return { 322.75f, 0.0f, -322.5f, -7316.0f, 5632744.75f, -0.2f }; }
std::array<float, 7> Get_f32_x7_Test0() { return { 1322.75f, -0.0f, -3222.5f, -736.0f, 56344.75f, -4.2f, 874.5f }; }
std::array<float, 8> Get_f32_x8_Test0() { return { 1322.75f, -0.0f, -3226.5f, -7365.0f, 5634.75f, -43.2f, 8174.5f, 3743477800.0f }; }
std::array<float, 9> Get_f32_x9_Test0() { return { 322.75f, -0.4f, 563.0f, -32226.5f, -735.0f, 56734.75f, -44.2f, 81274.5f, 3743477801.0f }; }
std::array<float, 19> Get_f32_x19_Test0()
{
	std::array<float, 19> res;
	for( uint32_t i= 0; i < 19u; ++i )
		res[i]= float(i) * float(i) * 13.5f - float(i) * 67.25f + 2647.0f;
	return res;
}
std::array<double, 1> Get_f64_x1_Test0() { return { 0.0625 }; }
std::array<double, 2> Get_f64_x2_Test0() { return { -2.0625, 5757.25 }; }
std::array<double, 3> Get_f64_x3_Test0() { return { 34.0625, 1757.75, 6741663000000000000.0 }; }
std::array<double, 4> Get_f64_x4_Test0() { return { -34.0625, 1726757.75, -67523676.25, 0.005 }; }
std::array<double, 5> Get_f64_x5_Test0() { return { 34.0625, 172657.0, -675276.25, 3.005, 643677.2 }; }
std::array<char, 1> Get_char8_x1_Test0() { return { 'H' }; }
std::array<char, 2> Get_char8_x2_Test0() { return { '-', '8' }; }
std::array<char, 3> Get_char8_x3_Test0() { return { 'K', 'e', 'k' }; }
std::array<char, 4> Get_char8_x4_Test0() { return { 'S', 'P', 'Q', 'R' }; }
std::array<char, 5> Get_char8_x5_Test0() { return { 'A', 'p', 'p', 'l', 'E' }; }
std::array<char, 6> Get_char8_x6_Test0() { return { '5', '6', ' ', 't', 'o', ' ' }; }
std::array<char, 7> Get_char8_x7_Test0() { return { '@', '#', '-', '-', 'A', 'B', 'e' }; }
std::array<char, 8> Get_char8_x8_Test0() { return { 'S', '.', 'P', '.', 'Q', '.', 'R', '.' }; }
std::array<char, 9> Get_char8_x9_Test0()
{
	std::array<char, 9> res;
	std::memcpy( res.data(), "Жопа!", 9 );
	return res;
}
std::array<char, 10> Get_char8_x10_Test0() { return { 'B', 'l', 'a', 'c', 'k', ' ', 'M', 'e', 's', 'a' }; }
std::array<char, 11> Get_char8_x11_Test0() { return { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[' }; }
std::array<char, 12> Get_char8_x12_Test0() { return { ']', '[', 'p', 'o', 'i', 'u', 'y', 't', 'r', 'e', 'w', 'q' }; }
std::array<char, 13> Get_char8_x13_Test0() { return { 'C', 'o', 'm', 'p', 'u', 't', 'e', 'r', 'l', 'i', 'e', 'b', 'e' }; }
std::array<char, 14> Get_char8_x14_Test0() { return { '1', '1', ' ', '+', ' ', '2', '2', ' ', '=', ' ', 's', 'o', 'm', 'e' }; }
std::array<char, 15> Get_char8_x15_Test0() { return { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O' }; }
std::array<char, 16> Get_char8_x16_Test0() { return { 'E', 'r', ' ', 'i', 's', 't', ' ', 'w', 'i', 'e', 'd', 'e', 'r', ' ', 'd', 'a' }; }
std::array<char, 17> Get_char8_x17_Test0() { return { 'X', 'Y', ' ', '=', ' ', '3', '3', ' ', '+', ' ', '4', '4', ' ', '-', ' ', '5', '5' }; }
std::array<char, 32> Get_char8_x32_Test0() { return { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'F', 'E', 'D', 'C', 'B', 'A', '9', '8', '7', '6', '5', '4', '3', '2', '1', '0' }; }
std::array<char, 39> Get_char8_x39_Test0() { return { 'F', 'i', 'c', 'k', 'e', 't', ' ', 'e', 'u', 'c', 'h', ',', ' ', 'i', 'h', 'r', ' ', 'b', 'e', 'l', 'e', 'i', 'd', 'i', 'g', 't', ' ', 'm', 'e', 'i', 'n', 'e', ' ', 'A', 'u', 'g', 'e', 'n', '!' }; }
Tuple2<int8_t, int8_t> Get_tup_i8_i8_Test0() { return { 37, -98 }; }
Tuple2<int8_t, uint16_t> Get_tup_i8_u16_Test0() { return { 97, 35712 }; }
Tuple2<int8_t, int32_t> Get_tup_i8_i32_Test0() { return { -67, -543467432 }; }
Tuple2<int8_t, uint64_t> Get_tup_i8_u64_Test0() { return { -67, 0x0022446688AACCEE }; }
#ifdef ENABLE_128BIT_INT_TESTS
Tuple2<int8_t, __int128_t> Get_tup_i8_i128_Test0() { return { 78, ( __int128_t( 0x0022446688AACCEEll ) << 64u ) | 0x1133557799BBDDFFll }; }
#endif
Tuple2<int8_t, float> Get_tup_i8_f32_Test0() { return { 127, 89.5f }; }
Tuple2<int8_t, double> Get_tup_i8_f64_Test0() { return { -128, -674730004400.0 }; }
Tuple2<uint16_t, uint8_t> Get_tup_u16_u8_Test0() { return { 0xABCD, 251 }; }
Tuple2<uint16_t, int16_t> Get_tup_u16_i16_Test0() { return { 0x5432, 30712 }; }
Tuple2<uint16_t, uint32_t> Get_tup_u16_u32_Test0() { return { 0x0123, 543467432 }; }
Tuple2<uint16_t, int64_t> Get_tup_u16_i64_Test0() { return { 0xFEDC, 0x0022446688AACCEE }; }
#ifdef ENABLE_128BIT_INT_TESTS
Tuple2<uint16_t, __uint128_t> Get_tup_u16_u128_Test0() { return { 0x6732, ( __uint128_t( 0x0022446688AACCEEull ) << 64u ) | 0x1133557799BBDDFFull }; }
#endif
Tuple2<uint16_t, float> Get_tup_u16_f32_Test0() { return { 0xFFF1, 89.5f }; }
Tuple2<uint16_t, double> Get_tup_u16_f64_Test0() { return { 0x7F5E, -674730004400.0 }; }
Tuple2<int32_t, int8_t> Get_tup_i32_i8_Test0() { return { 65247547, -98 }; }
Tuple2<int32_t, uint16_t> Get_tup_i32_u16_Test0() { return { -33467428, 35712 }; }
Tuple2<int32_t, int32_t> Get_tup_i32_i32_Test0() { return { 78423, -543467432 }; }
Tuple2<int32_t, uint64_t> Get_tup_i32_u64_Test0() { return { -377848, 0x0022446688AACCEE }; }
#ifdef ENABLE_128BIT_INT_TESTS
Tuple2<int32_t, __int128_t> Get_tup_i32_i128_Test0() { return { 6781134, ( __int128_t( 0x0022446688AACCEEll ) << 64u ) | 0x1133557799BBDDFFll }; }
#endif
Tuple2<int32_t, float> Get_tup_i32_f32_Test0() { return { -7712378, 89.5f }; }
Tuple2<int32_t, double> Get_tup_i32_f64_Test0() { return { 67844881, -674730004400.0 }; }
Tuple2<uint64_t, uint8_t> Get_tup_u64_u8_Test0() { return { 0x1122334455667788u, 251 }; }
Tuple2<uint64_t, int16_t> Get_tup_u64_i16_Test0() { return { 0x9ABCDEF01234567u, 30712 }; }
Tuple2<uint64_t, uint32_t> Get_tup_u64_u32_Test0() { return { 0xFEDCBA9876543210u, 543467432 }; }
Tuple2<uint64_t, int64_t> Get_tup_u64_i64_Test0() { return { 0x08192A3B4C5D6E7Fu, 0x0022446688AACCEE }; }
#ifdef ENABLE_128BIT_INT_TESTS
Tuple2<uint64_t, __uint128_t> Get_tup_u64_u128_Test0() { return { 0xF8E6D5C4B3A29180u, ( __uint128_t( 0x0022446688AACCEEull ) << 64u ) | 0x1133557799BBDDFFull }; }
#endif
Tuple2<uint64_t, float> Get_tup_u64_f32_Test0() { return { 0x121234345656787Au, 89.5f }; }
Tuple2<uint64_t, double> Get_tup_u64_f64_Test0() { return { 0x9A9ABCBCDEDEF0F0u, -674730004400.0 }; }
Tuple2<float, int8_t> Get_tup_f32_i8_Test0() { return { 674.1f, -98 }; }
Tuple2<float, uint16_t> Get_tup_f32_u16_Test0() { return { -0.75f, 35712 }; }
Tuple2<float, int32_t> Get_tup_f32_i32_Test0() { return { 677.5f, -543467432 }; }
Tuple2<float, uint64_t> Get_tup_f32_u64_Test0() { return { 12.25f, 0x0022446688AACCEE }; }
#ifdef ENABLE_128BIT_INT_TESTS
Tuple2<float, __int128_t> Get_tup_f32_i128_Test0() { return { 7558.0f, ( __int128_t( 0x0022446688AACCEEll ) << 64u ) | 0x1133557799BBDDFFll }; }
#endif
Tuple2<float, float> Get_tup_f32_f32_Test0() { return { -63674777.0f, 89.5f }; }
Tuple2<float, double> Get_tup_f32_f64_Test0() { return { 2252.5f, -674730004400.0 }; }
Tuple2<double, uint8_t> Get_tup_f64_u8_Test0() { return { 96.5, 251 }; }
Tuple2<double, int16_t> Get_tup_f64_i16_Test0() { return { -0.0625, 30712 }; }
Tuple2<double, uint32_t> Get_tup_f64_u32_Test0() { return { 73377700000.0, 543467432 }; }
Tuple2<double, int64_t> Get_tup_f64_i64_Test0() { return { -23.2, 0x0022446688AACCEE }; }
#ifdef ENABLE_128BIT_INT_TESTS
Tuple2<double, __uint128_t> Get_tup_f64_u128_Test0() { return { 7477.5, ( __uint128_t( 0x0022446688AACCEEull ) << 64u ) | 0x1133557799BBDDFFull }; }
#endif
Tuple2<double, float> Get_tup_f64_f32_Test0() { return { 733333300000000.0, 89.5f }; }
Tuple2<double, double> Get_tup_f64_f64_Test0() { return { -6723667.5, -674730004400.0 }; }
Tuple3<uint32_t, uint16_t, uint8_t> Get_tup_u32_u16_u8_Test0() { return { 0x01234567, 0x89AB, 0xCD }; }
Tuple3<uint32_t, uint16_t, uint16_t> Get_tup_u32_u16_u16_Test0() { return { 0x01234567, 0x89ABu, 0xCDEF }; }
Tuple3<uint8_t, uint16_t, uint32_t> Get_tup_u8_u16_u32_Test0() { return { 0x01, 0x2345, 0x6789ABCD }; }
Tuple3<uint16_t, uint16_t, uint32_t> Get_tup_u16_u16_u32_Test0() { return { 0x0123, 0x4567, 0x89ABCDEF }; }
Tuple4<uint64_t, uint32_t, uint16_t, uint8_t> Get_tup_u64_u32_u16_u8_Test0() { return { 0xFEDCBA9876543210, 0x01234567, 0x89AB, 0xCD }; }
Tuple4<uint64_t, uint32_t, uint16_t, uint16_t> Get_tup_u64_u32_u16_u16_Test0() { return { 0xFEDCBA9876543210, 0x01234567, 0x89AB, 0xCDEF }; }
Tuple4<uint8_t, uint16_t, uint32_t, uint64_t> Get_tup_u8_u16_u32_u64_Test0() { return { 0x01, 0x2345, 0x6789ABCD, 0xFEDCBA9876543210 }; }
Tuple4<uint16_t, uint16_t, uint32_t, uint64_t> Get_tup_u16_u16_u32_u64_Test0() { return { 0x0123, 0x4567, 0x89ABCDEF, 0xFEDCBA9876543210 }; }
Tuple3<uint8_t, uint16_t, uint8_t> Get_tup_u8_u16_u8_Test0() { return { 0xFE, 0xDCBA, 0x98 }; }
Tuple3<uint8_t, uint32_t, uint8_t> Get_tup_u8_u32_u8_Test0() { return { 0xFE, 0xDCBA9876, 0x54 }; }
Tuple3<uint8_t, uint64_t, uint8_t> Get_tup_u8_u64_u8_Test0() { return { 0xAB, 0x0123456789ABCDEF, 0x12 }; }
Tuple3<uint16_t, uint32_t, uint16_t> Get_tup_u16_u32_u16_Test0() { return { 0x0123, 0x456789AB, 0xCDEF }; }
Tuple3<uint16_t, uint64_t, uint16_t> Get_tup_u16_u64_u16_Test0() { return { 0xFEDC, 0x17283A4B5C6D7E8F, 0x9876 }; }
Tuple3<uint32_t, uint64_t, uint32_t> Get_tup_u32_u64_u32_Test0() { return { 0x01234567, 0x17283A4B5C6D7E8F, 0x89ABCEDF }; }
Tuple3<float, int32_t, int32_t> Get_tup_f32_i32_i32_Test0() { return { 123.45f, 266747477, -963237321 }; }
Tuple3<int32_t, float, int32_t> Get_tup_i32_f32_i32_Test0() { return { -196323732, 236.5f, 266745477  }; }
Tuple3<int32_t, int32_t, float> Get_tup_i32_i32_f32_Test0() { return { 196323735, 166745427, -0.7f }; }
Tuple3<float, uint64_t, uint64_t> Get_tup_f32_u64_u64_Test0() { return { 323.25f, 0x64AB3C5482367DE3u, 0x17283A4B5C6D7E8Fu }; }
Tuple3<uint64_t, float, uint64_t> Get_tup_u64_f32_u64_Test0() { return { 0x7637347A36B4E218u, 1336.5f, 0x067374735AE7DFC13u }; }
Tuple3<uint64_t, uint64_t, float> Get_tup_u64_u64_f32_Test0() { return { 0x27283A4B5C637E8Fu, 0xE637347436B47218u, 4.7f }; }
Tuple3<double, int32_t, int32_t> Get_tup_f64_i32_i32_Test0() { return { 123.45, 266747477, -963237321 }; }
Tuple3<int32_t, double, int32_t> Get_tup_i32_f64_i32_Test0() { return { -196323732, 236.5, 266745477 }; }
Tuple3<int32_t, int32_t, double> Get_tup_i32_i32_f64_Test0() { return { 196323735, 166745427, -0.7 }; }
Tuple3<double, uint64_t, uint64_t> Get_tup_f64_u64_u64_Test0() { return { 323.25, 0x64AB3C5482367DE3u, 0x17283A4B5C6D7E8Fu }; }
Tuple3<uint64_t, double, uint64_t> Get_tup_u64_f64_u64_Test0() { return { 0x7637347A36B4E218u, 1336.5, 0x067374735AE7DFC13u }; }
Tuple3<uint64_t, uint64_t, double> Get_tup_u64_u64_f64_Test0() { return { 0x27283A4B5C637E8Fu, 0xE637347436B47218u, 4.7 }; }
Tuple3<float, float, float> Get_tup_f32_f32_f32_Test0(){ return { 0.7f, 67567.5f, -256733770.0f }; }
Tuple3<float, float, double> Get_tup_f32_f32_f64_Test0(){ return { 0.7f, 67567.5f, -256733770.0 }; }
Tuple3<float, double, float> Get_tup_f32_f64_f32_Test0(){ return { 0.7f, 67567.5, -256733770.0f }; }
Tuple3<float, double, double> Get_tup_f32_f64_f64_Test0(){ return { 0.7f, 67567.5, -256733770.0 }; }
Tuple3<double, float, float> Get_tup_f64_f32_f32_Test0(){ return { 0.7, 67567.5f, -256733770.0f }; }
Tuple3<double, float, double> Get_tup_f64_f32_f64_Test0(){ return { 0.7, 67567.5f, -256733770.0 }; }
Tuple3<double, double, float> Get_tup_f64_f64_f32_Test0(){ return { 0.7, 67567.5, -256733770.0f }; }
Tuple3<double, double, double> Get_tup_f64_f64_f64_Test0(){ return { 0.7, 67567.5, -256733770.0 }; }

} // extern "C"
