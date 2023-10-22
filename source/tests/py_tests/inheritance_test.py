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


def InheritanceTest_ParentClassNameVisibleInChild_Test1():
	c_program_text= """
	class A polymorph
	{
		type I= i32;
	}
	class B : A
	{
		fn Foo(I i); // A::I must be visible here
	}
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ParentClassNameVisibleInChild_Test2():
	c_program_text= """
	class A interface
	{
		fn Foo(i32 x);
	}
	class B interface
	{
		fn Foo(f32 x);
	}
	class C : B, A
	{
		// Should inherit both "Foo" variants.
	}
	fn Bar()
	{
		C::Foo( 66 );
		C::Foo( 3.5f );
	}
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ParentClassNameVisibleInChild_Test3():
	c_program_text= """
	class A interface
	{
		fn Foo();
	}
	class B interface : A
	{
		fn Foo(i32 x);
	}
	class C interface : A
	{
		fn Foo(f32 x);
	}
	class D : B, C
	{
		// Should inherit all "Foo" variants.
	}
	fn Bar()
	{
		D::Foo();
		D::Foo( 66 );
		D::Foo( 3.5f );
	}
	"""
	tests_lib.build_program( c_program_text )


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


def InheritanceTest_ChildClassNameOverridesParentClassName_Test6():
	c_program_text= """
		class A polymorph
		{
			fn foo() : i32 { return 58585858; }
		}
		class B : A {}
		class C : B
		{
			fn foo( i32 x ) : i32 { return x; }   // Merge this function into one set with "foo" from undirect ancestor.
		}

		fn Foo() : i32
		{
			return C::foo();  // A::foo must be called
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 58585858 )


def InheritanceTest_ChildClassNameOverridesParentClassName_Test7():
	c_program_text= """
		class A interface
		{
			fn virtual pure Foo(this);
		}
		class B : A
		{
			// Inherit virtual A::Foo two times, override it.
			fn virtual override Foo(this);
		}
		class C : A, B
		{
			// Inherit both B::Foo and A::Foo
		}
		fn Bar(C& c)
		{
			c.Foo(); // Call virtual function.
		}
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_ChildClassNameOverridesParentClassName_Test8():
	c_program_text= """
		class A interface
		{
			fn virtual pure Foo(this);
		}
		class B interface : A {}
		class C interface : A {}
		class D : B, C
		{
			// Inherit virtual A::Foo two times, override it.
			fn virtual override Foo(this);
		}
		fn Bar(D& d)
		{
			d.Foo(); // Call virtual function.
		}
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_TypeTemplateShadowed_Test0():
	c_program_text= """
		class A polymorph
		{
			template</ type T /> struct S{ auto x= 9999; }
		}
		class B : A
		{
			// Template with exact signature shadows template from parent class
			template</ type T /> struct S{ auto x= 1111; }
		}
		static_assert( B::S</ f32 />::x == 1111 );
	"""
	tests_lib.build_program( c_program_text )


def InheritanceTest_TypeTemplateShadowed_Test1():
	c_program_text= """
		class A polymorph
		{
			template</ /> struct S</ u64 /> { auto x= 9999; }
		}
		class B : A
		{
			// Template with different, but less specialized signature. Use both type templates.
			template</ type T /> struct S{ auto x= 1111; }
		}
		static_assert( B::S</ u64 />::x == 9999 );
	"""
	tests_lib.build_program( c_program_text )



def InheritanceTest_TypeTemplateShadowed_Test2():
	c_program_text= """
		class A interface
		{
			template</ type T /> struct S{ auto x= 12345; }
		}
		class B : A{}
		class C : A, B {} // Get here two copies of same type template.
		static_assert( C::S</ f32 />::x == 12345 );
	"""
	tests_lib.build_program( c_program_text )


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
				return i32( f64(a) - f64(b) / f64(c) );  // Access parent fields via NamedOperand.
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


def InheritanceTest_ParentClassFieldAccess_Test2():
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
				return i32( f64(A::a) - f64(B::b) / f64(C::c) );  // Access parent fields via complex NamedOperand.
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


def InheritanceTest_ParentClassFieldAccess_Test3():
	c_program_text= """
		class One polymorph
		{
			i32 a;
			fn constructor()( a= 654 ){}
		}
		class Two : One
		{
			i32 a;
			fn constructor()( a= 321 ){}
		}
		class S : Two
		{
			fn GetA( this ) : i32 { return One::a - Two::a; } // Should access fields of defferent classes
		}

		fn Foo() : i32
		{
			var S c;
			return c.GetA();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 654 - 321 )


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
			fn constructor()( b= a ){}   // Must access parent class field after implicit base initialization.
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


