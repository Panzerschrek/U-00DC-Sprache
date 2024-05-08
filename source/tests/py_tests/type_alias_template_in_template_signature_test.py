from py_tests_common import *


def TypeAliasTemplateInAnotherTemplateSignature_Test0():
	c_program_text= """
		template</type T/> struct Box { T t; }
		template</type T/> type BoxAlias= Box</T/>;
		// Type alias as struct template signature param.
		template</type T/> struct CheckBox</ BoxAlias</T/> /> { auto is_box= true; }
		template</type T/> struct CheckBox { auto is_box= false; }
		type IntBox= Box</i32/>;
		static_assert(  CheckBox</ IntBox />::is_box );
		static_assert( !CheckBox</ i32 />::is_box );
	"""
	tests_lib.build_program( c_program_text )


def TypeAliasTemplateInAnotherTemplateSignature_Test1():
	c_program_text= """
		template</type T/> struct Box { T t; }
		template</type T/> type BoxAlias= Box</T/>;
		// Type alias as template function param.
		template</type T/> fn constexpr IsBox( BoxAlias</T/>& b ) : bool { return true; }
		template</type T/> fn constexpr IsBox( T& t ) : bool { return false; }
		fn Foo()
		{
			var Box</i32/> b= zero_init;
			static_assert(  IsBox( b ) );
			static_assert( !IsBox( b.t ) );
		}
	"""
	tests_lib.build_program( c_program_text )


def TypeAliasTemplateInAnotherTemplateSignature_Test2():
	c_program_text= """
		template</type A, type B/> struct MyTup{ A a; B b; }
		template</type A, type B/> type MyTupAlias= MyTup</A, B/>;
		template</type X, type Y/>
		struct MyTupUnwrapper</ MyTupAlias</Y, X/> /> // Reverse type alias template args in mapping.
		{
			type A= Y;
			type B= X;
		}
		type FloatIntTup= MyTup</f32, i32/>;
		static_assert( same_type</ MyTupUnwrapper</ FloatIntTup />::A, f32 /> );
		static_assert( same_type</ MyTupUnwrapper</ FloatIntTup />::B, i32 /> );
	"""
	tests_lib.build_program( c_program_text )


def TypeAliasTemplateInAnotherTemplateSignature_Test3():
	c_program_text= """
		template</type A, type B/> struct MyTup{ A a; B b; }
		template</type A, type B/> type MyTupAlias= MyTup</A, B/>;
		template</type X/>
		struct MyHomogeneousTupUnwrapper</ MyTupAlias</X, X/> />
		{
			type A= X;
			type B= X;
		}
		type LongIntPair= MyTup</u64, u64/>;
		static_assert( same_type</ MyHomogeneousTupUnwrapper</ LongIntPair />::A, u64 /> );
		static_assert( same_type</ MyHomogeneousTupUnwrapper</ LongIntPair />::B, u64 /> );
	"""
	tests_lib.build_program( c_program_text )


def TypeAliasTemplateInAnotherTemplateSignature_Test4():
	c_program_text= """
		template</type A, type B/> struct MyTup{ A a; B b; }
		template</type A/> type TupWithFirstInt= MyTup</i32, A/>;
		template</type X/>
		struct TupWithFirstIntUnwrapper</ TupWithFirstInt</X/> />
		{
			type A= i32;
			type B= X;
		}
		type IntCharTup= MyTup</i32, char8/>;
		static_assert( same_type</ TupWithFirstIntUnwrapper</ IntCharTup />::A, i32 /> );
		static_assert( same_type</ TupWithFirstIntUnwrapper</ IntCharTup />::B, char8 /> );
	"""
	tests_lib.build_program( c_program_text )


def TypeAliasTemplateInAnotherTemplateSignature_Test5():
	c_program_text= """
		template</type A, type B/> struct MyTup{ A a; B b; }
		template</type A/> type TupWithSecondInt= MyTup</A, i32/>;
		template</type X/>
		struct TupWithFirstIntUnwrapper</ TupWithSecondInt</X/> />
		{
			type A= X;
			type B= i32;
		}
		type FloatIntTup= MyTup</f32, i32/>;
		static_assert( same_type</ TupWithFirstIntUnwrapper</ FloatIntTup />::A, f32 /> );
		static_assert( same_type</ TupWithFirstIntUnwrapper</ FloatIntTup />::B, i32 /> );
	"""
	tests_lib.build_program( c_program_text )


