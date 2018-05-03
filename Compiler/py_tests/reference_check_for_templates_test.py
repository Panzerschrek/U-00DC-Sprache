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
			fn constructor( this'a', i32&'b in_x ) ' a <- imut b '
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
