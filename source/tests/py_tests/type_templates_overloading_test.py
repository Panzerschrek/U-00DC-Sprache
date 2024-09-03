from py_tests_common import *


def TypeTemplatesOvelroading_MustSelectSpecializedTemplate_Test0():
	c_program_text= """
		template</ /> struct S</ i32 />
		{
			auto constexpr x= 555;
		}

		template</ /> struct S</ f32 />
		{
			auto constexpr x= 999;
		}

		static_assert( S</i32/>::x == 555 );
		static_assert( S</f32/>::x == 999 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_MustSelectSpecializedTemplate_Test1():
	c_program_text= """
		template</ /> struct S</ i32 />
		{
			auto constexpr x= 555;
		}

		template</ /> struct S</ f32 />
		{
			auto constexpr x= 999;
		}

		template</ type T />
		struct F</ S</ T /> />  // Must select here one of overloaded type templates.
		{
			type TT= T;
		}

		fn Foo( F</ S</ i32 /> />::TT arg ) : i32 { return arg; }
		fn Foo( F</ S</ f32 /> />::TT arg ) : f32 { return arg; }
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test0():
	c_program_text= """
		template</ type T />
		struct S</ T />
		{
			auto constexpr x= 555;
		}

		template</ type T, type SizeType, SizeType s />
		struct S</ [ T, s ] />  // Array is more specialized.
		{
			auto constexpr x= 999;
		}

		static_assert( S</ [ i32, 4 ] />::x == 999 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test1():
	c_program_text= """
		template</ type T /> struct Box{}

		template</ type T />
		struct S</ Box</ T /> />  // template is more specialized.
		{
			auto constexpr x= 22258;
		}

		template</ type T />
		struct S</ T />
		{
			auto constexpr x= 88852;
		}

		static_assert( S</ Box</ f32 /> />::x == 22258 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test2():
	c_program_text= """
		template</  />
		struct S</ bool />  // concrete type is more specialized.
		{
			auto constexpr x= 44456;
		}

		template</ type T />
		struct S</ T />
		{
			auto constexpr x= 66654;
		}

		static_assert( S</ bool />::x == 44456 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test3():
	c_program_text= """
		template</ type T />
		struct S</ T />
		{
			auto constexpr x= 11111;
		}

		template</ type T />
		struct S</ fn() : T />  // function type is more specialized.
		{
			auto constexpr x= 55555;
		}

		static_assert( S</ fn() : i16 />::x == 55555 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test4():
	c_program_text= """
		template</ type T />
		struct S</ bool, T= i32 />  // concrete type is more specialized.
		{
			auto constexpr x= 88888;
		}

		template</ type T />
		struct S</ T />
		{
			auto constexpr x= 11;
		}

		static_assert( S</ bool />::x == 88888 );   // Should select type with default second argument.
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test5():
	c_program_text= """
		template</ type T />
		struct S</ bool, T= i32 />  // concrete type is more specialized.
		{
			auto constexpr x= 555;
		}

		template</ type T, type F />
		struct S</ T, F />
		{
			auto constexpr x= 999;
		}

		static_assert( S</ bool, i32 />::x == 555 );   // Should select type with default second argument.
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test6():
	c_program_text= """
		template</type A, type B/>
		struct Some</A, B= A/>{}

		// Skip default signature parameter of type template in template signature.
		template</type A/>
		struct Unwrapper</ Some</A/> />
		{
			auto val= 333;
		}

		template</type X/>
		struct Unwrapper</X/>
		{
			auto val= 777;
		}

		type I32Some= Some</i32/>;
		// Should select specialization for "Some".
		static_assert( Unwrapper</ I32Some />::val == 333 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test7():
	c_program_text= """
		template</type A, type B/>
		struct Some</A, B= A/>{}

		// Skip default signature parameter of type template in template signature.
		template</type A/>
		struct Unwrapper</ Some</A/> />
		{
			auto val= 333;
		}

		template</type X/>
		struct Unwrapper</X/>
		{
			auto val= 777;
		}

		type I32Some= Some</f64, f64/>;
		// Should select specialization for "Some", since second optional signature arguemnt of "Some" is equal to its default value.
		static_assert( Unwrapper</ I32Some />::val == 333 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test8():
	c_program_text= """
		template</type A, type B/>
		struct Some</A, B= A/>{}

		// Skip default signature parameter of type template in template signature.
		template</type A/>
		struct Unwrapper</ Some</A/> />
		{
			auto val= 333;
		}

		template</type X/>
		struct Unwrapper</X/>
		{
			auto val= 777;
		}

		type I32Some= Some</i32, f32/>;
		// Should NOT select specialization for "Some", since second optional signature arguemnt of "Some" is different from default.
		static_assert( Unwrapper</ I32Some />::val == 777 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test9():
	c_program_text= """
		template</type A, type B/>
		struct Some</A, B= i32/>{}

		// Skip default signature parameter of type template in template signature.
		template</type A/>
		struct Unwrapper</ Some</A/> />
		{
			auto val= 333;
		}

		template</type X/>
		struct Unwrapper</X/>
		{
			auto val= 777;
		}

		type I32Some= Some</i32/>;
		// Should select specialization for "Some".
		static_assert( Unwrapper</ I32Some />::val == 333 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test10():
	c_program_text= """
		template</type A, type B/>
		struct Some</A, B= i32/>{}

		// Skip default signature parameter of type template in template signature.
		template</type A/>
		struct Unwrapper</ Some</A/> />
		{
			auto val= 333;
		}

		template</type X/>
		struct Unwrapper</X/>
		{
			auto val= 777;
		}

		type I32Some= Some</f64, i32/>;
		// Should select specialization for "Some", since second optional signature arguemnt of "Some" is equal to its default value.
		static_assert( Unwrapper</ I32Some />::val == 333 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_Specialization_Test11():
	c_program_text= """
		template</type A, type B/>
		struct Some</A, B= i32/>{}

		// Skip default signature parameter of type template in template signature.
		template</type A/>
		struct Unwrapper</ Some</A/> />
		{
			auto val= 333;
		}

		template</type X/>
		struct Unwrapper</X/>
		{
			auto val= 777;
		}

		type I32Some= Some</i32, f32/>;
		// Should NOT select specialization for "Some", since second optional signature arguemnt of "Some" is different from default.
		static_assert( Unwrapper</ I32Some />::val == 777 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplateOverloadingByTemplateParamKind_Test0():
	c_program_text= """
		// Fine - overload type template by param kind (type vs variable vs type template).

		template</ type T /> struct S
		{
			auto order = 10;
		}

		template</ size_type x /> struct S
		{
			auto order= 20;
		}

		template</ type template T /> struct S
		{
			auto order= 30;
		}

		template</type T/> struct Box{ T t; }
		template</type T/> struct Wrapper{ T t; }

		// Select specialization for arguments-types.
		static_assert( S</ i32 />::order == 10 );
		static_assert( S</ [ f64, 4 ] />::order == 10 );
		static_assert( S</ tup[ bool, char8 ] />::order == 10 );

		// Select specialization for arguments-variables.
		static_assert( S</ 0s />::order == 20 );
		static_assert( S</ 12345s />::order == 20 );

		// Select specialization for type templates.
		static_assert( S</ Box />::order == 30 );
		static_assert( S</ Wrapper />::order == 30 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplateOverloadingByTemplateParamKind_Test1():
	c_program_text= """
		// Fine - overload type template by param kind (type and variable vs variable and type).

		template</ type T, size_type x /> struct S
		{
			auto order = 10;
		}

		template</ size_type x, type T /> struct S
		{
			auto order= 20;
		}


		// Select specialization for type and then value.
		static_assert( S</ i32, 67s />::order == 10 );
		static_assert( S</ [ f64, 4 ], 33s />::order == 10 );
		static_assert( S</ tup[ bool, char8 ], 0s />::order == 10 );

		// Select specialization for value and then type.
		static_assert( S</ 0s, bool />::order == 20 );
		static_assert( S</ 12345s, tup[] />::order == 20 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplateOverloadingByTemplateParamKind_Test2():
	c_program_text= """
		// Fine - overload type template by param kind (type vs variable vs type template). Add also specializations for each kind.

		template</ type T /> struct S
		{
			auto order = 10;
		}

		template<//> struct S</ i32 /> // Specialization for "i32" types.
		{
			auto order = 11;
		}

		template</ size_type x /> struct S
		{
			auto order= 20;
		}

		template<//> struct S</ 33s /> // Specialization for "33s" value.
		{
			auto order= 21;
		}

		template</ type template T /> struct S
		{
			auto order= 30;
		}

		template<//> struct S</ Box /> // Specialization for "Box" type template.
		{
			auto order= 31;
		}

		template</type T/> struct Box{ T t; }
		template</type T/> struct Wrapper{ T t; }

		// Select specialization for arguments-types.
		static_assert( S</ i32 />::order == 11 ); // Specialization for i32 type.
		static_assert( S</ [ f64, 4 ] />::order == 10 );
		static_assert( S</ tup[ bool, char8 ] />::order == 10 );

		// Select specialization for arguments-variables.
		static_assert( S</ 0s />::order == 20 );
		static_assert( S</ 33s />::order == 21 ); // Specialization for value 33s.
		static_assert( S</ 12345s />::order == 20 );

		// Select specialization for type templates.
		static_assert( S</ Box />::order == 31 ); // Specialization for "Box" type template.
		static_assert( S</ Wrapper />::order == 30 );
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplatesOvelroading_SpecializationErrors_Test0():
	c_program_text= """
		template</ type T /> struct S</ i32, T /> {}
		template</ type T /> struct S</ T, i32 /> {}

		fn Foo( S</ i32, i32 /> & s );  // Error, different best-specialized templates for first and second template arguments.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "CouldNotSelectMoreSpicializedTypeTemplate", 5 ) )


def LessSpecializedTemplateTypesNotGenerated_Test0():
	c_program_text= """
		template</ type T />
		struct IsArrayType
		{
			auto constexpr value= false;
		}

		template</ type T, type SizeType, SizeType s />
		struct IsArrayType</ [ T, s ] />
		{
			auto constexpr value= true;
		}

		template</ type T />
		struct S</ T />
		{
			static_assert( ! IsArrayType</ T />::value );
		}

		template</ type T, type SizeType, SizeType s />
		struct S</ [ T, s ] />
		{}

		fn Foo( S</ [ i32, 4 ] />& s );  // Must here select 'S', specialized for arrays and not generate body of less specialized 'S'.
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplateRedefinition_Test0():
	c_program_text= """
		// Simple redefinition.
		template</ type T /> struct S{ T x; }
		template</ type T /> struct S{ T x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 3 ) or HasError( errors_list, "TypeTemplateRedefinition", 4 ) )


def TypeTemplateRedefinition_Test1():
	c_program_text= """
		// Redefinition with different template param name.
		template</ type U /> struct S{ U x; }
		template</ type V /> struct S{ V x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 3 ) or HasError( errors_list, "TypeTemplateRedefinition", 4 ) )


def TypeTemplateRedefinition_Test2():
	c_program_text= """
		// Redefinition with long form.
		template</ type U /> struct S</U/>{ U x; }
		template</ type V /> struct S</V/>{ V x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 3 ) or HasError( errors_list, "TypeTemplateRedefinition", 4 ) )


def TypeTemplateRedefinition_Test3():
	c_program_text= """
		// Redefinition with long and short forms.
		template</ type U /> struct S</U/>{ U x; }
		template</ type V /> struct S{ V x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 3 ) or HasError( errors_list, "TypeTemplateRedefinition", 4 ) )


def TypeTemplateRedefinition_Test4():
	c_program_text= """
		// Redefinition with array signature param.
		template</ type U /> struct S</ [ U, 4 ] />{ U x; }
		template</ type V /> struct S</ [ V, 4 ] />{ V x; }

	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 3 ) or HasError( errors_list, "TypeTemplateRedefinition", 4 ) )


def TypeTemplateRedefinition_Test5():
	c_program_text= """
		// Redefinition with array signature param with different size - this is not error.
		template</ type U /> struct S</ [ U, 3 ] />{ T x; }
		template</ type V /> struct S</ [ V, 4 ] />{ T x; }
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplateRedefinition_Test6():
	c_program_text= """
		// Redefinition with tuple signature param.
		template</ type U, type V /> struct S</ tup[ U, V ] />{ U x; }
		template</ type X, type Y /> struct S</ tup[ X, Y ] />{ X x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 3 ) or HasError( errors_list, "TypeTemplateRedefinition", 4 ) )


def TypeTemplateRedefinition_Test7():
	c_program_text= """
		// Tuple signature param ok - different order.
		template</ type U, type V /> struct S</ tup[ U, V ] />{ U x; }
		template</ type X, type Y /> struct S</ tup[ Y, X ] />{ X x; }
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplateRedefinition_Test8():
	c_program_text= """
		// Typle function param.
		template</ type U, type V /> struct S</ fn(U u) : V />{ U x; }
		template</ type X, type Y /> struct S</ fn(X x) : Y />{ X x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 3 ) or HasError( errors_list, "TypeTemplateRedefinition", 4 ) )


