from py_tests_common import *


def LockTempsDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			auto lock_temps x= 0;
		}
	"""
	tests_lib.build_program( c_program_text )


def LockTempsDeclaration_Test1():
	c_program_text= """
		fn Foo()
		{
			auto lock_temps mut x= 0;
		}
	"""
	tests_lib.build_program( c_program_text )


def LockTempsDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			auto lock_temps & x= Pass(666); // Ok, "temp 666" locked.
		}
		fn Pass( i32& x ) : i32& { return x; }
	"""
	tests_lib.build_program( c_program_text )


def LockTempsDeclaration_Test3():
	c_program_text= """
	fn Foo()
	{
		auto lock_temps &imut x= Pass(8888); // Ok, "temp 888" locked.
	}
	fn Pass( i32& x ) : i32& { return x; }
	"""
	tests_lib.build_program( c_program_text )


def AutoLockTemps_Test0():
	c_program_text= """
	fn Foo()
	{
		// Lock temp numeric constant.
		auto lock_temps& x= Pass(0);
	}
	fn Pass( i32& x ) : i32& { return x; }
	"""
	tests_lib.build_program( c_program_text )


def AutoLockTemps_Test1():
	c_program_text= """
	fn Foo()
	{
		// Lock temp function result, passed through function.
		auto lock_temps& x= Pass( Add( 54, 71 ) );
	}
	fn Add( i32 x, i32 y ) : i32 { return x + y; }
	fn Pass( i32& x ) : i32& { return x; }
	"""
	tests_lib.build_program( c_program_text )


def AutoLockTemps_Test2():
	c_program_text= """
	fn Foo()
	{
		// Lock more, thhen one temp variable.
		auto lock_temps& x= Select( true, -74, 85647 );
	}
	fn Select( bool first, i32& x, i32& y ) : i32&
	{
		if( first ) { return x; }
		return y;
	}
	"""
	tests_lib.build_program( c_program_text )


def AutoLockTemps_Test3():
	c_program_text= """
	struct S
	{
		i32& mut x;
		fn constructor( this'a', i32 &'i mut in_x ) ' a <- mut i '
		( x= in_x) {}
		fn destructor() { x= 0; }
	}
	fn Foo()
	{
		var i32 mut x= 66;
		{
			auto lock_temps& s= Pass(S(x));
			halt if( x != 66 );
		} // "temp S" must be destroyed here.
		halt if( x != 0 );
	}
	fn Pass( S& s ) : S& { return s; }
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AutoLockTemps_Test4():
	c_program_text= """
	struct S
	{
		i32& x;
	}
	fn CreateS( i32&'r x ) : S'r'
	{
		var S s{ .x= x };
		return s;
	}
	fn Foo()
	{
		// Lock reference to temp numeric constant inside value.
		auto lock_temps ref= CreateS( 44 );
	}
	"""
	tests_lib.build_program( c_program_text )


def AutoLockTemps_Test5():
	c_program_text= """
	struct S
	{
		i32& x;
	}
	fn CreateS( i32&'r x ) : S'r'
	{
		var S s{ .x= x };
		return s;
	}
	fn Pass( S& s ) : S& { return s; }
	fn Foo()
	{
		// Lock 'temp S' and reference to temp numeric constant inside 'temp S'.
		auto lock_temps& ref= Pass( CreateS( 44 ) );
	}
	"""
	tests_lib.build_program( c_program_text )


def AutoLockTemps_Test6():
	c_program_text= """
	fn Foo()
	{
		// Can directly bind value to reference, if reference uses "lock_temps"
		auto lock_temps& ref= 42;
	}
	"""
	tests_lib.build_program( c_program_text )


def AutoLockTemps_Test7():
	c_program_text= """
	fn Foo()
	{
		// Can directly bind value to mutable reference, if reference uses "lock_temps"
		auto lock_temps&mut num= 42;
		++num;
		halt if( num != 43 );
	}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
