from py_tests_common import *


def ConversionConstructorIsConstructor_Test0():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
		}

		fn Foo() : i32
		{
			var IntWrapper i( 85411 ); // Conversion constructor may be called as regular constructor.
			return i.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 85411 )


def ConversionConstructorIsConstructor_Test1():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
		}

		struct S
		{
			IntWrapper iw;
		}

		fn Foo() : i32
		{
			var S s{ .iw(66541) }; // Conversion constructor may be called as regular constructor.
			return s.iw.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 66541 )


def TypeConversion_InExpressionInitializer_Test0():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
		}

		fn Foo() : i32
		{
			var IntWrapper i= 185474; // Conversion occurs here.
			return i.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 185474 )


def TypeConversion_InExpressionInitializer_Test1():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
		}

		struct S
		{
			IntWrapper iw;
		}

		fn Foo() : i32
		{
			var S s{ .iw= 125211 }; // Conversion in initializer of struct member.
			return s.iw.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 125211 )


def TypeConversion_InExpressionInitializer_Test2():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
		}

		fn Foo() : i32
		{
			var [ IntWrapper, 1 ] arr[ 55524 ]; // Conversion in construction of array element.
			return arr[0u].x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 55524 )


def TypeConversion_InFunctionCall_Test0():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
		}

		fn Extract( IntWrapper iw ) : i32 { return iw.x; }

		fn Foo() : i32
		{
			return Extract( 854777 ); // Type conversion for value arg.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 854777 )


def TypeConversion_InFunctionCall_Test1():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
		}

		fn Extract( IntWrapper &imut iw ) : i32 { return iw.x; }

		fn Foo() : i32
		{
			return Extract( 652321 ); // Type conversion for const-reference arg.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 652321 )


def TypeConversion_InFunctionCall_Test2():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
		}

		fn Foo() : i32
		{
			var IntWrapper mut iw(0);
			iw= 65411; // Conversion in calling of assignment operator.
			return iw.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 65411 )


def TypeConversion_InFunctionCall_Test3():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
		}

		fn Extract( IntWrapper &mut iw ) : i32 { return iw.x; }

		fn Foo() : i32
		{
			return Extract( 652321 ); // Can not convert type, if function requires mutable argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 12 )
