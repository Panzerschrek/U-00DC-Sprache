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
	"""
	tests_lib.build_program( c_program_text )
