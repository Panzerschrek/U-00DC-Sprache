from py_tests_common import *


def ConstexprHalt_Test0():
	c_program_text= """
		fn constexpr Foo( i32 x ) : i32
		{
			halt if( (x&1) == 0 );
			return x | 0xFF;
		}

		auto constexpr x= Foo( 84 );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionEvaluationError" )
	assert( errors_list[0].file_pos.line == 8 )


def ConstexprFunctionsMustHaveBody_Test0():
	c_program_text= """
		fn constexpr Foo();
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionsMustHaveBody" )
	assert( errors_list[0].file_pos.line == 2 )


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
	assert( errors_list[0].file_pos.line == 4 )


def ConstexprFunctionCanNotBeVirtual_Test0():
	c_program_text= """
		class S polymorph
		{
			fn virtual constexpr Foo(){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionCanNotBeVirtual" )
	assert( errors_list[0].file_pos.line == 4 )


def ConstexprFunctionCanNotBeVirtual_Test1():
	c_program_text= """
		class S polymorph
		{
			op virtual constexpr [](){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionCanNotBeVirtual" )
	assert( errors_list[0].file_pos.line == 4 )


def InvalidTypeForConstexprFunction_Test0():
	c_program_text= """
		// mutable reference args not allowed.
		fn constexpr Foo( i32& mut x ){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstexprFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def InvalidTypeForConstexprFunction_Test1():
	c_program_text= """
		struct S{ i32& mut r; }  // Struct is not constexpr.
		// Argument type is not constexpr.
		fn constexpr Foo( S& x ){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstexprFunction" )
	assert( errors_list[0].file_pos.line == 4 )


def InvalidTypeForConstexprFunction_Test2():
	c_program_text= """
		// Unsafe function can not be constexpr.
		fn constexpr Foo() unsafe {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstexprFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def InvalidTypeForConstexprFunction_Test3():
	c_program_text= """
		// Function, returnig reference, can not be constexpr.
		fn constexpr Foo( i32& x ) : i32& { return x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstexprFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def InvalidTypeForConstexprFunction_Test4():
	c_program_text= """
		struct S{ i32& r; }
		// Function, returnig reference inside struct, can not be constexpr.
		fn constexpr Foo( i32&'a x ) : S'a'
		{
			var S s{ .r= x };
			return s;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstexprFunction" )
	assert( errors_list[0].file_pos.line == 4 )


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
	assert( errors_list[0].file_pos.line == 7 )


def InvalidTypeForConstexprFunction_Test6():
	c_program_text= """
		fn constexpr Foo( void& v ) {}   // Currently we can not process "void" arguments.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstexprFunction" )
	assert( errors_list[0].file_pos.line == 2 )


def ConstexprFunctionContainsUnallowedOperations_Test0():
	c_program_text= """
		// Unsafe blocks not allowed in constexpr functions.
		fn constexpr Foo() { unsafe{} }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].file_pos.line == 3 )


def ConstexprFunctionContainsUnallowedOperations_Test1():
	c_program_text= """
		fn NonConstexprFunction(){}
		// Calling non-constexpr functions in constexpr function not allowed.
		fn constexpr Foo() { NonConstexprFunction(); }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].file_pos.line == 4 )


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
	assert( errors_list[0].file_pos.line == 2 )


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
	assert( errors_list[0].file_pos.line == 3 )


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
	assert( errors_list[0].file_pos.line == 3 )


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
	assert( errors_list[0].file_pos.line == 6 )


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
	assert( errors_list[0].file_pos.line == 6 )
