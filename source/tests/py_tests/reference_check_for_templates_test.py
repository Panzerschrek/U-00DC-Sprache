from py_tests_common import *


def ContinuousInnerReferenceTagDeclaration_Test0():
	c_program_text= """
		struct S{}
		fn Foo( S& s' x... ' ){}
	"""
	tests_lib.build_program( c_program_text )


def ContinuousInnerReferenceTagUsedAsReturnReferenceTag_Test0():
	c_program_text= """
		struct S
		{
			i32& x;
			fn constructor( this'a', i32&'b in_x ) ' a <- b '
			( x(in_x) ) {}
		}
		fn Extract( S& s' a... ' ) : i32 &'a
		{
			return s.x;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			auto& ref= Extract(S(x));  // 'ref' now contains reference to 'x'
			++x; // Error, 'x' have references
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].file_pos.line == 17 )


def ContinuousInnerReferenceTagUsedAsReturnReferenceTag_Test1():
	c_program_text= """
		struct S{}
		auto constexpr global_constant= 42;
		fn Extract( S& s' a... ' ) : i32 &'a   // continuous tag for struct with zero inner tags
		{
			return global_constant;
		}

		fn Foo()
		{
			Extract(S());
		}
	"""
	tests_lib.build_program( c_program_text )


def ContinuousInnerReferenceTagUsedAsReturnValueInnerReferenceTag_Test0():
	c_program_text= """
		struct S
		{
			i32& x;
			fn constructor( this'a', i32&'b in_x ) ' a <- b '
			( x(in_x) ) {}
		}
		fn Pass( S& s' a... ' ) : S' a '
		{
			return s;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			auto& ref= Pass(S(x)).x;  // 'ref' now contains reference to 'x'
			++x; // Error, 'x' have references
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].file_pos.line == 17 )


