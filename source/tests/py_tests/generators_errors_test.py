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


def GeneratorMismatch_Test2():
	c_program_text= """
		struct S
		{
			fn generator Foo(this) : i32;
		}
		fn S::Foo(this) : ( generator'imut this_tag' : i32 )'this' { }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GeneratorMismatch", 4 ) or HaveError( errors_list, "GeneratorMismatch", 6 ) )


def GeneratorMismatch_Test3():
	c_program_text= """
		struct S
		{
			fn Foo(this) : ( generator'imut this_tag' : i32 )'this';
		}
		fn generator S::Foo(this) : i32 { }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GeneratorMismatch", 4 ) or HaveError( errors_list, "GeneratorMismatch", 6 ) )


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


def Yield_TypesMismatch_Test2():
	c_program_text= """
		fn generator Foo() : i32
		{
			yield; // Expected non-void "yield".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def Yield_TypesMismatch_Test3():
	c_program_text= """
		fn generator Foo() : void&
		{
			yield; // Expected non-void "yield".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


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


def GeneratorIsNonDefaultConstructible_Test0():
	c_program_text= """
		fn Foo()
		{
			var (generator : u64) gen;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedInitializer", 4 ) )


def GeneratorIsNonDefaultConstructible_Test1():
	c_program_text= """
		struct S{ (generator : char16) gen; }
		fn Foo()
		{
			var S s{}; // Missing initializer for "gen" field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedInitializer", 5 ) )


def GeneratorIsNonDefaultConstructible_Test2():
	c_program_text= """
		struct S{ (generator : char16) gen; }
		fn Foo()
		{
			var S s; // "S" has no default constructor, because "gen" is not default-constructible.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedInitializer", 5 ) )


def GeneratorIsNonDefaultConstructible_Test3():
	c_program_text= """
		fn Foo()
		{
			var[ generator : byte32, 16 ] arr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ExpectedInitializer", 4 ) )


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
		struct S
		{
			fn generator SomeGen(this) : i32;
		}
		fn Foo()
		{
			var S mut s;
			auto gen= s.SomeGen();
			Bar( x ); // Error - taking mutable reference to "s", that has immutable reference inside "gen".
		}
		fn Bar( S &mut s );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 10 ) )


def AccessingVariable_LinkedToGeneratorArgument_Test4():
	c_program_text= """
		struct S
		{
			fn generator SomeGen(mut this) : i32;
		}
		fn Foo()
		{
			var S mut s;
			auto gen= s.SomeGen();
			auto& s_ref= s; // Error - taking immutable reference to "s", that has mutable reference inside "gen".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 10 ) )


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


def GeneratorsCanNotBeConstexpr_Test0():
	c_program_text= """
		fn generator constexpr Foo() : i32 {} // Generator is not constexpr.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidTypeForConstexprFunction", 2 ) )


def GeneratorsCanNotBeConstexpr_Test1():
	c_program_text= """
		fn constexpr Foo() : ( generator : i32 ) { halt; } // Return type is not constexpr.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidTypeForConstexprFunction", 2 ) )


def GeneratorsCanNotBeConstexpr_Test2():
	c_program_text= """
		fn constexpr Foo( ( generator : i32 ) gen ) { } // Param type is not constexpr.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidTypeForConstexprFunction", 2 ) )


def GeneratorsCanNotBeConstexpr_Test3():
	c_program_text= """
		fn generator SomeGen() : f32 {}
		fn constexpr Foo()
		{
			auto gen= SomeGen(); // Creating generator variable is not constexpr operation.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ConstexprFunctionContainsUnallowedOperations", 3 ) )


def GeneratorsCanNotBeConstexpr_Test4():
	c_program_text= """
		struct S{ (generator : bool) gen_field; } // This is not a constexpr type, because of generator field.
		fn constexpr Foo( S s ) {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidTypeForConstexprFunction", 3 ) )


def VirtualGenerator_Test0():
	c_program_text= """
		class A polymorph
		{
			fn virtual generator Foo(this) : i32;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "VirtualGenerator", 4 ) )


def VirtualGenerator_Test1():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo(this) : (generator'imut some_tag' : i32); // Ok - virtual method, returning generator.
		}
		class B : A
		{
			fn virtual override generator Foo(this) : i32; // Error, declaring overrided virtual function as generator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HaveError( errors_list, "VirtualGenerator", 4 ) )
	assert( HaveError( errors_list, "VirtualGenerator", 8 ) )


def VirtualGenerator_Test2():
	c_program_text= """
		class A interface
		{
			fn virtual pure Foo(this) : (generator'imut some_tag' : i32); // Ok - virtual method, returning generator.
		}
	"""
	tests_lib.build_program( c_program_text )


def GeneratorSpecialMethod_Test0():
	c_program_text= """
		struct S
		{
			fn generator constructor();
			fn generator constructor(i32 x);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GeneratorSpecialMethod", 4 ) )
	assert( HaveError( errors_list, "GeneratorSpecialMethod", 5 ) )


def GeneratorSpecialMethod_Test1():
	c_program_text= """
		struct S
		{
			fn generator destructor();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GeneratorSpecialMethod", 4 ) )


def InvalidReturnTypeForOperator_ForGeneratorOperator_Test0():
	c_program_text= """
		struct S
		{
			op generator ++(this); // This operator returns value of generator type and for this operator this is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidReturnTypeForOperator", 4 ) )


def InvalidReturnTypeForOperator_ForGeneratorOperator_Test1():
	c_program_text= """
		struct S
		{
			op generator =(mut this, S& other); // This operator returns value of generator type and for this operator this is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidReturnTypeForOperator", 4 ) )


def InvalidReturnTypeForOperator_ForGeneratorOperator_Test2():
	c_program_text= """
		struct S
		{
			op generator *=(mut this, S& other); // This operator returns value of generator type and for this operator this is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidReturnTypeForOperator", 4 ) )


def InvalidReturnTypeForOperator_ForGeneratorOperator_Test3():
	c_program_text= """
		struct S
		{
			op generator ==(mut this, S& other) : bool; // This operator returns value of generator type and for this operator this is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidReturnTypeForOperator", 4 ) )


def InvalidReturnTypeForOperator_ForGeneratorOperator_Test4():
	c_program_text= """
		struct S
		{
			op generator <=>(mut this, S& other) : i32; // This operator returns value of generator type and for this operator this is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "InvalidReturnTypeForOperator", 4 ) )


def AutoReturnGenerator_Test0():
	c_program_text= """
		fn generator Foo() : auto {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AutoReturnGenerator", 2 ) )


def GlobalsLoopDetected_ForGenerators_Test0():
	c_program_text= """
		struct S
		{
			// All methods type completeness required for class to be complete.
			// Generator function requires generator type completeness, for which value-param of type "S" completeness is required.
			fn generator Foo(S s) : char8;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GlobalsLoopDetected", 2 ) )


def GlobalsLoopDetected_ForGenerators_Test1():
	c_program_text= """
		struct S
		{
			// Ok - type completeness is not required for reference params of generator method.
			fn generator Foo(S& s) : char8;
		}
	"""
	tests_lib.build_program( c_program_text )
