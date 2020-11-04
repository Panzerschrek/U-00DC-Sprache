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
