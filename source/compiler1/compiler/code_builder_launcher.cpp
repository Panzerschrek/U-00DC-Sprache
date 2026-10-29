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

	std::vector<U1_StringView> inciude_dir_views;
	inciude_dir_views.reserve( include_dirs.size() );
	for( const std::string& s : include_dirs )
		inciude_dir_views.push_back( StringToStringView(s) );

	std::vector<U1_StringView> source_dir_views;
	source_dir_views.reserve( source_dirs.size() );
	for( const std::string& s : source_dirs )
		source_dir_views.push_back( StringToStringView(s) );

	std::vector<U1_StringView> input_file_views;
	input_file_views.reserve( input_files.size() );
	for( const std::string& s : input_files )
		input_file_views.push_back( StringToStringView(s) );

	static_assert( sizeof(LLVMModuleRef) == sizeof( std::unique_ptr<llvm::Module> ), "Unexpected size!" );

	result.has_errors=
		!U1_BuildProgramUsingVFS(
			inciude_dir_views.data(),
			inciude_dir_views.size(),
			source_dir_views.data(),
			source_dir_views.size(),
			prevent_imports_outside_given_directories,
			input_file_views.data(),
			input_file_views.size(),
			// Pas result array containing unique_ptr instances as array containing raw pointers.
			// This should work fine, assuming their layout is identical.
			reinterpret_cast<LLVMModuleRef*>( result.llvm_modules.data() ),
			llvm::wrap(&llvm_context),
			llvm::wrap(&data_layout ),
			StringToStringView(target_triple.normalize()),
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
