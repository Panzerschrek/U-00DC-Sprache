from py_tests_common import *

def SharedTagDeclaration_Test0():
	c_program_text= """
		// "shared" as simple tag (equivalent to shared(true)).
		struct A shared {}
		class B shared {}
		class C shared ordered {}
		class D interface shared {}
		class E : D shared {}
	"""
	tests_lib.build_program( c_program_text )


def SharedTagDeclaration_Test1():
	c_program_text= """
		// "shared" as tag with condition expression.
		class A shared(true) {}
		class B shared(false) {}
		class C shared( 2 + 2 == 4 ) {}

		auto constexpr c= 66;
		class D shared( c / 2 != 13 ) {}
		class E shared( 12 == c ) {}
	"""
	tests_lib.build_program( c_program_text )


def SharedExpressionDeclaration_Test0():
	c_program_text= """
		var bool a = shared</i32/>;
		struct S{}
		var bool s = shared</S/>;
		class C shared {}
		var bool c = shared</C/>;
		var bool a4 = shared</[f32, 4]/>;
		var bool fb = shared</tup[f32, bool]/>;
	"""
	tests_lib.build_program( c_program_text )


def SharedExporession_Test0():
	c_program_text= """
		// Fundamental types, raw pointers, function pointers, enums have no "shared" tag.
		static_assert( !shared</i32/> );
		static_assert( !shared</bool/> );
		static_assert( !shared</f32/> );
		static_assert( !shared</ [i32, 3] /> );
		static_assert( !shared</ tup[ bool, tup[], [f32, 4] ] /> );
		static_assert( !shared</ $(u64) /> );
		static_assert( !shared</ fn(i32 a, f32 b) /> );

		enum E{ A, B, C }
		static_assert( !shared</ E /> );
	"""
	tests_lib.build_program( c_program_text )


def SharedExporession_Test1():
	c_program_text= """
		struct A{ bool b; f32 f; f64 d; char16 c; }
		static_assert( !shared</A/> );
		static_assert( !shared</ [A, 7] /> );

		struct B shared{}
		static_assert( shared</B/> );
		static_assert( shared</ tup[bool, B] /> );
		static_assert( shared</ tup[C, B] /> );
		static_assert( shared</ tup[B, C] /> );

		struct C { i32 i; B b; }
		static_assert( shared</C/> );

		struct D{ [C, 2] c2; f32 f; }
		static_assert( shared</D/> );

		class E interface shared {}
		static_assert( shared</E/> );

		class F : E {}
		static_assert( shared</F/> );

		class G shared(true) {}
		static_assert( shared</G/> );

		class H shared(false) {}
		static_assert( !shared</H/> );

		class I shared( shared</A/> ) {}
		static_assert( !shared</I/> );

		class J shared( shared</B/> ) {}
		static_assert( shared</J/> );

	"""
	tests_lib.build_program( c_program_text )
