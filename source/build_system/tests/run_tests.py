import argparse
import ctypes
import os
import platform
import subprocess
import sys
import traceback


g_tests_path = ""
g_tests_build_root_path = ""
g_build_system_executable = ""
g_compiler_executable=  ""
g_build_system_imports_path = ""
g_ustlib_path = ""
g_configuration_options_file_path = ""
g_mangling_scheme = "itaniumabi"
g_sysroot = None

g_packages_repository_dir= "packages_repository"


def RunBuildSystemWithExplicitConfiguration( project_subdirectory, configuration ):
	project_root = os.path.join( g_tests_path, project_subdirectory )
	build_root = os.path.join( g_tests_build_root_path, project_subdirectory );
	build_system_args= [
		g_build_system_executable,
		"build",
		"-q",
		"--build-configuration", configuration,
		"--compiler-executable", g_compiler_executable,
		"--build-system-imports-path", g_build_system_imports_path,
		"--ustlib-path", g_ustlib_path,
		"--configuration-options", g_configuration_options_file_path,
		"--project-directory", project_root,
		"--build-directory", build_root,
		"--packages-repository-directory", os.path.join( g_tests_path, g_packages_repository_dir ),
		]

	if g_sysroot is not None:
		build_system_args.append( "--sysroot" )
		build_system_args.append( g_sysroot )
		build_system_args.append( "--host-sysroot" )
		build_system_args.append( g_sysroot )

	# Run the build.
	subprocess.check_call( build_system_args )


# Build for "release" configuration.
def RunBuildSystem( project_subdirectory ):
	return RunBuildSystemWithExplicitConfiguration( project_subdirectory, "release" )


# Returns subprocess result.
def RunBuildSystemWithErrors( project_subdirectory ):
	project_root = os.path.join( g_tests_path, project_subdirectory )
	build_root = os.path.join( g_tests_build_root_path, project_subdirectory );
	build_system_args= [
		g_build_system_executable,
		"build",
		"-q",
		"--compiler-executable", g_compiler_executable,
		"--build-system-imports-path", g_build_system_imports_path,
		"--ustlib-path", g_ustlib_path,
		"--configuration-options", g_configuration_options_file_path,
		"--project-directory", project_root,
		"--build-directory", build_root,
		"--packages-repository-directory", os.path.join( g_tests_path, g_packages_repository_dir ),
		]

	if g_sysroot is not None:
		build_system_args.append( "--sysroot" )
		build_system_args.append( g_sysroot )
		build_system_args.append( "--host-sysroot" )
		build_system_args.append( g_sysroot )

	# Run the build.
	return subprocess.run( build_system_args, stderr=subprocess.PIPE )


def RunExecutableWithExplicitConfiguration( project_subdirectory, executable_name, configuration ):
	return subprocess.check_call( [ os.path.join( g_tests_build_root_path, project_subdirectory, configuration, executable_name ) ], stdout= subprocess.DEVNULL )


# Run for "release" confuguration.
def RunExecutable( project_subdirectory, executable_name ):
	return RunExecutableWithExplicitConfiguration( project_subdirectory, executable_name, "release" )


def RunSingleProgramCompilationTest( test_name_base ):

	build_system_args= [
		g_build_system_executable,
		"build_single", os.path.join( g_tests_path, test_name_base + ".u" ),
		"-q",
		"--build-configuration", "release",
		"--compiler-executable", g_compiler_executable,
		"--ustlib-path", g_ustlib_path,
		"--build-directory", g_tests_build_root_path,
		]

	if g_sysroot is not None:
		build_system_args.append( "--sysroot" )
		build_system_args.append( g_sysroot )
		build_system_args.append( "--host-sysroot" )
		build_system_args.append( g_sysroot )

	# Run the build.
	subprocess.check_call( build_system_args )

	# Run result program
	subprocess.check_call( [ os.path.join( g_tests_build_root_path, test_name_base ) ], stdout= subprocess.DEVNULL )


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


def ConfigurationDependentTargetTest():
	# Build both "debug" and "release".
	RunBuildSystemWithExplicitConfiguration( "configuration_dependent_target", "debug" )
	RunBuildSystemWithExplicitConfiguration( "configuration_dependent_target", "release" )
	# Should get two executables in different subdirectories.
	# Debug executable name is different.
	RunExecutableWithExplicitConfiguration( "configuration_dependent_target", "configuration_dependent_target_d", "debug" )
	RunExecutableWithExplicitConfiguration( "configuration_dependent_target", "configuration_dependent_target", "release" )


def EmptyPackageTest():
	RunBuildSystem( "empty_package" )


def BuildFileLoggingTest():
	RunBuildSystem( "build_file_logging" )


def ConfigurationOptions0Test():
	project_subdirectory = "configuration_options0"
	project_root = os.path.join( g_tests_path, project_subdirectory )
	build_root = os.path.join( g_tests_build_root_path, project_subdirectory )
	options_file_path = os.path.join( project_root, "options.json" )
	build_system_args= [ g_build_system_executable, "build", "-q", "--compiler-executable", g_compiler_executable, "--build-system-imports-path", g_build_system_imports_path, "--ustlib-path", g_ustlib_path, "--project-directory", project_root, "--build-directory", build_root, "--configuration-options", options_file_path ]

	if g_sysroot is not None:
		build_system_args.append( "--sysroot" )
		build_system_args.append( g_sysroot )

	# Run the build.
	subprocess.check_call( build_system_args )

	# Should find executable with name dependent on configuration options.
	RunExecutable( project_subdirectory, "configuration_options_1234" )


