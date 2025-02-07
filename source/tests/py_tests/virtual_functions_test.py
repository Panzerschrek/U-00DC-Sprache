from py_tests_common import *


def VirtualFunctionDeclaration_Test0():
	c_program_text= """
		class S polymorph
		{
			fn virtual Foo( this );
		}
	"""
	tests_lib.build_program( c_program_text )


def VirtualFunctionDeclaration_Test1():
	c_program_text= """
		class S abstract
		{
			fn virtual pure Foo( this );
		}
	"""
	tests_lib.build_program( c_program_text )


def VirtualFunctionDeclaration_Test2():
	c_program_text= """
		class T polymorph
		{
			fn virtual Foo( this );
		}
		class S : T
		{
			fn virtual override Foo( this );
		}
	"""
	tests_lib.build_program( c_program_text )


def VirtualFunctionDeclaration_Test3():
	c_program_text= """
		class T polymorph
		{
			fn virtual Foo( this );
		}
		class S : T
		{
			fn virtual final Foo( this );
		}
	"""
	tests_lib.build_program( c_program_text )


def VirtualFunctionCallTest0():
	c_program_text= """
		class T polymorph
		{
			fn virtual Bar( this ) : i32 { return 555; }
		}
		class S : T
		{
			fn virtual override Bar( this ) : i32 { return 666; }
		}

		fn Bar( T& t ) : i32 { return t.Bar(); }   // Must call S::Bar here

		fn Foo() : i32
		{
			var S s;
			return Bar(s);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 666 )


def VirtualFunctionCallTest1():
	c_program_text= """
		class A polymorph
		{
			fn virtual Bar( this ) {}
		}
		class B interface
		{
			fn virtual pure Baz( this ) : i32;
		}
		class C : A, B
		{
			i32 x;
			fn constructor()( x= 777 ) {}
			fn virtual override Bar( this ) {}
			fn virtual override Baz( this ) : i32 { return x; }
		}

		fn Baz( B& b ) : i32 { return b.Baz(); }   // Must call C::Baz here.  Should convert virtual pointer for call.

		fn Foo() : i32
		{
			var C c;
			return Baz(c);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 777 )


def VirtualFunctionCallTest2():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor()( x= 586 ) {}
			fn virtual Bar( this ) : i32 { return x; }
		}
		class B interface
		{
			fn virtual pure Bar( this ) : i32;
		}
		class C : A, B    // Class must inherit A::Bar and use it for B::Bar
		{}

		fn Bar( B& b ) : i32 { return b.Bar(); }   // Must call A::Bar here.

		fn Foo() : i32
		{
			var C c;
			return Bar(c);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 586 )


def VirtualFunctionCallTest3():
	c_program_text= """
		class A polymorph
		{
			fn virtual Bar( this ) : i32 { return 6666; }
		}
		class B : A
		{
			fn virtual override Bar( this ) : i32 { return 65658; }
		}
		fn Bar( B& b ) : i32 { return b.Bar(); }  // Must directly call B::Bar here.
		fn Foo() : i32
		{
			var B b;
			return Bar(b);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 65658 )


def VirtualFunctionCallTest4():
	c_program_text= """
		class A interface
		{
			fn virtual pure Bar( this ) : i32;
		}
		class B interface
		{
			fn virtual pure Bar( this ) : i32;
		}
		class C : A, B
		{
			fn virtual override Bar( this ) : i32 { return 88888; }   // overrides A::Bar and B::Bar
		}

		fn Bar( A& a ) : i32 { return a.Bar(); }
		fn Bar( B& b ) : i32 { return b.Bar(); }
		fn ToA( C& c ) : A& { return c; }
		fn ToB( C& c ) : A& { return c; }

		fn Foo() : i32
		{
			var C c;
			halt if( Bar(ToA(c)) != 88888 );
			halt if( Bar(ToB(c)) != 88888 );
			return c.Bar();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 88888 )


