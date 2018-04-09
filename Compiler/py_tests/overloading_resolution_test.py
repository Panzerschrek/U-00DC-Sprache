from py_tests_common import *


def CouldNotOverloadFunction_Test0():
	c_program_text= """
		fn Bar();
		fn Bar() : i32;   // Error, difference only in return type.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotOverloadFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotOverloadFunction_Test1():
	c_program_text= """
		fn Bar( i32  x );
		fn Bar( i32& x );   // Error,value parameter and const-reference parameter have same overloading class.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotOverloadFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotOverloadFunction_Test2():
	c_program_text= """
		fn Bar( i32 mut  x );
		fn Bar( i32 imut x );   // Error, differs only mutability for value-parameter.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotOverloadFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotOverloadFunction_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
			i32 & r;
		}
		fn Bar( S &'x s'y' ) : i32 &'x;
		fn Bar( S &'x s'y' ) : i32 &'y;   // Error, differs only reference tag for return value.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotOverloadFunction" )
	assert( errors_list[0].file_pos.line == 8 )


def CouldNotOverloadFunction_Test4():
	c_program_text= """
		struct S
		{
			i32 & r;
		}
		fn Bar( S &mut s'y', i32&'x a );
		fn Bar( S &mut s'y', i32&'x a ) ' y <- x';   // Error, differs only reference pollution.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotOverloadFunction" )
	assert( errors_list[0].file_pos.line == 7 )


def OverloadingResolutionTest_OnlyMutabilityCheck_Test0():
	c_program_text= """
		fn Bar( i32 &imut x ) : i32 { return 111; }
		fn Bar( i32 & mut x ) : i32 { return 222; }
		fn Foo() : i32
		{
			var i32 mut x= 0;
			return Bar(x);  // Should select function with mutable reference argument, if given mutable reference.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 222 )


def OverloadingResolutionTest_OnlyMutabilityCheck_Test1():
	c_program_text= """
		fn Bar( i32 &imut x ) : i32 { return 111; }
		fn Bar( i32 & mut x ) : i32 { return 222; }
		fn Foo() : i32
		{
			var i32 x= 0;
			return Bar(x);  // Should select function with immutable reference argument, if given immutable reference.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 111 )


def OverloadingResolutionTest_OnlyMutabilityCheck_Test2():
	c_program_text= """
		fn Bar( i32 &imut x, i32& mut y ) : i32 { return 111; }
		fn Bar( i32 & mut x, i32&imut y ) : i32 { return 222; }
		fn Foo() : i32
		{
			var i32 mut x= 0, mut y= 0;
			return Bar(x, y);   // Error, given two "mut" args, can not select between (imut, mut) and (mut, imut)
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TooManySuitableOverloadedFunctions" )
	assert( errors_list[0].file_pos.line == 7 )


def OverloadingResolutionTest_OnlyMutabilityCheck_Test3():
	c_program_text= """
		fn Bar( i32 &imut x, i32&imut y ) : i32 { return 111; }
		fn Bar( i32 & mut x, i32& mut y ) : i32 { return 222; }
		fn Foo() : i32
		{
			var i32 mut x= 0, mut y= 0;
			return Bar(x, y);   // Should select (mut, mut)
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 222 )


def OverloadingResolutionTest_OnlyMutabilityCheck_Test4():
	c_program_text= """
		fn Bar( i32 &imut x, i32&imut y ) : i32 { return 111; }
		fn Bar( i32 &imut x, i32& mut y ) : i32 { return 222; }
		fn Foo() : i32
		{
			var i32 mut x= 0, mut y= 0;
			return Bar(x, y);   // Should select (imut, mut), which is better, then (imut, imut)
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 222 )


def OverloadingResolutionTest_OnlyMutabilityCheck_Test5():
	c_program_text= """
		fn Bar( i32 &imut x, i32&imut y ) : i32 { return 111; }
		fn Bar( i32 &imut x, i32& mut y ) : i32 { return 222; }
		fn Foo() : i32
		{
			var i32 imut x= 0, imut y= 0;
			return Bar(x, y);   // Should select (imut, imut), because (imut, mut) does not match.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 111 )


def OverloadingResolutionTest_OnlyMutabilityCheck_Test6():
	c_program_text= """
		fn Bar( i32 &imut x, i32 &imut y, i32 &imut z ) : i32 { return 111; }
		fn Bar( i32 &imut x, i32 &imut y, i32 & mut z ) : i32 { return 222; }
		fn Bar( i32 &imut x, i32 & mut y, i32 & mut z ) : i32 { return 333; }
		fn Foo() : i32
		{
			var i32 mut x= 0, mut y= 0, mut z= 0;
			return Bar(x, y, z);   // Should select (imut, mut, mut), because call to it contains less mutability conversions.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 333 )


def OverloadingResolutionTest_OnlyMutabilityCheck_Test7():
	c_program_text= """
		fn Bar( i32       x, i32      y ) : i32 { return 111; }
		fn Bar( i32 & mut x, i32& mut y ) : i32 { return 222; }
		fn Foo() : i32
		{
			var i32 mut x= 0, mut y= 0;
			return Bar(x, y);   // Should select (mut&, mut&), but not function with value-parameters.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 222 )


def OverloadingResolutionTest_OnlyMutabilityCheck_Test8():
	c_program_text= """
		fn Bar( i32       x, i32      y ) : i32 { return 111; }
		fn Bar( i32 & mut x, i32& mut y ) : i32 { return 222; }
		fn Foo() : i32
		{
			var i32 imut x= 0, imut y= 0;
			return Bar(x, y);   // Should select function with value parameters.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 111 )


def OverloadingResolutionTest_ReferenceConversions_Test0():
	c_program_text= """
		struct S{}
		fn Bar( S& s ) : i32 { return 111; }
		fn Bar( void& v ) : i32 { return 222; }
		fn Foo() : i32
		{
			var S s;
			return Bar(s);   // Should select Bar(S&), instead of Bar(void&). Any type is better, than void.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 111 )


def OverloadingResolutionTest_ReferenceConversions_Test1():
	c_program_text= """
		class A polymorph{}
		class B : A {}
		fn Bar( A& a ) : i32 { return 111; }
		fn Bar( B& b ) : i32 { return 222; }
		fn Foo() : i32
		{
			var B b;
			return Bar(b);   // Should select Bar(B&). Child is better, than parent.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 222 )


def OverloadingResolutionTest_ReferenceConversions_Test2():
	c_program_text= """
		class A polymorph{}
		class B : A {}
		class C : B {}
		fn Bar( A& a ) : i32 { return 111; }
		fn Bar( B& b ) : i32 { return 222; }
		fn Foo() : i32
		{
			var C c;
			return Bar(c);   // Should select Bar(B&). Conversion C->B is better, than C->A.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 222 )


def OverloadingResolutionTest_ReferenceConversions_Test3():
	c_program_text= """
		class A interface {}
		class B interface {}
		class C : A, B {}
		fn Bar( A& a ) : i32 { return 111; }
		fn Bar( B& b ) : i32 { return 222; }
		fn Foo() : i32
		{
			var C c;
			return Bar(c);   // Error, conversions ( C->A and C->B ) are incomparable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TooManySuitableOverloadedFunctions" )
	assert( errors_list[0].file_pos.line == 10 )


def OverloadingResolutionTest_ReferenceConversions_Test4():
	c_program_text= """
		class A interface {}
		class B interface {}
		class C : A, B {}
		class D : C {}
		fn Bar( A& a ) : i32 { return 111; }
		fn Bar( B& b ) : i32 { return 222; }
		fn Bar( C& c ) : i32 { return 333; }
		fn Foo() : i32
		{
			var D d;
			return Bar(d);   // Conversion ( D->C ) is better than conversions ( D->A ) and ( D->B ). Conversions ( D->A ) and ( D->B ) is incomparable here, but this does not matter.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 333 )


def OverloadingResolutionTest_MutabilityAndReferenceConversions_Test0():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Bar( A & mut a ) : i32 { return 111; }
		fn Bar( B &imut b ) : i32 { return 222; }
		fn Foo() : i32
		{
			var B mut b;
			return Bar(b);   // Error, conversions ( B &mut -> B &imut ) and ( B &mut ->  A &mut) are incomparable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TooManySuitableOverloadedFunctions" )
	assert( errors_list[0].file_pos.line == 9 )


def OverloadingResolutionTest_MutabilityAndReferenceConversions_Test1():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Bar( A &imut a ) : i32 { return 111; }
		fn Bar( B &imut b ) : i32 { return 222; }
		fn Foo() : i32
		{
			var B mut b;
			return Bar(b);   // Ok, mut->imut conversions in both cases, but in one of cases type conversion is better.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 222 )


def OverloadingResolutionTest_MutabilityAndReferenceConversions_Test2():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Bar( A &mut a ) : i32 { return 111; }
		fn Bar( B &mut b ) : i32 { return 222; }
		fn Foo() : i32
		{
			var B mut b;
			return Bar(b);   // Ok, no mutability conversions, one of type conversions is better.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 222 )


def OverloadingResolutionTest_MutabilityAndReferenceConversions_Tes3():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Bar( A &mut a ) : i32 { return 111; }
		fn Bar( B      b ) : i32 { return 222; }
		fn Foo() : i32
		{
			var B mut b;
			return Bar(b);   // Error, conversions ( B &mut -> B(value) ) and ( B &mut ->  A &mut) are incomparable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TooManySuitableOverloadedFunctions" )
	assert( errors_list[0].file_pos.line == 9 )


def OverloadingResolutionTest_MutabilityAndReferenceConversions_Tes4():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Bar( A &imut a ) : i32 { return 111; }
		fn Bar( B       b ) : i32 { return 222; }
		fn Foo() : i32
		{
			var B imut b;
			return Bar(b);   // Ok, conversion ( B &imut -> B(value) ) better, than ( B &imut ->  A &imut ).
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 222 )


def OverloadingResolutionTest_MutabilityAndReferenceConversions_Tes4():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Bar( A &imut a, i32 & mut x ) : i32 { return 111; }
		fn Bar( B &imut b, i32 &imut x ) : i32 { return 222; }
		fn Foo() : i32
		{
			var B imut b;
			var i32 mut x= 0;
			return Bar( b, x );  // Error, conversion for first arg ( B&imut -> B&imut ) is better, but for second arg conversion ( i32&mut -> i32& mut ) is better.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TooManySuitableOverloadedFunctions" )
	assert( errors_list[0].file_pos.line == 10 )


def OverloadingResolutionTest_MutabilityAndReferenceConversions_Tes5():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Bar( A &imut a, i32 &imut x ) : i32 { return 111; }
		fn Bar( B &imut b, void&imut x ) : i32 { return 222; }
		fn Foo() : i32
		{
			var B imut b;
			var i32 x= 0;
			return Bar( b, x );  // Error, conversion for first arg ( B&imut -> B&imut ) is better, but for second arg conversion ( i32&mut -> i32&imut ) is better.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TooManySuitableOverloadedFunctions" )
	assert( errors_list[0].file_pos.line == 10 )
