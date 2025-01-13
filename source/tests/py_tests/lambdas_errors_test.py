from py_tests_common import *

def AccessingThisInLambda_Test0():
	# "this" should be not available in lambdas for hidden "this" parameter of a lambda function.
	c_program_text= """
		fn Foo()
		{
			auto f=
				lambda()
				{
					auto& ref= this;
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ThisUnavailable", 7 ) )


def AccessingThisInLambda_Test1():
	# "this" should be not available in lambdas for hidden "this" parameter of a lambda function.
	# This is true even if lambda captures names by value.
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f=
				lambda [=] ()
				{
					auto x_copy= x;
					auto& ref= this;
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ThisUnavailable", 9 ) )


def AccessingThisInLambda_Test2():
	# "this" should be not available for struct.
	c_program_text= """
		struct S
		{
			fn Foo(this)
			{
				auto f=
					lambda[&]()
					{
						auto& self= this;
					};
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ThisUnavailable", 9 ) )


def AccessingThisInLambda_Test3():
	c_program_text= """
		class C
		{
			fn Foo(this)
			{
				auto f=
					lambda[&]()
					{
						Bar(); // Can't call this-call method, because "this" can't be captured.
					};
			}
			fn Bar( this );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CouldNotSelectOverloadedFunction", 9 ) )


def AccessingThisInLambda_Test4():
	c_program_text= """
		class Base polymorph {}
		class Derived : Base
		{
			fn Foo(this)
			{
				auto f=
					lambda[&]()
					{
						auto& base_ref= base; // Can't access "base", because "this" is not captured by the lambda.
					};
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "BaseUnavailable", 10 ) )


def AccessingThisInLambda_Test5():
	c_program_text= """
		struct S
		{
			f32 some_field;
			fn Foo( mut this )
			{
				auto f=
					lambda[&]()
					{
						auto& r=some_field; // Can't access field, because "this" is not captured by the lambda.
					};
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ClassFieldAccessInStaticMethod", 10 ) )


def AccessingThisInLambda_Test6():
	c_program_text= """
		class Base polymorph
		{
			i32 x;
		}
		class Derived : Base
		{
			fn Foo(this)
			{
				auto f=
					lambda[&]()
					{
						auto& x_ref= x; // Can't access field of the base class, because "this" is not captured by the lambda.
					};
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ClassFieldAccessInStaticMethod", 13 ) )


def AccessingThisInLambda_Test7():
	c_program_text= """
		struct S
		{
			f32 some_field;
			fn Foo( mut this )
			{
				auto f=
					lambda[=]() // Even by-value capture still doesn't give access to "this".
					{
						auto& r=some_field; // Can't access field, because "this" is not captured by the lambda.
					};
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ClassFieldAccessInStaticMethod", 10 ) )


def AccessingThisInLambda_Test8():
	c_program_text= """
		struct S
		{
			fn Foo( this )
			{
				auto f=
					lambda[=]() // Even by-value capture still doesn't give access to "this".
					{
						auto& r=this; // Can't access "this" because it is not captured.
					};
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ThisUnavailable", 9 ) )


def CopyConstructValueOfNoncopyableType_ForCapturedLambdaValue_Test0():
	c_program_text= """
		struct S
		{
			fn constructor( S& other )= delete;
		}
		fn Foo( S& s )
		{
			var i32 x= 0;
			auto f=
				lambda [=] () // Error, capturing "s" by copy, but it is not copy-constructible.
				{
					auto& s_ref= s;
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 10 ) )


def CopyConstructValueOfNoncopyableType_ForCapturedLambdaValue_Test1():
	c_program_text= """
		class C
		{
			fn constructor()= default;
		}
		fn Foo( S& s )
		{
			var C c;
			auto f=
				lambda [=] () // Error, capturing "c" by copy, but it is not copy-constructible.
				{
					auto& c_ref= c;
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 10 ) )


def CopyConstructValueOfNoncopyableType_ForCapturedLambdaValue_Test2():
	c_program_text= """
		class C
		{
			fn constructor()= default;
		}
		fn Foo( S& s )
		{
			var C c;
			auto f=
				lambda [ c_copy= c ] () // Try to call copying in initialization of lambda capture in capture lust.
				{
					auto& c_ref= c_copy;
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 10 ) )


def CopyConstructValueOfNoncopyableType_ForCapturedLambdaValue_Test3():
	c_program_text= """
		class C
		{
			fn constructor()= default;
		}
		fn Foo( S& s )
		{
			var C mut c;
			auto f=
				lambda [ c= move(c) ] () // Ok - move "c".
				{
					auto& c_ref= c;
				};
			auto f_copy= f; // Can't copy "f" - it is not copy-constructible, because it captures non-copy-costrunctible variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 14 ) )


def CopyConstructValueOfNoncopyableType_ForCapturedLambdaValue_Test4():
	c_program_text= """
		class C
		{
			fn constructor()= default;
		}
		fn Foo( S& s )
		{
			// This lambda is non-copyable, since it captures non-copyable class.
			auto f=
				lambda [ c= C() ] byval ()
				{
					auto& c_ref= c;
				};
			f(); // Error - copy-construction lambda in "byval this" call without moving of the lambda object.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CopyConstructValueOfNoncopyableType", 14 ) )


def CopyAssign_ForLambdaWithReferencesInside_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 13;
			var f32 y= 22.5f;
			auto mut f= lambda[&]() : f32 { return f32(x) * y; };
			auto f_copy= f;
			f= f_copy; // Copy assignment is not supported for lambdas with capturing references, because it's unclear, what to do with references.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 8 ) )


def CopyAssign_ForLambdaWithReferencesInside_Test1():
	c_program_text= """
		struct R{ i32& x; }
		fn Foo( R r )
		{
			auto mut f= lambda[=](){ auto& ref= r; };
			auto f_copy= f;
			f= f_copy; // Can't copy-assign, becausecaptured by value "r" is not copy-assignable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 7 ) )


def StructNamedInitializerForLambda_Test0():
	c_program_text= """
		fn Foo()
		{
			auto f= lambda(){};
			var typeof(f) other_f{}; // Can't use {} initializer for lambdas.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StructInitializerForNonStruct", 5 ) )


def StructNamedInitializerForLambda_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f= lambda[=]() : i32 { return x; };
			var typeof(f) other_f{ .x= 0 }; // Can't use {} initializer for lambdas.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StructInitializerForNonStruct", 6 ) )


def ZeroInitializer_ForLambda_Test0():
	c_program_text= """
		fn Foo()
		{
			auto f= lambda(){};
			var typeof(f) other_f= zero_init; // Can't use zero_init for lambdas.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ZeroInitializerForClass", 5 ) )


def ZeroInitializer_ForLambda_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f= lambda[=]() : i32 { return x; };
			var typeof(f) other_f= zero_init; // Can't use zero_init for lambdas with captured value.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ZeroInitializerForClass", 6 ) )


def ZeroInitializer_ForLambda_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f= lambda[&]() : i32 { return x; };
			var typeof(f) other_f= zero_init; // Can't use zero_init for lambdas with captured reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ZeroInitializerForClass", 6 ) )


def ReturnedFromLambdaReferenceIsLinkedToLambdaArg_Test0():
	c_program_text= """
		struct R{ i32& r; }
		fn Foo()
		{
			// Lambda returns passed reference inside a variable.
			auto f=
				lambda( i32& x ) : R
				{
					var R r{ .r= x };
					return r;
				};

			var i32 mut arg= 9890;
			var R r= f(arg);
			++arg; // Error, there is reference inside "r" to this variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 15 ) )


def ReturnedFromLambdaReferenceIsLinkedToLambdaItself_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 12345;
			auto mut f= lambda[=]() : i32& { return x; };
			auto& ref= f(); // Capture a reference to lambda.
			move(f); // Error, this lambda has references to it.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MovedVariableHasReferences", 7 ) )


def ReturnedFromLambdaReferenceIsLinkedToLambdaItself_Test1():
	c_program_text= """
		struct R{ i32& r; }
		fn Foo()
		{
			var i32 mut x= 776655;
			auto mut f=
				lambda[=]() : R
				{
					var R r{ .r= x };
					return r;
				};
			var R r= f();
			move(f); // Error, this lambda has references to it.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MovedVariableHasReferences", 13 ) )


def ReturnedFromLambdaReferenceIsLinkedToLambdaItself_Test2():
	c_program_text= """
		struct R{ i32& mut r; }
		fn Foo()
		{
			auto mut f= lambda[x= 42] mut () : R
			{
				return R{ .r= x }; // Return a reference to lambda field "x" inside "R" struct.
			};
			var R r= f(); // "r" now contains a mutable reference to "lambda.x".
			f(); // Error - accessing the lambda (including "x" ) but a reference to "x" is also stored inside "r".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 10 ) )


def ReturnedFromLambdaReferenceIsLinkedToCapturedVariableInnerReference_Test0():
	c_program_text= """
		struct R
		{
			f64 &mut r;
		}
		fn Foo()
		{
			var f64 mut x= 0.0;
			var R r { .r= x };
			// Stores inner reference of captured variable.
			auto f= lambda[=]() : f64 { return r.r; };
			auto& other_ref= r.r; // Error, a reference to "r.x" exists insied "f".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def LambdaCapturesReferences_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto f= lambda[&]() { auto x_copy= x; };
			auto &mut x_ref= x; // Error, lambda captures reference to variable "x".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def LambdaCapturesReferences_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto f= lambda[&]() { };
			auto &mut x_ref= x; // Ok, "x" is not captured.
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def LambdaCapturesReferences_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto f= lambda[&]() { ++x; };
			auto f_copy= f; // "f_copy" inner reference nodes are linked to "f" inner reference nodes.
			f(); // Can't call first lambda, because there is a reference to its inner node inside "f_copy".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def LambdaCapturesReferences_Test3():
	c_program_text= """
		struct R{ i32& mut r; }
		fn Foo()
		{
			var i32 mut x= 0;
			// This lambda captures a mutable reference to "x".
			auto f= lambda[&]() : R
			{
				return R{ .r= x }; // Return captured mutable to "x" inside "R" struct.
			};
			var R r= f(); // "r" now contains a mutable reference to "x".
			f(); // Error - accessing reference to "x" captured inside the lambda, but "x" is also stored inside "r".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 12 ) )


def LambdaModifyCapturedVariable_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f=
				lambda[=]()
				{
					++x; // Error - can't modify immutable value.
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 8 ) )


def LambdaModifyCapturedVariable_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto f=
				lambda[=]()
				{
					// Even if source variable is mutable, captured by value variable is immutable, because by default lambda op() uses "imut this".
					++x;
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 9 ) )


def LambdaModifyCapturedVariable_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f=
				lambda[&]()
				{
					// Can't modify captured by immutable reference variable.
					++x;
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 9 ) )


def LambdaModifyCapturedVariable_Test3():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f=
				lambda[&x]()
				{
					// Can't modify captured explicitly immutable reference.
					++x;
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 9 ) )


def LambdaModifyCapturedVariable_Test4():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f=
				lambda[x]()
				{
					// Can't modify captured explicitly copy.
					++x;
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 9 ) )


def LambdaModifyCapturedVariable_Test5():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			// Mutability modifier for captures with initializer is still determined via mutability of the source variable.
			auto f=
				lambda[&x_ref= x]()
				{
					// Can't modify captured explicitly reference.
					++x_ref;
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 10 ) )


def LambdaModifyCapturedVariable_Test5():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f=
				lambda[=] byval ()
				{
					++x; // Error - can't modify immutable value, even in "byval" but not "mut" lambda.
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 8 ) )


def LambdaMoveCapturedVariable_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto f=
				lambda[=]()
				{
					move(x); // In "imut this" lambda captured variable is assumed to be a class field, not a variable.
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedVariable", 8 ) or HasError( errors_list, "ExpectedReferenceValue", 8 ) )


def LambdaMoveCapturedVariable_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto f=
				lambda[&]()
				{
					move(x); // In "imut this" lambda captured reference is assumed to be a class refernce field, not a variable.
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedVariable", 8 ) )


def LambdaMoveCapturedVariable_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto f=
				lambda[&] byval mut ()
				{
					move(x); // In "byval mut this" lambda captured reference is assumed to be a class refernce field, not a variable.
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedVariable", 8 ) )


def LambdaMoveCapturedVariable_Test3():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto f=
				lambda[=] byval imut ()
				{
					move(x); // In "byval imut this" lambda captured variable is immutable.
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedVariable", 8 ) or HasError( errors_list, "ExpectedReferenceValue", 8 ) )


def LambdaMoveCapturedVariable_Test4():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto f=
				lambda[&x_ref= x] byval mut ()
				{
					move(x_ref); // Can't move explicitly-initialized reference capture in "byval mut" lambda.
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedVariable", 8 ) )


def ReferenceIndirectionDepthExceeded_ForLambdas_Test0():
	c_program_text= """
		struct R{ i32& x; }
		fn Foo( R r )
		{
			// It's fine to capture "R" by reference - it contains no second order inner references.
			auto f= lambda[&]() { auto& r_ref= r; };
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferenceIndirectionDepthExceeded_ForLambdas_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f0= lambda[&]() : i32 { return x; };
			// It's fine to capture "f0" by reference - it contains no second order inner references.
			auto f1= lambda[&]() : i32 { return f0(); };
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferenceIndirectionDepthExceeded_ForLambdas_Test2():
	c_program_text= """
		struct T{ i32& x; }
		struct R{ T& t; }
		fn Foo( R r )
		{
			// Since captured by reference variable becomes a reference field
			// it's not possible to capture by reference a variable with second order references inside.
			auto f= lambda[&]() { auto& r_ref= r; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceIndirectionDepthExceeded", 8 ) )


def ReferenceIndirectionDepthExceeded_ForLambdas_Test3():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f0= lambda[&]() : i32 { return x; }; // "f0" captures a reference.
			auto f1= lambda[&]() : i32 { return f0(); }; // "f1" captures a reference to "f0" and thus has a second order reference inside.
			auto f2= lambda[&]() : i32 { return f1(); }; // Error here - reference indirection limit reached.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( not HasError( errors_list, "ReferenceIndirectionDepthExceeded", 5 ) )
	assert( not HasError( errors_list, "ReferenceIndirectionDepthExceeded", 6 ) )
	assert( HasError( errors_list, "ReferenceIndirectionDepthExceeded", 7 ) )


def AccessingLambdaCapturedValueIsNotAllowed_Test0():
	c_program_text= """
		fn Foo()
		{
			auto x= 9899;
			auto f= lambda[=]() : i32 { return x; };
			auto x_copy= f.x; // Accessing value field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingNonpublicClassMember", 6 ) )


def AccessingLambdaCapturedValueIsNotAllowed_Test1():
	c_program_text= """
		fn Foo()
		{
			auto y= 3.5f;
			auto f= lambda[&]() : f32 { return y; };
			auto x_copy= f.y; // Accessing reference field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingNonpublicClassMember", 6 ) )


def LambdaIsNotEqualityComparable_Test0():
	c_program_text= """
		fn Foo()
		{
			auto f= lambda(){};
			auto eq= f == f;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def LambdaIsNotEqualityComparable_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f= lambda[=]() : i32 { return x; };
			auto ne= f != f;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 6 ) )


def LambdaIsNotEqualityComparable_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f= lambda[&]() : i32 { return x; };
			auto eq= f == f;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 6 ) )


def ReferenceNotationForLambda_Test0():
	c_program_text= """
		fn Foo()
		{
			var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
			auto f= lambda( i32& x ) : i32 & @(return_references) { return x; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationForLambda", 5 ) )


def ReferenceNotationForLambda_Test1():
	c_program_text= """
		struct R{ i32& x; }
		fn Foo()
		{
			var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
			auto f= lambda( i32& x ) : R @(return_inner_references) { halt; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationForLambda", 6 ) )


def ReferenceNotationForLambda_Test2():
	c_program_text= """
		struct R{ i32& x; }
		fn Foo()
		{
			var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1_" ] ];
			auto f= lambda( R &mut r, i32& x ) @( reference_pollution ) { halt; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationForLambda", 6 ) )


def VariableIsNotCapturedByLambda_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			// This non-capture lambda captures an external variable. Should produce an error.
			auto f= lambda() : i32 { return x; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableIsNotCapturedByLambda", 6 ) )


def VariableIsNotCapturedByLambda_Test1():
	c_program_text= """
		fn Foo( f32 x )
		{
			// This non-capture lambda captures an external variable - function parameter. Should produce an error.
			auto f= lambda() : f32 { return x * 1.7f; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableIsNotCapturedByLambda", 5 ) )


def VariableIsNotCapturedByLambda_Test2():
	c_program_text= """
		fn Foo()
		{
			var u32 x(0);
			auto f0=
				lambda[&]() : u32
				{
					auto f1=
						lambda[&]() : u32
						{
							// Capture a variable that is not captured specially by the outer lambda.
							// This should not be allowed.
							return x;
						};
					return f1();
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableIsNotCapturedByLambda", 13 ) )


def VariableIsNotCapturedByLambda_Test3():
	c_program_text= """
		fn Foo()
		{
			var u32 x(0);
			auto f0=
				lambda[=]() : u32
				{
					auto f1=
						lambda[=]() : u32
						{
							auto f2=
								lambda[=]() : u32
								{
									// Capture a variable that is not captured specially by the outer lambda.
									// This should not be allowed.
									return x;
								};
							return f2();
						};
					return f1();
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableIsNotCapturedByLambda", 16 ) )


def VariableIsNotCapturedByLambda_Test4():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			// Specify empty capture list - without actually referenced variable.
			auto f= lambda[]() : i32 { return x; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableIsNotCapturedByLambda", 6 ) )


def VariableIsNotCapturedByLambda_Test5():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0, y= 0;
			// Specify different from the used variable.
			auto f= lambda[&x]() : i32 { return y; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "VariableIsNotCapturedByLambda", 6 ) )


def LambaCaptureIsNotConstexpr_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 constexpr x= 333;
			auto f=
				lambda[=]()
				{
					// Captured by lambda "constexpr" local variables are not "constexpr" in the lambda.
					static_assert( x == 333 );
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StaticAssertExpressionIsNotConstant", 9 ) )


def LambaCaptureIsNotConstexpr_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 constexpr x= 333;
			auto f=
				lambda[&]()
				{
					// Captured by lambda "constexpr" local variables are not "constexpr" in the lambda.
					static_assert( x == 333 );
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StaticAssertExpressionIsNotConstant", 9 ) )


def LambaCaptureIsNotConstexpr_Test2():
	c_program_text= """
		var i32 constexpr x= 333;
		fn Foo()
		{
			auto f=
				lambda()
				{
					// Ok - "x" is not a captured variable but global variable and still remains "constexpr"
					static_assert( x == 333 );
				};
		}
	"""
	tests_lib.build_program( c_program_text )


def LambaCaptureIsNotConstexpr_Test3():
	c_program_text= """
		fn Foo()
		{
			auto f=
				lambda[x= 4455]()
				{
					// Captured by lambda "constexpr" expressions are not "constexpr" in the lambda.
					static_assert( x == 4455 );
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "StaticAssertExpressionIsNotConstant", 8 ) )


def DeriveFromLambda_Test0():
	c_program_text= """
		auto f= lambda(){};
		type FType= typeof(f);
		class C : FType {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CanNotDeriveFromThisType", 4 ) )


def GlobalsLoopForLambda_Test0():
	c_program_text= """
		auto f= lambda() : i32 { return f(); };
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalsLoopDetected", 2 ) )


def GlobalsLoopForLambda_Test1():
	c_program_text= """
		auto f= lambda() : i32 { return g(); };
		auto g= lambda() : i32 { auto mut f_copy= f; return 0; };
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalsLoopDetected", 2 ) )


def LambdaReferencePollutionForThis_Test0():
	c_program_text= """
	struct R{ i32 &mut x; }
	var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
	fn MakePollution( R &mut r, i32 &mut x ) @(pollution) {}
	fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var R r{ .x= x };
			auto mut f=
				lambda[=] mut ( i32 &mut a )
				{
					// Perform pollution for local copy of "r".
					MakePollution( r, a );
				};
			f(y);
			++y; // Can't change this variable, since it has a reference inside lambda "f".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 16 ) )


def LambdaReferencePollutionForThis_Test1():
	c_program_text= """
	struct R{ i32 &mut x; }
	var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
	fn MakePollution( R &mut r, i32 &mut x ) @(pollution) {}
	fn Foo()
		{
			var i32 mut x= 0, mut y= 0, mut z= 0;
			auto mut f=
				lambda[=] mut ( R &mut r )
				{
					// Perform pollution for argument "r" by local copy of "y".
					MakePollution( r, y );
				};

			var R mut r{ .x= x };
			f( r );
			var R mut another_r{ .x= z };
			f( another_r ); // Can't access this lambda anymore, because there is a mutable reference to it inside "r".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 18 ) )


def ExplicitCaptureListErrors_Test0():
	c_program_text= """
		fn Foo()
		{
			// Error - name "x" not found.
			lambda [x](){};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NameNotFound", 5 ) )


def ExplicitCaptureListErrors_Test1():
	c_program_text= """
		fn Foo()
		{
			// Error - name "f64" is not a variable.
			lambda [f64](){};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedVariable", 5 ) )


def ExplicitCaptureListErrors_Test2():
	c_program_text= """
		struct S
		{
			i32 x;
			fn Foo( this )
			{
				// Error - name "f64" is not a variable but a class field.
				lambda [x](){};
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedVariable", 8 ) )


def ExplicitCaptureListErrors_Test3():
	c_program_text= """
		var i32 x= 0;
		fn Foo()
		{
			lambda [x](){}; // Error - capturing global variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedVariable", 5 ) )


def ExplicitCaptureListErrors_Test4():
	c_program_text= """
		fn Foo()
		{
			lambda [this](){};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ThisUnavailable", 4 ) )


def ExplicitCaptureListErrors_Test5():
	c_program_text= """
		struct S
		{
			fn Foo( this )
			{
				// Can't capture "this" even with usage of capture list.
				auto f= lambda [this](){};
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedVariable", 7 ) )


def ExplicitCaptureListErrors_Test6():
	c_program_text= """
		fn Foo()
		{
			auto x= 6765;
			auto f0=
				lambda[&]() : i32
				{
					// Specify a variable from outer lambda in capture list of inner lambda.
					// This should still not be allowed.
					auto f1=
						lambda[x]() : i32
						{
							return x;
						};
					return f1();
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedVariable", 11 ) )


def DuplicatedCapture_Test0():
	c_program_text= """
		fn Foo()
		{
			auto x= 0;
			lambda [x, x](){}; // Capturing the same variable twice.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DuplicatedCapture", 5 ) )


def DuplicatedCapture_Test1():
	c_program_text= """
		fn Foo()
		{
			auto x= 0;
			lambda [&x, x](){}; // Capturing the same variable twice with different reference modifiers.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DuplicatedCapture", 5 ) )


def UnusedCapture_Test0():
	c_program_text= """
		fn Foo()
		{
			auto x= 0;
			lambda [x](){}; // "x" is not used.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnusedCapture", 5 ) )


def UnusedCapture_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			lambda [&x]()
			{
				static_if( false )
				{
					++x; // This doesn't count as capture, because it happens in false branch of static_if.
				}
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UnusedCapture", 5 ) )


def ExpectedReferenceValue_ForCaptureListExpression_Test0():
	c_program_text= """
		fn Foo()
		{
			// Initialize lambda reference with value.
			lambda[ &x= 42 ]() : i32 { return x; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 5 ) )


def ExpectedReferenceValue_ForCaptureListExpression_Test1():
	c_program_text= """
		fn Foo()
		{
			// Initialize lambda reference with value call result.
			lambda[ &x= Bar() ]() : f32 { return x; };
		}
		fn Bar() : f32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 5 ) )


def ExpectedReferenceValue_ForCaptureListExpression_Test2():
	c_program_text= """
		fn Foo( u32 x, u32 y )
		{
			// Initialize lambda reference with binary operator result.
			lambda[ &r= x + y ]() : f32 { return r; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 5 ) )


def ExpectedReferenceValue_ForCaptureListExpression_Test3():
	c_program_text= """
		fn Foo()
		{
			// Initialize lambda reference with temporary value.
			lambda[ &s= S(33) ]() : i32 { return s.x; };
		}
		struct S
		{
			i32 x;
			fn constructor( i32 in_x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 5 ) )


def UsingKeywordAsName_ForLambdaCaptureList_Test0():
	c_program_text= """
		fn Foo()
		{
			lambda[ this= 42 ]() : i32 { return tris; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UsingKeywordAsName", 4 ) )


def UsingKeywordAsName_ForLambdaCaptureList_Test1():
	c_program_text= """
		fn Foo()
		{
			lambda[ conversion_constructor= 42 ]() : i32 { return conversion_constructor; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UsingKeywordAsName", 4 ) )


def UsingKeywordAsName_ForLambdaCaptureList_Test2():
	c_program_text= """
		fn Foo()
		{
			lambda[ f64= 42 ]() : i32 { return f64; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "UsingKeywordAsName", 4 ) )


def DestroyedVariableStillHasReferences_ForLambdaCaptureListExpression_Test0():
	c_program_text= """
		var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1_" ] ];
		struct S
		{
			i32& x;
			fn constructor( i32& in_x ) @(reference_pollution)
				( x= in_x ) {}
		}
		fn Foo()
		{
			// Error - "f" contains reference to temporary "42".
			auto f= lambda[ s= S(42) ] () : i32 { return s.x; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 12 ) )


def DestroyedVariableStillHasReferences_ForLambdaCaptureListExpression_Test1():
	c_program_text= """
		var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1_" ] ];
		struct S
		{
			i32& x;
			fn constructor( i32& in_x ) @(reference_pollution)
				( x= in_x ) {}
		}
		fn Foo()
		{
			// Error - "f" contains reference to temporary call result.
			auto f= lambda[ s= S( Bar() ) ] () : i32 { return s.x; };
		}
		fn Bar() : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 12 ) )


def DestroyedVariableStillHasReferences_ForByvalLambda_Test0():
	c_program_text= """
		fn Foo()
		{
			auto x= 0;
			auto f=
				lambda[=] byval () : i32&
				{
					// Capture copy of "x" inside lambda.
					// Since this is "byval" lambda "this" will be destroyed at the end of this function execution.
					// This makes impossible returning of reference to "x".
					return x;
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 11 ) )


def DestroyedVariableStillHasReferences_ForByvalLambda_Test1():
	c_program_text= """
		struct R{ i32& x; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn MakePollution( R &mut r, i32& x ) @(pollution) {}
		fn Foo()
		{
			auto x= 0;
			auto f=
				lambda[=] byval ( R &mut r )
				{
					// Save reference to captured by value variable "x" in an argument.
					// But because this lambda is "byval", "this" including captured variable is destroyed at the end of this function.
					MakePollution( r, x );
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 14 ) )


def AccessingMovedVariable_ForByvalMutLambda_Test0():
	c_program_text= """
		fn Foo()
		{
			auto f=
				lambda[x= 34] byval mut ()
				{
					move(x);
					auto y= x;
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingMovedVariable", 8 ) )


def AccessingMovedVariable_ForByvalMutLambda_Test1():
	c_program_text= """
		fn Foo()
		{
			auto f=
				lambda[x= 34] byval mut ()
				{
					move(x);
					move(x);
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingMovedVariable", 8 ) )


def LambdaCapturedVariableMoveErrors_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut f=
				lambda[x= 123] byval mut ()
				{
					auto& x_ref= x;
					move(x); // Can't move - there is a reference to "x".
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MovedVariableHasReferences", 8 ) )


def LambdaCapturedVariableMoveErrors_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut f=
				lambda[x= 123] byval imut ()
				{
					move(x); // Can't move  - "byval this" is immutable.
				};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 7 ) )


def TypesMismtach_ForAutoReturnLambda_Test0():
	c_program_text= """
		fn Foo()
		{
			lambda( bool b ) : auto
			{
				if( b ) { return 17; }
				return 13u; // Expected "i32", got "u32"
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 7 ) )


def TypesMismtach_ForAutoReturnLambda_Test1():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 4 ] arr= zero_init;
			lambda[&]( [ i32, 5 ]& arg ) : auto&
			{
				if( arg[0] == 0 ) { return arg; }
				return arr; // Expected [ i32, 5 ], got [ i32, 4 ].
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 8 ) )


def ExpectedReferenceValue_ForAutoReturnLambda_Test0():
	c_program_text= """
		fn Foo()
		{
			lambda( ) : auto&
			{
				return false;
			};
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedReferenceValue", 6 ) )
