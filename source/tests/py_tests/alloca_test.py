from py_tests_common import *


def AllocaDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			// Size is static.
			alloca i32 arr[ 16s ];
			static_assert( same_type</ typeof(arr), $(i32) /> );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AllocaDeclaration_Test1():
	c_program_text= """
		fn Foo(u32 size)
		{
			// Size is dynamic.
			alloca u16 mem[ size_type(size) ];
			static_assert( same_type</ typeof(mem), $(u16) /> );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 128 )


def AllocaDeclaration_Test2():
	c_program_text= """
		fn Foo(u32 size)
		{
			// Conditional "alloca"
			if( size % 2u == 0u )
			{
				alloca char32 mem[ size_type(size) ];
				static_assert( same_type</ typeof(mem), $(char32) /> );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 33 )
	tests_lib.run_function( "_Z3Fooj", 34 )


def AllocaDeclaration_Test3():
	c_program_text= """
		fn Foo(u32 size)
		{
			// Potentially use heap fallback.
			alloca byte8 mem[ size_type(size) ];
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 80 )
	tests_lib.run_function( "_Z3Fooj", 800 )
	tests_lib.run_function( "_Z3Fooj", 8000 )
	tests_lib.run_function( "_Z3Fooj", 80000 )


def AllocaDeclaration_Test4():
	c_program_text= """
		fn Foo(u32 size)
		{
			// Create two allocations.
			var size_type s(size);
			alloca u32 ints[ s ];
			alloca f64 floats[ s ];
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 33 )


def AllocaDeclaration_Test5():
	c_program_text= """
		fn Foo(u32 size)
		{
			// Create two allocations in different branches.
			var size_type s(size);
			if( size % 2u == 0u )
			{
				alloca u32 ints[ s ];
			}
			else
			{
				alloca f64 floats[ s ];
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 33 )
	tests_lib.run_function( "_Z3Fooj", 34 )


def AllocaDeclaration_Test6():
	c_program_text= """
		fn Foo(u32 size)
		{
			// Alloca declaration in loop.
			// It's fine since it's scoped and memory is releazed after each iteration.
			for( auto mut i= 0u; i < size; ++i )
			{
				alloca char16 mem[ size_type(size) ];
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 10 )


def AllocaDeclaration_Test7():
	c_program_text= """
		class C
		{
			i32 x;
			f32 y;
		}
		static_assert( !typeinfo</C/>.is_default_constructible );
		fn Foo(u32 size)
		{
			// No constructors are called for "alloca".
			// So, it doesn't matter if this class isn't default-constructible at all.
			alloca C arr[ size_type(size) ];

		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 10 )


def AllocaDeclaration_Test8():
	c_program_text= """
		class C
		{
			i32 x;
			f32 y;
			// This constructor will crash.
			fn constructor()
				( x(0), y(0) )
			{
				halt;
			}
		}
		fn Foo(u32 size)
		{
			// No constructors are called for "alloca".
			// So no crash because of constructor call is possible.
			alloca C arr[ size_type(size) ];

		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 10 )


def AllocaDeclaration_Test9():
	c_program_text= """
		class C
		{
			i32 x;
			f32 y;
			// This destructor will crash.
			fn destructor()
			{
				halt;
			}
		}
		fn Foo(u32 size)
		{
			// No destructors are called for "alloca".
			// So no crash because of destructor call is possible.
			alloca C arr[ size_type(size) ];

		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 10 )


def UsingKeywordAsName_ForAllocaDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			alloca i32 virtual[ 16s ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UsingKeywordAsName", 4 ) )


def Redefinition_ForAllocaDeclaration_Test0():
	c_program_text= """
		fn Foo( i32 some_var )
		{
			alloca i32 some_var[ 16s ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "Redefinition", 4 ) )


def Redefinition_ForAllocaDeclaration_Test1():
	c_program_text= """
		fn Foo()
		{
			var f64 some_var = zero_init;
			alloca i32 some_var[ 16s ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "Redefinition", 5 ) )


def AllocaVariableIsImmutable_Test0():
	c_program_text= """
		fn Foo()
		{
			alloca i32 arr[ 16s ];
			move(arr); // Can't move - "arr" is immutable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 5 ) )


def AllocaVariableIsImmutable_Test1():
	c_program_text= """
		fn Foo()
		{
			alloca i32 arr[ 16s ];
			Bar(ptr); // Can't call this function - it requires mutable reference.
		}
		fn Bar( $(i32) &mut ptr );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 5 ) )


def AllocaVariableIsImmutable_Test2():
	c_program_text= """
		fn Foo()
		{
			alloca i32 arr[ 16s ];
			var $(i32) &mut ref= arr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "BindingConstReferenceToNonconstReference", 5 ) )


def TypesMismatch_ForAllocaDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			alloca i32 some_var[ 67 ]; // Size needs to be "size_type"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def TypesMismatch_ForAllocaDeclaration_Test1():
	c_program_text= """
		fn Foo(f32 x )
		{
			alloca i32 some_var[ x ]; // Size needs to be "size_type"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def TypesMismatch_ForAllocaDeclaration_Test2():
	c_program_text= """
		fn Foo( S s )
		{
			alloca i32 some_var[ s ]; // Size needs to be "size_type"
		}
		struct S{ size_type v; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def AllocaDeclaration_IsNotConstexpr_Test0():
	c_program_text= """
		fn constexpr Foo()
		{
			// For now can't use "alloca" in constexpr functions.
			alloca i32 arr[ 16s ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstexprFunctionContainsUnallowedOperations", 2 ) )


def AllocaDeclarationInsideCorouine_Test0():
	c_program_text= """
		fn async Foo()
		{
			alloca f32 ptr[16s];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AllocaInsideCorouine", 4 ) )


def AllocaDeclarationInsideCorouine_Test1():
	c_program_text= """
		fn generator Foo()
		{
			alloca u32 ptr[8s];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AllocaInsideCorouine", 4 ) )
