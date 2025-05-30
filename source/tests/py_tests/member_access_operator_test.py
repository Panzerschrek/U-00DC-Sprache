from py_tests_common import *


def MemberAccesOperator_AccessType_Test0():
	c_program_text= """
		struct S
		{
			type T= f64;
		}
		fn Foo()
		{
			var S s;
			auto x= s.T( 13.7 ); // Access a type alias via ".".
			static_assert( same_type</ typeof(x), f64 /> );
			static_assert( x == 13.7 );
		}
	"""
	tests_lib.build_program( c_program_text )


def MemberAccesOperator_AccessType_Test1():
	c_program_text= """
		struct S
		{
			struct Inner
			{
				i32 x; f32 y;
			}
		}
		fn Foo()
		{
			var S s;
			auto inner= s.Inner{ .x= 77, .y= -0.25f }; // Access a nested struct type via ".".
			static_assert( same_type</ typeof(inner), S::Inner /> );
			static_assert( inner.x == 77 );
			static_assert( inner.y == -0.25f );
		}
	"""
	tests_lib.build_program( c_program_text )


def MemberAccesOperator_AccessType_Test2():
	c_program_text= """
		class C
		{
		private:
			type T= f64;
		}
		fn Foo()
		{
			var C c;
			auto x= c.T( 13.7 ); // Access a type alias via ".". It's an error, since it's private.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingNonpublicClassMember", 10 ) )


def MemberAccesOperator_AccessType_Test3():
	c_program_text= """
		struct S
		{
			enum E { A, B, C }
		}
		fn Foo()
		{
			var S s;
			auto x= s.E( S::E::B ); // Access an enum via ".".
			static_assert( same_type</ typeof(x), S::E /> );
			static_assert( x == S::E::B );
		}
	"""
	tests_lib.build_program( c_program_text )


def MemberAccesOperator_AccessType_Test4():
	c_program_text= """
		template</type T/> struct Box{ T t; }
		struct S
		{
			type Some= char16;
		}
		fn Foo()
		{
			var S s;
			var Box</ s.Some /* Access a type alias via "." and use it for template type argument. */ /> box{ .t= 'я'c16 };
			static_assert( box.t == 'я'c16 );
		}
	"""
	tests_lib.build_program( c_program_text )


def MemberAccesOperator_AccessType_Test5():
	c_program_text= """
		struct S
		{
			type Some= i32;
		}
		fn Foo()
		{
			var S s;
			auto x= s.Some</ f32 />( 17 ); // Access type alias via ".". It's error, since template arguments list is provided and it's not template.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ValueIsNotTemplate", 9 ) )


def MemberAccesOperator_AccessType_Test6():
	c_program_text= """
		enum E{ A, B, C }
		fn Foo()
		{
			// Access inner type of a typeinfo class via ".".
			static_assert( typeinfo</E/>.underlying_type.src_type( E::C ) == 2u8 );
		}
	"""
	tests_lib.build_program( c_program_text )


def MemberAccesOperator_AccessGlobalVariable_Test0():
	c_program_text= """
		struct S
		{
			auto some_val= 123;
		}
		fn Foo()
		{
			var S s;
			static_assert( s.some_val == 123 ); // Access a global auto variable via "." operator.
		}
	"""
	tests_lib.build_program( c_program_text )


def MemberAccesOperator_AccessGlobalVariable_Test1():
	c_program_text= """
		struct S
		{
			var tup[ f64, bool, char8 ] some_val[ 7.8, false, 'H' ];
		}
		fn Foo()
		{
			var S s;
			static_assert( s.some_val[0] == 7.8 ); // Access a global variable via "." operator.
			static_assert( s.some_val[1] == false ); // Access a global variable via "." operator.
			static_assert( s.some_val[2] == 'H' ); // Access a global variable via "." operator.
		}
	"""
	tests_lib.build_program( c_program_text )


def MemberAccesOperator_AccessGlobalVariable_Test2():
	c_program_text= """
		struct S
		{
			var i32 mut x= 0;
		}
		fn Foo()
		{
			var S s;
			auto x= s.x; // Access a global mutable variable via "." operator. It's an error, because it happens outside unsafe block.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalMutableVariableAccessOutsideUnsafeBlock", 9 ) )


def MemberAccesOperator_AccessGlobalVariable_Test3():
	c_program_text= """
		struct S
		{
			thread_local i32 x= 13;
		}
		fn Foo()
		{
			var S s;
			unsafe
			{
				s.x+= 25; // Access a global mutable thread-local variable via "." operator.
			}
			halt if( unsafe( S::x ) != 13 + 25 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def MemberAccesOperator_AccessGlobalVariable_Test4():
	c_program_text= """
		struct S
		{
			thread_local i32 x= 0;
		}
		fn Foo()
		{
			var S s;
			auto x= s.x; // Access a global mutable thread-local variable via "." operator. It's an error, because it happens outside unsafe block.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalMutableVariableAccessOutsideUnsafeBlock", 9 ) )


def MemberAccesOperator_AccessGlobalVariable_Test5():
	c_program_text= """
		class C
		{
		private:
			auto some_val= 123;
		}
		fn Foo()
		{
			var C c;
			auto x= c.some_val == 123; // Access a global auto variable via "." operator. It's an error, since it's private.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingNonpublicClassMember", 10 ) )


def MemberAccesOperator_AccessGlobalVariable_Test6():
	c_program_text= """
		struct S
		{
			var i32 x= 0;
		}
		fn Foo()
		{
			var S s;
			auto x= s.x</ f32 />( 17 ); // Access global variable via ".". It's error, since template arguments list is provided and it's not template.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ValueIsNotTemplate", 9 ) )
