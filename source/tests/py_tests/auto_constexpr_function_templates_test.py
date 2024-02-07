from py_tests_common import *


def AutoConstexprFunctionTemplate_Test0():
	c_program_text= """
		template</ type T /> fn GetZero() : T { return T(0); }

		static_assert( GetZero</i32/>() == 0 );
		static_assert( GetZero</u64/>() == 0u64 );
		static_assert( GetZero</f32/>() == 0.0f );
	"""
	tests_lib.build_program( c_program_text )


def AutoConstexprFunctionTemplate_Test1():
	c_program_text= """
		template</ type T /> fn DoubleIt( T x ) : T { return x * T(2); }

		static_assert( DoubleIt( 0.4f ) == 0.8f );
		static_assert( DoubleIt( 85.4 ) == 170.8 );
		static_assert( DoubleIt( 651 ) == 1302 );
	"""
	tests_lib.build_program( c_program_text )


def AutoConstexprFunctionTemplate_Test2():
	c_program_text= """
		template</ type T /> fn CallBar( i32 mul ) : i32 { var T t; return t.Bar( mul ); }

		struct S
		{
			fn constexpr Bar( i32 mul ) : i32 { return 525852 * mul; }
		}

		static_assert( CallBar</S/>( 21 ) == 21 * 525852 );
	"""
	tests_lib.build_program( c_program_text )


def AutoConstexprFunctionTemplate_Test3():
	c_program_text= """
		template</ type T /> fn CallBar() : i32 { var T t; return t.Bar(); }

		struct S
		{
			fn Bar() : i32 { return 525852; }
		}

		static_assert( CallBar</S/>() == 525852 ); // Error, template instantiation is not constexpr, because S::Bar is not constexpr.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "StaticAssertExpressionIsNotConstant" )
	assert( errors_list[0].src_loc.line == 9 )


def AutoConstexprFunctionTemplate_Test4():
	# Recursive template function can't be auto-constexpr, since self-call inside its body considered to be non-constexpr.
	c_program_text= """
		template</type T/>
		fn Factorial( T t ) : T
		{
			if( t <= T(1) )
			{
				return 1;
			}

			return t * Factorial(t - T(1));
		}

		static_assert( Factorial(5) == 120 );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StaticAssertExpressionIsNotConstant", 13 ) )
