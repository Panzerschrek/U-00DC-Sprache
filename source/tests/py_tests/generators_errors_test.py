from py_tests_common import *


def CoroutineMismatch_Test0():
	c_program_text= """
		fn generator Foo() : i32;
		fn Foo() : ( generator : i32 ) { }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineMismatch", 2 ) or HasError( errors_list, "CoroutineMismatch", 3 ) )


def CoroutineMismatch_Test1():
	c_program_text= """
		fn generator Foo() : i32 {}
		fn Foo() : ( generator : i32 );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineMismatch", 2 ) or HasError( errors_list, "CoroutineMismatch", 3 ) )


def CoroutineMismatch_Test2():
	c_program_text= """
		struct S
		{
			fn generator Foo(this) : i32;
		}
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn S::Foo(this) : ( generator'imut' : i32 ) @(return_inner_references) { }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineMismatch", 4 ) or HasError( errors_list, "CoroutineMismatch", 7 ) )


def CoroutineMismatch_Test3():
	c_program_text= """
		struct S
		{
			var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
			fn Foo(this) : ( generator'imut' : i32 ) @(return_inner_references);
		}
		fn generator S::Foo(this) : i32 { }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineMismatch", 5 ) or HasError( errors_list, "CoroutineMismatch", 7 ) )


def NonDefaultCallingConventionForCoroutine_Test0():
	c_program_text= """
		fn generator Foo() call_conv("fast") : i32 {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NonDefaultCallingConventionForCoroutine", 2 ) )


def NonDefaultCallingConventionForCoroutine_Test1():
	c_program_text= """
		fn generator Foo() call_conv("cold") : i32 {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NonDefaultCallingConventionForCoroutine", 2 ) )


def NonDefaultCallingConventionForCoroutine_Test2():
	c_program_text= """
		fn generator Foo() call_conv("default") : i32 {} // Ok - using default calling convention.
	"""
	tests_lib.build_program( c_program_text )


def NonDefaultCallingConventionForCoroutine_Test3():
	c_program_text= """
		fn generator Foo() call_conv("C") : i32 {} // Ok - "C" is default calling convention.
	"""
	tests_lib.build_program( c_program_text )


def YieldOutsideCoroutine_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			yield 42;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "YieldOutsideCoroutine", 4 ) )


def YieldOutsideCoroutine_Test1():
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
	assert( HasError( errors_list, "YieldOutsideCoroutine", 6 ) )


def Yield_TypesMismatch_Test0():
	c_program_text= """
		fn generator Foo() : i32
		{
			yield 3.14; // Extected "i32", got "f64"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


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
	assert( HasError( errors_list, "TypesMismatch", 6 ) )


def Yield_TypesMismatch_Test2():
	c_program_text= """
		fn generator Foo() : i32
		{
			yield; // Expected non-void "yield".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def Yield_TypesMismatch_Test3():
	c_program_text= """
		fn generator Foo() : void&
		{
			yield; // Expected non-void "yield".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


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
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 10 ) )


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
	assert( HasError( errors_list, "BindingConstReferenceToNonconstReference", 4 ) )


def Yield_ExpectedReferenceValue_Test0():
	c_program_text= """
		fn generator Foo() : i32 &
		{
			yield 0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 4 ) )


def Yield_ForAbstractClass_Test0():
	c_program_text= """
		class A abstract
		{
			fn constructor( mut this, A& other )= default;
		}
		fn generator Foo(A& a) : A
		{
			yield a; // Trying to copy-construct abstract class.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstructingAbstractClassOrInterface", 8 ) )


def Yield_ForAbstractClass_Test1():
	c_program_text= """
		class A interface
		{
			fn constructor( mut this, A& other )= default;
		}
		fn generator Foo(A& a) : A
		{
			yield a; // Trying to copy-construct interface.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstructingAbstractClassOrInterface", 8 ) )


def IfCoroAdvanceForNonCoroutineValue_Test0():
	c_program_text= """
		fn Foo()
		{
			if_coro_advance( x : 42 ){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "IfCoroAdvanceForNonCoroutineValue", 4 ) )


def IfCoroAdvanceForNonCoroutineValue_Test1():
	c_program_text= """
		fn Foo( [f32, 4] arr )
		{
			if_coro_advance( x : arr ){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "IfCoroAdvanceForNonCoroutineValue", 4 ) )


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
	assert( HasError( errors_list, "IfCoroAdvanceForNonCoroutineValue", 4 ) )


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
	assert( HasError( errors_list, "BindingConstReferenceToNonconstReference", 6 ) )


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
	assert( HasError( errors_list, "BindingConstReferenceToNonconstReference", 6 ) )


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
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 6 ) )


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
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 6 ) )


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
	assert( HasError( errors_list, "ClassHasNoConstructors", 6 ) or HasError( errors_list, "CouldNotSelectOverloadedFunction", 6 ) )


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
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 7 ) )


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
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 6 ) )


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
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 7 ) )


