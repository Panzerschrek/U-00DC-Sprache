from py_tests_common import *

def NonSyncTagDeclaration_Test0():
	c_program_text= """
		// "non_sync" as simple tag (equivalent to non_sync(true)).
		struct A non_sync {}
		class B non_sync {}
		class C non_sync ordered {}
		class D interface non_sync {}
		class E : D non_sync {}
	"""
	tests_lib.build_program( c_program_text )


def NonSyncTagDeclaration_Test1():
	c_program_text= """
		// "non_sync" as tag with condition expression.
		class A non_sync(true) {}
		class B non_sync(false) {}
		class C non_sync( 2 + 2 == 4 ) {}

		auto constexpr c= 66;
		class D non_sync( c / 2 != 13 ) {}
		class E non_sync( 12 == c ) {}
	"""
	tests_lib.build_program( c_program_text )


def NonSyncExpressionDeclaration_Test0():
	c_program_text= """
		var bool a = non_sync</i32/>;
		struct S{}
		var bool s = non_sync</S/>;
		class C non_sync {}
		var bool c = non_sync</C/>;
		var bool a4 = non_sync</[f32, 4]/>;
		var bool fb = non_sync</tup[f32, bool]/>;
	"""
	tests_lib.build_program( c_program_text )


def NonSyncExporession_Test0():
	c_program_text= """
		// Fundamental types, raw pointers, function pointers, enums have no "non_sync" tag.
		static_assert( !non_sync</i32/> );
		static_assert( !non_sync</bool/> );
		static_assert( !non_sync</f32/> );
		static_assert( !non_sync</ [i32, 3] /> );
		static_assert( !non_sync</ tup[ bool, tup[], [f32, 4] ] /> );
		static_assert( !non_sync</ $(u64) /> );
		static_assert( !non_sync</ fn(i32 a, f32 b) /> );

		enum E{ A, B, C }
		static_assert( !non_sync</ E /> );
	"""
	tests_lib.build_program( c_program_text )


def NonSyncExporession_Test1():
	c_program_text= """
		struct A{ bool b; f32 f; f64 d; char16 c; }
		static_assert( !non_sync</A/> );
		static_assert( !non_sync</ [A, 7] /> );

		struct B non_sync{}
		static_assert( non_sync</B/> );
		static_assert( non_sync</ tup[bool, B] /> );
		static_assert( non_sync</ tup[C, B] /> );
		static_assert( non_sync</ tup[B, C] /> );

		struct C { i32 i; B b; }
		static_assert( non_sync</C/> );

		struct D{ [C, 2] c2; f32 f; }
		static_assert( non_sync</D/> );

		class E interface non_sync {}
		static_assert( non_sync</E/> );

		class F : E {}
		static_assert( non_sync</F/> );

		class G non_sync(true) {}
		static_assert( non_sync</G/> );

		class H non_sync(false) {}
		static_assert( !non_sync</H/> );

		class I non_sync( non_sync</A/> ) {}
		static_assert( !non_sync</I/> );

		class J non_sync( non_sync</B/> ) {}
		static_assert( non_sync</J/> );

		class K polymorph non_sync {}
		class L : K non_sync(false) {}
		static_assert( non_sync</L/> );

		class M non_sync(false) { L l; }
		static_assert( non_sync</M/> );
	"""
	tests_lib.build_program( c_program_text )


def NonSyncExporession_Test2():
	c_program_text= """
		// Use recursive non_sync tag calculation.

		class A non_sync( non_sync</A/> ) {}
		static_assert( !non_sync</A/> );

		class B non_sync( non_sync</C/> ) {}
		class C{ B b; }
		static_assert( !non_sync</B/> );
		static_assert( !non_sync</C/> );

		class D non_sync( non_sync</E/> ) {}
		class E non_sync { D d; }
		static_assert( non_sync</D/> );
		static_assert( non_sync</E/> );

		class F non_sync( non_sync</G/> ) {}
		class G non_sync( non_sync</F/> ) {}
		static_assert( !non_sync</F/> );
		static_assert( !non_sync</G/> );

		class H non_sync( non_sync</I/> ) {}
		class I non_sync( non_sync</H/> ) { J j; }
		class J non_sync {}
		static_assert( non_sync</H/> );
		static_assert( non_sync</I/> );

		class K non_sync( non_sync</L/> ) {}
		class L non_sync( non_sync</M/> ) {}
		class M non_sync( non_sync</K/> ) {}
		static_assert( !non_sync</K/> );
		static_assert( !non_sync</L/> );
		static_assert( !non_sync</M/> );
	"""
	tests_lib.build_program( c_program_text )
