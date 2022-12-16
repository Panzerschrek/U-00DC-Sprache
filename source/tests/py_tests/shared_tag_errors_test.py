from py_tests_common import *


def SharedTagExpression_Check0():
	c_program_text= """
		class A
		shared( unknown_name )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].src_loc.line == 3 )


def SharedTagExpression_Check1():
	c_program_text= """
		class A
		shared( 0 )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 3 )


def SharedTagExpression_Check2():
	c_program_text= """
		fn Foo() : bool;

		class A
		shared( Foo() )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedConstantExpression" )
	assert( errors_list[0].src_loc.line == 5 )


def SharedTagDependencyLoop_Test0():
	c_program_text= """
		struct S shared( v ) {} // Can't handle expression except "shared" expression without triggering globals loop.
		auto v = shared</S/>;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GlobalsLoopDetected", 2 ) or HaveError( errors_list, "GlobalsLoopDetected", 3 ) )


def SharedTagDependencyLoop_Test1():
	c_program_text= """
		struct S shared( !shared</S/> ) {} // Can't handle complex expression in "shared" tag.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GlobalsLoopDetected", 2 ) )


def SharedTagDependencyLoop_Test2():
	c_program_text= """
		auto x = false;
		struct S shared( x || shared</S/> ) {} // Can't handle complex expression in "shared" tag.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GlobalsLoopDetected", 3 ) )


def SharedTagAdditionInInheritance_Test0():
	c_program_text= """
		// Mark derived class as "shared".
		class A polymorph {}
		class B : A shared {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "SharedTagAdditionInInheritance" )
	assert( errors_list[0].src_loc.line == 4 )


def SharedTagAdditionInInheritance_Test1():
	c_program_text= """
		// Add "shared" fields into derived class.
		class A interface {}
		class B : A { C c; }
		class C shared {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "SharedTagAdditionInInheritance" )
	assert( errors_list[0].src_loc.line == 4 )


def SharedTagAdditionInInheritance_Test2():
	c_program_text= """
		// Derive "shared" tag from one of parents.
		class A interface {}
		class B interface shared {}
		class C : A, B {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "SharedTagAdditionInInheritance" )
	assert( errors_list[0].src_loc.line == 5 )


def SharedTagAdditionInInheritance_Test3():
	c_program_text= """
		// Derive "shared" tag from all parents. This is not an error.
		class A interface shared {}
		class B interface shared {}
		class C : A, B {}
	"""
	tests_lib.build_program( c_program_text )