def VirtualFunctionCallTest5():
	c_program_text= """
		class A interface
		{
			fn virtual pure Bar( this ) : i32;
		}
		class B : A
		{
			fn virtual override Bar( this ) : i32 { return 1111144; }
		}
		class C : B
		{
			// Class inherits B::Bar.
		}
		fn Bar( B& b ) : i32 { return b.Bar(); }  // Must directly call B::Bar here.
		fn Foo() : i32
		{
			var C c;
			halt if( c.Bar() != 1111144 ); // And here must call B::Bar
			return Bar(c);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 1111144 )


def VirtualFunctionCallTest6():
	c_program_text= """
		class A interface
		{
			fn virtual pure Bar( this ) : i32;
		}
		class B : A
		{
			fn virtual override Bar( this ) : i32 { return 0; }
		}
		class C : A, B
		{
			// Class contains two virtual tables for "A".
			fn virtual override Bar( this ) : i32 { return 666; }
		}

		fn ToA0( B& b ) : A& { return b; }
		fn ToA1( C& c ) : A& { return ToA0(c); }
		fn Foo() : i32
		{
			var C c;
			var i32 r0= ToA0(c).Bar();
			var i32 r1= ToA1(c).Bar();
			halt if( r0 != r1 );
			return r0;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 666 )


def VirtualFunctionCallTest7():
	c_program_text= """
		class A polymorph
		{
			fn virtual Bar( this ) : i32 { return 123; }
		}
		class B : A
		{
			fn virtual override Bar( this ) : i32 { return 456; }
		}
		fn Foo() : i32
		{
			var B b;
			return A::Bar(b); // Perform non-vitual call of virtual function. Should call "A::Bar" directly, without virtual dispatch.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 123 )


def VirtualFunctionCallTest8():
	c_program_text= """
		class A interface
		{
			fn virtual pure Bar( this ) : i32;
		}
		fn CalLBar(A& a)
		{
			// Call virtual pure function, passing "this" as non-this.
			// In such call no virtual call be performed, but call to A::Bar itself.
			// This code compiles, but linking should fail beause virtual pure function has no definition.
			A::Bar(a);
		}
	"""
	tests_lib.build_program( c_program_text )


def VirtualOperatorCall_Test0():
	c_program_text= """
		class A abstract
		{
			i32 x;
			fn constructor()( x= 0 ){}
			op virtual pure ++( mut this );
		}
		class B : A
		{
			op virtual override ++( mut this ) { ++x; }
		}
		fn Inc( A&mut a ) { ++a; }   // ++operator
		fn Foo() : i32
		{
			var B mut b;
			Inc(b);
			return b.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 1 )


def VirtualOperatorCall_Test1():
	c_program_text= """
		class A abstract
		{
			i32 x;
			fn constructor()( x= 777854 ){}
			op virtual pure ()( this ) : i32;
		}
		class B : A
		{
			op virtual override ()( this ) : i32{ return x; }
		}
		fn Call( A& a ) : i32 { return a(); }   // operator()
		fn Foo() : i32
		{
			var B mut b;
			return Call(b);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 777854 )


def VirtualOperatorCall_Test2():
	c_program_text= """
		class A abstract
		{
			i32 x;
			fn constructor()( x= 33321 ){}
			op virtual pure []( this, i32 mul ) : i32;
		}
		class B : A
		{
			op virtual override []( this, i32 mul ) : i32{ return x * mul; }
		}
		fn DoubleIt( A& a ) : i32 { return a[2]; }   // operator[]
		fn Foo() : i32
		{
			var B mut b;
			return DoubleIt(b);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 33321 * 2 )


def VirtualOperatorCall_Test3():
	c_program_text= """
		class A abstract
		{
			i32 x;
			fn constructor()( x= -8874 ){}
			op virtual pure -( this ) : i32;
		}
		class B : A
		{
			op virtual override -( this ) : i32{ return -x; }
		}
		fn Neg( A& a ) : i32 { return -a; }   // unary minus operator
		fn Foo() : i32
		{
			var B mut b;
			return Neg(b);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 8874 )


def VirtualDestructor_Test0():
	c_program_text= """
		class A polymorph
		{
			fn virtual destructor(){} // Manually declare virtual destructor.
		}
		class B : A
		{
			i32 y;
			fn constructor() ( y= 0 ) {}
			fn virtual override destructor() { y= 666; } // manually override it
		}
		fn Destruct( A&mut a )
		{
			unsafe{ a.destructor(); } // HACK! manully call destructor. hould Call B::destructor.
		}
		fn Foo() : i32
		{
			var B mut b;
			Destruct(b);
			return b.y;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 666 )


