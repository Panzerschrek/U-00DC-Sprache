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
		if( !file_path.empty() && file_path.front() == '/' )
			return file_path;
		return "/" + file_path;
	}

private:
	DocumentsContainer& documents_;
};

using CompletionItemsNormalized= std::vector<std::string>;

CompletionItemsNormalized NormalizeCompletionResult( const llvm::ArrayRef<CompletionItem> items )
{
	CompletionItemsNormalized result;
	result.reserve( items.size() );

	for( const CompletionItem& item : items )
		result.push_back( item.label );

	return result;
}

using SignatureHelpResultNormalized= std::vector<std::string>;

SignatureHelpResultNormalized NormalizeSignatureHelpResult( const llvm::ArrayRef<CodeBuilder::SignatureHelpItem> items )
{
	SignatureHelpResultNormalized result;
	result.reserve( items.size() );

	for( const CodeBuilder::SignatureHelpItem& item : items )
		result.push_back( item.label );

	return result;
}

Logger g_tests_logger( std::cout );
llvm::ThreadPool g_tests_thread_pool;

U_TEST( DocumentSetText_Test0 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);

	const IVfs::Path path= "/test.u";
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

	const IVfs::Path path= "/test.u";
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
	const IVfs::Path path= "/test.u";
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
	const IVfs::Path path= "/test.u";
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
	const IVfs::Path path= "/test.u";
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

