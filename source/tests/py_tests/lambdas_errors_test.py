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
	assert( HaveError( errors_list, "ThisUnavailable", 7 ) )


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
	assert( HaveError( errors_list, "ThisUnavailable", 9 ) )


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
	assert( HaveError( errors_list, "ThisUnavailable", 9 ) )


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
	assert( HaveError( errors_list, "CouldNotSelectOverloadedFunction", 9 ) )


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
	assert( HaveError( errors_list, "BaseUnavailable", 10 ) )


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
	assert( HaveError( errors_list, "ClassFieldAccessInStaticMethod", 10 ) )


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
	assert( HaveError( errors_list, "ClassFieldAccessInStaticMethod", 13 ) )


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
	assert( HaveError( errors_list, "ClassFieldAccessInStaticMethod", 10 ) )


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
	assert( HaveError( errors_list, "ThisUnavailable", 9 ) )


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
	assert( HaveError( errors_list, "CopyConstructValueOfNoncopyableType", 10 ) )


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
	assert( HaveError( errors_list, "CopyConstructValueOfNoncopyableType", 10 ) )


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
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 8 ) )


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
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 7 ) )


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
	assert( HaveError( errors_list, "StructInitializerForNonStruct", 5 ) )


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
	assert( HaveError( errors_list, "StructInitializerForNonStruct", 6 ) )


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
	assert( HaveError( errors_list, "ZeroInitializerForClass", 5 ) )


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
	assert( HaveError( errors_list, "ZeroInitializerForClass", 6 ) )


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
	assert( HaveError( errors_list, "ZeroInitializerForClass", 6 ) )


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
	assert( HaveError( errors_list, "ReferenceProtectionError", 15 ) )


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
	assert( HaveError( errors_list, "MovedVariableHaveReferences", 7 ) )


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
	assert( HaveError( errors_list, "MovedVariableHaveReferences", 13 ) )


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
	assert( HaveError( errors_list, "ReferenceProtectionError", 12 ) )


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
	assert( HaveError( errors_list, "ReferenceProtectionError", 6 ) )


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
	assert( HaveError( errors_list, "ReferenceProtectionError", 7 ) )


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
	assert( HaveError( errors_list, "ExpectedReferenceValue", 8 ) )


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
	assert( HaveError( errors_list, "ExpectedReferenceValue", 9 ) )


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
	assert( HaveError( errors_list, "ExpectedReferenceValue", 9 ) )


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
	assert( HaveError( errors_list, "ExpectedReferenceValue", 9 ) )


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
	assert( HaveError( errors_list, "ExpectedReferenceValue", 9 ) )


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
	assert( HaveError( errors_list, "ExpectedVariable", 8 ) or HaveError( errors_list, "ExpectedReferenceValue", 8 ) )


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
	assert( HaveError( errors_list, "ExpectedVariable", 8 ) )


def ReferenceFieldOfTypeWithReferencesInside_ForLambdas_Test0():
	c_program_text= """
		struct R{ i32& x; }
		fn Foo( R r )
		{
			// Since captured by reference variable becames a reference field
			// it's not possible to capture by reference a variable with references inside.
			// It's still not allowed to create reference fields of types with references inside.
			auto f= lambda[&]() { auto& r_ref= r; };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceFieldOfTypeWithReferencesInside", 8 ) )


def ReferenceFieldOfTypeWithReferencesInside_ForLambdas_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0;
			auto f0= lambda[&]() : i32 { return x; };
			// Capture another lambda by reference, which contains references inside.
			auto f1= lambda[&]() : i32 { return f0(); };
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceFieldOfTypeWithReferencesInside", 7 ) )


def AccessingLambdaCapturedValueIsNotAllowed_Tes0():
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
	assert( HaveError( errors_list, "AccessingNonpublicClassMember", 6 ) )


def AccessingLambdaCapturedValueIsNotAllowed_Tes1():
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
	assert( HaveError( errors_list, "AccessingNonpublicClassMember", 6 ) )


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
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 5 ) )


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
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 6 ) )


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
	assert( HaveError( errors_list, "OperationNotSupportedForThisType", 6 ) )


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
	assert( HaveError( errors_list, "ReferenceNotationForLambda", 5 ) )


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
	assert( HaveError( errors_list, "ReferenceNotationForLambda", 6 ) )


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
	assert( HaveError( errors_list, "ReferenceNotationForLambda", 6 ) )


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
	assert( HaveError( errors_list, "VariableIsNotCapturedByLambda", 6 ) )


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
	assert( HaveError( errors_list, "VariableIsNotCapturedByLambda", 5 ) )


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
	assert( HaveError( errors_list, "VariableIsNotCapturedByLambda", 13 ) )


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
	assert( HaveError( errors_list, "VariableIsNotCapturedByLambda", 16 ) )


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
	assert( HaveError( errors_list, "VariableIsNotCapturedByLambda", 6 ) )


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
	assert( HaveError( errors_list, "VariableIsNotCapturedByLambda", 6 ) )


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
	assert( HaveError( errors_list, "StaticAssertExpressionIsNotConstant", 9 ) )


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
	assert( HaveError( errors_list, "StaticAssertExpressionIsNotConstant", 9 ) )


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


def DeriveFromLambda_Test0():
	c_program_text= """
		auto f= lambda(){};
		type FType= typeof(f);
		class C : FType {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CanNotDeriveFromThisType", 4 ) )


def GlobalsLoopForLambda_Test0():
	c_program_text= """
		auto f= lambda() : i32 { return f(); };
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GlobalsLoopDetected", 2 ) )


def GlobalsLoopForLambda_Test1():
	c_program_text= """
		auto f= lambda() : i32 { return g(); };
		auto g= lambda() : i32 { auto mut f_copy= f; return 0; };
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "GlobalsLoopDetected", 2 ) )


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
	assert( HaveError( errors_list, "ReferenceProtectionError", 16 ) )


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
	assert( HaveError( errors_list, "ReferenceProtectionError", 18 ) )


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
	assert( HaveError( errors_list, "NameNotFound", 5 ) )


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
	assert( HaveError( errors_list, "ExpectedVariable", 5 ) )


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
	assert( HaveError( errors_list, "ExpectedVariable", 8 ) )


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
	assert( HaveError( errors_list, "ExpectedVariable", 5 ) )


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
	assert( HaveError( errors_list, "DuplicatedCapture", 5 ) )


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
	assert( HaveError( errors_list, "DuplicatedCapture", 5 ) )


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
	assert( HaveError( errors_list, "UnusedCapture", 5 ) )


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
	assert( HaveError( errors_list, "UnusedCapture", 5 ) )
