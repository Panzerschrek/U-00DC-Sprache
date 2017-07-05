import os
import sys

def GetCppFiles(dir):
	result = []

	for path in os.listdir(dir):
		full_name= os.path.join( dir, path )

		if os.path.isdir(full_name):
			result+= GetCppFiles( full_name )

		elif path.endswith(".cpp"):
			result.append( full_name )

	return result


def main():
	compiler_dir= "C:/Qt5.3.0/Tools/mingw482_32/bin"
	cpp_compiler= "g++.exe"
	full_cpp_compiler_path= os.path.join( compiler_dir, cpp_compiler )
	executable_name= "Interpreter.exe"

	# Add compiler dir to dynamic linker search path.
	path_env_variable= "PATH"
	os.environ[ path_env_variable ]= os.getenv( path_env_variable ) + ";" + compiler_dir

	files= GetCppFiles( "src" )
	options= "-Wall -Wextra -Wpedantic -std=c++11"

	command= full_cpp_compiler_path + " " + options + " " + " ".join(files) + " -o " + executable_name
	print( command )
	return os.system( command )


if __name__ == "__main__":
	sys.exit(main())

