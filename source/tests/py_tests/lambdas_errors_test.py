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
