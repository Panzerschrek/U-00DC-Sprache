#include <fstream>
#include <iostream>

#include "../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Frontend/FrontendActions.h>
#include  <llvm/Support/Options.h>

#include "../code_builder_lib/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/program_writer.hpp"
#include "u_ast_builder.hpp"

static 	llvm::cl::OptionCategory tool_category("C++ to Ü header converter options");

static llvm::cl::opt<std::string>
output_file_name("o", llvm::cl::desc("Set output filename"),
			   llvm::cl::value_desc("filename"),  llvm::cl::init(""));

int main( int argc, const char* argv[] )
{
	clang::tooling::CommonOptionsParser options_parser( argc, argv, tool_category );
	clang::tooling::ClangTool tool( options_parser.getCompilations(), options_parser.getSourcePathList() );

	if( output_file_name.getValue().empty() )
	{
		std::cerr << "No outpit file" << std::endl;
		return 1;
	}

	auto parsed_units= std::make_shared<U::ParsedUnits>();
	U::FrontendActionFactory factory(parsed_units);
	const int res= tool.run( &factory );
	if( res != 0 )
		return res;

	std::ofstream out_file( output_file_name.getValue() );

	for( const auto& unit : *parsed_units )
	{
		U::Synt::WriteProgram( unit.second, out_file );
	}

	return 0;
}