def GeneratorIsNonCopyable_Test6():
	c_program_text= """
		fn generator SomeGen() : i32 {}
		fn Foo() : (generator : i32)
		{
			auto gen= SomeGen();
			return safe(gen); // Try to call copy constructor for return value.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 6 ) )


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
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 7 ) )


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
	assert( HasError( errors_list, "ExpectedInitializer", 4 ) )


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
	assert( HasError( errors_list, "ExpectedInitializer", 5 ) )


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
	assert( HasError( errors_list, "ExpectedInitializer", 5 ) )


def GeneratorIsNonDefaultConstructible_Test3():
	c_program_text= """
		fn Foo()
		{
			var[ generator : byte32, 16 ] arr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedInitializer", 4 ) )


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
	assert( HasError( errors_list, "UsingKeywordAsName", 6 ) )


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
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 11 ) )


def IfCoroAdvance_UseAbstractType_Test0():
	c_program_text= """
		class A abstract
		{
			fn constructor(mut this, A& other)= default;
		}
		class B final : A
		{
			fn constructor(mut this, B& other)= default;
		}
		fn generator Gen(A& a) : A&;
		fn Foo()
		{
			var B b;
			auto mut gen= Gen(b);
			if_coro_advance( a : gen ) // Bind here abstract reference to value. This is an error, because value is abstract.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstructingAbstractClassOrInterface", 15 ) )


def IfCoroAdvance_UseAbstractType_Test1():
	c_program_text= """
		class A interface
		{
			fn constructor(mut this, A& other)= default;
		}
		fn Foo( (generator'imut' : A&) mut gen )
		{
			if_coro_advance( a : gen ) // Bind here abstract reference to value. This is an error, because value is abstract.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstructingAbstractClassOrInterface", 8 ) )


def ReferencesPollution_ForGenerator_Test0():
	c_program_text= """
		struct S{ i32 & x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn generator Foo( S &mut s, i32 & x ) @(pollution) : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NotImplemented", 4 ) )


def ReferenceIndirectionDepthExceeded_ForGenerators_Test0():
	c_program_text= """
		struct S{ i32 & x; }
		fn generator Foo( S & s ) : i32 {} // Can't pass structs with references inside by a reference into a generator.
	"""
	tests_lib.build_program_with_errors( c_program_text )
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceIndirectionDepthExceeded", 3 ) )


def ReferenceIndirectionDepthExceeded_ForGenerators_Test1():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn generator Foo( S & s ) : i32 {} // Can't pass structs with references inside by a reference into a generator.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceIndirectionDepthExceeded", 3 ) )


def ReferenceIndirectionDepthExceeded_ForGenerators_Test2():
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
			static_assert( typeinfo</ typeof(gen) />.reference_tag_count == 1s ); // Generator type must contain references inside.
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
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


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
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def AccessingVariable_LinkedToGeneratorArgument_Test3():
	c_program_text= """
		struct S
		{
			i32 &mut x;
			op=(mut this, S& other);
		}
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn generator SomeGen(i32 &mut x) : S @(return_inner_references);
		fn Foo()
		{
			var i32 mut y= 0;
			var S mut s{ .x= y };
			var i32 mut x= 0;
			if_coro_advance( res_s : SomeGen(x) ) // Generator contains reference to "x".
			{
				// "res_s" contains internal reference to "x".
				s= res_s; // Copy internal reference to "x" from "res_s" to "s".
			}
			auto &imut x_ref= x; // Error, "x" has a mutable reference inside "s".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 19 ) )


def AccessingVariable_LinkedToGeneratorArgument_Test4():
	c_program_text= """
		struct S
		{
			fn generator SomeGen(this) : i32;
		}
		fn Foo()
		{
			var S mut s;
			auto gen= s.SomeGen();
			Bar( s ); // Error - taking mutable reference to "s", that has immutable reference inside "gen".
		}
		fn Bar( S &mut s );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 10 ) )


def AccessingVariable_LinkedToGeneratorArgument_Test5():
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
	assert( HasError( errors_list, "ReferenceProtectionError", 10 ) )


def AccessingVariable_LinkedToGeneratorArgument_Test6():
	c_program_text= """
		fn generator Gen( i32 &mut x, i32 &imut y ) {}
		fn Foo()
		{
			var i32 mut x= 0, imut y= 0;
			// Since generator type contains inner references for all input references immutable args linked only immutable.
			auto gen= Gen( x, y );
			static_assert( typeinfo</ typeof(gen) />.reference_tag_count == 2s );
			auto& y_ref= y; // Ok - create second immutable reference when an immutable reference inside "gen" exists.
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingVariable_LinkedToGeneratorArgument_Test7():
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


def AccessingVariable_LinkedToGeneratorArgument_Test8():
	c_program_text= """
		struct S{ i32& mut x; }
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		fn generator SomeGen( i32 &mut x, i32 &mut y ) : i32 &mut @( return_references );
		fn Foo()
		{
			var i32 mut a= 0, mut b= 0, mut c= 0;
			var S mut s{ .x= a };
			{
				auto mut gen= SomeGen( b, c ); // Save references to "b" and "c" inside "gen".
				if_coro_advance( &mut res : gen ) // "res" is now a reference to "b".
				{
					var S mut other_s{ .x= res };
					s= move(other_s); // Save now reference to "res" inside "s".
				}
			}
			auto& mut b_ref= b; // Error - a mutable reference to "b" still exists inside "s".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 17 ) )


def AccessingVariable_LinkedToGeneratorArgument_Test9():
	c_program_text= """
		struct S{ i32& mut x; }
		var [ [ char8, 2 ], 1 ] return_references[ "1_" ];
		fn generator SomeGen( i32 &mut x, i32 &mut y ) : i32 &mut @( return_references );
		fn Foo()
		{
			var i32 mut a= 0, mut b= 0, mut c= 0;
			var S mut s{ .x= a };
			{
				auto mut gen= SomeGen( b, c ); // Save references to "b" and "c" inside "gen".
				if_coro_advance( &mut res : gen ) // "res" is now a reference to "c".
				{
					var S mut other_s{ .x= res };
					s= move(other_s); // Save now reference to "res" inside "s".
				}
			}
			auto& mut b_ref= b; // Ok - create reference to "b". There is no reference to it inside "s".
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingGenerator_InsideIfCoroAdvance_Test0():
	c_program_text= """
		fn generator SomeGen() : i32;
		fn Foo()
		{
			auto mut gen= SomeGen();
			if_coro_advance( x : gen )
			{
				auto& ref= gen; // Error, temporary mutable reference to this generator exists, can't create new reference.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def AccessingGenerator_InsideIfCoroAdvance_Test1():
	c_program_text= """
		fn generator SomeGen() : i32;
		fn Foo()
		{
			auto mut gen= SomeGen();
			if_coro_advance( x : gen )
			{
				if_coro_advance( x : gen ) // Error - capturing mutable reference to generator second time.
				{
				}
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def AccessingGenerator_InsideIfCoroAdvance_Test2():
	c_program_text= """
		fn generator SomeGen() : i32;
		fn Foo()
		{
			auto mut gen= SomeGen();
			if_coro_advance( x : gen )
			{
				move(gen); // Error, moving generator variable, to which a reference exists.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MovedVariableHasReferences", 8 ) )


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
	assert( HasError( errors_list, "ReturningUnallowedReference", 5 ) )


def ReturningUnallowedReference_ForGeneratorYield_Test1():
	c_program_text= """
		fn generator Foo( i32 x ) : i32&
		{
			yield x; // Returning reference to value-argument. This is also forbidden.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 4 ) )


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
		var [ [ char8, 2 ], 1 ] return_references[ "1_" ];
		fn generator Foo( i32& x, i32& y ) : i32& @(return_references)
		{
			yield x; // Error - only "x" is allowed for return.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 5 ) )


def ReturningUnallowedReference_ForGeneratorYield_Test5():
	c_program_text= """
		var [ [ char8, 2 ], 1 ] return_references[ "1_" ];
		fn generator Foo( i32& x, i32& y ) : i32& @(return_references)
		{
			yield y; // Ok - return allowed reference.
		}
	"""
	tests_lib.build_program( c_program_text )


def ReturningUnallowedReference_ForGeneratorYield_Test6():
	c_program_text= """
		struct S{ i32& x; }
		var [ [ char8, 2 ], 0 ] return_references[];
		fn generator Foo( S s ) : i32& @(return_references)
		{
			yield s.x; // Error - there is no references allowed to return at all.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 6 ) )


def ReturningUnallowedReference_ForGeneratorYield_Test7():
	c_program_text= """
		struct S{ i32& x; }
		var [ [ char8, 2 ], 1 ] return_references[ "0a" ];
		fn generator Foo( S s ) : i32& @(return_references)
		{
			yield s.x; // Ok - return allowed reference.
		}
	"""
	tests_lib.build_program( c_program_text )


def ReturningUnallowedReference_ForGeneratorYield_Test8():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ [ char8, 2 ], 0 ] ] return_inner_references[ [ ] ];
		fn generator Foo() : S @(return_inner_references)
		{
			var i32 x= 0;
			var S s{ .x= x };
			yield s; // Returning reference inside a struct to local variable. For now it is forbidden, only references to arguments may be returned.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 8 ) )


def ReturningUnallowedReference_ForGeneratorYield_Test9():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn generator Foo( i32 x ) : S @(return_inner_references)
		{
			var S s{ .x= x };
			yield s; // Returning reference inside a struct to value-argument. This is also forbidden.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReturningUnallowedReference", 7 ) )


def ReturningUnallowedReference_ForGeneratorYield_Test10():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn generator Foo( i32& x ) : S @(return_inner_references)
		{
			var S s{ .x= x };
			yield s; // Returning reference inside a struct to reference-argument. This is ok.
		}
	"""
	tests_lib.build_program( c_program_text )


def ReturningUnallowedReference_ForGeneratorYield_Test11():
	c_program_text= """
		struct S{ i32& x; }
		var i32 some_global= 0;
		var tup[ [ [ char8, 2 ], 0 ] ] return_inner_references[ [ ] ];
		fn generator Foo() : S @(return_inner_references)
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
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( S &mut s, i32 & x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 6 ) )


def UnallowedReferencePollution_ForGenerator_Test1():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn generator Foo( S &mut s, i32 &mut x ) : i32
		{
			DoPollution( s, x );
			return;
		}
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( S &mut s, i32 & mut x ) @(pollution);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnallowedReferencePollution", 6 ) )


def GeneratorsCanNotBeConstexpr_Test0():
	c_program_text= """
		fn generator constexpr Foo() : i32 {} // Generator is not constexpr.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidTypeForConstexprFunction", 2 ) )


def GeneratorsCanNotBeConstexpr_Test1():
	c_program_text= """
		fn constexpr Foo() : ( generator : i32 ) { halt; } // Return type is not constexpr.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidTypeForConstexprFunction", 2 ) )


def GeneratorsCanNotBeConstexpr_Test2():
	c_program_text= """
		fn constexpr Foo( ( generator : i32 ) gen ) { } // Param type is not constexpr.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidTypeForConstexprFunction", 2 ) )


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
	assert( HasError( errors_list, "ConstexprFunctionContainsUnallowedOperations", 3 ) )


def GeneratorsCanNotBeConstexpr_Test4():
	c_program_text= """
		struct S{ (generator : bool) gen_field; } // This is not a constexpr type, because of generator field.
		fn constexpr Foo( S s ) {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidTypeForConstexprFunction", 3 ) )


def VirtualGenerator_Test0():
	c_program_text= """
		class A polymorph
		{
			fn virtual generator Foo(this) : i32;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VirtualCoroutine", 4 ) )


def VirtualGenerator_Test1():
	c_program_text= """
		class A polymorph
		{
			fn virtual Foo(this) : (generator'imut' : i32); // Ok - virtual method, returning generator.
		}
		class B : A
		{
			fn virtual override generator Foo(this) : i32; // Error, declaring overrided virtual function as generator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "VirtualCoroutine", 4 ) )
	assert( HasError( errors_list, "VirtualCoroutine", 8 ) )


def VirtualGenerator_Test2():
	c_program_text= """
		class A interface
		{
			fn virtual pure Foo(this) : (generator'imut' : i32); // Ok - virtual method, returning generator.
		}
	"""
	tests_lib.build_program( c_program_text )


def CoroutineSpecialMethod_Test0():
	c_program_text= """
		struct S
		{
			fn generator constructor();
			fn generator constructor(i32 x);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineSpecialMethod", 4 ) )
	assert( HasError( errors_list, "CoroutineSpecialMethod", 5 ) )


def CoroutineSpecialMethod_Test1():
	c_program_text= """
		struct S
		{
			fn generator destructor();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineSpecialMethod", 4 ) )


def InvalidReturnTypeForOperator_ForGeneratorOperator_Test0():
	c_program_text= """
		struct S
		{
			op generator ++(this); // This operator returns value of generator type and for this operator this is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidReturnTypeForOperator", 4 ) )


def InvalidReturnTypeForOperator_ForGeneratorOperator_Test1():
	c_program_text= """
		struct S
		{
			op generator =(mut this, S& other); // This operator returns value of generator type and for this operator this is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidReturnTypeForOperator", 4 ) )


def InvalidReturnTypeForOperator_ForGeneratorOperator_Test2():
	c_program_text= """
		struct S
		{
			op generator *=(mut this, S& other); // This operator returns value of generator type and for this operator this is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidReturnTypeForOperator", 4 ) )


def InvalidReturnTypeForOperator_ForGeneratorOperator_Test3():
	c_program_text= """
		struct S
		{
			op generator ==(mut this, S& other) : bool; // This operator returns value of generator type and for this operator this is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidReturnTypeForOperator", 4 ) )


def InvalidReturnTypeForOperator_ForGeneratorOperator_Test4():
	c_program_text= """
		struct S
		{
			op generator <=>(mut this, S& other) : i32; // This operator returns value of generator type and for this operator this is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "InvalidReturnTypeForOperator", 4 ) )


def AutoReturnCoroutine_Test0():
	c_program_text= """
		fn generator Foo() : auto {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoReturnCoroutine", 2 ) )


def GlobalsLoopDetected_ForGenerators_Test0():
	c_program_text= """
		struct S
		{
			// Ok - now completeness is not required for a generator method with this class param.
			fn generator Foo(S s) : char8;
		}
	"""
	tests_lib.build_program( c_program_text )


def GlobalsLoopDetected_ForGenerators_Test1():
	c_program_text= """
		struct S
		{
			// Ok - type completeness is not required for reference params of generator method.
			fn generator Foo(S& s) : char8;
		}
	"""
	tests_lib.build_program( c_program_text )


def CoroutineNonSyncRequired_Test0():
	c_program_text= """
		struct S non_sync {}
		fn generator Foo(S s){} // Generator value argument is non-sync.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineNonSyncRequired", 3 ) )


def CoroutineNonSyncRequired_Test1():
	c_program_text= """
		struct S non_sync {}
		fn generator non_sync(false) Foo(S& s){} // Generator value argument is non-sync.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineNonSyncRequired", 3 ) )


def CoroutineNonSyncRequired_Test2():
	c_program_text= """
		struct S non_sync {}
		fn generator Foo() : S {} // Generator return value is non-sync.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineNonSyncRequired", 3 ) )


def CoroutineNonSyncRequired_Test3():
	c_program_text= """
		struct S non_sync {}
		fn generator Foo() : S& {} // Generator return reference is non-sync.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineNonSyncRequired", 3 ) )


def CoroutineNonSyncRequired_Test4():
	c_program_text= """
		struct S non_sync {}
		// Ok - non_sync tag exists and args/return value are non-sync.
		fn generator non_sync(true) Foo(S& s){}
		fn generator non_sync Bar(S s){}
		fn generator non_sync Baz() : S {}
		fn generator non_sync Lol() : S& {}
		type Gen= generator non_sync(true) : S &mut;
	"""
	tests_lib.build_program( c_program_text )


def CoroutineNonSyncRequired_Test5():
	c_program_text= """
		struct S non_sync {}
		type Gen= generator : S; // "S" is "non_sync", so, "non_sync" is required for generator type.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CoroutineNonSyncRequired", 3 ) )


def IfCoroAdvance_VariablesStateMerge_Test0():
	c_program_text= """
		fn generator SomeGen() : i32;
		fn Foo()
		{
			var i32 mut x= 0;
			auto mut gen= SomeGen();
			if_coro_advance( v : gen )
			{
				move(x);
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConditionalMove", 10 ) )


def IfCoroAdvance_VariablesStateMerge_Test1():
	c_program_text= """
		fn generator SomeGen() : i32;
		struct S{ i32& x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( S &mut s, i32 & x ) @(pollution);
		fn Foo()
		{
			var i32 x= 0, mut y= 0;
			var S mut s{ .x= x };
			auto mut gen= SomeGen();
			if_coro_advance( v : gen )
			{
				DoPollution( s, y );
			}
			++y; // Error, "y" has reference inside "s"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 15 ) )


def IfCoroAdvance_VariablesStateMerge_Test2():
	c_program_text= """
		fn generator SomeGen() : i32;
		fn Foo()
		{
			var i32 mut x= 0;
			auto mut gen= SomeGen();
			if_coro_advance( v : gen )
			{
			}
			else
			{
				move(x);
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConditionalMove", 13 ) )


def IfCoroAdvance_VariablesStateMerge_Test3():
	c_program_text= """
		fn generator SomeGen() : i32;
		fn Foo()
		{
			var i32 mut x= 0;
			auto mut gen= SomeGen();
			if_coro_advance( v : gen )
			{
				move(x);
			}
			else
			{
				move(x);
			} // Ok, move "x" in all branches
		}
	"""
	tests_lib.build_program( c_program_text )


def IfCoroAdvance_VariablesStateMerge_Test4():
	c_program_text= """
		fn generator SomeGen() : i32;
		fn Foo()
		{
			auto mut gen= SomeGen();
			if_coro_advance( v : gen )
			{
			}
			else
			{
				// generator lock doesn't exist in this branch. So, we can do anything with generator variable here.
				move(gen);
				return;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def DestroyedVariableStillHasReferences_ForGenerator_Test0():
	c_program_text= """
		fn generator SomeGen( i32& x );
		fn Foo()
		{
			auto gen= SomeGen( 66 ); // Generator object holds a reference to temporary value of type "i32".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 5 ) )


def DestroyedVariableStillHasReferences_ForGenerator_Test1():
	c_program_text= """
		fn generator SomeGen( f32& x );
		fn Bar() : f32;
		fn Foo()
		{
			auto gen= SomeGen( Bar() ); // Generator object holds a reference to temporary-result of "Bar" call.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 6 ) )


def DestroyedVariableStillHasReferences_ForGenerator_Test2():
	c_program_text= """
		struct S{ i32& x; }
		fn generator SomeGen( S s );
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn MakeS( i32& x ) : S @(return_inner_references);
		fn Foo()
		{
			auto gen= SomeGen( MakeS( 789 ) ); // Generator object holds a value of type "S" with a reference to a temporary inside.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 8 ) )


def DestroyedVariableStillHasReferences_ForGenerator_Test3():
	c_program_text= """
		struct S{ i32& x; }
		fn generator SomeGen( S s );
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn MakeS( i32& x ) : S @(return_inner_references);
		fn Foo()
		{
			var i32 some_local= 0;
			auto gen= SomeGen( MakeS( some_local ) ); // Generator object holds a value of type "S" with a reference to a local variable. This is ok.
		}
	"""
	tests_lib.build_program( c_program_text )
