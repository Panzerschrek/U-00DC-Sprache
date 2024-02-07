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


def VoidTypeInitialization_Test6():
	c_program_text= """
		fn Foo()
		{
			var void v(); // constructor initializer with zero args.
			static_assert( v == void() ); // constructor initializer with zero args produces constant value.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeInitialization_Test7():
	c_program_text= """
		fn Foo()
		{
			var void v0;
			var void v1(v0); // constructor initializer with one arg.
			static_assert( v1 == void() ); // constructor initializer with one arg produces constant value.
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


def VoidTypeIsConstexpr_Test5():
	c_program_text= """
		fn Bar(){}
		fn Foo()
		{
			// Result of non-constexpr "void" call is not constexpr.
			var void constexpr v= Bar();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 6 ) )


def VoidTypeIsConstexpr_Test56():
	c_program_text= """
		fn constexpr Foo( void& r ) {}  // Function with void reference param can be constexpr.
		var void v0= zero_init;
		var void v1= Foo(v0);
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
		static_assert( ti.align_of <= 1s );
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


def VoidEqualityCompare_Test0():
	c_program_text= """
		fn Bar(){}
		fn Foo()
		{
			// All "void" values are same.
			var void v0= zero_init;
			var void v1= v0;
			var void mut v2= Bar();
			halt if( v0 != v1 );
			halt if( v1 != v0 );
			halt if( v2 != v0 );
			halt if( v2 != v1 );
			halt if( v0 != v2 );
			halt if( v1 != v2 );
			halt if( v0 != v0 );
			halt if( v2 != v2 );

			halt if( !( v0 == v1 ) );
			halt if( !( v1 == v0 ) );
			halt if( !( v2 == v0 ) );
			halt if( !( v2 == v1 ) );
			halt if( !( v0 == v2 ) );
			halt if( !( v1 == v2 ) );
			halt if( !( v0 == v0 ) );
			halt if( !( v2 == v2 ) );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidEqualityCompare_Test1():
	c_program_text= """
		fn Foo()
		{
			// Compare result for "void" constexpr arguments is constexpr.
			var void constexpr v0= zero_init;
			var void constexpr v1= zero_init;
			static_assert( v0 == v1 );
			static_assert( !( v0 != v1 ) );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidEqualityCompare_Test2():
	c_program_text= """
		fn Bar(){}
		fn Foo()
		{
			// Compare result for "void" non-constexpr arguments is not constexpr.
			var void constexpr v0= zero_init;
			var void mut v1= Bar();
			static_assert( v0 == v1 );
			static_assert( v1 != v1 );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "StaticAssertExpressionIsNotConstant", 8 ) )
	assert( HasError( errors_list, "StaticAssertExpressionIsNotConstant", 9 ) )


def VoidOrderCompare_Test0():
	c_program_text= """
		fn Foo()
		{
			var void v0= zero_init, v1= zero_init;
			v0 > v1;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def VoidOrderCompare_Test1():
	c_program_text= """
		fn Foo()
		{
			var void v0= zero_init, v1= zero_init;
			v0 <= v1;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def ArithmeticOperatorForVoid_Test0():
	c_program_text= """
		fn Foo()
		{
			var void v0= zero_init, v1= zero_init;
			v0 + v1;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def ArithmeticOperatorForVoid_Test1():
	c_program_text= """
		fn Foo()
		{
			var void v0= zero_init, v1= zero_init;
			v0 / v1;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def ArithmeticOperatorForVoid_Test2():
	c_program_text= """
		fn Foo()
		{
			var void v;
			-v;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def BitwiseOperatorForVoid_Test0():
	c_program_text= """
		fn Foo()
		{
			var void v0, v1;
			v0 ^ v1;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def BitwiseOperatorForVoid_Test1():
	c_program_text= """
		fn Foo()
		{
			var void v;
			~v;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )
