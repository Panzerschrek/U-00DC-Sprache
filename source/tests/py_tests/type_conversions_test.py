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
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 12 ) )


def TypeConversion_InFunctionCall_Test4():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			template</ size_type size />
			fn conversion_constructor( [ char8, size ]& arr ) ( x= i32(size) ) {}
		}

		fn Extract( IntWrapper iw ) : i32 { return iw.x; }

		fn Foo() : i32
		{
			return Extract( "abcRf" ); // Call to tepmplate conversion constructor
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5 )


def TypeConversion_InFunctionCall_Test5():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor(){} // Prevent constexpr
		}

		template</ i32 mul />
		fn Extract( IntWrapper iw ) : i32 { return iw.x * mul; }

		fn Foo() : i32
		{
			return Extract</31/>( 856 ); // Should convert type in calling of template function, where argument is not template-dependent.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 856 * 31 )


def TypeConversion_InReturn_Test0():
	#conversion for fundamental type
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor(){} // Prevent constexpr
		}
		fn Wrap( i32 x ) : IntWrapper
		{
			return x; // Should convert to "IntWrapper" here.
		}
		fn Foo() : i32
		{
			return Wrap(123098).x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 123098 )


def TypeConversion_InReturn_Test1():
	#conversion in return disabled if have no conversion constructors
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {} // regular constructor, not conversion constructor
			fn destructor(){} // Prevent constexpr
		}
		fn Wrap( i32 x ) : IntWrapper
		{
			return x; // Should not convert to "IntWrapper" here.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 10 )


def TypeConversion_InReturn_Test2():
	# conversion for struct
	c_program_text= """
		struct A
		{
			i32 ax;
		}
		struct B
		{
			i32 bx;
			fn conversion_constructor( A a ) ( bx= a.ax ) {}
			fn destructor(){} // Prevent constexpr
		}
		fn Wrap( A a ) : B
		{
			return a; // Should convert to "B" here.
		}
		fn Foo() : i32
		{
			var A a{ .ax= 666786 };
			return Wrap(a).bx;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 666786 )


def TypeConversion_InReturn_Test3():
	# conversion for noncopyable type
	c_program_text= """
		class A
		{
			i32 ax= 654321;
		}
		class B
		{
			i32 bx;
			fn conversion_constructor( A a ) ( bx= a.ax ) {}
			fn destructor(){} // Prevent constexpr
		}
		fn Wrap( A mut a ) : B
		{
			return move(a); // Should convert to "B" here.
		}
		fn Foo() : i32
		{
			var A mut a;
			return Wrap( move(a) ).bx;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 654321 )


def TypeConversion_InReturn_Test4():
	# type conversion is disabled for reference result
	c_program_text= """
		struct A
		{
			i32 ax= 0;
		}
		struct B
		{
			i32 bx;
			fn conversion_constructor( A a ) ( bx= a.ax ) {}
			fn destructor(){} // Prevent constexpr
		}
		fn Wrap( A  a ) : B &
		{
			return a; // Should not convert to "B" here.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 14 )


def ConversionConstructorMustHaveOneArgument_Test0():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor() ( x= 0 ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConversionConstructorMustHaveOneArgument" )
	assert( errors_list[0].src_loc.line == 5 )


def ConversionConstructorMustHaveOneArgument_Test1():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( mut this ) ( x= 0 ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConversionConstructorMustHaveOneArgument" )
	assert( errors_list[0].src_loc.line == 5 )


def ConversionConstructorMustHaveOneArgument_Test2():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( i32 a, i32 b ) ( x= a * b ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConversionConstructorMustHaveOneArgument" )
	assert( errors_list[0].src_loc.line == 5 )


def ConversionConstructorMustHaveOneArgument_Test3():
	c_program_text= """
		struct IntWrapper
		{
			i32 x;
			fn conversion_constructor( mut this, i32 a, i32 b ) ( x= a * b ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConversionConstructorMustHaveOneArgument" )
	assert( errors_list[0].src_loc.line == 5 )


def DeepTypeConversionDisabled_Test0():
	c_program_text= """
		struct View
		{
			fn conversion_constructor( i32 x ){}
		}
		struct Str
		{
			fn conversion_constructor( View a ){}
		}
		fn Foo()
		{
			var Str str(42); // Can not convert 42 -> View -> Str
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 12 )


def DeepTypeConversionDisabled_Test1():
	c_program_text= """
		struct View
		{
			fn conversion_constructor( i32 x ){ halt; }
		}
		struct Str
		{
			fn conversion_constructor( View a ){}
			template</ />
			fn conversion_constructor( i32 x ) {}
		}
		fn Foo()
		{
			var Str str(42); // Must directly call Str::conversion_constructor(i32 x), not constructor of "View"
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def ConversionConstructorSourceTypeIsIdenticalToDestinationType_Test0():
	c_program_text= """
		struct S
		{
			fn conversion_constructor( S& other );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConversionConstructorSourceTypeIsIdenticalToDestinationType", 4 ) )


def ConversionConstructorSourceTypeIsIdenticalToDestinationType_Test1():
	c_program_text= """
		struct S
		{
			fn conversion_constructor( mut this, SAlias& other );
		}
		type SAlias= S;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConversionConstructorSourceTypeIsIdenticalToDestinationType", 4 ) )
