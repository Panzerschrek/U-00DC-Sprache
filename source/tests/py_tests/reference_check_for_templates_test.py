from py_tests_common import *


def ReferenceTagForTypeWithoutReferencesInside_Test0():
	c_program_text= """
		struct S{}
		fn Foo( S& s'x' ){}
	"""
	tests_lib.build_program( c_program_text )


def ReferenceTagForTypeWithoutReferencesInside_UsedAsReturnReferenceTag_Test1():
	c_program_text= """
		struct S{}
		auto constexpr global_constant= 42;
		fn Extract( S& s'a' ) : i32 &'a // tag for struct with zero inner tags
		{
			return global_constant;
		}

		fn Foo()
		{
			Extract(S());
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferenceTagForTypeWithoutReferencesInside_ForReturnValue_Test1():
	c_program_text= """
		struct S {}
		fn Bar( i32&'a x ) : S'a' //  tag for struct with zero inner tags
		{
			return S();
		}

		fn Foo()
		{
			Bar(42);
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferenceTagForTypeWithoutReferencesInside_ForThis_Test1():
	c_program_text= """
		struct S
		{
			i32 x;
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( this'a', i32&'b in_x ) @(pollution) // Pollution does not works here, because 'a' expands to zero reference tags.
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


def ReferenceTagForTypeWithoutReferencesInside_InPollution_Test1():
	c_program_text= """
		struct S{}
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn Bar( S &mut s'a', i32&'b x ) @(pollution) {}   // Actual pollution not happens, because "S" have no references inside.

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
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1a" ] ];
			fn push_back( mut this'x', T el'y' ) @(pollution) {}
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
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1a" ] ];
			fn push_back( mut this'x', T el'y' ) @(pollution) {}
			fn get_val( this'x' ) : T'x' { return T(0); }
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
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1a" ] ];
			fn push_back( mut this'x', T el'y' ) @(pollution) {}
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
	assert( errors_list[0].src_loc.line == 20 )


def VariativeReferenceTagsCount_InTemplateClass_Test3():
	c_program_text= """
		template</ type T />
		struct Box
		{
			T boxed;
			fn Get( this'x' ) : T'x' { return boxed; }
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
			fn Get( this'x' ) : T'x' { return boxed; }
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
	assert( errors_list[0].src_loc.line == 21 )


def ReferenceTagsForTemplateDependentType_Test0():
	c_program_text= """
		template</ type T />
		fn Foo( T t'a' ){} // Inner references tag for template-dependent arg type.
	"""
	tests_lib.build_program( c_program_text )


def ReferenceTagsForTemplateDependentType_Test1():
	c_program_text= """
		struct S{ i32& r; }
		template</ type T />
		fn Foo( S s'a' ) : T'a' { return T(); }  // Inner references tag for template-dependent return type.
	"""
	tests_lib.build_program( c_program_text )
