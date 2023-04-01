from py_tests_common import *


def GeneratorMismatch_Test0():
	c_program_text= """
		fn generator Foo() : i32;
		fn Foo() : ( generator : i32 ) { }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GeneratorMismatch", 2 ) or HaveError( errors_list, "GeneratorMismatch", 3 ) )



def GeneratorMismatch_Test1():
	c_program_text= """
		fn generator Foo() : i32 {}
		fn Foo() : ( generator : i32 );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GeneratorMismatch", 2 ) or HaveError( errors_list, "GeneratorMismatch", 3 ) )


def NonDefaultCallingConventionForGenerator_Test0():
	c_program_text= """
		fn generator Foo() call_conv("fast") : i32 {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NonDefaultCallingConventionForGenerator", 2 ) )


def NonDefaultCallingConventionForGenerator_Test1():
	c_program_text= """
		fn generator Foo() call_conv("cold") : i32 {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NonDefaultCallingConventionForGenerator", 2 ) )


def NonDefaultCallingConventionForGenerator_Test2():
	c_program_text= """
		fn generator Foo() call_conv("default") : i32 {} // Ok - using default calling convention.
	"""
	tests_lib.build_program( c_program_text )


def NonDefaultCallingConventionForGenerator_Test3():
	c_program_text= """
		fn generator Foo() call_conv("C") : i32 {} // Ok - "C" is default calling convention.
	"""
	tests_lib.build_program( c_program_text )


def YieldOutsideGenerator_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			yield 42;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "YieldOutsideGenerator", 4 ) )


