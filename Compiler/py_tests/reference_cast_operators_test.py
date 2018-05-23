from py_tests_common import *


def CastRef_OperatorDeclaration_Test():
	c_program_text= """
		fn Foo()
		{
			auto x= 0;
			cast_ref</ void />( x );
		}
	"""
	tests_lib.build_program( c_program_text )


def CastRefUnsafe_OperatorDeclaration_Test():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			unsafe{  cast_ref_unsafe</ f32 />( x );  }
		}
	"""
	tests_lib.build_program( c_program_text )


def CastImut_OperatorDeclaration_Test():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			cast_imut( x );
		}
	"""
	tests_lib.build_program( c_program_text )


def CastMut_OperatorDeclaration_Test():
	c_program_text= """
		fn Foo()
		{
			var i32 imut x= 0;
			unsafe{  cast_mut( x );  }
		}
	"""
	tests_lib.build_program( c_program_text )


def CastRef_Test0_ShouldCastToVoid():
	c_program_text= """
		fn ToVoid( i32& x ) : void&
		{
			return cast_ref</ void />(x);
		}
	"""
	tests_lib.build_program( c_program_text )


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
		fn Foo()
		{
			cast_ref</ void />( 42 );
		}
	"""
	tests_lib.build_program( c_program_text )


def CastRef_Test6_ShouldCastToVoidReferenceOfIncompleteType():
	c_program_text= """
		struct S;
		fn ToVoid( S& s ) : void&
		{
			return cast_ref</ void />(s);
		}
	"""
	tests_lib.build_program( c_program_text )


def CastRef_Test7_CompleteteTypeRequiredForSource():
	c_program_text= """
		class B;
		class A polymorph {}
		fn ToA( B& b ) : A&
		{
			return cast_ref</ A />(b);
		}
		class B : A {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].file_pos.line == 6 )


def CastRef_Test8_CompleteteTypeRequiredForDestination():
	c_program_text= """
		class A;
		class B;
		fn ToA( B& b ) : A&
		{
			return cast_ref</ A />(b);
		}
		class A polymorph {}
		class B : A {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].file_pos.line == 6 )


def CastRef_Test9_ShouldPreserveReferencedVariables():
	c_program_text= """
		fn Foo() : void&
		{
			auto x= 0;
			return cast_ref</ void />(x); //  Return reference to local variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "DestroyedVariableStillHaveReferences" )
	assert( errors_list[0].file_pos.line == 5 )


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
	assert( errors_list[0].file_pos.line == 12 )


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
			{
				auto &mut r= AsInt( cast_ref</ void />(x) );  // Cast to void and back
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
			{
				var S &mut r= AsS( cast_ref</ void />(s) );  // Cast to void and back
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
			var void &imut v= x;
			unsafe{  return A( cast_ref_unsafe</ i32 />(v) );  }
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
			var void &mut v= x;
			unsafe{  return A( cast_ref_unsafe</ i32 />(v) );  }
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
	assert( errors_list[0].file_pos.line == 4 )


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
	assert( errors_list[0].file_pos.line == 4 )


def CastRefUnsafe_Test8_CompletenessStillRequiredForUnsafeCast():
	c_program_text= """
		class A; class B polymorph {}
		fn ToA( B& b ) : A&
		{
			unsafe{  return cast_ref_unsafe</ A />(b);  }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].file_pos.line == 5 )


def CastRefUnsafe_Test9_CompletenessStillRequiredForUnsafeCast():
	c_program_text= """
		class A; class B polymorph {}
		fn ToB( A& a ) : B&
		{
			unsafe{  return cast_ref_unsafe</ B />(a);  }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].file_pos.line == 5 )