def ConfigurationOptions1Test():
	project_subdirectory = "configuration_options1"
	project_root = os.path.join( g_tests_path, project_subdirectory )
	build_root = os.path.join( g_tests_build_root_path, project_subdirectory )
	options_file_path = os.path.join( project_root, "non_existing_file.json" )
	build_system_args= [ g_build_system_executable, "build", "-q", "--compiler-executable", g_compiler_executable, "--build-system-imports-path", g_build_system_imports_path, "--ustlib-path", g_ustlib_path, "--project-directory", project_root, "--build-directory", build_root, "--configuration-options", options_file_path ]

	call_res= subprocess.run( build_system_args, stderr=subprocess.PIPE )
	stderr= str(call_res.stderr)
	assert( stderr.find( "Failed to load configuration options file" ) != -1 )


def ConfigurationOptions2Test():
	project_subdirectory = "configuration_options2"
	project_root = os.path.join( g_tests_path, project_subdirectory )
	build_root = os.path.join( g_tests_build_root_path, project_subdirectory )
	options_file_path = os.path.join( project_root, "options.json" )
	build_system_args= [ g_build_system_executable, "build", "-q", "--compiler-executable", g_compiler_executable, "--build-system-imports-path", g_build_system_imports_path, "--ustlib-path", g_ustlib_path, "--project-directory", project_root, "--build-directory", build_root, "--configuration-options", options_file_path ]

	call_res= subprocess.run( build_system_args, stderr=subprocess.PIPE )
	stderr= str(call_res.stderr)
	assert( stderr.find( "Failed to parse configuration options file" ) != -1 )


def TwoFilesExeTest():
	RunBuildSystem( "two_files_exe" )
	RunExecutable( "two_files_exe", "two_files_exe" )


def ManySourceFilesTest():
	RunBuildSystem( "many_source_files" )
	RunExecutable( "many_source_files", "many_source_files" )


def TwoTargetsTest():
	RunBuildSystem( "two_targets" )
	RunExecutable( "two_targets", "target_a" )
	RunExecutable( "two_targets", "target_b" )


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


def LibraryTargetTest():
	RunBuildSystem( "library_target" )


def ExeDependsOnLibraryTest():
	RunBuildSystem( "exe_depends_on_library" )
	RunExecutable( "exe_depends_on_library", "exe" )


def TransitiveLibraryDependencyTest():
	RunBuildSystem( "transitive_library_dependency" )
	RunExecutable( "transitive_library_dependency", "exe" )


def LibraryUsedInTwoExecutablesTest():
	RunBuildSystem( "library_used_in_two_executables" )
	RunExecutable( "library_used_in_two_executables", "exe_a" )
	RunExecutable( "library_used_in_two_executables", "exe_b" )


def CommonTransitiveDependencyTest():
	RunBuildSystem( "common_transitive_dependency" )
	RunExecutable( "common_transitive_dependency", "exe" )


def MultipleTargetIncludeDirectoriesTest():
	RunBuildSystem( "multiple_target_include_directories" )
	RunExecutable( "multiple_target_include_directories", "multiple_target_include_directories" )


def BuildTargetLocalSymbolsInternalizationTest():
	RunBuildSystem( "build_target_local_symbols_internalization" )
	RunExecutable( "build_target_local_symbols_internalization", "exe" )


def PrivateExeDependencyTest():
	RunBuildSystem( "private_exe_dependency" )
	RunExecutable( "private_exe_dependency", "exe" )


def PrivateDependenciesInternalizationTest():
	RunBuildSystem( "private_dependencies_internalization" )
	RunExecutable( "private_dependencies_internalization", "exe" )


def PrivateDependencyWithPublicDependencyTest():
	RunBuildSystem( "private_dependency_with_public_dependency" )
	RunExecutable( "private_dependency_with_public_dependency", "exe" )


def PrivateDependencyIsAlsoPublicTest():
	RunBuildSystem( "private_dependency_is_also_public" )
	RunExecutable( "private_dependency_is_also_public", "exe" )


def PublicDependencyOfPrivateDependencyIsAlsoPublicDependencyTest():
	RunBuildSystem( "public_dependency_of_private_dependency_is_also_public_dependency" )
	RunExecutable( "public_dependency_of_private_dependency_is_also_public_dependency", "exe" )


def PublicDependencyOfPrivateDependencyIsAlsoPrivateDependencyTest():
	RunBuildSystem( "public_dependency_of_private_dependency_is_also_private_dependency" )
	RunExecutable( "public_dependency_of_private_dependency_is_also_private_dependency", "exe" )


def TwoPrivateDependenciesSharedCommonPrivateDependency():
	RunBuildSystem( "two_private_dependencies_share_common_private_dependency" )
	RunExecutable( "two_private_dependencies_share_common_private_dependency", "exe" )


def SharedLibraryTargetTest():
	# Build the library
	RunBuildSystem( "shared_library_target" )
	# Load result shared library.
	library_file_path= os.path.join( g_tests_build_root_path, "shared_library_target", "release", "shared_library_target" )
	if platform.system() == "Windows":
		library_file_path+= ".dll"
	else:
		library_file_path+= ".so"
	library= ctypes.CDLL( library_file_path )
	# Call some functions.
	library.AddTwoNumbers.restype = ctypes.c_uint
	assert( library.AddTwoNumbers( ctypes.c_uint(42), ctypes.c_uint(123) ) == 42 + 123 )
	library.FloatDiv.restype = ctypes.c_float
	assert( library.FloatDiv( ctypes.c_float(30.5), ctypes.c_float(4.0) ) == 7.625 )
	# Internal implementation functions shouldn't be exported.
	assert( not hasattr( library, "InternalFunction" ) )


def ExeDependsOnSharedLibraryTest():
	RunBuildSystem( "exe_depends_on_shared_library" )
	RunExecutable( "exe_depends_on_shared_library", "exe" )


