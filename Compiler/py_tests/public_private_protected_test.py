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
	assert( errors_list[0].file_pos.line == 10 )


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
	assert( errors_list[0].file_pos.line == 8 )


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
	assert( errors_list[0].file_pos.line == 8 )


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


def AccessingPrivateMemberInsideClass_Test4():
	c_program_text= """
		class A
		{
		private:
			type II= i32;
			struct Inner;
		}

		struct A::Inner // Ok, accessing private struct, but for declaration
		{
			II i; // Ok, accessing private member of outer class.
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingPrivateMemberInsideClass_Test5():
	c_program_text= """
		class A
		{
		private:
			struct Inner
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
			struct Inner;
		}

		struct A::Inner
		{
			II i; // Ok, accessing private member of outer class form public class of uter class.
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
	assert( errors_list[0].file_pos.line == 10 )


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
	assert( errors_list[0].file_pos.line == 11 )


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
	assert( errors_list[0].file_pos.line == 7 )


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
	assert( errors_list[0].file_pos.line == 7 )


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
	assert( errors_list[0].file_pos.line == 8 )


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
	assert( errors_list[0].file_pos.line == 7 )


def FunctionsVisibilityMismatch_Test4():
	c_program_text= """
		class A polymorph
		{
		public:
			fn virtual Foo( this );
		}
		class B : A
		{
		private:
			fn virtual override Foo( this ); // Error, virtual functions must have same visibility
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "FunctionsVisibilityMismatch" )
	assert( errors_list[0].file_pos.line == 10 )


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
	assert( errors_list[0].file_pos.line == 10 )


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
	assert( errors_list[0].file_pos.line == 10 )


def FunctionBodyVisibilityIsUnsignificant_Test0():
	c_program_text= """
		class A
		{
		public:
			fn Foo();
		private:
			fn Foo(){} // Ok, body can have any visibility, we check visibility only for prototype
		}
	"""
	tests_lib.build_program( c_program_text )


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
