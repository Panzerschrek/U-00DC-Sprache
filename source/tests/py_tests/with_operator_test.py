from py_tests_common import *

def WithOperatorDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			with( x : 0 ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorDeclaration_Test1():
	c_program_text= """
		fn Foo()
		{
			with( & y : 0 )
			{
				return;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorDeclaration_Test2():
	c_program_text= """
		fn Foo( i32 mut a )
		{
			with( &mut z : a )
			{
				z*= 2;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorForValue_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			with( x : 6661 ) // Bind value
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 6661 )


def WithOperatorForValue_Test1():
	c_program_text= """
		fn Foo() : i32
		{
			auto s= 9514789;
			with( x : s ) // Copy constant reference to value.
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 9514789 )


def WithOperatorForValue_Test2():
	c_program_text= """
		fn Foo() : i32
		{
			auto mut s= 3214;
			with( x : s ) // Copy conent of mutable reference to value.
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3214 )


def WithOperatorForValue_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
			fn destructor(){ x= 0; }
		}
		fn GetS() : S
		{
			var S s{ .x= 8741 };
			return s;
		}
		fn Foo() : i32
		{
			var i32 mut res= zero_init;
			with( s : GetS() ) // Should move 's' here.
			{
				res= s.x; // Should read member here before 's' destruction.
			}
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 8741 )


def WithOperatorForValue_Test4():
	c_program_text= """
		struct S
		{
			i32 x;
			fn destructor(){ x= 0; }
		}
		fn Foo() : i32
		{
			var i32 mut res= zero_init;
			var S s_src{ .x= 987789 };
			with( s : s_src ) // Should copy 's_src' here.
			{
				res= s.x; // Should read member here before 's' destruction.
			}
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 987789 )


def WithOperatorForValue_Test5():
	c_program_text= """
		fn Foo() : i32
		{
			with( mut x : 9991 ) // Create mutable value.
			{
				x+= 8;
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 9999 )


def WithOperatorForImutReference_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			with( &imut x : 333222111 ) // Bind value to immutable reference.
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 333222111 )


def WithOperatorForImutReference_Test1():
	c_program_text= """
		fn Foo() : i32
		{
			auto s= 852741;
			with( &imut x : s ) // Bind immutable reference to immutable reference.
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 852741 )


def WithOperatorForImutReference_Test2():
	c_program_text= """
		fn Foo() : i32
		{
			auto mut s= 963147;
			with( &imut x : s ) // Bind mutable reference to immutable reference.
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 963147 )


def WithOperatorForImutReference_Test3():
	c_program_text= """
		fn Foo()
		{
			auto mut s= 963147;
			with( &imut x : s )
			{
				++s; // Error, 's' have reference - 'x'.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def WithOperatorForImutReference_Test4():
	c_program_text= """
		struct S
		{
			i32 x;
			fn destructor(){ x= 0; }
		}
		fn GetS() : S
		{
			var S s{ .x= 654123 };
			return s;
		}
		fn Foo() : i32
		{
			with( &imut s : GetS() ) // Bind temporary value to immutable reference.
			{
				return s.x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 654123 )


def WithOperatorForImutReference_Test5():
	c_program_text= """
		struct S
		{
			i32 x;
			fn destructor(){ x= 0; }
		}
		fn GetS() : S
		{
			var S s{ .x= 111118 };
			return s;
		}
		fn Foo() : i32
		{
			with( &imut x : GetS().x ) // Bind part of temporary value to immutable reference.
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 111118 )


def WithOperatorForImutReference_Test6():
	c_program_text= """
		struct S
		{
			i32 x;
			fn destructor(){ x= 0; }
		}
		fn GetS() : S
		{
			var S s{ .x= 999996 };
			return s;
		}
		fn Foo() : i32
		{
			var i32 mut res= zero_init;
			with( &imut s : GetS() ) // Bind temporary value to immutable reference.
			{
				res= s.x; // Should read member here before 's' destruction.
			}
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999996 )


def WithOperatorForMutReference_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut res= 0;
			with( &mut x : res )
			{
				x= 29567;
			}
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 29567 )


def WithOperatorForMutReference_Test1():
	c_program_text= """
		fn Foo()
		{
			with( &mut x : 66 ) //Ok, binding value to mutable reference.
			{
				++x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorForConstexprValue_Test0():
	c_program_text= """
		fn Foo()
		{
			with( x : 66 ) // Bind value to value.
			{
				static_assert( x == 66 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorForConstexprValue_Test1():
	c_program_text= """
		fn Foo()
		{
			with( &x : 73.5f ) // Bind value to reference.
			{
				static_assert( x == 73.5f );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorForConstexprValue_Test2():
	c_program_text= """
		fn Foo()
		{
			var u32 x= 78643u;
			with( x_ref : x ) // Bind reference to value.
			{
				static_assert( x_ref == 78643u );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorForConstexprValue_Test3():
	c_program_text= """
		fn Foo()
		{
			auto x= -786i64;
			with( &x_ref : x ) // Bind reference to reference.
			{
				static_assert( x_ref == -786i64 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorForConstexprValue_Test4():
	c_program_text= """
		fn Foo()
		{
			with( mut x : 7654 ) // Bind value to mutable value.
			{
				static_assert( x == 7654 ); // "x" initialized as constant, but not is an actual constant because it is mutable.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StaticAssertExpressionIsNotConstant", 6 ) )


def WithOperatorForConstexprValue_Test5():
	c_program_text= """
		fn Foo()
		{
			var i32 v= 44445;
			with( mut x : v ) // Bind reference to mutable value.
			{
				static_assert( x == 44445 ); // "x" initialized as constant, but not is an actual constant because it is mutable.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StaticAssertExpressionIsNotConstant", 7 ) )


def BindingConstReferenceToNonconstReference_For_WithOperator_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 p= 0;
			with( &mut x : p ) // Binding immatable reference to mutable reference.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "BindingConstReferenceToNonconstReference" )
	assert( errors_list[0].src_loc.line == 5 )


def UsingKeywordAsName_For_WithOperator_Test0():
	c_program_text= """
		fn Foo()
		{
			with( class : 66 )
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingKeywordAsName" )
	assert( errors_list[0].src_loc.line == 4 )


def WithOperatorVariableShadowsOuterVariables_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var f32 mut x= 0.0f;
			with( &x : 959595 )
			{
				return x; // Should see "with" operator variable here, not external variable.
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 959595 )


def WithOperatorVariableShadowsOuterVariables_Test1():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 x= 5;
			with( &x : x * 7 ) // Outer variable is visible in expression
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5 * 7 )


def TemporariesSaved_In_WithOperator_Test0():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor( i32 in_x )( x= in_x ) {}
			fn destructor(){ x= 0; }
		}
		fn Foo() : i32
		{
			var i32 mut res= 0;
			with( &x : S(998877).x ) // Create temporary of type 'S' and bind its member to reference.
			{
				res= x;
			} // 's' should be destroyed here
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 998877 )


def TemporariesSaved_In_WithOperator_Test1():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor( i32 in_x )( x= in_x ) {}
			fn destructor(){ x= 0; }
		}
		struct R
		{
			S& s;
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( this, S & in_s ) @(pollution) ( s= in_s ) {}
		}
		fn Foo() : i32
		{
			var i32 mut res= 0;
			with( r : R(S(66123)) ) // Create temporary of type 'S', then create tomporary of type 'R', which has reference to 's', then temporary of type 'R' to 'r'.
			{
				res= r.s.x;
			} // 's' should be destroyed here
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 66123 )


def WithOperator_ForAbstractValue_Test0():
	c_program_text= """
		class A abstract {}
		fn GetA() : A&;
		fn Foo()
		{
			with( a : GetA() ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstructingAbstractClassOrInterface", 6 ) )
