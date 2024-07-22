from py_tests_common import *


def TemplateTemplateParamDeclaration_Test0():
	c_program_text= """
		template</ template T />
		struct S
		{
			T</i32/> x;
		}
	"""
	tests_lib.build_program( c_program_text )


	def TemplateTemplateParamDeclaration_Test1():
		c_program_text= """
			template</ template T />
			fn Foo( T</i32/> arg ){}
		"""
		tests_lib.build_program( c_program_text )


def TemplateTypeTemplateArg_Test0():
	c_program_text= """
		template</template Container/>
		struct IntBox
		{
			Container</i32/> container;
		}

		template</type T/>
		struct Pair
		{
			T first;
			T second;
		}

		type IntBoxPair = IntBox</Pair/>;

		var IntBoxPair constexpr pair{ .container{ .first = 67, .second = -5 } };
	"""
	tests_lib.build_program( c_program_text )


def TemplateTypeTemplateArg_Test1():
	c_program_text= """
		template</type ContainedT, template Container/>
		struct ContainerBox
		{
			Container</ContainedT/> container;
		}

		template</type T/>
		struct Pair
		{
			T first;
			T second;
		}

		type FloatBoxPair = ContainerBox</f32, Pair/>;

		var FloatBoxPair constexpr pair{ .container{ .first = 78.0f, .second = -51.6f } };
	"""
	tests_lib.build_program( c_program_text )


def TemplateTypeTemplateArg_Test2():
	c_program_text= """
		template</type T/> class Vec{}

		template</ template Container, type ContainedT/>
		fn MakeContainer() : Container</ContainedT/>
		{
			return Container</ContainedT/>();
		}

		fn Foo()
		{
			var Vec</bool/> v = MakeContainer</Vec, bool/>();
		}
	"""
	tests_lib.build_program( c_program_text )
