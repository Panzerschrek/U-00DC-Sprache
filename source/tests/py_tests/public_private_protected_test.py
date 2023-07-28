from py_tests_common import *


def VisibilityLabelDeclaration_Test0():
	c_program_text= """
		class A
		{
		public:
			fn Foo(this);
		private:
			i32 x;
		}
	"""
	tests_lib.build_program( c_program_text )


def VisibilityLabelDeclaration_Test1():
	c_program_text= """
		class A polymorph
		{
		protected:
			i32 x;
		}
	"""
	tests_lib.build_program( c_program_text )


def DefaultClassVisibilityIsPublic_Test0():
	c_program_text= """
		class A
		{
			i32 x;
		}
		fn Foo( A& a ) : i32 { return a.x; } // Ok, 'A::x' is public by-default.
	"""
	tests_lib.build_program( c_program_text )


def VisibilityForEnumMember_Test0():
	c_program_text= """
		class A
		{
		private:
			enum E{ EE }
		}

		fn Foo()
		{
			var A::E e= zero_init;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 10 )


def VisibilityForTypeTempateMember_Test0():
	c_program_text= """
		class A
		{
		private:
			template</ type T />
			type Vec3= [ T, 3 ];
		}

		fn Foo()
		{
			var A::Vec3</ i32 /> e= zero_init;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 11 )


def AccessingPrivateMemberOutsideClass_Test0():
	c_program_text= """
		class A
		{
		private:
			type II= i32;
		}

		fn Foo()
		{
			var A::II i= 0; // Error, accessing private member outside class
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 10 )


def AccessingPrivateMemberOutsideClass_Test1():
	c_program_text= """
		class A
		{
		private:
			type II= i32;
		}

		fn Foo() : A::II {} // Error, accessing private member outside class.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 8 )


def AccessingPrivateMemberOutsideClass_Test2():
	c_program_text= """
		class A
		{
		private:
			type II= i32;
		}

		fn Foo( A::II i ) {} // Error, accessing private member outside class.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 8 )


def AccessingPrivateMemberOutsideClass_Test3():
	c_program_text= """
		class A polymorph
		{
		private:
			type II= i32;
		}

		class B : A
		{
			A::II i;  // error, A::I is private
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 10 )


