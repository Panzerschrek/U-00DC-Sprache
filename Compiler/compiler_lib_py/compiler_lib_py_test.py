import sys
sys.path.append('../../Compiler-build')
import sprache_compiler_lib

def main():
	sprache_compiler_lib.test_echo( "Hello, world" )

if __name__ == "__main__":
	sys.exit(main())
