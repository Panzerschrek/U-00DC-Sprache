from py_tests_common import *


def SharedState_Test0():
	c_program_text= """
		static_assert( typeinfo</ i32   />.have_shared_state == false );
		static_assert( typeinfo</ bool  />.have_shared_state == false );
		static_assert( typeinfo</ f64   />.have_shared_state == false );
		static_assert( typeinfo</ char8 />.have_shared_state == false );
	"""
	tests_lib.build_program( c_program_text )


def SharedState_Test1():
	c_program_text= """
		static_assert( typeinfo</ [ i32  ,  5 ] />.have_shared_state == false );
		static_assert( typeinfo</ [ bool ,  1 ] />.have_shared_state == false );
		static_assert( typeinfo</ [ f64  , 52 ] />.have_shared_state == false );
		static_assert( typeinfo</ [ char8, 11 ] />.have_shared_state == false );
	"""
	tests_lib.build_program( c_program_text )


def SharedState_Test2():
	c_program_text= """
		enum E{ A, B, C, }
		struct A {}
		struct B { i32 x; }
		struct C { B b; }
		class D{ [ f32, 8 ] arr; }
		static_assert( typeinfo</ fn() />.have_shared_state == false );
		static_assert( typeinfo</ E />.have_shared_state == false );
		static_assert( typeinfo</ A />.have_shared_state == false );
		static_assert( typeinfo</ B />.have_shared_state == false );
		static_assert( typeinfo</ C />.have_shared_state == false );
		static_assert( typeinfo</ [ B, 2 ] />.have_shared_state == false );
		static_assert( typeinfo</ D />.have_shared_state == false );
	"""
	tests_lib.build_program( c_program_text )


def SharedState_Test3():
	c_program_text= """
		struct A shared {}
		struct B { A a; }
		struct C { B b; }
		struct D { A & a; }
		struct E { B & b; }
		struct F{ [ C, 2 ] c_arr; }
		struct G{ [ A, 0 ] a_arr; }
		class X polymorph shared {}
		class Y final : X {}
		class Z interface shared {}
		class W : Z {}
		static_assert( typeinfo</ A />.have_shared_state );
		static_assert( typeinfo</ B />.have_shared_state );
		static_assert( typeinfo</ C />.have_shared_state );
		static_assert( typeinfo</ D />.have_shared_state );
		static_assert( typeinfo</ E />.have_shared_state );
		static_assert( typeinfo</ F />.have_shared_state );
		static_assert( typeinfo</ G />.have_shared_state );
		static_assert( typeinfo</ [ G, 8 ] />.have_shared_state );
		static_assert( typeinfo</ [ G, 0 ] />.have_shared_state );
		static_assert( typeinfo</ X />.have_shared_state );
		static_assert( typeinfo</ Y />.have_shared_state );
		static_assert( typeinfo</ Z />.have_shared_state );
		static_assert( typeinfo</ W />.have_shared_state );
	"""
	tests_lib.build_program( c_program_text )