def AccessingPrivateMemberOutsideClass_Test4():
	c_program_text= """
		class A
		{
		private:
			template</ type T /> type Vec3= [ T, 3 ];
		}

		template</type T/>
		struct Box</ A::Vec3</T/> />{}  // Accessing private member in template signature parameter.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 9 )


def AccessingPrivateMemberOutsideClass_Test5():
	c_program_text= """
		class A
		{
		public:
			fn constructor( mut this )= default;
		private:
			op()( this ){}
		}

		fn Foo()
		{
			var A a;
			a(); // Acessing private postfix operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 13 )


def AccessingPrivateMemberOutsideClass_Test6():
	c_program_text= """
		class A
		{
		public:
			fn constructor( mut this )= default;
		private:
			op++( mut this ){}
		}

		fn Foo()
		{
			var A mut a;
			++a; // Acessing private prefix operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 13 )


def AccessingPrivateMemberOutsideClass_Test7():
	c_program_text= """
		class A
		{
		public:
			fn constructor( mut this )= default;
		private:
			op+=( mut this, i32 x ){}
		}

		fn Foo()
		{
			var A mut a;
			a+= 66; // Acessing private binary operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 13 )


def AccessingPrivateMemberOutsideClass_Test8():
	c_program_text= """
		class A
		{
		public:
			fn constructor( mut this )= default;
		private:
			op[]( this, i32 x ) : i32 { return x; }
		}

		fn Foo()
		{
			var A a;
			a[17]; // Accessing private postfix operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 13 )


def AccessingPrivateMemberOutsideClass_Test9():
	c_program_text= """
		class A
		{
		public:
			fn constructor( mut this )= default;
		private:
			op~( this ) : i32 { return 0; }
		}

		fn Foo()
		{
			var A a;
			~a; // Accessing private unary prefix operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 13 )


def AccessingPrivateMemberInsideClass_Test0():
	c_program_text= """
		class A
		{
		private:
			type II= i32;
			fn Inner()
			{
				var II i= 0; // ok, accessing inside member function
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingPrivateMemberInsideClass_Test1():
	c_program_text= """
		class A
		{
		private:
			type II= i32;
			fn Inner();
		}

		fn A::Inner()
		{
			var II i= 0; // ok, accessing inside member function, with body outside class.
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingPrivateMemberInsideClass_Test2():
	c_program_text= """
		class A
		{
		private:
			type II= i32;
			II i; // Ok, accessing private type name for field type.
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingPrivateMemberInsideClass_Test3():
	c_program_text= """
		class A
		{
		private:
			type II= i32;

			struct Inner
			{
				II i; // Ok, accessing private member of outer class.
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingPrivateMemberInsideClass_Test5():
	c_program_text= """
		class A
		{
		private:
			class Inner
			{
			private:
				fn Foo();
			}
		}

		fn A::Inner::Foo(){}  // Ok, accessing private function, but for declaration
	"""
	tests_lib.build_program( c_program_text )


def AccessingPrivateMemberInsideClass_Test6():
	c_program_text= """
		class A
		{
		private:
			type II= i32;
		public:
			struct Inner
			{
				II i; // Ok, accessing private member of outer class form public class of outer class.
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingProtectedMember_Test0():
	c_program_text= """
		class A
		{
		protected:
			i32 x;
			fn Zero( mut this ) { x= 0; }  // Access protected member inside class
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingProtectedMember_Test1():
	c_program_text= """
		class A polymorph
		{
		protected:
			i32 x;
		}

		class B : A
		{
			fn Zero( mut this ) { A::x= 0; }  // Access protected member of parent class, unsing NamedOperand
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingProtectedMember_Test2():
	c_program_text= """
		class A polymorph
		{
		protected:
			i32 x;
		}

		fn Foo( A& a )
		{
			auto& x= a.x;  // Error, 'A::x' is protected
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 10 )


def AccessingProtectedMember_Test3():
	c_program_text= """
		class A polymorph
		{
		protected:
			i32 x;
		}
		class B : A {} // 'B' contains 'x' as protected.
		class C : B // 'C' contains 'x' as protected too.
		{
			fn Zero( mut this ) { A::x= 0; }  // Access protected member of parent class, unsing NamedOperand
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingProtectedMember_Test4():
	c_program_text= """
		class A polymorph
		{
		protected:
			i32 x;
		}
		class B : A
		{
			// "B" inherits "x" as protected
		}
		fn Foo(B& b)
		{
			auto& x= b.x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 13 )


def AccessingProtectedMember_Test5():
	c_program_text= """
		class A polymorph
		{
		protected:
			i32 x;
		}
		class B : A
		{
			// "B" inherits "x" as protected
		}
		class C : B
		{
			fn Zero( mut this ) { A::x= 0; }  // Ok, "x" is still protected, not private.
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingPrivateMemberOutsideClass_ViaMemberAccessOperator_Test0():
	c_program_text= """
		class A
		{
		private:
			i32 x;
		}

		fn Foo( A& a )
		{
			auto x= a.x;  // Error, A::x is private
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 10 )


def AccessingPrivateMemberOutsideClass_ViaMemberAccessOperator_Test1():
	c_program_text= """
		class A
		{
		private:
			fn GetXInternal( this ) : i32 { return x; }
			i32 x;
		}

		fn Foo( A& a )
		{
			auto x= a.GetXInternal();  // Error, A::GetXInternal is private
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 11 )


def FunctionsVisibilityMismatch_Test0():
	c_program_text= """
		class A
		{
		public:
			fn Foo( this );
		private:
			fn Foo( mut this ); // Error, functions with same name have different visibility
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionsVisibilityMismatch" )
	assert( errors_list[0].src_loc.line == 7 )


def FunctionsVisibilityMismatch_Test1():
	c_program_text= """
		class A
		{
		public:
			fn Foo( this );
		private:
			template</ type T />
			fn Foo( mut this ) : T { return T(); }  // Error, functions with same name have different visibility
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionsVisibilityMismatch" )
	assert( errors_list[0].src_loc.line == 7 )


def FunctionsVisibilityMismatch_Test2():
	c_program_text= """
		class A
		{
		public:
			template</ type T />
			fn Foo( this ) : T { return T(); }
		private:
			template</ type T />
			fn Foo( mut this ) : T { return T(); }  // Error, functions with same name have different visibility
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionsVisibilityMismatch" )
	assert( errors_list[0].src_loc.line == 8 )


def FunctionsVisibilityMismatch_Test3():
	c_program_text= """
		class A
		{
		public:
			fn Foo( i32 i );
		protected:
			fn Foo( f32 f ); // Error, functions with same name have different visibility
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionsVisibilityMismatch" )
	assert( errors_list[0].src_loc.line == 7 )


def FunctionsVisibilityMismatch_Test4():
	c_program_text= """
		class A polymorph
		{
		public:
			fn virtual Foo( this );
		}
		class B : A
		{
		protected:
			fn virtual override Foo( this ); // Error, virtual functions must have same visibility
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionsVisibilityMismatch" )
	assert( errors_list[0].src_loc.line == 10 )


def FunctionsVisibilityMismatch_Test5():
	c_program_text= """
		class A polymorph
		{
		protected:
			fn virtual Foo( this );
		}
		class B : A
		{
		public: // "Public Morozov"
			fn virtual override Foo( this ); // Error, virtual functions must have same visibility
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionsVisibilityMismatch" )
	assert( errors_list[0].src_loc.line == 10 )


def FunctionsVisibilityMismatch_Test6():
	c_program_text= """
		class A polymorph
		{
		public:
			fn Foo( f32 x );
		}
		class B : A
		{
		protected: // "Public Morozov"
			fn Foo( i32 x ); // Error, different visibility for non-virtual functions - own and inhereted.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionsVisibilityMismatch" )
	assert( errors_list[0].src_loc.line == 10 )


def TypeTemplatesVisibilityMismatch_Test0():
	c_program_text= """
		class A
		{
		public:
			template</ type T /> struct S{}
		private:
			template</ /> struct S</ i32 />{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypeTemplatesVisibilityMismatch" )
	assert( errors_list[0].src_loc.line == 7 )


def TypeTemplatesVisibilityMismatch_Test1():
	c_program_text= """
		class A polymorph
		{
		public:
			template</ type T /> struct S{}
		}
		class B : A
		{
		protected:
			template</ type T /> struct S</ $(T) />{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypeTemplatesVisibilityMismatch" )
	assert( errors_list[0].src_loc.line == 10 )


def FunctionBodyVisibilityIsUnsignificant_Test1():
	c_program_text= """
		class A
		{
		private:
			fn Foo();
		}

		fn A::Foo(){}  // Ok, private function body outside class.
	"""
	tests_lib.build_program( c_program_text )


def PrivateMembersNotInherited_Test0():
	c_program_text= """
		class A polymorph
		{
		private:
			i32 x;
		}
		class B : A
		{
			fn Foo( this )
			{
				var i32 x_copy= x; // Error, 'x' not visible here
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "NameNotFound", 11 ) )


def PrivateMembersNotInherited_Test1():
	c_program_text= """
		class A polymorph
		{
		private:
			type II= i32;
		}
		class B : A
		{
			fn Foo( this )
			{
				var II i= zero_init;  // Error, 'II' not visible here
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].src_loc.line == 11 )


def PrivateMembersNotInherited_Test2():
	c_program_text= """
		class A polymorph
		{
		private:
			i32 x;
		}
		class B : A
		{
			fn Foo( this )
			{
				var i32 x_copy= this.x; // Error, 'x' not visible here
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "NameNotFound", 11 ) )


def PrivateMembersNotInherited_Test3():
	c_program_text= """
		class A polymorph
		{
		private:
			i32 x;
		}
		class B : A {} // 'x' not inherited
		class C : B
		{
			fn Foo( this )
			{
				var i32 x_copy= B::x; // Error, 'x' not visible here
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "NameNotFound", 12 ) )


def PrivateMembersNotInherited_Test4():
	c_program_text= """
		class A polymorph
		{
		private:
			template</type T/> struct S{ auto x= 44; }
		}
		class B : A
		{
			template<//> struct S</ i32 /> { auto x= 99; }

			fn Foo()
			{
				static_assert( S</ i32 />::x == 99 );
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def ChildClassNameOverridesParentClassNameAndVisibility_Test0():
	c_program_text= """
		class A polymorph
		{
		protected:
			type TT= i32;
		}
		class B : A
		{
		public:
			type TT= f32;
		}

		fn Foo() : B::TT  // B::TT must be visible here
		{
			return 2.25f;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 2.25 )


def ChildClassNameOverridesParentClassNameAndVisibility_Test1():
	c_program_text= """
		class A polymorph
		{
		protected:
			type TT= i32;
		}
		class B : A
		{
		public:
			fn TT() : f64 { return 0.125; }   // reject 'TT' as type, now 'TT' is functions set.
		}

		fn Foo() : f64
		{
			return B::TT();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0.125 )


def ChildClassNameOverridesParentClassNameAndVisibility_Test2():
	c_program_text= """
		class A polymorph
		{
		protected:
			auto constexpr X= 5;
		}
		class B : A
		{
		public:
			auto constexpr X= A::X * 2; // override variable
		}

		fn Foo() : i32
		{
			return B::X;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 10 )


def VisibilityForStruct_Test0():
	c_program_text= """
		struct A
		{
		public:
			i32 x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "VisibilityForStruct" )
	assert( errors_list[0].src_loc.line == 4 )


def ThisMethodMustBePublic_ForConstructors_Test0():
	c_program_text= """
		class A
		{
		private:
			fn constructor(){} // private default constructor.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForConstructors_Test1():
	c_program_text= """
		class A
		{
		private:
			fn constructor(mut this, A& other){} // private copy constructor.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForConstructors_Test2():
	c_program_text= """
		class A
		{
		private:
			fn constructor(i32 x){} // private arbitrary constructor.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForConstructors_Test3():
	c_program_text= """
		class A
		{
		protected:
			fn constructor(i32 x, f32 y){} // protected arbitrary constructor.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForConstructors_Test4():
	c_program_text= """
		class A
		{
		private:
			fn conversion_constructor(i32 x){} // private conversion constructor.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForConstructors_Test5():
	c_program_text= """
		class A
		{
		private:
			fn constructor() = default; // private generated default constructor.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForConstructors_Test6():
	c_program_text= """
		class A
		{
		protected:
			fn constructor(mut this, A& other) = default; // private generated copy constructor.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForConstructors_Test7():
	c_program_text= """
		class A
		{
		protected:
			fn constructor(mut this, A& other) = delete; // private deleted copy constructor.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForDestructors_Test0():
	c_program_text= """
		class A
		{
		private:
			fn destructor(); // private destructor
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForDestructors_Test1():
	c_program_text= """
		class A
		{
		protected:
			fn destructor() {} // pritected destructor
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForAssignmentOperators_Test0():
	c_program_text= """
		class A
		{
		private:
			op=(mut this, A& other); // private copy-assignment operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForAssignmentOperators_Test1():
	c_program_text= """
		class A
		{
		private:
			op=(mut this, A& other) = default; // private generated copy-assignment operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForAssignmentOperators_Test2():
	c_program_text= """
		class A
		{
		protected:
			op=(mut this, A& other) = delete; // protected deleted copy-assignment operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForAssignmentOperators_Test3():
	c_program_text= """
		class A
		{
		private:
			op=(mut this, i32 x){} // private arbitrary assignment operator
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForEqualityCompareOperators_Test0():
	c_program_text= """
		class A
		{
		private:
			op==(A& l, A& r) : bool; // private equality compare operator
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForEqualityCompareOperators_Test1():
	c_program_text= """
		class A
		{
		protected:
			op==(A& l, A& r) : bool = default; // protected generated equality compare operator
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForEqualityCompareOperators_Test2():
	c_program_text= """
		class A
		{
		private:
			op==(A& l, A& r) : bool = delete; // private deleted equality compare operator
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForEqualityCompareOperators_Test3():
	c_program_text= """
		class A
		{
		private:
			op==(i32 x, A& r) : bool; // private other equality compare operator
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForOrderCompareOperators_Test0():
	c_program_text= """
		class A
		{
		protected:
			op<=>(A& l, A& r) : i32; // protected order compare operator
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForOrderCompareOperators_Test1():
	c_program_text= """
		class A
		{
		private:
			op<=>(A& l, f32 x) : i32; // private order compare operator
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ThisMethodMustBePublic", 5 ) )


def ThisMethodMustBePublic_ForOrderCompareOperators_Test2():
	c_program_text= """
		template</type T/>
		class A
		{
		private:
			op<=>(A& l, A& r) : i32; // private order compare operator
		}
		type AI= A</i32/>;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "TemplateContext", 8 ) )


def ThisMethodMustBePublic_ForOtherMethods_Test0():
	c_program_text= """
		class A
		{
		private:
			// Ok - all these methods may be non-public.
			op++(mut this);
			op()(this);
			op-=(mut this, A& other);
			op*(A x, A y) : A;
			op~(A x) : A;

			fn Foo();
			fn Foo(this);
		}
	"""
	tests_lib.build_program( c_program_text )
