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
	const llvm::ArrayRef<std::string> include_dirs,
	const llvm::ArrayRef<std::string> source_dirs,
	const bool prevent_imports_outside_given_directories,
	llvm::ArrayRef<std::string> input_files,
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
	result.llvm_modules.resize( input_files.size() );

	std::vector<U1_StringView> inciude_dir_views;
	inciude_dir_views.reserve( include_dirs.size() );
	for( const std::string& s : include_dirs )
		inciude_dir_views.push_back( StringToStringView(s) );

	std::vector<U1_StringView> source_dir_views;
	source_dir_views.reserve( source_dirs.size() );
	for( const std::string& s : source_dirs )
		source_dir_views.push_back( StringToStringView(s) );

	void* const vfs=
		U1_CreateVFS(
			inciude_dir_views.data(),
			inciude_dir_views.size(),
			source_dir_views.data(),
			source_dir_views.size(),
			prevent_imports_outside_given_directories );

	for( size_t i= 0; i < input_files.size(); ++i )
	{
		const LLVMModuleRef llvm_module=
			U1_BuildProgramUsingVFS(
				vfs,
				StringToStringView(input_files[i]),
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

		if( llvm_module != nullptr )
			result.llvm_modules[i]= std::unique_ptr<llvm::Module>( llvm::unwrap(llvm_module) ); // Take ownership over module.
	}

	return result;
}

extern const uint32_t c_compiler_generation= U_COMPILER_GENERATION;

} // namespace U
