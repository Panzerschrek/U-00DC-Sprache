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
			return byte8( "Q"c8 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('Q') )


def ByteTypesConstruction_Test4():
	c_program_text= """
		fn Foo()
		{
			// Can't construct from type of different size.
			var bytes16 b16( 0u8 );
			var bytes32 b32( 1i32 );
			var bytes64 b64( 0.7f );
			var bytes128 b128( "c"char8 );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 5 ) )
	assert( HaveError( errors_list, "TypesMismatch", 6 ) )
	assert( HaveError( errors_list, "TypesMismatch", 7 ) )
	assert( HaveError( errors_list, "TypesMismatch", 8 ) )


def ByteConversionPreservesValue_Test0():
	c_program_text= """
		static_assert( u8( byte8(7u8) ) == 7u8 );
		static_assert( i16( byte16(-25000i16) ) == -25000i16 );
		static_assert( u64( byte64(90000050001u64) ) == 90000050001u64 );
		static_assert( f32( byte32( 3.141592535f ) ) == 3.141592535f );
	"""
	tests_lib.build_program( c_program_text )
