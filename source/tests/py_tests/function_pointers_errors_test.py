from py_tests_common import *


def CouldNotConvertFunctionPointer_Test0():
	c_program_text= """
		fn a( i32 &mut x ){}
		fn Foo(){  var ( fn( i32 &imut x ) ) ptr= a;  }   // Mutable to immutable argument conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 3 )


def CouldNotConvertFunctionPointer_Test1():
	c_program_text= """
		fn a( i32 x ){}
		fn Foo(){  var ( fn( i32 & x ) ) ptr= a;  }   // Value argument to reference argument conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 3 )


def CouldNotConvertFunctionPointer_Test2():
	c_program_text= """
		fn a( i32& x ){}
		fn Foo(){  var ( fn( i32 x ) ) ptr= a;  }   // Reference argument to value argument conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 3 )


def CouldNotConvertFunctionPointer_Test3():
	c_program_text= """
		fn a( i32&mut x ) : i32 &imut { return x; }
		fn Foo(){  var ( fn( i32&mut x ) : i32 &mut  ) ptr= a;  }   // Immutable to mutable return reference conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 3 )


def CouldNotConvertFunctionPointer_Test4():
	c_program_text= """
		fn a( i32& x ) : i32 { return x; }
		fn Foo(){  var ( fn( i32& x ) : i32&  ) ptr= a;  }   // Value return value to reference conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 3 )


def CouldNotConvertFunctionPointer_Test5():
	c_program_text= """
		fn a( i32& x ) : i32& { return x; }
		fn Foo(){  var ( fn( i32& x ) : i32  ) ptr= a;  }   // Reference return value to value conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 3 )


def CouldNotConvertFunctionPointer_Test6():
	c_program_text= """
		fn a( f32 x ){}
		fn Foo(){  var ( fn( i32 x ) ) ptr= a;  }  // Different argument types
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 3 )


def CouldNotConvertFunctionPointer_Test7():
	c_program_text= """
		fn a() : f32 {  return 0.0f;  }
		fn Foo(){  var ( fn() : i32 ) ptr= a;  }   // Different return value types.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 3 )


def CouldNotConvertFunctionPointer_Test8():
	c_program_text= """
		fn a( i32 a, i32 b ) {}
		fn Foo(){  var ( fn( i32 a ) ) ptr= a;  }   // Different argument count.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 3 )


def CouldNotConvertFunctionPointer_Test9():
	c_program_text= """
		fn a( i32 a ) {}
		fn Foo(){  var ( fn( i32 a, i32 b ) ) ptr= a;  }   // Different argument count.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 3 )


def CouldNotConvertFunctionPointer_Test10():
	c_program_text= """
		fn a() unsafe {}
		fn Foo(){  var ( fn() ) ptr= a;  }   // Unsafe to safe conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 3 )


def CouldNotConvertFunctionPointer_Test11():
	c_program_text= """
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		type RetOnyFirstType= fn( i32& a, i32& b ) : i32& @(return_references);

		fn RetBoth( i32& a, i32& b ) : i32&
		{
			if( a == 0 ){ return a; }
			return b;
		}

		fn Foo(){  var RetOnyFirstType ptr= RetBoth;  }   // Destination have less return references, than source function.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 11 )


def CouldNotConvertFunctionPointer_Test12():
	c_program_text= """
		struct S{ i32& r; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPolltion( S &mut s, i32& r ) @(pollution) {}
		fn Foo(){  var ( fn( S &mut s, i32& r ) ) ptr= DoPolltion;  }   // Destination have less references pollution, than source function.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 5 )


def CouldNotSelectFunctionForPointer_Test0():
	c_program_text= """
		fn a( i32& mut x, i32&imut y ){}
		fn a( i32&imut x, i32& mut y ){}
		fn Foo(){  var ( fn( i32&mut x, i32&mut y ) ) ptr= a;  }  // Error, exist more, than one function, convertible to pointer type, but not equal to it.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TooManySuitableOverloadedFunctions" )
	assert( errors_list[0].src_loc.line == 4 )


