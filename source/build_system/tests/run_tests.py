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


def RunBuildSystemWithExplicitConfiguration( project_subdirectory, configuration ):
	project_root = os.path.join( g_tests_path, project_subdirectory )
	build_root = os.path.join( g_tests_build_root_path, project_subdirectory );
	build_system_args= [ g_build_system_executable, "-q", "--build-configuration", configuration, "--compiler-executable", g_compiler_executable, "--build-system-imports-path", g_build_system_imports_path, "--ustlib-path", g_ustlib_path, "--project-directory", project_root, "--build-directory", build_root ]

	# Run the build.
	subprocess.check_call( build_system_args )


# Build for "release" configuration.
def RunBuildSystem( project_subdirectory ):
	return RunBuildSystemWithExplicitConfiguration( project_subdirectory, "release" )


# Returns subprocess result.
def RunBuildSystemWithErrors( project_subdirectory ):
	project_root = os.path.join( g_tests_path, project_subdirectory )
	build_root = os.path.join( g_tests_build_root_path, project_subdirectory );
	build_system_args= [ g_build_system_executable, "-q", "--compiler-executable", g_compiler_executable, "--build-system-imports-path", g_build_system_imports_path, "--ustlib-path", g_ustlib_path, "--project-directory", project_root, "--build-directory", build_root ]

	# Run the build.
	return subprocess.run( build_system_args, stderr=subprocess.PIPE )


def RunExecutableWithExplicitConfiguration( project_subdirectory, executable_name, configuration ):
	return subprocess.check_call( [ os.path.join( g_tests_build_root_path, project_subdirectory, configuration, executable_name ) ], stdout= subprocess.DEVNULL )


# Run for "release" confuguration.
def RunExecutable( project_subdirectory, executable_name ):
	return RunExecutableWithExplicitConfiguration( project_subdirectory, executable_name, "release" )

#
# Tests itself
#

def HelloWorldTest():
	RunBuildSystem( "hello_world" )
	RunExecutable( "hello_world", "hello_world" )


def MultipleConfigurationsTest():
	# Build both "debug" and "release".
	RunBuildSystemWithExplicitConfiguration( "multiple_configurations", "debug" )
	RunBuildSystemWithExplicitConfiguration( "multiple_configurations", "release" )
	# Should get two executables in different subdirectories.
	RunExecutableWithExplicitConfiguration( "multiple_configurations", "multiple_configurations", "debug" )
	RunExecutableWithExplicitConfiguration( "multiple_configurations", "multiple_configurations", "release" )


def DebugOnlyProjectTest():
	RunBuildSystemWithExplicitConfiguration( "debug_only_project", "debug" )
	RunExecutableWithExplicitConfiguration( "debug_only_project", "debug_only_target", "debug" )


def EmptyPackageTest():
	RunBuildSystem( "empty_package" )


def BuildFileLoggingTest():
	RunBuildSystem( "build_file_logging" )


def TwoFilesExeTest():
	RunBuildSystem( "two_files_exe" )
	RunExecutable( "two_files_exe", "two_files_exe" )


def TwoTargetsTest():
	RunBuildSystem( "two_targets" )
	RunExecutable( "two_targets", "target_a" )
	RunExecutable( "two_targets", "target_b" )


def TwoTargetsCommonSourceTest():
	RunBuildSystem( "two_targets_common_source" )
	RunExecutable( "two_targets_common_source", "target_a" )
	RunExecutable( "two_targets_common_source", "target_b" )


def SourcesInDirectoriesTest():
	RunBuildSystem( "sources_in_directories" )
	RunExecutable( "sources_in_directories", "sources_in_directories" )


def ТестЮникода():
	# Non-ASCII project directory name, non-ASCII target name with spaces.
	RunBuildSystem( "Тест Юникода" )
	RunExecutable( "Тест Юникода", "Юникодный исполняемый файл - ☦ (православный)" )


def BuildFileWithImportsTest():
	RunBuildSystem( "build_file_with_imports" )
	RunExecutable( "build_file_with_imports", "build_file_with_imports" )


