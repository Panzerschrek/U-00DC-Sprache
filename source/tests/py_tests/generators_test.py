from py_tests_common import *


def SimpleGenerator_Test0():
	c_program_text= """
		fn generator SimpleGen() : i32
		{
			yield 42;
		}
		fn Foo()
		{
			auto mut gen= SimpleGen();
			auto mut advanced= false;
			if_coro_advance( x : gen )
			{
				advanced= true;
				halt if( x != 42 );
			}
			halt if( !advanced );

			advanced= false;
			if_coro_advance( x : gen )
			{
				advanced= true;
			}
			halt if( advanced );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleGenerator_Test1():
	c_program_text= """
		fn generator SimpleGen() : i32
		{
			yield 24;
			yield 13;
			yield -678;
		}
		fn Foo()
		{
			auto mut gen= SimpleGen();
			auto mut advanced= false;
			if_coro_advance( x : gen )
			{
				advanced= true;
				halt if( x != 24 );
			}
			halt if( !advanced );

			advanced= false;
			if_coro_advance( x : gen )
			{
				advanced= true;
				halt if( x != 13 );
			}
			halt if( !advanced );

			advanced= false;
			if_coro_advance( x : gen )
			{
				advanced= true;
				halt if( x != -678 );
			}
			halt if( !advanced );

			advanced= false;
			if_coro_advance( x : gen )
			{
				advanced= true;
			}
			halt if( advanced );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleGenerator_Test2():
	c_program_text= """
		fn generator SimpleGen() : u32
		{
			for( auto mut i= 0u; i < 12u; ++i )
			{
				yield i * i;
			}
		}
		fn Foo()
		{
			auto mut gen= SimpleGen();
			auto mut num_advanced= 0u;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != num_advanced * num_advanced );
					++num_advanced;
					continue;
				}
				break;
			}
			halt if( num_advanced != 12u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleGenerator_Test3():
	c_program_text= """
		fn generator SimpleGen() : u32
		{
			// Generator, that produces no value.
		}
		fn Foo()
		{
			auto mut gen= SimpleGen();
			if_coro_advance( x : gen ){ halt; }
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleGenerator_Test4():
	c_program_text= """
		struct S{ u32 x; u64 x2; }
		fn generator StructGen() : S
		{
			for( auto mut i= 0u; i < 16u; ++i )
			{
				var S s{ .x= i, .x2= u64(i) * u64(i) };
				yield s;
			}
		}
		fn Foo()
		{
			auto mut gen= StructGen();
			auto mut num_advanced= 0u;
			while( true )
			{
				if_coro_advance( s : gen )
				{
					halt if( s.x != num_advanced );
					halt if( s.x2 != u64(num_advanced) * u64(num_advanced) );
					++num_advanced;
					continue;
				}
				break;
			}
			halt if( num_advanced != 16u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleGenerator_Test5():
	c_program_text= """
		fn generator SimpleGen( u32 num ) : u32
		{
			for( auto mut i= 0u; i < num; ++i )
			{
				yield 100000u / (i + 1u);
			}
		}
		fn Bar(u32 x)
		{
			auto mut gen= SimpleGen(x);
			auto mut num_advanced= 0u;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != 100000u / (num_advanced + 1u) );
					++num_advanced;
					continue;
				}
				break;
			}
			halt if( num_advanced != x );
		}
		fn Foo()
		{
			Bar(0u);
			Bar(1u);
			Bar(2u);
			Bar(10u);
			Bar(64u);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleGenerator_Test6():
	c_program_text= """
		fn generator SquareGen( [u32, 2] size ) : u32
		{
			for( auto mut y= 0u; y < size[1]; ++y )
			{
				for( auto mut x= 0u; x < size[0]; ++x )
				{
					yield x * y;
				}
			}
		}
		fn Bar( [u32, 2] size )
		{
			auto mut gen= SquareGen( size );
			auto mut num_advanced= 0u;
			while( true )
			{
				if_coro_advance( val : gen )
				{
					halt if( val != (num_advanced / size[0]) * (num_advanced % size[0]) );
					++num_advanced;
					continue;
				}
				break;
			}
			halt if( num_advanced != size[0] * size[1] );
		}
		fn MakeSize( u32 x, u32 y) : [u32, 2]
		{
			var [u32, 2] arr[x, y];
			return arr;
		}
		fn Foo()
		{
			Bar( MakeSize(0u, 0u) );
			Bar( MakeSize(0u, 1u) );
			Bar( MakeSize(1u, 0u) );
			Bar( MakeSize(1u, 1u) );
			Bar( MakeSize(1u, 3u) );
			Bar( MakeSize(3u, 1u) );
			Bar( MakeSize(5u, 11u) );
			Bar( MakeSize(15u, 7u) );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorTypeName_Test0():
	c_program_text= """
		fn generator SimpleGen() : u32 {}
		fn Foo()
		{
			var generator : u32 gen= SimpleGen();
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorTypeName_Test1():
	c_program_text= """
		fn generator SimpleGen() : u32 {}
		fn Foo()
		{
			var generator : u32 mut gen= SimpleGen();
			var (generator : u32) &mut gen_ref= gen;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorTypeName_Test2():
	c_program_text= """
		fn generator SimpleGen() : u32 & {}
		fn Foo()
		{
			var generator : u32& gen= SimpleGen(); // Type name for generator that returns reference.
			var generator : u32 & &imut gen_ref= gen;
			var (generator : u32 &imut) & gen_ref_ref= gen_ref;
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorTypeName_Test3():
	c_program_text= """
		type FloatGen= generator : f32;
		type IntRefGen= generator : i32 &mut;
		type ArrayMutRefGen = ((( generator : [ u64, 4 ] &mut )));
	"""
	tests_lib.build_program( c_program_text )


def GeneratorTypeName_Test4():
	c_program_text= """
		struct S
		{
			generator : u64 gen; // Use generator type name as name for struct field.
		}
		fn generator SimpleGen() : u64
		{
			yield 7u64;
			yield 12345u64;
		}
		fn Foo()
		{
			var S mut s{ .gen= SimpleGen() };
			if_coro_advance( x : s.gen )
			{
				halt if( x != 7u64 );
			}
			if_coro_advance( x : s.gen )
			{
				halt if( x != 12345u64 );
			}
			if_coro_advance( x : s.gen ) { halt; }
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorTypeName_Test5():
	c_program_text= """
		type Gen= generator'some_tag' : i32;
		static_assert( typeinfo</Gen/>.references_tags_count == 1s );
	"""
	tests_lib.build_program( c_program_text )


def GeneratorTypeName_Test6():
	c_program_text= """
		type MutGen= generator'mut some_tag' : i32 &'some_tag;
		static_assert( typeinfo</MutGen/>.references_tags_count == 1s );
	"""
	tests_lib.build_program( c_program_text )


def GeneratorTypeName_Test7():
	c_program_text= """
		struct S{ i32 &imut x; }
		type ImutGen= generator'mut some_tag' : S'some_tag';
		static_assert( typeinfo</ImutGen/>.references_tags_count == 1s );
	"""
	tests_lib.build_program( c_program_text )
