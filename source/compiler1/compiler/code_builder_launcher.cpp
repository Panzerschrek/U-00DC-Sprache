#include "../../compilers_common_lib/code_builder_launcher.hpp"
#include "../launchers_common/funcs_c.hpp"

namespace U
{

namespace
{

U1_StringView StringToStringView( const std::string& s )
{
	return U1_StringView{ s.data(), s.size() };
}

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

	result_callback( user_data, StringToStringView(path) );
}

bool LoadFileContent(
	const UserHandle this_,
	const U1_StringView& path_normalized,
	const IVfsInterface::FillStringCallback result_callback,
	const UserHandle user_data )
{
	const std::optional<IVfs::FileContent> file_content=
		reinterpret_cast<IVfs*>(this_)->LoadFileContent( StringViewToString(path_normalized) );

	if( file_content == std::nullopt )
		return false;

	result_callback( user_data, StringToStringView( *file_content ) );
	return true;
}

UserHandle ErrorHanlder(
	const UserHandle data, // should be "CodeBuilderErrorsContainer"
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
	return reinterpret_cast<UserHandle>(&errors_container->back());
}

UserHandle TemplateErrorsContextHandler(
	const UserHandle data, // should be "CodeBuilderError*"
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

	return reinterpret_cast<UserHandle>(&out_error->template_context->errors);
}


void SourceFilePathProcessingFunction(
	const UserHandle data, // should be "std::vector<IVfs::Path>*"
	const U1_StringView& file_path )
{
	reinterpret_cast< std::vector<IVfs::Path>* >(data)->push_back( StringViewToString(file_path) );
}

void LexSyntErrorProcessingFunction(
	const UserHandle data, // Should be "LexSyntErrors*"
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
	IVfs& vfs,
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const llvm::Triple& target_triple,
	const bool generate_debug_info,
	const bool generate_tbaa_metadata,
	const bool avoid_building_function_bodies,
	const ManglingScheme mangling_scheme )
{
	CodeBuilderLaunchResult result;

	(void)avoid_building_function_bodies; // TODO - use it.

	const LLVMModuleRef llvm_module=
		U1_BuildProgramUsingVFS(
			IVfsInterface{ reinterpret_cast<UserHandle>(&vfs), GetFullFilePath, LoadFileContent },
			StringToStringView(input_file),
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout ),
			StringToStringView(target_triple.normalize()),
			generate_debug_info,
			generate_tbaa_metadata,
			mangling_scheme,
			SourceFilePathProcessingFunction,
			reinterpret_cast<UserHandle>(&result.dependent_files),
			LexSyntErrorProcessingFunction,
			reinterpret_cast<UserHandle>(&result.lex_synt_errors),
			ErrorsHandlingCallbacks{ ErrorHanlder, TemplateErrorsContextHandler },
			reinterpret_cast<UserHandle>(&result.code_builder_errors) );

	if( llvm_module == nullptr )
		return result;

	result.llvm_module.reset( llvm::unwrap(llvm_module) ); // Take ownership over module.

	return result;
}

uint32_t GetCompilerGeneration()
{
	return U_COMPILER_GENERATION;
}

} // namespace U