def MissingBuildFile():
	# A directory with no build file.
	res = RunBuildSystemWithErrors( "missing_build_file" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Can not get modification time for" ) != -1 )
	assert( stderr.find( "file does not exists?" ) != -1 )
	assert( stderr.find( "Failed to load/build the build script shared library" ) != -1 )


def BrokenBuildFile0Test():
	res = RunBuildSystemWithErrors( "broken_build_file0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "build.u" ) != -1 )
	assert( stderr.find( "Syntax error - unexpected lexem" ) != -1 )
	assert( stderr.find( "Compiler execution failed" ) != -1 )
	assert( stderr.find( "Failed to load/build the build script shared library" ) != -1 )


def BrokenBuildFile1Test():
	res = RunBuildSystemWithErrors( "broken_build_file1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "build.u" ) != -1 )
	assert( stderr.find( "Name \"wtarget\" not found" ) != -1 )
	assert( stderr.find( "Compiler execution failed" ) != -1 )
	assert( stderr.find( "Failed to load/build the build script shared library" ) != -1 )


def BrokenBuildFile2Test():
	res = RunBuildSystemWithErrors( "broken_build_file2" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "build_script_wrapper.u" ) != -1 )
	assert( stderr.find( " Name \"GetPackageInfo\" not found" ) != -1 )
	assert( stderr.find( "Compiler execution failed" ) != -1 )
	assert( stderr.find( "Failed to load/build the build script shared library" ) != -1 )


def BrokenBuildFile3Test():
	res = RunBuildSystemWithErrors( "broken_build_file3" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Can not read file" ) != -1 )
	assert( stderr.find( "non_existing_file_imported_in_build_file.uh" ) != -1 )
	assert( stderr.find( "Compiler execution failed" ) != -1 )


def MissingSourceFileTest():
	res = RunBuildSystemWithErrors( "missing_source_file" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Can not read file" ) != -1 )
	assert( stderr.find( "this_file_does_not_exist.u" ) != -1 )
	assert( stderr.find( "Command \"this_file_does_not_exist.u.bc\" execution failed" ) != -1 )


def SourceFileCompilationError0Test():
	res = RunBuildSystemWithErrors( "source_file_compilation_error0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Syntax error" ) != -1 )
	assert( stderr.find( "Command \"main.u.bc\" execution failed" ) != -1 )


def SourceFileCompilationError1Test():
	res = RunBuildSystemWithErrors( "source_file_compilation_error1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Name \"Foo\" not found" ) != -1 )
	assert( stderr.find( "Command \"main.u.bc\" execution failed" ) != -1 )


def SourceFileCompilationError2Test():
	res = RunBuildSystemWithErrors( "source_file_compilation_error2" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Can not read file" ) != -1 )
	assert( stderr.find( "non_existing_imported_file.uh" ) != -1 )
	assert( stderr.find( "Command \"main.u.bc\" execution failed" ) != -1 )


def LinkingError0Test():
	res = RunBuildSystemWithErrors( "linking_error0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( ( stderr.find( "undefined symbol" ) != -1 and stderr.find( "main" ) != -1 ) or stderr.find( "subsystem must be defined" ) != -1 )


def LinkingError1Test():
	res = RunBuildSystemWithErrors( "linking_error1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "undefined symbol" ) != -1 )
	assert( stderr.find( "Foo()" ) != -1 or stderr.find( "Foo(void)" ) != -1 )


def DuplicatedBuildTargetTest():
	res = RunBuildSystemWithErrors( "duplicated_build_target" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Error, duplicated build target \"target_c\"" ) != -1 )
	assert( stderr.find( "Package is invald" ) != -1 )


def DuplicatedSourceFileTest():
	res = RunBuildSystemWithErrors( "duplicated_source_file" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Error, duplicated source file \"main.u\" of the build target \"hello_world\"" ) != -1 )
	assert( stderr.find( "Error, duplicated source file \"dir/other.u\" of the build target \"hello_world\"" ) != -1 )
	assert( stderr.find( "Package is invald" ) != -1 )


def InvalidTargetName0Test():
	res = RunBuildSystemWithErrors( "invalid_target_name0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Error, invalid build target name \"hello_world.\"" ) != -1 )


def InvalidTargetName1Test():
	res = RunBuildSystemWithErrors( "invalid_target_name1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Error, invalid build target name \"\"" ) != -1 )


def InvalidTargetName2Test():
	res = RunBuildSystemWithErrors( "invalid_target_name2" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Error, invalid build target name \"hello/world\"" ) != -1 )


def InvalidTargetName3Test():
	res = RunBuildSystemWithErrors( "invalid_target_name3" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Error, invalid build target name \"hello" ) != -1 )


def InvalidTargetName4Test():
	res = RunBuildSystemWithErrors( "invalid_target_name4" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Error, invalid build target name \"hello" ) != -1 )


def InvalidSourceName0Test():
	res = RunBuildSystemWithErrors( "invalid_source_name0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Invalid, source name \"\" of build target \"hello_world\"" ) != -1 )


def InvalidSourceName1Test():
	res = RunBuildSystemWithErrors( "invalid_source_name1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Invalid, source name \"/source.u\" of build target \"hello_world\"" ) != -1 )


def InvalidSourceName2Test():
	res = RunBuildSystemWithErrors( "invalid_source_name2" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Invalid, source name \"C:/main.u\" of build target \"hello_world\"" ) != -1 )


def InvalidSourceName3Test():
	res = RunBuildSystemWithErrors( "invalid_source_name3" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Invalid, source name \"./main.u\" of build target \"hello_world\"" ) != -1 )


def InvalidSourceName4Test():
	res = RunBuildSystemWithErrors( "invalid_source_name4" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Invalid, source name \"some_dir/../main.u\" of build target \"hello_world\"" ) != -1 )


def InvalidSourceName5Test():
	res = RunBuildSystemWithErrors( "invalid_source_name5" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Invalid, source name \"../main.u\" of build target \"hello_world\"" ) != -1 )


def InvalidSourceName6Test():
	res = RunBuildSystemWithErrors( "invalid_source_name6" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Invalid, source name \".\" of build target \"hello_world\"" ) != -1 )


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

	test_funcs = [
		HelloWorldTest,
		MultipleConfigurationsTest,
		DebugOnlyProjectTest,
		EmptyPackageTest,
		BuildFileLoggingTest,
		TwoFilesExeTest,
		TwoTargetsTest,
		TwoTargetsCommonSourceTest,
		SourcesInDirectoriesTest,
		ТестЮникода,
		BuildFileWithImportsTest,
		MissingBuildFile,
		BrokenBuildFile0Test,
		BrokenBuildFile1Test,
		BrokenBuildFile2Test,
		BrokenBuildFile3Test,
		MissingSourceFileTest,
		SourceFileCompilationError0Test,
		SourceFileCompilationError1Test,
		SourceFileCompilationError2Test,
		LinkingError0Test,
		LinkingError1Test,
		DuplicatedBuildTargetTest,
		DuplicatedSourceFileTest,
		InvalidTargetName0Test,
		InvalidTargetName1Test,
		InvalidTargetName2Test,
		InvalidTargetName3Test,
		InvalidTargetName4Test,
		InvalidSourceName0Test,
		InvalidSourceName1Test,
		InvalidSourceName2Test,
		InvalidSourceName3Test,
		InvalidSourceName4Test,
		InvalidSourceName5Test,
		InvalidSourceName6Test ]

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
