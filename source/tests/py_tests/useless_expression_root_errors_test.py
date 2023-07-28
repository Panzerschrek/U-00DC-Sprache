from py_tests_common import *

def UselessExpressionRoot_Test0():
	c_program_text= """
		fn Foo()
		{
			42; // Useless number.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test1():
	c_program_text= """
		fn Foo()
		{
			42 + 24; // Useless constant expression.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test2():
	c_program_text= """
		fn Foo(i32 x)
		{
			x; // Useless argument access.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test3():
	c_program_text= """
		fn Foo()
		{
			var f32 x= 0.0f;
			x; // Useless local variable access.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 5 ) )


def UselessExpressionRoot_Test4():
	c_program_text= """
		fn Foo(i32 x, i32 y)
		{
			x % y; // Useless binary operation for args.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test5():
	c_program_text= """
		fn Foo()
		{
			var f32 mut x= 0.0, mut y= 1.0;
			x + y; // Useless binary operation for local variables.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 5 ) )


def UselessExpressionRoot_Test6():
	c_program_text= """
		fn Foo(f64 x)
		{
			-x; // Useless unary operation for arg.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test7():
	c_program_text= """
		fn Foo()
		{
			var u64 mut x= 0;
			~x; // Useless unary operation for local variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 5 ) )


def UselessExpressionRoot_Test8():
	c_program_text= """
		fn Foo( S& s )
		{
			s.x; // Useless member access.
		}
		struct S{ i32 x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test9():
	c_program_text= """
		fn Foo( [i32, 1024]& arr, u32 index )
		{
			arr[index]; // Useless indexation for array.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test10():
	c_program_text= """
		fn Foo( tup[f32, bool, i32]& t )
		{
			t[1]; // Useless indexation for tuple.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test11():
	c_program_text= """
		fn Foo()
		{
			x; // Useless global variable access.
		}
		var i32 x= 0;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test12():
	c_program_text= """
		fn Foo()
		{
			x; // Useless global mutable variable access.
		}
		var i32 mut x= 0;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test13():
	c_program_text= """
		fn Foo()
		{
			Ns::x; // Useless global variable access, using complex name.
		}
		namespace Ns{ var i32 mut x= 0; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test14():
	c_program_text= """
		fn Foo(u16 mut x)
		{
			$<(x); // Useless conversion to pointer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test15():
	c_program_text= """
		fn Foo($(bool) b)
		{
			unsafe{ $>(b); } // Useless pointer dereference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test16():
	c_program_text= """
		fn Foo()
		{
			false; // Useless bool constant.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test17():
	c_program_text= """
		fn Foo()
		{
			"i am useless"; // Useless string literal.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test18():
	c_program_text= """
		fn Foo(i32 mut x)
		{
			move(x); // Move is NOT useless.
		}
	"""
	tests_lib.build_program( c_program_text )


def UselessExpressionRoot_Test19():
	c_program_text= """
		fn Foo(S &mut s)
		{
			take(s); // Take is NOT useless.
		}
		struct S{}
	"""
	tests_lib.build_program( c_program_text )


def UselessExpressionRoot_Test20():
	c_program_text= """
		fn Foo(i32 x)
		{
			cast_mut(x); // Useless cast.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test21():
	c_program_text= """
		fn Foo(i32 x)
		{
			cast_imut(x); // Useless cast.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test22():
	c_program_text= """
		fn Foo(B& b)
		{
			cast_ref</A/>(b); // Useless cast.
		}
		class A polymorph {}
		class B : A{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test23():
	c_program_text= """
		fn Foo(i32 x)
		{
			cast_ref_unsafe</f32/>(x); // Useless cast.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test24():
	c_program_text= """
		fn Foo()
		{
			typeinfo</[i32, 4]/>; // Useless typeinfo.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test25():
	c_program_text= """
		fn Foo()
		{
			non_sync</[i32, 4]/>; // Useless non_sync expression.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test26():
	c_program_text= """
		fn Foo( i32 x )
		{
			safe( x ); // Useless argument access inside "safe" expression.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test27():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			unsafe( x ); // Useless variable access inside "unsafe" expression.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 5 ) )


def UselessExpressionRoot_Test28():
	c_program_text= """
		fn Foo()
		{
			unsafe( Bar() ); // Ok - call inside "unsafe" expression is NOT useless.
		}
		fn Bar();
	"""
	tests_lib.build_program( c_program_text )


def UselessExpressionRoot_Test29():
	c_program_text= """
		fn Foo()
		{
			Bar(); // Ok - function call is NOT useless.
		}
		fn Bar();
	"""
	tests_lib.build_program( c_program_text )


def UselessExpressionRoot_Test30():
	c_program_text= """
		fn Foo()
		{
			Bar(); // Ok - even constexpr function call is NOT useless. Do this, because constexpr property may or may not exist for same code in constexpr context.
		}
		fn constexpr Bar() : i32 { return 42; }
	"""
	tests_lib.build_program( c_program_text )


def UselessExpressionRoot_Test31():
	c_program_text= """
		fn Foo(S& s)
		{
			s.Bar(); // Ok - method call is NOT useless.
		}
		struct S{ fn Bar(this); }
	"""
	tests_lib.build_program( c_program_text )


def UselessExpressionRoot_Test32():
	c_program_text= """
		fn Foo()
		{
			f32; // Useless type name access.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test33():
	c_program_text= """
		fn Foo()
		{
			E; // Useless enum type name access.
		}
		enum E{ A }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test34():
	c_program_text= """
		fn Foo()
		{
			S; // Useless struct type name access.
		}
		struct S{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test35():
	c_program_text= """
		fn Foo()
		{
			Bar; // Useless function set access.
		}
		fn Bar();
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test36():
	c_program_text= """
		fn Foo(S& s)
		{
			s.Bar; // Useless this methods set access.
		}
		struct S{ fn Bar(this); }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test37():
	c_program_text= """
		struct S
		{
			fn Foo(this)
			{
				x; // Useless access of this class field.
			}
			i32 x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 6 ) )


def UselessExpressionRoot_Test38():
	c_program_text= """
		struct S
		{
			fn Foo(this)
			{
				Bar; // Useless access of this overloaded methods set.
			}
			fn Bar(this);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 6 ) )


def UselessExpressionRoot_Test39():
	c_program_text= """
		fn Foo()
		{
			Bar() * Baz(); // Multiplication is useless, even if calls itself may be usefull.
		}
		fn Bar() : i32;
		fn Baz() : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test40():
	c_program_text= """
		fn Foo()
		{
			!Bar(); // Unsary operator is useless, even if call itself may be usefull.
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UselessExpressionRoot", 4 ) )


def UselessExpressionRoot_Test41():
	c_program_text= """
		fn Foo()
		{
			S(); // OK - temp variable construction. Sometimes it is usefull, if constructor has side-effects.
		}
		struct S{}
	"""
	tests_lib.build_program( c_program_text )


def UselessExpressionRoot_Test42():
	c_program_text= """
		fn Foo()
		{
			i32(4.5f); // OK - variable conversion. For now it is not considered useless, since there is no way to dostinguish between trivial and non-trivial temp variable construction.
		}
	"""
	tests_lib.build_program( c_program_text )
