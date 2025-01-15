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
