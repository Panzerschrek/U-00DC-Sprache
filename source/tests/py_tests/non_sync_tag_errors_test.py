from py_tests_common import *


def NonSyncTagExpression_Check0():
	c_program_text= """
		class A
		non_sync( unknown_name )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].src_loc.line == 3 )


def NonSyncTagExpression_Check1():
	c_program_text= """
		class A
		non_sync( 0 )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 3 )


def NonSyncTagExpression_Check2():
	c_program_text= """
		fn Foo() : bool;

		class A
		non_sync( Foo() )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedConstantExpression" )
	assert( errors_list[0].src_loc.line == 5 )


def NonSyncTagDependencyLoop_Test0():
	c_program_text= """
		struct S non_sync( v ) {} // Can't handle expression except "non_sync" expression without triggering globals loop.
		auto v = non_sync</S/>;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalsLoopDetected", 2 ) or HasError( errors_list, "GlobalsLoopDetected", 3 ) )


def NonSyncTagDependencyLoop_Test1():
	c_program_text= """
		struct S non_sync( !non_sync</S/> ) {} // Can't handle complex expression in "non_sync" tag.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalsLoopDetected", 2 ) )


def NonSyncTagDependencyLoop_Test2():
	c_program_text= """
		auto x = false;
		struct S non_sync( x || non_sync</S/> ) {} // Can't handle complex expression in "non_sync" tag.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalsLoopDetected", 3 ) )


def NonSyncTagAdditionInInheritance_Test0():
	c_program_text= """
		// Mark derived class as "non_sync".
		class A polymorph {}
		class B : A non_sync {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NonSyncTagAdditionInInheritance" )
	assert( errors_list[0].src_loc.line == 4 )


def NonSyncTagAdditionInInheritance_Test1():
	c_program_text= """
		// Add "non_sync" fields into derived class.
		class A interface {}
		class B : A { C c; }
		class C non_sync {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NonSyncTagAdditionInInheritance" )
	assert( errors_list[0].src_loc.line == 4 )


def NonSyncTagAdditionInInheritance_Test2():
	c_program_text= """
		// Derive "non_sync" tag from one of parents.
		class A interface {}
		class B interface non_sync {}
		class C : A, B {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NonSyncTagAdditionInInheritance" )
	assert( errors_list[0].src_loc.line == 5 )


def NonSyncTagAdditionInInheritance_Test3():
	c_program_text= """
		// Derive "non_sync" tag from all parents. This is not an error.
		class A interface non_sync {}
		class B interface non_sync {}
		class C : A, B {}
	"""
	tests_lib.build_program( c_program_text )