def TypeTemplateRedefinition_Test9():
	c_program_text= """
		// Typle function param - ok, different unsafe flag.
		template</ type U, type V /> struct S</ fn(U u) unsafe : V />{ U x; }
		template</ type X, type Y /> struct S</ fn(X x) : Y />{ X x; }
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplateRedefinition_Test10():
	c_program_text= """
		// Typle function param - ok, different reference flag.
		template</ type U, type V /> struct S</ fn(U u) : V />{ U x; }
		template</ type X, type Y /> struct S</ fn(X& x) : Y />{ X x; }
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplateRedefinition_Test11():
	c_program_text= """
		// Typle function param - ok, different parameters count.
		template</ type U, type V /> struct S</ fn(U u, V v) : f32 />{ U x; }
		template</ type X, type Y /> struct S</ fn(X x0, Y y, X x1) : f32 />{ X x; }
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplateRedefinition_Test12():
	c_program_text= """
		type Int= i32;
		// Redefinition with same type (via alias) in signature.
		template</ type U /> struct S</ U, i32 />{ U x; }
		template</ type V /> struct S</ V, Int />{ V x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 4 ) or HasError( errors_list, "TypeTemplateRedefinition", 5 ) )


def TypeTemplateRedefinition_Test13():
	c_program_text= """
		template</ type T /> struct Box{ T x; }
		// Redefinition with same specialization for template.
		template</ type U /> struct S</ Box</U/> />{ U x; }
		template</ type V /> struct S</ Box</V/> />{ V x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 4 ) or HasError( errors_list, "TypeTemplateRedefinition", 5 ) )


def TypeTemplateRedefinition_Test14():
	c_program_text= """
		template</ type T /> struct Box{ T x; }
		namespace N
		{
			template</ type T /> struct Box{ T x; }
			// Ok - specialization for different templates from different namespaces with same name.
			template</ type U /> struct S</ ::Box</U/> />{ U x; }
			template</ type V /> struct S</ Box</V/> />{ V x; }
		}
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplateRedefinition_Test15():
	c_program_text= """
		// Ok - different type signature param.
		template</ type U /> struct S</ U, f32 />{ U t; }
		template</ type V /> struct S</ V, i32 />{ V t; }
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplateRedefinition_Test16():
	c_program_text= """
		// Redefinition - even if default signature params are different.
		template</ type U, type V /> struct S</ U, V= f32 />{}
		template</ type X, type Y /> struct S</ X, Y= u64 />{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 3 ) or HasError( errors_list, "TypeTemplateRedefinition", 4 ) )


def TypeTemplateRedefinition_Test17():
	c_program_text= """
		// Redefinition - same variable signature param.
		template<//> struct S</ 66u />{}
		template<//> struct S</ 66u />{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 3 ) or HasError( errors_list, "TypeTemplateRedefinition", 4 ) )


def TypeTemplateRedefinition_Test18():
	c_program_text= """
		// Ok - signature params of different types.
		template<//> struct S</ 71u32 />{}
		template<//> struct S</ 71i32 />{}
	"""
	tests_lib.build_program( c_program_text )


def TypeTemplateRedefinition_Test19():
	c_program_text= """
		// Redefinition for type alias templates.
		template</type U/> type S= [ U, 2 ];
		template</type V/> type S= [ V, 4 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 3 ) or HasError( errors_list, "TypeTemplateRedefinition", 4 ) )


def TypeTemplateRedefinition_Test20():
	c_program_text= """
		// Redefinition for same signature for type alias and struct template.
		template</type T/> type S= [ T, 4 ];
		template</type T/> struct S{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypeTemplateRedefinition", 3 ) or HasError( errors_list, "TypeTemplateRedefinition", 4 ) )
