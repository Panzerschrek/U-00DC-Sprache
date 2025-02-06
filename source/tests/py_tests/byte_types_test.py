from py_tests_common import *

def ByteTypesUsage_Test0():
	c_program_text= """
		type A = byte8;
		type B = byte16;
		type C = byte32;
		type D = byte64;
		type E = byte128;
	"""
	tests_lib.build_program( c_program_text )


def ByteTypesConstruction_Test0():
	c_program_text= """
		fn Foo() : byte16
		{
			// Construct bytes from unsigned integer.
			return byte16( 37584u16 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 37584 )


def ByteTypesConstruction_Test1():
	c_program_text= """
		fn Foo() : byte8
		{
			// Construct bytes from signed integer.
			return byte8( -1i8 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 255 )


def ByteTypesConstruction_Test2():
	c_program_text= """
		fn Foo() : f32
		{
			// Construct bytes from float.
			var byte32 b( 3.5f );
			return f32(b);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3.5 )


def ByteTypesConstruction_Test3():
	c_program_text= """
		fn Foo() : byte8
		{
			// Construct bytes from char.
			return byte8( 'Q' );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('Q') )


def ByteTypesConstruction_Test4():
	c_program_text= """
		// Can use byte type in constexpr context and have global variales of this type.
		var byte8 a( 7u8 );
		var [byte32, 3] arr[ (17), (8), (25566) ];
	"""
	tests_lib.build_program( c_program_text )


def ByteTypesConstexprFunctionCast_Test0():
	c_program_text= """
		auto a= cast(7.125f);
		static_assert(a == byte32(0x40E40000u32));

		auto b= cast(byte64(0x4057D33333333333u64));
		static_assert(b == 95.3);

		fn constexpr cast(f32 x) : byte32
		{
			return byte32(x);
		}

		fn constexpr cast(byte64 x) : f64
		{
			return f64(x);
		}
	"""
	tests_lib.build_program( c_program_text )


def ByteTypesConstruction_Test5():
	c_program_text= """
		fn Foo()
		{
			// Can't construct from type of different size.
			var bytes16 b16( 0u8 );
			var bytes32 b32( 1i32 );
			var bytes64 b64( 0.7f );
			var bytes128 b128( 'c'char8 );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 5 ) )
	assert( HasError( errors_list, "TypesMismatch", 6 ) )
	assert( HasError( errors_list, "TypesMismatch", 7 ) )
	assert( HasError( errors_list, "TypesMismatch", 8 ) )


def ByteTypesBitCast_Test0():
	c_program_text= """
		static_assert( u8(byte8(-17i8)) == 239u8 );
		static_assert( u16(byte16(-30567i16)) == 34969u16 );
		static_assert( u32( byte32( 11.5f ) ) == 0x41380000u );
		static_assert( byte32( 0xC0490FDB ) == byte32( -3.1415926535f ) );
		static_assert( byte64( 2.718281828 ) == byte64( 0x4005BF0A8B04919Bu64 ) );
	"""
	tests_lib.build_program( c_program_text )


def ByteConversionPreservesValue_Test0():
	c_program_text= """
		static_assert( u8( byte8(7u8) ) == 7u8 );
		static_assert( i16( byte16(-25000i16) ) == -25000i16 );
		static_assert( u64( byte64(90000050001u64) ) == 90000050001u64 );
		static_assert( f32( byte32( 3.141592535f ) ) == 3.141592535f );
		static_assert( char16( byte16('Ж'c16) ) == 'Ж'c16 );
	"""
	tests_lib.build_program( c_program_text )


def ByteTypesNonexistentOperations_Test0():
	c_program_text= """
		fn Foo()
		{
			var byte32 a= zero_init, b= zero_init;
			var byte8 c= zero_init, d= zero_init;
			// There are no arithmetic operations for byte types.
			a + b;
			b * a;
			a % b;
			c / d;
			c - d;
			-b;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 7 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 8 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 9 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 10 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 11 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 12 ) )


def ByteTypesNonexistentOperations_Test1():
	c_program_text= """
		fn Foo()
		{
			var byte64 a= zero_init, b= zero_init;
			var byte128 c= zero_init, d= zero_init;
			// There are no bit operations for byte types.
			a & b;
			b | a;
			a ^ b;
			c >> 1;
			d << 2;
			~c;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 7 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 8 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 9 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 10 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 11 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 12 ) )


def ByteTypesNonexistentOperations_Test2():
	c_program_text= """
		fn Foo()
		{
			var byte32 mut a= zero_init, mut b= zero_init;
			var byte64 mut c= zero_init, mut d= zero_init;
			// There are no compound assgnment operations for byte types.
			a += b;
			b *= a;
			a %= b;
			c |= d;
			d ^= c;
			c <<= d;
			++a;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 7 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 8 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 9 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 10 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 11 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 12 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 13 ) )


