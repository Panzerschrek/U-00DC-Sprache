from py_tests_common import *


def UsingIncompleteType_ForInheritance_Test0():
	c_program_text= """
		class A;
		class B : A {}  // Error - "A" is incomplete.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UsingIncompleteType" )
	assert( errors_list[0].src_loc.line == 3 )


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
	assert( HaveError( errors_list, "BaseUnavailable", 10 ) )


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
	assert( HaveError( errors_list, "FieldIsNotInitializedYet", 10 ) )


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
	assert( HaveError( errors_list, "FieldIsNotInitializedYet", 10 ) )


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
	assert( HaveError( errors_list, "FieldIsNotInitializedYet", 10 ) )


def CanNotDeriveFromThisType_Test0():
	c_program_text= """
		class A{}   // non-polymorph by-default
		class B : A{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CanNotDeriveFromThisType" )
	assert( errors_list[0].src_loc.line == 3 )


def CanNotDeriveFromThisType_Test1():
	c_program_text= """
		class A polymorph {}
		class B final : A{}
		class C : B {}    // Inherit from final class
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "CanNotDeriveFromThisType" )
	assert( errors_list[0].src_loc.line == 4 )


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
	assert( errors_list[0].error_code == "DuplicatedBaseClass" )
	assert( errors_list[0].src_loc.line == 4 )


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
	assert( HaveError( errors_list, "ConstructingAbstractClassOrInterface", 6 ) )


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
	assert( HaveError( errors_list, "ConstructingAbstractClassOrInterface", 6 ) )


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
	assert( errors_list[0].error_code == "TypesMismatch" )
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

	fn Bar(bool b)
	{
		var B mut b;
		take(cast_ref</A/>(b)); // Error, calling default constructor of abstract class "A" in "take" operator.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConstructingAbstractClassOrInterface" )
	assert( errors_list[0].src_loc.line == 16 )


def MoveAssignForNonFinalPolymorphClass_Test0():
	c_program_text= """
	class A polymorph {}
	class B final : A {}
	fn Bar(bool b)
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
	fn Bar(bool b)
	{
		var B mut b;
		b= B(); // Ok - move-assign for final polymorph class.
	}
	"""
	tests_lib.build_program( c_program_text )


def MoveAssignForNonFinalPolymorphClass_Test2():
	c_program_text= """
	class A polymorph {}
	fn Bar(bool b)
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
	fn Bar(bool b)
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
	fn Bar(bool b)
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
	fn Bar(bool b)
	{
		var B mut b;
		take(b); // Ok - taking final polymorph class.
	}
	"""
	tests_lib.build_program( c_program_text )


def FunctionOverridingWithReferencesNotationChange_Test0():
	c_program_text= """
	class A interface
	{
		fn virtual pure Foo(this'a') : i32 &'a ;
	}
	class B : A
	{
		// Error, returned reference changed from empty list to internal reference of "this", because reference field was added.
		fn virtual override Foo(this'a') : i32 &'a;
		i32& x;
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionOverridingWithReferencesNotationChange" )
	assert( errors_list[0].src_loc.line == 9 )


def FunctionOverridingWithReferencesNotationChange_Test1():
	c_program_text= """
	class A polymorph
	{
		fn virtual Foo(this'a') : i32 &'a
		{
			return g_zero;
		}
	}
	auto g_zero= 0;
	class B : A
	{
		// Error, returned reference changed from empty list to internal reference of "this", because reference field was added.
		fn virtual override Foo(this'a') : i32 &'a;
		i32& x;
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionOverridingWithReferencesNotationChange" )
	assert( errors_list[0].src_loc.line == 13 )
