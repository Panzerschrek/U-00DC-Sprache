import sys
sys.path.append('../../Compiler-build')
import sprache_compiler_tests_py_lib

def main():
	program= sprache_compiler_tests_py_lib.build_program( "fn foo() : i32 { return 42; }" )
	if program is None:
		raise "program is none"

	sprache_compiler_tests_py_lib.free_program(program)

if __name__ == "__main__":
	sys.exit(main())
