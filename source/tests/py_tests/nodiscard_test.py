from py_tests_common import *


def NodiscardClassDeclaration_Test0():
	c_program_text= """
		struct SomeStruct nodiscard
		{}
		static_assert( typeinfo</SomeStruct/>.is_nodiscard );
	"""
	tests_lib.build_program( c_program_text )


def NodiscardClassDeclaration_Test1():
	c_program_text= """
		struct SomeStruct ordered nodiscard // "nodiscard" should be specified after "ordered".
		{
			i32 x;
			i32 y;
		}
		static_assert( typeinfo</SomeStruct/>.is_nodiscard );
	"""
	tests_lib.build_program( c_program_text )


def NodiscardClassDeclaration_Test2():
	c_program_text= """
		class SomeClass nodiscard // "nodiscard" for a class.
		{}
		static_assert( typeinfo</SomeClass/>.is_nodiscard );
	"""
	tests_lib.build_program( c_program_text )


def NodiscardClassDeclaration_Test3():
	c_program_text= """
		class SomeClass non_sync nodiscard // "nodiscard" after "non_sync"
		{}
		static_assert( typeinfo</SomeClass/>.is_nodiscard );
	"""
	tests_lib.build_program( c_program_text )


def NodiscardClassDeclaration_Test4():
	c_program_text= """
		class SomeClass polymorph nodiscard // "nodiscard" after "polymorph"
		{}
		static_assert( typeinfo</SomeClass/>.is_nodiscard );
	"""
	tests_lib.build_program( c_program_text )


def NodiscardClassDeclaration_Test5():
	c_program_text= """
		class SomeInterface interface {}
		class SomeClass : SomeInterface nodiscard // "nodiscard" after parents list
		{}
		static_assert( !typeinfo</SomeInterface/>.is_nodiscard );
		static_assert( typeinfo</SomeClass/>.is_nodiscard );
	"""
	tests_lib.build_program( c_program_text )


def NodiscardTypinfoField_Test0():
	c_program_text= """
		// Fundamental types aren't "nodiscard".
		static_assert( !typeinfo</void/>.is_nodiscard );
		static_assert( !typeinfo</bool/>.is_nodiscard );
		static_assert( !typeinfo</i32/>.is_nodiscard );
		static_assert( !typeinfo</f32/>.is_nodiscard );
		static_assert( !typeinfo</u128/>.is_nodiscard );
		// Raw pointers aren't "nodiscard"
		static_assert( !typeinfo</$(i32)/>.is_nodiscard );
		static_assert( !typeinfo</$( tup[ f32, u64, bool ] )/>.is_nodiscard );
		static_assert( !typeinfo</$( SomeStruct )/>.is_nodiscard );
		static_assert( !typeinfo</$( NodiscardStruct )/>.is_nodiscard );
		// Function pointers aren't "nodiscard".
		static_assert( !typeinfo</ fn( i32 x ) : f32 />.is_nodiscard );
		static_assert( !typeinfo</ fn( NodiscardStruct s ) : NodiscardStruct />.is_nodiscard );

		// Structs and classes declared as "nodiscard" are nodiscard.
		static_assert( !typeinfo</ SomeStruct />.is_nodiscard );
		static_assert( !typeinfo</ SomeClass />.is_nodiscard );
		static_assert( typeinfo</ NodiscardStruct />.is_nodiscard );
		static_assert( typeinfo</ NodescardClass />.is_nodiscard );

		// Composites over "nodiscard" classes are nodiscard.
		static_assert( !typeinfo</ [ SomeStruct, 4 ] />.is_nodiscard );
		static_assert( !typeinfo</ tup[ bool, SomeClass, i32 ] />.is_nodiscard );
		static_assert( typeinfo</ tup[ NodiscardStruct, SomeStruct ] />.is_nodiscard );
		static_assert( typeinfo</ [ NodescardClass, 0 ] />.is_nodiscard );

		struct SomeStruct{}
		class SomeClass{}
		struct NodiscardStruct nodiscard {}
		class NodescardClass nodiscard {}
	"""
	tests_lib.build_program( c_program_text )


