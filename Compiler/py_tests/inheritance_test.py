from py_tests_common import *

def InheritanceTes_ClassKindAttribute_Test0():
	c_program_text= """
	class S final {}
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassKindAttribute_Test1():
	c_program_text= """
	class S polymorph {}
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassKindAttribute_Test2():
	c_program_text= """
	class S interface {}
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassKindAttribute_Test3():
	c_program_text= """
	class S abstract {}
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassKindAttribute_Test4():
	c_program_text= """
		template</ type T />
		class S</T/> abstract {}    // class kind attribute after template signature parameters
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassParentsList_Test0():
	c_program_text= """
		class A polymorph{}
		class C polymorph : A {}  // Single parent + kind attribute
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassParentsList_Test1():
	c_program_text= """
		class A interface{}
		class B polymorph{}
		class C final : A, B {} // Multiple parents + kind attribute
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassParentsList_Test2():
	c_program_text= """
		class A interface{}
		class B polymorph{}
		class C : A, B {} // Multiple parents and no kind attribute
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassParentsList_Test3():
	c_program_text= """
		class A polymorph{}
		template</ type T />
		class C</T/> polymorph : A {}  // Single parent + kind attribute + template
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassParentsList_Test4():
	c_program_text= """
		class A interface{}
		class B polymorph{}
		template</ type T />
		class C</T/> final : A, B {} // Multiple parents + kind attribute + template
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassParentsList_Test5():
	c_program_text= """
		class A interface{}
		class B polymorph{}
		template</ type T />
		class C</T/> : A, B {} // Multiple parents and no kind attribute + template
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ClassParentsList_Test6():
	c_program_text= """
		namespace NNN{   class A polymorph{}   }
		class C : NNN::A {}  // Single parent inside namespace
	"""
	tests_lib.build_program( c_program_text )
