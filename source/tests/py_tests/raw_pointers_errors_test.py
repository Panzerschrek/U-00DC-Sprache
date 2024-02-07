from py_tests_common import *

def ValueIsNotReference_Test0():
	c_program_text= """
		fn Foo()
		{
			$<(42); // Converting immediate value to pointer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ValueIsNotReference", 4 ) )


def ValueIsNotReference_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 66, y= -5;
			$<(x + y); // Converting binary operator result (immediate value) to pointer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ValueIsNotReference", 5 ) )


def ValueIsNotReference_Test2():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			$<(S()); // Converting  temp variable construction result (immediate value) to pointer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ValueIsNotReference", 5 ) )


def ValueIsNotReference_Test3():
	c_program_text= """
		fn Bar() : tup[ f32, bool ]
		{
			var tup[ f32, bool ] t[ 0.25f, true ];
			return t;
		}
		fn Foo()
		{
			$<(Bar()); // Converting function call result (immediate value) to pointer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ValueIsNotReference", 9 ) )


def ExpectedReferenceValue_ForReferenceToRawPointerConversion_Test0():
	c_program_text= """
		fn Foo()
		{
			auto x= 0;
			$<(x); // Error, `x` is not mutable
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 5 ) )


def ExpectedReferenceValue_ForReferenceToRawPointerConversion_Test1():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 4 ] arr= zero_init;
			$<(arr[2]); // Error, value is not a mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 5 ) )


def ExpectedReferenceValue_ForReferenceToRawPointerConversion_Test2():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		fn Foo()
		{
			var S s= zero_init;
			$<(s.y); // Error, value is not a mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 6 ) )


def RawPointerToReferenceConversionOutsideUnsafeBlock_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			auto ptr= $<(x);
			$>(ptr);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "RawPointerToReferenceConversionOutsideUnsafeBlock", 6 ) )


def RawPointerToReferenceConversionOutsideUnsafeBlock_Test1():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
				auto mut x= 0;
				auto ptr= $<(x);
				safe
				{
					$>(ptr);
				}
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "RawPointerToReferenceConversionOutsideUnsafeBlock", 10 ) )


def ValueIsNotPointer_Test0():
	c_program_text= """
		fn Foo()
		{
			$>(0); // Expected pointer, got value of fundamental type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ValueIsNotPointer", 4 ) )


def ValueIsNotPointer_Test1():
	c_program_text= """
		fn Foo()
		{
			var size_type s= 0;
			$>(s); // Expected pointer, got reference of fundamental type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ValueIsNotPointer", 5 ) )


def ValueIsNotPointer_Test2():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			var S s= zero_init;
			$>(s); // Expected pointer, got reference of struct type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ValueIsNotPointer", 6 ) )


def ValueIsNotPointer_Test3():
	c_program_text= """
		fn Foo()
		{
			var (fn()) this_ptr(Foo);
			$>(this_ptr); // Expected pointer, got function pointer (which is different kind of type).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ValueIsNotPointer", 5 ) )


def AddTooLargeIntegerToRawPointer_Test0():
	c_program_text= """
		fn Foo()
		{
			var $(i32) ptr= zero_init;
			ptr + i128(1);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def AddTooLargeIntegerToRawPointer_Test1():
	c_program_text= """
		fn Foo()
		{
			var $(i32) ptr= zero_init;
			u128(10) + ptr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def AddFloatToRawPointer_Test0():
	c_program_text= """
		fn Foo()
		{
			var $(i32) ptr= zero_init;
			ptr + 17.0f;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def AddRawPointerToRawPointer_Test0():
	c_program_text= """
		fn Foo()
		{
			var $(i32) ptr= zero_init;
			ptr + ptr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def AddStructToRawPointer_Test0():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			var $(i32) ptr= zero_init;
			var S s;
			s + ptr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 7 ) )


def SubtractTooLargeIntegerFromRawPointer_Test0():
	c_program_text= """
		fn Foo()
		{
			var $(i32) ptr= zero_init;
			ptr - i128(666);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def SubtractFloatFromRawPointer_Test0():
	c_program_text= """
		fn Foo()
		{
			var $(i32) ptr= zero_init;
			ptr - 17.0f;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def SubtructStructFromRawPointer_Test0():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			var $(i32) ptr= zero_init;
			var S s;
			ptr - s;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 7 ) )


def SubtractRawPointer_Test0():
	c_program_text= """
		fn Foo()
		{
			var $(i32) ptr= zero_init;
			66 - ptr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 5 ) )


def SubtractRawPointer_Test1():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			var $(i32) ptr= zero_init;
			S() - ptr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 6 ) )


def DifferenceBetweenRawPointersWithDifferentTypes_Test0():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			var $(i32) ptr_i= zero_init;
			var $(S) ptr_s= zero_init;
			s - ptr_i;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 7 ) )


def DifferenceBetweenRawPointersWithDifferentTypes_Test1():
	c_program_text= """
		fn Foo()
		{
			var $(i32) ptr_i= zero_init;
			var $(u32) ptr_s= zero_init;
			s - ptr_i; // Should generate an error even if size of element is same.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 6 ) )


def DifferenceBetweenRawPointersWithZeroElementSize_Test0():
	c_program_text= """
		fn Foo()
		{
			var $(tup[]) ptr= zero_init;
			ptr - ptr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "DifferenceBetweenRawPointersWithZeroElementSize", 5 ) )


def DifferenceBetweenRawPointersWithZeroElementSize_Test1():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			var $(S) ptr= zero_init;
			ptr - ptr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "DifferenceBetweenRawPointersWithZeroElementSize", 6 ) )


def DifferenceBetweenRawPointersWithZeroElementSize_Test2():
	c_program_text= """
		fn Foo()
		{
			var $( [ i32, 0 ] ) ptr= zero_init;
			ptr - ptr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "DifferenceBetweenRawPointersWithZeroElementSize", 5 ) )


def DifferenceBetweenRawPointersWithZeroElementSize_Test3():
	c_program_text= """
		fn Foo()
		{
			var $(void) ptr= zero_init; // Size of "void" is zero.
			ptr - ptr;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "DifferenceBetweenRawPointersWithZeroElementSize", 5 ) )


def AdditiveAssignmentErrors_ForRawPointers_Test0():
	c_program_text= """
		fn Foo()
		{
			var $(i32) ptr= zero_init;
			ptr+= 66; // Can not mutate immutable pointer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedReferenceValue", 5 ) )


def AdditiveAssignmentErrors_ForRawPointers_Test1():
	c_program_text= """
		fn Foo()
		{
			var $(i32) mut ptr= zero_init;
			ptr+= 66.0f; // Can not add float value.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 5 ) )


def AdditiveAssignmentErrors_ForRawPointers_Test2():
	c_program_text= """
		fn Foo()
		{
			var $(i32) mut ptr= zero_init;
			ptr*= 5; // Can not multiply pointer by integer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 5 ) )


def AdditiveAssignmentErrors_ForRawPointers_Test3():
	c_program_text= """
		template<//> struct get_signed_type  </ u32 /> { type t= i32; }
		template<//> struct get_signed_type  </ u64 /> { type t= i64; }
		fn Foo()
		{
			var $(i32) mut ptr= zero_init;
			var get_signed_type</size_type/>::t mut i(0);
			i+= ptr; // Can not add pointer to integer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 8 ) )


def AdditiveAssignmentErrors_ForRawPointers_Test4():
	c_program_text= """
		fn Foo()
		{
			var $(i32) mut ptr0= zero_init, ptr1= zero_init;
			ptr0-= ptr1; // Can not subtract pointer from pointer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 5 ) )


def RawPointerTypeIsNotConstexpr_Test0():
	c_program_text= """
		fn constexpr Foo()
		{
			var $(i32) ptr= zero_init;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ConstexprFunctionContainsUnallowedOperations", 2 ) )


def RawPointerTypeIsNotConstexpr_Test1():
	c_program_text= """
		fn constexpr Foo()
		{
			auto mut x= 0;
			$<(x); // Reference to raw pointer conversion breaks constexpr rules.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ConstexprFunctionContainsUnallowedOperations", 2 ) )


def RawPointerTypeIsNotConstexpr_Test2():
	c_program_text= """
		fn constexpr Foo()
		{
			auto mut x= 0;
			auto ptr= $<(x);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ConstexprFunctionContainsUnallowedOperations", 2 ) )


def RawPointerTypeIsNotConstexpr_Test3():
	c_program_text= """
		fn constexpr Foo( $(i32) ptr ){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidTypeForConstexprFunction", 2 ) )


def RawPointerTypeIsNotConstexpr_Test4():
	c_program_text= """
		struct S{ $(i32) ptr; }
		fn constexpr Foo( S s ){}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidTypeForConstexprFunction", 3 ) )


def RawPointerTypeIsNotConstexpr_Test5():
	c_program_text= """
		fn Foo()
		{
			var $(i32) constexpr ptr= zero_init;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidTypeForConstantExpressionVariable", 4 ) )


def RawPointerTypeIsNotConstexpr_Test6():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			auto constexpr ptr= $<(x);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "VariableInitializerIsNotConstantExpression", 5 ) or HasError( errors_list, "InvalidTypeForConstantExpressionVariable", 5 ) )


def PointerDifferenceForUnrelatedTypes_Test0():
	c_program_text= """
		fn Foo( $(i32) x, $(f32) y )
		{
			x - y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 4 ) )


def PointerDifferenceForUnrelatedTypes_Test1():
	c_program_text= """
		fn Foo( $(byte8) x, $(u8) y )
		{
			x - y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 4 ) )
