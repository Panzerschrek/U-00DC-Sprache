from py_tests_common import *


def UnsafeInitializerDeclaration_Test0():
	c_program_text= """
		struct T{ fn constructor( i32 x, i32 y ) {} }
		struct V{ f32 x; f32 y; }
		struct S{ i32 x; f32 y; [char8, 4] z; T t; V v; }
		fn Foo()
		{
			var S s
			{
				.x unsafe( 4 ),
				.y= unsafe( 5.0f ),
				.z[ safe("a"c8), unsafe("b"c8), safe(unsafe("c"c8)), "d"c8 ],
				.t unsafe( (1, 2) ),
				.v unsafe( { .x= 0.0f, .y=1.0f } )
			};
		}
	"""
	tests_lib.build_program( c_program_text )

