import sys
sys.path.append('../../Compiler-build')
import sprache_compiler_tests_py_lib

def main():
	program= sprache_compiler_tests_py_lib.build_program( "fn Foo( i32 x, i32 y ) : f32 { return f32(x * y) + 0.5f; }" )
	if program is None:
		raise "program is none"

	call_result= sprache_compiler_tests_py_lib.run_function( program, "_Z3Fooii", 1, 45 )
	print( call_result )

	sprache_compiler_tests_py_lib.free_program(program)

if __name__ == "__main__":
	sys.exit(main())