def YieldOutsideGenerator_Test1():
	c_program_text= """
		struct S
		{
			fn Foo(this) : i32
			{
				yield 42;
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "YieldOutsideGenerator", 6 ) )


def Yield_TypesMismatch_Test0():
	c_program_text= """
		fn generator Foo() : i32
		{
			yield 3.14; // Extected "i32", got "f64"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def Yield_TypesMismatch_Test1():
	c_program_text= """
		struct S{}
		struct T{}
		fn generator Foo(S& s) : T&
		{
			yield s; // Extected reference to "T", got reference to "S"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "TypesMismatch", 6 ) )


def Yield_ForNonCopyableValue_Test0():
	c_program_text= """
		struct S
		{
			fn constructor();
			fn constructor(mut this, S& other)= delete;
		}
		fn generator Foo() : S
		{
			var S s;
			yield s; // Can't copy "s" here - it is non-copyable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 10 ) )


def Yield_ForNonCopyableValue_Test1():
	c_program_text= """
		struct S
		{
			fn constructor();
			fn constructor(mut this, S& other)= delete;
		}
		fn generator Foo() : S
		{
			var S mut s;
			yield move(s); // Ok - move non-copyable value.
		}
	"""
	tests_lib.build_program( c_program_text )


def Yield_BindingConstReferenceToNonconstReference_Test0():
	c_program_text= """
		fn generator Foo(i32& x) : i32 &mut
		{
			yield x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "BindingConstReferenceToNonconstReference", 4 ) )


def Yield_ExpectedReferenceValue_Test0():
	c_program_text= """
		fn generator Foo() : i32 &
		{
			yield 0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedReferenceValue", 4 ) )


def IfCoroAdvanceForNonCoroutineValue_Test0():
	c_program_text= """
		fn Foo()
		{
			if_coro_advance( x : 42 ){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "IfCoroAdvanceForNonCoroutineValue", 4 ) )


def IfCoroAdvanceForNonCoroutineValue_Test1():
	c_program_text= """
		fn Foo( [f32, 4] arr )
		{
			if_coro_advance( x : arr ){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "IfCoroAdvanceForNonCoroutineValue", 4 ) )


def IfCoroAdvanceForNonCoroutineValue_Test2():
	c_program_text= """
		fn Foo()
		{
			if_coro_advance( x : GetS() ){}
		}
		struct S{}
		fn GetS() : S;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "IfCoroAdvanceForNonCoroutineValue", 4 ) )


def BindingConstReferenceToNonconstReference_For_IfCoroAdvance_Test0():
	c_program_text= """
		fn generator SomeGen() : i32;
		fn Foo()
		{
			auto gen= SomeGen();
			if_coro_advance( x : gen ){} // Error, can't advance immutable generator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "BindingConstReferenceToNonconstReference", 6 ) )


def BindingConstReferenceToNonconstReference_For_IfCoroAdvance_Test1():
	c_program_text= """
		fn generator SomeGen() : i32;
		fn Foo()
		{
			auto mut gen= SomeGen();
			if_coro_advance( x : gen ){} // Ok, can advance mutable generator.
		}
	"""
	tests_lib.build_program( c_program_text )


def BindingConstReferenceToNonconstReference_For_IfCoroAdvance_Test2():
	c_program_text= """
		fn generator SomeGen() : i32;
		fn Foo()
		{
			if_coro_advance( x : SomeGen() ){} // Ok, can advance temp generator.
		}
	"""
	tests_lib.build_program( c_program_text )


def BindingConstReferenceToNonconstReference_For_IfCoroAdvance_Test3():
	c_program_text= """
		fn generator SomeGen() : i32 &;
		fn Foo()
		{
			auto mut gen= SomeGen();
			if_coro_advance( &mut x : gen ){} // Binding "x" as mutable reference to immutable reference result of generator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "BindingConstReferenceToNonconstReference", 6 ) )


def GeneratorIsNonCopyable_Test0():
	c_program_text= """
		fn generator SomeGen() : i32 {}
		fn Foo()
		{
			auto gen= SomeGen();
			auto gen_copy= gen;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 6 ) )


def GeneratorIsNonCopyable_Test1():
	c_program_text= """
		fn generator SomeGen() : i32 {}
		fn Foo()
		{
			auto gen= SomeGen();
			var (generator : i32) gen_copy= gen;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 6 ) )


def GeneratorIsNonCopyable_Test2():
	c_program_text= """
		fn generator SomeGen() : i32 {}
		fn Foo()
		{
			auto gen= SomeGen();
			var (generator : i32) gen_copy(gen);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ClassHaveNoConstructors", 6 ) )


def GeneratorIsNonCopyable_Test3():
	c_program_text= """
		fn generator SomeGen() : i32 {}
		struct S{ (generator : i32) gen; }
		fn Foo()
		{
			auto gen= SomeGen();
			var S s { .gen= gen };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 7 ) )


def GeneratorIsNonCopyable_Test4():
	c_program_text= """
		fn generator SomeGen() : i32 {}
		fn Foo()
		{
			auto gen= SomeGen();
			var [ (generator : i32 ), 1 ] arr[ gen ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 6 ) )


def GeneratorIsNonCopyable_Test5():
	c_program_text= """
		fn generator SomeGen() : i32 {}
		fn Foo()
		{
			auto mut gen0= SomeGen();
			auto mut gen1= SomeGen();
			gen0= gen1; // Try to call copy assignment operator here.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 7 ) )


def GeneratorIsNonCopyable_Test6():
	c_program_text= """
		fn generator SomeGen() : i32 {}
		fn Foo() : (generator : i32)
		{
			auto gen= SomeGen();
			return gen; // Try to call copy constructor for return value.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 6 ) )


def GeneratorIsNonCopyable_Test7():
	c_program_text= """
		fn generator SomeGen() : i32 {}
		fn Pass( (generator : i32) gen );
		fn Foo()
		{
			auto gen= SomeGen();
			Pass( gen ); // Try to call copy constructor for argument.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 7 ) )


def GeneratorIsNonCopyable_Test8():
	c_program_text= """
		type Gen= generator : i32;
		static_assert( !typeinfo</Gen/>.is_copy_constructible );
		static_assert( !typeinfo</Gen/>.is_copy_assignable );
	"""
	tests_lib.build_program( c_program_text )


def UsingKeywordAsName_For_IfCoroAdvance_Test0():
	c_program_text= """
		fn generator SomeGen() : i32;
		fn Foo()
		{
			auto mut gen= SomeGen();
			if_coro_advance( virtual : gen ){} // "virtual" is a keyword.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UsingKeywordAsName", 6 ) )


def IfCoroAdvance_ForNonCopyableValue_Test0():
	c_program_text= """
		struct S
		{
			fn constructor();
			fn constructor(mut this, S& other)= delete;
		}
		fn generator SomeGen() : S&;
		fn Foo()
		{
			auto mut gen= SomeGen();
			if_coro_advance( x : gen ) {} // Generator returns reference and it is copied into a value, but type of this value is noncopyable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 11 ) )


def NameNotFound_ForGeneratorTypeTag_Test0():
	c_program_text= """
		struct S{ i32 &imut x; }
		type Gen= generator : S'unknown_tag';
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NameNotFound", 3 ) )


def NameNotFound_ForGeneratorTypeTag_Test1():
	c_program_text= """
		type Gen= generator'imut known_tag' : i32 &'unknow_tag;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NameNotFound", 2 ) )


def ReferencesPollution_ForGenerator_Test0():
	c_program_text= """
		struct S{ i32 & x; }
		fn generator Foo( S &mut s'a', i32 &'b x ) ' a <- b ' : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NotImplemented", 3 ) )


def ExplicitReturReferenceTags_ForGenerators_Test0():
	c_program_text= """
		fn generator Foo( i32 &'a x ) : i32 &'a;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NotImplemented", 2 ) )


def ExplicitReturReferenceTags_ForGenerators_Test1():
	c_program_text= """
		struct S{ i32 & x; }
		fn generator Foo( i32 &'a x ) : S'a';
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "NotImplemented", 3 ) )


def ReferenceFieldOfTypeWithReferencesInside_ForGenerators_Test0():
	c_program_text= """
		struct S{ i32 & x; }
		fn generator Foo( S & s ) : i32 {} // Can't pass structs with references inside by a reference into a generator.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceFieldOfTypeWithReferencesInside", 3 ) )


def ReferenceFieldOfTypeWithReferencesInside_ForGenerators_Test1():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn generator Foo( S & s ) : i32 {} // Can't pass structs with references inside by a reference into a generator.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceFieldOfTypeWithReferencesInside", 3 ) )


def ReferenceFieldOfTypeWithReferencesInside_ForGenerators_Test2():
	c_program_text= """
		struct S{ i32 & x; }
		fn generator Foo( S s ) : i32 {} // Ok - pass struct with reference inside by value.
	"""
	tests_lib.build_program( c_program_text )


def AccessingVariable_LinkedToGeneratorArgument_Test0():
	c_program_text= """
		fn generator SomeGen(i32& x) : i32;
		fn Foo()
		{
			var i32 mut x= 42;
			auto gen= SomeGen(x);
			static_assert( typeinfo</ typeof(gen) />.references_tags_count == 1s ); // Generator type must contain references inside.
		}
	"""
	tests_lib.build_program ( c_program_text )


def AccessingVariable_LinkedToGeneratorArgument_Test1():
	c_program_text= """
		fn generator SomeGen(i32& x) : i32;
		fn Foo()
		{
			var i32 mut x= 42;
			auto gen= SomeGen(x);
			++x; // Error, generator "gen" contains a reference to "x", that was passed as reference parameter.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 7 ) )


def AccessingVariable_LinkedToGeneratorArgument_Test2():
	c_program_text= """
		fn generator SomeGen(i32 &mut x) : i32;
		fn Foo()
		{
			var i32 mut x= 42;
			auto gen= SomeGen(x);
			Bar( x ); // Error - taking immutable reference to "x", that has mutable reference inside "gen".
		}
		fn Bar( i32& x );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 7 ) )


def AccessingVariable_LinkedToGeneratorArgument_Test3():
	c_program_text= """
		fn generator SomeGen(i32 x) : i32;
		fn Foo()
		{
			var i32 mut x= 42;
			auto gen= SomeGen(x);
			++x; // Ok, "x" is passed by value and was not linked to "gen".
		}
	"""
	tests_lib.build_program( c_program_text )


def ReturningUnallowedReference_ForGeneratorYield_Test0():
	c_program_text= """
		fn generator Foo() : i32&
		{
			var i32 x= 0;
			yield x; // Returning reference to local variable. For now it is forbidden, only references to arguments may be returned.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 5 ) )


def ReturningUnallowedReference_ForGeneratorYield_Test1():
	c_program_text= """
		fn generator Foo( i32 x ) : i32&
		{
			yield x; // Returning reference to value-argument. This is also forbidden.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 4 ) )


def ReturningUnallowedReference_ForGeneratorYield_Test2():
	c_program_text= """
		fn generator Foo( i32& x ) : i32&
		{
			yield x; // Returning reference to reference-argument. This is ok.
		}
	"""
	tests_lib.build_program( c_program_text )


def ReturningUnallowedReference_ForGeneratorYield_Test3():
	c_program_text= """
		var i32 some_global= 0;
		fn generator Foo() : i32&
		{
			yield some_global; // Returning reference to global variable. This is ok.
		}
	"""
	tests_lib.build_program( c_program_text )


def ReturningUnallowedReference_ForGeneratorYield_Test4():
	c_program_text= """
		struct S{ i32& x; }
		fn generator Foo() : S
		{
			var i32 x= 0;
			var S s{ .x= x };
			yield s; // Returning reference inside a struct to local variable. For now it is forbidden, only references to arguments may be returned.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 7 ) )


def ReturningUnallowedReference_ForGeneratorYield_Test5():
	c_program_text= """
		struct S{ i32& x; }
		fn generator Foo( i32 x ) : S
		{
			var S s{ .x= x };
			yield s; // Returning reference inside a struct to value-argument. This is also forbidden.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 6 ) )


def ReturningUnallowedReference_ForGeneratorYield_Test6():
	c_program_text= """
		struct S{ i32& x; }
		fn generator Foo( i32& x ) : S
		{
			var S s{ .x= x };
			yield s; // Returning reference inside a struct to reference-argument. This is ok.
		}
	"""
	tests_lib.build_program( c_program_text )


def ReturningUnallowedReference_ForGeneratorYield_Test7():
	c_program_text= """
		struct S{ i32& x; }
		var i32 some_global= 0;
		fn generator Foo() : S
		{
			var S s{ .x= some_global };
			yield s; // Returning reference inside a struct to global variable. This is ok.
		}
	"""
	tests_lib.build_program( c_program_text )


def UnallowedReferencePollution_ForGenerator_Test0():
	c_program_text= """
		struct S{ i32& x; }
		fn generator Foo( S &mut s, i32 & x ) : i32
		{
			DoPollution( s, x );
		}
		fn DoPollution( S &mut s'a', i32 &'b x ) ' a <- b ';
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnallowedReferencePollution", 6 ) )


def UnallowedReferencePollution_ForGenerator_Test1():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn generator Foo( S &mut s, i32 &mut x ) : i32
		{
			DoPollution( s, x );
			return;
		}
		fn DoPollution( S &mut s'a', i32 &'b mut x ) ' a <- b ';
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnallowedReferencePollution", 6 ) )
