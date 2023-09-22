from py_tests_common import *


def SameTypeDeclaration_Test0():
	c_program_text= """
		fn Foo() : bool
		{
			return same_type</ i32, f32 />;
		}
	"""
	tests_lib.build_program( c_program_text )


def SameTypeDeclaration_Test1():
	c_program_text= """
		var bool same= same_type</ u64, size_type />;
	"""
	tests_lib.build_program( c_program_text )


def SameType_test0():
	# Compare fundamental types against itself.
	c_program_text= """
		static_assert( same_type</ u8, u8 /> );
		static_assert( same_type</ f32, f32 /> );
		static_assert( same_type</ bool, bool /> );
		static_assert( same_type</ char8, char8 /> );
		static_assert( same_type</ byte16, byte16 /> );
	"""
	tests_lib.build_program( c_program_text )


def SameType_test1():
	# Compare composite types against itself.
	c_program_text= """
		static_assert( same_type</ [ i32, 4 ], [ i32, 4 ] /> );
		static_assert( same_type</ [ bool, 2 ], [ bool, 2 ] /> );
		static_assert( same_type</ [ [ f32, 4 ], 3 ], [ [ f32, 4 ], 3 ] /> );
		static_assert( same_type</ [ tup[ byte8, f32 ], 12 ], [ tup[ byte8, f32 ], 12 ] /> );
		static_assert( same_type</ tup[], tup[] /> );
		static_assert( same_type</ tup[ f32, i32 ], tup[ f32, i32 ] /> );
		static_assert( same_type</ tup[ f32, tup[ char8 ], bool ], tup[ f32, tup[ char8 ], bool ] /> );
		static_assert( same_type</ fn( i32 x, f32 y ), fn( i32 a, f32 b ) /> );
		static_assert( same_type</ fn() unsafe : bool, fn() unsafe : bool /> );
		static_assert( same_type</ fn() call_conv("C"), fn() call_conv("default") /> );
	"""
	tests_lib.build_program( c_program_text )


def SameType_test2():
	# Compare different types.
	c_program_text= """
		static_assert( !same_type</ char8, u8 /> );
		static_assert( !same_type</ char8, i8 /> );
		static_assert( !same_type</ char8, byte8 /> );
		static_assert( !same_type</ byte8, u8 /> );
		static_assert( !same_type</ byte8, i8 /> );
		static_assert( !same_type</ byte8, char8 /> );
		static_assert( !same_type</ u8, i8 /> );
		static_assert( !same_type</ u8, byte8 /> );
		static_assert( !same_type</ u8, char8 /> );
		static_assert( !same_type</ i8, u8 /> );
		static_assert( !same_type</ i8, byte8 /> );
		static_assert( !same_type</ i8, char8 /> );
		static_assert( !same_type</ [ i32, 2 ], [ i32, 3 ] /> );
		static_assert( !same_type</ [ i32, 2 ], [ f32, 2 ] /> );
		static_assert( !same_type</ tup[ f64, u32 ], tup[ u32, f64 ] /> );
		static_assert( !same_type</ [ i32, 2 ], tup[ i32, i32 ] /> );
		static_assert( !same_type</ [ i32, 2 ], i64 /> );
		static_assert( !same_type</ i64, tup[ i32, i32 ] /> );
		static_assert( !same_type</ fn( u32 x ), fn( i32 x ) /> );
		static_assert( !same_type</ fn(), fn() unsafe /> );
		static_assert( !same_type</ fn( i32& x ), fn( i32 x ) /> );
		static_assert( !same_type</ fn( i32 &imut x ), fn( i32 &mut x ) /> );
		static_assert( !same_type</ fn() call_conv("C"), fn() call_conv("fast") /> );
	"""
	tests_lib.build_program( c_program_text )


def SameType_test3():
	# Compare for enums.
	c_program_text= """
		enum Ampel{ Rot, Gelb, Grün }
		enum Светофор{ Красный, Жёлтый, Зелёный }
		static_assert( same_type</ Ampel, Ampel /> );
		static_assert( same_type</ Светофор, Светофор /> );
		static_assert( !same_type</ Ampel, Светофор /> );
		static_assert( !same_type</ Ampel, u8 /> );
		static_assert( !same_type</ Ampel, i8 /> );
		static_assert( !same_type</ Ampel, byte8 /> );
		static_assert( !same_type</ Ampel, char8 /> );
		static_assert( !same_type</ Ampel, [ i8, 1 ] /> );
		static_assert( !same_type</ Светофор, u8 /> );
		static_assert( !same_type</ Светофор, i8 /> );
		static_assert( !same_type</ Светофор, byte8 /> );
		static_assert( !same_type</ Светофор, char8 /> );
		static_assert( !same_type</ Светофор, tup[ u8 ] /> );
	"""
	tests_lib.build_program( c_program_text )


def SameType_test4():
	# Compare for classes.
	c_program_text= """
		struct S{}
		struct T{}
		class C{}
		class D{}
		static_assert(  same_type</ S, S /> );
		static_assert( !same_type</ S, T /> );
		static_assert( !same_type</ S, C /> );
		static_assert( !same_type</ S, D /> );
		static_assert( !same_type</ T, S /> );
		static_assert(  same_type</ T, T /> );
		static_assert( !same_type</ T, C /> );
		static_assert( !same_type</ T, D /> );
		static_assert( !same_type</ C, S /> );
		static_assert( !same_type</ C, T /> );
		static_assert(  same_type</ C, C /> );
		static_assert( !same_type</ C, D /> );
		static_assert( !same_type</ D, S /> );
		static_assert( !same_type</ D, T /> );
		static_assert( !same_type</ D, C /> );
		static_assert(  same_type</ D, D /> );
	"""
	tests_lib.build_program( c_program_text )


def SameType_test5():
	# Compare expands properly alternative naes.
	c_program_text= """
		struct S{}
		type S_alias= S;
		type Int= i32;
		type Float= f32;
		type IntVec3= [ Int, 3 ];
		enum E{ A, B, C }
		type E_alias= E;
		template</ type T /> type Pass= T;

		static_assert( same_type</ S, S_alias /> );
		static_assert( same_type</ Int, i32 /> );
		static_assert( same_type</ f32, Float /> );
		static_assert( same_type</ IntVec3, [ i32, 3s ] /> );
		static_assert( same_type</ E, E_alias /> );
		static_assert( same_type</ Pass</bool/>, bool /> );
		static_assert( same_type</ Pass</ tup[ f32, char16 ] />, tup[ f32, char16 ] /> );
		static_assert( same_type</ Pass</ S_alias />, S /> );
	"""
	tests_lib.build_program( c_program_text )


def SameType_test6():
	# same_type result is bool.
	c_program_text= """
		static_assert( same_type</ typeof( same_type</ i32, f32/> ), bool /> );
	"""
	tests_lib.build_program( c_program_text )


def SameTypeResult_IsValue_Test0():
	c_program_text= """
		fn Foo()
		{
			auto& s= same_type</ i8, u8 />; // Creating reference to temporary variable - result of "same_type".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedReferenceValue", 4 ) )


def SameTypeResult_IsValue_Test1():
	c_program_text= """
		fn Foo()
		{
			var bool& s= same_type</ char8, char8 />; // Creating reference to temporary variable - result of "same_type".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedReferenceValue", 4 ) )


def SameTypeResult_IsValue_Test2():
	c_program_text= """
		fn Foo() : bool&
		{
			return same_type</ char8, char8 />; // Returning reference to temporary variable - result of "same_type".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedReferenceValue", 4 ) )
