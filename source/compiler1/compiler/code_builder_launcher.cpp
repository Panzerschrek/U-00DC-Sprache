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
			llvm::wrap(&data_layout ) );

	// TODO - process errors
	if( llvm_module == nullptr )
		return result;

	result.llvm_module.reset( llvm::unwrap(llvm_module) ); // Take ownership over module.

	return result;
}

} // namespace U
