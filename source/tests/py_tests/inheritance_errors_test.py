from py_tests_common import *


def NameIsNotTypeName_ForClassParent_Test0():
	c_program_text= """
		fn Foo();
		class A : Foo {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NameIsNotTypeName", 3 ) )


def NameIsNotTypeName_ForClassParent_Test1():
	c_program_text= """
		var i32 some_number= 675;
		class A : some_number {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NameIsNotTypeName", 3 ) )


def BaseUnavailable_Test0():
	c_program_text= """
		class A
		{
			fn constructor()
			( base() )   // error, trying intitialize "base", but current class have no base class.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "BaseUnavailable" )
	assert( errors_list[0].src_loc.line == 5 )


def BaseUnavailable_Test1():
	# TODO - fix this
	c_program_text= """
		class A abstract
		{
			i32 x;
			fn constructor()( x= 0 ){}
		}
		class B
		{
			i32 y;
			fn constructor()( y= base.y ) {}  // Error, "base" in constructor initializer list is unavailable, because "base" is abstract.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "BaseUnavailable", 10 ) )


def BaseUnavailable_Test2():
	c_program_text= """
		class A polymorph
		{}
		class B : A
		{
			fn Foo()
			{
				base;  // Error - "base" access in static function.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "BaseUnavailable" )
	assert( errors_list[0].src_loc.line == 8 )


def BaseUnavailable_Test3():
	c_program_text= """
		class A
		{
			fn Foo( this )
			{
				base;  // Error - class have no "base" class.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "BaseUnavailable" )
	assert( errors_list[0].src_loc.line == 6 )


def FieldIsNotInitializedYet_ForBase_Test0():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x );
		}
		class B : A
		{
			i32 y;
			fn constructor() ( y= base.x, base(666) ) {} // Accessing 'base' before its initialization
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FieldIsNotInitializedYet", 10 ) )


def FieldIsNotInitializedYet_ForBase_Test1():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x );
		}
		class B : A
		{
			i32 y;
			fn constructor() ( y= x, base(666) ) {} // Accessing 'base.x' before 'base" initialization
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FieldIsNotInitializedYet", 10 ) )


def FieldIsNotInitializedYet_ForBase_Test2():
	c_program_text= """
		class A polymorph
		{
			i32 x;
			fn constructor( i32 in_x );
		}
		class B : A
		{
			i32 y;
			fn constructor() ( y= A::x, base(666) ) {} // Accessing 'A::x' before 'base" initialization
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FieldIsNotInitializedYet", 10 ) )


def FieldIsNotInitializedYet_ForBase_Test3():
	c_program_text= """
		class A abstract
		{
			fn virtual pure Foo(this) : i32;
		}
		class B : A
		{
			i32 y;
			fn constructor() ( y= base.Foo() ) {} // It is unsafe to access "base" here, because it is abstract and whole "this" (with proper virtual table) is still unawailable.
			fn virtual override Foo(this) : i32 { return 765432; }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "FieldIsNotInitializedYet", 9 ) )


def CanNotDeriveFromThisType_Test0():
	c_program_text= """
		class A{}   // non-polymorph by-default
		class B : A{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CanNotDeriveFromThisType", 3 ) )


def CanNotDeriveFromThisType_Test1():
	c_program_text= """
		class A polymorph {}
		class B final : A{}
		class C : B {}    // Inherit from final class
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CanNotDeriveFromThisType", 4 ) )


def CanNotDeriveFromThisType_Test2():
	c_program_text= """
		class A : i32 {}   // Inherit from non-class
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CanNotDeriveFromThisType" )
	assert( errors_list[0].src_loc.line == 2 )


def DuplicatedParentClass_Test0():
	c_program_text= """
		class A interface {}
		class B : A, A {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "DuplicatedParentClass" )
	assert( errors_list[0].src_loc.line == 3 )


def DuplicatedParentClass_Test1():
	c_program_text= """
		class A interface {}
		type A_alias= A;
		class B interface {}
		class C : A, B, A_alias {}   // Using direct name and alias name for same base class
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "DuplicatedParentClass" )
	assert( errors_list[0].src_loc.line == 5 )


def DuplicatedBaseClass_Test0():
	c_program_text= """
		class A polymorph {}
		class B polymorph {}
		class C : A, B {}   // error - inherit more than one non-interface class.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "DuplicatedBaseClass", 4 ) )


def FieldsForInterfacesNotAllowed_Test0():
	c_program_text= """
		class A interface
		{
			f32 x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FieldsForInterfacesNotAllowed" )
	assert( errors_list[0].src_loc.line == 2 )


def BaseClassForInterface_Test0():
	c_program_text= """
		class A polymorph {} // non-interface
		class B interface : A {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "BaseClassForInterface" )
	assert( errors_list[0].src_loc.line == 3 )


def ConstructorForInterface_Test0():
	c_program_text= """
		class A interface
		{
			fn constructor(){}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructorForInterface" )
	assert( errors_list[0].src_loc.line == 2 )


def ConstructingAbstractClassOrInterface_Test0():
	c_program_text= """
		class A interface {}
		fn Foo()
		{
			var A a;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructingAbstractClassOrInterface" )
	assert( errors_list[0].src_loc.line == 5 )


def ConstructingAbstractClassOrInterface_Test1():
	c_program_text= """
		class A interface {}
		fn GetA() : A&;
		fn Foo()
		{
			auto a= GetA();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ConstructingAbstractClassOrInterface", 6 ) )


def ConstructingAbstractClassOrInterface_Test2():
	c_program_text= """
		class A abstract {}
		fn Bar( A& a );
		fn Foo()
		{
			Bar( A() );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ConstructingAbstractClassOrInterface", 6 ) )


def ConstructingAbstractClassOrInterface_Test3():
	c_program_text= """
		class A abstract {}
		class S{ A a; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructingAbstractClassOrInterface" )
	assert( errors_list[0].src_loc.line == 3 )


def ConstructingAbstractClassOrInterface_Test4():
	c_program_text= """
		class A interface {}
		class S{ A& a; } // ok, because field is reference
	"""
	tests_lib.build_program( c_program_text )


def ConstructingAbstractClassOrInterface_Test5():
	c_program_text= """
		class A abstract {}
		fn GetA() : A&;
		fn Foo()
		{
			auto& a= GetA(); // ok, because is reference
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstructingAbstractClassOrInterface_Test6():
	c_program_text= """
		class A abstract {}
		fn GetA() : A&;
		fn Foo()
		{
			var A & a= GetA(); // ok, because is reference
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstructingAbstractClassOrInterface_Test7():
	c_program_text= """
		class A abstract {}
		fn Foo()
		{
			var [ A, 2 ] arr; // Array of abstact classes can not be constructed
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructingAbstractClassOrInterface" )
	assert( errors_list[0].src_loc.line == 5 )


def ConstructingAbstractClassOrInterface_Test8():
	c_program_text= """
		class A interface {}
		fn Foo()
		{
			var [ A, 2 ] arr; // Array of interfaces can not be constructed
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructingAbstractClassOrInterface" )
	assert( errors_list[0].src_loc.line == 5 )


def ConstructingAbstractClassOrInterface_Test9():
	c_program_text= """
		class A abstract {}
		fn Foo()
		{
			var tup[ A, bool ] arr; // Tuple of abstact classes can not be constructed
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructingAbstractClassOrInterface" )
	assert( errors_list[0].src_loc.line == 5 )


def ConstructingAbstractClassOrInterface_Test10():
	c_program_text= """
		class A interface {}
		fn Foo()
		{
			var tup[ f32, A ] arr; // Tuple of interfaces can not be constructed
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructingAbstractClassOrInterface" )
	assert( errors_list[0].src_loc.line == 5 )


def ConstructingAbstractClassOrInterface_Test11():
	c_program_text= """
		class A interface {}
		fn Foo()
		{
			var [ A, 0 ] arr; // ok - array size is zero
		}
	"""
	tests_lib.build_program( c_program_text )


def ConstructingAbstractClassOrInterface_Test12():
	c_program_text= """
	class A abstract
	{
		fn constructor( mut this, A& a )= default;
		fn virtual pure Foo(this);
	}

	class B final : A
	{
		fn virtual override Foo(this){}
	}

	fn Foo(A a);

	fn Bar()
	{
		Foo(B()); // Trying to construct value argument of abstract type "A", using its child "B".
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructingAbstractClassOrInterface" )
	assert( errors_list[0].src_loc.line == 17 )


def ConstructingAbstractClassOrInterface_Test13():
	c_program_text= """
	class A abstract
	{
		fn constructor( mut this, A& a )= default;
		fn virtual pure Foo(this);
	}

	class B final : A
	{
		fn virtual override Foo(this){}
	}

	fn Foo() : A
	{
		return B(); // Trying to construct return value of abstract class "A", using value of its child "B".
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructingAbstractClassOrInterface" )
	assert( errors_list[0].src_loc.line == 15 )


def ConstructingAbstractClassOrInterface_Test14():
	c_program_text= """
	class A abstract
	{
		fn constructor( mut this, A& a )= default;
		fn virtual pure Foo(this);
	}

	class B final : A
	{
		fn virtual override Foo(this){}
	}

	fn Bar()
	{
		var A a= B(); // Trying to use initializer expression to initialize value of abstract clas "A" using value of its child "B".
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructingAbstractClassOrInterface" )
	assert( errors_list[0].src_loc.line == 15 )


def ConstructingAbstractClassOrInterface_Test15():
	c_program_text= """
	class A abstract
	{
		fn constructor( mut this, A& a )= default;
		fn virtual pure Foo(this);
	}

	class B final : A
	{
		fn virtual override Foo(this){}
	}

	fn Bar()
	{
		var A a(B()); // Trying to use initializer expression to initialize value of abstract clas "A" using value of its child "B".
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructingAbstractClassOrInterface" )
	assert( errors_list[0].src_loc.line == 15 )


def ConstructingAbstractClassOrInterface_Test16():
	c_program_text= """
	class A abstract
	{
		fn constructor( mut this, A& a )= default;
		fn virtual pure Foo(this);
	}

	class B final : A
	{
		fn virtual override Foo(this){}
	}

	fn Bar()
	{
		var B mut b;
		take(cast_ref</A/>(b)); // Error, calling default constructor of abstract class "A" in "take" operator.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructingAbstractClassOrInterface" )
	assert( errors_list[0].src_loc.line == 16 )


def ConstructingAbstractClassOrInterface_Test17():
	c_program_text= """
		class A abstract
		{
			fn constructor(mut this, A& other)= default;
		}
		fn Foo( tup[ A ]& t )
		{
			for( el : t ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstructingAbstractClassOrInterface", 8 ) )


def ConstructingAbstractClassOrInterface_Test18():
	c_program_text= """
		class A abstract
		{
			fn constructor(mut this, A& other)= default;
		}
		fn Foo( A& a ) : A
		{
			return a; // Trying to copy-constructg abstract class.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstructingAbstractClassOrInterface", 8 ) )


def MoveAssignForNonFinalPolymorphClass_Test0():
	c_program_text= """
	class A polymorph {}
	class B final : A {}
	fn Bar()
	{
		var B mut b;
		cast_ref</A/>(b)= A(); // Error - move-assign value to reference of non-final polymorph class.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MoveAssignForNonFinalPolymorphClass" )
	assert( errors_list[0].src_loc.line == 7 )


def MoveAssignForNonFinalPolymorphClass_Tes1():
	c_program_text= """
	class A abstract {}
	class B final : A {}
	fn Bar()
	{
		var B mut b;
		b= B(); // Ok - move-assign for final polymorph class.
	}
	"""
	tests_lib.build_program( c_program_text )


def MoveAssignForNonFinalPolymorphClass_Test2():
	c_program_text= """
	class A polymorph {}
	fn Bar()
	{
		var A mut a;
		a= A(); // Error - move-assign value to reference of non-final polymorph class.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MoveAssignForNonFinalPolymorphClass" )
	assert( errors_list[0].src_loc.line == 6 )


def TakeForNonFinalPolymorphClass_Test0():
	c_program_text= """
	class A polymorph {}
	fn Bar()
	{
		var A mut a;
		take(a); // Error - taking polymorh non-final class.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TakeForNonFinalPolymorphClass" )
	assert( errors_list[0].src_loc.line == 6 )


def TakeForNonFinalPolymorphClass_Test1():
	c_program_text= """
	class A polymorph {}
	class B : A {}
	fn Bar()
	{
		var B mut b;
		take(b); // Error - taking polymorh non-final class (which is derived from some base class).
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TakeForNonFinalPolymorphClass" )
	assert( errors_list[0].src_loc.line == 7 )


def TakeForNonFinalPolymorphClass_Test2():
	c_program_text= """
	class A polymorph {}
	class B final : A {}
	fn Bar()
	{
		var B mut b;
		take(b); // Ok - taking final polymorph class.
	}
	"""
	tests_lib.build_program( c_program_text )


def EqualityCompareOperatorIsNotInherited_Test0():
	c_program_text= """
		class Base polymorph
		{
			op==(Base& l, Base& r) : bool = default;
		}
		class Derived : Base {} // Should not inherit "==" from base class. So, fetch of "==" from this class will find nothing.
		fn Foo(Derived& l, Derived& r)
		{
			l == r; // Should get error here and avoid call of "==" of "Base" class (with references cast).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 9 ) )


def EqualityCompareOperatorIsNotInherited_Test1():
	c_program_text= """
		class Base polymorph
		{
			op==(Base& l, i32 x) : bool { return false; }
		}
		class Derived : Base {} // Should not inherit "==" from base class. So, fetch of "==" from this class will find nothing.
		fn Foo(Derived& l)
		{
			l == 0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 9 ) )


def EqualityCompareOperatorIsNotInherited_Test2():
	c_program_text= """
		class Base polymorph
		{
			op==(Base& l, Base& r) : bool = default;
		}
		class Derived : Base
		{
			op==(Derived& l, Derived& r) : bool = default; // Introduce our own "==".
		}
		fn Foo(Derived& l, Derived& r)
		{
			auto eq= l == r; // Ok, call "Derived::=="
		}
	"""
	tests_lib.build_program( c_program_text )


def OrderCompareOperatorIsNotInherited_Test0():
	c_program_text= """
		class Base polymorph
		{
			op<=>(Base& l, Base& r) : i32 { return 0; }
		}
		class Derived : Base {} // Should not inherit "<=>" from base class. So, fetch of "<=>" from this class will find nothing.
		fn Foo(Derived& l, Derived& r)
		{
			l <=> r; // Should get error here and avoid call of "<=>" of "Base" class (with references cast).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "OperationNotSupportedForThisType", 9 ) )


def OrderCompareOperatorIsNotInherited_Test1():
	c_program_text= """
		class Base polymorph
		{
			op<=>( Base& l, i32 x ) : i32 { return 0; }
		}
		class Derived : Base {} // Should not inherit "<=>" from base class. So, fetch of "<=>" from this class will find nothing.
		fn Foo(Derived& l)
		{
			l <=> 0; // Should get error here and avoid call of "<=>" of "Base" class (with references cast).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "NoMatchBinaryOperatorForGivenTypes", 9 ) )


def OrderCompareOperatorIsNotInherited_Test2():
	c_program_text= """
		class Base polymorph
		{
			op<=>(Base& l, Base& r) : i32 { return 0; }
		}
		class Derived : Base
		{
			op<=>(Derived& l, Derived& r) : i32 { return cast_ref</Base/>(l) <=> cast_ref</Base/>(r); }
		}
		fn Foo(Derived& l, Derived& r)
		{
			auto cmp= l <=> r; // Ok, call Derived::<=>
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionDeclarationOutsideItsScope_ForInheritance_Test0():
	c_program_text= """
		class A polymorph
		{
			fn Foo();
		}
		class B : A {}
		fn B::Foo() {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "FunctionDeclarationOutsideItsScope", 7 ) or HasError( errors_list, "NameNotFound", 7 ) )


def FunctionDeclarationOutsideItsScope_ForInheritance_Test1():
	c_program_text= """
		class A polymorph
		{
			fn Foo() : i32;
		}
		class B : A
		{
			// This is actually a new function declaration (A::Foo is shadowed).
			fn Foo() : i32
			{
				return 678;
			}

			fn CallFoo() : i32 { return Foo(); }
		}
		fn Bar() : i32
		{
			return B::CallFoo();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Barv" )
	assert( call_result == 678 )


def MutableReferenceFieldAccessInDestructor_ForBaseField_Test0():
	c_program_text= """
		class A polymorph
		{
			i32 &mut x;
		}
		class B : A
		{
			fn destructor()
			{
				auto& x_ref= x; // Accessing mutable reference field, which is not allowed.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MutableReferenceFieldAccessInDestructor", 10 ) )


def MutableReferenceFieldAccessInDestructor_ForBaseField_Test1():
	c_program_text= """
		class A polymorph
		{
			i32 &mut x;
		}
		class B : A
		{
			fn destructor()
			{
				Foo(x); // Accessing mutable reference field, which is not allowed.
			}
		}
		fn Foo( i32 &mut x );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "MutableReferenceFieldAccessInDestructor", 10 ) )


def AccessingFieldWithMutableReferencesInsideInDestructor_ForBaseField_Test0():
	c_program_text= """
		struct S
		{
			i32 &mut x;
		}
		class A polymorph
		{
			S s;
		}
		class B : A
		{
			fn destructor()
			{
				auto &mut s= s; // Accessing field with mutable references inside, which is not allowed.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingFieldWithMutableReferencesInsideInDestructor", 14 ) )


def AccessingFieldWithMutableReferencesInsideInDestructor_ForBaseField_Test1():
	c_program_text= """
		struct S
		{
			i32 &mut x;
		}
		class A polymorph
		{
			S s;
		}
		class B : A
		{
			fn destructor()
			{
				Foo(s); // Accessing field with mutable references inside, which is not allowed.
			}
		}
		fn Foo( S s );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AccessingFieldWithMutableReferencesInsideInDestructor", 14 ) )


def ThisUnavailable_InDestructorOfStructWithMutableReferencesInside_ForBase_Test0():
	c_program_text= """
		class A polymorph
		{
			i32 &mut x;
		}
		class B : A
		{
			fn destructor()
			{
				auto& self= this; // Can't access "this", because this class contains mutable references inside.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ThisUnavailable", 10 ) )


def BaseUnavailable_InDestructorOfStructWithMutableReferencesInside_ForBase_Test1():
	c_program_text= """
		class A polymorph
		{
			i32 &mut x;
		}
		class B : A
		{
			fn destructor()
			{
				auto& b= base; // Can't access "base", because it contains mutable references inside.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "BaseUnavailable", 10 ) )


def BreakReferenceIndirectionDepthExceededWithInheritance_Test0():
	c_program_text= """
		class A interface
		{
		}
		class B : A
		{
			A& a_;
			fn constructor( A& a ) @(pollution)
				( a_= a )
			{}

			var [ [ [char8, 2], 2 ], 1 ] constexpr pollution[ [ "0a", "1_" ] ];
		}
		class C : A {}
		fn Foo()
		{
			// It shouldn't be possible to create a linked-list of arbitrary size in Ü, using only references.
			// But this code creates such list and this is an language defect.
			var C c;
			var B b0(c);
			var B b1(b0);
			var B b2(b1);
			var B b3(b2);
			var B b4(b3), b5(b4), b6(b5);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )


def BreakReferenceIndirectionDepthExceededWithInheritance_Test1():
	c_program_text= """
		class A interface
		{
			fn virtual pure Some( this, A& other );
			fn virtual pure MutateReferencedVariable( this );
		}
		class B : A
		{
			i32 &mut x_;
			fn constructor( i32 &mut x ) @(pollution)
				( x_= x )
			{}

			var [ [ [char8, 2], 2 ], 1 ] constexpr pollution[ [ "0a", "1_" ] ];

			fn virtual override Some( this, A& other )
			{
				var i32 &mut x_ref= x_; // Hold a mutable reference to this reference field.
				other.MutateReferencedVariable(); // Mutate referenced variable of "other". This may lead to a potenital shared mutable access, if "this" and "other" is the same object.
			}
			fn virtual override MutateReferencedVariable( this )
			{
				++x_;
			}
		}
		static_assert( typeinfo</B/>.reference_tag_count == 1s );
		fn Foo()
		{
			var i32 mut x= 0;
			var B b(x);
			// Pass "b" two times into the same function.
			// This doesn't trigger a reference protection error for inner reference tag of "b", since it's casted to "A" with no references indide.
			b.Some( b );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )


def BreakReferenceIndirectionDepthExceededWithInheritance_Test2():
	c_program_text= """
		class A interface
		{
			fn virtual pure Some( this );
			fn virtual pure MutateReferencedVariable( this );
		}
		class B : A
		{
			i32 &mut x_;
			fn constructor( i32 &mut x ) @(pollution)
				( x_= x )
			{}

			var [ [ [char8, 2], 2 ], 1 ] constexpr pollution[ [ "0a", "1_" ] ];

			fn virtual override Some( this )
			{
				var i32 &mut x_ref= x_; // Hold a mutable reference to this reference field.
				var A& a= this; // Perform reference cast here.
				a.MutateReferencedVariable(); // Get the second mutable access to the stored inner reference, which violates single mutable reference rule.
			}
			fn virtual override MutateReferencedVariable( this )
			{
				++x_; // Modify referenced variable, when another mutable reference to it exists.
			}
		}
		static_assert( typeinfo</B/>.reference_tag_count == 1s );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )


def BreakReferenceIndirectionDepthExceededWithInheritance_Test3():
	c_program_text= """
		class A interface
		{
			fn virtual pure Some( this );
		}
		class B : A
		{
			i32 &mut x_;
			fn constructor( i32 &mut x ) @(pollution)
				( x_= x )
			{}

			var [ [ [char8, 2], 2 ], 1 ] constexpr pollution[ [ "0a", "1_" ] ];
			var [ [ char8, 2 ], 1 ] return_references[ "0a" ];

			fn virtual override Some( this )
			{
				++x_; // Modify referenced variable, when another mutable reference to it exists.
			}

			fn GetX( this ) : i32 &mut @(return_references)
			{
				return x_;
			}
		}
		static_assert( typeinfo</B/>.reference_tag_count == 1s );

		fn Foo()
		{
			var i32 mut x= 0;
			var B b( x );
			var A& a= b;
			var i32 &mut x_ref= b.GetX();
			a.Some(); // Call a virtual method, which has access to internal mutable reference "x_", when a derived reference to it exists.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )


def ChangingReferenceTagCountInInheritance_Test0():
	c_program_text= """
		class A interface
		{}
		class B : A // Parent has no references, child has 1 reference inside.
		{
			i32& x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ChangingReferenceTagCountInInheritance", 4 ) )


def ChangingReferenceTagCountInInheritance_Test1():
	c_program_text= """
		class A polymorph
		{}
		class B : A // Parent has no references, child has 1 reference inside.
		{
			i32& x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ChangingReferenceTagCountInInheritance", 4 ) )


def ChangingReferenceTagCountInInheritance_Test2():
	c_program_text= """
		class A polymorph
		{
			i32 & @("a"c8) x;
		}
		class B : A // Parent has 1 reference, child has 2 references inside.
		{
			i32 & @("b"c8) y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ChangingReferenceTagCountInInheritance", 6 ) )


def ChangingReferenceTagCountInInheritance_Test3():
	c_program_text= """
		class A interface {}
		class B polymorph
		{
			i32 & @("a"c8) x;
		}
		class C : A, B // Parent "A" has no references, child has 1 reference inside.
		{
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ChangingReferenceTagCountInInheritance", 7 ) )
