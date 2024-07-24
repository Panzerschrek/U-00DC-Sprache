from py_tests_common import *


def TemplateTemplateParamDeclaration_Test0():
	c_program_text= """
		template</ type template T />
		struct S
		{
			T</i32/> x;
		}
	"""
	tests_lib.build_program( c_program_text )


def TemplateTemplateParamDeclaration_Test1():
	c_program_text= """
		template</ type template T />
		fn Foo( T</i32/> arg ){}
	"""
	tests_lib.build_program( c_program_text )


def TemplateTypeTemplateArg_Test0():
	c_program_text= """
		template</type template Container/>
		struct IntBox
		{
			Container</i32/> container;
		}

		template</type T/>
		struct Pair
		{
			T first;
			T second;
		}

		type IntBoxPair = IntBox</Pair/>;

		var IntBoxPair constexpr pair{ .container{ .first = 67, .second = -5 } };
	"""
	tests_lib.build_program( c_program_text )


def TemplateTypeTemplateArg_Test1():
	c_program_text= """
		template</type ContainedT, type template Container/>
		struct ContainerBox
		{
			Container</ContainedT/> container;
		}

		template</type T/>
		struct Pair
		{
			T first;
			T second;
		}

		type FloatBoxPair = ContainerBox</f32, Pair/>;

		var FloatBoxPair constexpr pair{ .container{ .first = 78.0f, .second = -51.6f } };
	"""
	tests_lib.build_program( c_program_text )


def TemplateTypeTemplateArg_Test2():
	c_program_text= """
		template</type T/> class Vec{}

		template</ type template Container, type ContainedT/>
		fn MakeContainer() : Container</ContainedT/>
		{
			return Container</ContainedT/>();
		}

		fn Foo()
		{
			var Vec</bool/> v = MakeContainer</Vec, bool/>();
		}
	"""
	tests_lib.build_program( c_program_text )


def TemplateTypeTemplateArg_Test3():
	c_program_text= """
		template</type T/> struct Box{ T val; }

		template</ type template Container, type ContainedT/>
		fn MakeContainer( ContainedT mut val ) : Container</ContainedT/>
		{
			return Container</ContainedT/>{ .val= move(val) };
		}

		var Box</f32/> v = MakeContainer</Box/>( 1.2345f );
		static_assert( v.val == 1.2345f );
	"""
	tests_lib.build_program( c_program_text )


def TemplateTypeTemplateArg_Test4():
	c_program_text= """
		// Use signature param to split given argument into container and type components.
		template</type template Container, type Element/>
		struct S</ Container</Element/> />
		{
			type IntsContainer= Container</i32/>;
			type ElementType= Element;
		}

		template</ type T /> struct Box{ T x; }

		type FloatBox= Box</f32/>;

		type IntsBox= S</ FloatBox />::IntsContainer;

		static_assert( same_type</ IntsBox, Box</i32/> /> );
		static_assert( same_type</ S</ FloatBox />::ElementType, f32 /> );
	"""
	tests_lib.build_program( c_program_text )


def TemplateTypeTemplateArg_Test5():
	c_program_text= """
		template</ type template Container, type PrevType, type NewType />
		type ReplaceContainedType</ Container</ PrevType />, NewType /> = Container</ NewType />;

		template</ type T /> struct Box{ T x; }

		type FloatBox= Box</f32/>;

		type BigIntsBox= ReplaceContainedType</ FloatBox, u64 />;

		static_assert( same_type</ BigIntsBox, Box</u64/> /> );
	"""
	tests_lib.build_program( c_program_text )


def TemplateTypeTemplateArg_Test6():
	c_program_text= """
		template</ type template T />
		struct Some
		{
			type SelfT= T</Some/>;
		}

		// Some kind of masturbation - use template itself as argument of this template.
		type SomeSome= Some</ Some />;
		static_assert( same_type</ SomeSome, SomeSome::SelfT /> );
	"""
	tests_lib.build_program( c_program_text )


