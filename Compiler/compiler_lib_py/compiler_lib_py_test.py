import sys
sys.path.append('C:/lnk/U+00DC-Sprache/Compiler-build')
import sprache_compiler_lib
#from 'C:/lnk/U+00DC-Sprache/Compiler-build/sprache_compiler_lib import 'sprache_compiler_lib'

def main():
	sprache_compiler_lib.test_echo( "Hello, world" )

if __name__ == "__main__":
	sys.exit(main())
