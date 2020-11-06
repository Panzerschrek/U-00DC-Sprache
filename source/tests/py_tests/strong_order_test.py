from py_tests_common import *


def VirtualTableOrder_Test0():
	c_program_text= """
		class C polymorph
		{
			fn virtual B( this ){}
			fn virtual A( this ){}
			fn virtual C( this ){}
			fn virtual Foo2( this ){}
			fn virtual Foo0( this ){}
			fn virtual Foo1( this ){}
		}

		type CMethod= fn( C& c );

		// This struct should have same layout, as compiler-generated virtual table.
		struct VirtualTable ordered
		{
			size_type offset;
			size_type type_id;
			[ CMethod, 128 ] functions;
		}

		struct P ordered
		{
			VirtualTable& virtual_table;
		}

		fn Foo()
		{
			var C c;
			unsafe
			{
				var P& c_casted= cast_ref_unsafe</P/>( c );
				var VirtualTable& virtual_table= c_casted.virtual_table;

				var ( fn( C &mut c ) ) destructor_ptr= C::destructor;

				// Virtual functions should be sorted in order of mangled name.
				halt if( virtual_table.functions[0] != cast_ref_unsafe</CMethod/>( destructor_ptr ) );
				halt if( virtual_table.functions[1] != CMethod( C::A ) );
				halt if( virtual_table.functions[2] != CMethod( C::B ) );
				halt if( virtual_table.functions[3] != CMethod( C::C ) );
				halt if( virtual_table.functions[4] != CMethod( C::Foo0 ) );
				halt if( virtual_table.functions[5] != CMethod( C::Foo1 ) );
				halt if( virtual_table.functions[6] != CMethod( C::Foo2 ) );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VirtualTableOrder_Test1():
	c_program_text= """
		class CBase polymorph
		{
			fn virtual Foo3( this ){}
			fn virtual Foo2( this ){}
			fn virtual Foo0( this ){}
			fn virtual Foo1( this ){}
			fn virtual Foo4( this ){}
		}
		class I interface
		{
			fn virtual pure I2( this );
			fn virtual pure I0( this );
			fn virtual pure I1( this );
		}
		class C : I, CBase
		{
			fn virtual B( this ){}
			fn virtual A( this ){}
			fn virtual C( this ){}
			fn virtual override Foo2( this ){}
			fn virtual override I0( this ){}
			fn virtual override I2( this ){}
			fn virtual override I1( this ){}
		}

		type CBaseMethod= fn( CBase& c );
		type CMethod= fn( C& c );

		// This struct should have same layout, as compiler-generated virtual table.
		struct VirtualTable ordered
		{
			size_type offset;
			size_type type_id;
			[ CMethod, 128 ] functions;
		}

		struct P ordered
		{
			VirtualTable& virtual_table;
		}

		fn Foo()
		{
			var C c;
			unsafe
			{
				var P& c_casted= cast_ref_unsafe</P/>( c );
				var VirtualTable& virtual_table= c_casted.virtual_table;

				var ( fn( C &mut c ) ) destructor_ptr= C::destructor;
				var CBaseMethod fn_foo0= C::Foo0, fn_foo1= C::Foo1, fn_foo3= C::Foo3, fn_foo4= C::Foo4;

				// Virtual functions of base class should be placed before virtual functions of derived class.
				// Base class should be first, than, other parent classes (interfaces) in order of declaration.

				// Methods of "CBase" class.
				halt if( virtual_table.functions[0] != cast_ref_unsafe</CMethod/>( destructor_ptr ) );
				halt if( virtual_table.functions[1] != cast_ref_unsafe</CMethod/>( fn_foo0 ) );
				halt if( virtual_table.functions[2] != cast_ref_unsafe</CMethod/>( fn_foo1 ) );
				halt if( virtual_table.functions[3] != CMethod( C::Foo2 ) );
				halt if( virtual_table.functions[4] != cast_ref_unsafe</CMethod/>( fn_foo3 ) );
				halt if( virtual_table.functions[5] != cast_ref_unsafe</CMethod/>( fn_foo4 ) );
				// "I" virtual table start.
				halt if( cast_ref_unsafe</size_type/>(virtual_table.functions[6]) == 0s ); // "I" should have non-zero offset.
				halt if( cast_ref_unsafe</size_type/>(virtual_table.functions[7]) != virtual_table.type_id ); // Type id for "I" is same as for "C"
				// Methods of "I" interface.
				halt if( virtual_table.functions[ 8] != cast_ref_unsafe</CMethod/>( destructor_ptr ) );
				halt if( virtual_table.functions[ 9] != CMethod( C::I0 ) );
				halt if( virtual_table.functions[10] != CMethod( C::I1 ) );
				halt if( virtual_table.functions[11] != CMethod( C::I2 ) );
				// New methods of "C" class.
				halt if( virtual_table.functions[12] != CMethod( C::A ) );
				halt if( virtual_table.functions[13] != CMethod( C::B ) );
				halt if( virtual_table.functions[14] != CMethod( C::C ) );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TypeinfoFunctionParamsList_Order_Test0():
	c_program_text= """
		struct S{}
		type FnPtr= fn( i32 x, f32 y, bool z, S& s );
		auto& ti= typeinfo</FnPtr/>.element_type;

		static_assert( ti.arguments_list[0].type.is_signed_integer );
		static_assert( !ti.arguments_list[0].is_reference );

		static_assert( ti.arguments_list[1].type.is_float );
		static_assert( !ti.arguments_list[1].is_reference );

		static_assert( ti.arguments_list[2].type.is_bool );
		static_assert( !ti.arguments_list[2].is_reference );

		static_assert( ti.arguments_list[3].type.is_class );
		static_assert( ti.arguments_list[3].is_reference );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoTupleElementsListList_Order_Test0():
	c_program_text= """
		struct S{}
		type t0= tup[ i32, f32, bool, S ];
		type t1= tup[ S, i32, bool, f32 ];
		auto& ti0= typeinfo</t0/>;
		auto& ti1= typeinfo</t1/>;

		// Elemenst in typeinfo have same order as in original tuple.
		// Tuple elements placed in memory continiously.

		static_assert( ti0.elements_list[0].type.is_signed_integer );
		static_assert( ti0.elements_list[1].type.is_float );
		static_assert( ti0.elements_list[2].type.is_bool );
		static_assert( ti0.elements_list[3].type.is_class );

		static_assert( ti0.elements_list[0].offset <= ti0.elements_list[1].offset );
		static_assert( ti0.elements_list[1].offset <= ti0.elements_list[2].offset );
		static_assert( ti0.elements_list[1].offset <= ti0.elements_list[3].offset );

		static_assert( ti1.elements_list[0].type.is_class );
		static_assert( ti1.elements_list[1].type.is_signed_integer );
		static_assert( ti1.elements_list[2].type.is_bool );
		static_assert( ti1.elements_list[3].type.is_float );

		static_assert( ti1.elements_list[0].offset <= ti1.elements_list[1].offset );
		static_assert( ti1.elements_list[1].offset <= ti1.elements_list[2].offset );
		static_assert( ti1.elements_list[1].offset <= ti1.elements_list[3].offset );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoClassFieldsList_Order_Test0():
	c_program_text= """
		struct S
		{
			i32 b;
			[ i32, 2 ] qwerty;
			f32 a;
			bool ar;
			void& fff;
		}
		// Order should be "a", "ar", "b", "fff", "qwerty"

		auto& ti= typeinfo</S/>;

		static_assert( ti.fields_list[0].type.is_float );
		static_assert( ti.fields_list[0].name[0] == "a"c8 );

		static_assert( ti.fields_list[1].type.is_bool );
		static_assert( ti.fields_list[1].name[0] == "a"c8 );

		static_assert( ti.fields_list[2].type.is_signed_integer );
		static_assert( ti.fields_list[2].name[0] == "b"c8 );

		static_assert( ti.fields_list[3].type.is_void );
		static_assert( ti.fields_list[3].name[0] == "f"c8 );

		static_assert( ti.fields_list[4].type.is_array );
		static_assert( ti.fields_list[4].name[0] == "q"c8 );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoClassFieldsList_Order_Test1():
	c_program_text= """
		class Base polymorph
		{
			f32 c;
			i16 a;
			[ f64, 8 ] e;
		}

		class Derived : Base
		{
			u32 b;
			bool d;
			tup[] f;
		}
		// Order should be "a", "b", "c", "d", "e", "f"

		auto& ti= typeinfo</Derived/>;

		static_assert( ti.fields_list[0].type.is_signed_integer );
		static_assert( ti.fields_list[0].name[0] == "a"c8 );

		static_assert( ti.fields_list[1].type.is_unsigned_integer );
		static_assert( ti.fields_list[1].name[0] == "b"c8 );

		static_assert( ti.fields_list[2].type.is_float );
		static_assert( ti.fields_list[2].name[0] == "c"c8 );

		static_assert( ti.fields_list[3].type.is_bool );
		static_assert( ti.fields_list[3].name[0] == "d"c8 );

		static_assert( ti.fields_list[4].type.is_array );
		static_assert( ti.fields_list[4].name[0] == "e"c8 );

		static_assert( ti.fields_list[5].type.is_tuple );
		static_assert( ti.fields_list[5].name[0] == "f"c8 );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoClassTypesList_Order_Test0():
	c_program_text= """
		struct S
		{
			type b= bool;
			type lolwat= tup[ f32, i32 ];
			type abc= f32;
			type wtf= i32;
			type arr= [ i32, 4 ];
		}
		// Order should be "abc", "arr", "b", "lolwat", "wtf"

		auto& ti= typeinfo</S/>;

		static_assert( ti.types_list[0].type.is_float );
		static_assert( ti.types_list[0].name[0] == "a"c8 );

		static_assert( ti.types_list[1].type.is_array );
		static_assert( ti.types_list[1].name[0] == "a"c8 );

		static_assert( ti.types_list[2].type.is_bool );
		static_assert( ti.types_list[2].name[0] == "b"c8 );

		static_assert( ti.types_list[3].type.is_tuple );
		static_assert( ti.types_list[3].name[0] == "l"c8 );

		static_assert( ti.types_list[4].type.is_signed_integer );
		static_assert( ti.types_list[4].name[0] == "w"c8 );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoClassTypesList_Order_Test1():
	c_program_text= """
		class Base polymorph
		{
			type Q= i8;
			type B= bool;
		}

		class Derived : Base
		{
			type A= [ f32, 16 ];
			type O= void;
		}
		// Order should be "A", "B", "Q", "O"

		auto& ti= typeinfo</Derived/>;

		static_assert( ti.types_list[0].type.is_array );
		static_assert( ti.types_list[0].name[0] == "A"c8 );

		static_assert( ti.types_list[1].type.is_bool );
		static_assert( ti.types_list[1].name[0] == "B"c8 );

		static_assert( ti.types_list[2].type.is_void );
		static_assert( ti.types_list[2].name[0] == "O"c8 );

		static_assert( ti.types_list[3].type.is_signed_integer );
		static_assert( ti.types_list[3].name[0] == "Q"c8 );
	"""
	tests_lib.build_program( c_program_text )
