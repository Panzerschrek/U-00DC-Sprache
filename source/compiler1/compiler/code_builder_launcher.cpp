#include "../../compilers_common_lib/code_builder_launcher.hpp"
#include "../tests_common/funcs_c.hpp"

namespace U
{

namespace
{

const std::string StringViewToString( const U1_StringView view )
{
	return std::string( view.data, view.data + view.size );
}

void GetFullFilePath(
	const UserHandle this_,
	const U1_StringView& file_path,
	const U1_StringView& parent_file_path_normalized,
	const  IVfsInterface::FillStringCallback result_callback,
	const UserHandle user_data )
{
	const std::string path=
		reinterpret_cast<IVfs*>(this_)->GetFullFilePath(
			StringViewToString(file_path),
			StringViewToString(parent_file_path_normalized) );

	result_callback( user_data, U1_StringView{ path.data(), path.size() } );
}

bool LoadFileContent(
	const UserHandle this_,
	const U1_StringView& path,
	const U1_StringView& parent_file_path_normalized,
	const IVfsInterface::FillStringCallback result_callback,
	const UserHandle user_data )
{
	const std::optional<IVfs::LoadFileResult> load_file_result=
		reinterpret_cast<IVfs*>(this_)->LoadFileContent(
			StringViewToString(path),
			StringViewToString(parent_file_path_normalized) );

	if( load_file_result == std::nullopt )
		return false;

	result_callback(
		user_data,
		U1_StringView{ load_file_result->file_content.data(), load_file_result->file_content.size() } );

	return true;
}

UserHandle ErrorHanlder(
	const UserHandle data, // should be "CodeBuilderErrorsContainer"
	const uint32_t line,
	const uint32_t column,
	const uint32_t error_code,
	const U1_StringView& error_text )
{
	CodeBuilderError error;
	error.file_pos= FilePos( 0u, line, column );
	error.code= CodeBuilderErrorCode(error_code);
	error.text= std::string( error_text.data, error_text.data + error_text.size );

	const auto errors_container= reinterpret_cast<CodeBuilderErrorsContainer*>(data);
	errors_container->push_back( std::move(error) );
	return reinterpret_cast<UserHandle>(&errors_container->back());
}

UserHandle TemplateErrorsContextHandler(
	const UserHandle data, // should be "CodeBuilderError*"
	const uint32_t line,
	const uint32_t column,
	const U1_StringView& context_name,
	const U1_StringView& args_description )
{
	const auto out_error= reinterpret_cast<CodeBuilderError*>(data);
	out_error->template_context= std::make_shared<TemplateErrorsContext>();
	out_error->template_context->context_declaration_file_pos= FilePos( 0u, line, column );
	out_error->template_context->context_name= std::string( context_name.data, context_name.data + context_name.size );
	out_error->template_context->parameters_description= std::string( args_description.data, args_description.data + args_description.size );

	return reinterpret_cast<UserHandle>( & out_error->template_context->errors );
}

const ErrorsHandlingCallbacks g_error_handling_callbacks
{
	ErrorHanlder,
	TemplateErrorsContextHandler,
};

void SourceFilePathProcessingFunction(
	const UserHandle data, // should be "std::vector<IVfs::Path>*"
	const U1_StringView& file_path )
{
	reinterpret_cast< std::vector<IVfs::Path>* >(data)->emplace_back( file_path.data, file_path.data + file_path.size );
}

} // namespace

CodeBuilderLaunchResult launchCodeBuilder(
	const IVfs::Path& input_file,
	const IVfsPtr& vfs,
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const bool generate_debug_info )
{
	(void)generate_debug_info;

	CodeBuilderLaunchResult result;

	const LLVMModuleRef llvm_module=
		U1_BuildProgrammUsingVFS(
			IVfsInterface{ reinterpret_cast<UserHandle>(vfs.get()), GetFullFilePath, LoadFileContent },
			U1_StringView{ input_file.data(), input_file.size() },
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout ),
			SourceFilePathProcessingFunction,
			reinterpret_cast<UserHandle>(&result.dependent_files),
			g_error_handling_callbacks,
			reinterpret_cast<UserHandle>(&result.code_builder_errors) );

	// TODO - process syntax errors.
	if( llvm_module == nullptr )
		return result;

	result.llvm_module.reset( llvm::unwrap(llvm_module) ); // Take ownership over module.

	return result;
}

std::string_view CodeBuilderErrorCodeToString( const CodeBuilderErrorCode code )
{
	const char* text= nullptr;
	size_t length= 0;
	U1_CodeBuilderCodeToString( uint32_t(code), text, length );
	return std::string_view( text, length );
}

} // namespace U