def ContinuousInnerReferenceTagForReturnValue_Test0():
	c_program_text= """
		struct S
		{
			i32& x;
			fn constructor( this'a', i32&'b in_x ) ' a <- b '
			( x(in_x) ) {}
		}
		fn Box( i32&'a x ) : S' a... '
		{
			return S(x);
		}

		fn Foo()
		{
			var i32 mut x= 0;
			auto& ref= Box(x).x;  // 'ref' now contains reference to 'x'
			++x; // Error, 'x' have references
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].file_pos.line == 17 )


def ContinuousInnerReferenceTagForReturnValue_Test1():
	c_program_text= """
		struct S {}
		fn Bar( i32&'a x ) : S' a... '   // continuous tag for struct with zero inner tags
		{
			return S();
		}

		fn Foo()
		{
			Bar(42);
		}
	"""
	tests_lib.build_program( c_program_text )


def ContinuousInnerReferenceTagForReturnValue_Test2():
	c_program_text= """
		struct S
		{
			i32& x;
			fn constructor( this'a', i32&'b in_x ) ' a <- b '
			( x(in_x) ) {}
		}
		fn Pass( S& s'a' ) : S' a... '
		{
			return s;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			auto& ref= Pass(S(x)).x;  // 'ref' now contains reference to 'x'
			++x; // Error, 'x' have references
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].file_pos.line == 17 )


def ContinuousInnerReferenceTagForReturnValue_Test3():
	c_program_text= """
		struct S
		{
			i32& x;
			fn constructor( this'a', i32&'b in_x ) ' a <- b '
			( x(in_x) ) {}
		}
		fn Pass( S& s' a... ' ) : S' a... '   // Here we bind all inner tags of arg to all inner tags of return value
		{
			return s;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			auto& ref= Pass(S(x)).x;  // 'ref' now contains reference to 'x'
			++x; // Error, 'x' have references
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].file_pos.line == 17 )


def ContinuousInnerReferenceTagForReturnValue_Test5():
	c_program_text= """
		struct T{}
		struct S
		{
			i32& x;
			fn constructor( this'a', i32&'b in_x ) ' a <- b '
			( x(in_x) ) {}
		}
		auto constexpr global_constant= 42;
		fn Convert( T& t' a... ' ) : S' a... '   // 'a...' for 't' actually not used
		{
			return S(global_constant);
		}

		fn Foo()
		{
			var i32 mut x= 0;
			auto& ref= Convert(T()).x;  // 'ref' now contains reference to 'x'
			++x; // Ok, 'x' have no references
		}
	"""
	tests_lib.build_program( c_program_text )


def ContinuousInnerReferenceTagForThis_Test0():
	c_program_text= """
		struct S
		{
			i32& x;
			fn constructor( this'a...', i32&'b in_x ) ' a <- b '    // Pollution works here, because 'a...' expands to one reference tag.
			( x(in_x) ) {}
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var S s( x );
			++x; // Error, 'x' have reference inside 's'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].file_pos.line == 13 )


def ContinuousInnerReferenceTagForThis_Test1():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor( this'a...', i32&'b in_x ) ' a <- b '    // Pollution does not works here, because 'a...' expands to zero reference tags.
			( x(in_x) ) {}
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var S s( x );
			++x; // Ok, 'x' has no references.
		}
	"""
	tests_lib.build_program( c_program_text )


def ContinuousInnerReferenceTag_InPollution_Test0():
	c_program_text= """
		struct S
		{
			i32& x;
			fn constructor( this'a', i32&'b in_x ) ' a <- b '
			( x(in_x) ) {}
		}

		fn Bar( S &mut s'a...', i32&'b x ) ' a <- b' {}   // For pullution used continuous tag.

		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s(x);
			Bar(s, y);  // Save reference to 'y' inside 's'
			++y; // Error, 'y' have references
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].file_pos.line == 16 )


def ContinuousInnerReferenceTag_InPollution_Test1():
	c_program_text= """
		struct S{}
		fn Bar( S &mut s'a...', i32&'b x ) ' a <- b' {}   // For pullution used continuous tag, but actual pollution not happens.

		fn Foo()
		{
			var i32 mut x= 0;
			var S mut s;
			Bar(s, x);
			++x; // ok, 'x' have no references
		}
	"""
	tests_lib.build_program( c_program_text )


def VariativeReferenceTagsCount_InTemplateClass_Test0():
	c_program_text= """
		template</ type T />
		class Vec
		{
			fn push_back( mut this'x...', T el'y...' ) ' x <- y ' {}
			[ T, 0u ] container_marker;
		}

		struct S{ i32& r; i32 v; }

		fn Foo()
		{
			var i32 mut a= 0;
			var Vec</i32/> mut vec;
			{
				var S s{ .r= a, .v= 0 };
				vec.push_back( s.v ); // Pushes part of struct, which contains reference. But must NOT save reference inside, because pollution for Vec</i32/> does not works.
			}
			++a; // Ok, can modify, because 'a' has no references
		}
	"""
	tests_lib.build_program( c_program_text )


def VariativeReferenceTagsCount_InTemplateClass_Test1():
	c_program_text= """
		template</ type T />
		class Vec
		{
			fn push_back( mut this'x...', T el'y...' ) ' x <- y '{}
			fn get_val( this'x...' ) : T'x...' { return T(0); }
			[ T, 0u ] container_marker;
		}

		fn Foo()
		{
			var i32 mut a= 0, mut b= 0;
			{
				var Vec</ i32 /> mut vec;
				vec.push_back(a);
				b= vec.get_val();
			}
			++a; // Ok, can modify, because 'a' has no references
		}
	"""
	tests_lib.build_program( c_program_text )


def VariativeReferenceTagsCount_InTemplateClass_Test2():
	c_program_text= """
		template</ type T />
		class Vec
		{
			fn push_back( mut this'x...', T el'y...' ) ' x <- y ' {}
			[ T, 0u ] container_marker;
		}

		struct S{ i32& r; i32 v; }

		fn Foo()
		{
			var i32 mut a= 0;
			var Vec</S/> mut vec;
			{
				var S s{ .r= a, .v= 0 };
				vec.push_back( s ); // Must save reference inside.
			}
			++a; // Error, can not modify, because 'a' has reference inside 'vec'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].file_pos.line == 19 )


def VariativeReferenceTagsCount_InTemplateClass_Test3():
	c_program_text= """
		template</ type T />
		struct Box
		{
			T boxed;
			fn Get( this'x...' ) : T'x...' { return boxed; }
		}

		struct S{ i32& r; }

		fn Foo()
		{
			var Box</i32/> box{ .boxed= 0 };
			box.Get();

			var Box</S/> s{ .boxed{ .r= box.boxed } };
			s.Get();
		}
	"""
	tests_lib.build_program( c_program_text )


def VariativeReferenceTagsCount_InTemplateClass_Test4():
	c_program_text= """
		template</ type T />
		struct Box
		{
			T boxed;
			fn Get( this'x...' ) : T'x...' { return boxed; }
		}

		struct S{ i32& r; }

		fn Pass( i32& x ) : i32&
		{
			var Box</S/> box{ .boxed{ .r= x } };
			return box.Get().r;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			auto& r= Pass(x);
			++x; // Error, 'x' have references.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].file_pos.line == 21 )


def ReferenceTagsForTemplateDependentType_Test0():
	c_program_text= """
		template</ type T />
		fn Foo( T t' a ' ){} // Inner references tag for template-dependent arg type.
	"""
	tests_lib.build_program( c_program_text )


def ReferenceTagsForTemplateDependentType_Test1():
	c_program_text= """
		struct S{ i32& r; }
		template</ type T />
		fn Foo( S s'a' ) : T'a' { return T(); }  // Inner references tag for template-dependent return type.
	"""
	tests_lib.build_program( c_program_text )


def ReferenceTagsForTemplateDependentType_Test2():
	c_program_text= """
		template</ type T />
		fn Foo( i32 x'a' ) : T { return T(); }  // Invalid tag count for non-template-dependent argument of template function.

		var (fn(i32 x ) : i32) constexpr ptr( Foo</i32/> );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TemplateContext" )
	assert( errors_list[0].template_errors.errors[0].error_code == "InvalidReferenceTagCount" )
	assert( errors_list[0].template_errors.errors[0].file_pos.line == 3 )


def ReferenceTagsForTemplateDependentType_Test3():
	c_program_text= """
		template</ type T />
		fn Foo( T t ) : i32'a' { return T(); }  // Invalid tag count for non-template-dependent return type of template function.

		var (fn(i32 x ) : i32) constexpr ptr( Foo</i32/> );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TemplateContext" )
	assert( errors_list[0].template_errors.errors[1].error_code == "InvalidReferenceTagCount" )
	assert( errors_list[0].template_errors.errors[1].file_pos.line == 3 )
