from py_tests_common import *

def GeneratedDefaultConstructorConstexprFlagDependsOnInClassFieldInitializer_Test0():
	c_program_text= """
		fn constexpr GetX() : i32 { return 0; }
		struct S
		{
			i32 x= GetX(); // Constexpr initializer.
		}

		fn constexpr Foo()
		{
			var S s; // Call here generated default constructor. It is "constexpr", because all fields initializers are constexpr.
		}
	"""
	tests_lib.build_program( c_program_text )


def GeneratedDefaultConstructorConstexprFlagDependsOnInClassFieldInitializer_Test1():
	c_program_text= """
		fn GetX() : i32;
		struct S
		{
			i32 x= GetX(); // Non-constexpr initializer.
		}

		fn constexpr Foo()
		{
			var S s; // Call here generated default constructor. It is not "constexpr", because one of fields initializers is not "constexpr".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 8 )


def GeneratedDefaultConstructorConstexprFlagDependsOnInClassFieldInitializer_Test2():
	c_program_text= """
		auto zero= 0;
		struct S
		{
			i32& x= zero; // Constexpr initializer.
		}

		fn constexpr Foo()
		{
			var S s; // Call here generated default constructor. It is "constexpr", because all fields initializers are constexpr.
		}
	"""
	tests_lib.build_program( c_program_text )


def GeneratedDefaultConstructorConstexprFlagDependsOnInClassFieldInitializer_Test3():
	c_program_text= """
		fn GetX() : i32&;
		struct S
		{
			i32& x= GetX(); // Non-constexpr initializer.
		}

		fn constexpr Foo()
		{
			var S s; // Call here generated default constructor. It is not "constexpr", because one of fields initializers is not "constexpr".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 8 )


def GeneratedDefaultConstructorConstexprFlagDependsOnInClassFieldInitializer_Test4():
	c_program_text= """
		struct A
		{
			i32 x;
			fn constexpr constructor(i32 in_x)( x= in_x ) {}
		}
		struct B
		{
			A a(42);
		}
		fn constexpr Foo()
		{
			var B b; // Call here generated default constructor. It is "constexpr", because all fields initializers are constexpr.
		}
	"""
	tests_lib.build_program( c_program_text )


def GeneratedDefaultConstructorConstexprFlagDependsOnInClassFieldInitializer_Test5():
	c_program_text= """
		struct A
		{
			i32 x;
			fn constructor(i32 in_x)( x= in_x ) {}
		}
		struct B
		{
			A a(42);
		}
		fn constexpr Foo()
		{
			var B b; // Call here generated default constructor. It is not "constexpr", because one of fields initializers is not "constexpr".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 11 )


def GeneratedDefaultConstructorConstexprFlagDependsOnFieldDefaultConstructor_Test0():
	c_program_text= """
		struct A
		{
			i32 x;
			fn constexpr constructor()( x= 777 ) {}
		}
		struct B
		{
			A a;
			// Default constructor should be "constexpr", because field default constructor is "constexpr".
		}
		fn constexpr Foo()
		{
			var B b; // Call here generated default constructor. It is "constexpr", because all fields initializers are constexpr.
		}
	"""
	tests_lib.build_program( c_program_text )


def GeneratedDefaultConstructorConstexprFlagDependsOnFieldDefaultConstructor_Test1():
	c_program_text= """
		struct A
		{
			i32 x;
			fn constructor();
		}
		struct B
		{
			A a;
			// Default constructor should not be "constexpr", because field default constructor is not "constexpr".
		}
		fn constexpr Foo()
		{
			var B b; // Call here generated default constructor. It is not "constexpr", because one of fields initializers is not "constexpr".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 12 )


def GeneratedMethodIsNotConstexprAsExpected_Test0():
	c_program_text= """
		fn GetX() : i32;
		struct S
		{
			i32 x= GetX();
			fn constexpr constructor() = default; // In-class field initializer is not "constexpr", result default constructor will not be "constexpr", as user requested.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 6 )


def GeneratedMethodIsNotConstexprAsExpected_Test1():
	c_program_text= """
		struct A
		{
			i32 x;
			fn constructor() ( x= 12345 ) {} // Non-constexpr constructor
		}
		struct B
		{
			A a;
			fn constexpr constructor() = default; // Field initializer is not "constexpr", result default constructor will not be "constexpr", as user requested.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 10 )


def GeneratedMethodIsNotConstexprAsExpected_Test2():
	c_program_text= """
		class C
		{
			fn constexpr constructor() = default; // This constructor can't be "constexpr", because "this" param is not "constexpr", because class can't be "constexpr".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstexprFunctionContainsUnallowedOperations" )
	assert( errors_list[0].src_loc.line == 4 )
