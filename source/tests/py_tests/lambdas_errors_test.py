from py_tests_common import *

def ThisUnavailable_ForLambdaClassThis_Tetst0():
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


def ThisUnavailable_ForLambdaClassThis_Tetst1():
	# "this" should be not available in lambdas for hidden "this" parameter of a lambda function.
	# This is true even if lamda captures names by value.
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
