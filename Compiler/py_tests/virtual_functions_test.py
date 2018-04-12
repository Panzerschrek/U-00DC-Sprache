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
			a.destructor(); // HACK! manully call destructor. hould Call B::destructor.
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
			a.destructor(); // HACK! manully call destructor. hould Call B::destructor.
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
			a.destructor(); // HACK! manully call destructor. hould Call C::destructor.
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
			a.destructor(); // HACK! manully call destructor. hould Call B::destructor.
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
			a.destructor(); // HACK! manully call destructor. hould Call B::destructor.
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
			b.destructor(); // HACK! Manual destructor call.
			halt if( b.Foo() != 42 );  // But after parent class construction, must call B::Foo.
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
	assert( errors_list[0].file_pos.line == 2 )


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
	assert( errors_list[0].file_pos.line == 4 )


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
	assert( errors_list[0].file_pos.line == 4 )


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
	assert( errors_list[0].file_pos.line == 8 )


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
	assert( errors_list[0].file_pos.line == 8 )


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
	assert( errors_list[0].file_pos.line == 8 )


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
	assert( errors_list[0].file_pos.line == 8 )


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
	assert( errors_list[0].file_pos.line == 12 )


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
	assert( errors_list[0].file_pos.line == 12 )


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
	assert( errors_list[0].file_pos.line == 4 )


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
	assert( errors_list[0].file_pos.line == 4 )


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
	assert( errors_list[0].error_code == "FunctionBodyDuplication" )
	assert( errors_list[0].file_pos.line == 6 )


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
	assert( errors_list[0].file_pos.line == 4 )


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
	assert( errors_list[0].file_pos.line == 7 )


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
	assert( errors_list[0].file_pos.line == 7 )


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
	assert( errors_list[0].file_pos.line == 7 )


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
	assert( errors_list[0].file_pos.line == 3 )
