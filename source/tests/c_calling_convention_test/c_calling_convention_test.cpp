#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

#define TEST_ASSERT(x) { if( !(x) ) { std::cerr << "Line " << __LINE__ << " assertion failed: " << #x << std::endl; std::abort(); } }

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

} // extern "C"
