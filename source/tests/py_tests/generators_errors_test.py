from py_tests_common import *


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