def FunctionPointerReferencesIsNotCompatible_Test0():
	c_program_text= """
		fn Foo( i32 &mut x ){}
		var ( fn(i32 & mut x ) )  constexpr ptr= Foo;
		var ( fn(i32 &imut x ) ) &constexpr ref_to_ptr= ptr;  // Error, references to function pointers are not compatible.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 4 )


def FunctionPointerReferencesIsNotCompatible_Test1():
	c_program_text= """
		fn Foo( i32 &imut x ){}
		var ( fn(i32 &imut x ) )  constexpr ptr= Foo;
		var ( fn(i32 & mut x ) ) &constexpr ref_to_ptr= ptr;  // Error, references is not compatible, even if function pointers iteslf are compativble.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 4 )


def InvalidFunctionArgumentCount_Test0():
	c_program_text= """
		fn a(){}
		fn Foo()
		{
			var (fn()) ptr= a;
			ptr(42);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidFunctionArgumentCount" )
	assert( errors_list[0].src_loc.line == 6 )


def InvalidFunctionArgumentCount_Test1():
	c_program_text= """
		fn a( i32 x ){}
		fn Foo()
		{
			var (fn( i32 x )) ptr= a;
			ptr();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidFunctionArgumentCount" )
	assert( errors_list[0].src_loc.line == 6 )


def TypesMismatch_InFunctionPointerCall_Test0():
	c_program_text= """
		fn a( i32 x ){}
		fn Foo()
		{
			var (fn( i32 x )) ptr= a;
			ptr( 4.0f );   // expected 'i32', given 'f32'
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 6 )


def TypesMismatch_InFunctionPointerCall_Test1():
	c_program_text= """
		fn a( i32& x ){}
		fn Foo()
		{
			var (fn( i32& x )) ptr= a;
			ptr( 4.0f );   // expected 'i32', given 'f32'
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 6 )


def ExpectedMutableReference_InFunctionPointerCall_Test0():
	c_program_text= """
		fn a( i32&mut x ){}
		fn Foo()
		{
			var (fn( i32&mut x )) ptr= a;
			auto x= 0;
			ptr( x );   // expected mutable reference, got immutable reference
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedMutableReference" )
	assert( errors_list[0].src_loc.line == 7 )


def ExpectedMutableReference_InFunctionPointerCall_Test0():
	c_program_text= """
		fn a( i32&mut x ){}
		fn Foo()
		{
			var (fn( i32&mut x )) ptr= a;
			ptr( 66 );   // expected mutable reference, got value
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedMutableReference" )
	assert( errors_list[0].src_loc.line == 6 )


def TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test0():
	c_program_text= """
		template</ type T />
		struct S</ fn( T x ) /> {}
		fn Foo()
		{
			var S</ fn() /> s;  // expected function with one argument, given function with zero arguments.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TemplateParametersDeductionFailed", 6 ) )


def TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test1():
	c_program_text= """
		template</ />
		struct S</ fn() /> {}
		fn Foo()
		{
			var S</ fn( i32 x ) /> s;  // expected function with zero argument, given function with one argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TemplateParametersDeductionFailed", 6 ) )


def TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test2():
	c_program_text= """
		template</ type T />
		struct S</ fn( T x ) : i32 /> {}
		fn Foo()
		{
			var S</ fn( i32 x ) /> s;  // expected function, returning 'i32', get function, returning 'void'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TemplateParametersDeductionFailed", 6 ) )


def TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test3():
	c_program_text= """
		template</ type T />
		struct S</ fn( T x ) /> {}
		fn Foo()
		{
			var S</ fn( i32 x ) unsafe /> s;  // Given unsafe function, expected safe function.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TemplateParametersDeductionFailed", 6 ) )


def TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test4():
	c_program_text= """
		template</ type T />
		struct S</ fn( T x ) unsafe /> {}
		fn Foo()
		{
			var S</ fn( i32 x ) /> s;  // Given safe function, expected unsafe function.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TemplateParametersDeductionFailed", 6 ) )


def TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test5():
	c_program_text= """
		template</ type T />
		struct S</ fn( T& x ) /> {}
		fn Foo()
		{
			var S</ fn( i32 x ) /> s;  // Expected reference argument, given value argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TemplateParametersDeductionFailed", 6 ) )


def TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test6():
	c_program_text= """
		template</ type T />
		struct S</ fn( T x ) /> {}
		fn Foo()
		{
			var S</ fn( i32& x ) /> s;  // Expected value argument, given reference argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TemplateParametersDeductionFailed", 6 ) )


def TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test7():
	c_program_text= """
		template</ type T />
		struct S</ fn( T& x ) /> {}
		fn Foo()
		{
			var S</ fn( i32&mut x ) /> s;  // Expected immutable argument, given mutable argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TemplateParametersDeductionFailed", 6 ) )


def TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test8():
	c_program_text= """
		template</ type T />
		struct S</ fn( T& t ) : T /> {}
		fn Foo()
		{
			var S</ fn( i32& x ) : i32& /> s;  // Expected returning value, got returning reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TemplateParametersDeductionFailed", 6 ) )


def TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test9():
	c_program_text= """
		template</ type T />
		struct S</ fn( T& t ) : T& /> {}
		fn Foo()
		{
			var S</ fn( i32& x ) : i32 /> s;  // Expected returning reference, got returning value.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TemplateParametersDeductionFailed", 6 ) )


def TemplateDeductionFailed_WithFunctionPointerTypeAsTemplateSignatureArgument_Test10():
	c_program_text= """
		template</ type T />
		struct S</ fn( T t ) : T /> {}
		fn Foo()
		{
			var S</ fn( i32 x ) : f32 /> s;  // Expected same types of argument and return value.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TemplateParametersDeductionFailed", 6 ) )


def ImplicitFunctionsSetToPointerConversionErrors_Test0():
	c_program_text= """
		fn Bar( f32 x );
		fn Bar( bool b );
		fn Foo()
		{
			var ( fn( f32 x ) ) mut ptr= zero_init;
			// Error - conversion doesn't work, because there are two functions with the same name.
			// It doesn't work even if assignment destination type is known, since it is not known at conversion point.
			ptr= Bar;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedVariable", 9 ) )


def ImplicitFunctionsSetToPointerConversionErrors_Test1():
	c_program_text= """
		fn Bar( f32 x );
		template</type T/> fn Bar( T t ) {}
		fn Foo()
		{
			var ( fn( f32 x ) ) mut ptr= zero_init;
			// Conversion doesn't work because of template function.
			ptr= Bar;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedVariable", 8 ) )


def ImplicitFunctionsSetToPointerConversionErrors_Test2():
	c_program_text= """
		template</type T/> fn Bar( T t ) {}
		fn Baz( ( fn( f32 x ) ) ptr );
		fn Foo()
		{
			// Conversion doesn't work because the single possible function is template.
			Baz( Bar );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedVariable", 7 ) )


def ImplicitFunctionsSetToPointerConversionErrors_Test3():
	c_program_text= """
		fn Bar( i32 x ) {}
		fn Baz( ( fn( f32 x ) ) ptr );
		fn Foo()
		{
			// Conversion works, but function pointer type is wrong.
			Baz( Bar );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 7 ) )


def FunctionPointersHaveConstructorsWithExactlyOneParameter_Test0():
	c_program_text= """
		fn Foo()
		{
			var (fn()) ptr( Bar, Bar ); // Too many arguments.
		}
		fn Bar();
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionPointersHaveConstructorsWithExactlyOneParameter", 4 ) )


def FunctionPointersHaveConstructorsWithExactlyOneParameter_Test1():
	c_program_text= """
		fn Foo()
		{
			var (fn()) ptr(); // Too few arguments.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionPointersHaveConstructorsWithExactlyOneParameter", 4 ) )
