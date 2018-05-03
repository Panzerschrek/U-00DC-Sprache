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


def ContinuousInnerReferenceTagUsedAsReturnValueInnerReferenceTag_Test0():
	c_program_text= """
		struct S
		{
			i32& x;
			fn constructor( this'a', i32&'b in_x ) ' a <- imut b '
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
			fn constructor( this'a', i32&'b in_x ) ' a <- imut b '
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
			fn constructor( this'a', i32&'b in_x ) ' a <- imut b '
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
			fn constructor( this'a', i32&'b in_x ) ' a <- imut b '
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
			fn constructor( this'a', i32&'b in_x ) ' a <- imut b '
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
