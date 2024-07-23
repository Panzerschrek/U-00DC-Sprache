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
