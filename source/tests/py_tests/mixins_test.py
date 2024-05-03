from py_tests_common import *


def NamespaceMixinDeclaration_Test0():
	c_program_text= """
		mixin( "fn Foo() : i32 { return 665533; }" );
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 665533 )


def NamespaceMixinDeclaration_Test1():
	c_program_text= """
		namespace Some
		{
			mixin( "fn Foo() : f32 { return 3.25f; }" );
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_ZN4Some3FooEv" ) == 3.25 )


def ClassMixinDeclaration_Test0():
	c_program_text= """
		struct S
		{
			mixin( "i32 x;" ); // Add a field via mixin.
		}
		fn Foo() : i32
		{
			var S s{ .x= 76567 };
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 76567 )


def ClassMixinDeclaration_Test1():
	c_program_text= """
		struct S
		{
			auto x_field= "u64 x;";
			auto x_method= "fn GetSquaredX( this ) : u64 { return x * x; }";
			mixin( x_field + "\\n\\t" + x_method ); // Add a field and a method in mixin.
		}
		fn Foo() : u64
		{
			var S s{ .x(543) };
			return s.GetSquaredX();
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 543 * 543 )


def MixinWithinTemplate_Test0():
	c_program_text= """
		template</type T/>
		struct S
		{
			mixin( "T t;" ); // Mixin with string independent on template args.
		}
		fn Foo() : i32
		{
			var S</f32/> s0{ .t= 15.3f };
			var S</u32/> s1{ .t= 66u };
			return i32(s0.t) + i32(s1.t);
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 66 + 15 )


def MixinWithinTemplate_Test1():
	c_program_text= """
		template</type T, size_type n/>
		struct S
		{
			mixin(MakeFields()); // Mixin string depends on template args.

			fn constexpr MakeFields() : auto
			{
				var [ char8, 128 ] mut s= zero_init;
				auto mut dst= 0s;
				for( auto mut i= 0s; i < n; ++i )
				{
					StrAppend( s, dst, "T field" );
					s[dst]= char8( size_type("0"c8) + i );
					++dst;
					StrAppend( s, dst, ";\\n" );
				}

				return s;
			}
		}
		template</size_type S0, size_type S1/>
		fn constexpr StrAppend( [ char8, S0 ] &mut dst, size_type &mut dst_index, [ char8, S1 ]& src )
		{
			for( auto mut i= 0s; i < S1; ++i )
			{
				dst[ dst_index + i ]= src[i];
			}
			dst_index+= S1;
		}
		fn Foo() : i32
		{
			var S</i32, 3s/> s{ .field0= 87, .field1= 16, .field2= 3 };
			return s.field0 * s.field1 - s.field2;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 87 * 16 - 3 )


def MixinWithinTemplate_Test2():
	c_program_text= """
		template</type T/>
		struct FieldsCopy
		{
			mixin(MakeFields()); // Mixin string depends on template args.

			// Generate fields exactly like in source class.
			fn constexpr MakeFields() : auto
			{
				var [ char8, 1024 ] mut s= zero_init;
				auto mut dst= 0s;
				auto mut i= 0s;
				for( &field_info : typeinfo</T/>.fields_list )
				{
					StrAppend( s, dst, "typeof(typeinfo</T/>.fields_list[" );
					s[dst]= char8( size_type("0"c8) + i );
					++dst;
					StrAppend( s, dst, "].type)::src_type " );
					StrAppend( s, dst, field_info.name );
					StrAppend( s, dst, ";\\n" );

					++i;
				}

				return s;
			}
		}
		template</size_type S0, size_type S1/>
		fn constexpr StrAppend( [ char8, S0 ] &mut dst, size_type &mut dst_index, [ char8, S1 ]& src )
		{
			for( auto mut i= 0s; i < S1; ++i )
			{
				dst[ dst_index + i ]= src[i];
			}
			dst_index+= S1;
		}
		struct S
		{
			i32 int_field;
			f32 float_field;
			bool bool_field;
		}
		struct T
		{
			u64 single_field;
		}
		fn Foo() : i32
		{
			var FieldsCopy</S/> s_fields_copy{ .int_field= 55, .float_field= 14.6f, .bool_field= false };
			var FieldsCopy</T/> t_fields_copy{ .single_field(12345678) };
			return 0;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def MixinWithinTemplate_Test3():
	c_program_text= """
		template</type T/>
		struct S
		{
			struct Impl
			{
				mixin( "T t;" ); // Expand mixin in struct nested inside template struct.
			}
			Impl impl;
		}
		fn Foo() : i32
		{
			var S</f32/> s0{ .impl{ .t= 15.3f } };
			var S</u32/> s1{ .impl{ .t= 66u } };
			return i32(s0.impl.t) + i32(s1.impl.t);
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 66 + 15 )


def MixinVisibilityLabel_Test0():
	c_program_text= """
		class C
		{
			mixin( "i32 x= 45;" ); // No visibility label specified - it's public.
		}
		fn Foo() : i32
		{
			var C c;
			return c.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 45 )


def MixinVisibilityLabel_Test1():
	c_program_text= """
		class C
		{
		private:
			mixin( "public: i32 x= 6498;" ); // Override "private" with public.
		}
		fn Foo() : i32
		{
			var C c;
			return c.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 6498 )


def MixinVisibilityLabel_Test2():
	c_program_text= """
		class C
		{
		private:
			mixin( "i32 x= 45;" ); // "Private" is used here for mixin contents.
		}
		fn Foo() : i32
		{
			var C c;
			return c.x; // Error "x" is private.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingNonpublicClassMember", 10 ) )


def MixinVisibilityLabel_Test3():
	c_program_text= """
		class C
		{
		public:
			mixin( "private: i32 x= 45;" ); // Override "public" with "private".
		}
		fn Foo() : i32
		{
			var C c;
			return c.x; // Error "x" is private.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingNonpublicClassMember", 10 ) )


def MixinVisibilityLabel_Test4():
	c_program_text= """
		class C
		{
			mixin( "private: " ); // Visibility label within miin can't affect code outside this mixin.
			i32 x= 7769;
		}
		fn Foo() : i32
		{
			var C c;
			return c.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 7769 )


def MixinWithinMixin_Test0():
	c_program_text= """
		mixin( mixin0_text );
		auto mixin0_text= "mixin( mixin1_text );";
		auto mixin1_text= " fn Foo() : i32 { return 12876; } ";
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 12876 )


def MixinWithinMixin_Test1():
	c_program_text= """
		mixin( mixin0_text );
		auto mixin0_text= "mixin( mixin1_text );";
		auto mixin1_text= "mixin( mixin2_text );";
		auto mixin2_text= " fn Foo() : i32 { return 7778; } ";
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 7778 )


def MixinWithinMixin_Test2():
	c_program_text= """
		template</type T/>
		struct S
		{
			mixin( mixin0_text );
			auto mixin0_text= "mixin( mixin1_text );";
			auto mixin1_text= "mixin( mixin2_text );";
			auto mixin2_text= " fn Foo() : i32 { return 4445; } ";
		}
		fn Foo() : i32
		{
			return S</f64/>::Foo();
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 4445 )


def MacroUsageInsideMixin_Test0():
	c_program_text= """
		?macro <? DEFINE_FUNC:namespace ?name:ident ?>  ->
		<? fn ?name() : i32 { return 987678; } ?>
		mixin( "DEFINE_FUNC Foo" );
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 987678 )


def MacroUsageInsideMixin_Test1():
	c_program_text= """
		?macro <? DEFINE_INT_FIELD:class ?name:ident ?>  ->
		<? i32 ?name; ?>
		struct S
		{
			mixin( "DEFINE_INT_FIELD x" );
		}
		fn Foo() : i32
		{
			var S s{ .x= 787878 };
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 787878 )


def CommentInsideMixin_Test0():
	c_program_text= """
		namespace SomeSpace
		{
			mixin( "// useless line comment" );
		}
	"""
	tests_lib.build_program( c_program_text )


def CommentInsideMixin_Test1():
	c_program_text= """
		struct S
		{
			mixin( "/* useless multiline-style single-line comment */" );
		}
	"""
	tests_lib.build_program( c_program_text )


def CommentInsideMixin_Test2():
	c_program_text= """
		mixin( "// comment before \\n fn Foo() : i32 { /* comment \\n inside \\n */ return 656; } // comment after " );
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 656 )


def MixinWithNoSyntaxElements_Test0():
	c_program_text= """
		// Only whitespaces.
		mixin( " \\t  \\n \\r  " );
	"""
	tests_lib.build_program( c_program_text )


def MixinWithNoSyntaxElements_Test1():
	c_program_text= """
		// Empty string.
		mixin( "" );
	"""
	tests_lib.build_program( c_program_text )


def MixinWithNoSyntaxElements_Test2():
	c_program_text= """
		// Zero string.
		mixin( s );
		var [ char8, 16 ] s= zero_init;
	"""
	tests_lib.build_program( c_program_text )


def MixinsFieldsOrdered_Test0():
	c_program_text= """
		template</ size_type size0, size_type size1 />
		fn constexpr StringEquals( [ char8, size0 ]& s0, [ char8, size1 ]& s1 ) : bool
		{
			if( size0 != size1 ) { return false; }
			var size_type mut i(0);
			while( i < size0 )
			{
				if( s0[i] != s1[i] ) { return false; }
				++i;
			}
			return true;
		}

		template</ type T, size_type name_size />
		fn constexpr GetFieldOffset( T& list, [ char8, name_size ]& name ) : size_type
		{
			for( &list_element : list )
			{
				if( StringEquals( list_element.name, name ) )
				{
					return list_element.offset;
				}
			}
			halt;
		}

		// Mixins are processed after regular fields (they have order after them).
		struct A ordered
		{
			mixin( "i32 z;" );
			f32 x;
			u32 y;
		}
		static_assert( GetFieldOffset( typeinfo</A/>.fields_list, "x" ) == 0s );
		static_assert( GetFieldOffset( typeinfo</A/>.fields_list, "y" ) == 4s );
		static_assert( GetFieldOffset( typeinfo</A/>.fields_list, "z" ) == 8s );

		struct B ordered
		{
			// Mixins are ordered one relative to another.
			mixin( "i32 z;" );
			mixin( "f32 y;" );
			mixin( "u32 x;" );
		}
		static_assert( GetFieldOffset( typeinfo</B/>.fields_list, "z" ) == 0s );
		static_assert( GetFieldOffset( typeinfo</B/>.fields_list, "y" ) == 4s );
		static_assert( GetFieldOffset( typeinfo</B/>.fields_list, "x" ) == 8s );

		struct BAltered ordered
		{
			// Mixins are ordered one relative to another.
			mixin( "u32 x;" );
			mixin( "f32 y;" );
			mixin( "i32 z;" );
		}
		static_assert( GetFieldOffset( typeinfo</BAltered/>.fields_list, "x" ) == 0s );
		static_assert( GetFieldOffset( typeinfo</BAltered/>.fields_list, "y" ) == 4s );
		static_assert( GetFieldOffset( typeinfo</BAltered/>.fields_list, "z" ) == 8s );

		struct C ordered
		{
			// Nested mixins have greater order.
			mixin( "mixin( \\"i32 x;\\" );" );
			mixin( "u32 y;" );
		}
		static_assert( GetFieldOffset( typeinfo</C/>.fields_list, "y" ) == 0s );
		static_assert( GetFieldOffset( typeinfo</C/>.fields_list, "x" ) == 4s );
	"""
	tests_lib.build_program( c_program_text )


def MixinOutOfLineFunction_Test0():
	c_program_text= """
		namespace Some
		{
			fn Foo() : f32;
		}
		// Add out of line function in mixin, that was declared previously using regular way.
		mixin( " fn Some::Foo() : f32 { return 3.25f; } " );
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_ZN4Some3FooEv" ) == 3.25 )


def MixinOutOfLineFunction_Test1():
	c_program_text= """
		// Add both declaration and out of line definition for a function.
		mixin( " namespace Some{ fn Foo() : i32; } fn Some::Foo() : i32 { return 7023; } " );
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_ZN4Some3FooEv" ) == 7023 )


def MixinOutOfLineFunction_Test2():
	c_program_text= """
		// Add both declaration and out of line definition for a function.
		mixin( " struct Some{ fn Foo() : i32; } fn Some::Foo() : i32 { return 967; } " );
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_ZN4Some3FooEv" ) == 967 )


def MixinWithinBlock_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			mixin( "   " );
			return 54;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 54 )


