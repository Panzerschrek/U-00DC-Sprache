from py_tests_common import *


def CouldNotConvertFunctionPointer_Test0():
	c_program_text= """
		fn a( i32 &mut x ){}
		var ( fn( i32 &imut x ) ) constexpr ptr= a;   // Mutable to immutable argument conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotConvertFunctionPointer_Test1():
	c_program_text= """
		fn a( i32 x ){}
		var ( fn( i32 & x ) ) constexpr ptr= a;   // Value argument to reference argument conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotConvertFunctionPointer_Test2():
	c_program_text= """
		fn a( i32& x ){}
		var ( fn( i32 x ) ) constexpr ptr= a;   // Reference argument to value argument conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotConvertFunctionPointer_Test3():
	c_program_text= """
		fn a( i32&mut x ) : i32 &imut { return x; }
		var ( fn( i32&mut x ) : i32 &mut  ) constexpr ptr= a;   // Immutable to mutable return reference conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotConvertFunctionPointer_Test4():
	c_program_text= """
		fn a( i32& x ) : i32 { return x; }
		var ( fn( i32& x ) : i32&  ) constexpr ptr= a;   // Value return value to reference conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotConvertFunctionPointer_Test5():
	c_program_text= """
		fn a( i32& x ) : i32& { return x; }
		var ( fn( i32& x ) : i32  ) constexpr ptr= a;   // Reference return value to value conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotConvertFunctionPointer_Test6():
	c_program_text= """
		fn a( f32 x ){}
		var ( fn( i32 x ) ) constexpr ptr= a;   // Different argument types
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotConvertFunctionPointer_Test7():
	c_program_text= """
		fn a() : f32 {  return 0.0f;  }
		var ( fn() : i32 ) constexpr ptr= a;   // Different return value types.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotConvertFunctionPointer_Test8():
	c_program_text= """
		fn a( i32 a, i32 b ) {}
		var ( fn( i32 a ) ) constexpr ptr= a;   // Different argument count.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotConvertFunctionPointer_Test9():
	c_program_text= """
		fn a( i32 a ) {}
		var ( fn( i32 a, i32 b ) ) constexpr ptr= a;   // Different argument count.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotConvertFunctionPointer_Test10():
	c_program_text= """
		fn a() unsafe {}
		var ( fn() ) constexpr ptr= a;   // Unsafe to safe conversion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 3 )


def CouldNotConvertFunctionPointer_Test11():
	c_program_text= """
		type RetOnyFirstType= fn( i32&'x a, i32&'y b ) : i32&'x;

		fn RetBoth( i32& a, i32& b ) : i32&
		{
			if( a == 0 ){ return a; }
			return b;
		}

		var RetOnyFirstType constexpr ptr= RetBoth;   // Destination have less return references, than source function.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 10 )


def CouldNotConvertFunctionPointer_Test12():
	c_program_text= """
		struct S{ i32& r; }
		fn DoPolltion( S &mut s'a', i32&'b r ) ' a <- imut b ' {}
		var ( fn( S &mut s, i32& r ) )  constexpr ptr= DoPolltion;   // Destination have less references pollution, than source function.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].file_pos.line == 4 )


def FunctionPointerReferencesIsNotCompatible_Test0():
	c_program_text= """
		fn Foo( i32 &mut x ){}
		var ( fn(i32 & mut x ) )  constexpr ptr= Foo;
		var ( fn(i32 &imut x ) ) &constexpr ref_to_ptr= ptr;  // Error, references to function pointers are not compatible.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 4 )


def FunctionPointerReferencesIsNotCompatible_Test1():
	c_program_text= """
		fn Foo( i32 &imut x ){}
		var ( fn(i32 &imut x ) )  constexpr ptr= Foo;
		var ( fn(i32 & mut x ) ) &constexpr ref_to_ptr= ptr;  // Error, references is not compatible, even if function pointers iteslf are compativble.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 4 )
