from py_tests_common import *


def TakeForPart_Test0():
	c_program_text= """
		// Take from struct.
		struct S
		{
			i32 x;
			fn constructor() ( x= 0 ) {}
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { x= -1; }
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		struct T{ S s; }
		fn Foo()
		{
			var T mut t{ .s(666) };
			var S s= take(t.s);
			halt if( t.s.x != 0 );
			halt if( s.x != 666 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TakeForPart_Test1():
	c_program_text= """
		// Take from array.
		struct S
		{
			i32 x;
			fn constructor() ( x= 0 ) {}
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { x= -1; }
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		fn Foo()
		{
			var [ S, 3 ] mut arr[ (55), (77), (99) ];
			var S s= take(arr[1]);
			halt if( arr[1].x != 0 );
			halt if( s.x != 77 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TakeForValueVariable_Test0():
	c_program_text= """
		// Take temp value.
		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { x= -1; }
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		fn Foo()
		{
			var S s= take(S(56789));
			halt if(s.x != 56789);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TakeForValueVariable_Test1():
	c_program_text= """
		// Take value, returned from function.
		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { x= -1; }
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		fn GetS() : S
		{
			return S(321);
		}
		fn Foo()
		{
			var S s= take(GetS());
			halt if(s.x != 321);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TakeForValueVariable_Test2():
	c_program_text= """
		// Take moved value.
		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { x= -1; }
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		fn Foo()
		{
			var S mut s0(5555);
			var S s1= take(move(s0));
			halt if(s1.x != 5555);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TakeForConstReference_Test0():
	c_program_text= """
		// Take from struct.
		struct S
		{
			i32 x;
			fn constructor() ( x= 0 ) {}
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { x= -1; }
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		struct T{ S s; }
		fn Foo()
		{
			var T t{ .s(666) };
			take(t.s);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].src_loc.line == 16 )


def TakeForConstReference_Test1():
	c_program_text= """
		// Take from array.
		struct S
		{
			i32 x;
			fn constructor() ( x= 0 ) {}
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn destructor() { x= -1; }
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		fn Foo()
		{
			var [ S, 3 ] arr[ (1), (2), (3) ];
			take(arr[1]);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].src_loc.line == 15 )


def TakenVariableHaveReferences_Test0():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor() ( x = 0 ) {}
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		struct T{ S s; }
		fn Foo()
		{
			var T mut t{ .s(666) };
			auto& ref= t; // Reference to variable.
			take(t.s);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MovedVariableHasReferences" )
	assert( errors_list[0].src_loc.line == 15 )


def TakenVariableHaveReferences_Test1():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor() ( x = 0 ) {}
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		struct T{ S s; }
		fn Foo()
		{
			var T mut t{ .s(666) };
			auto& ref= t.s; // Reference to member.
			take(t.s);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MovedVariableHasReferences" )
	assert( errors_list[0].src_loc.line == 15 )


def TakenVariableHaveReferences_Test2():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor() ( x = 0 ) {}
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		struct T{ S s; }
		fn Foo()
		{
			var T mut t{ .s(666) };
			auto &mut ref= t.s; // Mutable reference to member.
			take(t.s);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MovedVariableHasReferences" )
	assert( errors_list[0].src_loc.line == 15 )


def TakenVariableHaveReferences_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor()( x= 0 ) {}
			fn constructor( i32 in_x ) ( x= in_x ) {}
		}
		struct T{ S s; }
		fn Bar(S &imut a, S b){}
		fn Foo()
		{
			var T mut t{ .s(666) };
			Bar(t.s, take(t.s)); // Reference exists in argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "MovedVariableHasReferences", 13 ) )


def InnereReferenceTransferedInTakeOperator_Test0():
	c_program_text= """
		struct S
		{
			i32& r;

			auto constexpr default_value= 0;
			fn constructor()( r= default_value ) {}
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( this, i32 & in_r ) @(pollution) ( r= in_r ) {}
		}
		fn Foo()
		{
			var i32 mut x= 0;
			var S mut s0;
			{
				var S mut s1(x);
				s0= take(s1);
			}
			++x; // 's0' contains reference to 'x'
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 19 )
