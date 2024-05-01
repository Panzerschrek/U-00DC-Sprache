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
		<? i32 ?name; } ?>
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
