from py_tests_common import *


def ConstexprHalt_Test0():
	c_program_text= """
		fn constexpr Foo( i32 x ) : i32
		{
			halt if( (x&1) == 0 );
			return x | 0xFF;
		}

		fn Baz(){  Foo( 84 );  }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionEvaluationError" )
	assert( errors_list[0].src_loc.line == 8 )


def ConstexprFunctionEvaluationError_Test3():
	c_program_text= """
		fn constexpr Count( u32 x ) : u32
		{
			if( x == 0u ) { return 0u; }
			return 1u + Count( x - 1u );
		}
		fn Foo()
		{
			Count(16u * 65536u);  // Recursive call depth here is too big.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionEvaluationError" )
	assert( errors_list[0].src_loc.line == 9 )
	assert( errors_list[0].text.find( "Max call stack depth" ) != -1 )


def ConstexprFunctionEvaluationError_Test4():
	c_program_text= """
		fn constexpr Bar( u32 x ) : u32
		{
			var [ u8, 1024u * 1024u * 80u ] imut bytes= zero_init;   // Allocating too big chunk of memory on stack.
			return x;
		}
		fn Foo(){  Bar(0u);  }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionEvaluationError" )
	assert( errors_list[0].src_loc.line == 7 )


def ConstexprFunctionsMustHaveBody_Test0():
	c_program_text= """
		fn constexpr Foo();
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionsMustHaveBody" )
	assert( errors_list[0].src_loc.line == 2 )


def ConstexprFunctionsMustHaveBody_Test1():
	c_program_text= """
		struct S
		{
			fn constexpr Foo();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionsMustHaveBody" )
	assert( errors_list[0].src_loc.line == 4 )


def ConstexprFunctionCanNotBeVirtual_Test0():
	c_program_text= """
		class S polymorph
		{
			fn virtual constexpr Foo(){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ConstexprFunctionCanNotBeVirtual", 4 ) )


def ConstexprFunctionCanNotBeVirtual_Test1():
	c_program_text= """
		class S polymorph
		{
			op virtual constexpr []( this, u32 x ){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ConstexprFunctionCanNotBeVirtual", 4 ) )


def InvalidTypeForConstexprFunction_Test1():
	c_program_text= """
		struct S{ i32& mut r; }  // Struct is not constexpr.
		// Argument type is not constexpr.
		fn constexpr Foo( S& x ){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstexprFunction" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidTypeForConstexprFunction_Test2():
	c_program_text= """
		// Unsafe function can not be constexpr.
		fn constexpr Foo() unsafe {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstexprFunction" )
	assert( errors_list[0].src_loc.line == 3 )


def InvalidTypeForConstexprFunction_Test3():
	c_program_text= """
		// Function with function pointer in signature can not be constexpr.
		type fn_ptr= (fn());
		fn constexpr Foo( fn_ptr ptr ) {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstexprFunction" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidTypeForConstexprFunction_Test5():
	c_program_text= """
		struct S
		{
			fn destructor(){}
		} // Struct is not constexpr, because it have explicit destructor.
		// Function, returnig non-constexpr result, can not be constexpr.
		fn constexpr Foo() : S { return S(); }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstexprFunction" )
	assert( errors_list[0].src_loc.line == 7 )


def ConstexprFunctionContainsUnallowedOperations_Test0():
	c_program_text= """
		// Unsafe blocks not allowed in constexpr functions.
		fn constexpr Foo() { unsafe{} }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 3 )


def ConstexprFunctionContainsUnallowedOperations_Test1():
	c_program_text= """
		fn NonConstexprFunction(){}
		// Calling non-constexpr functions in constexpr function not allowed.
		fn constexpr Foo() { NonConstexprFunction(); }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 4 )


def ConstexprFunctionContainsUnallowedOperations_Test2():
	c_program_text= """
		fn constexpr Foo()
		{
			var (fn()) ptr= Foo;
			ptr(); // Calling function pointers not allowed in constexpr functions.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 2 )


def ConstexprFunctionContainsUnallowedOperations_Test3():
	c_program_text= """
		struct S{ fn destructor(){} }
		fn constexpr Foo()
		{
			var S s{};  // Decalring variable with non-constexpr type not allowed on constexpr functions.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 3 )


def ConstexprFunctionContainsUnallowedOperations_Test4():
	c_program_text= """
		struct S{ fn destructor(){} }
		fn constexpr Foo()
		{
			auto s= S();  // Decalring auto variable with non-constexpr type not allowed on constexpr functions.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 3 )


def ConstexprFunctionContainsUnallowedOperations_Test5():
	c_program_text= """
		struct S
		{
			op++( mut this ){}
		}
		fn constexpr Foo()
		{
			var S mut s{};
			++s;   // Calling non-constexpr overloaded operator in constexpr functions not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 6 )


def ConstexprFunctionContainsUnallowedOperations_Test6():
	c_program_text= """
		struct S
		{
			op+=( mut this, S& other ){}
		}
		fn constexpr Foo()
		{
			var S mut a{}, mut b{};
			a+= b;   // Calling non-constexpr overloaded operator in constexpr functions not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 6 )


def ConstexprFunctionContainsUnallowedOperations_Test7():
	c_program_text= """
		struct S
		{
			op()( this ){}
		}
		fn constexpr Foo()
		{
			var S s{};
			s();   // Calling non-constexpr overloaded postfix operator in constexpr functions not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 6 )


def ConstexprFunctionContainsUnallowedOperations_Test8():
	c_program_text= """
		struct S
		{
			op~( S a ) : S { return S(); }
		}
		fn constexpr Foo()
		{
			var S s{};
			~s;   // Calling non-constexpr overloaded unary prefix operator in constexpr functions not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 6 )


def ConstexprCallLoop_Test0():
	c_program_text= """
		// This example triggers "FooA" constexpr call when its building isn't finished yet.
		auto x= FooA();
		fn constexpr FooA() : i32
		{
			if( false )
			{
				static_assert( FooB() == 33 );
			}
			return 33;
		}
		fn constexpr FooB() : i32
		{
			return FooA();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstexprFunctionEvaluationError", 8 ) )
	for error in errors_list:
		if error.error_code == "ConstexprFunctionEvaluationError":
			assert( error.text.find( "executing incomplete function" ) != -1 )
