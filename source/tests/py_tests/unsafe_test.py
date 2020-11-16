from py_tests_common import *


def UnsafeBlockDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			unsafe{}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnsafeFunctionDeclaration_Test0():
	c_program_text= """
		fn Foo() unsafe;
	"""
	tests_lib.build_program( c_program_text )


def UnsafeFunctionDeclaration_Test1():
	c_program_text= """
		fn Foo() unsafe : i32;
	"""
	tests_lib.build_program( c_program_text )


def UnsafeFunctionCallOutsideUnsafeBlock_Test0():
	c_program_text= """
		fn Bar() unsafe;
		fn Foo()
		{
			Bar(); // Regular function call
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 5 )


def UnsafeFunctionCallOutsideUnsafeBlock_Test1():
	c_program_text= """
		fn Bar( i32 &imut x );
		fn Bar( i32 & mut x ) unsafe;
		fn Foo()
		{
			Bar(42); // Ok, safe function selected.
		}
	"""
	tests_lib.build_program( c_program_text )


def UnsafeFunctionCallOutsideUnsafeBlock_Test2():
	c_program_text= """
		fn Bar( i32 &imut x );
		fn Bar( i32 & mut x ) unsafe;
		fn Foo()
		{
			var i32 mut x= 0;
			Bar(x); // Error, unsafe function selected.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 7 )


def UnsafeFunctionCallOutsideUnsafeBlock_Test3():
	c_program_text= """
		struct S
		{
			fn constructor() unsafe {}
		}
		fn Foo()
		{
			var S s; // Error, implicitly calling unsafe constructor
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 8 )


def UnsafeFunctionCallOutsideUnsafeBlock_Test4():
	c_program_text= """
		struct S
		{
			fn constructor( i32 x ) unsafe {}
		}
		fn Foo()
		{
			var S s( 666 ); // Error, explicitly calling unsafe constructor
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 8 )


def UnsafeFunctionCallOutsideUnsafeBlock_Test5():
	c_program_text= """
		struct S
		{
			fn constructor( S& other ) unsafe {}  // unsafe copy constructor
		}
		fn Foo()
		{
			var S s0;
			var S s1= s0; // Error, calling unsafe copy-constructor
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 9 )


def UnsafeFunctionCallOutsideUnsafeBlock_Test6():
	c_program_text= """
		struct S
		{
			op= ( mut this, S& other ) unsafe {}
		}
		fn Foo()
		{
			var S s0, mut s1;
			s1= s0; // Error, calling unsafe copy assignment operator
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 9 )


def UnsafeFunctionCallOutsideUnsafeBlock_Test7():
	c_program_text= """
		struct S
		{
			op++( mut this ) unsafe {}
		}
		fn Foo()
		{
			var S mut s;
			++s;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 9 )


def UnsafeFunctionCallOutsideUnsafeBlock_Test8():
	c_program_text= """
		struct S
		{
			op[]( mut this, i32 x ) unsafe {}
		}
		fn Foo()
		{
			var S mut s;
			s[0];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 9 )


def UnsafeFunctionCallOutsideUnsafeBlock_Test9():
	c_program_text= """
		struct S
		{
			op()( this ) unsafe {}
		}
		fn Foo()
		{
			var S mut s;
			s();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 9 )


def UnsafeFunctionCallOutsideUnsafeBlock_Test10():
	c_program_text= """
		fn Bar() unsafe;
		fn Foo() unsafe
		{
			Bar();  // Even in unsafe function we needs unsafe block to call other unsafe functions.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 5 )


def UnsafeFunctionCallOutsideUnsafeBlock_Test11():
	c_program_text= """
		struct S
		{
			fn destructor() unsafe {}
		}
		fn Foo()
		{
			var S s;
		} // Error, calling unsafe destructor here
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 9 )


def UnsafeFunctionCallOutsideUnsafeBlock_Test12():
	c_program_text= """
		struct S
		{
			fn destructor() unsafe {}
		}
		struct B  // Error, while generating default-destructor. Currently, classes with unsafe destructor can not be members of other classes.
		{
			S s;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 6 )


def UnsafeFunctionCallInsideUnsafeBlock_Test0():
	c_program_text= """
		fn Bar() unsafe;
		fn Foo()
		{
			unsafe
			{
				Bar(); // Ok, we are inside unsafe block
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnsafeFunctionCallInsideUnsafeBlock_Test1():
	c_program_text= """
		struct S
		{
			fn constructor() unsafe {}
		}
		fn Foo()
		{
			unsafe
			{
				var S s; // Ok, implicitly call here unsafe constructor
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnsafeFunctionCallInsideUnsafeBlock_Test2():
	c_program_text= """
		fn Bar() unsafe;
		fn Foo()
		{
			unsafe
			{
				{
					Bar(); // Ok, we are inside unsafe block
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnsafeFunctionCallInsideUnsafeBlock_Test3():
	c_program_text= """
		fn Bar() unsafe;
		fn Foo()
		{
			unsafe
			{
				if(true)
				{
					Bar(); // Ok, we are inside unsafe block
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnsafeFunctionCallInsideUnsafeBlock_Test4():
	c_program_text= """
		fn Bar() unsafe;
		fn Foo()
		{
			unsafe
			{
				while(true)
				{
					Bar(); // Ok, we are inside unsafe block
					break;
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def UnsafeFunctionCallInsideUnsafeBlock_Test5():
	c_program_text= """
		struct S { fn destructor() unsafe {} }
		fn Foo()
		{
			unsafe
			{
				var S s;
			} // Ok, call unsafe destructor at end of unsafe block.
		}
	"""
	tests_lib.build_program( c_program_text )


def CouldNotOverloadFunction_ForUnsafe_Test0():
	c_program_text= """
		fn Foo();
		fn Foo() unsafe;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotOverloadFunction" )
	assert( errors_list[0].src_loc.line == 2 or errors_list[0].src_loc.line == 3 )


def CouldNotOverloadFunction_ForUnsafe_Test1():
	c_program_text= """
		fn Foo() unsafe;
		fn Foo() {}   // Trying to create body without 'unsafe'
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CouldNotOverloadFunction" )
	assert( errors_list[0].src_loc.line == 2 or errors_list[0].src_loc.line == 3 )


def FunctionDoesNotOverride_ForUnsafe_Test0():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo( this );
		}

		class B : A
		{
			fn virtual override Foo( this ) unsafe;  // 'unsafe' breaks 'override' here
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionDoesNotOverride" )
	assert( errors_list[0].src_loc.line == 9 )


def ExplicitAccessToSpecialMethodsIsUnsafe_Test0():
	c_program_text= """
		struct S {} // have generated default-constructor
		fn Foo()
		{
			var S mut s;
			s.constructor;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExplicitAccessToThisMethodIsUnsafe" )
	assert( errors_list[0].src_loc.line == 6 )


def ExplicitAccessToSpecialMethodsIsUnsafe_Test1():
	c_program_text= """
		struct S {} // have generated default-constructor
		fn Foo()
		{
			S::constructor;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExplicitAccessToThisMethodIsUnsafe" )
	assert( errors_list[0].src_loc.line == 5 )


def ExplicitAccessToSpecialMethodsIsUnsafe_Test2():
	c_program_text= """
		struct S {  fn destructor(){}  }
		fn Foo()
		{
			var S mut s;
			s.destructor();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExplicitAccessToThisMethodIsUnsafe" )
	assert( errors_list[0].src_loc.line == 6 )


def ExplicitAccessToSpecialMethodsIsUnsafe_Test3():
	c_program_text= """
		struct S {  fn destructor(){}  }
		fn Foo()
		{
			::S::destructor;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExplicitAccessToThisMethodIsUnsafe" )
	assert( errors_list[0].src_loc.line == 5 )


def ExplicitAccessToSpecialMethodsIsUnsafe_Test4():
	c_program_text= """
		struct S {  fn constructor( i32 x ){}  }
		fn Foo()
		{
			var S mut s(0);
			unsafe{  s.constructor(42);  }   // ok, can access constructor in unsafe block
		}
	"""
	tests_lib.build_program( c_program_text )


def ExplicitAccessToSpecialMethodsIsUnsafe_Test5():
	c_program_text= """
		struct S {  fn destructor(){} }
		fn Foo()
		{
			var S mut s;
			unsafe{  s.destructor();  }   // ok, can access destructor in unsafe block
		}
	"""
	tests_lib.build_program( c_program_text )


def SafeBlockResetsUnsafe_Test():
	c_program_text= """
		fn Bar() unsafe;
		fn Foo()
		{
			unsafe
			{
				safe
				{
					Bar();
				}
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeFunctionCallOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 9 )


def UnsafeInsideUnsafe_Test():
	c_program_text= """
		fn Bar() unsafe;
		fn Foo()
		{
			unsafe
			{
				{
					unsafe
					{
						Bar();
					}
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
