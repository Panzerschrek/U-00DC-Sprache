#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#define TEST_ASSERT(x) { if( !(x) ) { std::cerr << "Line " << __LINE__ << " assertion failed: " << #x << std::endl; std::abort(); } }

// std::tuple isn't POD, so, use our own replacement.
template<typename T0> struct Tuple1 { T0 v0; };
template<typename T0, typename T1> struct Tuple2{ T0 v0; T1 v1; };
template<typename T0, typename T1, typename T2> struct Tuple3{ T0 v0; T1 v1; T2 v2; };

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
	TEST_ASSERT( x4 == 0 ); TEST_ASSERT( x5 == 0x7FFFFFFF ); TEST_ASSERT( x6 == 54 ); TEST_ASSERT( x7 == -int32_t(0x80000000) );
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
void Pass_i128_Test0( const __int128_t x ) { TEST_ASSERT( x == ( ( __int128_t( 0x0123456789ABCDEFll ) << 64u ) | __int128_t(0xFEDCBA9876543210ll) ) ); }
void Pass_u128_Test0( const __uint128_t x ) { TEST_ASSERT( x == ( ( __uint128_t( 0xFEDCBA9876543210ull ) << 64u ) | __uint128_t(0x0123456789ABCDEFull) ) ); }
void Pass_char8_Test0( const char x ) { TEST_ASSERT( x == 'Q' ); }
void Pass_char8_Test1( const char x ) { TEST_ASSERT( x == '!' ); }
void Pass_char8_Test2( const char x ) { TEST_ASSERT( x == ' ' ); }
void Pass_char8_Test3( const char x ) { TEST_ASSERT( x == char(240) ); }
void Pass_char16_Test0( const char16_t x ) { TEST_ASSERT( x == u'Ж' ); }
void Pass_char16_Test1( const char16_t x ) { TEST_ASSERT( x == u'Ꙥ' ); }
void Pass_char32_Test0( const char32_t x ) { TEST_ASSERT( x == U'😀' ); }
void Pass_f32_Test0( const float x ) { TEST_ASSERT( x == 0.0f ); }
void Pass_f32_Test1( const float x ) { TEST_ASSERT( x == 0.125f ); }
void Pass_f32_Test2( const float x ) { TEST_ASSERT( x == 6743.5f ); }
void Pass_f32_Test3( const float x ) { TEST_ASSERT( x == -7689543378437.0f ); }
void Pass_f32_Test4( const float x ) { TEST_ASSERT( x == 1.0f / 0.0f ); }
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
void Pass_f64_Test4( const double x ) { TEST_ASSERT( x == 1.0 / 0.0 ); }
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
		TEST_ASSERT( x[i] == i * i * i * 337547u + i * i * i * 563454548u + 34565224787u );
	}
}
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

} // extern "C"
