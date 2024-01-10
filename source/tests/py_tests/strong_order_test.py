from py_tests_common import *


def ClassParentsOrder_Test0():
	c_program_text= """
		class Base polymorph {}
		class I0 interface {}
		class I1 interface {}
		class I2 interface {}

		class C : I1, I0, Base, I2 {}

		auto& ti= typeinfo</C/>;
		auto ptr_size= typeinfo</size_type/>.size_of;

		template</ type T /> fn MustBeSame( T& a, T& b ) : bool { return true; }

		// Base class always have zero offset.
		// Other parents (interfaces) are placed in order of declaration.
		// ORder in typeinfo is equal to order of declaration for all parents.

		static_assert( ti.parents_list[0].offset == ptr_size );
		static_assert( ti.parents_list[0].type.is_interface );
		static_assert( MustBeSame( ti.parents_list[0].type, typeinfo</I1/> ) );

		static_assert( ti.parents_list[1].offset == ptr_size * 2s );
		static_assert( ti.parents_list[1].type.is_interface );
		static_assert( MustBeSame( ti.parents_list[1].type, typeinfo</I0/> ) );

		static_assert( ti.parents_list[2].offset == 0s );
		static_assert( !ti.parents_list[2].type.is_interface );
		static_assert( MustBeSame( ti.parents_list[2].type, typeinfo</Base/> ) );

		static_assert( ti.parents_list[3].offset == ptr_size * 3s );
		static_assert( ti.parents_list[3].type.is_interface );
		static_assert( MustBeSame( ti.parents_list[3].type, typeinfo</I2/> ) );
	"""
	tests_lib.build_program( c_program_text )


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
			$(void) type_id;
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
			$(void) type_id;
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
				halt if( cast_ref_unsafe</$(void)/>(virtual_table.functions[7]) != virtual_table.type_id ); // Type id for "I" is same as for "C"
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
		auto& ti= typeinfo</FnPtr/>;

		static_assert( ti.params_list[0].type.is_signed_integer );
		static_assert( !ti.params_list[0].is_reference );

		static_assert( ti.params_list[1].type.is_float );
		static_assert( !ti.params_list[1].is_reference );

		static_assert( ti.params_list[2].type.is_bool );
		static_assert( !ti.params_list[2].is_reference );

		static_assert( ti.params_list[3].type.is_class );
		static_assert( ti.params_list[3].is_reference );
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
		// Order should be "b", "d", f" (parents fields are not listed )

		auto& ti= typeinfo</Derived/>;

		static_assert( ti.fields_list[0].type.is_unsigned_integer );
		static_assert( ti.fields_list[0].name[0] == "b"c8 );

		static_assert( ti.fields_list[1].type.is_bool );
		static_assert( ti.fields_list[1].name[0] == "d"c8 );

		static_assert( ti.fields_list[2].type.is_tuple );
		static_assert( ti.fields_list[2].name[0] == "f"c8 );
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
		// Order should be "A", "O" (parent types are not listed).

		auto& ti= typeinfo</Derived/>;

		static_assert( ti.types_list[0].type.is_array );
		static_assert( ti.types_list[0].name[0] == "A"c8 );

		static_assert( ti.types_list[1].type.is_void );
		static_assert( ti.types_list[1].name[0] == "O"c8 );
	"""
	tests_lib.build_program( c_program_text )


def TypeinfoClassFunctionsList_Order_Test0():
	c_program_text= """
		struct S
		{
			fn Foo(){}
			fn Bar( this ){}
			fn Lolwat( this, i32 x ){}
			fn Get( this, i32 x ){}
			fn Get( this, f32 x ){}
			fn Get( this ) {}
		}
		auto& ti= typeinfo</S/>;

		static_assert( StringEquals( ti.functions_list[0].name, "destructor" ) ); // _ZN1S10destructorERS_
		static_assert( StringEquals( ti.functions_list[1].name, "constructor" ) ); // _ZN1S11constructorERS_
		static_assert( StringEquals( ti.functions_list[2].name, "constructor" ) ); // _ZN1S11constructorERS_RKS_
		static_assert( StringEquals( ti.functions_list[3].name, "Bar" ) ); // _ZN1S3BarERKS_
		static_assert( StringEquals( ti.functions_list[4].name, "Foo" ) ); // _ZN1S3FooEv
		static_assert( StringEquals( ti.functions_list[5].name, "Get" ) ); // _ZN1S3GetERKS_

		// _ZN1S3GetERKS_f
		static_assert( StringEquals( ti.functions_list[6].name, "Get" ) );
		static_assert( ti.functions_list[6].type.params_list[1].type.is_float );

		// _ZN1S3GetERKS_f
		static_assert( StringEquals( ti.functions_list[7].name, "Get" ) );
		static_assert( ti.functions_list[7].type.params_list[1].type.is_signed_integer );

		static_assert( StringEquals( ti.functions_list[8].name, "Lolwat" ) ); // _ZN1S6LolwatERKS_i
		static_assert( StringEquals( ti.functions_list[9].name, "=" ) ); // _ZN1SaSERS_RKS_

		template</ size_type size0, size_type size1 />
		fn constexpr StringEquals( [ char8, size0 ]& s0, [ char8, size1 ]& s1 ) : bool
		{
			if( size0 != size1 ) { return false; }
			var size_type mut i(0);
			while( i < size0 )
			{
				if( s0[i] != s1[i] ) { return false; }
				++i;
			}
			return true;
		}

	"""
	tests_lib.build_program( c_program_text )


def TypeinfoEnumElementsList_Order_Test0():
	c_program_text= """
		enum E
		{
			b,
			qwerty,
			a,
			ar,
			fff,
		}
		// Order should be alphabetical - "a", "ar", "b", "fff", "qwerty"

		auto& ti= typeinfo</E/>;

		static_assert( ti.elements_list[0].value == u8(E::a) );
		static_assert( ti.elements_list[0].name[0] == "a"c8 );

		static_assert( ti.elements_list[1].value == u8(E::ar) );
		static_assert( ti.elements_list[1].name[0] == "a"c8 );

		static_assert( ti.elements_list[2].value == u8(E::b) );
		static_assert( ti.elements_list[2].name[0] == "b"c8 );

		static_assert( ti.elements_list[3].value == u8(E::fff) );
		static_assert( ti.elements_list[3].name[0] == "f"c8 );

		static_assert( ti.elements_list[4].value == u8(E::qwerty) );
		static_assert( ti.elements_list[4].name[0] == "q"c8 );
	"""
	tests_lib.build_program( c_program_text )


def ArgumenstEvaluationOrder_Test0():
	c_program_text= """
		fn AddMul10( i32 &mut x, i32 y ) : i32
		{
			x= x * 10 + y;
			return x;
		}

		fn Bar( i32 a, i32 b, i32 c ){}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			// Function arguments should be evaluated in direct order.
			Bar( AddMul10( x, 3 ), AddMul10( x, 7 ), AddMul10( x, 2 ) );
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 372 )


