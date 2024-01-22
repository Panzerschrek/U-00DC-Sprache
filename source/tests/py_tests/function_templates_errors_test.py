from py_tests_common import *


def FunctionDeclarationOutsideItsScope_ForFunctionTemplates_Test0():
	c_program_text= """
		namespace NS{}
		template</ type T />
		fn NS::Foo(){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "FunctionDeclarationOutsideItsScope", 4 ) )


def ValueIsNotTemplate_ForFunctionTemplates_Test0():
	c_program_text= """
		fn Bar();
		fn Foo()
		{
			Bar</ i32 />();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ValueIsNotTemplate", 5 ) )


def ValueIsNotTemplate_ForFunctionTemplates_Test1():
	c_program_text= """
		struct S
		{
			i32 x;
		}

		fn Foo()
		{
			var S s= zero_init;
			s.x</ 42 />;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ValueIsNotTemplate", 10 ) )


def ValueIsNotTemplate_ForFunctionTemplates_Test2():
	c_program_text= """
		struct S
		{
			fn Bar(){}
		}

		fn Foo()
		{
			var S s;
			s.Bar</ 42 />;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ValueIsNotTemplate", 10 ) )


def ValueIsNotTemplate_ForFunctionTemplates_Test3():
	c_program_text= """
		struct S
		{
			fn Bar(this);
			fn Foo( this )
			{
				Bar</ i32 />();
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ValueIsNotTemplate", 7 ) )


def IncompleteMemberOfClassTemplate_ForFunctionTemplates_Test0():
	c_program_text= """
		template</ type T />
		fn Foo();  // Needs body for function template.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "IncompleteMemberOfClassTemplate" )
	assert( errors_list[0].src_loc.line == 3 )


def Redefinition_ForFunctionTemplateParameter():
	c_program_text= """
		template</ type T, type T />
		fn Foo(){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "Redefinition" )
	assert( errors_list[0].src_loc.line == 2 )


def NameNotFound_ForFunctionTemplateParameter():
	c_program_text= """
		template</ UnknownName param />
		fn Foo(){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "NameNotFound", 2 ) )


def NameIsNotTypeName_ForFunctionTemplateParameter():
	c_program_text= """
		fn Bar(){}
		template</ Bar param />
		fn Foo(){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidTypeOfTemplateVariableArgument", 3 ) or HaveError( errors_list, "InvalidValueAsTemplateArgument", 3 ) )


def InvalidTypeOfTemplateVariableArgument_ForFunctionTemplateParameter():
	c_program_text= """
		struct Bar{}
		template</ Bar param />
		fn Foo(){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeOfTemplateVariableArgument" )
	assert( errors_list[0].src_loc.line == 3 )


def VirtualForFunctionTemplate_Test0():
	c_program_text= """
		template</ type T />
		fn virtual Foo(){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VirtualForFunctionTemplate" )
	assert( errors_list[0].src_loc.line == 3 )


def TemplateParametersDeductionFailed_Test0():
	c_program_text= """
		template</ type SRC, type DST />
		fn NumConvert( SRC src ) : DST
		{
			return DST(src);
		}

		fn Foo()
		{
			var i32 x= 0;
			NumConvert(x); // Error, can not deduce template parameter "DST", using given arguments.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TemplateParametersDeductionFailed", 11 ) )


def TemplateParametersDeductionFailed_Test1():
	c_program_text= """
		template</ type T />
		fn NewValue() : T { return T(); }

		fn Foo()
		{
			NewValue();  // Error, can not deduce template parameter "T", using given arguments.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TemplateParametersDeductionFailed", 7 ) )


def TemplateParametersDeductionFailed_Test2():
	c_program_text= """
		template</ type T />
		fn ProcessMut( T &mut t ) {}

		fn Foo()
		{
			var i32 imut x= 0;
			ProcessMut(x);  // Error, immutable reference given, can not bind it to "&mut" parameter.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" ) # In given case must be this error.
	assert( errors_list[0].src_loc.line == 8 )


def TemplateParametersDeductionFailed_Test3():
	c_program_text= """
		template</ type T />
		fn Max( T& a, T& b ) : T&
		{
			if( a > b ) { return a; }
			return b;
		}

		fn Foo()
		{
			Max( 42, 85.0 ); // Error, different types given - can not deduce.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 11 )


def TemplateParametersDeductionFailed_Test4():
	c_program_text= """
		template</ type T /> struct Box{ T boxed; }

		template</ type T />
		fn UnboxAndAssign( Box</T/>& box, T&mut dst )
		{
			dst= box.boxed;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var Box</ f32 /> box{ .boxed= 0.0f };
			UnboxAndAssign( box, x );  // Error, from first parameter we deduced "T"= f32, but from second "T"= i32.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 14 )


def TemplateParametersDeductionFailed_Test5():
	c_program_text= """
		template</ type T /> struct Box{ T boxed; }

		template</ type T />
		fn Unbox( Box</T/>& box ) : T&
		{
			return box.boxed;
		}

		fn Foo()
		{
			var i32 x= 0;
			Unbox( x ); // Error, Box</T/> expected, but i32 given.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 13 )


def TemplateParametersDeductionFailed_Test6():
	c_program_text= """
		template</ type T />
		fn Vec3Square( [ T, 3u ]& vec ) : T
		{
			return vec[0u] * vec[0u] + vec[1u] * vec[1u] + vec[2u] * vec[2u];
		}

		fn Foo()
		{
			var [ i32, 2 ] vec2= zero_init;
			Vec3Square( vec2 );  // Error, required array with size 3, but array with size 2 given.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 11 )


def TemplateParametersDeductionFailed_Test7():
	c_program_text= """
		template</ u32 size />
		fn IntVecSquare( [ i32, size ]& vec ) : i32
		{
			var i32 mut result= 0;
			var u32 mut i= 0u;
			while( i < size )
			{
				result+= vec[i] * vec[i];
				++i;
			}
			return result;
		}

		fn Foo()
		{
			var [ f32, 2 ] vec2= zero_init;
			IntVecSquare( vec2 );  // Error, required array of "i32", but array of "f32" given.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 18 )


def TemplateParametersDeductionFailed_Test8():
	c_program_text= """
		template</ type T />
		fn Bar( T& t, i32 x ) {}

		fn Foo()
		{
			Bar( false, 0.25f ); // Error, second argument of non-template type does not match given argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 7 )


def TemplateParametersDeductionFailed_Test9():
	c_program_text= """
		template</ type T />
		fn Bar( T& t, i32 x ) {}

		fn Foo()
		{
			Bar( false, 0, 1 ); // Error, argument count mismatch.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 7 )


def TemplateParametersDeductionFailed_Test10():
	c_program_text= """
		template</ type T />
		fn Max( T& a, T& b ) : T&
		{
			if( a > b ) { return a; }
			return b;
		}

		fn Foo()
		{
			Max( 0.0f ); // Error, argument count mismatch.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 11 )


def TemplateParametersDeductionFailed_Test11():
	c_program_text= """
		template</ type T />
		fn Bar( T& a, T& b ){}

		class A polymorph{}
		class B : A{}
		fn Foo()
		{
			Bar( B(), A() ); // Error, T deduced to "B" and later "A" can't be casted to "B".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 9 )


def TemplateFunctionGenerationFailed_Test0():
	c_program_text= """
		template</ type T />
		fn CastIntTo( i32 i ) : T { return T(i); }

		fn Foo()
		{
			CastIntTo</ i32, f64 />; // Error, function generation failed, because parameter count is greater, then needs.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TemplateFunctionGenerationFailed", 7 ) )


def TemplateFunctionGenerationFailed_Test1():
	c_program_text= """
		template</ type T />
		fn CastIntTo( i32 i ) : T { return T(i); }

		fn Foo()
		{
			CastIntTo</ 42 />; // Error, function generation failed, value parameter given, but expected type parameter.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TemplateFunctionGenerationFailed", 7 ) )


def TemplateFunctionGenerationFailed_Test2():
	c_program_text= """
		template</ u32 value />
		fn Set( u32 &mut u ) { u= value; }

		fn Foo()
		{
			var u32 mut x= 0u;
			Set</ 42 />( x ); // Error, function generation failed, given value parameter type does not match expected type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TemplateFunctionGenerationFailed", 8 ) )


def TemplateFunctionGenerationFailed_Test3():
	c_program_text= """
		template</ type T, T value />
		fn Set( T &mut t ) { t= value; }

		fn Foo()
		{
			var u32 mut x= 0u;
			Set</ u32, false />( x ); // Error, function generation failed, given value parameter type does not match expected type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TemplateFunctionGenerationFailed", 8 ) )


def TemplateFunctionGenerationFailed_Test5():
	c_program_text= """
		template</ type T/>
		fn Bar(){}

		fn Foo()
		{
			Bar</ i32, f32 />(); // Error, template parameter count is greater, then expected.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TemplateFunctionGenerationFailed", 7 ) )


def UsingKeywordAsName_ForTemplateFunction_Test0():
	c_program_text= """
		template</type T/> fn public() {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "UsingKeywordAsName", 2 ) )
