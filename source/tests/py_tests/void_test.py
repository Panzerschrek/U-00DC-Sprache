from py_tests_common import *


def VoidTypeIsComplete_Test0():
	c_program_text= """
		struct S{ void v; }
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeIsComplete_Test1():
	c_program_text= """
		fn Foo( void v ){}
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeIsComplete_Test2():
	c_program_text= """
		fn Foo( void& v ){}
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeInitialization_Test0():
	c_program_text= """
		fn Foo()
		{
			var void v= zero_init;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeInitialization_Test1():
	c_program_text= """
		fn Foo()
		{
			var void v0= zero_init;
			var void v1= v0;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeInitialization_Test2():
	c_program_text= """
		fn Foo()
		{
			var void v0= zero_init;
			var void v1(v0);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeInitialization_Test3():
	c_program_text= """
		fn Foo()
		{
			var void v0= zero_init;
			auto v1= v0;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeInitialization_Test4():
	c_program_text= """
		fn Foo()
		{
			var void v0;
			static_assert( typeinfo</void/>.is_default_constructible);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeInitialization_Test5():
	c_program_text= """
		struct S{ void f; [ void, 4 ] a; }
		fn Foo()
		{
			var S s;
			static_assert( typeinfo</S/>.is_default_constructible);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeAssignment_Test0():
	c_program_text= """
		fn Foo()
		{
			var void mut v0= zero_init, v1;
			v0= v1;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def UseVoidInFunction_Test0():
	c_program_text= """
		fn Bar(){}
		fn Foo()
		{
			var void v= Bar();
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def UseVoidInFunction_Test1():
	c_program_text= """
		fn Bar(){}
		fn Baz(void v){}
		fn Foo()
		{
			Baz(Bar());
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def UseVoidInFunction_Test2():
	c_program_text= """
		fn Bar(void v){}
		fn Foo()
		{
			var void mut v= zero_init;
			Bar(v);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeIsConstexpr_Test0():
	c_program_text= """
		fn Foo()
		{
			var void constexpr v= zero_init;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeIsConstexpr_Test1():
	c_program_text= """
		fn Foo()
		{
			var void constexpr v0= zero_init;
			var void constexpr v1= v0;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeIsConstexpr_Test2():
	c_program_text= """
		var void constexpr v0= zero_init;
		auto constexpr v1= v0;
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeIsConstexpr_Test3():
	c_program_text= """
		fn constexpr Foo(){}
		auto constexpr v= Foo();
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeIsConstexpr_Test4():
	c_program_text= """
		fn constexpr Bar(){}
		fn constexpr Baz(void v){}
		fn constexpr Foo(){ Baz(Bar()); }
		auto constexpr v= Foo();
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeTypeinfo_Test0():
	c_program_text= """
		auto& ti= typeinfo</void/>;
		static_assert( ti.is_fundamental );
		static_assert( ti.is_void );
		static_assert( ti.is_default_constructible );
		static_assert( ti.is_copy_constructible );
		static_assert( ti.is_copy_assignable );
		static_assert( ti.size_of == 0s );
		static_assert( ti.size_of <= 1s );
	"""
	tests_lib.build_program( c_program_text )


def VoidTypeTypeinfo_Test1():
	c_program_text= """
		struct S{ f32 x; void v; }
		static_assert( typeinfo</S/>.size_of == typeinfo</f32/>.size_of );

		struct T{ void v; i64 y; }
		static_assert( typeinfo</T/>.size_of == typeinfo</i64/>.size_of );

		static_assert( typeinfo</ [ void, 64 ] />.size_of == 0s );

		static_assert( typeinfo</ tup[ void, f32, void, u32, void ] />.size_of == 8s );
	"""
	tests_lib.build_program( c_program_text )