def VirtualDestructor_Test1():
	c_program_text= """
		class A polymorph
		{
			// Class have implicit generated destructor/
		}
		class B : A
		{
			i32 y;
			fn constructor() ( y= 0 ) {}
			fn virtual override destructor() { y= 555; } // manually override implicit destructor.
		}
		fn Destruct( A&mut a )
		{
			unsafe{ a.destructor(); } // HACK! manully call destructor. hould Call B::destructor.
		}
		fn Foo() : i32
		{
			var B mut b;
			Destruct(b);
			return b.y;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 555 )


def VirtualDestructor_Test2():
	c_program_text= """
		class A polymorph
		{
			// Class have implicit generated destructor.
		}
		class B : A
		{
			i32 y;
			fn constructor() ( y= 0 ) {}
			fn virtual override destructor() {} // manually override implicit destructor.
		}
		class C : B
		{
			fn virtual override destructor() { y= 555; } // manually override destructor again.
		}

		fn Destruct( A&mut a )
		{
			unsafe{ a.destructor(); } // HACK! manully call destructor. hould Call C::destructor.
		}
		fn Foo() : i32
		{
			var C mut c;
			Destruct(c);
			return c.y;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 555 )


def VirtualDestructor_Test3():
	c_program_text= """
		class A polymorph
		{
			// Class have implicit generated destructor.
		}
		class B : A
		{
			i32 y;
			fn constructor() ( y= 0 ) {}
			fn  destructor() { y= 44422; } // manually override implicit destructor, without using "virtual". Destructor is virtual implicitly.
		}
		fn Destruct( A&mut a )
		{
			unsafe{ a.destructor(); } // HACK! manully call destructor. hould Call B::destructor.
		}
		fn Foo() : i32
		{
			var B mut b;
			Destruct(b);
			return b.y;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 44422 )


def VirtualDestructor_Test4():
	c_program_text= """
		class A polymorph
		{
			fn  destructor() {}  // Destructor is virtual implicitly.
		}
		class B : A
		{
			i32 y;
			fn constructor() ( y= 0 ) {}
			fn  destructor() { y= 111117; } // manually override implicit destructor, without using "virtual". Destructor is virtual implicitly.
		}
		fn Destruct( A&mut a )
		{
			unsafe{ a.destructor(); } // HACK! manully call destructor. hould Call B::destructor.
		}
		fn Foo() : i32
		{
			var B mut b;
			Destruct(b);
			return b.y;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 111117 )


def VirtualCallInConstructor_Test0():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor()( x= 0 ){ x= Foo(); }   // Must call A::Foo here.
			fn virtual Foo(this) : i32 { return 88877; }
		}
		class B : A
		{
			fn virtual override Foo(this) : i32 { return 42; }
		}

		fn Foo() : i32
		{
			var B b;
			halt if( b.Foo() != 42 );  // But after parent class construction, must call B::Foo.
			return b.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 88877 )


def VirtualCallInConstructor_Test1():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo(this) : i32 { return 3335214; }
		}
		class B : A
		{
			i32 x;
			fn constructor()( x= 0 ){ x= Foo(); }  // Must call A::Foo here.
		}

		fn Foo() : i32
		{
			var B b;
			return b.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3335214 )


def VirtualCallInDestructor_Test0():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor()( x= 0 ){}
			fn virtual Foo(this) : i32 { return 88877; }
			fn destructor()
			{
				x= Foo();  // Must call A::Foo. In destructor virtual table pointer must point to current class virtual table.
			}
		}
		class B : A
		{
			fn virtual override Foo(this) : i32 { return 42; }
		}

		fn Foo() : i32
		{
			var B mut b;
			halt if( b.Foo() !=    42 ); // Before destructor call "b" constains virtual table for "B".
			unsafe{ b.destructor(); } // HACK! Manual destructor call.
			halt if( b.Foo() != 88877 ); // After  destructor call "b" constains virtual table for "A".
			return b.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 88877 )


