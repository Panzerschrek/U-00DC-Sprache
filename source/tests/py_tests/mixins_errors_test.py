from py_tests_common import *


def ExpectedConstantExpression_ForMixins_Test0():
	c_program_text= """
		mixin( Foo() ); // Given function isn't constant.
		fn Foo() : [ char8, 128 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 2 ) )


def ExpectedConstantExpression_ForMixins_Test1():
	c_program_text= """
		auto mut s = "fn Foo();";
		mixin( s ); // Given variable is mutable.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 3 ) )


def TypesMismatch_ForMixins_Test0():
	c_program_text= """
		mixin( "var i32 x= 0;"u16 ); // For now support only UTF-8 strings.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 2 ) )


def TypesMismatch_ForMixins_Test1():
	c_program_text= """
		mixin( "var i32 x= 0;"u32 ); // For now support only UTF-8 strings.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 2 ) )


def TypesMismatch_ForMixins_Test2():
	c_program_text= """
		mixin( 0.25f ); // f32 given
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 2 ) )


def TypesMismatch_ForMixins_Test3():
	c_program_text= """
		mixin( arr ); // an integer array given
		var [ i32, 16 ] arr= zero_init;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 2 ) )


def TypesMismatch_ForMixins_Test4():
	c_program_text= """
		mixin( s ); // Struct given
		struct S{ char8 c; }
		var S s= zero_init;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 2 ) )


def MixinLexicalError_Test0():
	c_program_text= """
		mixin( " auto s= \\"\\\\urrrr\\"; " );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinLexicalError", 2 ) )


def MixinLexicalError_Test1():
	c_program_text= """

		// Lines in mixin expansions are counted started from mixin expansion line.

		mixin( "\\n\\nauto s= \\"\\\\urrrr\\"; " );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinLexicalError", 7 ) )


def MixinSyntaxError_Test0():
	c_program_text= """
		mixin( "auto s= " ); // statement isn't finished
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 2 ) )


def MixinSyntaxError_Test1():
	c_program_text= """
		mixin( "fn Foo()\\n{\\n lol\\n }" ); // syntactically-incorrect function body
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 5 ) )


def ErrorInsideMixin_Test0():
	c_program_text= """
		mixin( "var i32 f= 0.0;" ); // Converting f64 to i32
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "TypesMismatch", 2 ) )


def ErrorInsideMixin_Test1():
	c_program_text= """
		mixin( "fn Foo() : i32\\n{\\n \\n}\\n" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "NoReturnInFunctionReturningNonVoid", 5 ) )


def MixinRedefinition_Test0():
	c_program_text= """
		mixin( "var i32 x= 0;" );
		mixin( "namespace x{}" ); // Redefine one mixin name with another.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "Redefinition", 3 ) )


def MixinRedefinition_Test1():
	c_program_text= """
		var i32 x= 0;
		mixin( "namespace x{}" ); // Redefine non-mixin name with mixin name.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "Redefinition", 3 ) )


def MixinRedefinition_Test2():
	c_program_text= """
		mixin( "var i32 x= 0;" ); // Redefine non-mixin name with mixin name.
		namespace x{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "Redefinition", 2 ) )


def MixinRedefinition_Test3():
	c_program_text= """
		struct S
		{
			fn Foo(i32 x);
			mixin( "fn Foo(i32& x);" ); // Overload a method defined via mixin, but such overloading isn't allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "CouldNotOverloadFunction", 5 ) )


def MixinRedefinition_Test4():
	c_program_text= """
		mixin( "var i32 x= 0;" ); // Has no collision with "x" from SubSpace.
		namespace SubSpace
		{
			fn x(){} // Ok - redefine
		}
	"""
	tests_lib.build_program( c_program_text )


def MixinRedefinition_Test5():
	c_program_text= """
		auto x= 0.25;
		namespace SubSpace
		{
			mixin( "var i32 x= 0;" ); // Has no collision with "x" from outside.
		}
	"""
	tests_lib.build_program( c_program_text )
