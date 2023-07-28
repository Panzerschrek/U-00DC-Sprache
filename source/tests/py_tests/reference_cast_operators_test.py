from py_tests_common import *


def CastRef_OperatorDeclaration_Test():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Foo()
		{
			var B b;
			cast_ref</ A/>( b );
		}
	"""
	return
	tests_lib.build_program( c_program_text )


def CastRefUnsafe_OperatorDeclaration_Test():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			unsafe{  auto& x_casted= cast_ref_unsafe</ f32 />( x );  }
		}
	"""
	tests_lib.build_program( c_program_text )


def CastImut_OperatorDeclaration_Test():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto& x_casted= cast_imut( x );
		}
	"""
	tests_lib.build_program( c_program_text )


def CastMut_OperatorDeclaration_Test():
	c_program_text= """
		fn Foo()
		{
			var i32 imut x= 0;
			unsafe{  auto& mut x_casted= cast_mut( x );  }
		}
	"""
	tests_lib.build_program( c_program_text )


def CastRef_Test0_ShouldNotCastToVoid():
	c_program_text= """
		fn ToVoid( i32& x ) : void&
		{
			return cast_ref</ void />(x);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def CastRef_Test1_ShouldCastChildToParent():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn ToA( B& b ) : A&
		{
			return cast_ref</ A />(b);
		}
	"""
	tests_lib.build_program( c_program_text )


def CastRef_Test2_CastToSameType():
	c_program_text= """
		fn ToF32( f32& f ) : f32&
		{
			return cast_ref</ f32 />(f);
		}
	"""
	tests_lib.build_program( c_program_text )


def CastRef_Test3_ShouldSaveMutability():
	c_program_text= """
		fn A( i32&imut x ) : i32 { return 555; }
		fn A( i32& mut x ) : i32 { return 999; }

		fn Foo() : i32
		{
			auto mut x= 0;
			return A( cast_ref</ i32 />(x) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999 )


def CastRef_Test4_ShouldSaveMutability():
	c_program_text= """
		fn A( i32&imut x ) : i32 { return 555; }
		fn A( i32& mut x ) : i32 { return 999; }

		fn Foo() : i32
		{
			auto imut x= 0;
			return A( cast_ref</ i32 />(x) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 555 )


def CastRef_Test5_ShouldCastValue():
	c_program_text= """
		class A polymorph {}
		class B : A {}
		fn Foo()
		{
			with( &val : cast_ref</ A />( B() ) )
			{}
		}
	"""
	tests_lib.build_program( c_program_text )


def CastRef_Test9_ShouldPreserveReferencedVariables():
	c_program_text= """
		fn Foo() : void&
		{
			auto x= 0;
			unsafe{  return cast_ref_unsafe</ void />(x);  } //  Return reference to local variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "DestroyedVariableStillHaveReferences" )
	assert( errors_list[0].src_loc.line == 5 )


def CastRef_Test10_ShouldPreserveReferencedVariables():
	c_program_text= """
		class A polymorph {}
		class B : A
		{
			i32 x;
			fn constructor() ( x= 0 ) {}
		}
		fn Foo()
		{
			var B mut b;
			auto& r= cast_ref</ A />(b);
			++b.x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 12 )


def CastRefUnsafe_Test0_SholudCastIncompatibleReferences():
	c_program_text= """
		fn ToI32( f32& f ) : i32&
		{
			unsafe{  return cast_ref_unsafe</ i32 />(f);  }
		}
	"""
	tests_lib.build_program( c_program_text )


def CastRefUnsafe_Test1_SholudCastVoidToAnything():
	c_program_text= """
		fn AsInt( void&mut v ) : i32&mut
		{
			unsafe{  return cast_ref_unsafe</ i32 />(v);  }
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			unsafe
			{
				auto &mut r= AsInt( cast_ref_unsafe</ void />(x) );  // Cast to void and back
				r= 66854;
			}
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 66854 )


def CastRefUnsafe_Test2_SholudCastVoidToAnything():
	c_program_text= """
		struct S{ f32 x; }
		fn AsS( void&mut v ) : S&mut
		{
			unsafe{  return cast_ref_unsafe</ S />(v);  }
		}

		fn Foo() : f32
		{
			var S mut s{ .x= 0.0f };
			unsafe
			{
				var S &mut r= AsS( cast_ref_unsafe</ void />(s) );  // Cast to void and back
				r.x= 5565.5f;
			}
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5565.5 )


def CastRefUnsafe_Test3_ShouldSaveMutability():
	c_program_text= """
		fn A( i32&imut x ) : i32 { return 555; }
		fn A( i32& mut x ) : i32 { return 999; }

		fn Foo() : i32
		{
			auto imut x= 0;
			unsafe
			{
				auto &imut v= cast_ref_unsafe</void/>(x);
				return A( cast_ref_unsafe</ i32 />(v) );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 555 )


def CastRefUnsafe_Test4_ShouldSaveMutability():
	c_program_text= """
		fn A( i32&imut x ) : i32 { return 555; }
		fn A( i32& mut x ) : i32 { return 999; }

		fn Foo() : i32
		{
			auto mut x= 0;
			unsafe
			{
				auto &mut v= cast_ref_unsafe</void/>( x );
				return A( cast_ref_unsafe</ i32 />(v) );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999 )


def CastRefUnsafe_Test5_UnsafeCastWorksAlsoForSafeCasting():
	c_program_text= """
		fn Foo( i32& i ) : void&
		{
			unsafe{  return cast_ref_unsafe</ void />(i);  }
		}
	"""
	tests_lib.build_program( c_program_text )


def CastRefUnsafe_Test6_OperatorAllowedOnlyInsideUnsafeBlock():
	c_program_text= """
		fn Foo( void& v ) : i32&
		{
			return cast_ref_unsafe</ i32 />(v);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeReferenceCastOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 4 )


def CastRefUnsafe_Test7_OperatorAllowedOnlyInsideUnsafeBlock():
	c_program_text= """
		fn Foo( i32& i ) : void&
		{
			return cast_ref_unsafe</ void />(i);   // Even if cast is safe, using 'cast_ref_unsafe' operator is unsafe.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UnsafeReferenceCastOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 4 )


def CastImut_Test0_CastMutableReferenceToImmutableReference():
	c_program_text= """
		fn A( i32&imut x ) : i32 { return 555; }
		fn A( i32& mut x ) : i32 { return 999; }

		fn Foo() : i32
		{
			auto mut x= 0;
			return A( cast_imut(x) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 555 )


def CastImut_Test1_CastImmutableReferenceToImmutableReference():
	c_program_text= """
		fn A( i32&imut x ) : i32 { return 555; }
		fn A( i32& mut x ) : i32 { return 999; }

		fn Foo() : i32
		{
			auto imut x= 0;
			return A( cast_imut(x) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 555 )


def CastImut_Test2_CastValueToImmutableReference():
	c_program_text= """
		fn A( i32&imut x ) : i32 { return 555; }
		fn A( i32& mut x ) : i32 { return 999; }

		fn Foo() : i32
		{
			return A( cast_imut(-1) );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 555 )


def CastImut_Test3_ShouldPreserveReferences():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			auto &imut r= cast_imut(x); // Save reference here
			++x; // error, modifying 'x', when reference to it exists.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 6 )


def CastMut_Test0_CastImmutableReferenceToMutableReference():
	c_program_text= """
		fn A( i32&imut x ) : i32 { return 555; }
		fn A( i32& mut x ) : i32 { return 999; }

		fn Foo() : i32
		{
			auto imut x= 0;
			unsafe{  return A( cast_mut(x) );  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999 )


def CastMut_Test1_CastMutableReferenceToMutableReference():
	c_program_text= """
		fn A( i32&imut x ) : i32 { return 555; }
		fn A( i32& mut x ) : i32 { return 999; }

		fn Foo() : i32
		{
			auto mut x= 0;
			unsafe{  return A( cast_mut(x) );  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999 )


def CastMut_Test2_CastValueToMutableReference():
	c_program_text= """
		fn A( i32&imut x ) : i32 { return 555; }
		fn A( i32& mut x ) : i32 { return 999; }

		fn Foo() : i32
		{
			unsafe{  return A( cast_mut(42) );  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999 )


def CastMut_Test3_ShouldPreserveReferences():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
				auto mut x= 0;
				auto &mut r= cast_mut(x); // Save reference here
				++x; // error, modifying 'x', when reference to it exists.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 8 )


def CastMut_Test4_OperationIsUnsafe():
	c_program_text= """
		fn Foo()
		{
			auto imut x= 0;
			auto& x_casted= cast_mut( x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MutableReferenceCastOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 5 )


def CastMut_Test5_OperationIsUnsafe():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			auto &mut x_ref= cast_mut( x );  // even mut->mut casting is unsafe
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MutableReferenceCastOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 5 )


def CastMut_Test6_ConstexprLostInConversion():
	c_program_text= """
		fn Foo()
		{
			auto imut x= 0;
			unsafe
			{
				auto &constexpr x_ref= cast_mut(x);
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VariableInitializerIsNotConstantExpression" )
	assert( errors_list[0].src_loc.line == 7 )
