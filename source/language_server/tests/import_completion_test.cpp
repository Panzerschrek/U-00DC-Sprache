#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/FileSystem.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../tests/tests_lib/funcs_registrator.hpp"
#include "../../tests/tests_lib/tests.hpp"
#include "../../compilers_support_lib/vfs.hpp"

namespace U
{

const std::string tests_directory= "tests/imports_completion_test/";

U_TEST( ImportCompletion_Test0 )
{
	// Basic completion for relative import.

	const auto vfs= CreateVfsOverSystemFS( {}, {} );

	const IVfs::Path main_file_full_path= vfs->GetFullFilePath( tests_directory + "test0/main.u", "" );

	{ // Complete first file.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "so", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "some_import.iu" );
	}
	{ // Complete second file.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "oth", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "other_import.iu" );
	}
	{ // Complete both files.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "mp", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 2 );
		U_TEST_ASSERT( completions[0].completed_path == "other_import.iu" );
		U_TEST_ASSERT( completions[1].completed_path == "some_import.iu" );
	}
	{ // Complete for empty string - should get all files in the directory with parent file.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 3 );
		U_TEST_ASSERT( completions[0].completed_path == "main.u" );
		U_TEST_ASSERT( completions[1].completed_path == "other_import.iu" );
		U_TEST_ASSERT( completions[2].completed_path == "some_import.iu" );
	}
}

U_TEST( ImportCompletion_Test1 )
{
	// Completion for subdirectories.

	const auto vfs= CreateVfsOverSystemFS( {}, {} );

	const IVfs::Path main_file_full_path= vfs->GetFullFilePath( tests_directory + "test1/main.u", "" );

	{ // Should complete all items - files and directories.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 5 );
		U_TEST_ASSERT( completions[0].completed_path == "main.u" );
		U_TEST_ASSERT( completions[1].completed_path == "subdir_a/" );
		U_TEST_ASSERT( completions[2].completed_path == "subdir_b/" );
		U_TEST_ASSERT( completions[3].completed_path == "subdir_bbb/" );
		U_TEST_ASSERT( completions[4].completed_path == "subfile.iu" );
	}
	{ // Should complete matching directories and files.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "sub", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 4 );
		U_TEST_ASSERT( completions[0].completed_path == "subdir_a/" );
		U_TEST_ASSERT( completions[1].completed_path == "subdir_b/" );
		U_TEST_ASSERT( completions[2].completed_path == "subdir_bbb/" );
		U_TEST_ASSERT( completions[3].completed_path == "subfile.iu" );
	}
	{ // Even if full name of a directory is entered, directory name itself should be completed.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "subdir_b", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 2 );
		U_TEST_ASSERT( completions[0].completed_path == "subdir_b/" );
		U_TEST_ASSERT( completions[1].completed_path == "subdir_bbb/" );
	}
	{ // If "/" if present - contents of a directory is completed.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "subdir_a/", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "a.iu" );
	}
	{ // If "/" if present - contents of a directory is completed.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "subdir_b/", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "b.iu" );
	}
	{ // If "/" if present - contents of a directory is completed.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "subdir_bbb/", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 2 );
		U_TEST_ASSERT( completions[0].completed_path == "bbb.iu" );
		U_TEST_ASSERT( completions[1].completed_path == "deep/" );
	}
	{ // If "/" if present - contents of a directory is completed.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "subdir_bbb/deep/", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "even_deeper/" );
	}
	{ // If "/" if present - contents of a directory is completed.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "subdir_bbb/deep/even_deeper/", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "deepest_import.iu" );
	}
}

U_TEST( ImportCompletion_Test2 )
{
	// Case-insensitivity.

	const auto vfs= CreateVfsOverSystemFS( {}, {} );

	const IVfs::Path main_file_full_path= vfs->GetFullFilePath( tests_directory + "test2/main.u", "" );

	{ // Complete both lowercase and uppercase files for lowercase input.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "file", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 3 );
		U_TEST_ASSERT( completions[0].completed_path == "FILE_UPPER.iu" );
		U_TEST_ASSERT( completions[1].completed_path == "FiLe_MiXeD.iu" );
		U_TEST_ASSERT( completions[2].completed_path == "file_lower.iu" );
	}
	{ // Complete both lowercase and uppercase files for uppercase input.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "FILE", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 3 );
		U_TEST_ASSERT( completions[0].completed_path == "FILE_UPPER.iu" );
		U_TEST_ASSERT( completions[1].completed_path == "FiLe_MiXeD.iu" );
		U_TEST_ASSERT( completions[2].completed_path == "file_lower.iu" );
	}
	{ // Complete both lowercase and uppercase files for mixed input.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "fIlE", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 3 );
		U_TEST_ASSERT( completions[0].completed_path == "FILE_UPPER.iu" );
		U_TEST_ASSERT( completions[1].completed_path == "FiLe_MiXeD.iu" );
		U_TEST_ASSERT( completions[2].completed_path == "file_lower.iu" );
	}
}

U_TEST( ImportCompletion_Test3)
{
	// Case-insensitivity for directories.

	const auto vfs= CreateVfsOverSystemFS( {}, {} );

	const IVfs::Path main_file_full_path= vfs->GetFullFilePath( tests_directory + "test3/main.u", "" );

	{ // Complete lowercase directory for lowercase input.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "dir_lower/", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "lower.iu" );
	}
	if( false ) // TODO - fix it.
	{ // Complete lowercase directory for uppercase input.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "DIR_LOWER/", main_file_full_path );
		//U_TEST_ASSERT( completions.size() == 1 );
		//U_TEST_ASSERT( completions[0].completed_path == "lower.iu" );
	}
	if( false ) // TODO - fix it.
	{ // Complete uppercase directory for lowercase input.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "dir_upper/", main_file_full_path );
		//U_TEST_ASSERT( completions.size() == 1 );
		//U_TEST_ASSERT( completions[0].completed_path == "UPPER.iu" );
	}
	{ // Complete uppercase directory for uppercase input.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "DIR_UPPER/", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "UPPER.iu" );
	}
}

U_TEST( ImportCompletion_Test4 )
{
	// Use ".."

	const auto vfs= CreateVfsOverSystemFS( {}, {} );

	const IVfs::Path main_file_full_path= vfs->GetFullFilePath( tests_directory + "test4/dir_entry/main.u", "" );

	{ // Complete the whole contents of the directory above for "..".
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "..", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 4 );
		U_TEST_ASSERT( completions[0].completed_path == "dir0/" );
		U_TEST_ASSERT( completions[1].completed_path == "dir1/" );
		U_TEST_ASSERT( completions[2].completed_path == "dir_entry/" );
		U_TEST_ASSERT( completions[3].completed_path == "some_file.iu" );
	}
	{ // Complete the whole contents of the directory above for "../".
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "../", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 4 );
		U_TEST_ASSERT( completions[0].completed_path == "dir0/" );
		U_TEST_ASSERT( completions[1].completed_path == "dir1/" );
		U_TEST_ASSERT( completions[2].completed_path == "dir_entry/" );
		U_TEST_ASSERT( completions[3].completed_path == "some_file.iu" );
	}
	{ // Complete files matching with pattern including ".."
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "../some_", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "some_file.iu" );
	}
	{ // Complete directories matching with pattern including ".."
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "../dir", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 3 );
		U_TEST_ASSERT( completions[0].completed_path == "dir0/" );
		U_TEST_ASSERT( completions[1].completed_path == "dir1/" );
		U_TEST_ASSERT( completions[2].completed_path == "dir_entry/" );
	}
	{ // Complete directory contents with path including ".."
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "../dir0/", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "file0.iu" );
	}
	{ // Complete directory contents with path including ".."
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "../dir1/", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "file1.iu" );
	}
	{ // Complete directory contents with path including ".."
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "../dir_entry/", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "main.u" );
	}
}