def ExeDependsOnLibraryWithPrivateSharedLibraryDependency():
	RunBuildSystem( "exe_depends_on_library_with_private_shared_library_dependency" )
	RunExecutable( "exe_depends_on_library_with_private_shared_library_dependency", "exe" )


def PrivateSharedLibraryWithPrivateSharedLibraryDependency():
	RunBuildSystem( "private_shared_library_dependency_with_private_shared_library_dependency" )
	RunExecutable( "private_shared_library_dependency_with_private_shared_library_dependency", "exe" )


def PrivateSharedLibraryWithPublicSharedLibraryDependency():
	RunBuildSystem( "private_shared_library_dependency_with_public_shared_library_dependency" )
	RunExecutable( "private_shared_library_dependency_with_public_shared_library_dependency", "exe" )


def CommonTransitiveSharedLibraryDependencyTest():
	RunBuildSystem( "common_transitive_shared_library_dependency" )
	RunExecutable( "common_transitive_shared_library_dependency", "exe" )


def PrivateSharedLibraryDependencyWithPublicLibraryDependencyTest():
	test_dir= "private_shared_library_dependency_with_public_library_dependency"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "exe" )
	# Load result shared library.
	library_file_path= os.path.join( g_tests_build_root_path, test_dir, "release", "a" )
	if platform.system() == "Windows":
		library_file_path+= ".dll"
	else:
		library_file_path+= ".so"
	library= ctypes.CDLL( library_file_path )
	if g_mangling_scheme == "msvc":
		a_func= getattr( library, "?AFunc@@YAIXZ" )
		b_func= getattr( library, "?BFunc@@YAIXZ" )
	else:
		a_func= getattr( library, "_Z5AFuncv" )
		b_func= getattr( library, "_Z5BFuncv" )
	# Call "A" function which is part of public interface "A".
	a_func.restype = ctypes.c_uint
	assert( a_func() == 66664 * 7 )
	# Call "B" function, which should be also exported, because it's a public dependency of the shared library.
	b_func.restype = ctypes.c_uint
	assert( b_func() == 66664 )


def PrivateSharedLibraryDependencyWithPrivateLibraryDependencyTest():
	test_dir= "private_shared_library_dependency_with_private_library_dependency"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "exe" )
	# Load result shared library.
	library_file_path= os.path.join( g_tests_build_root_path, test_dir, "release", "a" )
	if platform.system() == "Windows":
		library_file_path+= ".dll"
	else:
		library_file_path+= ".so"
	library= ctypes.CDLL( library_file_path )
	if g_mangling_scheme == "msvc":
		a_func= getattr( library, "?AFunc@@YAIXZ" )
	else:
		a_func= getattr( library, "_Z5AFuncv" )
	# Call "A" function which is part of public interface "A".
	a_func.restype = ctypes.c_uint
	assert( a_func() == 66664 * 7 )
	# Functions from "B" shouldn't be exported, since "B" is a private dependency of shared library "a".
	assert( not hasattr( library, "_Z5BFuncv" ) )
	assert( not hasattr( library, "?BFunc@@YAIXZ" ) )


def SharedLibraryDeduplicatedTransitivePublicSharedLibraryDependencyTest():
	test_dir= "shared_library_deduplicated_transitive_public_shared_library_dependency"
	RunBuildSystem( test_dir )

	# Load result shared library.
	library_file_path= os.path.join( g_tests_build_root_path, test_dir, "release", "a" )
	if platform.system() == "Windows":
		# HACK! Load "b" first in order for it to be found while loading "a".
		# This is necessary since Windows dll's have no "rpath=$ORIGIN".
		b_library= ctypes.CDLL( os.path.join( g_tests_build_root_path, test_dir, "release", "b.dll" ) )
		library_file_path+= ".dll"
	else:
		library_file_path+= ".so"
	library= ctypes.CDLL( library_file_path )

	# Call "A" function which is part of public interface "A".
	if g_mangling_scheme == "msvc":
		a_func= getattr( library, "?AFunc@@YAIXZ" )
	else:
		a_func= getattr( library, "_Z5AFuncv" )
	a_func.restype = ctypes.c_uint

	assert( a_func() == (11177 + 17) * 5 )
	if platform.system() == "Windows":
		# Functions from "b" shouldn't be exported, since "b" is a public shared library dependency of "a".
		# This works only for windows DLLs.
		assert( not hasattr( library, "_Z5BFuncv" ) )
		assert( not hasattr( library, "?BFunc@@YAIXZ" ) )
		# Functions from "c" shouldn't be exported, since "c" is a public dependency of shared library "b", which is public dependency of "a".
		# So, "b" already contains "c".
		assert( not hasattr( library, "_Z5CFuncv" ) )
		assert( not hasattr( library, "?CFunc@@YAIXZ" ) )
	# Imported symbols are still listed in library functions list of ".so" libraries, in order to load them from dependent libraries properly.
	# But they are not actually implemented there, only imported.
	# The similar behavior of "dlsym" function is observable also for any symbols from dependent libraries of this library.
	# So, even functions like "printf" are present in the result loaded library, even if they are defined in glibc.


def SharedLibraryUsedInTwoExecutablesTest():
	RunBuildSystem( "shared_library_used_in_two_executables" )
	RunExecutable( "shared_library_used_in_two_executables", "exe_a" )
	RunExecutable( "shared_library_used_in_two_executables", "exe_b" )


def ObjectFileTargetTest():
	RunBuildSystem( "object_file_target" )
	if platform.system() == "Linux":
		# Run "nm" and check output - which symbols are present in result object file.
		object_file_path= os.path.join( g_tests_build_root_path, "object_file_target", "release", "object_file_target.o" )
		nm_res= subprocess.run( [ "nm", object_file_path ], stdout=subprocess.PIPE )
		assert( nm_res.returncode == 0 )
		stdout= str(nm_res.stdout)
		# Should export public functions
		assert( stdout.find( "AddTwoNumbers" ) != -1 )
		assert( stdout.find( "FloatDiv" ) != -1 )
		assert( stdout.find( "FloatDiv" ) != -1 )
		# Should not export internal functions
		assert( stdout.find( "InternalFunction" ) == -1 )


