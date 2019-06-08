from py_tests_common import *


def TypeNameInErrorMessage_FundamentalTypes():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 0.0f;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	# must print something, like "conversion from f32 to i32"
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 4 )
	assert( errors_list[0].text.find( "i32" ) != -1 )
	assert( errors_list[0].text.find( "f32" ) != -1 )


def TypeNameInErrorMessage_ClassTypeInGlobalNamespace():
	c_program_text= """
		struct SomeType{}
		fn Foo()
		{
			var i32 x= SomeType();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	# must print something, like "conversion from SomeType to i32"
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 5 )
	assert( errors_list[0].text.find( "i32" ) != -1 )
	assert( errors_list[0].text.find( "SomeType" ) != -1 )


def TypeNameInErrorMessage_ClassTypeInNamespace_Test0():
	c_program_text= """
		namespace NNN{ struct SomeType{} }
		fn Foo()
		{
			var i32 x=	NNN::SomeType();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	# must print something, like "conversion from SomeType to i32"
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 5 )
	assert( errors_list[0].text.find( "i32" ) != -1 )
	assert( errors_list[0].text.find( "NNN::SomeType" ) != -1 )


def TypeNameInErrorMessage_ClassTypeInNamespace_Test1():
	c_program_text= """
		namespace NNN{ namespace Bar{ struct SomeType{} } }
		fn Foo()
		{
			var i32 x= NNN::Bar::SomeType();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	# must print something, like "conversion from NNN::SomeType to i32"
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 5 )
	assert( errors_list[0].text.find( "i32" ) != -1 )
	assert( errors_list[0].text.find( "NNN::Bar::SomeType" ) != -1 )


def TypeNameInErrorMessage_ClassTypeInNamespace_Test2():
	c_program_text= """
		namespace NNN
		{
			namespace Bar
			{
				struct SomeType{}
				fn Foo()
				{
					var i32 x= SomeType();
				}
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	# must print full type name
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 9 )
	assert( errors_list[0].text.find( "i32" ) != -1 )
	assert( errors_list[0].text.find( "NNN::Bar::SomeType" ) != -1 )


def TypeNameInErrorMessage_ClassTemplate_Test0():
	c_program_text= """
		template</ type T /> struct Box {}
		fn Foo()
		{
			var i32 x= Box</f64/>();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 5 )
	assert( errors_list[0].text.find( "i32" ) != -1 )
	assert( errors_list[0].text.find( "Box</f64/>" ) != -1 )


def TypeNameInErrorMessage_ClassTemplate_Test1():
	c_program_text= """
		namespace Bar{ template</ type T /> struct Box {} }
		fn Foo()
		{
			var i32 x= Bar::Box</bool/>();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 5 )
	assert( errors_list[0].text.find( "i32" ) != -1 )
	assert( errors_list[0].text.find( "Bar::Box</bool/>" ) != -1 )


def TypeNameInErrorMessage_ClassTemplate_Test2():
	c_program_text= """
		struct S{}
		template</ type T /> struct Box {}
		fn Foo()
		{
			var i32 x= Box</S/>();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 6 )
	assert( errors_list[0].text.find( "i32" ) != -1 )
	assert( errors_list[0].text.find( "Box</S/>" ) != -1 )


def TypeNameInErrorMessage_ClassTemplate_Test3():
	c_program_text= """
		struct S{}
		template</ type T, size_type X /> struct Box {}
		fn Foo()
		{
			var i32 x= Box</S, size_type(66) />();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 6 )
	assert( errors_list[0].text.find( "i32" ) != -1 )
	assert( errors_list[0].text.find( "Box</S, 66/>" ) != -1 )


def TypeNameInErrorMessage_ClassTemplate_Test4():
	c_program_text= """
		enum E { A, B, C, D, E, F, G, H, I, }
		template</ E a, E b, E c /> struct Box{}
		fn Foo()
		{
			var i32 x= Box</ E::B, E::G, E::A />();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 6 )
	assert( errors_list[0].text.find( "i32" ) != -1 )
	assert( errors_list[0].text.find( "Box</E::B, E::G, E::A/>" ) != -1 )


def TypeNameInErrorMessage_ClassTemplate_Test5():
	c_program_text= """
		template<//> struct Box{}
		fn Foo()
		{
			var i32 x= Box<//>();
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 5 )
	assert( errors_list[0].text.find( "i32" ) != -1 )
	assert( errors_list[0].text.find( "Box<//>" ) != -1 )
