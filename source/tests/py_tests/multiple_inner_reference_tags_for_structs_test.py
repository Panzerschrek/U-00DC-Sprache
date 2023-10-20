from py_tests_common import *


def ClassFieldReferenceNotation_Test0():
	c_program_text= """
		struct S
		{
			i32 & @("a"c8) ref;
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassFieldReferenceNotation_Test1():
	c_program_text= """
		struct S
		{
			i32 &mut @("a"c8) ref;
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassFieldReferenceNotation_Test2():
	c_program_text= """
		struct S
		{
			i32 &imut @("a"c8) ref;
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassFieldReferenceNotation_Test3():
	c_program_text= """
		struct S{ i32& x; }
		struct T
		{
			S @("a") s;
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassFieldReferenceNotation_Test4():
	c_program_text= """
		struct S{ i32& x; }
		struct T
		{
			S @("a") imut s;
		}
	"""
	tests_lib.build_program( c_program_text )


def ExpectedReferenceNotation_Test0():
	# Two reference fields - reference notation is required for both.
	c_program_text= """
		struct S
		{
			i32 & x;
			i32 & y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 4 ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 5 ) )


def ExpectedReferenceNotation_Test1():
	# Two reference fields - reference notation is required where it is missing.
	c_program_text= """
		struct S
		{
			i32 & x;
			i32 & @("a"c8) y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 4 ) )


def ExpectedReferenceNotation_Test2():
	# Three reference fields - reference notation is required where it is missing.
	c_program_text= """
		struct S
		{
			i32 & x;
			i32 & @("a"c8) y;
			i32 & z;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 4 ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 6 ) )


def ExpectedReferenceNotation_Test3():
	# Two fields with references inside - reference notation is required for both of them.
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T x;
			T y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 5 ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 6 ) )


def ExpectedReferenceNotation_Test4():
	# Two fields with references inside - reference notation is required where it is missing.
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T @("a") x;
			T y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 6 ) )


def ExpectedReferenceNotation_Test5():
	# Reference field and value field with references inside. Reference notation is required for both of them.
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T x;
			i32& y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 5 ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 6 ) )


def ExpectedReferenceNotation_Test6():
	# Reference field and value field with references inside. Reference notation is required for both of them.
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T x;
			i32 & @("a"c8) y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 5 ) )


def ExpectedReferenceNotation_Test7():
	# Reference field and value field with references inside. Reference notation is required for both of them.
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T @("a") x;
			i32& y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 6 ) )


def InvalidInnerReferenceTagName_Test0():
	c_program_text= """
		struct S
		{
			i32& @("*"c8) y; // Non-letter symobl.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidInnerReferenceTagName", 4 ) )


def InvalidInnerReferenceTagName_Test1():
	c_program_text= """
		struct S
		{
			i32& @("F"c8) y; // Capital letters for now are not supported.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidInnerReferenceTagName", 4 ) )


def InvalidInnerReferenceTagName_Test2():
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T @("$") t;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidInnerReferenceTagName", 5 ) )


def InnerReferenceTagCountMismatch_Test0():
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T @("ab") t; // Expected 1, got 2.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InnerReferenceTagCountMismatch", 5 ) )


def InnerReferenceTagCountMismatch_Test1():
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T @("") t; // Expected 1, got 0.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InnerReferenceTagCountMismatch", 5 ) )


def InnerReferenceTagCountMismatch_Test2():
	c_program_text= """
		struct T
		{
			i32 &mut @("a"c8) x;
			i32 &mut @("b"c8) y;
		}
		struct S
		{
			T @("a") t; // Expected 2, got 1.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InnerReferenceTagCountMismatch", 9 ) )


def InnerReferenceTagCountMismatch_Test3():
	c_program_text= """
		struct T{}
		struct S
		{
			T @("a") t; // Expected 0, got 1.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InnerReferenceTagCountMismatch", 5 ) )