def InheritanceTest_InitializeBaseClass_Test5():
	c_program_text= """
		class A polymorph
		{
			i32 a;
			fn constructor( i32 x )( a= x ){}
		}
		class B : A
		{
			i32 b;
			fn constructor( i32 x )( base(x), b= base.a ){}   // Must access "base" after explicit "base" initialization.
		}

		fn Foo()
		{
			var B b( 77457 );
			halt if( b.a != 77457 );
			halt if( b.b != 77457 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def InheritanceTest_InitializeBaseClass_Test6():
	c_program_text= """
		class A polymorph
		{
			i32 a;
			fn constructor()( a= 66633625 ){}
		}
		class B : A
		{
			i32 b;
			fn constructor()( b= base.a ){}   // Must access "base" after implicit "base" initialization.
		}

		fn Foo()
		{
			var B b;
			halt if( b.a != 66633625 );
			halt if( b.b != 66633625 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def InheritanceTest_BaseReference_Test0():
	c_program_text= """
		class A polymorph
		{
			i32 a;
			fn constructor( i32 in_a )( a= in_a ){}
		}
		class B : A
		{
			i32 a;
			fn constructor( i32 in_base_a, i32 in_a )( base(in_base_a), a= in_a ){}
			fn GetA( this ) : i32
			{
				return a;
			}
			fn GeBasetA( this ) : i32
			{
				return base.a;
			}
		}

		fn Foo()
		{
			var B b( 584, 99965 );
			halt if( b.GetA() != 99965 );
			halt if( b.GeBasetA() != 584 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def Desturctors_ForInheritance_Test0():
	c_program_text= """
		class A polymorph
		{
			i32 &mut a;
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( this, i32 & mut in_a ) @(pollution)
			( a= in_a ){}
			fn destructor()
			{
				a= 0;
			}
		}
		class B : A
		{
			fn constructor( this, i32 & mut in_a ) @(pollution)
			( base(in_a) ){}
			// Must generate default destructor, which calls base destructor.
		}

		fn Foo()
		{
			var i32 mut x= 586;
			{
				var B b( x );  // Destuctor for 'A' (base of 'B') must be called.
			}
			halt if( x != 0 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def Desturctors_ForInheritance_Test1():
	c_program_text= """
		class A polymorph
		{
			i32 &mut a;
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( this, i32 & mut in_a ) @(pollution)
			( a= in_a ){}
			fn destructor()
			{
				a= 0;
			}
		}
		class B : A
		{
			fn constructor( this, i32 & mut in_a ) @(pollution)
			( base(in_a) ){}
			fn destructor()
			{
				// after end of this destructor, destructor for base must be called.
			}
		}

		fn Foo()
		{
			var i32 mut x= 847;
			{
				var B b( x );
			}
			halt if( x != 0 );
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
	tests_lib.build_program( c_program_text )
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


def CopyChildToParent_Test0():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( A &imut other )= default;
		}
		class B : A
		{
			fn constructor( i32 in_x ) ( base(in_x) ) {}
		}
		fn Foo() : i32
		{
			var B b( 5635224 );
			var A a= b; // Copy via expression initializer.
			return a.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5635224 )


def CopyChildToParent_Test1():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( A &imut other )= default;
		}
		class B : A
		{
			fn constructor( i32 in_x ) ( base(in_x) ) {}
		}
		fn Foo() : i32
		{
			var B b( 11241 );
			var A a(b); // Copy via copy constructor call with reference cast.
			return a.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 11241 )


def CopyChildToParent_Test2():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			op=( mut this, A &imut other )= default;
		}
		class B : A
		{
			fn constructor( i32 in_x ) ( base(in_x) ) {}
		}
		fn Foo() : i32
		{
			var A mut a( 0 );
			var B b( 66685 );
			a= b;  // Call copy-assignnment
			return a.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 66685 )


def CopyChildToParent_Test3():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( A &imut other )= default;
		}
		class B : A
		{
			fn constructor( i32 in_x ) ( base(in_x) ) {}
		}
		fn Bar( A a ) : i32 { return a.x; }
		fn Foo() : i32
		{
			var B b( 44758 );
			return Bar(b); // Copy in function call
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 44758 )


def MoveClassWithParent_Test0():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
		}
		class B : A
		{
			fn constructor( i32 in_x ) ( base(in_x) ) {}
		}

		fn Bar( B b ) : i32 { return b.x; }
		fn Foo() : i32
		{
			return Bar( B( 58 ) ); // Move in function call
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 58 )


def MoveClassWithParent_Test1():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
		}
		class B : A
		{
			fn constructor( i32 in_x ) ( base(in_x) ) {}
		}

		fn Foo() : i32
		{
			auto b= B( 66584 ); // Move in aut-variable initialization
			return b.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 66584 )


def MoveClassWithParent_Test2():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
		}
		class B : A
		{
			fn constructor( i32 in_x ) ( base(in_x) ) {}
		}

		fn Foo() : i32
		{
			var B b= B( 965856 ); // Move in variable initialization
			return b.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 965856 )


def MoveClassWithParent_Test3():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
		}
		class B final : A
		{
			fn constructor( i32 in_x ) ( base(in_x) ) {}
		}

		fn Foo() : i32
		{
			var B mut b(0);
			b= B( 11125 ); // Move in assignment
			return b.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 11125 )


def GeneratedCopyConstructor_Test0():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( A &imut other )= default;
		}
		class B : A
		{
			fn constructor( i32 in_x ) ( base(in_x) ) {}
			fn constructor( B &imut other )= default;
		}
		fn Foo() : i32
		{
			var B b( 99965 );
			var B b2= b;
			halt if( b.x != b2.x );
			return b2.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 99965 )


def AbstractClassConstructor_Test0():
	c_program_text= """
		class A abstract
		{
			fn Foo( this ){}
			fn constructor()
			{
				Foo();   // "this" unavailable in constructor of abstrat class, so, we can not here call "thiscall" function.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotSelectOverloadedFunction" )
	assert( errors_list[0].src_loc.line == 7 )


def AbstractClassConstructor_Test1():
	c_program_text= """
		class A abstract
		{
			fn constructor()
			{
				this;   // "this" unavailable in constructor of abstrat class.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ThisUnavailable" )
	assert( errors_list[0].src_loc.line == 6 )


def AbstractClassConstructor_Test2():
	c_program_text= """
		class A abstract
		{
			i32 x;
			fn constructor()
			( x= 0 )
			{
				x= 42; // Ok, can directly access fields.
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def AbstractClassDestructor_Test0():
	c_program_text= """
		class A abstract
		{
			fn destructor()
			{
				this;   // "this" unavailable in destructor of abstract class.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ThisUnavailable" )
	assert( errors_list[0].src_loc.line == 6 )


def AbstractClassDestructor_Test1():
	c_program_text= """
		class A interface
		{
			fn destructor()
			{
				this;   // "this" unavailable in destructor of interface.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ThisUnavailable" )
	assert( errors_list[0].src_loc.line == 6 )


def AbstractClassDestructor_Test2():
	c_program_text= """
		fn Foo( A& a ){}
		class A abstract
		{
			i32 x;
			fn constructor()( x= 0 ){}
			fn destructor()
			{
				x= 42; // Ok, can directly access fields.
			}
		}
	"""
	tests_lib.build_program( c_program_text )