def TemplateTypeTemplateArg_Test7():
	c_program_text= """
		// Recursive type alias should be an error.
		template</ type template T />
		type Some= T</Some/>;

		type SomeSome= Some</ Some />;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	# TODO - create special error for this.


def TemplateParamOverloading_Test0():
	c_program_text= """
		template<//> struct S{}
		template<//> struct T{}

		template</ type template A /> struct SChecker{ auto is_s = false; }
		template</ /> struct SChecker</ S /> { auto is_s = true; }

		static_assert(  SChecker</S/>::is_s );
		static_assert( !SChecker</T/>::is_s );
	"""
	tests_lib.build_program( c_program_text )


def TemplateParamOverloading_Test1():
	c_program_text= """
		template</type X/> struct S{ X val; }
		template</type X/> struct T{ X val; }

		template</ type X /> struct SChecker</ S</X/> /> { auto is_s = true; }
		template</ type template A, type X /> struct SChecker</ A</X/> />{ auto is_s = false; }

		static_assert( !SChecker</ T</f64/> />::is_s );
		static_assert(  SChecker</ S</f64/> />::is_s );
	"""
	tests_lib.build_program( c_program_text )


def TemplateParamOverloading_Test2():
	c_program_text= """
		template</type X/> struct S{ X val; }
		template</type X/> struct T{ X val; }
		template</type X/> struct U{ X val; }

		// Overloading for S-templates
		template</type X/> fn Foo( S</X/> arg ) : i32 { return 1; }
		// Overloading for T-templates
		template</type X/> fn Foo( T</X/> arg ) : i32 { return 2; }
		// Overloading for other templates
		template</type template R, type X/> fn Foo( R</X/> arg ) : i32 { return 3; }
		// Overloading for other types
		template</type X/> fn Foo( X arg ) : i32 { return 4; }

		var S</f32/> s= zero_init;
		var T</i32/> t= zero_init;
		var U</u32/> u= zero_init;
		var u64 z= zero_init;

		static_assert( Foo(s) == 1 ); // Should select overloading for "S" template.
		static_assert( Foo(t) == 2 ); // Should select overloading for "T" template.
		static_assert( Foo(u) == 3 ); // Should select overloading for other templates.
		static_assert( Foo(z) == 4 ); // Should select overloading for non-templates.
	"""
	tests_lib.build_program( c_program_text )


def TemplateParamOverloading_Test3():
	c_program_text= """
		// Function template with type param.
		template</type T/> fn Make() : T
		{
			var T t{ .t= 1 };
			return t;
		}

		// Function template with type template param.
		template</type template T/> fn Make() : T</i32/>
		{
			var T</i32/> t{ .t= 2 };
			return t;
		}

		template</type T/> struct S{ T t; }

		// Select overloaded function with type param.
		static_assert( Make</ S</i32/> />().t == 1 );
		// Select overloaded function with type template param.
		static_assert( Make</ S />().t == 2 );
	"""
	tests_lib.build_program( c_program_text )


def TemplateParamOverloading_Test4():
	c_program_text= """
		template</ type A /> struct S{}
		template</ type A, type B /> struct S{}

		template</ type A />
		struct TemplateUnwrapper</ S</A/> /> {}

		template</ type A, type template T />
		struct TemplateUnwrapper</ T</A/> /> {}

		type SFloat= S</f32/>;

		// Error here - it's for now impossible to compare set of two or more templates against single type template param.
		type Unwrapper= TemplateUnwrapper</ SFloat />;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CouldNotSelectMoreSpicializedTypeTemplate", 14 ) )


def TemplateParamOverloading_Test5():
	c_program_text= """
		template</ type A /> struct S{}

		template</ type A />
		struct TemplateUnwrapper</ S</A/> /> { auto order= 1; }

		template</ type A, type template T />
		struct TemplateUnwrapper</ T</A/> /> { auto order = 2; }

		type SFloat= S</f32/>;

		// Fine - compare type template param against specific type template.
		static_assert( TemplateUnwrapper</ SFloat />::order == 1 );
	"""
	tests_lib.build_program( c_program_text )


def TemplateParamOverloading_Test6():
	c_program_text= """
		template</ type T /> struct Box{ T t; }

		template</ type T />
		struct S</ Box</T/> />{}

		template</ type template T />
		struct S</ T</ i32 /> /> {}

		// Error while selecting best specialized template.
		// The first alternative is better, because type template is more specialized.
		// The econd alternative is better, because type template argument is more specialized.
		type SIntBox= S</ Box</ i32 /> />;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CouldNotSelectMoreSpicializedTypeTemplate", 13 ) )


def TemplateParamOverloading_Test7():
	c_program_text= """
		template</ type T /> struct Box{ T t; }

		template</ type T />
		fn Bar( Box</T/> arg ) {}

		template</ type template T />
		fn Bar( T</ i32 /> arg ) {}

		fn Foo()
		{
			var Box</i32/> box= zero_init;
			// Error while selecting best specialized template.
			// The first alternative is better, because type template is more specialized.
			// The econd alternative is better, because type template argument is more specialized.
			Bar(box);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TooManySuitableOverloadedFunctions", 16 ) )


def MoreThanOneTypeTemplateAsTemplateArgument_Test0():
	c_program_text= """
		template</ type template A />
		struct S{}

		template</ type A /> struct Tup{}
		template</ type A, type B /> struct Tup{}

		type STup= S</ Tup />;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MoreThanOneTypeTemplateAsTemplateArgument", 8 ) )


def MoreThanOneTypeTemplateAsTemplateArgument_Test1():
	c_program_text= """
		template</ type A /> struct Tup{}
		template</ type A, type B /> struct Tup{}

		template<//>
		struct S</ Tup />
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MoreThanOneTypeTemplateAsTemplateArgument", 6 ) )