def ByteTypesNonexistentOperations_Test3():
	c_program_text= """
		var byte8 b(66u8);
		type Arr= [ f32, b ]; // Can't use byte types as array size.
		fn Foo()
		{
			var [i32, 128] arr= zero_init;
			arr[b]; // Can't use byte types as array indices.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ArraySizeIsNotInteger", 2 ) )
	assert( HasError( errors_list, "TypesMismatch", 7 ) )


def ByteTypesNonexistentOperations_Test3():
	c_program_text= """
		fn Foo()
		{
			var byte8 a= zero_init, b= zero_init;
			var byte32 c= zero_init, d= zero_init;
			// There are no order compare operations for byte types.
			a < b;
			b <= a;
			a >= b;
			c >= d;
			d <=> c;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 7 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 8 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 9 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 10 ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 11 ) )


def ByteTypesNonexistentOperations_Test4():
	c_program_text= """
		fn Foo()
		{
			// "bool" can't be converted ito "byte8"
			var byte8 byte_value(false);
			var bool some_bool= true;
			var byte8 other_byte_value= some_bool;
			// byte8 can't be converted into bool.
			var byte8 zero_byte= zero_init;
			var bool bool_value_0= zero_byte;
			var bool bool_value_1(zero_byte);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 5 ) )
	assert( HasError( errors_list, "TypesMismatch", 7 ) )
	assert( HasError( errors_list, "TypesMismatch", 10 ) )
	assert( HasError( errors_list, "TypesMismatch", 11 ) )


def ByteTypesEqualityCompare_Test0():
	c_program_text= """
		fn Foo()
		{
			var byte8 a(1u8), b(2u8), c(3u8), d(2u8), e(1u8);
			halt if( a != a );
			halt if( a == b );
			halt if( b != b );
			halt if( b != d );
			halt if( d != b );
			halt if( a != e );
			halt if( !( a == e ) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def ByteTypesAreAssignable_Test0():
	c_program_text= """
		fn Foo()
		{
			var byte32 mut a(17);
			halt if( i32(a) != 17 );
			a= byte32( -765 );
			halt if( i32(a) != -765 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def ByteTypesAreCopyConstructible_Test0():
	c_program_text= """
		fn Foo()
		{
			var byte16 mut a(888u16);
			var byte16 mut b(a);
			halt if( b != a );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def ByteTypesAreCopyConstructible_Test1():
	c_program_text= """
		struct S{ byte32 b; }
		fn Foo()
		{
			var S s{ .b(678) };
			var S s_copy(s);
			halt if(s != s_copy);
			halt if(s.b != s_copy.b);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def ByteTypesAreExpressionConstructible_Test0():
	c_program_text= """
		fn Foo()
		{
			var byte64 mut a(123456789u64);
			var byte64 mut b = a;
			halt if( b != a );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def ByteTypesParameters_Test0():
	c_program_text= """
		fn ByteDiv( byte8 a, byte8 b ) : byte8 { return byte8( u8( u32(u8(a)) / u32(u8(b)) ) ); }
		fn Foo()
		{
			var byte8 mut a(37u8), mut b(3u8);
			halt if( u8(ByteDiv( a, b )) != u8(37 / 3) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def ByteTypesParameters_Test1():
	c_program_text= """
		fn Byte32Inverse( byte32& mut b )
		{
			b= byte32( ~u32(b) );
		}
		fn Foo()
		{
			var byte32 mut b(6425u32);
			Byte32Inverse(b);
			halt if( u32(b) != ~6425u32 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def ByteTypesFields_Test0():
	c_program_text= """
		struct S
		{
			byte32 b32;
			byte16 b16;
			byte8 b8;

			fn constructor()
			( b32= zero_init, b16= zero_init, b8( 13u8 ) )
			{}
		}
		static_assert( typeinfo</S/>.size_of == 8s );

		fn Foo()
		{
			var S s;
			halt if( s.b32 != byte32(0) );
			halt if( s.b16 != byte16(0u16) );
			halt if( u8(s.b8) != 13u8 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def ByteTypesFields_Test1():
	c_program_text= """
		struct S
		{
			byte32 b32;
		}

		fn Foo()
		{
			// Field of type "byte32" is not default-constructible.
			var S s;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedInitializer", 10 ) )


def ByteTypesTypeinfo_Test0():
	c_program_text= """
		static_assert( typeinfo</byte8/>.is_fundamental );
		static_assert( !typeinfo</byte8/>.is_default_constructible );
		static_assert( typeinfo</byte8/>.is_copy_constructible );
		static_assert( typeinfo</byte8/>.is_copy_assignable );
		static_assert( typeinfo</byte8/>.is_equality_comparable );

		static_assert( !typeinfo</byte16/>.is_enum );
		static_assert( !typeinfo</byte16/>.is_array );
		static_assert( !typeinfo</byte16/>.is_tuple );
		static_assert( !typeinfo</byte16/>.is_class );
		static_assert( !typeinfo</byte16/>.is_raw_pointer );
		static_assert( !typeinfo</byte16/>.is_function_pointer );
		static_assert( !typeinfo</byte16/>.is_numeric );
		static_assert( !typeinfo</byte16/>.is_integer );
		static_assert( !typeinfo</byte16/>.is_signed_integer );
		static_assert( !typeinfo</byte16/>.is_unsigned_integer );
		static_assert( !typeinfo</byte16/>.is_float );
		static_assert( !typeinfo</byte16/>.is_char );
		static_assert( !typeinfo</byte16/>.is_bool );
		static_assert( !typeinfo</byte16/>.is_void );

		static_assert( typeinfo</byte8/>.is_byte );
		static_assert( typeinfo</byte8/>.size_of == 1s );
		static_assert( typeinfo</byte8/>.align_of == typeinfo</u8/>.align_of );

		static_assert( typeinfo</byte16/>.is_byte );
		static_assert( typeinfo</byte16/>.size_of == 2s );
		static_assert( typeinfo</byte16/>.align_of == typeinfo</u16/>.align_of );

		static_assert( typeinfo</byte32/>.is_byte );
		static_assert( typeinfo</byte32/>.size_of == 4s );
		static_assert( typeinfo</byte32/>.align_of == typeinfo</u32/>.align_of );

		static_assert( typeinfo</byte64/>.is_byte );
		static_assert( typeinfo</byte64/>.size_of == 8s );
		static_assert( typeinfo</byte64/>.align_of == typeinfo</u64/>.align_of );

		static_assert( typeinfo</byte128/>.is_byte );
		static_assert( typeinfo</byte128/>.size_of == 16s );
		static_assert( typeinfo</byte128/>.align_of == typeinfo</u128/>.align_of );
	"""
	tests_lib.build_program( c_program_text )


def ByteTypesAreDistinctFromOtherFundamentalTypes_Test0():
	c_program_text= """
	static_assert( is_same_type</byte8, byte8/>() );
	static_assert( is_same_type</byte16, byte16/>() );
	static_assert( is_same_type</byte32, byte32/>() );
	static_assert( is_same_type</byte64, byte64/>() );
	static_assert( is_same_type</byte128, byte128/>() );

	static_assert( !is_same_type</byte8, u8/>() );
	static_assert( !is_same_type</byte8, i8/>() );
	static_assert( !is_same_type</u16, byte16/>() );
	static_assert( !is_same_type</i16, byte16/>() );
	static_assert( !is_same_type</byte32, u32/>() );
	static_assert( !is_same_type</byte32, i32/>() );
	static_assert( !is_same_type</u64, byte64/>() );
	static_assert( !is_same_type</i64, byte64/>() );
	static_assert( !is_same_type</byte128, u128/>() );
	static_assert( !is_same_type</byte128, i128/>() );

	static_assert( !is_same_type</byte8, char8/>() );
	static_assert( !is_same_type</byte16, char16/>() );
	static_assert( !is_same_type</byte32, char32/>() );

	static_assert( !is_same_type</byte32, f32/>() );
	static_assert( !is_same_type</byte64, f64/>() );

	static_assert( !is_same_type</byte16, tup[u16]/>() );
	static_assert( !is_same_type</byte64, tup[i64]/>() );

	template</byte8 b/> struct S{}

	type A17= S</ byte8(17u8) />;
	type B17= S</ byte8(17u8) />;
	type A18 =  S</ byte8(18u8) />;

	static_assert( is_same_type</A17, B17/>() );
	static_assert( is_same_type</B17, A17/>() );
	static_assert( !is_same_type</A17, A18/>() );
	static_assert( !is_same_type</A18, B17/>() );

	template</ type A, type B />
	fn constexpr is_same_type() : bool
	{
		return is_same_type_impl</A/>::same</B/>::value;
	}

	template</ type A />
	struct is_same_type_impl
	{
		// Use type templates overloading for type equality comparison.
		template</ type B /> struct same</ B />
		{
			auto constexpr value= false;
		}

		template</ /> struct same</ A />
		{
			auto constexpr value= true;
		}
	}
	"""
	tests_lib.build_program( c_program_text )


def ByteTypeCanNotBeEnumUnderlyingType_Test0():
	c_program_text= """
		enum E : byte32 { A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 2 ) )
