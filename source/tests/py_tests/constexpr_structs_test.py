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
		struct S{ u64 & r; }

		auto constexpr x= 999854u64;
		var S constexpr s{ .r= x };

		static_assert( s.r == 999854u64 );
		static_assert( s.r == x );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprReferenceInsideStruct_Test1():
	c_program_text= """
		struct S{ [ i32, 2 ] & r; }

		var [ i32, 2 ] constexpr arr[ 999, 77785 ];
		var S constexpr s{ .r= arr };

		static_assert( s.r[0u] == 999 );
		static_assert( s.r[1u] == 77785 );
	"""
	tests_lib.build_program( c_program_text )


def ConstexprReferenceInsideStruct_Test2():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ f32& r; }
		var S constexpr s= zero_init;
		var T constexpr t{ .r= s.y };

		static_assert( t.r == 0.0f );
	"""
	tests_lib.build_program( c_program_text )


def FunctionPointerInConstexprStruct_Test0():
	c_program_text= """
		type fn_ptr= fn();
		struct S
		{
			fn_ptr ref;
		}

		fn Foo(){}

		var S constexpr s{ .ref= Foo };
		static_assert( s.ref == fn_ptr(Foo) );
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
	assert( errors_list[0].src_loc.line == 4 )


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
	assert( errors_list[0].src_loc.line == 5 )


def VariableInitializerIsNotConstantExpression_ForStructs_Test2():
	c_program_text= """
		fn GetTrue() : bool { return true; }
		struct S{ bool b; }
		var S constexpr s{ .b= GetTrue() };

	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VariableInitializerIsNotConstantExpression" )
	assert( errors_list[0].src_loc.line == 4 )


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
	assert( errors_list[0].src_loc.line == 8 )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test3():
	c_program_text= """
		struct S { i32 &mut r; }  // struct can not be constexpr, because it contains mutable reference
		auto constexpr x= 0;
		var S constexpr s{ .r= x };

	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstantExpressionVariable" )
	assert( errors_list[0].src_loc.line == 4 )


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
	assert( errors_list[0].src_loc.line == 6 )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test5():
	c_program_text= """
		struct T
		{
			fn destructor(){}
		} // 'T' con not be constexpr, because it have explicit destructor.
		struct S
		{
			[ T, 2 ] t;
		}

		var S constexpr s; // Error, 's' can not be constexpr, because it have non-constexpr field 't'.

	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstantExpressionVariable" )
	assert( errors_list[0].src_loc.line == 11 )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test6():
	c_program_text= """
		struct S
		{
			fn constructor( S& other ){}
		}
		var S constexpr s; // Error, 's' can not be constexpr, because it have non-default copy constructor.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstantExpressionVariable" )
	assert( errors_list[0].src_loc.line == 6 )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test7():
	c_program_text= """
		struct S
		{
			op=( mut this, S& other ) {}
		}
		var S constexpr s; // Error, 's' can not be constexpr, because it have non-default copy assignment operator.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstantExpressionVariable" )
	assert( errors_list[0].src_loc.line == 6 )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test8():
	c_program_text= """
		struct S
		{
			op==(S& a, S& b) : bool { return true; }
		}
		var S constexpr s; // Error, 's' can not be constexpr, because it have non-default equality compare operator.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InvalidTypeForConstantExpressionVariable" )
	assert( errors_list[0].src_loc.line == 6 )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test9():
	c_program_text= """
		struct S
		{
			i32 x;
			op==(S& a, S& b) : bool = default;
		}
		var S constexpr s{ .x = 0 }; // Ok, struct with generated "==" can be constexpr.
	"""
	tests_lib.build_program( c_program_text )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test10():
	c_program_text= """
		struct S
		{
			i32 x = 0;
			fn constructor() = default;
			fn constructor(i32 in_x) (x= in_x) {}
		}
		fn constexpr GetS() : S { var S s; return s; }
		var S constexpr s = GetS(); // Ok, struct with generated default constructor can be constexpr.
	"""
	tests_lib.build_program( c_program_text )


def InvalidTypeForConstantExpressionVariable_ForStructs_Test11():
	c_program_text= """
		struct S
		{
			i32 x = 0;
			fn constructor(mut this, S& other)= default;
			op=(mut this, S& other)= default;
		}
		var S constexpr s = zero_init; // Ok, struct with generated copy constructor and copy-assignment operatorcan be constexpr.
	"""
	tests_lib.build_program( c_program_text )