def ExternalLibraryLinking0Test():
	RunBuildSystem( "external_library_linking0" )
	RunExecutable( "external_library_linking0", "exe" )


def ExternalLibraryLinking1Test():
	test_dir = "external_library_linking1"
	RunBuildSystem( test_dir )

	external_shared_lib_dir = os.path.normpath( g_tests_build_root_path + "/.." )
	# Slightly hacky way to find necessary shared library upon launch - set LD_LIBRARY_PATH.
	env_tweaked = os.environ
	if platform.system() == "Linux":
		env_tweaked["LD_LIBRARY_PATH"]= external_shared_lib_dir
	# Set also current directory - Windows searches for dll's in current directory.
	subprocess.check_call( [ os.path.join( g_tests_build_root_path, test_dir, "release", "exe" ) ], env= env_tweaked, cwd = external_shared_lib_dir )


def ExternalLibraryLinking2Test():
	RunBuildSystem( "external_library_linking2" )
	RunExecutable( "external_library_linking2", "exe" )


def ExternalLibraryLinking3Test():
	RunBuildSystem( "external_library_linking3" )
	RunExecutable( "external_library_linking3", "exe" )


def ChildPackage0Test():
	RunBuildSystem( "child_package0" )
	RunExecutable( "child_package0", "exe" )


def ChildPackage1Test():
	RunBuildSystem( "child_package1" )
	RunExecutable( "child_package1", "exe" )


def ChildPackage2Test():
	RunBuildSystem( "child_package2" )
	RunExecutable( "child_package2", "exe" )


def ChildPackage3Test():
	RunBuildSystem( "child_package3" )
	RunExecutable( "child_package3", "exe" )


def ChildPackage4Test():
	RunBuildSystem( "child_package4" )
	RunExecutable( "child_package4", "sub_package/child_package_exe" )


def ChildPackage5Test():
	RunBuildSystem( "child_package5" )
	RunExecutable( "child_package5", "exe" )


def ChildPackage6Test():
	RunBuildSystem( "child_package6" )
	RunExecutable( "child_package6", "sub_package/exe" )


def ChildPackage7Test():
	RunBuildSystem( "child_package7" )
	RunExecutable( "child_package7", "exe" )


def ChildPackage8Test():
	RunBuildSystem( "child_package8" )
	RunExecutable( "child_package8", "exe" )


def UsePackageFromRepositoryTest0():
	RunBuildSystem( "use_package_from_repository0" )
	RunExecutable( "use_package_from_repository0", "exe" )


def UsePackageFromRepositoryTest1():
	RunBuildSystem( "use_package_from_repository1" )
	RunExecutable( "use_package_from_repository1", "exe" )


def UsePackageFromRepositoryTest2():
	RunBuildSystem( "use_package_from_repository2" )
	RunExecutable( "use_package_from_repository2", "exe" )


def UsePackageFromRepositoryTest3():
	RunBuildSystem( "use_package_from_repository3" )
	RunExecutable( "use_package_from_repository3", "exe1" )
	RunExecutable( "use_package_from_repository3", "exe2" )


def UsePackageFromRepositoryTest4():
	RunBuildSystem( "use_package_from_repository4" )
	RunExecutable( "use_package_from_repository4", "exe" )


def CustomBuildStep0Test():
	test_dir= "custom_build_step0"
	RunBuildSystem( test_dir )
	with open( os.path.join( g_tests_build_root_path, test_dir, "release", "some_file_multiplied.txt" ), "r") as file:
		data = file.read()
		assert( data == "abcabcabcabc" )


def CustomBuildStep1Test():
	test_dir= "custom_build_step1"
	RunBuildSystem( test_dir )
	with open( os.path.join( g_tests_build_root_path, test_dir, "release", "some_file_intermediate.txt" ), "r") as file:
		data = file.read()
		assert( data == "abcabc" )
	with open( os.path.join( g_tests_build_root_path, test_dir, "release", "some_file_final.txt" ), "r") as file:
		data = file.read()
		assert( data == "abcabcabcabcabcabc" )


def CustomBuildStep2Test():
	test_dir= "custom_build_step2"
	RunBuildSystem( test_dir )
	with open( os.path.join( g_tests_build_root_path, test_dir, "release", "combined.txt" ), "r") as file:
		data = file.read()
		assert( data == "12345671234567123456712345671234567" )


def CustomBuildStep3Test():
	test_dir= "custom_build_step3"
	RunBuildSystem( test_dir )
	with open( os.path.join( g_tests_build_root_path, test_dir, "release", "some_file_copy.txt" ), "r") as file:
		data = file.read()
		assert( data == "abc" )


def CustomBuildStep4Test():
	test_dir= "custom_build_step4"
	RunBuildSystem( test_dir )
	with open( os.path.join( g_tests_build_root_path, test_dir, "release", "some_generated_file.txt" ), "r") as file:
		data = file.read()
		assert( data == "Ewigheim" )


def CustomBuildStep5Test():
	test_dir= "custom_build_step5"
	RunBuildSystem( test_dir )
	# Run copy of an executable build target, which was produces by custom build step.
	RunExecutable( test_dir, "custom_build_step5_copy" )


def GeneratedSources0Test():
	test_dir= "generated_sources0"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "generated_sources0" )


def GeneratedSources1Test():
	test_dir= "generated_sources1"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "generated_sources1" )


def GeneratedSources2Test():
	test_dir= "generated_sources2"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "generated_sources2" )


def GeneratedSources3Test():
	test_dir= "generated_sources3"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "exe" )


