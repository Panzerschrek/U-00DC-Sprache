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


def GeneratorReturn_Test0():
	c_program_text= """
		fn generator SimpleGen( bool cond ) : i32
		{
			yield 42;
			if( cond )
			{
				yield 24;
			}
			else
			{
				return; // Conditional return - stops evaluation of generator.
			}
		}
		fn Bar( bool cond )
		{
			auto mut gen= SimpleGen( cond );
			auto mut advanced= 0s;
			if_coro_advance( val : gen )
			{
				halt if( val != 42 );
				+++advanced;
			}
			if_coro_advance( val : gen )
			{
				halt if( val != 24 );
				+++advanced;
			}
			halt if( advanced != select( cond ? 2s : 1s ) );
		}
		fn Foo()
		{
			Bar(true);
			Bar(false);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorReturn_Test1():
	c_program_text= """
		fn generator SimpleGen( bool cond ) : i32
		{
			yield 66;
			if( !cond )
			{
				return; // Conditional return - stops evaluation of generator.
			}
			yield 77;
		}
		fn Bar( bool cond )
		{
			auto mut gen= SimpleGen( cond );
			auto mut advanced= 0s;
			if_coro_advance( val : gen )
			{
				halt if( val != 66 );
				+++advanced;
			}
			if_coro_advance( val : gen )
			{
				halt if( val != 77 );
				+++advanced;
			}
			halt if( advanced != select( cond ? 2s : 1s ) );
		}
		fn Foo()
		{
			Bar(true);
			Bar(false);
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorReturn_Test2():
	c_program_text= """
		fn generator SimpleGen( bool cond ) : i32
		{
			yield 17;
			if( cond )
			{
				return 19; // ""return" with a value in generator is equivalent to "yield" + empty "return".
			}
			yield 23;
		}
		fn Bar( bool cond )
		{
			auto mut gen= SimpleGen( cond );
			auto mut advanced= 0s;
			if_coro_advance( val : gen )
			{
				halt if( val != 17 );
				+++advanced;
			}
			if_coro_advance( val : gen )
			{
				halt if( val != select( cond ? 19 : 23 ) );
				+++advanced;
			}
			halt if( advanced != 2s );
		}
		fn Foo()
		{
			Bar(true);
			Bar(false);
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


def GeneratorTypeName_Test8():
	c_program_text= """
		type Gen= generator non_sync : i32;
		static_assert( non_sync</ Gen /> );
	"""


def GeneratorTypeName_Test9():
	c_program_text= """
		type Gen= generator'imut some_tag' non_sync : i32;
		static_assert( non_sync</ Gen /> );
	"""


def GeneratorTypeName_Test10():
	c_program_text= """
		type Gen= generator non_sync(false) : i32;
		static_assert( !non_sync</ Gen /> );
	"""


def GeneratorTypeName_AsTemplateSignatureArgument_Test0():
	c_program_text= """
		template</ type T />
		struct S</ generator : T />
		{
			type GenRet= T;
		}
		static_assert( typeinfo</ S</ generator : bool />::GenRet />.is_bool );
	"""
	tests_lib.build_program( c_program_text )


def GeneratorTypeName_AsTemplateSignatureArgument_Test1():
	c_program_text= """
		template</ type T />
		struct S</ generator : tup[T] />
		{
			type GenRet= T;
		}
		static_assert( typeinfo</ S</ generator : tup[f32] />::GenRet />.is_float );
	"""
	tests_lib.build_program( c_program_text )


def GeneratorTypeName_AsTemplateSignatureArgument_Test2():
	c_program_text= """
		template</ type T />
		struct S</ generator : T& />
		{
			type GenRet= T;
		}
		type Some= S</ generator : i32 />; // Deduction failed - expected generator, returning reference, given generator, returning value.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "TemplateParametersDeductionFailed", 7 ) )


def GeneratorTypeName_AsTemplateSignatureArgument_Test3():
	c_program_text= """
		template</ type T />
		struct S</ generator non_sync : T />
		{
			type GenRet= T;
		}
		type Some= S</ generator : i32 />; // Deduction failed - expected "non_sync" generator.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "TemplateParametersDeductionFailed", 7 ) )


def VoidTypeGenerator_Test0():
	c_program_text= """
		fn generator VoidGen() // Useless generator, but totally valid.
		{
			yield void();
			yield void();
			yield void();
		}
		fn Foo()
		{
			auto mut gen= VoidGen();
			auto mut advanced= 0;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != void() );
					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 3 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def VoidTypeGenerator_Test1():
	c_program_text= """
		// Useless generator, but totally valid.
		// Actually, void-return generators may be usable together with mutable-reference parameters or other kind of mutability.
		fn generator VoidGen()
		{
			yield; yield; // It is totally fine to use empty "yield" for void-return generator.
			yield; yield;
		}
		fn Foo()
		{
			auto mut gen= VoidGen();
			auto mut advanced= 0;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != void() );
					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 4 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorMethod_Test0():
	c_program_text= """
		struct S
		{
			fn generator SimpleGen() : i32 // Static generator method
			{
				yield 7;
				yield 14;
				yield 21;
			}
		}
		fn Foo()
		{
			auto mut gen= S::SimpleGen();
			auto mut advanced= 0;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != (advanced + 1) * 7 );
					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 3 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorMethod_Test1():
	c_program_text= """
		struct S
		{
			i32 x;
			fn generator SimpleGen(this) : i32 // Thiscall generator method.
			{
				yield x + 1;
				yield x + 2;
				yield x + 3;
				yield x + 4;
			}
		}
		fn Foo()
		{
			var S s{ .x= 42 };
			auto mut gen= s.SimpleGen();
			static_assert( typeinfo</ typeof(gen) />.references_tags_count == 1s ); // generator holds reference to "this".
			auto mut advanced= 0;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != 42 + 1 + advanced );
					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 4 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorMethod_Test2():
	c_program_text= """
		struct S
		{
			i32 x;
			fn generator SimpleGen( mut this, i32 step ) : i32 &mut // Thiscall generator method, that mutates "this".
			{
				for( auto mut i= 0s; i < 10s; ++i )
				{
					yield x;
					x += step;
				}
			}
		}
		fn Foo()
		{
			var S mut s{ .x= 17 };
			auto mut gen= s.SimpleGen(3);
			static_assert( typeinfo</ typeof(gen) />.references_tags_count == 1s ); // generator holds reference to "this".
			auto mut advanced= 0;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != 17 + 3 * advanced );
					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 10 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorMethod_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
			fn generator SimpleGen(this) : i32; // Thiscall generator method.
		}
		fn generator S::SimpleGen(this) : i32 // External implementation of generator method.
		{
			yield x + 10;
			yield x + 20;
			yield x + 30;
			yield x + 40;
		}
		fn Foo()
		{
			var S s{ .x= 777 };
			auto mut gen= s.SimpleGen();
			static_assert( typeinfo</ typeof(gen) />.references_tags_count == 1s ); // generator holds reference to "this".
			auto mut advanced= 0;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != 777 + (1 + advanced) * 10 );
					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 4 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorsNonTrivialUsage_Test0():
	c_program_text= """
		fn generator SimpleGen() : i32
		{
			yield 0;
			yield 1;
			yield 4;
			yield 9;
			yield 16;
		}
		fn MakeGen() : generator : i32 // This function returns object of generator type.
		{
			return SimpleGen();
		}
		fn Foo()
		{
			auto mut gen= MakeGen();
			auto mut advanced= 0;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != advanced * advanced );
					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 5 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorsNonTrivialUsage_Test1():
	c_program_text= """
		fn generator SimpleGen() : i32
		{
			yield 0;
			yield 1;
			yield 8;
			yield 27;
			yield 64;
		}
		fn MakeGen() : generator : i32
		{
			auto mut gen= SimpleGen();
			if_coro_advance( x : gen )
			{
				halt if( x != 0 );
			}
			return move(gen);
		}
		fn Foo()
		{
			auto mut gen= MakeGen(); // Obtain generator object, that was already advanced by one step.
			auto mut advanced= 1;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != advanced * advanced * advanced );
					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 5 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorsNonTrivialUsage_Test2():
	c_program_text= """
		fn generator SimpleGen() : i32
		{
			yield -0;
			yield -1;
			yield -2;
			yield -3;
		}
		fn ProcessGen( (generator : i32) mut gen )
		{
			auto mut advanced= 0;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != -advanced );
					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 4 );
		}
		fn Foo()
		{
			auto mut gen= SimpleGen();
			ProcessGen( move(gen) ); // Move value of generator type.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorsNonTrivialUsage_Test3():
	c_program_text= """
		fn generator SimpleGen(i32 add) : i32
		{
			yield add +  0;
			yield add +  2;
			yield add +  4;
			yield add +  6;
			yield add +  8;
			yield add + 10;
		}
		fn Foo()
		{
			// Use simultaniously two generator objects.
			var[ (generator : i32), 2 ] mut gens[ SimpleGen(1), SimpleGen(2) ];
			var [i32, 2] mut advanced[ 0, 0 ];
			while(true)
			{
				auto mut num_current_advanced= 0;
				if_coro_advance( x : gens[0] )
				{
					halt if( x != advanced[0] * 2 + 1 );
					++advanced[0];
					++num_current_advanced;
				}
				if_coro_advance( x : gens[1] )
				{
					halt if( x != advanced[1] * 2 + 2 );
					++advanced[1];
					++num_current_advanced;
				}

				if( num_current_advanced == 0 )
				{
					break;
				}
			}
			halt if( advanced[0] != 6 );
			halt if( advanced[1] != 6 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorsNonTrivialUsage_Test4():
	c_program_text= """
		// Use generator inside generator inside generator...
		fn generator GenNumbers( u32 max ) : u32
		{
			for( auto mut i= 0u; i < max; ++i )
			{
				yield i;
			}
		}
		fn generator GenSquareNumbers( u32 max ) : u32
		{
			auto mut gen= GenNumbers( max );
			while( true )
			{
				if_coro_advance( x : gen )
				{
					yield x * x;
					continue;
				}
				break;
			}
		}
		fn generator GenSquareNumbersPairs( u32 max ) : u32
		{
			auto mut gen= GenSquareNumbers( max );
			while( true )
			{
				if_coro_advance( x : gen )
				{
					yield x * 2u + 0u;
					yield x * 2u + 1u;
					continue;
				}
				break;
			}
		}
		fn Foo()
		{
			auto mut gen= GenSquareNumbersPairs( 17u );
			auto mut advanced= 0u;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != (advanced / 2u) * (advanced / 2u) * 2u + advanced % 2u );
					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 17u * 2u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorsNonTrivialUsage_Test5():
	c_program_text= """
		// Create twodimetional loop with generators.
		fn generator GenNumbers( u32 max ) : u32
		{
			for( auto mut i= 0u; i < max; ++i )
			{
				yield i;
			}
		}
		fn generator GenNumbersGrid( u32 width, u32 height ) : u32
		{
			auto mut height_gen= GenNumbers( height );
			while( true )
			{
				if_coro_advance( y : height_gen )
				{
					auto mut width_gen= GenNumbers( width );
					while( true )
					{
						if_coro_advance( x : width_gen )
						{
							yield x + y * width;
							continue;
						}
						break;
					}
					continue;
				}
				break;
			}
		}
		fn Foo()
		{
			auto mut gen= GenNumbersGrid( 5u, 9u );
			auto mut advanced= 0u;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != advanced );
					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 5u * 9u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorsNonTrivialUsage_Test6():
	c_program_text= """
		// Recursive generator.
		fn generator GenSequenceUpToPow2( u32 power ) : u32
		{
			if( power == 0u )
			{
				yield 0u;
				return;
			}

			auto mut gen= GenSequenceUpToPow2( power - 1u );
			while( true )
			{
				if_coro_advance( x : gen )
				{
					yield x * 2u + 0u;
					yield x * 2u + 1u;
					continue;
				}
				return;
			}
		}
		fn Foo()
		{
			auto mut gen= GenSequenceUpToPow2( 5u );
			auto mut advanced= 0u;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != advanced );
					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 1u << 5u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorsNonTrivialUsage_Test7():
	c_program_text= """
		fn generator GenNumbers( u32 size ) : u32
		{
			for( auto mut i= 0u; i < size; ++i ) { yield i; }
		}
		// Generator, that returns other generator.
		fn generator GenSequences( u32 size ) : (generator : u32)
		{
			for( auto mut i= 0u; i < size; ++i ) { yield GenNumbers(i); }
		}
		fn Foo()
		{
			auto mut gen= GenSequences( 6u );
			auto mut advanced= 0u;
			while( true )
			{
				if_coro_advance( mut inner_gen : gen )
				{
					auto mut inner_advanced= 0u;
					while(true)
					{
						if_coro_advance( x : inner_gen )
						{
							halt if( x != inner_advanced );
							++inner_advanced;
							continue;
						}
						break;
					}
					halt if( inner_advanced != advanced );

					++advanced;
					continue;
				}
				break;
			}
			halt if( advanced != 6u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorsNonTrivialUsage_Test8():
	c_program_text= """
		// Create generator inside "if_coro_advance"
		fn generator NoAdvanceGen() : i32 {}
		fn Foo()
		{
			if_coro_advance( x : NoAdvanceGen() ){ halt; }
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def GeneratorsNonTrivialUsage_Test9():
	c_program_text= """
		// Create generator inside "if_coro_advance"
		fn generator SingleAdvanceGen() : i32 { yield 42; }
		fn Foo()
		{
			auto mut advanced= false;
			if_coro_advance( x : SingleAdvanceGen() )
			{
				halt if( x != 42 );
				advanced= true;
			}
			halt if( !advanced );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TemplateGenerator_Test0():
	c_program_text= """
		template</type T/>
		fn generator GenNumbers(T max) : T
		{
			for( var T mut i(0); i < max; i += T(1) )
			{
				yield i;
			}
		}
		template</type T/>
		fn CheckGenNumbers( T max )
		{
			auto mut gen= GenNumbers( max ); // Use here template parameters deduction.
			auto mut advanced= 0;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != T(advanced) );
					++advanced;
					continue;
				}
				break;
			}
			halt if( T(advanced) != max );
		}
		fn Foo()
		{
			CheckGenNumbers( 7 );
			CheckGenNumbers( 11u );
			CheckGenNumbers( 13.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def TemplateGenerator_Test0():
	c_program_text= """
		template</type T/>
		fn generator SimpleGen() : T
		{
			yield T(7);
			yield T(77);
			yield T(777);
		}
		template</type T/>
		fn CheckSimpleGen()
		{
			auto mut gen= SimpleGen</T/>(); // Specify template args directly.
			auto mut advanced= 0;
			if_coro_advance( x : gen )
			{
				halt if( x != T(7) );
				++advanced;
			}
			if_coro_advance( x : gen )
			{
				halt if( x != T(77) );
				++advanced;
			}
			if_coro_advance( x : gen )
			{
				halt if( x != T(777) );
				++advanced;
			}
			if_coro_advance( x : gen )
			{
				halt;
			}
			halt if( advanced != 3 );
		}
		fn Foo()
		{
			CheckSimpleGen</i32/>();
			CheckSimpleGen</f32/>();
			CheckSimpleGen</u64/>();
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def Typeinfo_ForGenerators_Test0():
	c_program_text= """
		type IntGen= generator : i32;
		auto& int_gen_typeinfo= typeinfo</IntGen/>;

		static_assert( int_gen_typeinfo.size_of == typeinfo</$(byte8)/>.size_of ); // Coroutines have size of pointer.
		static_assert( int_gen_typeinfo.is_class ); // Generators are classes, because they have destructor method.
		static_assert( int_gen_typeinfo.is_coroutine );
		static_assert( int_gen_typeinfo.is_generator );
		static_assert( int_gen_typeinfo.coroutine_return_type.is_integer );
		static_assert( int_gen_typeinfo.coroutine_return_type.size_of == 4s );
		static_assert( !int_gen_typeinfo.coroutine_return_value_is_mutable );
		static_assert( !int_gen_typeinfo.coroutine_return_value_is_reference );
		static_assert( int_gen_typeinfo.references_tags_count == 0s ); // This coroutine type has no references inside.
	"""
	tests_lib.build_program( c_program_text )


def Typeinfo_ForGenerators_Test1():
	c_program_text= """
		type F64RefGen= generator'imut some_tag' : f64 &;
		auto& f64_ref_gen_typeinfo= typeinfo</F64RefGen/>;

		static_assert( f64_ref_gen_typeinfo.size_of == typeinfo</$(byte8)/>.size_of ); // Coroutines have size of pointer.
		static_assert( f64_ref_gen_typeinfo.is_class ); // Generators are classes, because they have destructor method.
		static_assert( f64_ref_gen_typeinfo.is_coroutine );
		static_assert( f64_ref_gen_typeinfo.is_generator );
		static_assert( f64_ref_gen_typeinfo.coroutine_return_type.is_float );
		static_assert( f64_ref_gen_typeinfo.coroutine_return_type.size_of == 8s );
		static_assert( !f64_ref_gen_typeinfo.coroutine_return_value_is_mutable );
		static_assert( f64_ref_gen_typeinfo.coroutine_return_value_is_reference );
		static_assert( f64_ref_gen_typeinfo.references_tags_count == 1s ); // This coroutine type has references inside.
	"""
	tests_lib.build_program( c_program_text )


def Typeinfo_ForGenerators_Test2():
	c_program_text= """
		type MutCharRefGen= generator'mut some_tag' : char8 &mut;
		auto& mut_ref_char_gen_typeinfo= typeinfo</MutCharRefGen/>;

		static_assert( mut_ref_char_gen_typeinfo.size_of == typeinfo</$(byte8)/>.size_of ); // Coroutines have size of pointer.
		static_assert( mut_ref_char_gen_typeinfo.is_class ); // Generators are classes, because they have destructor method.
		static_assert( mut_ref_char_gen_typeinfo.is_coroutine );
		static_assert( mut_ref_char_gen_typeinfo.is_generator );
		static_assert( mut_ref_char_gen_typeinfo.coroutine_return_type.is_char );
		static_assert( mut_ref_char_gen_typeinfo.coroutine_return_type.size_of == 1s );
		static_assert( mut_ref_char_gen_typeinfo.coroutine_return_value_is_mutable );
		static_assert( mut_ref_char_gen_typeinfo.coroutine_return_value_is_reference );
		static_assert( mut_ref_char_gen_typeinfo.references_tags_count == 1s ); // This coroutine type has references inside.
	"""
	tests_lib.build_program( c_program_text )


def GeneratorsEqualityCompare_Test0():
	c_program_text= """
		fn generator SomeGen() : i32
		{
			yield 1;
			yield 2;
			yield 3;
		}
		static_assert( typeinfo</ typeof(SomeGen()) />.is_equality_comparable );
		fn Foo()
		{
			auto mut gen0= SomeGen();
			auto mut gen1= SomeGen();
			// == compares objects of generator type. Each object is unique.
			halt if( gen0 == gen1 );
			halt if( !( gen1 != gen0 ) );
			halt if( gen0 != gen0 );
			halt if( gen1 != gen1 );
			halt if( !( gen0 == gen0 ) );
			halt if( !( gen1 == gen1 ) );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def Generator_InnerReferenceTagDeduction_Test0():
	c_program_text= """
		fn generator Gen() : i32 {}
		static_assert( typeinfo</typeof(Gen())/>.references_tags_count == 0s ); // No references for generator with no params.
	"""
	tests_lib.build_program( c_program_text )


def Generator_InnerReferenceTagDeduction_Test1():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		fn generator Gen(bool b, S s, [i32, 4] a, tup[f32, char8] t) : i32 {}
		fn Foo()
		{
			var S s= zero_init;
			var [i32, 4] a=zero_init;
			var tup[f32, char8] t= zero_init;
			auto gen= Gen(false, s, a, t);
			static_assert( typeinfo</typeof(gen)/>.references_tags_count == 0s ); // No references for generator with value params.
		}
	"""
	tests_lib.build_program( c_program_text )


def Generator_InnerReferenceTagDeduction_Test1():
	c_program_text= """
		fn generator Gen(i32& x) : i32 {}
		static_assert( typeinfo</typeof(Gen(0))/>.references_tags_count == 1s ); // Has references for generator with reference-params.
	"""
	tests_lib.build_program( c_program_text )


def Generator_InnerReferenceTagDeduction_Test2():
	c_program_text= """
		fn generator Gen(f32& mut x) : i32 {}
		fn Foo()
		{
			var f32 mut x= 0.0f;
			auto mut gen= Gen(x);
			static_assert( typeinfo</typeof(gen)/>.references_tags_count == 1s ); // Has references for generator with mutable reference-params.
		}
	"""
	tests_lib.build_program( c_program_text )


def Generator_InnerReferenceTagDeduction_Test3():
	c_program_text= """
		struct S{ i32& x; }
		fn generator Gen(S s) : i32 {}
		fn Foo()
		{
			var i32 x= 0;
			var S s{ .x= x };
			auto gen= Gen(s);
			static_assert( typeinfo</typeof(gen)/>.references_tags_count == 1s ); // Has references for generator with value-params, containing refrerences inside.
		}
	"""
	tests_lib.build_program( c_program_text )


def GeneratorNonSyncTag_Test0():
	c_program_text= """
		fn generator SyncGen() : i32;
		static_assert( !non_sync</ typeof(SyncGen()) /> );
	"""
	tests_lib.build_program( c_program_text )


def GeneratorNonSyncTag_Test1():
	c_program_text= """
		fn generator non_sync NonSyncGen() : i32;
		static_assert( non_sync</ typeof(NonSyncGen()) /> );
	"""
	tests_lib.build_program( c_program_text )


def GeneratorNonSyncTag_Test2():
	c_program_text= """
		fn generator non_sync(false) SyncGen() : i32;
		static_assert( !non_sync</ typeof(SyncGen()) /> );
	"""
	tests_lib.build_program( c_program_text )
