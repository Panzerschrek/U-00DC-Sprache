import argparse
import os
import subprocess
import sys
import traceback


g_tests_path = ""
g_tests_build_root_path = ""
g_build_system_executable = ""
g_compiler_executable=  ""
g_build_system_imports_path = ""
g_ustlib_path = ""


def RunBuildSystem( project_subdirectory ):
	project_root = os.path.join( g_tests_path, project_subdirectory )
	build_root = os.path.join( g_tests_build_root_path, project_subdirectory );
	build_system_args= [ g_build_system_executable, "-q", "--compiler-executable", g_compiler_executable, "--build-system-imports-path", g_build_system_imports_path, "--ustlib-path", g_ustlib_path, "--project-directory", project_root, "--build-directory", build_root ]

	# Run the build.
	subprocess.check_call( build_system_args )


# Returns subprocess result.
def RunBuildSystemWithErrors( project_subdirectory ):
	project_root = os.path.join( g_tests_path, project_subdirectory )
	build_root = os.path.join( g_tests_build_root_path, project_subdirectory );
	build_system_args= [ g_build_system_executable, "-q", "--compiler-executable", g_compiler_executable, "--build-system-imports-path", g_build_system_imports_path, "--ustlib-path", g_ustlib_path, "--project-directory", project_root, "--build-directory", build_root ]

	# Run the build.
	return subprocess.run( build_system_args, stderr=subprocess.PIPE )


def RunExecutable( project_subdirectory, executable_name ):
	subprocess.check_call( [ os.path.join( os.path.join( g_tests_build_root_path, project_subdirectory ), executable_name ) ], stdout= subprocess.DEVNULL )

#
# Tests itself
#

def HelloWorldTest():
	RunBuildSystem( "hello_world" )
	RunExecutable( "hello_world", "hello_world" )


def TwoFilesExeTest():
	RunBuildSystem( "two_files_exe" )
	RunExecutable( "two_files_exe", "two_files_exe" )


def TwoTargetsTest():
	RunBuildSystem( "two_targets" )
	RunExecutable( "two_targets", "target_a" )
	RunExecutable( "two_targets", "target_b" )


def ТестЮникода():
	# Non-ASCII project directory name, non-ASCII target name with spaces.
	RunBuildSystem( "Тест Юникода" )
	RunExecutable( "Тест Юникода", "Юникодный исполняемый файл - ☦ (православный)" )


def MissingBuildFile():
	# A directory with no build file.
	res = RunBuildSystemWithErrors( "missing_build_file" )
	stderr = str(res.stderr)
	assert( stderr.find( "Compiler execution failed" ) != -1 )
	assert( stderr.find( "Failed to load/build the build script shared library" ) != -1 )


#
# End tests list
#


def main():
	parser= argparse.ArgumentParser( description= "Run Bürokratie tests." )
	parser.add_argument( "--tests-path", help= "path to tests", type=str, required= True )
	parser.add_argument( "--tests-build-root-path", help= "path to tests build root", type=str, required= True )
	parser.add_argument( "--build-system-executable", help= "path to build system executable", type=str, required= True )
	parser.add_argument( "--compiler-executable", help= "path to compiler executable", type=str, required= True )
	parser.add_argument( "--build-system-imports-path", help= "path to build system imports", type=str, required= True )
	parser.add_argument( "--ustlib-path", help= "path to ustlib", type=str, required= True )

	args= parser.parse_args()

	global g_tests_path
	g_tests_path= args.tests_path

	global g_tests_build_root_path
	g_tests_build_root_path = args.tests_build_root_path

	global g_build_system_executable
	g_build_system_executable= args.build_system_executable

	global g_compiler_executable
	g_compiler_executable= args.compiler_executable

	global g_build_system_imports_path
	g_build_system_imports_path= args.build_system_imports_path

	global g_ustlib_path
	g_ustlib_path= args.ustlib_path


	test_funcs = [ HelloWorldTest, TwoFilesExeTest, TwoTargetsTest, ТестЮникода, MissingBuildFile ]
	print( "Run " + str(len(test_funcs)) + " Bürokratie tests" )

	tests_passed= 0
	tests_failed= 0

	for test_func in test_funcs:
		try:
			test_func()
			tests_passed+= 1
		except Exception as ex:
			print( "test " + str(test_func) + " failed" )
			traceback.print_exc( file= sys.stdout )
			print()
			tests_failed+= 1

	print( "" )
	print( str(tests_passed) + " tests passed" )
	print( str(tests_failed) + " tests failed" )
	return tests_failed


if __name__ == "__main__":
	sys.exit(main())
