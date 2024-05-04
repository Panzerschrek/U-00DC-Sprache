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


def ExpectedConstantExpression_ForMixins_Test2():
	c_program_text= """
		fn Foo()
		{
			var [ char8, 16 ] mut s= zero_init;
			mixin( s ); // Given string isn't constant, but mutable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 5 ) )


def ExpectedConstantExpression_ForMixins_Test3():
	c_program_text= """
		fn Foo( [ char8, 16 ]& s )
		{
			mixin( s ); // Given string isn't constant, but is function argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 4 ) )


def ExpectedConstantExpression_ForMixins_Test4():
	c_program_text= """
		fn Foo( [ char8, 16 ]& s ) : i32
		{
			return mixin( s ); // Given string isn't constant, but is function argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 4 ) )


def ExpectedConstantExpression_ForMixins_Test5():
	c_program_text= """
		type T= mixin( Foo() ); // Given string isn't constant, but is function call result.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 2 ) )


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


def MixinInvalidUTF8_Test0():
	c_program_text= """
		mixin( s );
		var[ char8, 1 ] s[ 230c8 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "MixinInvalidUTF8", 2 ) )


def MixinInvalidUTF8_Test1():
	c_program_text= """
		mixin( s );
		var[ char8, 3 ] s[ 199c8, "b"c8, 177c8 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "MixinInvalidUTF8", 2 ) )


def MixinInvalidUTF8_Test2():
	c_program_text= """
		mixin( s );
		auto s= "\\0  \\0 \\t  \\n \\0 \\0"; // Having zeros is considered to be valid.
	"""
	tests_lib.build_program( c_program_text )


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


def MixinLexicalError_Test2():
	c_program_text= """
		fn Foo()
		{
			mixin( " auto s= \\"\\\\urrrr\\"; " );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinLexicalError", 4 ) )


def MixinLexicalError_Test3():
	c_program_text= """
		fn Foo()
		{
			take( mixin( " auto s= \\"\\\\urrrr\\"; " ) );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinLexicalError", 4 ) )


def MixinLexicalError_Test4():
	c_program_text= """
		type T= mixin( " typeof( \\"\\\\urrrr\\" ) " );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinLexicalError", 2 ) )


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


def MixinSyntaxError_Test2():
	c_program_text= """
		mixin( "}" ); // "}" isn't expected here
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 2 ) )


def MixinSyntaxError_Test3():
	c_program_text= """
		struct S
		{
			mixin( "}" ); // "}" isn't expected here
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 4 ) )


def MixinSyntaxError_Test4():
	c_program_text= """
		namespace N
		{
			mixin( "}" ); // "}" isn't expected here
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 4 ) )


def MixinSyntaxError_Test5():
	c_program_text= """
		mixin( " ?macro <? Pass:expr ?e:expr ?>  ->  <? ?e ?> " ); // "Can't define a macro within mixin.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 2 ) )


def MixinSyntaxError_Test6():
	c_program_text= """
		fn Foo()
		{
			mixin( "auto x= 0" ); // missing ;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 4 ) )


def MixinSyntaxError_Test7():
	c_program_text= """
		fn Foo()
		{
			mixin( "lol what" ); // broken syntax
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 4 ) )


def MixinSyntaxError_Test8():
	c_program_text= """
		fn Foo()
		{
			mixin( "}" ); // unexpected }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 4 ) )


def MixinSyntaxError_Test9():
	c_program_text= """
		fn Foo()
		{
			mixin( " if(true){}\\n}" ); // unexpected }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 5 ) )


def MixinSyntaxError_Test10():
	c_program_text= """
		fn Foo()
		{
			take( mixin( "}" ) ); // unexpected }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 4 ) )


def MixinSyntaxError_Test11():
	c_program_text= """
		fn Foo()
		{
			take( mixin( "abyr valg" ) ); // meaningless expression
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 4 ) )


def MixinSyntaxError_Test12():
	c_program_text= """
		type T= mixin( " [ element_type " );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 2 ) )


def MixinSyntaxError_Test13():
	c_program_text= """
		type T= mixin( " Leck mich am Arsch! " );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 2 ) )


def MixinSyntaxError_Test14():
	c_program_text= """
		type T= mixin( " " ); // Only whitespaces.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 2 ) )


def MixinSyntaxError_Test15():
	c_program_text= """
		fn Foo()
		{
			type T= typeof( mixin( " \\t  \\n" ) ); // Only whitespaces.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 5 ) )


def MixinSyntaxError_Test16():
	c_program_text= """
		type T= mixin( "" ); // Empty string.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 2 ) )


def MixinSyntaxError_Test17():
	c_program_text= """
		fn Foo()
		{
			type T= typeof( mixin( "" ) ); // Empty string.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 4 ) )


def MixinSyntaxError_Test18():
	c_program_text= """
		type T= mixin( "\\0\\0\\0" ); // Zeros string.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 2 ) )


def MixinSyntaxError_Test19():
	c_program_text= """
		fn Foo()
		{
			type T= typeof( mixin( "\\0\\0\\0\\0" ) ); // Zeros string.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "MixinSyntaxError", 4 ) )


def MixinNamesAreNotVisibleInOtherMixinExpressions_Test0():
	c_program_text= """
		mixin( "var [ char8, 16 ] s= zero_init;" );
		mixin(s); // "s" is not visible, because evaluation of all mixin expressions happens before all mixins expansion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NameNotFound", 3 ) )


def MixinNamesAreNotVisibleInOtherMixinExpressions_Test1():
	c_program_text= """
		mixin(s); // "s" is not visible, because evaluation of all mixin expressions happens before all mixins expansion.
		mixin( "var [ char8, 16 ] s= zero_init;" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NameNotFound", 2 ) )


def MixinNamesAreNotVisibleInOtherMixinExpressions_Test2():
	c_program_text= """
		namespace Space
		{
			mixin( "var [ char8, 16 ] s= zero_init;" );
		}
		mixin(Space::s); // "Space::s" is not visible, because evaluation of all mixin expressions happens before all mixins expansion.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NameNotFound", 6 ) )


def MixinNamesAreNotVisibleInOtherMixinExpressions_Test3():
	c_program_text= """
		namespace SomeSpace
		{
			mixin(s); // "s" is not visible, because evaluation of all mixin expressions happens before all mixins expansion.
		}
		mixin( "var [ char8, 16 ] s= zero_init;" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NameNotFound", 4 ) )


def MixinNamesAreNotVisibleInOtherMixinExpressions_Test4():
	c_program_text= """
		template</type T/>
		struct SomeStruct
		{
			mixin(s); // "s" is not visible, because evaluation of all mixin expressions happens before all mixins expansion.
			mixin( "var [ char8, 16 ] s= zero_init;" );

			T t;
		}
		type X= SomeStruct</i32/>;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "TemplateContext" )
	assert( HasError( errors_list[0].template_errors.errors, "NameNotFound", 5 ) )


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


