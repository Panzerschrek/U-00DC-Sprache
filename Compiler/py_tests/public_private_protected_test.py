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

		struct A::Inner
		{
			II i; // Ok, accessing private member of outer class.
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
