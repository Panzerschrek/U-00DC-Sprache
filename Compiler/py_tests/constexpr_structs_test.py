from py_tests_common import *


def ConstexprStructDeclaration_Test0():
	c_program_text= """
		struct S{ i32 x; }
		var S constexpr s{ .x= 0 };
	"""
	tests_lib.build_program( c_program_text )


def ConstexprStructDeclaration_Test1():
	c_program_text= """
		struct S{ }
		var S constexpr s{ };
	"""
	tests_lib.build_program( c_program_text )


def ConstexprStructMemberIsConstexpr_Test0():
	c_program_text= """
		struct S{ i32 x; }
		var S constexpr s{ .x= 666 };
		static_assert( s.x == 666 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprStructMemberIsConstexpr_Test1():
	c_program_text= """
		struct S{ i32 x; }
		var S constexpr s= zero_init;
		static_assert( s.x == 0 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprStructMemberIsConstexpr_Test2():
	c_program_text= """
		struct S{ [ f32, 2 ] arr; }
		var S constexpr s{ .arr[ 3.14f, 0.25f ] };

		static_assert( s.arr[0u] == 3.14f );
		static_assert( s.arr[1u] == 0.25f );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprReferenceInsideStruct_Test0():
	c_program_text= """
		struct S{ u64& r; }

		auto constexpr x= 999854u64;
		var S constexpr s{ .r= x };

		static_assert( s.r == 999854u64 );
		static_assert( s.r == x );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprReferenceInsideStruct_Test1():
	c_program_text= """
		struct S{ [ i32, 2 ]& r; }

		var [ i32, 2 ] constexpr arr[ 999, 77785 ];
		var S constexpr s{ .r= arr };

		static_assert( s.r[0u] == 999 );
		static_assert( s.r[1u] == 77785 );
	"""
	tests_lib.build_program( c_program_text )


def ZeroInitForStructIsConstexpr_Test0():
	c_program_text= """
		struct S{ [ f32, 3 ] arr; }
		var S constexpr s= zero_init;

		static_assert( s.arr[0u] == 0.0f );
		static_assert( s.arr[1u] == 0.0f );
		static_assert( s.arr[2u] == 0.0f );
	"""
	tests_lib.build_program( c_program_text )


def ZeroInitForStructIsConstexpr_Test1():
	c_program_text= """
		struct S{ f32 f; }
		struct T{ S s; }
		var T constexpr t= zero_init;

		static_assert( t.s.f == 0.0f );
	"""
	tests_lib.build_program( c_program_text )


def EnumAsConstexprStructMember_Test0():
	c_program_text= """
		enum E{ A, B, C, }
		struct S{ E e; }

		var S constexpr s{ .e= E::B };
		static_assert( s.e == E::B );
	"""
	tests_lib.build_program( c_program_text )


def VariableInitializerIsNotConstantExpression_ForStructs_Test0():
	c_program_text= """
		fn Foo() : i32 { return 0; } // Function returns non-constexpr value.
		struct S{ i32 a; i32 b; }
		var S constexpr s{ .a= 0, .b= Foo() }; // Second initializer is not constant.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VariableInitializerIsNotConstantExpression" )
	assert( errors_list[0].file_pos.line == 4 )


def VariableInitializerIsNotConstantExpression_ForStructs_Test1():
	c_program_text= """
		struct S{ i32 a; i32 b; }
		fn Foo( i32 x )
		{
			var S constexpr s{ .a= x, .b= 0 }; // First initializer is not constant.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VariableInitializerIsNotConstantExpression" )
	assert( errors_list[0].file_pos.line == 5 )


def VariableInitializerIsNotConstantExpression_ForStructs_Test2():
	c_program_text= """
		fn GetTrue() : bool { return true; }
		struct S{ bool b; }
		var S constexpr s{ .b= GetTrue() };

	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VariableInitializerIsNotConstantExpression" )
	assert( errors_list[0].file_pos.line == 4 )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test0():
	c_program_text= """
		struct S
		{
			fn constructor(){}
		}

		var S constexpr s; // Error, 's' can not be constexpr, because it have explicit noncopy constructor.

	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstantExpressionVariable" )
	assert( errors_list[0].file_pos.line == 7 )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test1():
	c_program_text= """
		class T{} // 'T' can not be constexpr, because it is class.
		struct S
		{
			T t;
		}

		var S constexpr s; // Error, 's' can not be constexpr, because it have non-constexpr field 't'.

	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstantExpressionVariable" )
	assert( errors_list[0].file_pos.line == 8 )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test2():
	c_program_text= """
		struct T
		{
			fn constructor(){}
		} // 'T' con not be constexpr, because it have explicit noncopy constructor.
		struct S
		{
			[ T, 2 ] t;
		}

		var S constexpr s; // Error, 's' can not be constexpr, because it have non-constexpr field 't'.

	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstantExpressionVariable" )
	assert( errors_list[0].file_pos.line == 11 )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test3():
	c_program_text= """
		struct S { i32 &mut r; }  // struct can not be constexpr, because it contains mutable reference
		auto constexpr x= 0;
		var S constexpr s{ .r= x };

	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstantExpressionVariable" )
	assert( errors_list[0].file_pos.line == 4 )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test4():
	c_program_text= """
		struct S  // struct can not be constexpr, because it contains user-defined destructor.
		{
			fn destructor(){}
		}
		var S constexpr s;

	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstantExpressionVariable" )
	assert( errors_list[0].file_pos.line == 6 )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test5():
	c_program_text= """
		struct T
		{
			fn destructor(){}
		} // 'T' con not be constexpr, because it have explicit destructor constructor.
		struct S
		{
			[ T, 2 ] t;
		}

		var S constexpr s; // Error, 's' can not be constexpr, because it have non-constexpr field 't'.

	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstantExpressionVariable" )
	assert( errors_list[0].file_pos.line == 11 )