def MixinWithinBlock_Test1():
	c_program_text= """
		fn Foo() : i32
		{
			mixin( "return 786;" );
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 786 )


def MixinWithinBlock_Test2():
	c_program_text= """
		fn Foo() : i32
		{
			mixin( "var i32 x= 89898;" );
			return x; // Access a variable, declared within mixin.
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 89898 )


def MixinWithinBlock_Test3():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 x= 444555;
			mixin( "return x;" ); // Access from a mixin a local variable.
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 444555 )


def MixinWithinBlock_Test4():
	c_program_text= """
		fn Foo() : i32
		{
			// Declare two variables in separate mixins and access them in third mixin.
			mixin( "var i32 x= 89898;" );
			mixin( "var i32 y= 675;" );
			mixin( "return x - y;" );
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 89898 - 675 )


def MixinWithinBlock_Test5():
	c_program_text= """
		fn Foo() : i32
		{
			// Several statements within mixin.
			mixin( "var i32 x= 680; var i32 y= 4; return x / y;" );
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 680 / 4 )


def MixinWithinBlock_Test5():
	c_program_text= """
		struct S
		{
			auto ret_444= "return 444;";
			fn Foo() : i32
			{
				mixin( ret_444 );
			}
		}
		fn Foo() : i32
		{
			return S::Foo();
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 444 )
