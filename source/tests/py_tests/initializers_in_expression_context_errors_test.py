from py_tests_common import *

def NameIsNotTypeName_ForStructInitializationExpression_Test0():
	c_program_text= """
		fn Foo()
		{
			auto x= Foo{}; // Expected type name for {} initializer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NameIsNotTypeName", 4 ) )


def NameIsNotTypeName_ForStructInitializationExpression_Test1():
	c_program_text= """
		auto some_var= 55;
		auto x= some_var{ .x= 754 }; // Expected type name for {} initializer.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NameIsNotTypeName", 3 ) )


def NameIsNotTypeName_ForStructInitializationExpression_Test2():
	c_program_text= """
		auto x= 42{}; // Expected type name for {} initializer, got numeric value.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NameIsNotTypeName", 2 ) )


def NameIsNotTypeName_ForStructInitializationExpression_Test3():
	c_program_text= """
		var i32 x= 65, y= 4;
		auto z= (x / y){}; // Expected type name for {} initializer, got binary expression result.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NameIsNotTypeName", 3 ) )


def ConstructingAbstractClassOrInterface_ForStructInitializationExpression_Test0():
	c_program_text= """
		class A abstract {}
		fn Foo()
		{
			auto a= A{};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstructingAbstractClassOrInterface", 5 ) )


def ConstructingAbstractClassOrInterface_ForStructInitializationExpression_Test1():
	c_program_text= """
		class A interface {}
		fn Foo()
		{
			auto a= A{};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstructingAbstractClassOrInterface", 5 ) )


def StructInitializerForNonStruct_ForStructInitializationExpression_Test0():
	c_program_text= """
		fn Foo()
		{
			auto x= i32{};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StructInitializerForNonStruct", 4 ) )


def StructInitializerForNonStruct_ForStructInitializationExpression_Test1():
	c_program_text= """
		fn Foo()
		{
			auto x= tup[i32, f32]{};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StructInitializerForNonStruct", 4 ) )


def StructInitializerForNonStruct_ForStructInitializationExpression_Test2():
	c_program_text= """
		fn Foo()
		{
			auto x= [u32, 4]{};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StructInitializerForNonStruct", 4 ) )


def StructInitializerForNonStruct_ForStructInitializationExpression_Test3():
	c_program_text= """
		fn Foo()
		{
			auto x= C{}; // {} initializer can't be used for classes.
		}
		class C{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StructInitializerForNonStruct", 4 ) )


def InitializerDisabledBecauseClassHasExplicitNoncopyConstructors_ForStructInitializationExpression_Test0():
	c_program_text= """
		fn Foo()
		{
			auto x= S{ .x= 42 }; // Can't use {} initializer, because class have a constructor.
		}
		struct S
		{
			i32 x;
			fn constructor(f32 y);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InitializerDisabledBecauseClassHasExplicitNoncopyConstructors", 4 ) )


def NameNotFound_ForStructField_ForStructInitializationExpression_Test0():
	c_program_text= """
		struct S{ i32 x; }
		fn Foo()
		{
			auto s= S{ .y= 0 }; // "y" isn't known.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NameNotFound", 5 ) )


def InitializerForNonfieldStructMember_ForStructInitializationExpression_Test0():
	c_program_text= """
		struct S{ var i32 x; }
		fn Foo()
		{
			auto s= S{ .x= 0 }; // "x" isn't a field but a global variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InitializerForNonfieldStructMember", 5 ) )


def DuplicatedStructMemberInitializer_ForStructInitializationExpression_Test0():
	c_program_text= """
		struct S{ i32 x; }
		fn Foo()
		{
			auto s= S{ .x= 0, .x= 0 };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DuplicatedStructMemberInitializer", 5 ) )


def ExpectedInitializer_ForStructInitializationExpression_Test0():
	c_program_text= """
		struct S{ i32 x; }
		fn Foo()
		{
			auto s= S{}; // "x" remains uninitialized.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedInitializer", 5 ) )


def ExpectedInitializer_ForStructInitializationExpression_Test1():
	c_program_text= """
		struct S{ u32 x; f32 y= 0.0f; bool z; }
		fn Foo()
		{
			auto s= S{ .z= false }; // "x" remains uninitialized.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedInitializer", 5 ) )


def ExpectedReferenceValue_ForStructInitializationExpression_Test0():
	c_program_text= """
		struct S{ i32 x; }
		fn Foo()
		{
			auto& s= S{ .x= 0 }; // This expressions produces a value, which is used to initialize reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 5 ) )


def DestroyedVariableStillHasReferences_ForStructInitializationExpression_Test0():
	c_program_text= """
		struct S{ i32 x; }
		fn Foo()
		{
			auto& s= PassS( S{ .x= 0 } ); // Passing a reference to a temporary variable with {} initializer through a function and try to save a reference to this temporary.
		}
		fn PassS(S& s) : S& { return s; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 5 ) )


def DestroyedVariableStillHasReferences_ForStructInitializationExpression_Test1():
	c_program_text= """
		struct S{ i32 x; }
		fn Foo()
		{
			var S stack_s= zero_init;
			select( true ? stack_s : PassS( S{ .x= 0 } ) ); // Passing a reference to a temporary variable with {} initializer through a function and try to save a reference to this temporary.
		}
		fn PassS(S& s) : S& { return s; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 6 ) )


def DestroyedVariableStillHasReferences_ForStructInitializationExpression_Test2():
	c_program_text= """
		struct S{ i32 x; }
		fn Foo() : S&
		{
			return PassS(S{ .x= 0 }); // returning a reference to a temporary, constructed with {}.
		}
		fn PassS(S& s) : S& { return s; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 5 ) )
