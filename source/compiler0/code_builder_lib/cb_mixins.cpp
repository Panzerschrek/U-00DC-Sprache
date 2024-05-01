#include "../../lex_synt_lib_common/assert.hpp"
#include "../lex_synt_lib/lexical_analyzer.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"

namespace U
{

namespace
{

constexpr size_t g_max_mixins_depth= 4;

} // namespace

void CodeBuilder::ProcessMixins( NamesScope& names_scope )
{
	// Perform several iterations in order to process mixins within mixins.
	for( size_t i= 0; i < g_max_mixins_depth; ++i )
	{
		// First evaluate all expressions. Doing so we prevent symbols produced in expansion of one mixin visible in expression of another.
		const size_t num_expressions= EvaluateMixinsExpressions_r( names_scope );
		if(num_expressions == 0)
			break;

		if( i == g_max_mixins_depth - 1 )
			REPORT_ERROR( MixinExpansionDepthReached, names_scope.GetErrors(), SrcLoc( 0, 1, 0 ) );

		// Populate name scopes using expressions evaluated on previous step.
		ExpandNamespaceMixins_r( names_scope );
	}
}

size_t CodeBuilder::EvaluateMixinsExpressions_r( NamesScope& names_scope )
{
	size_t result= 0;

	names_scope.ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( const NamesScopePtr inner_namespace= value.GetNamespace() )
				result+= EvaluateMixinsExpressions_r( *inner_namespace );
			else if( const Type* const type= value.GetTypeName() )
			{
				if( const ClassPtr class_type= type->GetClassType() )
				{
					// Expand mixins only from parent namespace.
					// Otherwise we can get loop, using type alias.
					if( class_type->members->GetParent() == &names_scope )
						result+= EvaluateMixinsExpressions_r( *class_type->members );
				}
			}
			else if( const auto mixins= value.GetMixins() )
			{
				result+= mixins->size();
				for( Mixin& mixin : *mixins )
					EvaluateMixinExpression( names_scope, mixin );
			}
		} );

	return result;
}

void CodeBuilder::ExpandNamespaceMixins_r( NamesScope& names_scope )
{
	// First collect mixins into a container, than expand mixins.
	// Doing so we avoid modifying names_scope while iterating it.
	llvm::SmallVector<Mixins, 1> all_mixins;

	names_scope.ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( const NamesScopePtr inner_namespace= value.GetNamespace() )
				ExpandNamespaceMixins_r( *inner_namespace );
			else if( const Type* const type= value.GetTypeName() )
			{
				if( const ClassPtr class_type= type->GetClassType() )
				{
					// Expand mixins only from parent namespace.
					// Otherwise we can get loop, using type alias.
					if( class_type->members->GetParent() == &names_scope )
						ExpandClassMixins_r( class_type );
				}
			}
			else if( const auto mixins= value.GetMixins() )
				all_mixins.push_back( std::move(*mixins) ); // Move mixins, because they are not needed later.
		} );

	for( Mixins& mixins : all_mixins )
		for( Mixin& mixin : mixins )
			ExpandNamespaceMixin( names_scope, mixin );
}

void CodeBuilder::ProcessClassMixins( const ClassPtr class_type )
{
	// Perform several iterations in order to process mixins within mixins.
	for( size_t i= 0; i < g_max_mixins_depth; ++i )
	{
		// First evaluate all expressions.
		const size_t num_expressions= EvaluateMixinsExpressions_r( *class_type->members );
		if(num_expressions == 0)
			break;

		if( i == g_max_mixins_depth - 1 )
			REPORT_ERROR( MixinExpansionDepthReached, class_type->members->GetErrors(), SrcLoc( 0, 1, 0 ) );

		// Than perform expansion.
		ExpandClassMixins_r( class_type );
	}
}

