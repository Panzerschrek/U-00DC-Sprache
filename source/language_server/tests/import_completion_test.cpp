#include "../../tests/tests_lib/funcs_registrator.hpp"
#include "../../tests/tests_lib/tests.hpp"
#include "../../compilers_support_lib/vfs.hpp"

namespace U
{

const std::string tests_directory= "tests/imports_completion_test/";

U_TEST( ImportCompletion_Test0 )
{
	const auto vfs= CreateVfsOverSystemFS( {}, {} );

	const IVfs::Path main_file_full_path= vfs->GetFullFilePath( tests_directory + "test0.u", "" );

	const std::vector<IVfs::PathCompletionItem> completions= vfs->CompletePath( "so", main_file_full_path );
	U_TEST_ASSERT( completions.size() == 1 );

	const IVfs::PathCompletionItem& completion= completions.front();
	U_TEST_ASSERT( completion.completed_path == "some_import.iu" );
}

} // namespace
