import argparse
import os
import subprocess
import sys


class FilePos:
	file_index= 0
	line= 0
	pos_in_line= 0


class CodeBuilderError:
	error_code= ""
	text= ""
	file_pos= FilePos()


class ParseResult:
	test_kind= "success" # may be "fail", and "success"
	errors_list= []
	parse_ok= False


g_compiler_executable= "Compiler"
g_entry_point_executable= "../Compiler/data/entry.cpp"
g_cpp_compiler_executable= "g++"


def LoadFile( file_name ):
	with open( file_name, "r", encoding="utf-8" ) as file:
		file.seek( 0, os.SEEK_END )
		file_sise= file.tell()
		file.seek( 0, os.SEEK_SET )
		return file.read( file_sise )


def ParseFile( file_content ):
	result= ParseResult()

	expected_start= "//##"
	success_test_start= expected_start + "success_test"
	fail_test_start= expected_start + "fail_test"
	if not file_content.startswith( expected_start ):
		print( "Unexpected test file. Expected file, with " + expected_start + " at start" )
		return result

	if file_content.startswith( success_test_start ):
		result.test_kind= "success"
	elif file_content.startswith( fail_test_start ):
		result.test_kind= "fail"
	else:
		print( "Unexpected test file. Expected file, with " + success_test_start + " or " + fail_test_start + " at start" )
		return result

	i= 0
	while i < len(file_content):
		i= i + 1

	result.parse_ok= True
	return result


def DoFailTest( file_path ):
	return


def DoSuccessTest( file_path ):
	object_file= "temp.o"
	executable_file= "temp.exe"

	if subprocess.call( [ g_compiler_executable, file_path, "--produce-object-file", "-o", object_file ] ) != 0:
		print( "Compilation failed" )
		return 1

	if subprocess.call( [ g_cpp_compiler_executable, g_entry_point_executable, object_file, "-o", executable_file ] ) != 0:
		print( "linking failed" )
		os.remove( object_file )
		return 1

	if subprocess.call( [ executable_file ] ) != 0:
		print( "running failed" )
		os.remove( object_file )
		os.remove( executable_file )
		return 1

	os.remove( object_file )
	os.remove( executable_file )
	return 0


def main():
	parser= argparse.ArgumentParser( description= 'Run annotated Ü tests.' )
	parser.add_argument( "-i", help= "input Ü test source", type=str, required= True )

	args= parser.parse_args()
	file_path= args.i

	file_content= LoadFile( file_path )
	parse_result= ParseFile( file_content )

	if not parse_result.parse_ok:
		return 1

	if parse_result.test_kind == "fail":
		return DoFailTest()
	elif parse_result.test_kind == "success":
		return DoSuccessTest( file_path )

if __name__ == "__main__":
	sys.exit(main())
