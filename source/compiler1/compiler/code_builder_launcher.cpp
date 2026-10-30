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

void SourceFilePathProcessingFunction(
	const U1_UserHandle data, // should be "std::vector<std::string>*"
	const U1_StringView& file_path )
{
	reinterpret_cast< std::vector<std::string>* >(data)->push_back( StringViewToString(file_path) );
}

std::vector<U1_StringView> MakeVectorOfStringViews( const llvm::ArrayRef<std::string> strings )
{
	std::vector<U1_StringView> result( strings.size() );

	for( size_t i= 0; i < strings.size(); ++i )
		result[i]= StringToStringView( strings[i] );

	return result;
}

} // namespace

CodeBuilderLaunchResult LaunchCodeBuilder(
	const llvm::ArrayRef<std::string> include_dirs,
	const llvm::ArrayRef<std::string> source_dirs,
	const bool prevent_imports_outside_given_directories,
	const llvm::ArrayRef<std::string> input_files,
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const llvm::Triple& target_triple,
	const bool generate_debug_info,
	const bool generate_tbaa_metadata,
	const bool allow_unused_names,
	const ManglingScheme mangling_scheme,
	const ErrorsFormat errors_format,
	const bool tests_output,
	const std::string_view prelude_code )
{
	CodeBuilderLaunchResult result;
	result.llvm_modules.resize( input_files.size() );

	const std::vector<U1_StringView> inciude_dir_views= MakeVectorOfStringViews( include_dirs );
	const std::vector<U1_StringView> source_dir_views= MakeVectorOfStringViews( source_dirs );
	const std::vector<U1_StringView> input_file_views= MakeVectorOfStringViews( input_files );

	static_assert( sizeof(LLVMModuleRef) == sizeof( std::unique_ptr<llvm::Module> ), "Unexpected size!" );

	const std::string target_triple_str= target_triple.normalize();

	result.has_errors=
		!U1_BuildProgramUsingVFS(
			inciude_dir_views.data(),
			inciude_dir_views.size(),
			source_dir_views.data(),
			source_dir_views.size(),
			prevent_imports_outside_given_directories,
			input_file_views.data(),
			input_file_views.size(),
			// Pass the result array containing unique_ptr instances as array containing raw pointers.
			// This should work fine, assuming their layout is identical.
			reinterpret_cast<LLVMModuleRef*>( result.llvm_modules.data() ),
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout ),
			StringToStringView(target_triple_str),
			generate_debug_info,
			generate_tbaa_metadata,
			allow_unused_names,
			mangling_scheme,
			errors_format,
			tests_output,
			StringToStringView(prelude_code),
			SourceFilePathProcessingFunction,
			reinterpret_cast<U1_UserHandle>(&result.dependent_files) );

	return result;
}

extern const uint32_t c_compiler_generation= U_COMPILER_GENERATION;

} // namespace U
