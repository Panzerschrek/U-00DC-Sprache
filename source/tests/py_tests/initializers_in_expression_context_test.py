from py_tests_common import *

def StructInitializationExpression_Test0():
	c_program_text= """
		struct S{ i32 x; }
		fn Foo()
		{
			// Initialize single field.
			auto s= S{ .x= 56 };
			halt if( s.x != 56 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructInitializationExpression_Test1():
	c_program_text= """
		struct S{ f32 y; i32 x; }
		fn Foo()
		{
			// Initialize two fields.
			auto s= S{ .x= 765, .y= 43.2f };
			halt if( s.x != 765 );
			halt if( s.y != 43.2f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructInitializationExpression_Test2():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			auto s= S{}; // Initialize no fields.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructInitializationExpression_Test3():
	c_program_text= """
		struct S{ u32 x; f32 y= 5.6f; bool z; }
		fn Foo()
		{
			// Use default initializer for "y".
			auto s= S{ .z= true, .x= 111u };
			halt if( s.x != 111u );
			halt if( s.y != 5.6f );
			halt if( s.z != true );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructInitializationExpression_Test4():
	c_program_text= """
		struct S
		{
			i32 x;
			op-(S a, S b ) : S
			{
				// Use struct initializator in return expression.
				return S{ .x= a.x - b.x };
			}
		}
		fn Foo()
		{
			// Apply overloaded binary operator to expressions with "{}" inside.
			auto diff= S{ .x= 76 } - S{ .x= 33 };
			halt if( diff.x != 76 - 33 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructInitializationExpression_Test5():
	c_program_text= """
		struct S
		{
			i32 x;
			op[](this, i32 y) : i32
			{
				return x * y;
			}
		}
		fn Foo()
		{
			// Apply postfix [] after struct initializer.
			halt if( S{ .x= 12 }[5] != 60 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructInitializationExpression_Test6():
	c_program_text= """
		struct S
		{
			i32 x;
			op~(this) : S
			{
				return S{ .x = ~x };
			}
		}
		fn Foo()
		{
			// Apply prefix ~ before struct initializer.
			halt if( (~S{ .x= 786 }).x != ~786 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructInitializationExpression_Test7():
	c_program_text= """
		struct S { f32 x; }
		fn ExtractX( S& s ) : f32 { return s.x; }
		fn Foo()
		{
			// Pass a variable, created with {} initializer into a function.
			var f32 x= ExtractX( S{ .x= 3.141592535f } );
			halt if( x != 3.141592535f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructInitializationExpression_Test8():
	c_program_text= """
		struct S{ i32 x; }
		fn Foo()
		{
			// Use member access operator after struct initialization.
			halt if( S{ .x= 65 }.x != 65 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructInitializationExpression_Test9():
	c_program_text= """
		struct S{ i32 x; u64 y(66); f32 z; }
		// {} initializer in expression context may produce constexpr values.
		auto s= S{ .x= 55, .z= -1.0f };
		static_assert( s.x == 55 );
		static_assert( s.y == u64(66) );
		static_assert( s.z == -1.0f );
	"""
	tests_lib.build_program( c_program_text )


def StructInitializationExpression_Test10():
	c_program_text= """
		struct S{ i32 x; }
		// {} initializer in expression context may produce constexpr values.
		static_assert( S{ .x= 1266 }.x == 1266 );
	"""
	tests_lib.build_program( c_program_text )


def StructInitializationExpression_Test11():
	c_program_text= """
		struct S{ i32 &mut r; }
		fn Foo()
		{
			var i32 mut x= 643;
			++ S{ .r= x }.r; // Initialize reference field in {}, than access it and modify referenced value.
			halt if( x != 644 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructInitializationExpression_Test12():
	c_program_text= """
		struct S{ i32& r; }
		fn Foo()
		{
			var i32 mut x= 0;
			// Initialize reference field in "s".
			// This causes an error, because a mutable and an immutable references are passed into a function in the same time.
			Bar( S{ .r= x }, x );
		}
		fn Bar(S s, i32 &mut x);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )
