from py_tests_common import *


def StructFieldChildNodes_Test0():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			// Ok - create mutable references to different fields of same struct.
			var i32 &mut x= s.x;
			var f32 &mut y= s.y;
			++x;
			y *= 2.0f;
			halt if( x != 1 );
			halt if( y != 2.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructFieldChildNodes_Test1():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 5, .y= 7.0f };
			{
				// Ok - create mutable references to different fields of same struct, accessed by reference.
				auto &mut s_ref= s;
				var i32 &mut x= s_ref.x;
				var f32 &mut y= s_ref.y;
				++x;
				y *= 2.0f;
			}
			halt if( s.x != 6 );
			halt if( s.y != 14.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructFieldChildNodes_Test2():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			var i32 &mut x= s.x;
			var i32 &mut x_again= s.x; // Error - creating second mutable reference to same field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			var i32 &mut x= s.x;
			auto & s_whole= s; // Error - creating reference to whole struct with existing mutable reference to its field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test4():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			var i32 & x= s.x;
			var S &mut s_whole= s; // Error - creating mutable reference to whole struct with existing reference to its field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test5():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			auto &mut s_whole= s;
			auto & x= s.x; // Error - creating reference to struct field with existing mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test6():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			auto & s_whole= s;
			auto &mut x= s.x; // Error - creating mutable reference to struct field with existing reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test7():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			auto & s_whole= s;
			auto & x= s.x; // Ok - creating reference to struct field for struct with existing reference.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test8():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			auto & x= s.x;
			auto & s_whole= s; // Ok - creating reference to struct for struct with existing reference to field.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test9():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( i32& mut x, f32 &mut y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s.x, s.y ); // Ok - passing to function two mutable references to different fields of same struct.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test10():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( f32& mut x, f32 &mut y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s.y, s.y ); // Error - passing to function two mutable references to same field of same struct.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test11():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( i32& mut x, S& mut s ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s.x, s ); // Error - passing to function mutable reference to struct field and mutable reference to whole struct.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test12():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( S& mut s, f32& mut y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s, s.y ); // Error - passing to function mutable reference to whole struct and mutable reference to struct field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test13():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( S& s, f32& mut y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s, s.y ); // Error - passing to function reference to whole struct and mutable reference to struct field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test14():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( S &mut s, f32& y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s, s.y ); // Error - passing to function mutable reference to whole struct and reference to struct field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test15():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( S & s, f32& y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s, s.y ); // Ok - passing to function reference to whole struct and reference to struct field.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test16():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( i32& x, S& s ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s.x, s ); // Ok - passing to function reference to struct field and reference to whole struct.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test17():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( i32& x, f32& y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s.x, s.y ); // Ok - passing to function references to two struct fields.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test18():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( i32& x, i32& y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s.x, s.x ); // Ok - passing to function two references to same struct field.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test19():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ S s; bool z; }
		fn Foo()
		{
			var T mut t{ .s{ .x= 0, .y= 1.0f }, .z= false };
			{
				// Ok - create mutable references to different fields of same struct.
				var i32 &mut x= t.s.x;
				var f32 &mut y= t.s.y;
				var bool &mut z= t.z;
				x= 16;
				y= 45.3f;
				z= false;
			}
			halt if( t.s.x != 16 );
			halt if( t.s.y != 45.3f );
			halt if( t.z != false );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructFieldChildNodes_Test20():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ S s; bool z; }
		fn Foo()
		{
			var T mut t{ .s{ .x= 0, .y= 1.0f }, .z= false };
			{
				// Ok - create mutable references to different fields of same struct.
				var S &mut s= t.s;
				var i32 &mut x= s.x;
				var f32 &mut y= s.y;
				var bool &mut z= t.z;
				x= 16;
				y= 45.3f;
				z= false;
			}
			halt if( t.s.x != 16 );
			halt if( t.s.y != 45.3f );
			halt if( t.z != false );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructFieldChildNodes_Test21():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ S s; bool z; }
		fn Foo()
		{
			var T mut t{ .s{ .x= 0, .y= 1.0f }, .z= false };
			var S &mut s= t.s;
			var i32 &mut x= t.s.x; // Error - creating mutable reference to "t.s" which already has one mutable reference.

		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test22():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ S s; bool z; }
		fn Foo()
		{
			var T mut t{ .s{ .x= 0, .y= 1.0f }, .z= false };
			var S & s= t.s;
			var i32 &mut x= t.s.x; // Error - creating mutable reference to "t.s" which already has one reference.

		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test23():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ S s; bool z; }
		fn Foo()
		{
			var T mut t{ .s{ .x= 0, .y= 1.0f }, .z= false };
			var S &mut s= t.s;
			var i32 & x= t.s.x; // Error - creating reference to "t.s" which already has one mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test24():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ S s; bool z; }
		fn Foo()
		{
			var T mut t{ .s{ .x= 0, .y= 1.0f }, .z= false };
			var S &mut s= t.s;
			var i32 &mut x= s.x; // Ok - creating reference, linked to reference "s".
			var f32 &mut y= s.y; // Ok - creating reference, linked to reference "s".
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test25():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				// Ok - creating two mutable references to "this".
				var i32 &mut x_ref= x;
				var f32 &mut y_ref= y;
				x_ref= 0;
				y_ref= 0.0f;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test26():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				var i32 &mut x_ref_again= x; // Error - creating second reference to same "this" field.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test27():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				Bar(); // Error - creating mutable reference to "this" in call to thiscall method "Bar", when mutable reference to one of "this" fields exists.
			}
			fn Bar( mut this ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test28():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var f32 &imut y_ref= y;
				Bar(); // Error - creating mutable reference to "this" in call to thiscall method "Bar", when reference to one of "this" fields exists.
			}
			fn Bar( mut this ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test29():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var f32 &mut y_ref= y;
				Bar(); // Error - creating reference to "this" in call to thiscall method "Bar", when mutable reference to one of "this" fields exists.
			}
			fn Bar( this ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test30():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 & x_ref= x;
				Bar(); // Ok - calling method with immutable "this" when immutable reference to one of fields exists.
			}
			fn Bar( this ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test31():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				this.y /= 1.0f; // Ok, modifying field of "this" using explicit "this", when mutable reference to other field exists.
			}
			fn Bar( this ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test32():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				y /= 1.0f; // Ok, modifying field of "this" using implicit "this", when mutable reference to other field exists.
			}
			fn Bar( this ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test33():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				this.x /= 1; // Error - modifying field of "this" using explicit "this", when mutable reference to same field exists.
			}
			fn Bar( this ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test34():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				x /= 1; // Error - modifying field of "this" using implicit "this", when mutable reference to same field exists.
			}
			fn Bar( this ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test35():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				auto &this_ref= this;
				x /= 1; // Error - modifying field of "this" using implicit "this", when mutable reference to whole "this".
			}
			fn Bar( this ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test36():
	c_program_text= """
	struct S { i32 x; f32 y; }
	fn Pass( i32 &mut x ) : i32 &mut { return x; }
	fn Foo()
	{
		var S mut s{ .x= 0, .y= 1.0f };
		var i32& mut x= Pass( s.x ); // Ok, pass mutable reference to field "x". Result reference will point only to field node.
		var f32 &mut y= s.y; // Ok create reference to anouther field.
	}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test37():
	c_program_text= """
	struct S { i32 x; f32 y; }
	fn Pass( i32 &mut x ) : i32 &mut { return x; }
	fn Foo()
	{
		var S mut s{ .x= 0, .y= 1.0f };
		var i32& mut x= Pass( s.x ); // Ok, pass reference to field "x". Result reference will point only to field node.
		var i32 &mut x_again= s.x; // Error - create mutable reference to same field.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test38():
	c_program_text= """
	struct S { i32 x; f32 y; }
	fn Foo()
	{
		var S mut s{ .x= 0, .y= 1.0f };
		var i32& mut x= s.x;
		cast_imut(s); // Error, create intermediate immutable reference to whole "s" while mutable reference to "s.x" exists.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def StructFieldChildNodes_Test39():
	c_program_text= """
	struct S { i32 x; i32 y; i32 z; }
	fn Foo( S& mut s, bool condition )
	{
		var i32 &mut ref= ( condition ? s.x : s.y ); // Create reference to boths fields "s.x" and "s.y" (acutally not, but logically create.
		var i32& mut z_ref= s.z; // Ok, create reference to third field.
	}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test40():
	c_program_text= """
	struct S { i32 x; i32 y; }
	fn Foo( S& mut s, bool condition )
	{
		var i32 &mut ref= ( condition ? s.x : s.y ); // Create reference to boths fields "s.x" and "s.y" (acutally not, but logically create.
		var i32& mut x_ref= s.x; // Error, create reference to "s.x" again.
		var i32& mut y_ref= s.y; // Error, create reference to "s.y" again.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 2 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def StructFieldChildNodes_Test41():
	c_program_text= """
	struct S { i32 x; i32 y; }
	fn Foo( S& mut s, bool condition )
	{
		var i32 &mut ref= ( condition ? s.x : s.y ); // Create reference to boths fields "s.x" and "s.y" (acutally not, but logically create.
		var S& s_ref= s; // Error, create reference to whole "s".
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def StructFieldChildNodes_Test42():
	c_program_text= """
	class A abstract
	{
		i32 x;
		fn virtual pure Bar( mut this );
	}
	class B : A
	{
		f32 y;
		fn Foo( mut this )
		{
			auto &mut y_ref= y;
			// Error - calling a virtual method, using "base" when a mutable reference to one of "this" fields exists.
			// This is an error because whole "this" may be still accessible via virtual call.
			base.Bar();
		}
		fn virtual override Bar( mut this ) { y= 0.0f; }
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 15 ) )


def StructFieldChildNodes_Test43():
	c_program_text= """
	class A polymorph { i32 x; }
	class B : A
	{
		f32 y;
		fn Foo( mut this )
		{
			auto &mut y_ref= y;
			// Create a reference to "base" when a mutable reference to one of "this" fields exists.
			auto& b= base;
		}
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 10 ) )


def StructFieldChildNodes_Test44():
	c_program_text= """
	class A polymorph { i32 x; }
	class B : A
	{
		f32 y;
		fn Foo( mut this )
		{
			auto & y_ref= y;
			// Create a mutable reference to "base" when a reference to one of "this" fields exists.
			auto &mut b= base;
		}
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 10 ) )


def StructFieldChildNodes_Test45():
	c_program_text= """
	class A abstract
	{
		i32 x;
		fn virtual pure Bar( this );
	}
	class B : A
	{
		f32 y;
		fn Foo( this )
		{
			auto & y_ref= y;
			base.Bar(); // Ok - calling immutable virtual method via "base" when a reference to one of "this" fields exists.
		}
		fn virtual override Bar( this ) {}
	}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test46():
	c_program_text= """
	class A interface {}
	class B : A
	{
		f32 y;
		fn Foo( mut this )
		{
			auto & y_ref= y;
			cast_ref</A/>(this); // Error - reference cast produces a reference linked with whole "this" when a reference to one of "this" fields exists.
		}
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


def StructFieldChildNodes_Test47():
	c_program_text= """
	class A interface {}
	class B : A
	{
		f32 y;
		fn Foo( mut this )
		{
			auto & y_ref= y;
			var A &mut a= this; // Error - implicit reference cast produces a reference linked with whole "this" when a reference to one of "this" fields exists.
		}
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


def StructFieldChildNodes_Test48():
	c_program_text= """
	class A interface {}
	class B : A
	{
		f32 y;
		fn Foo( mut this )
		{
			auto & y_ref= y;
			Bar(this); // Error - implicit reference cast produces a reference linked with whole "this" when a reference to one of "this" fields exists.
		}
	}
	fn Bar( A &mut a ) {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


def StructFieldChildNodes_Test49():
	c_program_text= """
		struct S { i32 x; f32 y; }
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			for( auto mut i= 0; i < 1; ++i )
			{
				s.x= 0; // Lazily create children node in the loop.
			}
			move(s); // Should handle here also children nodes.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test50():
	c_program_text= """
		struct S { i32 x; f32 y; }
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			while( ExternalCondition() )
			{
				s.x= 0; // Lazily create children node in the loop.
			}
			move(s); // Should handle here also children nodes.
		}
		fn ExternalCondition() : bool;
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test51():
	c_program_text= """
		struct S { i32 x; f32 y; }
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			if( ExternalCondition() )
			{
				s.x= 66; // Laziliy create child node in one branch.
			}
			else
			{
				auto& mut s_ref= s; // Access whole struct and check if there is no references to "s".
				s_ref.y= 44.0f;
			}
		}
		fn ExternalCondition() : bool;
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test52():
	c_program_text= """
		struct S { i32 x; f32 y; }
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			if( ExternalCondition() )
			{
				s.x= 66; // Laziliy create child node in one branch.
				return;
			}

			auto& mut s_ref= s;  // Access whole struct and check if there is no references to "s".
			s_ref.y= 44.0f;
		}
		fn ExternalCondition() : bool;
	"""
	tests_lib.build_program( c_program_text )


def TupleElementChildNodes_Test0():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			// Ok - create mutable references to different elements of same tuple.
			var i32 &mut x= s[0];
			var f32 &mut y= s[1];
			++x;
			y *= 2.0f;
			halt if( x != 1 );
			halt if( y != 2.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TupleElementNodes_Test1():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 5, 7.0f ];
			{
				// Ok - create mutable references to different elements of same tuple, accessed by reference.
				auto &mut s_ref= s;
				var i32 &mut x= s_ref[0];
				var f32 &mut y= s_ref[1];
				++x;
				y *= 2.0f;
			}
			halt if( s[0] != 6 );
			halt if( s[1] != 14.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TupleElementNodes_Test2():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			var i32 &mut x= s[0];
			var i32 &mut x_again= s[0]; // Error - creating second mutable reference to same tuple element.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test3():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			var i32 &mut x= s[0];
			auto & s_whole= s; // Error - creating reference to whole tuple with existing mutable reference to its element.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test4():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			var i32 & x= s[0];
			auto &mut s_whole= s; // Error - creating mutable reference to whole tuple with existing reference to its element.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test5():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			auto &mut s_whole= s;
			auto & x= s[0]; // Error - creating reference to tuple element with existing mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test6():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			auto & s_whole= s;
			auto &mut x= s[0]; // Error - creating mutable reference to tuple element with existing reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test7():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			auto & s_whole= s;
			auto & x= s[0]; // Ok - creating reference to tuple element for tuple with existing reference.
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleElementNodes_Test8():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			auto & x= s[0];
			auto & s_whole= s; // Ok - creating reference to tuple for tuple with existing reference to element.
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleElementNodes_Test9():
	c_program_text= """
		fn Bar( i32& mut x, f32 &mut y ) {}
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			Bar( s[0], s[1] ); // Ok - passing to function two mutable references to different elements of same tuple.
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleElementNodes_Test10():
	c_program_text= """
		fn Bar( f32& mut x, f32 &mut y ) {}
		fn Foo()
		{
			var tup[ f32, f32 ] mut s[ 0.0f, 1.0f ];
			Bar( s[1], s[1] ); // Error - passing to function two mutable references to same element of same tuple.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test11():
	c_program_text= """
		fn Bar( i32& mut x, tup[ i32, f32 ]& mut s ) {}
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			Bar( s[0], s ); // Error - passing to function mutable reference to tuple element and mutable reference to whole tuple.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test12():
	c_program_text= """
		fn Bar( tup[ i32, f32 ]& mut s, f32& mut y ) {}
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			Bar( s, s[1] ); // Error - passing to function mutable reference to whole tuple and mutable reference to tuple element.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test13():
	c_program_text= """
		fn Bar( tup[ i32, f32 ]& s, f32& mut y ) {}
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			Bar( s, s[1] ); // Error - passing to function reference to whole tuple and mutable reference to tuple element.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test14():
	c_program_text= """
		fn Bar( tup[ i32, f32 ] &mut s, f32& y ) {}
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			Bar( s, s[1] ); // Error - passing to function mutable reference to whole tuple and reference to tuple element.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test15():
	c_program_text= """
		fn Bar( tup[ i32, f32 ] & s, f32& y ) {}
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			Bar( s, s[1] ); // Ok - passing to function reference to whole tuple and reference to tuple element.
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleElementNodes_Test16():
	c_program_text= """
		fn Bar( i32& x, tup[ i32, f32 ]& s ) {}
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			Bar( s[0], s ); // Ok - passing to function reference to tuple element and reference to whole tuple.
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleElementNodes_Test17():
	c_program_text= """
		fn Bar( i32& x, f32& y ) {}
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			Bar( s[0], s[1] ); // Ok - passing to function references to two tuple elements.
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleElementNodes_Test18():
	c_program_text= """
		fn Bar( i32& x, i32& y ) {}
		fn Foo()
		{
			var tup[ i32, f32 ] mut s[ 0, 1.0f ];
			Bar( s[0], s[0] ); // Ok - passing to function two references to same tuple element.
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleElementNodes_Test19():
	c_program_text= """
		fn Foo()
		{
			var tup[ tup[ i32, f32 ], bool ] mut t[ [ 0, 1.0f ], false ];
			{
				// Ok - create mutable references to different elements of same tuple.
				var i32 &mut x= t[0][0];
				var f32 &mut y= t[0][1];
				var bool &mut z= t[1];
				x= 16;
				y= 45.3f;
				z= false;
			}
			halt if( t[0][0] != 16 );
			halt if( t[0][1] != 45.3f );
			halt if( t[1] != false );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TupleElementNodes_Test20():
	c_program_text= """
		fn Foo()
		{
			var tup[ tup[ i32, f32 ], bool ] mut t[ [ 0, 1.0f ], false ];
			{
				// Ok - create mutable references to different elements of same tuple.
				var tup[ i32, f32 ] &mut s= t[0];
				var i32 &mut x= s[0];
				var f32 &mut y= s[1];
				var bool &mut z= t[1];
				x= 16;
				y= 45.3f;
				z= false;
			}
			halt if( t[0][0] != 16 );
			halt if( t[0][1] != 45.3f );
			halt if( t[1] != false );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TupleElementNodes_Test21():
	c_program_text= """
		fn Foo()
		{
			var tup[ tup[ i32, f32 ], bool ] mut t[ [ 0, 1.0f ], false ];
			var tup[ i32, f32 ] &mut s= t[0];
			var i32 &mut x= t[0][0]; // Error - creating mutable reference to "t[0]" which already has one mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test22():
	c_program_text= """
		fn Foo()
		{
			var tup[ tup[ i32, f32 ], bool ] mut t[ [ 0, 1.0f ], false ];
			var tup[ i32, f32 ] & s= t[0];
			var i32 &mut x= t[0][0]; // Error - creating mutable reference to "t[0]" which already has one reference.

		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test23():
	c_program_text= """
		fn Foo()
		{
			var tup[ tup[ i32, f32 ], bool ] mut t[ [ 0, 1.0f ], false ];
			var tup[ i32, f32 ] &mut s= t[0];
			var i32 & x= t[0][0]; // Error - creating reference to "t[0]" which already has one mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test24():
	c_program_text= """
		fn Foo()
		{
			var tup[ tup[ i32, f32 ], bool ] mut t[ [ 0, 1.0f ], false ];
			var tup[ i32, f32 ] &mut s= t[0];
			var i32 &mut x= s[0]; // Ok - creating reference, linked to reference "s".
			var f32 &mut y= s[1]; // Ok - creating reference, linked to reference "s".
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleElementNodes_Test25():
	c_program_text= """
	fn Pass( i32 &mut x ) : i32 &mut { return x; }
	fn Foo()
	{
		var tup[ i32, f32 ] mut s[ 0, 1.0f ];
		var i32& mut x= Pass( s[0] ); // Ok, pass mutable reference to element 0. Result reference will point only to element node.
		var f32 &mut y= s[1]; // Ok create reference to anouther element.
	}
	"""
	tests_lib.build_program( c_program_text )


def TupleElementNodes_Test26():
	c_program_text= """
	fn Pass( i32 &mut x ) : i32 &mut { return x; }
	fn Foo()
	{
		var tup[ i32, f32 ] mut s[ 0, 1.0f ];
		var i32& mut x= Pass( s[0] ); // Ok, pass reference to element 0. Result reference will point only to element node.
		var i32 &mut x_again= s[0]; // Error - create mutable reference to same element.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def TupleElementNodes_Test27():
	c_program_text= """
	fn Foo()
	{
		var tup[ i32, f32 ] mut s[ 0, 1.0f ];
		var i32& mut x= s[0];
		cast_imut(s); // Error, create intermediate immutable reference to whole "s" while mutable reference to element "s.x" exists.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test28():
	c_program_text= """
	fn Foo( tup[ i32, i32, i32 ]& mut s, bool condition )
	{
		var i32 &mut ref= ( condition ? s[0] : s[1] ); // Create reference to both elements 0 and 1 (acutally not, but logically create.
		var i32& mut z_ref= s[2]; // Ok, create reference to element 2.
	}
	"""
	tests_lib.build_program( c_program_text )


def TupleElementNodes_Test29():
	c_program_text= """
	fn Foo( tup[ i32, i32, i32 ]& mut s, bool condition )
	{
		var i32 &mut ref= ( condition ? s[0] : s[1] ); // Create reference to both elements 0 and 1 (acutally not, but logically create.
		var i32& mut x_ref= s[0]; // Error, create reference to element 0 again.
		var i32& mut y_ref= s[1]; // Error, create reference to element 1 again.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 2 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def TupleElementNodes_Test30():
	c_program_text= """
	fn Foo( tup[ i32, i32, i32 ]& mut s, bool condition )
	{
		var i32 &mut ref= ( condition ? s[0] : s[1] ); // Create reference to boths elements 0 and 1 (acutally not, but logically create.
		var tup[ i32, i32, i32 ]& s_ref= s; // Error, create reference to whole "s".
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )
