#include "../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Frontend/FrontendActions.h>
#include "../code_builder_lib/pop_llvm_warnings.hpp"

#include "u_ast_builder.hpp"


int main( int argc, const char* argv[] )
{
	llvm::cl::OptionCategory tool_category("C++ to Ü header converter options");

	clang::tooling::CommonOptionsParser options_parser( argc, argv, tool_category );
	clang::tooling::ClangTool tool( options_parser.getCompilations(), options_parser.getSourcePathList() );

	auto parsed_units= std::make_shared<U::ParsedUnits>();
	U::FrontendActionFactory factory(parsed_units);
	tool.run( &factory );

	return 0;
}
