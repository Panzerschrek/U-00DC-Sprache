from py_tests_common import *


def InheritanceTest_ClassKindAttribute_Test0():
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
		class A polymorph
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
		class A polymorph
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


def InheritanceTest_ChildClassNameOverridesParentClassName_Test1():
	c_program_text= """
		class A polymorph
		{
			type I= f64;
		}
		class B : A
		{
			fn I() : i32    // Child class have different kind of symbol with same name.
			{
				return 4447854;
			}
		}

		fn Foo() : i32
		{
			return B::I();   // Must access function B::I, not type A::I
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 4447854 )


def InheritanceTest_ChildClassNameOverridesParentClassName_Test2():
	c_program_text= """
		class A polymorph
		{
			fn I() : i32 { return 0; }
		}
		class B : A
		{
			type I= i32;   // Child class have different kind of symbol with same name.
		}

		fn Foo() : i32
		{
			var B::I r= 658566;   // type B::I must be selected, instead of function A::I.
			return r;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 658566 )


def InheritanceTest_ChildClassNameOverridesParentClassName_Test3():
	c_program_text= """
		class A polymorph
		{
			fn foo() : i32 { return 0; }
		}
		class B : A
		{
			fn foo() : i32 { return 5584; }   // Static function shadows parent class function with exact signature.
		}

		fn Foo() : i32
		{
			return B::foo();  // B::foo must be called
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5584 )


def InheritanceTest_ChildClassNameOverridesParentClassName_Test4():
	c_program_text= """
		class A polymorph
		{
			fn foo( i32 x ) : i32 { return x; }
		}
		class B : A
		{
			fn foo() : i32 { return 0; }   // Function in child class merged with one functions set with parent class functions.
		}

		fn Foo() : i32
		{
			return B::foo( 996544 );  // A::foo must be called
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 996544 )


def InheritanceTest_ChildClassNameOverridesParentClassName_Test5():
	c_program_text= """
		class A polymorph
		{
			f32 x;
			fn constructor() ( x= 0.0f ) {}
		}
		class B : A
		{
			i32 x;
			fn constructor() ( x= 0 ) {}
		}

		fn Foo() : i32
		{
			var B mut b;
			b.x= 66541211; // member B::x must be selected
			return b.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 66541211 )


def InheritanceTest_ParentClassFieldAccess_Test0():
	c_program_text= """
		class A polymorph
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
		class A polymorph
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
		class A polymorph
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
		class A polymorph
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


def InheritanceTest_InitializeBaseClass_Test2():
	c_program_text= """
		class A polymorph
		{
			i32 a;
			fn constructor()( a= 988541 ){}
		}
		class B : A
		{
			// Must generate default constructor, that calls A::constructor
		}

		fn Foo()
		{
			var B b;
			halt if( b.a != 988541 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def InheritanceTest_InitializeBaseClass_Test3():
	c_program_text= """
		class A polymorph
		{
			i32 a;
			fn constructor( i32 x )( a= x ){}
		}
		class B : A
		{
			i32 b;
			fn constructor( i32 x )( base(x), b= a ){}   // Must access parent class field after explicit base initialization.
		}

		fn Foo()
		{
			var B b( 1451 );
			halt if( b.a != 1451 );
			halt if( b.b != 1451 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def InheritanceTest_InitializeBaseClass_Test4():
	c_program_text= """
		class A polymorph
		{
			i32 a;
			fn constructor()( a= 2018 ){}
		}
		class B : A
		{
			i32 b;
			fn constructor()(  b= a ){}   // Must access parent class field after implicit base initialization.
		}

		fn Foo()
		{
			var B b;
			halt if( b.a != 2018 );
			halt if( b.b != 2018 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ChildToParentReferenceCast_Test0():
	c_program_text= """
		class A polymorph{}
		class B : A {}
		fn Bar( A& a ) {}
		fn Foo()
		{
			var B b;
			Bar(b); // Must convert B& to A&. Direct child to parent conversion.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ChildToParentReferenceCast_Test1():
	c_program_text= """
		class A polymorph{}
		class AA interface{}
		class B : A, AA {}
		fn Bar( A& a ) {}
		fn Baz( AA& aa ) {}
		fn Foo()
		{
			var B b;
			// Direct child to parent conversion for class with two parents.
			Bar(b); // Must convert B& to A&.
			Baz(b); // Must convert B& to AA&.
		}
	"""
	tests_lib.build_program( c_program_text, )
	tests_lib.run_function( "_Z3Foov" )


def ChildToParentReferenceCast_Test2():
	c_program_text= """
		class A polymorph{}
		class B : A {}
		class C : B {}
		fn Bar( A& a ) {}
		fn Foo()
		{
			var C c;
			// Undirect child to parent conversion.
			Bar(c); // Must convert B& to A&.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )



def ChildToParentReferenceCast_Test3():
	c_program_text= """
		class A0 interface{}
		class A1 interface{}
		class A interface : A0, A1 {}
		class B0 interface{}
		class B1 interface{}
		class B interface : B0, B1 {}
		class C : A, B {}
		fn BarA0( A0& a0 ) {}
		fn BarA1( A1& a1 ) {}
		fn BarB0( B0& b0 ) {}
		fn BarB1( B1& b1 ) {}
		fn BarA ( A & a  ) {}
		fn BarB ( B & b  ) {}
		fn BarC ( C & c  ) {}
		fn Foo()
		{
			var C c;
			// Undirect child to parent conversion for class with multiple parents.
			BarA0(c);
			BarA1(c);
			BarB0(c);
			BarB1(c);
			BarA (c);
			BarB (c);
			BarC (c);
		}
	"""
	tests_lib.build_program( c_program_text, )
	tests_lib.run_function( "_Z3Foov" )