U_TEST( ImportCompletion_Test5 )
{
	// Suggest files with given pattern not only at the start.

	const auto vfs= CreateVfsOverSystemFS( {}, {} );

	const IVfs::Path main_file_full_path= vfs->GetFullFilePath( tests_directory + "test5/main.u", "" );

	{
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "eins", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 3 );
		U_TEST_ASSERT( completions[0].completed_path == "eins_drei_vier.iu" );
		U_TEST_ASSERT( completions[1].completed_path == "eins_zwei_drei.iu" );
		U_TEST_ASSERT( completions[2].completed_path == "eins_zwei_vier.iu" );
	}
	{
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "zwei", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 3 );
		U_TEST_ASSERT( completions[0].completed_path == "zwei_drei_vier.iu" ); // Goes first, since it's at start.
		U_TEST_ASSERT( completions[1].completed_path == "eins_zwei_drei.iu" );
		U_TEST_ASSERT( completions[2].completed_path == "eins_zwei_vier.iu" );
	}
	{
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "drei", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 3 );
		U_TEST_ASSERT( completions[0].completed_path == "eins_drei_vier.iu" );
		U_TEST_ASSERT( completions[1].completed_path == "eins_zwei_drei.iu" );
		U_TEST_ASSERT( completions[2].completed_path == "zwei_drei_vier.iu" );
	}
	{
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "vier", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 3 );
		U_TEST_ASSERT( completions[0].completed_path == "eins_drei_vier.iu" );
		U_TEST_ASSERT( completions[1].completed_path == "eins_zwei_vier.iu" );
		U_TEST_ASSERT( completions[2].completed_path == "zwei_drei_vier.iu" );
	}
}

U_TEST( ImportCompletion_Test6 )
{
	// Basic completion for absolute import with non-prefixed directories.

	const std::string import_directories[]
	{
		tests_directory + "test6/dir0",
		tests_directory + "test6/dir1",
		tests_directory + "test6/dir2",
	};

	const auto vfs= CreateVfsOverSystemFS( import_directories, {} );
	U_TEST_ASSERT( vfs != nullptr );

	const IVfs::Path main_file_full_path= vfs->GetFullFilePath( tests_directory + "test6/main.u", "" );

	{ // Complete for "/" - should suggest all files in all import directories.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "/", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 7 );
		U_TEST_ASSERT( completions[0].completed_path == "file0.iu" );
		U_TEST_ASSERT( completions[1].completed_path == "file1.iu" );
		U_TEST_ASSERT( completions[2].completed_path == "file2.iu" );
		U_TEST_ASSERT( completions[3].completed_path == "file22.iu" );
		U_TEST_ASSERT( completions[4].completed_path == "subdir/" );
		U_TEST_ASSERT( completions[5].completed_path == "subdir/" ); // Have second copy, since have two such subdirectories.
		U_TEST_ASSERT( completions[6].completed_path == "unique_prefix.iu" );
	}
	{ // Complete for path starting with "/" - should suggest all matching files in all import directories.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "/file", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 4 );
		U_TEST_ASSERT( completions[0].completed_path == "file0.iu" );
		U_TEST_ASSERT( completions[1].completed_path == "file1.iu" );
		U_TEST_ASSERT( completions[2].completed_path == "file2.iu" );
		U_TEST_ASSERT( completions[3].completed_path == "file22.iu" );
	}
	{ // Complete for path starting with "/" - should suggest all matching files in all import directories.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "/unique", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "unique_prefix.iu" );
	}
	{ // Complete for path starting with prefix leading to more than one subdirectory - should suggest all matching files in all import directories.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "/subdir/", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 2 );
		U_TEST_ASSERT( completions[0].completed_path == "subfile0.iu" );
		U_TEST_ASSERT( completions[1].completed_path == "subfile1.iu" );
	}
	{ // Complete for path starting with prefix leading to more than one subdirectory - should suggest all matching files in all import directories.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "/subdir/su", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 2 );
		U_TEST_ASSERT( completions[0].completed_path == "subfile0.iu" );
		U_TEST_ASSERT( completions[1].completed_path == "subfile1.iu" );
	}
	{ // Complete for path starting with prefix leading to single file in a subdirectory.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "/subdir/0", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "subfile0.iu" );
	}
	{ // Complete for path starting with prefix leading to single file in a subdirectory.
		const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "/subdir/file1", main_file_full_path );
		U_TEST_ASSERT( completions.size() == 1 );
		U_TEST_ASSERT( completions[0].completed_path == "subfile1.iu" );
	}
}

} // namespace
