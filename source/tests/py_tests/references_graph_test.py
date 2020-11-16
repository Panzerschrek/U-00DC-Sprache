from py_tests_common import *


def SimpleProgramTest():
	c_program_text= """
		fn Add( i32 a, i32 b ) : i32
		{
			return a + b;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Addii", 95, -5 )
	assert( call_result == 95 - 5 )


def LocalVariableTest():
	c_program_text= """
		fn Sub( i32 a, i32 b ) : i32
		{
			var i32 mut x= zero_init;
			x= a - b;
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Subii", 654, 112 )
	assert( call_result == 654 - 112 )


def CreateMutableReferenceToVariableWithImmutableReference_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= zero_init;
			auto& imut ri= x;
			auto& mut  rm= x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 6 )


def CreateMutableReferenceToVariableWithImmutableReference_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= zero_init;
			var i32 &imut ri= x;
			var i32 &mut  rm= x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 6 )


def CreateMutableReferenceToVariableWithMutableReference():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			var i32 &mut r0= x;
			var i32 &mut r1= x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 6 )


def CreateMutableReferenceToAnotherMutableReference_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			var i32 &mut r0= x;
			var i32 &mut r1= r0; // ok
		}
	"""
	tests_lib.build_program( c_program_text )


def CreateMutableReferenceToAnotherMutableReference_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			var i32 &mut r0= x;
			var i32 &mut r1= r0; // ok
			var i32 &mut r2= r0; // Not ok
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def AccessingVariableThatHaveMutableReference_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			var i32 &mut r0= x;
			var i32 &mut r1= r0;
			var i32& r2= r0; // error, accessing 'r0', that have reference 'r1'
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def AccessingVariableThatHaveMutableReference_Test2():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			var i32 &mut r= x;
			var i32 x_copy= r; //ok, accessing end reference
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionReferencePass_Test0():
	c_program_text= """
		fn Pass( i32& x ) : i32& { return x; }
		fn Foo()
		{
			auto mut x= 0;
			auto &imut ri= Pass(x);
			auto &mut rm= x; // Error, 'x' have reference already.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def FunctionReferencePass_Test1():
	c_program_text= """
		fn PassFirst( i32&'f x, i32&'s y ) : i32&'f { return x; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			auto &imut ri= PassFirst( x, y );
			auto &mut rm= y; // Ok, 'y' have no references.
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionReferencePass_Test2():
	c_program_text= """
		fn Pass( i32&mut x ) : i32&mut { return x; }
		fn Foo()
		{
			auto mut x= 0;
			auto &mut r0= Pass(x);
			auto& x_ref= x; // Error, taking reference to 'x', which have mutable rerefernce.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def ReferenceInsideStruct_Test0():
	c_program_text= """
		struct S{ i32 &imut r; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .r= x };
			++x; // Error, 'x' have reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def ReferenceInsideStruct_Test1():
	c_program_text= """
		struct S{ i32 &mut r; }
		fn Foo()
		{
			var i32 mut x= 0;
			auto &imut r= x;
			var S s{ .r= x }; // Error, creating mutable reference to variable, that have reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def ReturnReferenceInsideStruct_Test0():
	c_program_text= """
		struct S{ i32 &imut r; }
		fn GetS( i32 &'a imut x ) : S'a'
		{
			var S s{ .r= x };
			return s;
		}
		fn Foo()
		{
			var i32 mut x= 0;
			auto &imut r= GetS(x).r;
			++x; // Error, reference to 'x' exists inside 'r'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 12 )


def ReturnReferenceFromStruct_Test0():
	c_program_text= """
		struct S{ i32 &mut r; }
		fn GetR( S& s'x' ) : i32 &'x mut
		{
			return s.r;
		}
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .r= x };
			var i32 &mut x_ref= GetR( s );
			++s.r; // Error, reference to 'x' exists inside 'r'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 12 )


def ReturnReferenceInsideStruct_Test0():
	c_program_text= """
		struct S{ i32& x; }
		fn GetS( i32&'r x ) : S'r'
		{
			var S s{ .x= x };
			return s;
		}
		fn Foo()
		{
			var i32 mut x= 0;
			auto s= GetS(x);
			++x; // Error, reference to 'x' exists inside 's'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 12 )


def PollutionTest0():
	c_program_text= """
		struct S{ i32& x; }
		fn Pollution( S &mut s'dst', i32&'src x ) ' dst <- src' {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			Pollution( s, y );
			++y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 9 )


def PollutionTest1():
	c_program_text= """
		struct S{ i32& x; }
		fn Pollution( S &mut s'dst', i32&'src x ) ' dst <- src' {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			auto& mut s_ref= s;
			Pollution( s_ref, y );
			++y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 10 )


def PollutionTest2():
	c_program_text= """
		struct S{ i32& x; }
		fn Pollution( S &mut a'dst', S& b'src' ) ' dst <- src' {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut a{ .x= x };
			{
				var S mut b{ .x= y };
				Pollution( a, b ); // Now, 'a', contains reference to 'y'
			}
			++y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 12 )


def ReferenceFieldAccess_Test0():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			auto& s_ref= s;
			auto& x_ref= s_ref.x; // Accessing reference field of struct 's', using reference to struct 's'
			++s.x; // Error, reference to 'x' inside 's' is not unique.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 9 )


def PassMutableReferenceTwoTimes_Tes0():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S& s, i32 &mut i ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			Bar( s, s.x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 8 )


def PassMutableReferenceTwoTimes_Tes1():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S s, i32 &mut i ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			Bar( s, s.x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 8 )


def PassMutableReferenceTwoTimes_Tes2():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S& a, S& b ) { ++a.x; ++b.x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			Bar( s, s );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 8 )


def PassMutableReferenceTwoTimes_Tes3():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S a, S b ) { ++a.x; ++b.x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			Bar( s, s );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 8 )