def GeneratedSources4Test():
	test_dir= "generated_sources4"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "exe" )


def GeneratedSources5Test():
	test_dir= "generated_sources5"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "generated_sources5" )


def HostDependentPackage0Test():
	test_dir= "host_dependent_package0"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "exe" )


def HostDependentPackage1Test():
	test_dir= "host_dependent_package1"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "exe" )


def GlobalPackagesBuildTargetsVersionUnification0Test():
	test_dir= "global_packages_build_targets_version_unification0"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "unification_test" )


def GlobalPackagesBuildTargetsVersionUnification1Test():
	test_dir= "global_packages_build_targets_version_unification1"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "unification_test" )


def GlobalPackagesBuildTargetsVersionUnification2Test():
	test_dir= "global_packages_build_targets_version_unification2"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "unification_test" )


def GlobalPackagesBuildTargetsVersionUnification3Test():
	test_dir= "global_packages_build_targets_version_unification3"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "unification_test" )


def GlobalPackagesBuildTargetsVersionUnification4Test():
	test_dir= "global_packages_build_targets_version_unification4"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "unification_test" )


def BuildTargetWithoutSources0Test():
	RunBuildSystem( "build_target_without_sources0" )


def BuildTargetWithoutSources1Test():
	test_dir= "build_target_without_sources1"
	RunBuildSystem( test_dir )
	RunExecutable( test_dir, "exe" )


def SingleFileProgram0Test():
	RunSingleProgramCompilationTest( "single_file_program0" )


def SingleFileProgram1Test():
	RunSingleProgramCompilationTest( "single_file_program1" )


def MissingBuildFileTest():
	# A directory with no build file.
	res = RunBuildSystemWithErrors( "missing_build_file" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Can not get modification time for" ) != -1 )
	assert( stderr.find( "file does not exist?" ) != -1 )
	assert( stderr.find( "Failed to load/build the build script shared library" ) != -1 )


def MissingPackage0Test():
	res = RunBuildSystemWithErrors( "missing_package0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Can not get modification time for" ) != -1 )
	assert( stderr.find( "non_existing_package/build.u" ) != -1 )
	assert( stderr.find( "file does not exist?" ) != -1 )
	assert( stderr.find( "Failed to load/build the build script shared library" ) != -1 )


def MissingPackage1Test():
	res = RunBuildSystemWithErrors( "missing_package1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Can not get modification time for" ) != -1 )
	assert( stderr.find( "sub_package/build.u" ) != -1 )
	assert( stderr.find( "file does not exist?" ) != -1 )
	assert( stderr.find( "Failed to load/build the build script shared library" ) != -1 )


def MissingPackage2Test():
	res = RunBuildSystemWithErrors( "missing_package2" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Failed to resolve package dependency \"..\" relative to \"\" - too many \"..\"!" ) != -1 )


def BuildScriptNullResultTest():
	res = RunBuildSystemWithErrors( "build_script_null_result" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Failed to get package info") != -1 )
	assert( stderr.find( "build script returned empty optional" ) != -1 )


def BuildScriptHaltTest():
	res = RunBuildSystemWithErrors( "build_script_halt" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Abort signal recieved. Halt in build script file?" ) != -1 )


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
	assert( stderr.find( "Can not read file" ) != -1 or stderr.find( "can not read file" ) != -1 )
	assert( stderr.find( "non_existing_file_imported_in_build_file.uh" ) != -1 )
	assert( stderr.find( "Compiler execution failed" ) != -1 )


def MissingSourceFileTest():
	res = RunBuildSystemWithErrors( "missing_source_file" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Can not read file" ) != -1 or stderr.find( "can not read file" ) != -1 )
	assert( stderr.find( "this_file_does_not_exist.u" ) != -1 )


def SourceFileCompilationError0Test():
	res = RunBuildSystemWithErrors( "source_file_compilation_error0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Syntax error" ) != -1 )


def SourceFileCompilationError1Test():
	res = RunBuildSystemWithErrors( "source_file_compilation_error1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Name \"Foo\" not found" ) != -1 )


def SourceFileCompilationError2Test():
	res = RunBuildSystemWithErrors( "source_file_compilation_error2" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Can not read file" ) != -1 or stderr.find( "can not read file" ) != -1 )
	assert( stderr.find( "non_existing_imported_file.uh" ) != -1 )


def SourceFileCompilationError3Test():
	res = RunBuildSystemWithErrors( "source_file_compilation_error3" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Missing \"return\" in function, returning non-void." ) != -1 )


def LinkingError0Test():
	res = RunBuildSystemWithErrors( "linking_error0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( ( stderr.find( "undefined symbol" ) != -1 and ( stderr.find( "main" ) != -1 or stderr.find( "WinMain" ) != -1 ) ) or stderr.find( "subsystem must be defined" ) != -1 )


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
	assert( stderr.find( "Package" ) != -1 )
	assert( stderr.find( "is invald" ) != -1 )


def DuplicatedSourceFileTest():
	res = RunBuildSystemWithErrors( "duplicated_source_file" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Error, duplicated source file \"main.u\" of the build target \"hello_world\"" ) != -1 )
	if platform.system() == "Windows":
		assert( stderr.find( "Error, duplicated source file \"dir\\\\other.u\" of the build target \"hello_world\"" ) != -1 )
	assert( stderr.find( "Package" ) != -1 )
	assert( stderr.find( "is invald" ) != -1 )


def DuplicatedGeneratedPublicHeaderFileTest():
	res = RunBuildSystemWithErrors( "duplicated_generated_public_header_file" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	if platform.system() == "Windows":
		assert( stderr.find( "Error, duplicated generated public header file \"dir\\\\other.uh\" of the build target \"hello_world\"!" ) != -1 )
	assert( stderr.find( "Error, duplicated generated public header file \"some.uh\" of the build target \"hello_world\"!" ) != -1 )


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


def InvalidSourceName7Test():
	res = RunBuildSystemWithErrors( "invalid_source_name7" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Invalid, generated source name \"../some.u\" of build target \"hello_world\"!" ) != -1 )


def InvalidSourceName8Test():
	res = RunBuildSystemWithErrors( "invalid_source_name8" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Invalid, generated public header file name \"\" of build target \"hello_world\"!" ) != -1 )


def InvalidGlobalPackageName0Test():
	res = RunBuildSystemWithErrors( "invalid_global_package_name0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Global package dependency \"wrong . name\" is not valid!" ) != -1 )


def InvalidGlobalPackageName1Test():
	res = RunBuildSystemWithErrors( "invalid_global_package_name1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Can\\'t specify zero version for global package \"two_returner\"!" ) != -1 )


def SourceDirectoriesConflict0Test():
	res = RunBuildSystemWithErrors( "source_directories_conflict0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Source directory \"some_dir/\" of the build target \"target_b\" is already in use." ) != -1 )


def SourceDirectoriesConflict1Test():
	res = RunBuildSystemWithErrors( "source_directories_conflict1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Source directory \"\" of the build target \"target_b\" is already in use." ) != -1 )


def SourceDirectoriesConflict2Test():
	res = RunBuildSystemWithErrors( "source_directories_conflict2" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Source directory \"sub_dir/\" of the build target \"target_b\" is located within another used directory." ) != -1 )


def SourceDirectoriesConflict3Test():
	res = RunBuildSystemWithErrors( "source_directories_conflict3" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Source directory \"some_dir/\" of the build target \"target_b\" is a prefix of another used directory." ) != -1 )


def SourceDirectoriesConflict4Test():
	res = RunBuildSystemWithErrors( "source_directories_conflict4" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Public include directory \"some_dir\" of the build target \"target_a\" is already in use." ) != -1 )


def SourceDirectoriesConflict5Test():
	res = RunBuildSystemWithErrors( "source_directories_conflict5" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Public include directory \"include_dir\" of the build target \"target_a\" is already in use." ) != -1 )


def SourceDirectoriesConflict6Test():
	res = RunBuildSystemWithErrors( "source_directories_conflict6" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Public include directory \"include_dir\" of the build target \"target_b\" is already in use." ) != -1 )


def SourceDirectoriesConflict7Test():
	res = RunBuildSystemWithErrors( "source_directories_conflict7" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Public include directory \"target_a/include\" of the build target \"target_b\" is located within another used directory." ) != -1 )


def SourceDirectoriesConflict8Test():
	res = RunBuildSystemWithErrors( "source_directories_conflict8" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Public include directory \"common_dir\" of the build target \"target_b\" is a prefix of another used directory." ) != -1 )


def PackageDirectoryNameConflictTest():
	res = RunBuildSystemWithErrors( "package_directory_name_conflict" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Package dependency directory \"sub_package\" is located within another used directory." ) != -1 )


def SelfDependency0Test():
	res = RunBuildSystemWithErrors( "self_dependency0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Build target \"library_target\" depends on itself." ) != -1 )


def SelfDependency1Test():
	res = RunBuildSystemWithErrors( "self_dependency1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Build target \"library_target\" depends on itself." ) != -1 )


def MissingDependencyTest():
	res = RunBuildSystemWithErrors( "missing_dependency" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Dependency \"unknown_dependency\" not found." ) != -1 )


def DependencyNameConflictTest():
	res = RunBuildSystemWithErrors( "dependency_name_conflict" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Build target \"exe\" has dependencies with identical name \"sub_package_a/lib\" and \"sub_package_b/lib\"!" ) != -1 )


def ChildPackageBuildTargetNameConflictTest():
	res = RunBuildSystemWithErrors( "child_package_build_target_name_conflict" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Package dependency directory \"sub_package\" conflicts with build target name \"sub_package\"!" ) != -1 )


def DependencyLoop0Test():
	res = RunBuildSystemWithErrors( "dependency_loop0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Dependency loop detected: \"lib_a\" -> \"lib_b\" -> \"lib_a\"" ) != -1 )


def DependencyLoop1Test():
	res = RunBuildSystemWithErrors( "dependency_loop1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Dependency loop detected: \"lib_a\" -> \"lib_b\" -> \"lib_c\" -> \"lib_d\" -> \"lib_a\"" ) != -1 )


def DependencyLoop2Test():
	res = RunBuildSystemWithErrors( "dependency_loop2" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Dependency loop detected: \"lib_a\" -> \"lib_b\" -> \"lib_a\"" ) != -1 )


def DependencyLoop3Test():
	res = RunBuildSystemWithErrors( "dependency_loop3" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Dependency loop detected: \"lib_a\" -> \"lib_b\" -> \"lib_c\" -> \"lib_d\" -> \"lib_a\"" ) != -1 )


def DependencyLoop4Test():
	res = RunBuildSystemWithErrors( "dependency_loop4" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Dependency loop detected: \"lib_a\" -> \"lib_b\" -> \"lib_a\"" ) != -1 )


def DependencyLoop5Test():
	res = RunBuildSystemWithErrors( "dependency_loop5" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Dependency loop detected: \"lib_a\" -> \"lib_b\" -> \"lib_c\" -> \"lib_d\" -> \"lib_a\"" ) != -1 )


def DependencyOnExe0Test():
	res = RunBuildSystemWithErrors( "dependency_on_exe0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Build target \"exe_a\" depends on non-library build target \"exe_b\"." ) != -1 )


def DependencyOnExe1Test():
	res = RunBuildSystemWithErrors( "dependency_on_exe1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Build target \"exe_a\" depends on non-library build target \"exe_b\"." ) != -1 )


def DependencyOnObjectFileTest():
	res = RunBuildSystemWithErrors( "dependency_on_object_file" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Build target \"exe\" depends on non-library build target \"object_file\"." ) != -1 )


def UnallowedImport0():
	res = RunBuildSystemWithErrors( "unallowed_import0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Importing file \"" ) != -1 )
	assert( stderr.find( "t allowed." ) != -1 )


def UnallowedImport1():
	res = RunBuildSystemWithErrors( "unallowed_import1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Importing file \"" ) != -1 )
	assert( stderr.find( "t allowed." ) != -1 )


def UnallowedImport2():
	res = RunBuildSystemWithErrors( "unallowed_import2" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Importing file \"" ) != -1 )
	assert( stderr.find( "t allowed." ) != -1 )


def UnallowedImport3():
	res = RunBuildSystemWithErrors( "unallowed_import3" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "not allowed to embed file \"" ) != -1 )


def UnallowedImport4():
	res = RunBuildSystemWithErrors( "unallowed_import4" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Importing file \"" ) != -1 )
	assert( stderr.find( "not_allowed_import4.uh" ) != -1 )
	assert( stderr.find( "t allowed." ) != -1 )


def ExePublicDependencyTest():
	res = RunBuildSystemWithErrors( "exe_public_dependency" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Non-empty public dependencies list for an executable build target \"exe\"." ) != -1 )


def ExePublicIncludeDirectoriesTest():
	res = RunBuildSystemWithErrors( "exe_public_include_directories" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Non-empty public include directories list for an executable build target \"exe\"." ) != -1 )


def ExePublicGeneratedHeadersTest():
	res = RunBuildSystemWithErrors( "exe_public_generated_headers" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Non-empty generated public headers list for an executable build target \"exe\"." ) != -1 )


def CustomBuildStepFilePathIsNotAbsolute0Test():
	res = RunBuildSystemWithErrors( "custom_build_step_file_path_is_not_absolute0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Error, custom build step input file path \"some_file.txt\" is not absolute!" ) != -1 )


def CustomBuildStepFilePathIsNotAbsolute1Test():
	res = RunBuildSystemWithErrors( "custom_build_step_file_path_is_not_absolute1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Error, custom build step output file path \"some_file_multiplied.txt\" is not absolute!" ) != -1 )


def CustomBuildStepFilePathIsNotAbsolute2Test():
	res = RunBuildSystemWithErrors( "custom_build_step_file_path_is_not_absolute2" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Error, custom build step executable path \"BuildSystemTestFileGenerationTool\" is not absolute!" ) != -1 )


def CustomBuildStepsShareSameOutputFileTest():
	res = RunBuildSystemWithErrors( "custom_build_steps_share_same_output_file" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Error, two custom build steps \"step0\" and \"step1\" share same output file" ) != -1 )


def CustomBuildStepsDependencyLoopTest():
	res = RunBuildSystemWithErrors( "custom_build_steps_dependency_loop" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert(
		stderr.find( "Broken build graph - node \"step0\" was not built, likely due to dependency loops." ) != -1 or
		stderr.find( "Broken build graph - node \"step1\" was not built, likely due to dependency loops." ) != -1 )


def MissingCustomBuildStepForGeneratedFile0Test():
	res = RunBuildSystemWithErrors( "missing_custom_build_step_for_generated_file0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "No custom build step found for generated source file \"main.u\" of build target \"exe\"!" ) != -1 )


def MissingCustomBuildStepForGeneratedFile1Test():
	res = RunBuildSystemWithErrors( "missing_custom_build_step_for_generated_file1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "No custom build step found for generated private header file \"some.uh\" of build target \"exe\"!" ) != -1 )


def MissingCustomBuildStepForGeneratedFile2Test():
	res = RunBuildSystemWithErrors( "missing_custom_build_step_for_generated_file2" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "No custom build step found for generated public header file \"some.uh\" of build target \"lib\"!" ) != -1 )


def HostBuildTargetCommandError0Test():
	res = RunBuildSystemWithErrors( "host_build_target_command_error0" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Host build target \"sub_package/helper_tool\" used in custom build step \"test.uh\" is not executable!" ) != -1 )


