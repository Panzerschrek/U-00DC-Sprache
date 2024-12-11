#include "../../compilers_common_lib/code_builder_launcher.hpp"
#include "../imports/funcs_c.hpp"

namespace U
{

namespace
{

U1_StringView StringToStringView( const std::string_view s )
{
	return U1_StringView{ s.data(), s.size() };
}

const std::string StringViewToString( const U1_StringView view )
{
	return std::string( view.data, view.data + view.size );
}

void GetFullFilePath(
	const U1_UserHandle this_,
	const U1_StringView& file_path,
	const U1_StringView& parent_file_path_normalized,
	const  U1_IVfsInterface::FillStringCallback result_callback,
	const U1_UserHandle user_data )
{
	const std::string path=
		reinterpret_cast<IVfs*>(this_)->GetFullFilePath(
			StringViewToString(file_path),
			StringViewToString(parent_file_path_normalized) );

	result_callback( user_data, StringToStringView(path) );
}

bool LoadFileContent(
	const U1_UserHandle this_,
	const U1_StringView& path_normalized,
	const U1_IVfsInterface::FillStringCallback result_callback,
	const U1_UserHandle user_data )
{
	const std::optional<IVfs::FileContent> file_content=
		reinterpret_cast<IVfs*>(this_)->LoadFileContent( StringViewToString(path_normalized) );

	if( file_content == std::nullopt )
		return false;

	result_callback( user_data, StringToStringView( *file_content ) );
	return true;
}

bool IsImportingFileAllowed(
	const U1_UserHandle this_,
	const U1_StringView& path_normalized )
{
	return reinterpret_cast<IVfs*>(this_)->IsImportingFileAllowed( StringViewToString(path_normalized) );
}

bool IsFileFromSourcesDirectory(
	const U1_UserHandle this_,
	const U1_StringView& path_normalized )
{
	return reinterpret_cast<IVfs*>(this_)->IsFileFromSourcesDirectory( StringViewToString(path_normalized) );
}

U1_UserHandle ErrorHanlder(
	const U1_UserHandle data, // should be "CodeBuilderErrorsContainer"
	const uint32_t file_index,
	const uint32_t line,
	const uint32_t column,
	const uint32_t error_code,
	const U1_StringView& error_text )
{
	CodeBuilderError error;
	error.src_loc= SrcLoc( file_index, line, column );
	error.code= CodeBuilderErrorCode(error_code);
	error.text= StringViewToString( error_text );

	const auto errors_container= reinterpret_cast<CodeBuilderErrorsContainer*>(data);
	errors_container->push_back( std::move(error) );
	return reinterpret_cast<U1_UserHandle>(&errors_container->back());
}

U1_UserHandle TemplateErrorsContextHandler(
	const U1_UserHandle data, // should be "CodeBuilderError*"
	const uint32_t file_index,
	const uint32_t line,
	const uint32_t column,
	const U1_StringView& context_name,
	const U1_StringView& args_description )
{
	const auto out_error= reinterpret_cast<CodeBuilderError*>(data);
	out_error->template_context= std::make_shared<TemplateErrorsContext>();
	out_error->template_context->context_declaration_src_loc= SrcLoc( file_index, line, column );
	out_error->template_context->context_name= StringViewToString( context_name );
	out_error->template_context->parameters_description= StringViewToString( args_description );

	return reinterpret_cast<U1_UserHandle>(&out_error->template_context->errors);
}

void SourceFilePathProcessingFunction(
	const U1_UserHandle data, // should be "std::vector<IVfs::Path>*"
	const U1_StringView& file_path )
{
	reinterpret_cast< std::vector<IVfs::Path>* >(data)->push_back( StringViewToString(file_path) );
}

void LexSyntErrorProcessingFunction(
	const U1_UserHandle data, // Should be "LexSyntErrors*"
	const uint32_t file_index,
	const uint32_t line,
	const uint32_t column,
	const U1_StringView& text )
{
	LexSyntError out_error;
	out_error.src_loc= SrcLoc( file_index, line, column );
	out_error.text= StringViewToString(text);

	reinterpret_cast< LexSyntErrors* >(data)->push_back( std::move(out_error) );
}

} // namespace

CodeBuilderLaunchResult LaunchCodeBuilder(
	const IVfs::Path& input_file,
	const IVfsSharedPtr& vfs,
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const llvm::Triple& target_triple,
	const bool generate_debug_info,
	const bool generate_tbaa_metadata,
	const bool allow_unused_names,
	const ManglingScheme mangling_scheme,
	const std::string_view prelude_code )
{
	CodeBuilderLaunchResult result;

	const LLVMModuleRef llvm_module=
		U1_BuildProgramUsingVFS(
			U1_IVfsInterface{
				reinterpret_cast<U1_UserHandle>(vfs.get()),
				GetFullFilePath,
				LoadFileContent,
				IsImportingFileAllowed,
				IsFileFromSourcesDirectory },
			StringToStringView(input_file),
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout ),
			StringToStringView(target_triple.normalize()),
			generate_debug_info,
			generate_tbaa_metadata,
			allow_unused_names,
			mangling_scheme,
			StringToStringView(prelude_code),
			SourceFilePathProcessingFunction,
			reinterpret_cast<U1_UserHandle>(&result.dependent_files),
			LexSyntErrorProcessingFunction,
			reinterpret_cast<U1_UserHandle>(&result.lex_synt_errors),
			U1_ErrorsHandlingCallbacks{ ErrorHanlder, TemplateErrorsContextHandler },
			reinterpret_cast<U1_UserHandle>(&result.code_builder_errors) );

	if( llvm_module == nullptr )
		return result;

	result.llvm_module.reset( llvm::unwrap(llvm_module) ); // Take ownership over module.

	return result;
}

extern const uint32_t c_compiler_generation= U_COMPILER_GENERATION;

} // namespace U