def NodiscardTypinfoField_Test1():
	c_program_text= """
		struct NodiscardStruct nodiscard {}
		static_assert( typeinfo</ NodiscardStruct />.is_nodiscard );

		// Having a field of a "nodiscard" type doesn't make this struct "nodiscard".
		struct SomeStruct { NodiscardStruct f; }
		static_assert( !typeinfo</ SomeStruct />.is_nodiscard );

		// Having value/reference of a "nodiscard" type doesn't make this class "nodiscard".
		class SomeClass
		{
			NodiscardStruct f0;
			NodiscardStruct& f1;
		}
		static_assert( !typeinfo</ SomeClass />.is_nodiscard );
	"""
	tests_lib.build_program( c_program_text )


def DiscardingValueOfNodiscardType_Test0():
	c_program_text= """
		struct SomeStruct nodiscard {}
		fn Bar() : SomeStruct;
		fn Foo()
		{
			Bar(); // Discard result of function call of "nodiscard" type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DiscardingValueOfNodiscardType", 6 ) )


def DiscardingValueOfNodiscardType_Test1():
	c_program_text= """
		struct SomeStruct nodiscard {}
		fn Bar() : SomeStruct &;
		fn Foo()
		{
			Bar(); // Discard reference result of function call of "nodiscard" type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DiscardingValueOfNodiscardType", 6 ) )


def DiscardingValueOfNodiscardType_Test2():
	c_program_text= """
		struct SomeStruct nodiscard {}
		fn Bar() : SomeStruct &mut;
		fn Foo()
		{
			Bar(); // Discard mutable reference result of function call of "nodiscard" type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DiscardingValueOfNodiscardType", 6 ) )


def DiscardingValueOfNodiscardType_Test3():
	c_program_text= """
		struct SomeStruct nodiscard {}
		fn Bar() : tup[ bool, SomeStruct, i32 ];
		fn Foo()
		{
			Bar(); // Discard result of function call of "nodiscard" type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DiscardingValueOfNodiscardType", 6 ) )


def DiscardingValueOfNodiscardType_Test4():
	c_program_text= """
		class SomeClass nodiscard {}
		fn Bar() : [ SomeClass, 4 ] &;
		fn Foo()
		{
			Bar(); // Discard rererence result of function call of "nodiscard" type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DiscardingValueOfNodiscardType", 6 ) )


def DiscardingValueOfNodiscardType_Test5():
	c_program_text= """
		struct SomeStruct nodiscard {}
		fn Foo()
		{
			SomeStruct(); // Discard result of temporary variable construction of "nodiscard" type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DiscardingValueOfNodiscardType", 5 ) )


def DiscardingValueOfNodiscardType_Test6():
	c_program_text= """
		struct SomeStruct nodiscard {}
		fn Foo( SomeStruct mut s )
		{
			move(s); // Fine - it's fine to move values of nodiscard types with following destruction.
		}
	"""
	tests_lib.build_program( c_program_text )


def DiscardingValueOfNodiscardType_Test7():
	c_program_text= """
		class SomeStruct nodiscard {}
		fn Foo( [ SomeStruct, 16 ]& arr )
		{
			arr[3]; // Discard rererence result of operator [] for "nodiscard" type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DiscardingValueOfNodiscardType", 5 ) )


def DiscardingValueOfNodiscardType_Test8():
	c_program_text= """
		class SomeStruct nodiscard {}
		class C
		{
			op[]( this, i32 x ) : SomeStruct;
		}
		fn Foo( C& c )
		{
			c[ 42 ]; // Discard result of overloaded operator [] call of "nodiscard" type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DiscardingValueOfNodiscardType", 9 ) )


def DiscardingValueOfNodiscardType_Test9():
	c_program_text= """
		class SomeStruct nodiscard {}
		fn Foo( SomeStruct& s )
		{
			s; // Even accessing a variable of a "nodiscard" type without doing anything with it is an error.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DiscardingValueOfNodiscardType", 5 ) )


def DiscardingValueOfNodiscardType_Test10():
	c_program_text= """
		struct SomeStruct nodiscard {}
		fn async Bar() : SomeStruct;
		fn async Foo()
		{
			Bar().await; // Discard result of async function call of "nodiscard" type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DiscardingValueOfNodiscardType", 6 ) )