def ArgumenstEvaluationOrder_Test1():
	c_program_text= """
		fn AddMul10( i32 &mut x, i32 y ) : i32
		{
			x= x * 10 + y;
			return x;
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			// Binary operator argumenst should be evaluated in direct order.
			auto add_result= AddMul10( x, 5 ) * AddMul10( x, 3 );
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 53 )


def ArgumenstEvaluationOrder_Test2():
	c_program_text= """
		fn AddMul10( i32 &mut x, i32 y ) : i32
		{
			x= x * 10 + y;
			return x;
		}

		struct S
		{
			fn constructor()= default;
			fn constructor( i32 x ) {}
			op+( S& a, S& b ) : S { return S(); }
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			// Overloaded binary operator argumenst should be evaluated in direct order.
			auto res= S( AddMul10( x, 7 ) ) + S( AddMul10( x, 1 ) );
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 71 )


def ArgumenstEvaluationOrder_Test3():
	c_program_text= """
		var [ [ char8, 2 ], 1 ] return_references[ "2_" ];
		fn AddMul10Pass( i32 &mut x, i32 y, i32 & mut z ) : i32 &mut @(return_references)
		{
			x= x * 10 + y;
			return z;
		}

		fn Foo()
		{
			var i32 mut x= 0, mut y= 66, mut z= 5;
			// Additive assignment operator argumenst should be evaluated in reverse order.
			AddMul10Pass( x, 3, y ) /= AddMul10Pass( x, 8, z );

			halt if( x != 83 );
			halt if( y != 66 / 5 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def ArgumenstEvaluationOrder_Test4():
	c_program_text= """
		var [ [ char8, 2 ], 1 ] return_references[ "2_" ];
		fn AddMul10Pass( i32 &mut x, i32 y, S & mut z ) : S &mut @(return_references)
		{
			x= x * 10 + y;
			return z;
		}

		struct S
		{
			fn constructor( i32 x ) (v=x) {}
			op/=( mut this, S& b ){ this.v /= b.v; }

			i32 v;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var S mut a(675), mut b(12);
			// Overloaded additive assignment operator argumenst should be evaluated in reverse order.
			AddMul10Pass( x, 4, a ) /= AddMul10Pass( x, 9, b );

			halt if( x != 94 );
			halt if( a.v != 675 / 12 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def ArgumenstEvaluationOrder_Test6():
	c_program_text= """
		fn AddMul10( i32 &mut x, i32 y ) : i32
		{
			x= x * 10 + y;
			return x;
		}

		struct S
		{
			op()( this, i32 a, i32 b, i32 c ){}
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			var S s;
			// overloaded () operator arguments should be evaluated in direct order.
			s( AddMul10( x, 2 ), AddMul10( x, 1 ), AddMul10( x, 7 ) );
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 217 )


def StructElementsInitializationOrder_Test0():
	c_program_text= """
		fn AddMul10( i32 &mut x, i32 y ) : i32
		{
			x= x * 10 + y;
			return x;
		}

		struct S
		{
			[ i16, 1 ] f;
			i32 b;
			i32 a;
			bool c;
			f32 d;
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			// Initialization order should be equal to order in initializator.
			var S s
			{
				.b= AddMul10( x, 5 ),
				.a= AddMul10( x, 1 ),
				.f[ i16( AddMul10( x, 7 ) ) ],
				.c= AddMul10( x, 9 ) != 0,
				.d( AddMul10( x, 4 ) ),
			};
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 51794 )


def StructElementsInitializationOrder_Test1():
	c_program_text= """
		fn AddMul10( i32 &mut x, i32 y ) : i32
		{
			x= x * 10 + y;
			return x;
		}

		struct S
		{
			[ i16, 1 ] f;
			i32 b;
			i32 a;
			bool c;
			f32 d;

			fn constructor( i32 &mut x )
			// Initialization order should be equal to order in initializator.
			(
				b= AddMul10( x, 3 ),
				a= AddMul10( x, 6 ),
				f[ i16( AddMul10( x, 8 ) ) ],
				c= AddMul10( x, 7 ) != 0,
				d( AddMul10( x, 1 ) ),
			)
			{}
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			var S s(x);
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 36871 )


def ArrayElementsInitializationOrder_Test0():
	c_program_text= """
		fn AddMul10( i32 &mut x, i32 y ) : i32
		{
			x= x * 10 + y;
			return x;
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			// Initialization order should be equal to order in initializator.
			var [ i32, 6 ] a
			[
				AddMul10( x, 5 ),
				AddMul10( x, 2 ),
				AddMul10( x, 4 ),
				AddMul10( x, 6 ),
				AddMul10( x, 9 ),
				AddMul10( x, 1 ),
			];
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 524691 )


def TupleElementsInitializationOrder_Test0():
	c_program_text= """
		fn AddMul10( i32 &mut x, i32 y ) : i32
		{
			x= x * 10 + y;
			return x;
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			// Initialization order should be equal to order in initializator.
			var tup[ i32, f64, bool, f32, i16, char8 ] a
			[
				AddMul10( x, 2 ),
				f64( AddMul10( x, 3 ) ),
				AddMul10( x, 7 ) == 0,
				f32( AddMul10( x, 9 ) ),
				i16( AddMul10( x, 4 ) ),
				char8( AddMul10( x, 1 ) ),
			];
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 237941 )


def VariablesInitializationOrder_Test0():
	c_program_text= """
		fn AddMul10( i32 &mut x, i32 y ) : i32
		{
			x= x * 10 + y;
			return x;
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			// Initializator call order should be equal to declaration order.
			var i32 c= AddMul10( x, 7 ), B= AddMul10( x, 6 ), a= AddMul10( x, 1 ), d1= AddMul10( x, 5 );
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 7615 )


def VariablesInitializationOrder_Test1():
	c_program_text= """
		fn Foo() : i32
		{
			// Variables, became visible after initialization and can be used for next variables initializatio.
			var i32 a= 5, b= a + 7, c= b * 3, d= c - 1;
			return d;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == (5 + 7) * 3 - 1 )


def CStyleForOperator_IterationPartEvaluationOrder_Test0():
	c_program_text= """
		fn AddMul10( i32 &mut x, i32 y ) : i32
		{
			x= x * 10 + y;
			return x;
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			for( ; x == 0; AddMul10( x, 5 ), AddMul10( x, 2 ), AddMul10( x, 7 ), AddMul10( x, 1 ) )
			{}
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5271 )


def LambdaCaptureListOrder_Test0():
	c_program_text= """
		fn AddMul10( i32 &mut x, i32 y ) : i32
		{
			x= x * 10 + y;
			return x;
		}

		fn Foo() : i32
		{
			var i32 mut q= 0;
			// Expressions in lambda capture list should be evaluated in direct order.
			auto f=
				lambda [ a= AddMul10( q, 5 ), x= AddMul10( q, 7 ), b= AddMul10( q, 3 ), y= AddMul10( q, 2 ), c= AddMul10( q, 9 ), z= AddMul10( q, 6 ) ] () : i32
				{
					halt if( a != 5 );
					halt if( x != 57 );
					halt if( b != 573 );
					halt if( y != 5732 );
					halt if( c != 57329 );
					halt if( z != 573296 );
					return a * x + b * y + c * z;
				};
			f();
			return q;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 573296 )
