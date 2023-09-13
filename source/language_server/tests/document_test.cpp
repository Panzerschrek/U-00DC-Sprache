#include "../document.hpp"
#include "../../compilers_support_lib/prelude.hpp"
#include "../../lex_synt_lib_common/assert.hpp"
#include "../../tests/tests_lib/funcs_registrator.hpp"
#include "../../tests/tests_lib/tests.hpp"
#include "../../tests/tests_common.hpp"


namespace U
{

namespace LangServer
{

namespace
{

DocumentBuildOptions GetTestDocumentBuildOptions()
{
	DocumentBuildOptions build_options
	{
		llvm::DataLayout( GetTestsDataLayout() ),
		llvm::Triple( llvm::sys::getDefaultTargetTriple() ),
		"",
	};

	const llvm::StringRef features;
	const llvm::StringRef cpu_name;
	const char optimization_level= '0';
	const bool generate_debug_info= 0;
	const uint32_t compiler_generation= 0;
	build_options.prelude=
		GenerateCompilerPreludeCode(
			build_options.target_triple,
			build_options.data_layout,
			features,
			cpu_name,
			optimization_level,
			generate_debug_info,
			compiler_generation );

	return build_options;
}

using DocumentsContainer= std::map<IVfs::Path, Document*>;

class TestVfs final : public IVfs
{
public:
	explicit TestVfs( DocumentsContainer& documents )
		: documents_(documents)
	{}

	virtual std::optional<FileContent> LoadFileContent( const Path& full_file_path ) override
	{
		if( const auto it= documents_.find( full_file_path ); it != documents_.end() )
			return it->second->GetTextForCompilation();

		return std::nullopt;
	}

	virtual Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path ) override
	{
		U_UNUSED(full_parent_file_path);
		return file_path;
	}

private:
	DocumentsContainer& documents_;
};

Logger g_tests_logger( std::cout );
llvm::ThreadPool g_tests_thread_pool;

U_TEST( DocumentSetText_Test0 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);

	const IVfs::Path path= "test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	U_TEST_ASSERT( document.GetCurrentText() == "" );
	U_TEST_ASSERT( document.GetTextForCompilation() == "" );

	document.SetText( "auto x= 0;" );
	U_TEST_ASSERT( document.GetCurrentText() == "auto x= 0;" );
	U_TEST_ASSERT( document.GetTextForCompilation() == "auto x= 0;" );

	document.SetText( "wtf" );
	U_TEST_ASSERT( document.GetCurrentText() == "wtf" );
	U_TEST_ASSERT( document.GetTextForCompilation() == "wtf" );

	document.SetText( "66\n77" );
	U_TEST_ASSERT( document.GetCurrentText() == "66\n77" );
	U_TEST_ASSERT( document.GetTextForCompilation() == "66\n77" );

	document.SetText( "" );
	U_TEST_ASSERT( document.GetCurrentText() == "" );
	U_TEST_ASSERT( document.GetTextForCompilation() == "" );
}

U_TEST( DocumentUpdateText_Test0 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);

	const IVfs::Path path= "test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "auto x= 0;\nauto y= x;" );
	U_TEST_ASSERT( document.GetCurrentText() == "auto x= 0;\nauto y= x;" );

	// Add range.
	document.UpdateText( DocumentRange{ { 1, 10 }, { 1, 10 } }, " struct S{}" );
	U_TEST_ASSERT( document.GetCurrentText() == "auto x= 0; struct S{}\nauto y= x;" );

	// Replace range.
	document.UpdateText( DocumentRange{ { 2, 5 }, { 2, 6 } }, "new_y" );
	U_TEST_ASSERT( document.GetCurrentText() == "auto x= 0; struct S{}\nauto new_y= x;" );

	// Remove range with newline inside.
	document.UpdateText( DocumentRange{ { 1, 10 }, { 2, 5 } }, "" );
	U_TEST_ASSERT( document.GetCurrentText() == "auto x= 0;new_y= x;" );

	// Add range with newlines.
	document.UpdateText( DocumentRange{ { 1, 5 }, { 1, 19 } }, "new_auto= 42;\nstruct NewStruct{}\nenum E{A}" );
	U_TEST_ASSERT( document.GetCurrentText() == "auto new_auto= 42;\nstruct NewStruct{}\nenum E{A}" );

	// Add something at start.
	document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "broken_syntax " );
	U_TEST_ASSERT( document.GetCurrentText() == "broken_syntax auto new_auto= 42;\nstruct NewStruct{}\nenum E{A}" );

	// Add something at end.
	document.UpdateText( DocumentRange{ { 3, 9 }, { 3, 9 } }, " var i32 t= 0;" );
	U_TEST_ASSERT( document.GetCurrentText() == "broken_syntax auto new_auto= 42;\nstruct NewStruct{}\nenum E{A} var i32 t= 0;" );

	// Remove something at start.
	document.UpdateText( DocumentRange{ { 1, 0 }, { 2, 0 } }, "" );
	U_TEST_ASSERT( document.GetCurrentText() == "struct NewStruct{}\nenum E{A} var i32 t= 0;" );

	// Remove something at end.
	document.UpdateText( DocumentRange{ { 1, 0 }, { 2, 23 } }, "" );
	U_TEST_ASSERT( document.GetCurrentText() == "" );
}

U_TEST( DocumentRebuild_Test0 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	U_TEST_ASSERT( document.RebuildFinished() == false );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();
	U_TEST_ASSERT( document.RebuildFinished() == true );
	document.ResetRebuildFinishedFlag();
	U_TEST_ASSERT( document.RebuildFinished() == false );
}

U_TEST( DocumentRebuild_Test1 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "auto x= 0;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();
	U_TEST_ASSERT( document.RebuildFinished() == true );
	U_TEST_ASSERT( document.GetTextForCompilation() == "auto x= 0;" );

	// After rebuild text for compilation is taken from last rebuild result.
	document.SetText( "type F= f32;" );
	U_TEST_ASSERT( document.GetCurrentText() == "type F= f32;" );
	U_TEST_ASSERT( document.GetTextForCompilation() == "auto x= 0;" );

	// After new succsessful rebuild text for compilation is chanhed.
	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();
	U_TEST_ASSERT( document.RebuildFinished() == true );
	U_TEST_ASSERT( document.GetCurrentText() == "type F= f32;" );
	U_TEST_ASSERT( document.GetTextForCompilation() == "type F= f32;" );
}

U_TEST( DocumentRebuild_Test2 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "auto x= 0;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();
	U_TEST_ASSERT( document.RebuildFinished() == true );
	document.ResetRebuildFinishedFlag();
	U_TEST_ASSERT( document.GetTextForCompilation() == "auto x= 0;" );

	// In case of broken syntax compiled state with last valid text is preserved.
	document.SetText( "wrong syntax" );
	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();
	U_TEST_ASSERT( document.RebuildFinished() == true );
	U_TEST_ASSERT( document.GetCurrentText() == "wrong syntax" );
	U_TEST_ASSERT( document.GetTextForCompilation() == "auto x= 0;" );
}

} // namespace

} // namespace LangServer

} // namespace U
