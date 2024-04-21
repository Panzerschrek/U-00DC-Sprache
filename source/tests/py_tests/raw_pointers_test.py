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


def ElementTypeCompletenessIsNotRequiredForRawPointerDeclaration_Test2():
	c_program_text= """
		struct T{ $(T) ptr; }
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
			var i32 mut x= 0;
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
			var i32 mut x= 0;
			var $(i32) mut x_ptr= $<(x);
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
			var $(f64) mut f_ptr= $<(f);
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
			auto mut x= 73472;
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
			auto mut x= 654.5f;
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
			var char8 mut c= "$"c8;
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
			var f64 mut f= 3.25;
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
			var char8 mut c= "E"c8;
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
			var f32 mut f= 568.125f;
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
			var i32 mut i= 44445;
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
			var i32 mut i= 56;
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


def RawPointersCompare_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var $(i32) x_ptr= $<(x), y_ptr= $<(y);
			halt if( x_ptr == y_ptr );
			halt if( y_ptr == x_ptr );
			halt if( !( x_ptr != y_ptr ) );
			halt if( !( y_ptr != x_ptr ) );
			halt if( x_ptr != x_ptr );
			halt if( y_ptr != y_ptr );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def RawPointersCompare_Test1():
	c_program_text= """
		fn Foo()
		{
			var [ f32, 3 ] mut a= zero_init;
			var $(f32) ptr0= $<(a[0]), ptr1= $<(a[1]), ptr2= $<(a[2]);

			// <
			halt if( !( ptr0 < ptr1 ) );
			halt if( !( ptr1 < ptr2 ) );
			halt if( !( ptr0 < ptr2 ) );
			halt if( ptr0 < ptr0 );
			halt if( ptr1 < ptr1 );
			halt if( ptr2 < ptr2 );
			halt if( ptr1 < ptr0 );
			halt if( ptr2 < ptr1 );
			halt if( ptr2 < ptr0 );

			// <=
			halt if( !( ptr0 <= ptr1 ) );
			halt if( !( ptr1 <= ptr2 ) );
			halt if( !( ptr0 <= ptr2 ) );
			halt if( !( ptr0 <= ptr0 ) );
			halt if( !( ptr1 <= ptr1 ) );
			halt if( !( ptr2 <= ptr2 ) );
			halt if( ptr1 <= ptr0 );
			halt if( ptr2 <= ptr1 );
			halt if( ptr2 <= ptr0 );

			// >
			halt if( ptr0 > ptr1 );
			halt if( ptr1 > ptr2 );
			halt if( ptr0 > ptr2 );
			halt if( ptr0 > ptr0 );
			halt if( ptr1 > ptr1 );
			halt if( ptr2 > ptr2 );
			halt if( !( ptr1 > ptr0 ) );
			halt if( !( ptr2 > ptr1 ) );
			halt if( !( ptr2 > ptr0 ) );

			// >=
			halt if( ptr0 >= ptr1 );
			halt if( ptr1 >= ptr2 );
			halt if( ptr0 >= ptr2 );
			halt if( !( ptr0 >= ptr0 ) );
			halt if( !( ptr1 >= ptr1 ) );
			halt if( !( ptr2 >= ptr2 ) );
			halt if( !( ptr1 >= ptr0 ) );
			halt if( !( ptr2 >= ptr1 ) );
			halt if( !( ptr2 >= ptr0 ) );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def RawPointerIntegerAdd_Test0():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 3 ] mut a[ 33, 55, 77 ];
			var $(i32) ptr0= $<(a[0]), ptr1= $<(a[1]), ptr2= $<(a[2]);

			unsafe
			{
				halt if( ptr0 + 0 != ptr0 );
				halt if( 0 + ptr0 != ptr0 );

				halt if( ptr0 + 1 != ptr1 );
				halt if( 1 + ptr0 != ptr1 );

				halt if( ptr0 + 2 != ptr2 );
				halt if( 2 + ptr0 != ptr2 );

				halt if( ptr1 + 1 != ptr2 );
				halt if( 1 + ptr1 != ptr2 );

				halt if( ptr1 + (-1) != ptr0 );
				halt if( (-1) + ptr1 != ptr0 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def RawPointerIntegerAdd_Test1():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 3 ] mut a[ 33, 55, 77 ];
			var $(i32) ptr0= $<(a[0]), ptr1= $<(a[1]), ptr2= $<(a[2]);

			unsafe
			{
				// Add signed positive value.
				halt if( ptr0 + i32(1) != ptr1 );
				halt if( i32(1) + ptr0 != ptr1 );

				// Add unsigned positive value.
				halt if( ptr0 + u32(1) != ptr1 );
				halt if( u32(1) + ptr0 != ptr1 );

				// Add signed negative value.
				halt if( ptr1 + i32(-1) != ptr0 );
				halt if( i32(-1) + ptr1 != ptr0 );

				// Add unsigned value of small type.
				halt if( ptr2 + u8(0xFE) < ptr2 );
				halt if( u8(0xFE) + ptr2 < ptr2 );

				// Add signed value of small type.
				halt if( ptr2 + i8(0xFE) != ptr0 );
				halt if( i8(0xFE) + ptr2 !=  ptr0 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def RawPointerIntegerAdd_Test2():
	c_program_text= """
		fn Foo()
		{
			var tup[] mut t= zero_init;
			auto ptr= $<(t);

			unsafe
			{
				// Integer to pointer addition produces pointer value itslef for element types with zero size.
				halt if( ptr + 1 != ptr );
				halt if( ptr - 2 != ptr );
				halt if( ptr + 100 != ptr - 222 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def RawPointerIntegerAdd_Test3():
	c_program_text= """
		fn Foo()
		{
			var void mut v= zero_init;
			auto ptr= $<(v);

			unsafe
			{
				// Integer to pointer addition produces pointer value itslef for element types with zero size.
				halt if( ptr + 1 != ptr );
				halt if( ptr - 2 != ptr );
				halt if( ptr + 100 != ptr - 222 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def RawPointerIntegerSub_Test0():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 3 ] mut a[ 33, 55, 77 ];
			var $(i32) ptr0= $<(a[0]), ptr1= $<(a[1]), ptr2= $<(a[2]);

			unsafe
			{
				halt if( ptr0 - 0 != ptr0 );
				halt if( ptr1 - 1 != ptr0 );
				halt if( ptr2 - 2 != ptr0 );
				halt if( ptr2 - 1 != ptr1 );

				halt if( $>(ptr1 - 1) != 33 );
				halt if( $>(ptr2 - 2) != 33 );
				halt if( $>(ptr2 - 1) != 55 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def RawPointerIntegerSub_Test1():
	c_program_text= """
		fn Foo()
		{
			unsafe{ var [ u64, 1024 * 1024 ] arr= uninitialized; } // Add to stack some large value in order to avoid pointer overflow.
			var [ i32, 3 ] mut a[ 33, 55, 77 ];
			var $(i32) ptr0= $<(a[0]), ptr1= $<(a[1]), ptr2= $<(a[2]);

			unsafe
			{
				// Subtract signed positive value.
				halt if( ptr1 - i32(1) != ptr0 );
				halt if( $>( ptr1 - i32(1) ) != 33 );
			}
			unsafe
			{
				// Subtract unsigned positive value.
				halt if( ptr2 - u16(1) != ptr1 );
				halt if( $>( ptr2 - u16(1) ) != 55 );
			}
			unsafe
			{
				// Subtract signed negative value value.
				halt if( ptr0 - i8(-2) != ptr2 );
				halt if( $>( ptr0 - i8(-2) ) != 77 );
			}
			unsafe
			{
				// Subtract large unsinged value.
				halt if( ptr0 - u16(0xFABC) >= ptr0 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def RawPointersDifference_Test0():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 3 ] mut a= zero_init;
			var $(i32) ptr0= $<(a[0]), ptr1= $<(a[1]), ptr2= $<(a[2]);

			unsafe
			{
				// Difference between pointer is divided by element size.
				halt if( i32(ptr0 - ptr0) != 0 );
				halt if( i32(ptr1 - ptr0) != 1 );
				halt if( i32(ptr2 - ptr0) != 2 );
				halt if( i32(ptr0 - ptr1) != -1 );
				halt if( i32(ptr0 - ptr2) != -2 );

				halt if( (ptr2 - ptr1) + (ptr1 - ptr0) != ptr2 - ptr0 );

				halt if( i32( ptr2 + 100 - ptr2 ) != 100 );

				// Difference type must be signed integer.
				halt if( !typeinfo</ typeof( ptr0 - ptr0 ) />.is_signed_integer );

				// Size of difference type must be equal to size of "size_type".
				halt if( typeinfo</size_type/>.size_of != typeinfo</ typeof( ptr0 - ptr0 ) />.size_of );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AdditiveAssignmentForRawPointers_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var [ i32, 3 ] mut a[ 33, 55, 77 ];
			var $(i32) mut ptr= $<(a[0]);

			unsafe
			{
				ptr+= 2;
				ptr-= 1;
				return $>(ptr);
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 55 )


def AdditiveAssignmentForRawPointers_Test1():
	c_program_text= """
		fn Foo() : f64
		{
			var f64 mut f= 67785.5;
			var $(f64) mut ptr= zero_init;
			unsafe
			{
				ptr+= ( $<(f) - ptr );
				return $>(ptr);
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 67785.5 )


def RawPointerDeltaOne_Test0():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 3 ] mut a[ 33, 55, 77 ];
			var $(i32) ptr0= $<(a[0]), ptr1= $<(a[1]), ptr2= $<(a[2]);

			auto mut ptr= ptr0;

			unsafe
			{
				++ptr;
				halt if( ptr != ptr1 );
				halt if( $>(ptr) != 55 );

				++ptr;
				halt if( ptr != ptr2 );
				halt if( $>(ptr) != 77 );

				--ptr;
				halt if( ptr != ptr1 );
				halt if( $>(ptr) != 55 );

				--ptr;
				halt if( ptr != ptr0 );
				halt if( $>(ptr) != 33 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def Typeinfo_ForRawPointerType_Test0():
	c_program_text= """
		auto& ti= typeinfo</ $(i32) />;
		static_assert( ti.is_raw_pointer );
		static_assert( ensure_same_type( ti.element_type, typeinfo</i32/> ) );

		template</type T/> fn ensure_same_type( T& a, T& b ) : bool { return true; }
	"""
	tests_lib.build_program( c_program_text )


def Typeinfo_ForRawPointerType_Test1():
	c_program_text= """
		// Size of all pointers is same.
		static_assert( typeinfo</ $(i32) />.size_of == typeinfo</ $(i16) />.size_of );
		static_assert( typeinfo</ $(f64) />.size_of == typeinfo</ $( tup[ f32, bool, char16 ] ) />.size_of );
		static_assert( typeinfo</ $(char8) />.size_of == typeinfo</ $( [ i32, 256 ] ) />.size_of );

		// Size of pointer is equal to size of "size_type"
		static_assert( typeinfo</ $([ f64, 4 ] ) />.size_of == typeinfo</ size_type />.size_of );
	"""
	tests_lib.build_program( c_program_text )


def Typeinfo_ForRawPointerType_Test2():
	c_program_text= """
		class C{}
		auto& ti= typeinfo</ $(C) />;
		static_assert( !ti.is_default_constructible );
		static_assert( ti.is_copy_constructible );
		static_assert( ti.is_copy_assignable );
	"""
	tests_lib.build_program( c_program_text )


def RawPointerTypeTemplateSpecialization_Test0():
	c_program_text= """
		template</ type T />
		struct S</ $(T) />
		{
			auto is_pointer= true;
		}

		static_assert( S</ $(char32) />::is_pointer );
		static_assert( S</ $( $(bool) ) />::is_pointer );

		type PtrAlias= $( tup[ bool, char16, [ i32, 2 ] ] );
		static_assert( S</ PtrAlias />::is_pointer );

	"""
	tests_lib.build_program( c_program_text )


def RawPointerTypeTemplateSpecialization_Test1():
	c_program_text= """
		template</ type T />
		struct RemovePointer</ $(T) />
		{
			type t= T;
		}

		static_assert( typeinfo</ RemovePointer</ $(char8) />::t />.is_char );
		static_assert( typeinfo</ RemovePointer</ $(tup[]) />::t />.is_tuple );

		static_assert( typeinfo</ RemovePointer</ $($(f32)) />::t />.is_raw_pointer );
		static_assert( typeinfo</ RemovePointer</ $($(f32)) />::t />.element_type.is_float );
	"""
	tests_lib.build_program( c_program_text )


def RawPointerTypeTemplateSpecialization_Test2():
	c_program_text= """

		// Template with raw pointer type is more specialized, so, this template will be selected recursively until non-pointer type reached.

		template</ type T />
		struct RemovePointer_r</ $(T) />
		{
			type t= ::RemovePointer_r</ T />::t;
			auto constexpr depth= 1s + ::RemovePointer_r</ T />::depth;
		}

		template</ type T />
		struct RemovePointer_r</ T />
		{
			type t= T;
			auto constexpr depth= 0s;
		}

		static_assert( typeinfo</ RemovePointer_r</i32/>::t />.is_signed_integer );
		static_assert( RemovePointer_r</i32/>::depth == 0s );

		static_assert( typeinfo</ RemovePointer_r</ $(f32) />::t />.is_float );
		static_assert( RemovePointer_r</ $(f32) />::depth == 1s );

		static_assert( typeinfo</ RemovePointer_r</ $($($( [ bool, 16] ))) />::t />.is_array );
		static_assert( RemovePointer_r</ $($($( [ bool, 16] ))) />::depth == 3s );
	"""
	tests_lib.build_program( c_program_text )


def RawPointerTypeTemplateSpecialization_Test3():
	c_program_text= """

		// One of two specializations selected.

		template</ type T />
		struct ExtractElement</ $(T) />
		{
			type t= T;
		}

		template</ type T, size_type s />
		struct ExtractElement</ [ T, s ] />
		{
			type t= T;
		}

		static_assert( typeinfo</ ExtractElement</ $(i32) />::t />.is_signed_integer );
		static_assert( typeinfo</ ExtractElement</ [ bool, 64 ] />::t />.is_bool );

		static_assert( typeinfo</ ExtractElement</ $($(u16)) />::t />.is_raw_pointer );
		static_assert( typeinfo</ ExtractElement</ $( [ f32, 3 ] ) />::t />.is_array );
	"""
	tests_lib.build_program( c_program_text )


def RawPointerTypeTemplateSpecialization_Test4():
	c_program_text= """
		template</ type T />
		struct S</ T />
		{
			auto c= 3s;
		}

		template</ type T />
		struct S</ $(T) />
		{
			auto c= 22s;
		}

		template</ />
		struct S</ $(char8) />
		{
			auto c= 666s;
		}

		static_assert( S</ i8 />::c == 3s ); // Selected specialization for any type (not only pointer).
		static_assert( S</ $(f32) />::c == 22s ); // General specialization selected.
		static_assert( S</ $(char8) />::c == 666s ); // Exact specialization selected.
	"""
	tests_lib.build_program( c_program_text )


def RawPointerTypeTemplateSpecialization_Test5():
	c_program_text= """
		template</ type T />
		struct RemovePointer</ $(T) />
		{
			type t= T;
		}

		type WTF= RemovePointer</ bool />;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TemplateParametersDeductionFailed", 8 ) )


def RawPointerTemplateFunctionSpecialization_Test0():
	c_program_text= """
		template</ type T /> fn Bar( T t ) : size_type { return  3s; }
		template</ type T /> fn Bar( $(T) t ) : size_type { return 9s; }
		template</ type T, size_type s /> fn Bar( [ T, s ] t ) : size_type { return 99999999s; }
		template<//> fn Bar( $(i32) t ) : size_type { return 27s; }
		template</ type T, size_type s /> fn Bar( $( [ T, s ] ) t ) : size_type { return s; }
		template</ type T, size_type s /> fn Bar( [ $(T), s ] arr ) : size_type{ return s * 100s; }

		fn Foo()
		{
			var i32 mut x= 0;
			var $(i32) mut x_ptr= $<(x);
			var $($(i32)) x_ptr_ptr= $<(x_ptr);

			halt if( Bar( x ) != 3s ); // General function selected.
			halt if( Bar( x_ptr ) != 27s ); // Exact specialization selected.
			halt if( Bar( x_ptr_ptr ) != 9s ); // Specialization for general raw pointer selected.

			// Specialization for pointer to array.
			var [ f32, 33 ] mut f_arr= zero_init;
			var [ bool, 7 ] mut b_arr= zero_init;
			halt if( Bar( $<(f_arr) ) != 33s );
			halt if( Bar( $<(b_arr) ) != 7s );

			// Specialization for array of pointers.
			var [ $(i32), 3 ] int_ptr_arr= zero_init;
			var [ $($(f32)), 11 ] f_ptr_ptr_arr= zero_init;
			halt if( Bar( int_ptr_arr ) != 300s );
			halt if( Bar( f_ptr_ptr_arr ) != 1100s );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