U_TEST( DocumentCompletion_Test0 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "auto xyz= 0;\nauto qwerty= 0;\nvar i33 xrt= 0;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	{
		document.UpdateText( DocumentRange{ { 1, 12 }, { 1, 12 } }, "auto w=x" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto xyz= 0;auto w=x\nauto qwerty= 0;\nvar i33 xrt= 0;" );

		// Complete starts with "x"
		const auto completion_result= document.Complete( DocumentPosition{ 1, 20 } );
		const CompletionItemsNormalized expected_completion_result{ "xrt", "xyz" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		document.UpdateText( DocumentRange{ { 1, 12 }, { 1, 20 } }, "auto w=q" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto xyz= 0;auto w=q\nauto qwerty= 0;\nvar i33 xrt= 0;" );

		// Complete starts with "q"
		const auto completion_result= document.Complete( DocumentPosition{ 1, 20 } );
		const CompletionItemsNormalized expected_completion_result{ "qwerty" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
}

U_TEST( DocumentCompletion_Test1 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Completion with search inside the word.
	document.SetText( " type PerfectCode= i32; type CodeUnit= f32; type Cod= bool; auto CodeCompletion= 0; var i32 ThisCodeIsWrong= 0;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "type w=Code" );
	U_TEST_ASSERT( document.GetCurrentText() == "type w=Code type PerfectCode= i32; type CodeUnit= f32; type Cod= bool; auto CodeCompletion= 0; var i32 ThisCodeIsWrong= 0;" );

	// Words starting with "Code" are suggested first. Than words with "Code" inside.
	const auto completion_result= document.Complete( DocumentPosition{ 1, 11 } );
	const CompletionItemsNormalized expected_completion_result{ "CodeCompletion", "CodeUnit", "PerfectCode", "ThisCodeIsWrong" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test2 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Completion is case-insensetive.
	document.SetText( " auto ab= 0; auto Ab= 0; auto QAB= 0; auto TaBs= 0;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "auto x= ab" );
	U_TEST_ASSERT( document.GetCurrentText() == "auto x= ab auto ab= 0; auto Ab= 0; auto QAB= 0; auto TaBs= 0;" );

	const auto completion_result= document.Complete( DocumentPosition{ 1, 10 } );
	const CompletionItemsNormalized expected_completion_result{ "Ab", "ab", "QAB", "TaBs" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test3 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Completion returns nothing.
	document.SetText( " auto qwerty= 0;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "auto x= ab" );
	U_TEST_ASSERT( document.GetCurrentText() == "auto x= ab auto qwerty= 0;" );

	const auto completion_result= document.Complete( DocumentPosition{ 1, 10 } );
	const CompletionItemsNormalized expected_completion_result{};
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test4 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( " auto var3= 0; namespace NN{ auto var4= 0; } auto var5= 0;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	{
		document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "auto x= var" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto x= var auto var3= 0; namespace NN{ auto var4= 0; } auto var5= 0;" );

		// Completion returns only items visible within completion point namespace.
		// Items from inner namespace are not visible.
		const auto completion_result= document.Complete( DocumentPosition{ 1, 11 } );
		const CompletionItemsNormalized expected_completion_result{ "var3", "var5" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 12 } }, "" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto var3= 0; namespace NN{ auto var4= 0; } auto var5= 0;" );
		document.UpdateText( DocumentRange{ { 1, 27 }, { 1, 27 } }, "auto x= var" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto var3= 0; namespace NN{auto x= var auto var4= 0; } auto var5= 0;" );

		// Now "var4" from inner namespace is visible.
		const auto completion_result= document.Complete( DocumentPosition{ 1, 38 } );
		const CompletionItemsNormalized expected_completion_result{ "var3", "var4", "var5" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
}

U_TEST( DocumentCompletion_Test5 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "struct S{ i32 field; fn Foo(); } fn S::Foo(){}" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 45 }, { 1, 45 } }, "fi" );
	U_TEST_ASSERT( document.GetCurrentText() == "struct S{ i32 field; fn Foo(); } fn S::Foo(){fi}" );

	// Class field is visible from out-of-line method.
	const auto completion_result= document.Complete( DocumentPosition{ 1, 47 } );
	const CompletionItemsNormalized expected_completion_result{ "field" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test6 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( " struct S{ type GHMQ= i32; fn Foo(); i32 lol; }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Access class memebers via "::".
	{
		document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "auto x=S::GH" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto x=S::GH struct S{ type GHMQ= i32; fn Foo(); i32 lol; }" );

		const auto completion_result= document.Complete( DocumentPosition{ 1, 12 } );
		const CompletionItemsNormalized expected_completion_result{ "GHMQ" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 12 } }, "auto x=S::oo" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto x=S::oo struct S{ type GHMQ= i32; fn Foo(); i32 lol; }" );

		const auto completion_result= document.Complete( DocumentPosition{ 1, 12 } );
		const CompletionItemsNormalized expected_completion_result{ "Foo" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 12 } }, "auto x=S::LO" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto x=S::LO struct S{ type GHMQ= i32; fn Foo(); i32 lol; }" );

		const auto completion_result= document.Complete( DocumentPosition{ 1, 12 } );
		const CompletionItemsNormalized expected_completion_result{ "lol" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 12 } }, "auto x=S::con" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto x=S::con struct S{ type GHMQ= i32; fn Foo(); i32 lol; }" );

		// "constructor" name is implicitely created.
		const auto completion_result= document.Complete( DocumentPosition{ 1, 13 } );
		const CompletionItemsNormalized expected_completion_result{ "constructor" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 13 } }, "auto x=S::des" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto x=S::des struct S{ type GHMQ= i32; fn Foo(); i32 lol; }" );

		// "destructor" name is implicitely created.
		const auto completion_result= document.Complete( DocumentPosition{ 1, 13 } );
		const CompletionItemsNormalized expected_completion_result{ "destructor" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
}

U_TEST( DocumentCompletion_Test7 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( " namespace Abc{ auto x= 0; type Tt= f32; fn Qwerty(); enum EE{A, B, C} var i32 el= 0; }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	{
		// Via single "::" (with empty name) all namespace members are accessible.
		document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "type t= Abc::" );
		U_TEST_ASSERT( document.GetCurrentText() == "type t= Abc:: namespace Abc{ auto x= 0; type Tt= f32; fn Qwerty(); enum EE{A, B, C} var i32 el= 0; }" );

		const auto completion_result= document.Complete( DocumentPosition{ 1, 13 } );
		const CompletionItemsNormalized expected_completion_result{ "EE", "Qwerty", "Tt", "el", "x" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		// Via "::" with non-empty name accessible only matched elements.
		document.UpdateText( DocumentRange{ { 1, 13 }, { 1, 13 } }, "e" );
		U_TEST_ASSERT( document.GetCurrentText() == "type t= Abc::e namespace Abc{ auto x= 0; type Tt= f32; fn Qwerty(); enum EE{A, B, C} var i32 el= 0; }" );

		const auto completion_result= document.Complete( DocumentPosition{ 1, 14 } );
		const CompletionItemsNormalized expected_completion_result{ "EE", "el", "Qwerty" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
}

U_TEST( DocumentCompletion_Test8 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( " struct S{ i32 field0; i32 field1; i32 other_field; f32 rr; type ll= bool; fn some_func(); type typef= i32; type ftype= i32; struct Inner_f{} } var S s= zero_init;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	{
		document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "auto x= s.f" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto x= s.f struct S{ i32 field0; i32 field1; i32 other_field; f32 rr; type ll= bool; fn some_func(); type typef= i32; type ftype= i32; struct Inner_f{} } var S s= zero_init;" );

		// With name after "." items are filled.
		const auto completion_result= document.Complete( DocumentPosition{ 1, 11 } );
		const CompletionItemsNormalized expected_completion_result{ "field0", "field1", "ftype", "Inner_f", "other_field", "some_func", "typef" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		document.UpdateText( DocumentRange{ { 1, 10 }, { 1, 11 } }, "" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto x= s. struct S{ i32 field0; i32 field1; i32 other_field; f32 rr; type ll= bool; fn some_func(); type typef= i32; type ftype= i32; struct Inner_f{} } var S s= zero_init;" );

		// With no name after "." all class internals are provided.
		const auto completion_result= document.Complete( DocumentPosition{ 1, 10 } );
		const CompletionItemsNormalized expected_completion_result{ "Inner_f", "constructor", "destructor", "field0", "field1", "ftype", "ll", "other_field", "rr", "some_func", "typef" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
}

U_TEST( DocumentCompletion_Test9 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( " struct S{ op++(S &mut s); op()(this); op-(S& s) : S; } var S s= zero_init;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Overloaded operators are ignored via "." access.
	document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "auto x= s." );
	U_TEST_ASSERT( document.GetCurrentText() == "auto x= s. struct S{ op++(S &mut s); op()(this); op-(S& s) : S; } var S s= zero_init;" );

	const auto completion_result= document.Complete( DocumentPosition{ 1, 10 } );
	const CompletionItemsNormalized expected_completion_result{ "constructor", "destructor" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test10 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Completion inside function. Later defined variables and variables in outer scope are not visible.

	document.SetText( R"(
fn Foo()
{
	auto var0= 0; // visible
	{
		auto var1= 0; // not visible
	}
	{
		auto var2= 0; // visible
		{
			auto var3= 0; // visible
// Complete here.
			auto var4= 0; // not visible
		}
		auto var5= 0; // not visible
	}
	auto var6= 0; // not visible
}
)" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 12, 0 }, { 12, 0 } }, "auto x= va" );

	const auto completion_result= document.Complete( DocumentPosition{ 12, 10 } );
	const CompletionItemsNormalized expected_completion_result{ "var0", "var2", "var3" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test11 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "namespace NN{  } type CustomType= i32; fn Foo();" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	{
		// Access via "::" provides all global names, including built-in types.
		document.UpdateText( DocumentRange{ { 1, 14 }, { 1, 14 } }, "type t= ::" );

		const auto completion_result= document.Complete( DocumentPosition{ 1, 24 } );
		const CompletionItemsNormalized expected_completion_result
		{
			"CustomType",
			"Foo",
			"NN",
			"bool",
			"byte128", "byte16", "byte32", "byte64", "byte8",
			"char16", "char32", "char8",
			"compiler", // name from the prelude
			"f32", "f64",
			"i128", "i16", "i32", "i64", "i8",
			"size_type",
			"u128", "u16", "u32", "u64", "u8",
			"void"
		};
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		// Access via "::" with name provides matching globals.
		document.UpdateText( DocumentRange{ { 1, 14 }, { 1, 24 } }, "type t= ::byte" );

		const auto completion_result= document.Complete( DocumentPosition{ 1, 28 } );
		const CompletionItemsNormalized expected_completion_result{ "byte128", "byte16", "byte32", "byte64", "byte8", };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
}

U_TEST( DocumentCompletion_Test12 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Completion inside function inside struct inside struct inside a couple of namespaces.

	document.SetText( R"(
namespace Abc
{
	namespace Unvisible0{ auto lol_5= 0; }
	namespace Def
	{
		namespace Unvisible1{ auto lol_6= 0; }
		struct Ghi
		{
			struct Unvisible2{ auto lol_7= 0; }
			struct Klm
			{
				struct Unvisible3{ auto lol_8= 0; }
				fn Nop()
				{
// Complete here
				}
				auto lol_0= 0;
			}
			auto lol_1= 0;
		}
		auto lol_2= 0;
	}
	auto lol_3= 0;
}
auto lol_4= 0;
)" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 16, 0 }, { 16, 0 } }, "auto x= lol" );

	const auto completion_result= document.Complete( DocumentPosition{ 16, 11 } );
	const CompletionItemsNormalized expected_completion_result{ "lol_0", "lol_1", "lol_2", "lol_3", "lol_4" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test13 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "fn foo(i32 mut qwerty){  }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Complete name in "move".
	document.UpdateText( DocumentRange{ { 1, 24 }, { 1, 24 } }, "move( q" );

	const auto completion_result= document.Complete( DocumentPosition{ 1, 31 } );
	const CompletionItemsNormalized expected_completion_result{ "qwerty" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test14 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( " struct S{ i32 a; i32 b; i32 wtf; fn Foo(); type T= i32; }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Complete name in "struct named initializer". Only fields are suggested.
	document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "var S s { ." );

	const auto completion_result= document.Complete( DocumentPosition{ 1, 11 } );
	const CompletionItemsNormalized expected_completion_result{ "a", "b", "wtf" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test15 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "struct S{  i32 field0; i32 field1; i32 other_field; i32 lol; }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Complete name in constructor initializer list. Only fields are suggested.
	document.UpdateText( DocumentRange{ { 1, 10 }, { 1, 10 } }, "fn constructor() ( f ) {}" );

	const auto completion_result= document.Complete( DocumentPosition{ 1, 30 } );
	const CompletionItemsNormalized expected_completion_result{ "field0", "field1", "other_field" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test16 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Complete in complex function name.

	document.SetText( R"(
namespace Abc
{
	namespace Def
	{
		fn Foo();
	}
}
// Complete here
)" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	{
		document.UpdateText( DocumentRange{ { 9, 0 }, { 9, 0 } }, "fn Ab" );
		const auto completion_result= document.Complete( DocumentPosition{ 9, 5 } );
		const CompletionItemsNormalized expected_completion_result{ "Abc" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		document.UpdateText( DocumentRange{ { 9, 5 }, { 9, 5 } }, "c::" );
		const auto completion_result= document.Complete( DocumentPosition{ 9, 8 } );
		const CompletionItemsNormalized expected_completion_result{ "Def" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		document.UpdateText( DocumentRange{ { 9, 8 }, { 9, 8 } }, "d" );
		const auto completion_result= document.Complete( DocumentPosition{ 9, 9 } );
		const CompletionItemsNormalized expected_completion_result{ "Def" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		document.UpdateText( DocumentRange{ { 9, 8 }, { 9, 9 } }, "Def::" );
		const auto completion_result= document.Complete( DocumentPosition{ 9, 13 } );
		const CompletionItemsNormalized expected_completion_result{ "Foo" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
	{
		document.UpdateText( DocumentRange{ { 9, 13 }, { 9, 13 } }, "oo" );
		const auto completion_result= document.Complete( DocumentPosition{ 9, 15 } );
		const CompletionItemsNormalized expected_completion_result{ "Foo" };
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
}

U_TEST( DocumentCompletion_Test17 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "fn foo(){ auto mut lol= Some::A; } enum Some{ A, B, C } " );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Complete name in assignment operator suggestion.
	document.UpdateText( DocumentRange{ { 1, 32 }, { 1, 32 } }, "lol= Som" );

	const auto completion_result= document.Complete( DocumentPosition{ 1, 40 } );
	const CompletionItemsNormalized expected_completion_result{ "Some" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test18 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Complete inside function template.
	document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "template</ type T /> fn Foo( T t_param ){par}" );

	const auto completion_result= document.Complete( DocumentPosition{ 1, 44 } );
	const CompletionItemsNormalized expected_completion_result{ "t_param" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test19 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "template</ type Qwerty /> struct Box{ }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Complete inside struct template.
	document.UpdateText( DocumentRange{ { 1, 37 }, { 1, 37 } }, "Qw" );

	const auto completion_result= document.Complete( DocumentPosition{ 1, 39 } );
	const CompletionItemsNormalized expected_completion_result{ "Qwerty" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test20 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Complete inside struct template with whole type template added after compilation.
	document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "template</ type Qwerty /> struct Box{we }" );

	const auto completion_result= document.Complete( DocumentPosition{ 1, 39 } );
	const CompletionItemsNormalized expected_completion_result{ "Qwerty" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test21 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "template</ type Qwerty /> struct Box {}" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Complete inside struct template, but outside struct itslef (inside non-sync tag).
	document.UpdateText( DocumentRange{ { 1, 37 }, { 1, 37 } }, "non_sync( Qw )" );

	const auto completion_result= document.Complete( DocumentPosition{ 1, 49 } );
	const CompletionItemsNormalized expected_completion_result{ "Qwerty" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test22 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Complete inside type template.
	document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "template</ type Qwerty /> type Arr= [ Q " );

	const auto completion_result= document.Complete( DocumentPosition{ 1, 39 } );
	const CompletionItemsNormalized expected_completion_result{ "Qwerty" };
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test23 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "template</ type T, T value_arg /> struct Box</ value_arg /> {}" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Should properly handle template with complex signature.
	document.UpdateText( DocumentRange{ { 1, 61 }, { 1, 61 } }, "auto x= arg" );

	const CompletionItemsNormalized expected_completion_result{ "value_arg" };
	for( size_t i= 0; i < 2; ++i )
	{
		const auto completion_result= document.Complete( DocumentPosition{ 1, 72 } );
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
}

U_TEST( DocumentCompletion_Test24 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "template</ type T, size_type S /> fn foo( [ T, S ]& arr_arg ) {} " );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Should properly handle template with complex signature.
	document.UpdateText( DocumentRange{ { 1, 63 }, { 1, 63 } }, "arr" );

	const CompletionItemsNormalized expected_completion_result{ "arr_arg" };
	for( size_t i= 0; i < 2; ++i )
	{
		// Should complete properly more then once.
		const auto completion_result= document.Complete( DocumentPosition{ 1, 66 } );
		U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
	}
}

U_TEST( DocumentCompletion_Test25 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "fn Foo() { var i32 external_variable= 0; auto f = lambda[&](){  }; }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Should suggest external variable in lambda body.
	document.UpdateText( DocumentRange{ { 1, 63 }, { 1, 63 } }, "auto c= ext" );

	const CompletionItemsNormalized expected_completion_result{ "external_variable" };

	const auto completion_result= document.Complete( DocumentPosition{ 1, 74 } );
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test26 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "fn Foo() { var i32 external_variable= 0; auto f = lambda[](){}; }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Should suggest external variable in lambda capture list.
	document.UpdateText( DocumentRange{ { 1, 57 }, { 1, 57 } }, "nal" );

	const CompletionItemsNormalized expected_completion_result{ "external_variable" };

	const auto completion_result= document.Complete( DocumentPosition{ 1, 60 } );
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test27 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "fn Foo() { var i32 external_variable= 0; auto f = lambda[](){}; }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Should suggest external variable in lambda capture list expression.
	document.UpdateText( DocumentRange{ { 1, 57 }, { 1, 57 } }, "x= terna" );

	const CompletionItemsNormalized expected_completion_result{ "external_variable" };

	const auto completion_result= document.Complete( DocumentPosition{ 1, 65 } );
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test28 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "struct S{ i32 some; } fn Foo(){  }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Should suggest member name in struct initializer in expression context.
	document.UpdateText( DocumentRange{ { 1, 32 }, { 1, 32 } }, "auto s= S{ ." );

	const CompletionItemsNormalized expected_completion_result{ "some" };

	const auto completion_result= document.Complete( DocumentPosition{ 1, 44 } );
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test29 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "struct Some{} fn Foo(){  }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Should suggest type name in struct initializer in expression context.
	document.UpdateText( DocumentRange{ { 1, 24 }, { 1, 24 } }, "auto s= So" );

	const CompletionItemsNormalized expected_completion_result{ "Some" };

	const auto completion_result= document.Complete( DocumentPosition{ 1, 34 } );
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentCompletion_Test30 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "struct S{ i32 some; } fn Foo(){  }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Should suggest field name in member access after struct initializer in expression context.
	document.UpdateText( DocumentRange{ { 1, 32 }, { 1, 32 } }, "auto s= S{}.ome" );

	const CompletionItemsNormalized expected_completion_result{ "some" };

	const auto completion_result= document.Complete( DocumentPosition{ 1, 47 } );
	U_TEST_ASSERT( NormalizeCompletionResult( completion_result ) == expected_completion_result );
}

U_TEST( DocumentSignatureHelp_Test0 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "fn bar(){} fn foo();" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "foo(" );
	U_TEST_ASSERT( document.GetCurrentText() == "fn bar(){foo(} fn foo();" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 13 } );
	const SignatureHelpResultNormalized expected_result{ "foo() : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test1 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "fn bar(){} fn foo( i32 x ); fn foo( f32 y, bool z ) : f64; fn foo( [ i32, 2 ] &mut a, tup[ f32, void ] t ) unsafe : i32 &mut;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "foo(" );
	U_TEST_ASSERT( document.GetCurrentText() == "fn bar(){foo(} fn foo( i32 x ); fn foo( f32 y, bool z ) : f64; fn foo( [ i32, 2 ] &mut a, tup[ f32, void ] t ) unsafe : i32 &mut;" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 13 } );
	const SignatureHelpResultNormalized expected_result{ "foo( [ i32, 2 ] &mut a, tup[ f32, void ] t ) unsafe : i32 &mut", "foo( f32 y, bool z ) : f64", "foo( i32 x ) : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test2 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Only last component of the function name is used.
	document.SetText( "fn bar(){} namespace Abc{ struct Str{ fn foo(); } }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "Abc::Str::foo(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 23 } );
	const SignatureHelpResultNormalized expected_result{ "foo() : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test3 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Type names as they are writen are used - without unwrapping type aliases and calculating variable values.
	document.SetText( "fn bar(){} fn foo( [ Int, size ] arr ) : Float; type Int= i64; auto size= 16s; type Float= f32;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "foo(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 13 } );
	const SignatureHelpResultNormalized expected_result{ "foo( [ Int, size ] arr ) : Float" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test4 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should properly suggest signature of method call.
	document.SetText( "fn bar(S& s){} struct S{ fn foo(this); }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 13 }, { 1, 13 } }, "s.foo(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 19 } );
	const SignatureHelpResultNormalized expected_result{ "foo( this ) : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test5 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should properly suggest signature of method call.
	document.SetText( "fn bar(S& s){} struct S{ fn foo( mut this, f32 x ); }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 13 }, { 1, 13 } }, "s.foo(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 19 } );
	const SignatureHelpResultNormalized expected_result{ "foo( mut this, f32 x ) : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test6 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should properly suggest signature of method call.
	document.SetText( "fn bar(S& s){} struct S{ fn foo( imut this ) : bool; i32& ref_field; }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 13 }, { 1, 13 } }, "s.foo(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 19 } );
	const SignatureHelpResultNormalized expected_result{ "foo( imut this ) : bool" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test7 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should properly suggest signature of static method call.
	document.SetText( "fn bar(S& s){} struct S{ fn foo(); fn foo( i32 x ); }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 13 }, { 1, 13 } }, "s.foo(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 19 } );
	const SignatureHelpResultNormalized expected_result{ "foo( i32 x ) : void", "foo() : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test8 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should properly suggest call to generated constructor.
	document.SetText( "fn bar(S &mut s){} struct S{}" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 17 }, { 1, 17 } }, "unsafe( s.constructor(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 39 } );
	const SignatureHelpResultNormalized expected_result{ "constructor( mut this ) : void", "constructor( mut this, S &imut other ) : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test9 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should properly suggest call to generated destructor.
	document.SetText( "fn bar(S &mut s){} struct S{}" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 17 }, { 1, 17 } }, "unsafe( s.destructor(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 38 } );
	const SignatureHelpResultNormalized expected_result{ "destructor( mut this ) : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test10 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should provide signature help for temp variable construction.
	document.SetText( "fn bar(){} struct S{ fn constructor()= default; fn constructor( i32 x ); }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "S(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 11 } );
	const SignatureHelpResultNormalized expected_result{ "constructor( mut this ) : void", "constructor( mut this, S &imut other ) : void", "constructor( mut this, i32 x ) : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test11 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should provide signature help for function pointer call.
	document.SetText( "fn bar(){} var (fn(i32 x, f32 & y ) : bool &mut ) ptr= zero_init;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "ptr(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 13 } );
	const SignatureHelpResultNormalized expected_result{ "fn( i32 _, f32 &imut _ ) : bool &mut" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test12 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should properly suggest call to overloaded operator ().
	document.SetText( "fn bar(S& s){} struct S{ op()( this, bool b ) : f32; }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 13 }, { 1, 13 } }, "s(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 15 } );
	const SignatureHelpResultNormalized expected_result{ "()( this, bool b ) : f32" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test13 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should provide signature help for variable construction.
	document.SetText( "fn bar(){} struct S{ fn constructor()= default; fn constructor( i32 x ); }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "var S s(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 17 } );
	const SignatureHelpResultNormalized expected_result{ "constructor( mut this ) : void", "constructor( mut this, S &imut other ) : void", "constructor( mut this, i32 x ) : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test14 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should provide signature help for ",", that is part of call operator.
	document.SetText( "fn bar(){} fn foo( i32 x, f32 y );" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "foo( 42," );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 17 } );
	const SignatureHelpResultNormalized expected_result{ "foo( i32 x, f32 y ) : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test15 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should provide signature help for"," in variable construction.
	document.SetText( "fn bar(){} class S{ fn constructor()= delete; fn constructor( f32 x, bool y ); }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "var S s( 0.5f," );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 23 } );
	const SignatureHelpResultNormalized expected_result{ "constructor( mut this, f32 x, bool y ) : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test16 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should provide signature help for ")" and return outer function.
	document.SetText( "fn bar(){} fn foo( i32 x, f32 y ); fn baz() : i32;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "foo( baz()" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 19 } );
	const SignatureHelpResultNormalized expected_result{ "foo( i32 x, f32 y ) : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test17 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should provide no signature help for ")" that terminates call operator.
	document.SetText( "fn bar(){} fn foo();" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "foo()" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 14 } );
	const SignatureHelpResultNormalized expected_result{ };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test18 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should suggest template function.
	document.SetText( "fn bar(){} template</ type T, size_type S /> fn foo( [ T, S ]& arr ){}" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "foo(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 13 } );
	const SignatureHelpResultNormalized expected_result{ "template</ type T, size_type S /> fn foo( [ T, S ] &arr ) : void" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test19 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should suggest template function with explicit args.
	document.SetText( "fn bar(){} template</ type T /> fn foo() : T { return T(0); }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 9 }, { 1, 9 } }, "foo</i32/>(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 20 } );
	const SignatureHelpResultNormalized expected_result{ "template</ type T /> fn foo() : T" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( Document_GetFileForImportPoint_Test0 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	const IVfs::Path imported_path= "/some.u";
	Document imported_document(  path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[ imported_path ]= &imported_document;

	document.SetText( "import \"some.u\" " );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();
	const auto result= document.GetFileForImportPoint( { 1, 10 } );
	const Uri expected_result= Uri::FromFilePath( imported_path );
	U_TEST_ASSERT( result == expected_result );
}

U_TEST( Document_GetFileForImportPoint_Test1 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	const IVfs::Path imported_path= "/some_global.u";
	Document imported_document( imported_path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[ imported_path ]= &imported_document;

	document.SetText( "import \"/some_global.u\" " );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Should properly handle file with "/" at start.
	const auto result= document.GetFileForImportPoint( { 1, 15 } );
	const Uri expected_result= Uri::FromFilePath( imported_path );
	U_TEST_ASSERT( result == expected_result );
}

U_TEST( Document_GetFileForImportPoint_Test2 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( "auto this_is_not_import= 0;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Should return none for non-import point.
	const auto result= document.GetFileForImportPoint( { 1, 15 } );
	U_TEST_ASSERT( result == std::nullopt );
}

U_TEST( Document_GetFileForImportPoint_Test3 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	const IVfs::Path imported_path= "/some.u";
	Document imported_document(  path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[ imported_path ]= &imported_document;

	document.SetText( "import \"some.u\"         " );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	// Should return none if click position is outside import string.
	U_TEST_ASSERT( document.GetFileForImportPoint( { 1,  2 } ) == std::nullopt );
	U_TEST_ASSERT( document.GetFileForImportPoint( { 1, 12 } ) == Uri::FromFilePath( imported_path ) );
	U_TEST_ASSERT( document.GetFileForImportPoint( { 1, 18 } ) == std::nullopt );
}

U_TEST( Document_GetFileForImportPoint_Test4 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "/test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	const IVfs::Path imported_path= "/file with spaces.u";
	Document imported_document(  path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[ imported_path ]= &imported_document;

	// Should properly handle whitespaces in import line and in import string.
	document.SetText( R"(
	  import   "file with spaces.u"
)" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	const auto result= document.GetFileForImportPoint( { 2, 22 } );
	const Uri expected_result= Uri::FromFilePath( imported_path );
	U_TEST_ASSERT( result == expected_result );
}

} // namespace

} // namespace LangServer

} // namespace U
