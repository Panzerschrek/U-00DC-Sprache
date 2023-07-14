#include <sstream>
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Triple.h>
#include <llvm/MC/SubtargetFeature.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "prelude.hpp"
#include "../sprache_version/sprache_version.hpp"

namespace U
{

std::string GenerateCompilerPreludeCode(
	const llvm::Triple& target_triple,
	const llvm::DataLayout& data_layout,
	const llvm::StringRef features,
	const llvm::StringRef cpu_name,
	const char optimization_level,
	const bool generate_debug_info,
	const uint32_t compiler_generation )
{
	std::ostringstream ss;

	ss << "namespace compiler\n{\n";
	{
		// Info about compiler itself.
		ss << "auto& version = \"" << getSpracheVersion() << "\";\n";
		ss << "auto& git_revision = \"" << getGitRevision() << "\";\n";
		ss << "var size_type generation = " << std::to_string(compiler_generation) << "s;\n";

		// Options.
		ss << "namespace options\n{\n";
		{
			ss << "var char8 optimization_level = \"" << optimization_level << "\"c8;\n";
			ss << "var bool generate_debug_info = " << (generate_debug_info ? "true" : "false") << ";\n";
			ss << "auto& cpu_name = \"" << cpu_name.str() << "\";\n";

			// Features
			{
				const llvm::SubtargetFeatures features_parsed( features );
				const auto features_list= features_parsed.getFeatures();

				ss << "var tup[ ";
				for( const std::string& feature : features_list )
				{
					ss << "[ char8, " << std::to_string(feature.size()) << " ]";
					if( &feature != &features_list.back() )
						ss << ", ";
				}
				ss << " ]";

				ss << " features[ ";
				for( const std::string& feature : features_list )
				{
					ss << "\"" << feature << "\"";
					if( &feature != &features_list.back() )
						ss << ", ";
				}
				ss << " ];\n";
			}
		}
		ss << "}\n";

		// Target triple.
		ss << "namespace target\n{\n";
		{
			ss << "auto& str = \"" << target_triple.str() << "\";\n";
			ss << "auto& arch = \"" << target_triple.getArchName().str() << "\";\n";
			ss << "auto& vendor = \"" << target_triple.getVendorName().str() << "\";\n";
			ss << "auto& os = \"" << target_triple.getOSName().str() << "\";\n";
			ss << "auto& environment = \"" << target_triple.getEnvironmentName().str() << "\";\n";
			ss << "auto& os_and_environment = \"" << target_triple.getOSAndEnvironmentName().str() << "\";\n";

			ss << "var bool is_big_endian = " << (data_layout.isBigEndian() ? "true" : "false") << ";\n";
		}
		ss << "}\n";
	}
	ss << "}\n";

	return ss.str();
}

} // namespace U