void CodeBuilder::ExpandClassMixins_r( const ClassPtr class_type )
{
	// First collect mixins into a container, than expand mixins.
	// Doing so we avoid modifying names_scope while iterating it.
	llvm::SmallVector<Mixins, 1> all_mixins;

	class_type->members->ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( const Type* const type= value.GetTypeName() )
			{
				if( const ClassPtr inner_class_type= type->GetClassType() )
				{
					// Expand mixins only from parent namespace.
					// Otherwise we can get loop, using type alias.
					if( inner_class_type->members->GetParent() == class_type->members.get() )
						ExpandClassMixins_r( inner_class_type );
				}
			}
			else if( const auto mixins= value.GetMixins() )
				all_mixins.push_back( std::move(*mixins) ); // Move mixins, because they are not needed later.
		} );

	for( Mixins& mixins : all_mixins )
		for( Mixin& mixin : mixins )
			ExpandClassMixin( class_type, mixin );
}

void CodeBuilder::ExpandNamespaceMixin( NamesScope& names_scope, Mixin& mixin )
{
	if( mixin.string_constant == nullptr )
		return;

	const llvm::StringRef mixin_text= mixin.string_constant->getRawDataValues();
	mixin.string_constant= nullptr;

	const MixinExpansionKey key{ mixin.src_loc, mixin_text.str() };
	auto it= namespace_mixin_expansions_.find(key);

	if( it == namespace_mixin_expansions_.end() )
	{
		const auto lexems= PrepareMixinLexems( names_scope, mixin.src_loc, mixin_text );
		if( lexems == std::nullopt )
			return;

		// TODO - handle wrong file index.
		const SourceGraph::Node& source_graph_node= source_graph_->nodes_storage[ mixin.src_loc.GetFileIndex() ];

		Synt::NamespaceParsingResult synt_result=
			Synt::ParseNamespaceElements(
				*lexems,
				Synt::MacrosByContextMap(), // TODO - use proper macros
				source_graph_->macro_expansion_contexts,
				source_graph_node.contents_hash );

		if( !synt_result.error_messages.empty() )
		{
			for( const LexSyntError& error : synt_result.error_messages )
				REPORT_ERROR( MixinSyntaxError, names_scope.GetErrors(), error.src_loc, error.text );

			return;
		}

		// We need to preserve syntax result, because we store raw pointers to syntax elements.
		it= namespace_mixin_expansions_.emplace( std::make_pair( key, NamespaceMixinExpansionResult{ std::move(synt_result.namespace_elements) } ) ).first;
	}

	NamesScopeFill( names_scope, it->second.program_elements );
}

void CodeBuilder::ExpandClassMixin( const ClassPtr class_type, Mixin& mixin )
{
	NamesScope& class_members= *class_type->members;

	if( mixin.string_constant == nullptr )
		return;

	const llvm::StringRef mixin_text= mixin.string_constant->getRawDataValues();
	mixin.string_constant= nullptr;

	const MixinExpansionKey key{ mixin.src_loc, mixin_text.str() };
	auto it= class_mixin_expansions_.find(key);

	if( it == class_mixin_expansions_.end() )
	{
		const auto lexems= PrepareMixinLexems( class_members, mixin.src_loc, mixin_text );
		if( lexems == std::nullopt )
			return;

		// TODO - handle wrong file index.
		const SourceGraph::Node& source_graph_node= source_graph_->nodes_storage[ mixin.src_loc.GetFileIndex() ];

		Synt::ClassElementsParsingResult synt_result=
			Synt::ParseClassElements(
				*lexems,
				Synt::MacrosByContextMap(), // TODO - use proper macros
				source_graph_->macro_expansion_contexts,
				source_graph_node.contents_hash );

		if( !synt_result.error_messages.empty() )
		{
			for( const LexSyntError& error : synt_result.error_messages )
				REPORT_ERROR( MixinSyntaxError, class_members.GetErrors(), error.src_loc, error.text );

			return;
		}

		// We need to preserve syntax result, because we store raw pointers to syntax elements.
		it= class_mixin_expansions_.emplace( std::make_pair( key, ClassMixinExpansionResult{ std::move(synt_result.class_elements) } ) ).first;
	}

	Synt::ClassKindAttribute class_kind= Synt::ClassKindAttribute::Struct;
	std::string_view class_name;
	if( class_type->syntax_element != nullptr )
	{
		class_kind= class_type->syntax_element->kind_attribute_;
		class_name= class_type->syntax_element->name;
	}
	else
		class_name= class_members.GetThisNamespaceName();

	FillClassNamesScope( class_type, class_name, class_kind, it->second.class_elements, mixin.visibility );
}