def TypeAliasTemplateInAnotherTemplateSignature_Test6():
	c_program_text= """
		template</type T, size_type S/> struct MyArray{ [ T, S ] arr; }
		template</type T/> type SingleElementArray= MyArray</ T, 1s />;
		template</type X/> struct SingleElementArrayUnwrapper</ SingleElementArray</X/> /> { auto is_single_element_array= true; }
		template</type X/> struct SingleElementArrayUnwrapper{ auto is_single_element_array= false; }
		static_assert(  SingleElementArrayUnwrapper</ MyArray</ i32, 1s /> />::is_single_element_array );
		static_assert(  SingleElementArrayUnwrapper</ MyArray</ u64, 1s /> />::is_single_element_array );
		static_assert( !SingleElementArrayUnwrapper</ MyArray</ i32, 2s /> />::is_single_element_array );
		static_assert( !SingleElementArrayUnwrapper</ MyArray</ f32, 0s /> />::is_single_element_array );
	"""
	tests_lib.build_program( c_program_text )


def TypeAliasTemplateInAnotherTemplateSignature_Test7():
	c_program_text= """
		template</type T, size_type S/> struct MyArray{ [ T, S ] arr; }
		template</size_type S/> type FloatArray= MyArray</ f32, S />;
		template</size_type S/> struct FloatArrayUnwrapper</ FloatArray</S/> /> { auto is_float_array= true; }
		template</type X/> struct FloatArrayUnwrapper{ auto is_float_array= false; }
		static_assert(  FloatArrayUnwrapper</ MyArray</ f32, 1s /> />::is_float_array );
		static_assert(  FloatArrayUnwrapper</ MyArray</ f32, 2s /> />::is_float_array );
		static_assert(  FloatArrayUnwrapper</ MyArray</ f32, 9s /> />::is_float_array );
		static_assert(  FloatArrayUnwrapper</ MyArray</ f32, 0s /> />::is_float_array );
		static_assert( !FloatArrayUnwrapper</ MyArray</ i32, 0s /> />::is_float_array );
		static_assert( !FloatArrayUnwrapper</ MyArray</ i32, 1s /> />::is_float_array );
		static_assert( !FloatArrayUnwrapper</ MyArray</ i32, 2s /> />::is_float_array );
	"""
	tests_lib.build_program( c_program_text )


def TypeAliasTemplateInAnotherTemplateSignature_Test8():
	c_program_text= """
		template</type T, size_type S/> type MyArray= [ T, S ];
		template</type X/> struct SingleElementArrayUnwrapper</ MyArray</ X, 1s /> /> { auto is_single_element_array= true; }
		template</type X/> struct SingleElementArrayUnwrapper{ auto is_single_element_array= false; }
		static_assert(  SingleElementArrayUnwrapper</ MyArray</ i32, 1s /> />::is_single_element_array );
		static_assert(  SingleElementArrayUnwrapper</ MyArray</ u64, 1s /> />::is_single_element_array );
		static_assert( !SingleElementArrayUnwrapper</ MyArray</ i32, 2s /> />::is_single_element_array );
		static_assert( !SingleElementArrayUnwrapper</ MyArray</ f32, 0s /> />::is_single_element_array );
	"""
	tests_lib.build_program( c_program_text )


def TypeAliasTemplateInAnotherTemplateSignature_Test9():
	c_program_text= """
		template</type T, size_type S/> type MyArray= [ T, S ];
		template</size_type S/> struct FloatArrayUnwrapper</ MyArray</ f32, S /> /> { auto is_float_array= true; }
		template</type X/> struct FloatArrayUnwrapper{ auto is_float_array= false; }
		static_assert(  FloatArrayUnwrapper</ MyArray</ f32, 1s /> />::is_float_array );
		static_assert(  FloatArrayUnwrapper</ MyArray</ f32, 2s /> />::is_float_array );
		static_assert(  FloatArrayUnwrapper</ MyArray</ f32, 9s /> />::is_float_array );
		static_assert(  FloatArrayUnwrapper</ MyArray</ f32, 0s /> />::is_float_array );
		static_assert( !FloatArrayUnwrapper</ MyArray</ i32, 0s /> />::is_float_array );
		static_assert( !FloatArrayUnwrapper</ MyArray</ i32, 1s /> />::is_float_array );
		static_assert( !FloatArrayUnwrapper</ MyArray</ i32, 2s /> />::is_float_array );
	"""
	tests_lib.build_program( c_program_text )
