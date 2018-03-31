import sys
sys.path.append('../../Compiler-build')
import sprache_compiler_tests_py_lib as tests_lib

def main():
	tests_lib.build_program( "fn Foo( i32 x, i32 y ) : f32 { return f32(x * y) + 0.5f; }" )

	call_result= tests_lib.run_function( "_Z3Fooii", 3, 7 )
	print( call_result )

	tests_lib.free_program()

if __name__ == "__main__":
	sys.exit(main())
