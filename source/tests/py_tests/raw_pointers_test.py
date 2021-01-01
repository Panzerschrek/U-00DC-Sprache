from py_tests_common import *

def PointerTypeDeclaration_Test0():
	c_program_text= """
		type IntPtr = $(i32);
	"""
	tests_lib.build_program( c_program_text )


def PointerTypeDeclaration_Test1():
	c_program_text= """
		type IntPtrPtr = $($(i32));
	"""
	tests_lib.build_program( c_program_text )


def PointerTypeDeclaration_Test2():
	c_program_text= """
		type IntArray4Ptr = $([i32, 4]);
	"""
	tests_lib.build_program( c_program_text )


def PointerTypeDeclaration_Test3():
	c_program_text= """
		type EmptyTupPtr = $(tup[]);
	"""
	tests_lib.build_program( c_program_text )


def RawPointerTypeUsage_Test0():
	c_program_text= """
		fn Foo( $(i32) x, $($(f32)) y ){}
	"""
	tests_lib.build_program( c_program_text )


def RawPointerInitializers_Test0():
	c_program_text= """
		fn Foo() : size_type
		{
			var $(i32) z= zero_init; // Zero initializer for raw pointer.
			unsafe{  return cast_ref_unsafe</size_type/>(z);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0 )


def RawPointerInitializers_Test1():
	c_program_text= """
		struct S{ $(i32) ptr; }
		fn Foo() : size_type
		{
			var S s= zero_init; // Zero initializer for raw pointer inside struct.
			unsafe{  return cast_ref_unsafe</size_type/>(s.ptr);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0 )


def RawPointerInitializers_Test2():
	c_program_text= """
		struct S{ $(i32) ptr; }
		fn Foo() : size_type
		{
			var S s{ .ptr= zero_init }; // Zero initializer for raw pointer inside struct.
			unsafe{  return cast_ref_unsafe</size_type/>(s.ptr);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0 )


def RawPointerInitializers_Test3():
	c_program_text= """
		fn Foo() : size_type
		{
			var [ $(i32), 4 ] arr= zero_init; // Zero initializer for array of pointers.
			unsafe{  return cast_ref_unsafe</size_type/>(arr[2]);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0 )


def RawPointerInitializers_Test4():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut x= 0;
			var $(i32) ptr= $<(x); // Expression initializer for pointer.
			unsafe{  $>(ptr)= 67890;  }
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 67890 )


def RawPointerInitializers_Test5():
	c_program_text= """
		struct S{ $(i32) ptr; }
		fn Foo() : i32
		{
			var i32 mut x= 0;
			var S s{ .ptr= $<(x) }; // Expression initializer for pointer inside struct.
			unsafe{  $>(s.ptr)= 9632;  }
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 9632 )


def RawPointerInitializers_Test6():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut x= 0;
			var $(i32) ptr($<(x)); // Constructor initializer for pointer.
			unsafe{  $>(ptr)= 67890;  }
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 67890 )


def RawPointerInitializers_Test7():
	c_program_text= """
		struct S{ $(i32) ptr; }
		fn Foo() : i32
		{
			var i32 mut x= 0;
			var S s{ .ptr( $<(x) ) }; // Constructor initializer for pointer inside struct.
			unsafe{  $>(s.ptr)= 9632;  }
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 9632 )


def RawPointerInitializers_Test8():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut x= 7645621;
			auto ptr= $<(x); // Auto variable of pointer type.
			unsafe{  return $>(ptr);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 7645621 )


def ReferenceToPointerOperator_Test0():
	c_program_text= """
		fn Foo() : size_type
		{
			var i32 x= 0;
			var $(i32) x_ptr= $<(x);
			unsafe{  return cast_ref_unsafe</size_type/>(x_ptr);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result != 0 ) # variable should have non-zero address


def ReferenceToPointerOperator_Test1():
	c_program_text= """
		fn Foo() : size_type
		{
			var i32 x= 0;
			var $(i32) x_ptr= $<(x);
			var $($(i32)) x_ptr_ptr= $<(x_ptr);
			unsafe{  return cast_ref_unsafe</size_type/>(x_ptr_ptr);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result != 0 ) # variable should have non-zero address


def PointerToReferenceOperator_Test0():
	c_program_text= """
		fn Foo() : u32
		{
			var u32 mut x= 0u;
			var $(u32) x_ptr= $<(x);
			unsafe{  $>(x_ptr)= 678u;  } // Write value, using pointer.
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 678 )


def PointerToReferenceOperator_Test1():
	c_program_text= """
		fn Foo() : f64
		{
			auto mut f= -1.0;
			var $(f64) f_ptr= $<(f);
			auto f_ptr_ptr= $<(f_ptr);
			unsafe{  $>($>(f_ptr_ptr))= 37.5;  }
			return f;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 37.5 )


def PointerToReferenceOperator_Test2():
	c_program_text= """
		fn Foo() : i32
		{
			auto x= 73472;
			auto ptr= $<(x);
			unsafe{  return $>(ptr);  } // Read value, using pointer.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 73472 )


def PointerToReferenceOperator_Test3():
	c_program_text= """
		fn Foo() : f32
		{
			auto x= 654.5f;
			unsafe{  return $>($<(x));  } // Directly use pointer for conversion to reference
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 654.5 )


def RawPointerAssignment_Test0():
	c_program_text= """
		fn Foo() : u32
		{
			var u32 mut x= 77u, mut y= 1452u;
			auto mut ptr= $<(x);
			ptr= $<(y); // Re-assign pointer value.
			unsafe{  return $>(ptr);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 1452 )


def RawPointerAssignment_Test1():
	c_program_text= """
		struct S{ $(i64) ptr; }
		fn Foo() : i64
		{
			var S mut s= zero_init;
			var i64 mut iii(95655);
			s.ptr= $<(iii); // Assign value to struct member - pointer.
			unsafe{  return $>(s.ptr);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 95655 )


def RawPointerAssignment_Test2():
	c_program_text= """
		struct S{ $(char8) ptr; }
		fn Foo() : char8
		{
			var char8 c= "$"c8;
			var S mut s0= zero_init, s1{ .ptr= $<(c) };
			s0= s1; // Assign struct with pointer inside. Generated assignment operator should assign raw pointers.
			unsafe{  return $>(s0.ptr);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('$') )


def RawPointerAssignment_Test3():
	c_program_text= """
		fn Foo() : f64
		{
			var f64 f= 3.25;
			var tup[ f32, $(f64) ] mut t0= zero_init, t1[ 0.0f, $<(f) ];
			t0= t1; // Assignment should work for pointer member of tuple.
			unsafe{  return $>(t0[1]);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3.25 )


def RawPointerCopy_Test0():
	c_program_text= """
		struct S{ $(char8) ptr; }
		fn Foo() : char8
		{
			var char8 c= "E"c8;
			var S s0{ .ptr= $<(c) };
			var S s1(s0); // Generated copy constructor should copy raw pointer.
			unsafe{  return $>(s0.ptr);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == ord('E') )


def RawPointerCopy_Test1():
	c_program_text= """
		fn Foo() : f32
		{
			var f32 f= 568.125f;
			var tup[ i32, $(f32), bool ] t0[ 5, $<(f), false ], t1= t0; // Should copy pointer in copy-initializer for tuple.
			unsafe{  return $>(t1[1]);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 568.125 )


def RawPointerCopy_Test2():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 i= 44445;
			var [ $(i32), 4 ] a0[ zero_init, zero_init, $<(i), zero_init ], a1(a0); // Should copy pointer in copy-initializer for array.
			unsafe{  return $>(a1[2]);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 44445 )


def RawPointerAsFunctionArgument_Test0():
	c_program_text= """
		fn DerefAndAdd1( $(i32) ptr ) : i32
		{
			unsafe{  return $>(ptr) + 1;  }
		}

		fn Foo() : i32
		{
			var i32 i= 56;
			return DerefAndAdd1( $<(i) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 57 )


def RawPointerAsFunctionArgument_Test1():
	c_program_text= """
		fn SetToMax( $(u32) ptr )
		{
			unsafe{  $>(ptr)= ~0u;  }
		}

		fn Foo() : u32
		{
			var u32 mut i= 985u;
			SetToMax( $<(i) );
			return i;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0xFFFFFFFF )


def RawPointerAsReturnValue_Test0():
	c_program_text= """
		fn ArrayMemberToPtr( [ f32, 4 ] &mut arr, size_type index ) : $(f32)
		{
			return $<(arr[index]);
		}

		fn Foo() : f32
		{
			var [ f32, 4 ] mut a[ 67.5f, -22.0f, 17.0f, 6666666.0f ];
			unsafe{  return $>( ArrayMemberToPtr( a, 2s ) );  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 17.0 )


def RawPointerAsReturnValue_Test1():
	c_program_text= """
		struct S{ f32 y; u64 x; }
		fn GetX( S &mut s ) : $(u64)
		{
			return $<(s.x);
		}

		fn Foo() : u64
		{
			var S mut s= zero_init;
			unsafe{  $>( GetX( s ) )= u64(6536234);  }
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 6536234 )
