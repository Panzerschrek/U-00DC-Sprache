#include <fstream>
#include <iostream>

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/TargetSelect.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../compiler0/lex_synt_lib/program_writer.hpp"
#include "u_ast_builder.hpp"

int main( int argc, const char* argv[] )
{
	// Initialize targets for clang module support.
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmPrinters();
	llvm::InitializeAllAsmParsers();

	llvm::cl::OptionCategory tool_category( "C++ to Ü header converter options" );

	llvm::cl::opt<std::string> output_file_name(
		"o",
		llvm::cl::desc("Set output filename"),
		llvm::cl::value_desc("filename"),
		llvm::cl::cat(tool_category) );

	llvm::cl::opt<bool> skip_declarations_from_includes(
		"skip-declarations-from-includes",
		llvm::cl::desc("Skip declarations from includes."),
		llvm::cl::init(false),
		llvm::cl::cat(tool_category) );

	llvm::cl::list<std::string> force_import(
		"force-import",
		llvm::cl::CommaSeparated,
		llvm::cl::desc("Specify list of files, added to imports section of result file."),
		llvm::cl::value_desc("file1, file2, fileN,..."),
		llvm::cl::Optional,
		llvm::cl::cat(tool_category) );

	auto options_parser_opt= clang::tooling::CommonOptionsParser::create( argc, argv, tool_category );
	if( !options_parser_opt )
	{
		std::cerr << "Failed to parse options" << std::endl;
		return 1;
	}

	auto& options_parser= options_parser_opt.get();
	clang::tooling::ClangTool tool( options_parser.getCompilations(), options_parser.getSourcePathList() );

	if( output_file_name.getValue().empty() )
	{
		std::cerr << "No outpit file" << std::endl;
		return 1;
	}

	const auto parsed_units= std::make_shared<U::ParsedUnits>();
	U::FrontendActionFactory factory(parsed_units, skip_declarations_from_includes);
	const int res= tool.run( &factory );
	if( res != 0 )
		return res;

	std::ofstream out_file( output_file_name.getValue() );
	for( const std::string& import : force_import )
		out_file << "import " << "\"" << import << "\"\n";

	for( auto& unit : *parsed_units )
		U::Synt::WriteProgram( unit.second.Build(), out_file );

	return 0;
}
