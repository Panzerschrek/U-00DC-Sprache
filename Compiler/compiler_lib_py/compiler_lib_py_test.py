import sys
sys.path.append('../../Compiler-build')
import sprache_compiler_lib

def main():
	print( "test0" )
	sprache_compiler_lib.build_program( "fn foo() {}" )
	print( "test1" )
	sprache_compiler_lib.build_program( "fn foo() : i32 {}" )

if __name__ == "__main__":
	sys.exit(main())