def ErrorInsideMixin_Test2():
	c_program_text= """
		namespace Some{ fn Foo(i32 x); }
		mixin( "fn Some::Foo(f32 y) {} " );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "FunctionDeclarationOutsideItsScope", 3 ) )


def ErrorInsideMixin_Test3():
	c_program_text= """
		fn Foo()
		{
			mixin( "return unknown_variable;" );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "NameNotFound", 4 ) )


def ErrorInsideMixin_Test4():
	c_program_text= """
		fn Foo()
		{
			take( mixin( "unknown_variable" ) );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "NameNotFound", 4 ) )


def ErrorInsideMixin_Test5():
	c_program_text= """
		type T= mixin( "UnknownType" );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "NameNotFound", 2 ) )


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
	if errors_list[0].error_code == "MacroExpansionContext":
		assert( HasError( errors_list[0].template_errors.errors, "CouldNotOverloadFunction", 5 ) )
	else:
		assert( HasError( errors_list, "CouldNotOverloadFunction", 4 ) )


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


def MixinRedefinition_Test6():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			mixin( "auto x= 5;" ); // Redefine local variable in block mixin.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "Redefinition", 5 ) )


def MixinRedefinition_Test7():
	c_program_text= """
		fn Foo()
		{
			mixin( "auto x= 5;" );
			var i32 x= 0; // Redefine local variable declared previously in block mixin.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "Redefinition", 5 ) )


def MixinExpansionDepthReached_Test0():
	c_program_text= """
		// Non-recursive deep mixin.
		mixin( mixin0_text );
		auto mixin0_text= "mixin( mixin1_text );";
		auto mixin1_text= "mixin( mixin2_text );";
		auto mixin2_text= "mixin( mixin3_text );";
		auto mixin3_text= " fn Foo() : i32 { return 7778; } ";
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "MixinExpansionDepthReached", 1 ) )


def MixinExpansionDepthReached_Test1():
	c_program_text= """
		// Recursive mixin.
		mixin( mixin_text );
		auto mixin_text= "mixin( mixin_text );";
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "MixinExpansionDepthReached", 1 ) )


def UnreachableCode_ForBlockMixin_Test0():
	c_program_text= """
		fn Foo()
		{
			return 42;
			mixin( "auto x= 5;" ); // Code inside mixin is unreachable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "UnreachableCode", 5 ) )


def UnreachableCode_ForBlockMixin_Test1():
	c_program_text= """
		fn Foo()
		{
			return 42;
			mixin( "" ); // Even empty mixin is not reachable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "UnreachableCode", 5 ) )


def UnreachableCode_ForBlockMixin_Test2():
	c_program_text= """
		fn Foo()
		{
			mixin( "return 67;" );
			if( true ) { Foo(); }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "UnreachableCode", 5 ) )


def UnreachableCode_ForBlockMixin_Test3():
	c_program_text= """
		fn Foo()
		{
			mixin( " return 42; var i32 x= 0; " ); // Code in second mixin statement isn't reachable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( HasError( errors_list[0].template_errors.errors, "UnreachableCode", 4 ) )


def UnreachableCode_ForBlockMixin_Test4():
	c_program_text= """
		fn Foo()
		{
			mixin( "return 37;" );
			mixin( "" ); // Code in second mixin expansion isn't reachable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "UnreachableCode", 5 ) )
