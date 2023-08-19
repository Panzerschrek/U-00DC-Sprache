#include "../code_builder_lib_common/source_file_contents_hash.hpp"
#include "../compiler0/lex_synt_lib/syntax_analyzer.hpp"
#include "../compiler0/code_builder_lib/code_builder.hpp"
#include "../tests/tests_common.hpp"
#include "syntax_tree_lookup.hpp"
#include "document.hpp"

namespace U
{

namespace LangServer
{

namespace
{

// Complexity is linear.
std::optional<SrcLoc> GetLexemSrcLocForPosition( const uint32_t line, const uint32_t column, const Lexems& lexems )
{
	// TODO - return none, if position is between lexems.

	const SrcLoc pos_loc( 0, line, column );
	auto it= lexems.begin();
	for( ; it < lexems.end(); ++it )
	{
		// Compare without file index and macro expansion context.
		const SrcLoc lexem_loc( 0, it->src_loc.GetLine(), it->src_loc.GetColumn() );
		if( pos_loc == lexem_loc )
			return lexem_loc;
		if( pos_loc < lexem_loc )
		{
			if( it != lexems.begin() )
				return std::prev(it)->src_loc;
		}
	}

	return std::nullopt;
}

} // namespace

Document::Document( std::string text )
{
	SetText( std::move(text) );
}

LexSyntErrors Document::GetLexErrors() const
{
	return lex_errors_;
}

LexSyntErrors Document::GetSyntErrors() const
{
	return synt_errors_;
}

CodeBuilderErrorsContainer Document::GetCodeBuilderErrors() const
{
	return code_builder_errors_;
}

std::optional<SrcLoc> Document::GetDefinitionPoint( const SrcLoc& src_loc )
{
	if( last_valid_state_ == std::nullopt )
		return std::nullopt;

	// Find lexem, where position is located.
	const auto lexem_position= GetLexemSrcLocForPosition( src_loc.GetLine(), src_loc.GetColumn(), last_valid_state_->lexems );
	if( lexem_position == std::nullopt )
		return std::nullopt;

	// Find syntax element for given syntax element.
	const NamedSyntaxElement syntax_element=
		FindSyntaxElementForPosition( lexem_position->GetLine(), lexem_position->GetColumn(), last_valid_state_->source_graph.nodes_storage.front().ast.program_elements );

	// TODO - perform actual lookup.
	(void)syntax_element;

	return lexem_position;
}

void Document::SetText( std::string text )
{
	if( text == text_ )
		return;

	text_= text;

	lex_errors_.clear();
	synt_errors_.clear();
	code_builder_errors_.clear();

	LexicalAnalysisResult lex_result= LexicalAnalysis( text_ );
	lex_errors_= std::move( lex_result.errors );
	if( !lex_errors_.empty() )
		return;

	// TODO - parse imports and read files or request another opended documents.
	// TODO - provide options for import directories.
	// TODO - fill macros from imported files.

	const auto macro_expansion_contexts= std::make_shared<Synt::MacroExpansionContexts>();

	Synt::SyntaxAnalysisResult synt_result=
		Synt::SyntaxAnalysis(
			lex_result.lexems,
			Synt::MacrosByContextMap(),
			macro_expansion_contexts,
			CalculateSourceFileContentsHash( text_ ) );

	synt_errors_= std::move(synt_result.error_messages);
	if( !synt_errors_.empty() )
		return;

	// TODO - add also generated prelude.

	SourceGraph::Node source_graph_node;
	source_graph_node.ast= std::move(synt_result);

	SourceGraph source_graph;
	source_graph.nodes_storage.push_back( std::move(source_graph_node) );
	source_graph.macro_expansion_contexts= macro_expansion_contexts;

	llvm::LLVMContext llvm_context;

	// TODO - create proper target machine.
	llvm::DataLayout data_layout( GetTestsDataLayout() );
	// TODO - use target triple, dependent on compilation options.
	llvm::Triple target_triple( llvm::sys::getDefaultTargetTriple() );

	// Disable almost all code builder options.
	// We do not need to generate code here - only assist developer (retrieve errors, etc.).
	CodeBuilderOptions options;
	options.build_debug_info= false;
	options.create_lifetimes= false;
	options.generate_lifetime_start_end_debug_calls= false;
	options.generate_tbaa_metadata= false;
	options.report_about_unused_names= false;

	CodeBuilder::BuildResult build_result=
		CodeBuilder(
			llvm_context,
			data_layout,
			target_triple,
			options ).BuildProgram( source_graph );

	code_builder_errors_= std::move(build_result.errors);

	last_valid_state_= std::nullopt;
	last_valid_state_= CompiledState{ std::move( lex_result.lexems ), std::move( source_graph ) };
}

} // namespace LangServer

} // namespace U
