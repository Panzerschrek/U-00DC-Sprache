from py_tests_common import *


def ClassHaveNoCopyConstructorByDefault_Test0():
	c_program_text= """
		class A{}
		fn Foo()
		{
			var A a;
			var A a_copy(a);  // Error, "A" is noncopyable
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 6 )


def ClassHaveNoCopyConstructorByDefault_Test1():
	c_program_text= """
		class A{}
		fn CreateA() : A
		{
			var A a;
			return a; // Create copy of class in return
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CopyConstructValueOfNoncopyableType" )
	assert( errors_list[0].src_loc.line == 6 )


def ClassHaveNoCopyConstructorByDefault_Test2():
	c_program_text= """
		class A{}
		fn Foo()
		{
			var A a;
			auto a_copy= a; // Create copy of class in auto variable declaration
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CopyConstructValueOfNoncopyableType" )
	assert( errors_list[0].src_loc.line == 6 )


def ClassHaveNoCopyAssignementOperatorByDefault_Test0():
	c_program_text= """
		class A{}
		fn Foo()
		{
			var A mut a0, mut a1;
			a0= a1;  // Error, "A" is noncopyable
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OperationNotSupportedForThisType" )
	assert( errors_list[0].src_loc.line == 6 )


def CopyConstructorGeneration_Test0():
	c_program_text= """
		class A
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( A &imut other )= default;
		}

		fn Foo() : i32
		{
			var A a( 99951 );
			var A a_copy(a);
			return a_copy.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 99951 )


def CopyAssignentOperatorGeneration_Test0():
	c_program_text= """
		class A
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			op=( mut this, A &imut other )= default;
		}

		fn Foo() : i32
		{
			var A a0( 8885654 ), mut a1(0);
			a1= a0;
			return a1.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 8885654 )


def DefaultConstructorGeneration_Test0():
	c_program_text= """
		class A
		{
			i32 x;
			fn constructor() ( x= 666 ) {}
		}
		class B
		{
			A a;
			fn constructor()= default;
		}

		fn Foo() : i32
		{
			var B b;
			return b.a.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 666 )


def DisableDefaultConstructor_Test0():
	c_program_text= """
		struct A
		{
			fn constructor()= delete;
		}
		fn Foo()
		{
			var A a;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedInitializer" )
	assert( errors_list[0].src_loc.line == 8 )


def DisableDefaultConstructor_Test1():
	c_program_text= """
		class A
		{
			fn constructor()= delete;
		}
		fn Foo()
		{
			var A a;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedInitializer" )
	assert( errors_list[0].src_loc.line == 8 )


def DisableCopyConstructor_Test0():
	c_program_text= """
		struct A
		{
			fn constructor( A &imut other )= delete;
		}
		fn Foo()
		{
			var A a0;
			var A a1(a0);  // Error, copy-constructor deleted
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingDeletedMethod" )
	assert( errors_list[0].src_loc.line == 9 )


def DisableCopyAssignmentOperator_Test0():
	c_program_text= """
		struct A
		{
			op=( mut this, A& imut other )= delete;
		}
		fn Foo()
		{
			var A a0, mut a1;
			a1= a0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "AccessingDeletedMethod", 9 ) )


def BodyForGeneratedFunction_Test0():
	c_program_text= """
		struct S
		{
			fn constructor()= default;
		}
		fn S::constructor(){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "BodyForGeneratedFunction" )
	assert( errors_list[0].src_loc.line == 4 )


def BodyForGeneratedFunction_Test1():
	c_program_text= """
		struct S
		{
			fn constructor()= default;
			fn constructor(){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "BodyForGeneratedFunction" )
	assert( errors_list[0].src_loc.line == 4 )


def BodyForDeletedFunction_Test0():
	c_program_text= """
		struct S
		{
			fn constructor()= delete;
		}
		fn S::constructor(){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "BodyForDeletedFunction" )
	assert( errors_list[0].src_loc.line == 4 )


def BodyForDeletedFunction_Test1():
	c_program_text= """
		struct S
		{
			fn constructor()= delete;
			fn constructor(){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "BodyForDeletedFunction" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidMethodForBodyGeneration_Test0():
	c_program_text= """
		fn Foo()= default; // Non-class function.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidMethodForBodyGeneration" )
	assert( errors_list[0].src_loc.line == 2 )


def InvalidMethodForBodyGeneration_Test1():
	c_program_text= """
		class A
		{
			fn constructor( i32 x )= default; // Non-special constructor.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidMethodForBodyGeneration" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidMethodForBodyGeneration_Test2():
	c_program_text= """
		class A
		{
			op=( mut this, i32 x )= default; // Non-special assignemnt operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidMethodForBodyGeneration" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidMethodForBodyGeneration_Test3():
	c_program_text= """
		class A
		{
			fn Foo( this )= default; // Non-special method.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidMethodForBodyGeneration" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidMethodForBodyGeneration_Test4():
	c_program_text= """
		class A
		{
			op++( mut this )= default; // Non-special operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidMethodForBodyGeneration" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidMethodForBodyGeneration_Test5():
	c_program_text= """
		class A
		{
			fn destructor()= default; // There is no reason to set "=default" for destructor.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidMethodForBodyGeneration" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidMethodForBodyGeneration_Test6():
	c_program_text= """
		class A
		{
			fn destructor()= delete; // There is no reason to set "=delete" for destructor.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidMethodForBodyGeneration" )
	assert( errors_list[0].src_loc.line == 4 )


def MethodBodyGenerationFailed_Test0():
	c_program_text= """
		class Noncopyable{}
		class A
		{
			Noncopyable nc;
			fn constructor( A &imut other )= default;  // Error, can not generate copy constructor, because field is noncopyable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MethodBodyGenerationFailed" )
	assert( errors_list[0].src_loc.line == 6 )


def MethodBodyGenerationFailed_Test1():
	c_program_text= """
		class Noncopyable{}
		class A
		{
			Noncopyable nc;
			op=( mut this, A &imut other )= default;  // Error, can not generate copy assignment operator, because field is noncopyable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MethodBodyGenerationFailed" )
	assert( errors_list[0].src_loc.line == 6 )


def MethodBodyGenerationFailed_Test2():
	c_program_text= """
		class A
		{
			i32& r;
			op=( mut this, A &imut other )= default;  // Error, can not generate copy assignment operator, because field is reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MethodBodyGenerationFailed" )
	assert( errors_list[0].src_loc.line == 5 )


def MethodBodyGenerationFailed_Test3():
	c_program_text= """
		class Noncopyable polymorph {}
		class A : Noncopyable
		{
			op=( mut this, A &imut other )= default;  // Error, can not generate copy assignment operator, because base class is noncopyable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MethodBodyGenerationFailed" )
	assert( errors_list[0].src_loc.line == 5 )


def MethodBodyGenerationFailed_Test4():
	c_program_text= """
		class A
		{
			i32 x;
			fn constructor()= default; // Error, can not generate default constructor, because class contains non-default-constructible fields.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MethodBodyGenerationFailed" )
	assert( errors_list[0].src_loc.line == 5 )