def VirtualForNonclassFunction_Test0():
	c_program_text= """
		fn virtual Foo();
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VirtualForNonclassFunction" )
	assert( errors_list[0].src_loc.line == 2 )


def VirtualForNonThisCallFunction_Test0():
	c_program_text= """
		class S polymorph
		{
			fn virtual Foo();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VirtualForNonThisCallFunction" )
	assert( errors_list[0].src_loc.line == 4 )


def FunctionCanNotBeVirtual_Test0():
	c_program_text= """
		class S polymorph
		{
			fn virtual constructor();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionCanNotBeVirtual" )
	assert( errors_list[0].src_loc.line == 4 )


def VirtualRequired_Test0():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo(this){}
		}
		class B : A
		{
			fn Foo(this){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VirtualRequired" )
	assert( errors_list[0].src_loc.line == 8 )


def OverrideRequired_Test0():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo(this){}
		}
		class B : A
		{
			fn virtual Foo(this){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OverrideRequired" )
	assert( errors_list[0].src_loc.line == 8 )


def OverrideRequired_Test1():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo(this){}
		}
		class B : A
		{
			fn virtual pure Foo(this);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OverrideRequired" )
	assert( errors_list[0].src_loc.line == 8 )


def FunctionDoesNotOverride_Test0():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo(this){}
		}
		class B : A
		{
			fn virtual override Foo( this, i32 x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionDoesNotOverride" )
	assert( errors_list[0].src_loc.line == 8 )


def FunctionDoesNotOverride_Test1():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo(mut this){}
		}
		class B : A
		{
			fn virtual override Foo(imut this);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionDoesNotOverride" )
	assert( errors_list[0].src_loc.line == 8 )


def FunctionDoesNotOverride_Test2():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo(imut this){}
		}
		class B : A
		{
			fn virtual override Foo(mut this);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionDoesNotOverride" )
	assert( errors_list[0].src_loc.line == 8 )


def FunctionDoesNotOverride_Test3():
	c_program_text= """
		class A polymorph
		{
			var [ [ char8, 2 ], 1 ] return_references[ "1_" ];
			fn virtual Foo(this, i32 & x) : i32 & @(return_references);
		}
		class B : A
		{
			fn virtual override Foo(this, i32 & x) : i32 &; // Different return references (both "this" and "x" by-default).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionDoesNotOverride" )
	assert( errors_list[0].src_loc.line == 9 )


def FunctionDoesNotOverride_Test4():
	c_program_text= """
		class A polymorph
		{
			i32& x;
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn virtual Foo(mut this, i32 & x) @(pollution);
		}
		class B : A
		{
			fn virtual override Foo(mut this, i32 & x); // Different references pollution.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionDoesNotOverride" )
	assert( errors_list[0].src_loc.line == 10 )


def OverrideFinalFunction_Test0():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo(this){}
		}
		class B : A
		{
			fn virtual final Foo(this){}
		}
		class C : B
		{
			fn virtual override Foo(this){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OverrideFinalFunction" )
	assert( errors_list[0].src_loc.line == 12 )


def OverrideFinalFunction_Test1():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo(this){}
		}
		class B : A
		{
			fn virtual final Foo(this){}
		}
		class C : B
		{
			fn virtual final Foo(this){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "OverrideFinalFunction" )
	assert( errors_list[0].src_loc.line == 12 )


def FinalForFirstVirtualFunction_Test0():
	c_program_text= """
		class A polymorph
		{
			fn virtual final Foo(this){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FinalForFirstVirtualFunction" )
	assert( errors_list[0].src_loc.line == 4 )


def BodyForPureVirtualFunction_Test0():
	c_program_text= """
		class A interface
		{
			fn virtual pure Foo(this){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "BodyForPureVirtualFunction" )
	assert( errors_list[0].src_loc.line == 4 )


def BodyForPureVirtualFunction_Test1():
	c_program_text= """
		class A interface
		{
			fn virtual pure Foo(this);
		}
		fn A::Foo( this ){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "BodyForPureVirtualFunction" )
	assert( errors_list[0].src_loc.line == 6 )


def VirtualMismatch_Test0():
	c_program_text= """
		class C polymorph
		{
			fn virtual Foo(this){}
		}
		class D : C
		{
			fn virtual override Foo(this);
			fn virtual final Foo(this){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VirtualMismatch" )
	assert( errors_list[0].src_loc.line == 9 )


def VirtualForNonpolymorphClass_Test0():
	c_program_text= """
		class A
		{
			fn virtual Foo(this){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VirtualForNonpolymorphClass" )
	assert( errors_list[0].src_loc.line == 4 )


def ClassContainsPureVirtualFunctions_Test0():
	c_program_text= """
		class A interface
		{
			fn virtual pure Foo(this);
		}
		class B : A  // Must be abstract or interface, if contains pure virtual functions.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ClassContainsPureVirtualFunctions" )
	assert( errors_list[0].src_loc.line == 6 )


def ClassContainsPureVirtualFunctions_Test1():
	c_program_text= """
		class A interface
		{
			fn virtual pure Foo(this);
		}
		class B final : A  // Must be abstract or interface, if contains pure virtual functions.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ClassContainsPureVirtualFunctions" )
	assert( errors_list[0].src_loc.line == 6 )


def ClassContainsPureVirtualFunctions_Test2():
	c_program_text= """
		class A interface
		{
			fn virtual pure Foo(this);
		}
		class B polymorph : A  // Must be abstract or interface, if contains pure virtual functions.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ClassContainsPureVirtualFunctions" )
	assert( errors_list[0].src_loc.line == 6 )


def NonPureVirtualFunctionInInterface_Test0():
	c_program_text= """
		class A interface
		{
			fn virtual Foo(this){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NonPureVirtualFunctionInInterface" )
	assert( errors_list[0].src_loc.line == 2 )


def PureDestructor_Test0():
	c_program_text= """
		class A interface
		{
			fn virtual pure destructor();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "PureDestructor" )
	assert( errors_list[0].src_loc.line == 4 )


def VirtualForFunctionImplementation_Test2():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo(this);
		}
		fn virtual A::Foo(this){} // virtual for function implementation outside class.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VirtualForFunctionImplementation" )
	assert( errors_list[0].src_loc.line == 6 )


def VirtualForPrivateFunction_Test0():
	c_program_text= """
		class A polymorph
		{
		private:
			fn virtual Foo(this);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VirtualForPrivateFunction" )
	assert( errors_list[0].src_loc.line == 5 )


def PointerCastForVirtualCall_Test0():
	c_program_text= """
		class A interface{}
		class B polymorph
		{
			i32 x= 0;
			fn virtual GetX( this ) : i32 { return x; }
		}
		class C interface{}
		class D final : A, B, C {}

		fn GetXB( B& b ) : i32 { return b.GetX(); }
		fn GetXD( D& d ) : i32 { return d.GetX(); }
		fn Foo() : i32
		{
			var D mut d;
			d.x= 654;
			halt if( GetXB(d) != GetXD(d) );
			return d.GetX();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 654 )


def PointerCastForVirtualCall_Test1():
	c_program_text= """
		class A interface{ fn virtual pure GetX( this ) : i32; }
		class B polymorph
		{
			i32 x= 0;
			fn virtual GetX( this ) : i32 { return x; }
		}
		class C interface{ fn virtual pure GetX( this ) : i32; }
		class D final : A, B, C {}

		fn GetXA( A& a ) : i32 { return a.GetX(); }
		fn GetXB( B& b ) : i32 { return b.GetX(); }
		fn GetXC( C& c ) : i32 { return c.GetX(); }
		fn Foo() : i32
		{
			var D mut d;
			d.x= 95257;
			auto x= cast_ref</B/>(d).GetX();
			halt if( x != GetXA(d) );
			halt if( x != GetXB(d) );
			halt if( x != GetXC(d) );
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 95257 )


def PointerCastForVirtualCall_Test2():
	c_program_text= """
		class A interface{ fn virtual pure GetX( this ) : i32; }
		class B polymorph
		{
			i32 x= 0;
			fn virtual GetX( this ) : i32 { return x; }
		}
		class C interface{ fn virtual pure GetX( this ) : i32; }
		class D final : A, B, C
		{
			fn virtual override GetX( this ) : i32 { return x; }
		}

		fn GetXA( A& a ) : i32 { return a.GetX(); }
		fn GetXB( B& b ) : i32 { return b.GetX(); }
		fn GetXC( C& c ) : i32 { return c.GetX(); }
		fn Foo() : i32
		{
			var D mut d;
			d.x= 95474125;
			auto x= d.GetX();
			halt if( x != GetXA(d) );
			halt if( x != GetXB(d) );
			halt if( x != GetXC(d) );
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 95474125 )


def PointerCastForVirtualCall_Test3():
	c_program_text= """
		class A interface{ fn virtual pure GetX( this ) : i32; }
		class B polymorph
		{
			i32 x= 0;
			fn virtual GetX( this ) : i32 { return x; }
		}
		class C interface{ fn virtual pure GetX( this ) : i32; }
		class D : A, B, C
		{
			fn virtual override GetX( this ) : i32 { return x; }
		}
		class E interface {}
		class F : E, D { i32 y= 0; }

		fn GetXA( A& a ) : i32 { return a.GetX(); }
		fn GetXB( B& b ) : i32 { return b.GetX(); }
		fn GetXC( C& c ) : i32 { return c.GetX(); }
		fn GetXD( D& d ) : i32 { return d.GetX(); }
		fn Foo() : i32
		{
			var F mut f;
			f.x= 852;
			auto x= f.GetX();
			halt if( x != GetXA(f) );
			halt if( x != GetXB(f) );
			halt if( x != GetXC(f) );
			halt if( x != GetXD(f) );
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 852 )


def VirtualCall_UsingDifferentVirtualTables_Test0():
	c_program_text= """
		class Interface0 interface
		{
			fn virtual pure Foo0( this, i32 a ) : i32;
			fn virtual pure Foo1( this, i64 b ) : i32;
		}
		class Interface1 interface
		{
			fn virtual pure Bar0( this, f32 c ) : i32;
			fn virtual pure Bar1( this, f64 d ) : i32;
		}
		class MetaInterface interface : Interface0, Interface1{}
		class Impl : MetaInterface
		{
			fn virtual override Foo0( this, i32 a ) : i32 { return 0; }
			fn virtual override Foo1( this, i64 b ) : i32 { return 1; }
			fn virtual override Bar0( this, f32 c ) : i32 { return 2; }
			fn virtual override Bar1( this, f64 d ) : i32 { return 3; }
		}
		class Impl2 : Impl
		{
			fn virtual Baz0( this, char16  c ) : i32 { return 4; }
			fn virtual Baz1( this, char32 e ) : i32 { return 5; }
		}
		fn Foo()
		{
			var Impl2 impl2;
			halt if( impl2.Foo0( 0i32 ) != 0 );
			halt if( impl2.Foo1( 0i64 ) != 1 );
			halt if( impl2.Bar0( 0.0f ) != 2 );
			halt if( impl2.Bar1( 0.0  ) != 3 );
			halt if( impl2.Baz0( 'a'c16 ) != 4 );
			halt if( impl2.Baz1( 'a'c32 ) != 5 );
			var Impl &impl_ref= impl2;
			halt if( impl_ref.Foo0( 0i32 ) != 0 );
			halt if( impl_ref.Foo1( 0i64 ) != 1 );
			halt if( impl_ref.Bar0( 0.0f ) != 2 );
			halt if( impl_ref.Bar1( 0.0  ) != 3 );
			var MetaInterface &meta_ref= impl2;
			halt if( meta_ref.Foo0( 0i32 ) != 0 );
			halt if( meta_ref.Foo1( 0i64 ) != 1 );
			halt if( meta_ref.Bar0( 0.0f ) != 2 );
			halt if( meta_ref.Bar1( 0.0  ) != 3 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VirtualTablePointerSave_InMove_Test0():
	c_program_text= """
		class A polymorph
		{
			fn virtual GetX( this ) : i32 { return 42; }
		}
		class B : A
		{
			fn virtual override GetX( this ) : i32 { return 55541; }
		}
		fn Foo() : i32
		{
			auto b= B(); // Must save virtual table pointer in move initialization of auto variable.
			return b.GetX();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 55541 )