def PassMutableReferenceTwoTimes_Tes4():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S& a, S& b ) { ++a.x; ++b.x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			auto& s0= s;
			auto& s1= s;
			Bar( s0, s1 );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 10 )


def PassMutableReferenceTwoTimes_Tes5():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S a, S b ) { ++a.x; ++b.x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			auto& s0= s;
			auto& s1= s;
			Bar( s0, s1 );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 10 )


def PassMutableReferenceTwoTimes_Tes6():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S& a, S& b ) { ++a.x; ++b.x; }
		fn Foo( S& s )
		{
			Bar( s, s );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 6 )


def PassImmutableReferenceTwoTimes_Test0():
	c_program_text= """
		struct S{ i32 & x; }
		fn Bar( S& a, S& b ) { }
		fn Foo( S& s )
		{
			Bar( s, s );
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferencesLoop_Test0():
	c_program_text= """
		struct S{ i32 & x; }
		fn DoPollution( S& mut s0'a', S &imut s1'b' ) ' a <- b ';
		fn Foo()
		{
			var i32 x= 0;
			var S mut s{ .x= x };
			auto s_copy = s;
			DoPollution( s, s_copy );
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferencesLoop_Test1():
	c_program_text= """
		struct S
		{
			i32 & x;
			op=( mut this, S &imut other ) {} // Does reference pollution
		}
		fn Foo()
		{
			var i32 x= 0;
			var S mut s{ .x= x };
			auto s_copy = s;
			s= s_copy;
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferencesLoop_Test3():
	c_program_text= """
		struct S{ i32 & x; }
		fn DoPollution( S& mut s0'a', i32 &'b x ) ' a <- b ';
		fn Foo()
		{
			var i32 x= 0;
			var S mut s{ .x= x };
			auto s_copy = s;
			DoPollution( s, s_copy.x );
		}
	"""
	tests_lib.build_program( c_program_text )
