#include "../../tests/py_tests_common.hpp"

#include "../code_builder_lib/code_builder.hpp"
#include "../../code_builder_lib_common/long_stable_hash.hpp"
#include "../lex_synt_lib/lexical_analyzer.hpp"
#include "../lex_synt_lib/syntax_analyzer.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"
#include "../../tests/tests_common.hpp"

namespace U
{

class SingeFileVfs final : public IVfs
{
public:

	SingeFileVfs( std::string file_path, const char* const text )
		: file_path_(file_path), file_text_(text)
	{}

	virtual std::optional<FileContent> LoadFileContent( const Path& full_file_path ) override
	{
		if( full_file_path == file_path_ )
			return file_text_;
		return std::nullopt;
	}

	virtual Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path ) override
	{
		U_UNUSED(full_parent_file_path);
		return file_path;
	}

	virtual bool IsImportingFileAllowed( const Path& full_file_path ) override
	{
		(void) full_file_path;
		return true;
	}

	virtual bool IsFileFromSourcesDirectory( const Path& full_file_path ) override
	{
		(void) full_file_path;
		return true;
	}

private:
	const std::string file_path_;
	const char* const file_text_;
};

void PrintLexSyntErrors( const SourceGraph& source_graph )
{
	std::vector<std::string> source_files;
	source_files.reserve( source_graph.nodes_storage.size() );
	for( const auto& node : source_graph.nodes_storage )
		source_files.push_back( node.file_path );

	PrintLexSyntErrors( source_files, source_graph.errors );
}

CodeBuilder::BuildResult RunCodeBuilder( const bool enable_unused_name_errors, IVfsSharedPtr vfs, SourceGraph source_graph )
{
	CodeBuilderOptions options;
	options.build_debug_info= true;
	options.generate_tbaa_metadata= true;
	options.report_about_unused_names= enable_unused_name_errors;

	return
		CodeBuilder::BuildProgram(
			*g_llvm_context,
			llvm::DataLayout( GetTestsDataLayout() ),
			GetTestsTargetTriple(),
			options,
			std::make_shared<SourceGraph>( std::move(source_graph) ),
			std::move(vfs) );
}

std::unique_ptr<llvm::Module> BuildProgramImpl( const char* const text, const bool enable_unsed_name_errors )
{
	const std::string file_path= "_";
	const auto vfs= std::make_shared<SingeFileVfs>( file_path, text );
	SourceGraph source_graph= LoadSourceGraph( *vfs, CalculateLongStableHash, file_path );

	PrintLexSyntErrors( source_graph );

	if( !source_graph.errors.empty() )
		return nullptr;

	CodeBuilder::BuildResult build_result= RunCodeBuilder( enable_unsed_name_errors, vfs, std::move(source_graph) );

	for( const CodeBuilderError& error : build_result.errors )
		std::cerr << error.src_loc.GetLine() << ":" << error.src_loc.GetColumn() << " " << error.text << "\n";

	if( !build_result.errors.empty() )
		return nullptr;

	return std::move(build_result.module);
}

PyObject* BuildSrcLoc( const SrcLoc& src_loc )
{
	PyObject* const dict= PyDict_New();
	PyDict_SetItemString( dict, "file_index", PyLong_FromLongLong( src_loc.GetFileIndex() ) );
	PyDict_SetItemString( dict, "line", PyLong_FromLongLong( src_loc.GetLine() ) );
	PyDict_SetItemString( dict, "column", PyLong_FromLongLong( src_loc.GetColumn() ) );
	return dict;
}

PyObject* BuildString( const std::string_view str )
{
	return PyUnicode_DecodeUTF8( str.data(), Py_ssize_t(str.size()), nullptr );
}

PyObject* BuildErrorsList( const CodeBuilderErrorsContainer& errors )
{
	PyObject* const list= PyList_New(0);

	for( const CodeBuilderError& error : errors )
	{
		PyObject* const dict= PyDict_New();

		PyDict_SetItemString( dict, "src_loc", BuildSrcLoc( error.src_loc ) );
		PyDict_SetItemString( dict, "code", BuildString( CodeBuilderErrorCodeToString( error.code ) ) );
		PyDict_SetItemString( dict, "text", BuildString( error.text ) );

		if( error.template_context != nullptr )
		{
			PyObject* const template_context_dict= PyDict_New();

			PyDict_SetItemString( template_context_dict, "errors", BuildErrorsList( error.template_context->errors ) );
			PyDict_SetItemString( template_context_dict, "src_loc", BuildSrcLoc( error.template_context->context_declaration_src_loc ) );
			PyDict_SetItemString( template_context_dict, "template_name", BuildString( error.template_context->context_name ) );
			PyDict_SetItemString( template_context_dict, "parameters_description", BuildString( error.template_context->parameters_description ) );

			PyDict_SetItemString( dict, "template_context", template_context_dict );
		}

		PyList_Append( list, dict );
	}

	return list;
}

PyObject* BuildProgramWithErrors( PyObject* const self, PyObject* const args )
{
	U_UNUSED(self);

	PyObject* prorgam_text_arg= nullptr;
	PyObject* enable_unused_variable_errors_arg= nullptr;

	if( !PyArg_UnpackTuple( args, "", 1, 2, &prorgam_text_arg, &enable_unused_variable_errors_arg ) )
		return nullptr; // Parse will raise

	const char* program_text= nullptr;
	if( !PyArg_Parse( prorgam_text_arg, "s", &program_text ) )
		return nullptr; // Parse will raise

	int enable_unused_variable_errors= 0;
	if( enable_unused_variable_errors_arg != nullptr && !PyArg_Parse( enable_unused_variable_errors_arg, "p", &enable_unused_variable_errors ) )
		return nullptr; // Parse will raise

	const std::string file_path= "_";
	const auto vfs= std::make_shared<SingeFileVfs>( file_path, program_text );
	SourceGraph source_graph= LoadSourceGraph( *vfs, CalculateLongStableHash, file_path );

	PrintLexSyntErrors( source_graph );

	if( !source_graph.errors.empty() )
	{
		PyErr_SetString( PyExc_RuntimeError, "source tree build failed" );
		return nullptr;
	}

	PyObject* const list= BuildErrorsList( RunCodeBuilder( enable_unused_variable_errors != 0, vfs, std::move(source_graph) ).errors );
	llvm::llvm_shutdown();

	return list;
}

PyObject* BuildProgramWithSyntaxErrors( PyObject* const self, PyObject* const args )
{
	U_UNUSED(self);

	const char* program_text= nullptr;

	if( !PyArg_ParseTuple( args, "s", &program_text ) )
		return nullptr;

	const std::string file_path= "_";

	SingeFileVfs vfs( file_path, program_text );
	const SourceGraph source_graph= LoadSourceGraph( vfs, CalculateLongStableHash, file_path );

	std::vector<CodeBuilderError> errors_converted;
	errors_converted.reserve( source_graph.errors.size() );
	for( const LexSyntError& error_message : source_graph.errors )
	{
		CodeBuilderError error_converted;
		error_converted.code= CodeBuilderErrorCode::BuildFailed;
		error_converted.src_loc= error_message.src_loc;
		error_converted.text= error_message.text;
		errors_converted.push_back( std::move(error_converted) );
	}

	return BuildErrorsList( errors_converted );
}

bool FilterTestImpl( const char* const test_name )
{
	const std::string test_name_str= test_name;

	static const std::unordered_set<std::string> c_test_to_disable
	{
	};

	return c_test_to_disable.count( test_name_str ) > 0;
}

} // namespace U
