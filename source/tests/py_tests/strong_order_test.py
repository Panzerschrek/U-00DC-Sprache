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
