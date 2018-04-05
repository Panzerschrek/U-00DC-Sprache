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


def InheritanceTest_ParentClassNameVisibleInChild_Test0():
	c_program_text= """
		class A
		{
			type I= i32;
		}
		class B : A{}

		fn Foo() : i32
		{
			var B::I r= 5652111;   // B::I must be visible
			return r;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5652111 )


def InheritanceTest_ChildClassNameOverridesParentClassName_Test0():
	c_program_text= """
		class A
		{
			type I= f64;
		}
		class B : A
		{
			type I= i32;
		}

		fn Foo() : i32
		{
			var B::I r= 24574;   // B::I must be selected, instead of A::I
			return r;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 24574 )


def InheritanceTest_ParentClassFieldAccess_Test0():
	c_program_text= """
		class A
		{
			i32 a;
			fn constructor()( a= 541 ){}
		}
		class B : A
		{
			f32 b;
			fn constructor()( b= 124.3f ){}
		}
		class C : B
		{
			f64 c;
			fn constructor()( c= -54.2 ){}
		}

		fn Foo() : i32
		{

			var C c;
			return i32( f64(c.a) - f64(c.b) / f64(c.c) );   // Access parent fields via .member_access.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def InheritanceTest_ParentClassFieldAccess_Test1():
	c_program_text= """
		class A
		{
			i32 a;
			fn constructor()( a= 541 ){}
		}
		class B : A
		{
			f32 b;
			fn constructor()( b= 124.3f ){}
		}
		class C : B
		{
			f64 c;
			fn constructor()( c= -54.2 ){}
			fn Foo( this ) :i32
			{
				return i32( f64(a) - f64(b) / f64(c) );  // Access parent fileds via NamedOperand.
			}
		}

		fn Foo() : i32
		{
			var C c;
			return c.Foo();
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def InheritanceTest_InitializeBaseClass_Test0():
	c_program_text= """
		class A
		{
			i32 a;
			fn constructor()( a= 541 ){}
		}
		class B : A
		{
			f32 b;
			fn constructor()( b= 124.3f ){}   // Must implicitly call A::constructor
		}

		fn Foo()
		{
			var B b;
			halt if( b.a != 541 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def InheritanceTest_InitializeBaseClass_Test1():
	c_program_text= """
		class A
		{
			i32 a;
			fn constructor( i32 x )( a= x ){}
		}
		class B : A
		{
			fn constructor( i32 x )( base(x) ){}   // Must explicitly call A::constructor
		}

		fn Foo()
		{
			var B b( 55521 );
			halt if( b.a != 55521 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