void CodeBuilder::EvaluateMixinExpression( NamesScope& names_scope, Mixin& mixin )
{
	if( mixin.syntax_element == nullptr || mixin.string_constant != nullptr )
		return;

	const Synt::Mixin& syntax_element= *mixin.syntax_element;

	VariablePtr variable;
	{
		const StackVariablesStorage dummy_stack_variables_storage( *global_function_context_ );
		variable= BuildExpressionCodeEnsureVariable( syntax_element.expression, names_scope, *global_function_context_ );
		global_function_context_->args_preevaluation_cache.clear();
	}

	mixin.syntax_element= nullptr;

	if( variable->constexpr_value == nullptr )
	{
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), syntax_element.src_loc );
		return;
	}

	const auto array_type= variable->type.GetArrayType();
	if( array_type == nullptr ||
		array_type->element_type != FundamentalType( U_FundamentalType::char8_, fundamental_llvm_types_.char8_ )  )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), syntax_element.src_loc, "char8 array", variable->type.ToString() );
		return;
	}

	const auto constant_data= llvm::dyn_cast<llvm::ConstantDataArray>( variable->constexpr_value );
	if( constant_data == nullptr )
	{
		REPORT_ERROR( NotImplemented, names_scope.GetErrors(), syntax_element.src_loc, "non-trivial mixin constants" );
		return;
	}
	// TODO - check UTF-8 is valid.

	mixin.string_constant= constant_data;
}

std::optional<Lexems> CodeBuilder::PrepareMixinLexems( NamesScope& names_scope, const SrcLoc& src_loc, std::string_view mixin_text )
{
	LexicalAnalysisResult lex_result= LexicalAnalysis( mixin_text );

	// Create new macro expansion context for mixin expansion.
	const uint32_t macro_expansion_index= uint32_t(macro_expansion_contexts_->size());

	{
		Synt::MacroExpansionContext mixin_context;
		mixin_context.macro_declaration_src_loc= src_loc;
		mixin_context.macro_name= Keyword( Keywords::mixin_ );
		mixin_context.src_loc= src_loc;

		macro_expansion_contexts_->push_back( std::move(mixin_context) );
	}

	// Use file index of mixin.
	const uint32_t file_index= src_loc.GetFileIndex();

	// Numerate lines in mixin lexems starting with line of mixin expansion point.
	const uint32_t line_shift= src_loc.GetLine() - 1;

	for( Lexem& lexem : lex_result.lexems )
	{
		lexem.src_loc.SetFileIndex( file_index );
		lexem.src_loc.SetMacroExpansionIndex( macro_expansion_index );
		lexem.src_loc.SetLine( lexem.src_loc.GetLine() + line_shift );
	}

	if( !lex_result.errors.empty() )
	{
		for( const LexSyntError& error : lex_result.errors )
		{
			SrcLoc src_loc_corrected= error.src_loc;
			src_loc_corrected.SetFileIndex( file_index );
			src_loc_corrected.SetMacroExpansionIndex( macro_expansion_index );
			src_loc_corrected.SetLine( src_loc_corrected.GetLine() + line_shift );

			REPORT_ERROR( MixinLexicalError, names_scope.GetErrors(), src_loc_corrected, error.text );
		}

		return std::nullopt;
	}

	return std::move(lex_result.lexems);
}

} // namespace U
