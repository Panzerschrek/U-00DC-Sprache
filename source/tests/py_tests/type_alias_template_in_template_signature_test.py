from py_tests_common import *


def TypeAliasTemplateInAnotherTemplateSignature_Test0():
	c_program_text= """
		template</type T/> struct Box { T t; }
		template</type T/> type BoxAlias= Box</T/>;
		template</type T/> struct CheckBox { auto is_box= false; }
		template</type T/> struct CheckBox</ BoxAlias</T/> /> { auto is_box= true; }
		type IntBox= Box</i32/>;
		static_assert( CheckBox</ IntBox />::is_box );
		static_assert( !CheckBox</ i32 />::is_box );
	"""
	tests_lib.build_program( c_program_text )


def TypeAliasTemplateInAnotherTemplateSignature_Test1():
	c_program_text= """
		template</type T/> struct Box { T t; }
		template</type T/> type BoxAlias= Box</T/>;
		template</type T/> fn constexpr IsBox( BoxAlias</T/>& b ) : bool { return true; }
		template</type T/> fn constexpr IsBox( T& t ) : bool { return false; }
		fn Foo()
		{
			var Box</i32/> b= zero_init;
			static_assert( IsBox( b ) );
			static_assert( !IsBox( b.t ) );
		}
	"""
	tests_lib.build_program( c_program_text )