def HostBuildTargetCommandError1Test():
	res = RunBuildSystemWithErrors( "host_build_target_command_error1" )
	assert( res.returncode != 0 )
	stderr = str(res.stderr)
	assert( stderr.find( "Host build target \"sub_package/helper_tool\" used in custom build step \"test.uh\" not found!" ) != -1 )


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
	parser.add_argument( "--configuration-options-file-path", help= "path to configuration options JSON file", type=str, required= True )
	parser.add_argument( "--mangling-scheme", help= "mangling scheme - msvc or intaniumabi", type=str, default= "itaniumabi" )
	parser.add_argument( "--sysroot", help= "provide sysroot for the compiler", type=str, default= None )

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

	global g_configuration_options_file_path
	g_configuration_options_file_path= args.configuration_options_file_path

	global g_mangling_scheme
	g_mangling_scheme= args.mangling_scheme

	global g_sysroot
	g_sysroot= args.sysroot

	test_funcs = [
		HelloWorldTest,
		MultipleConfigurationsTest,
		DebugOnlyProjectTest,
		ConfigurationDependentTargetTest,
		EmptyPackageTest,
		BuildFileLoggingTest,
		ConfigurationOptions0Test,
		ConfigurationOptions1Test,
		ConfigurationOptions2Test,
		TwoFilesExeTest,
		ManySourceFilesTest,
		TwoTargetsTest,
		SourcesInDirectoriesTest,
		ТестЮникода,
		BuildFileWithImportsTest,
		LibraryTargetTest,
		ExeDependsOnLibraryTest,
		TransitiveLibraryDependencyTest,
		LibraryUsedInTwoExecutablesTest,
		CommonTransitiveDependencyTest,
		MultipleTargetIncludeDirectoriesTest,
		BuildTargetLocalSymbolsInternalizationTest,
		PrivateExeDependencyTest,
		PrivateDependenciesInternalizationTest,
		PrivateDependencyWithPublicDependencyTest,
		PrivateDependencyIsAlsoPublicTest,
		PublicDependencyOfPrivateDependencyIsAlsoPublicDependencyTest,
		PublicDependencyOfPrivateDependencyIsAlsoPrivateDependencyTest,
		TwoPrivateDependenciesSharedCommonPrivateDependency,
		SharedLibraryTargetTest,
		ExeDependsOnSharedLibraryTest,
		ExeDependsOnLibraryWithPrivateSharedLibraryDependency,
		PrivateSharedLibraryWithPrivateSharedLibraryDependency,
		PrivateSharedLibraryWithPublicSharedLibraryDependency,
		CommonTransitiveSharedLibraryDependencyTest,
		PrivateSharedLibraryDependencyWithPublicLibraryDependencyTest,
		PrivateSharedLibraryDependencyWithPrivateLibraryDependencyTest,
		SharedLibraryDeduplicatedTransitivePublicSharedLibraryDependencyTest,
		SharedLibraryUsedInTwoExecutablesTest,
		ObjectFileTargetTest,
		ExternalLibraryLinking0Test,
		ExternalLibraryLinking1Test,
		ExternalLibraryLinking2Test,
		ExternalLibraryLinking3Test,
		ChildPackage0Test,
		ChildPackage1Test,
		ChildPackage2Test,
		ChildPackage3Test,
		ChildPackage4Test,
		ChildPackage5Test,
		ChildPackage6Test,
		ChildPackage7Test,
		ChildPackage8Test,
		UsePackageFromRepositoryTest0,
		UsePackageFromRepositoryTest1,
		UsePackageFromRepositoryTest2,
		UsePackageFromRepositoryTest3,
		UsePackageFromRepositoryTest4,
		CustomBuildStep0Test,
		CustomBuildStep1Test,
		CustomBuildStep2Test,
		CustomBuildStep3Test,
		CustomBuildStep4Test,
		CustomBuildStep5Test,
		GeneratedSources0Test,
		GeneratedSources1Test,
		GeneratedSources2Test,
		GeneratedSources3Test,
		GeneratedSources4Test,
		GeneratedSources5Test,
		HostDependentPackage0Test,
		HostDependentPackage1Test,
		GlobalPackagesBuildTargetsVersionUnification0Test,
		GlobalPackagesBuildTargetsVersionUnification1Test,
		GlobalPackagesBuildTargetsVersionUnification2Test,
		GlobalPackagesBuildTargetsVersionUnification3Test,
		GlobalPackagesBuildTargetsVersionUnification4Test,
		BuildTargetWithoutSources0Test,
		BuildTargetWithoutSources1Test,
		SingleFileProgram0Test,
		SingleFileProgram1Test,
		MissingBuildFileTest,
		MissingPackage0Test,
		MissingPackage1Test,
		MissingPackage2Test,
		BuildScriptNullResultTest,
		BuildScriptHaltTest,
		BrokenBuildFile0Test,
		BrokenBuildFile1Test,
		BrokenBuildFile2Test,
		BrokenBuildFile3Test,
		MissingSourceFileTest,
		SourceFileCompilationError0Test,
		SourceFileCompilationError1Test,
		SourceFileCompilationError2Test,
		SourceFileCompilationError3Test,
		LinkingError0Test,
		LinkingError1Test,
		DuplicatedBuildTargetTest,
		DuplicatedSourceFileTest,
		DuplicatedGeneratedPublicHeaderFileTest,
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
		InvalidSourceName6Test,
		InvalidSourceName7Test,
		InvalidSourceName8Test,
		InvalidGlobalPackageName0Test,
		InvalidGlobalPackageName1Test,
		SourceDirectoriesConflict0Test,
		SourceDirectoriesConflict1Test,
		SourceDirectoriesConflict2Test,
		SourceDirectoriesConflict3Test,
		SourceDirectoriesConflict4Test,
		SourceDirectoriesConflict5Test,
		SourceDirectoriesConflict6Test,
		SourceDirectoriesConflict7Test,
		SourceDirectoriesConflict8Test,
		PackageDirectoryNameConflictTest,
		SelfDependency0Test,
		SelfDependency1Test,
		MissingDependencyTest,
		DependencyNameConflictTest,
		ChildPackageBuildTargetNameConflictTest,
		DependencyLoop0Test,
		DependencyLoop1Test,
		DependencyLoop2Test,
		DependencyLoop3Test,
		DependencyLoop4Test,
		DependencyLoop5Test,
		DependencyOnExe0Test,
		DependencyOnExe1Test,
		DependencyOnObjectFileTest,
		UnallowedImport0,
		UnallowedImport1,
		UnallowedImport2,
		UnallowedImport3,
		UnallowedImport4,
		ExePublicDependencyTest,
		ExePublicIncludeDirectoriesTest,
		ExePublicGeneratedHeadersTest,
		CustomBuildStepFilePathIsNotAbsolute0Test,
		CustomBuildStepFilePathIsNotAbsolute1Test,
		CustomBuildStepFilePathIsNotAbsolute2Test,
		CustomBuildStepsShareSameOutputFileTest,
		CustomBuildStepsDependencyLoopTest,
		MissingCustomBuildStepForGeneratedFile0Test,
		MissingCustomBuildStepForGeneratedFile1Test,
		MissingCustomBuildStepForGeneratedFile2Test,
		HostBuildTargetCommandError0Test,
		HostBuildTargetCommandError1Test,
		]

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
