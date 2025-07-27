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
template<typename T0, typename T1, typename T2, typename T3> struct Tuple4{ T0 v0; T1 v1; T2 v2; T3 v3; };

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
	TEST_ASSERT( x4 == 0 ); TEST_ASSERT( x5 == 0x7FFFFFFF ); TEST_ASSERT( x6 == 54 ); TEST_ASSERT( x7 == int32_t(-0x80000000) );
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
void U_Pass_i128_Test0( __int128_t x );
void U_Pass_u128_Test0( __uint128_t x );
void U_Pass_char8_Test0( char x );
void U_Pass_char8_Test1( char x );
void U_Pass_char8_Test2( char x );
void U_Pass_char8_Test3( char x );
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
void U_Pass_u128_x1_Test0( std::array<__uint128_t, 1> x );
void U_Pass_u128_x2_Test0( std::array<__uint128_t, 2> x );
void U_Pass_u128_x3_Test0( std::array<__uint128_t, 3> x );
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
void U_Pass_tup_i8_u128_Test0( Tuple2<int8_t, __uint128_t> x );
void U_Pass_tup_i8_u128_Test1( Tuple2<int8_t, __uint128_t> x );
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
void U_Pass_tup_u16_i128_Test0( Tuple2<uint16_t, __int128_t> x );
void U_Pass_tup_u16_i128_Test1( Tuple2<uint16_t, __int128_t> x );
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
void U_Pass_tup_i32_u128_Test0( Tuple2<int32_t, __uint128_t> x );
void U_Pass_tup_i32_u128_Test1( Tuple2<int32_t, __uint128_t> x );
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
void U_Pass_tup_u64_i128_Test0( Tuple2<uint64_t, __int128_t> x );
void U_Pass_tup_u64_i128_Test1( Tuple2<uint64_t, __int128_t> x );
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
void U_Pass_tup_f32_u128_Test0( Tuple2<float, __uint128_t> x );
void U_Pass_tup_f32_u128_Test1( Tuple2<float, __uint128_t> x );
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
void U_Pass_tup_f64_i128_Test0( Tuple2<double, __int128_t> x );
void U_Pass_tup_f64_i128_Test1( Tuple2<double, __int128_t> x );
void U_Pass_tup_f64_f32_Test0( Tuple2<double, float> x );
void U_Pass_tup_f64_f32_Test1( Tuple2<double, float> x );
void U_Pass_tup_f64_f64_Test0( Tuple2<double, double> x );
void U_Pass_tup_f64_f64_Test1( Tuple2<double, double> x );

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
	U_Pass_i32_Test4( 6531, -75247554, 456424, 8565523, 0, 0x7FFFFFFF, 54, int32_t(-0x80000000), 643, 7621375, 7567863, -24782, 786234786, 12308562, -8624557, 867245 );
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
	U_Pass_i128_Test0( ( __int128_t( 0x0123456789ABCDEFll) << 64u ) | 0xFEDCBA9876543210ll );
	U_Pass_u128_Test0( ( __uint128_t(0xFEDCBA9876543210ull) << 64u ) | 0x0123456789ABCDEFull );
	U_Pass_char8_Test0( 'Q' );
	U_Pass_char8_Test1( '!' );
	U_Pass_char8_Test2( ' ' );
	U_Pass_char8_Test3( char( 240 ) );
	U_Pass_char16_Test0( u'Ж' );
	U_Pass_char16_Test1( u'Ꙥ' );
	U_Pass_char32_Test0( U'😀' );
	U_Pass_f32_Test0( 0.0f );
	U_Pass_f32_Test1( 0.125f );
	U_Pass_f32_Test2( 6743.5f );
	U_Pass_f32_Test3( -7689543378437.0f );
	U_Pass_f32_Test4( 1.0f / 0.0f );
	U_Pass_f32_Test5( 0.0f / 0.0f );
	U_Pass_f32_Test6( 1786.5f, -643.4f, 754.0f, 353347.0f, 3000000.0f, -4454.25f, 0.0f, 66434.0f, 3643.3f, 367341.5f, 67436.125f, 378436.0f, 42.75f, -7542.2f, 6564.0f, 7854300000000.0f );
	U_Pass_f64_Test0( 0.0 );
	U_Pass_f64_Test1( 0.0625 );
	U_Pass_f64_Test2( 173.25 );
	U_Pass_f64_Test3( -569907695478437.0 );
	U_Pass_f64_Test4( 1.0 / 0.0 );
	U_Pass_f64_Test5( 0.0 / 0.0 );
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
			arg[i]= i * i * i * 337547ull + i * i * i * 563454548ull + 34565224787ull;
		U_Pass_u64_x11_Test0( arg );
	}
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
	U_Pass_tup_i8_u128_Test0( { 71, ( __uint128_t( 0x0123456789ABCDEFull ) << 64u ) | 0xFEDCBA9876543210ull } );
	U_Pass_tup_i8_u128_Test1( { -88, ( __uint128_t( 0xFEDCBA9876543210ull ) << 64u  ) | 0x0123456789ABCDEFull } );
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
	U_Pass_tup_u16_i128_Test0( { 712, ( __int128_t( 0x0123456789ABCDEFll ) << 64u ) | 0x7EDCBA9876543210ll } );
	U_Pass_tup_u16_i128_Test1( { 8812, ( __int128_t( 0x1EDCBA9876543210ll ) << 64u ) | 0x0123456789ABCDEFll } );
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
	U_Pass_tup_i32_u128_Test0( { -643647, ( __uint128_t(0x0123456789ABCDEFull) << 64u ) | 0xFEDCBA9876543210ull } );
	U_Pass_tup_i32_u128_Test1( { 856247, ( __uint128_t(0xFEDCBA9876543210ull) << 64u ) | 0x0123456789ABCDEFull } );
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
	U_Pass_tup_u64_i128_Test0( { 0xD1D16AEA7E54778Cull, ( __int128_t( 0x0123456789ABCDEFll ) << 64u ) | 0x7EDCBA9876543210ll } );
	U_Pass_tup_u64_i128_Test1( { 0x11D16AEA7E54278Cull, ( __int128_t( 0x1EDCBA9876543210ll ) << 64u ) | 0x0123456789ABCDEFll } );
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
	U_Pass_tup_f32_u128_Test0( { -3366.75f, ( __uint128_t( 0x0123456789ABCDEFull ) << 64u ) | 0xFEDCBA9876543210ull } );
	U_Pass_tup_f32_u128_Test1( { 0.125f, ( __uint128_t( 0xFEDCBA9876543210ull ) << 64u ) | 0x0123456789ABCDEFull } );
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
	U_Pass_tup_f64_i128_Test0( { -5362370.5, ( __int128_t( 0x0123456789ABCDEFll ) << 64u ) | 0x7EDCBA9876543210ll } );
	U_Pass_tup_f64_i128_Test1( { -37485856855542.0, ( __int128_t( 0x1EDCBA9876543210ll ) << 64u ) | 0x0123456789ABCDEFll } );
	U_Pass_tup_f64_f32_Test0( { 2632647.0, 6263.5f } );
	U_Pass_tup_f64_f32_Test1( { 11.125, -6356470.0f } );
	U_Pass_tup_f64_f64_Test0( { -1.75, -37163.125 } );
	U_Pass_tup_f64_f64_Test1( { 0.0, 253.0 } );
}

} // extern "C"
