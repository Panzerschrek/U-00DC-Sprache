from py_tests_common import *


def MoveForPart_Test0():
	c_program_text= """
		// Move from struct.
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
			var S s= move(t.s);
			halt if( t.s.x != 0 );
			halt if( s.x != 666 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def MoveForPart_Test1():
	c_program_text= """
		// Move from array.
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
			var S s= move(arr[1]);
			halt if( arr[1].x != 0 );
			halt if( s.x != 77 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def MoveForValueVariable_Test0():
	c_program_text= """
		// Move temp value.
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
			var S s= move(S(56789));
			halt if(s.x != 56789);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def MoveForValueVariable_Test1():
	c_program_text= """
		// Move value, returned from function.
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
			var S s= move(GetS());
			halt if(s.x != 321);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def MoveForValueVariable_Test2():
	c_program_text= """
		// Move moved value.
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
			var S s1= move(move(s0));
			halt if(s1.x != 5555);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def MoveForConstReference_Test0():
	c_program_text= """
		// Move from struct.
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
			move(t.s);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].file_pos.line == 16 )


def MoveForConstReference_Test1():
	c_program_text= """
		// Move from array.
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
			move(arr[1]);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].file_pos.line == 15 )


def MovedVariableHaveReferences_Test0():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		struct T{ S s; }
		fn Foo()
		{
			var T mut t{ .s(666) };
			auto& ref= t; // Reference to variable.
			move(t.s);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MovedVariableHaveReferences" )
	assert( errors_list[0].file_pos.line == 14 )


def MovedVariableHaveReferences_Test1():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		struct T{ S s; }
		fn Foo()
		{
			var T mut t{ .s(666) };
			auto& ref= t.s; // Reference to member.
			move(t.s);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MovedVariableHaveReferences" )
	assert( errors_list[0].file_pos.line == 14 )


def MovedVariableHaveReferences_Test2():
	c_program_text= """
		struct S
		{
			i32 x;
			fn constructor( i32 in_x ) ( x= in_x ) {}
			fn constructor( mut this, S &imut other )= delete;
			op=( mut this, S &imut other )= delete;
		}
		struct T{ S s; }
		fn Foo()
		{
			var T mut t{ .s(666) };
			auto &mut ref= t.s; // Mutable reference to member.
			move(t.s);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MovedVariableHaveReferences" )
	assert( errors_list[0].file_pos.line == 14 )


def MovedVariableHaveReferences_Test3():
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
			Bar(t.s, move(t.s)); // Reference exists in argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 1 )
	assert( errors_list[1].error_code == "MovedVariableHaveReferences" )
	assert( errors_list[1].file_pos.line == 13 )
