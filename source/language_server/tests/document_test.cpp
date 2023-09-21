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

using CompletionItemsNormalized= std::vector<std::string>;

CompletionItemsNormalized NormalizeCompletionResult( const llvm::ArrayRef<CompletionItem> items )
{
	std::vector items_copy= items.vec();
	std::sort(
		items_copy.begin(),
		items_copy.end(),
		[]( const CompletionItem& l, const CompletionItem& r ) { return l.sort_text < r.sort_text; } );

	CompletionItemsNormalized result;
	result.reserve( items.size() );
	for( const CompletionItem& item : items_copy )
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

	std::sort( result.begin(), result.end() );
	return result;
}

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

U_TEST( DocumentCompletion_Test0 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	document.SetText( " struct S{ i32 field0; i32 field1; i32 other_field; f32 rr; type ll= bool; fn some_func(); type typef= i32; type ftype= i32; struct Inner_f{} } var S s= zero_init;" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	{
		document.UpdateText( DocumentRange{ { 1, 0 }, { 1, 0 } }, "auto x= s.f" );
		U_TEST_ASSERT( document.GetCurrentText() == "auto x= s.f struct S{ i32 field0; i32 field1; i32 other_field; f32 rr; type ll= bool; fn some_func(); type typef= i32; type ftype= i32; struct Inner_f{} } var S s= zero_init;" );

		// With name after "." items are filedered.
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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

U_TEST( DocumentSignatureHelp_Test0 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
	Document document( path, GetTestDocumentBuildOptions(), vfs, g_tests_logger );
	documents[path]= &document;

	// Should properly suggest signature of method call.
	document.SetText( "fn bar(S& s){} struct S{ fn foo( imut this'x' ) : bool; i32& ref_field; }" );

	document.StartRebuild( g_tests_thread_pool );
	document.WaitUntilRebuildFinished();

	document.UpdateText( DocumentRange{ { 1, 13 }, { 1, 13 } }, "s.foo(" );

	const auto result= document.GetSignatureHelp( DocumentPosition{ 1, 19 } );
	const SignatureHelpResultNormalized expected_result{ "foo( imut this'x' ) : bool" };
	U_TEST_ASSERT( NormalizeSignatureHelpResult( result ) == expected_result );
}

U_TEST( DocumentSignatureHelp_Test7 )
{
	DocumentsContainer documents;
	TestVfs vfs(documents);
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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
	const IVfs::Path path= "test.u";
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

} // namespace

} // namespace LangServer

} // namespace U
