import argparse
import os
import subprocess
import sys


class CodeBuilderError:
	def __init__(self):
		self.file_name= ""
		self.error_code= ""
		self.line_number= 0


class ParseResult:
	def __init__(self):
		self.test_kind= "success" # may be "fail", and "success"
		self.errors_list= []
		self.parse_ok= False


g_compiler_executable= "Compiler"
g_use_position_independent_code= False
g_additional_libraries_to_link= []


def LoadFile( file_name ):
	with open( file_name, "r", encoding="utf-8" ) as file:
		file.seek( 0, os.SEEK_END )
		file_sise= file.tell()
		file.seek( 0, os.SEEK_SET )
		return file.read( file_sise )


def IsAlphaCharacher( character ):
	return ( ord(character) >= ord('a') and ord(character) <= ord('z') ) or ( ord(character) >= ord('A') and ord(character) <= ord('Z') )


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

	i= 4
	file_length= len(file_content)

	line_number= 1 # Line numbers in compiler starts with 1

	while i < len(file_content):

		if file_content.startswith( "//##", i ):

			error_annotaton= "//##expect_error "
			if not file_content.startswith( error_annotaton, i ):
				print( "Unexpected annotation. Expected " + error_annotaton + " [error_code]" )
				return result
			i+= len(error_annotaton)

			expected_error= CodeBuilderError()
			expected_error.line_number= line_number

			while IsAlphaCharacher(file_content[i]) and i < len(file_content):
				expected_error.error_code= expected_error.error_code + file_content[i]
				i= i + 1

			result.errors_list.append( expected_error )

		elif file_content[i] == '\n':

			line_number= line_number + 1
			i= i + 1

		else:
			i= i + 1

	if len(result.errors_list) != 0 and result.test_kind == "success":
		print( "Success test must have no error annotations." )
		return result
	if len(result.errors_list) == 0 and result.test_kind == "fail":
		print( "Fail test must have at least one error annotation." )
		return result

	result.parse_ok= True
	return result


def ParseCompilerErrorsOutput( output_str ):
	result= []

	i= 0
	output_str_len= len(output_str)
	while i < output_str_len:
		error= CodeBuilderError()

		while i < output_str_len and output_str[i] != ' ':
			error.file_name= error.file_name + output_str[i]
			i= i + 1
		i= i + 1

		line_num_str= ""
		while i < output_str_len and output_str[i] != ' ':
			line_num_str= line_num_str + output_str[i]
			i= i + 1
		i= i + 1
		error.line_number= int(line_num_str)

		while i < output_str_len and IsAlphaCharacher( output_str[i] ):
			error.error_code= error.error_code + output_str[i]
			i= i + 1

		result.append(error)

		while i < output_str_len and output_str[i] != '\n':
			i= i + 1
		i= i + 1

	return result


def DoFailTest( file_path, expected_errors_list ):
	output_str= ""
	try:
		output_str= str( subprocess.check_output( [ g_compiler_executable, file_path, "--tests-output", "-filetype", "null", "--allow-unused-names" ], universal_newlines= True ) )
	except subprocess.CalledProcessError as called_process_error:
		output_str= str(called_process_error.output)

	actual_errors_list= ParseCompilerErrorsOutput( output_str )
	all_ok= True

	file_path_normalized= os.path.abspath( file_path )
	for error in actual_errors_list:
		if os.path.abspath( error.file_name ) != file_path_normalized:
			all_ok= False
			print( "Unexpected error in included file:\n" + error.file_name + " " + str(error.line_number) + " " + error.error_code )

	for expected_error in expected_errors_list:
		found= False
		for actual_error in actual_errors_list:
			if expected_error.line_number == actual_error.line_number and expected_error.error_code == actual_error.error_code:
				found= True
				break
		if not found:
			all_ok= False
			print( "Expected error " + expected_error.error_code + " at line " + str( expected_error.line_number ) + " not found in actual compiler output" )

	if not all_ok:
		print( "Actual errors:\n" + output_str )

	if all_ok:
		return 0
	return 1


def DoSuccessTest( file_path ):
	file_name= os.path.basename( file_path )
	executable_file= file_name + "_temp.exe"

	compiler_args= [ g_compiler_executable, file_path, "-o", executable_file, "--filetype", "exe", "--allow-unused-names" ]
	if g_use_position_independent_code :
		compiler_args= compiler_args + [ "--relocation-model", "pic" ]

	for library in g_additional_libraries_to_link:
		compiler_args.append( "-Wl," + library )

	if subprocess.call( compiler_args ) != 0:
		print( "Compilation failed" )
		return 1

	# Run result executable with current directory equal to directory of the source file.
	# Do this in order to have access to files in this directory from a test.
	file_dir= os.path.dirname( file_path )

	return_code= subprocess.call( [ os.path.abspath( executable_file )], cwd= file_dir )
	if return_code != 0:
		print( "running failed with code " + str(return_code) )
		os.remove( executable_file )
		return 1

	os.remove( executable_file )
	return 0


def RunTestForFile( file_path ):
	print( "Running test for file \"" + file_path + "\"" )
	file_content= LoadFile( file_path )
	parse_result= ParseFile( file_content )

	if not parse_result.parse_ok:
		return 1

	result= 0
	if parse_result.test_kind == "fail":
		result= DoFailTest( file_path, parse_result.errors_list )
	elif parse_result.test_kind == "success":
		result= DoSuccessTest( file_path )

	if( result == 0 ):
		print( "Test for file \"" + file_path + "\" successed" )
	else:
		print( "Test for file \"" + file_path + "\" failed" )

	return result


def RunTestsInDirectory( dir_path ):
	print( "Running test for directory \"" + dir_path + "\"" )

	result= 0
	for path in os.listdir( dir_path ):

		full_name= os.path.join( dir_path, path )

		if os.path.isdir(full_name):
			result+= RunTestsInDirectory( full_name )
		elif os.path.isfile( full_name ) and full_name.endswith( ".u" ):
			result+= RunTestForFile( full_name )

	return result


def main():
	parser= argparse.ArgumentParser( description= 'Run annotated Ü tests.' )
	parser.add_argument( "--input-file", help= "input Ü test source", type=str )
	parser.add_argument( "--input-dir", help= "input Ü test sources directory", type=str )
	parser.add_argument( "--compiler-executable", help= "path to compiler executable", type=str )
	parser.add_argument( "--use-position-independent-code", help= "use or not position independent code", action="store_true" )
	parser.add_argument( "--add-library", help= "specify an additional library for linking", action= "append" )

	args= parser.parse_args()

	if args.compiler_executable is not None:
		global g_compiler_executable
		g_compiler_executable= args.compiler_executable

	if args.use_position_independent_code is not None:
		global g_use_position_independent_code
		g_use_position_independent_code= args.use_position_independent_code

	if args.add_library is not None:
		global g_additional_libraries_to_link
		for library in args.add_library:
			g_additional_libraries_to_link.append( library )

	if args.input_file is not None:
		return RunTestForFile( args.input_file )
	elif args.input_dir is not None:
		return RunTestsInDirectory( args.input_dir )
	else:
		print( "expected single input file or directory with files" )
		return 1


if __name__ == "__main__":
	sys.exit(main())
