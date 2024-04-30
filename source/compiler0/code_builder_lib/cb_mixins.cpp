#include "../../lex_synt_lib_common/assert.hpp"
#include "../lex_synt_lib/lexical_analyzer.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"

namespace U
{

void CodeBuilder::ExpandMixins_r( NamesScope& names_scope )
{
	llvm::SmallVector<Mixins*, 1> all_mixins;

	names_scope.ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( const NamesScopePtr inner_namespace= value.GetNamespace() )
				ExpandMixins_r( *inner_namespace );
			else if( const Type* const type= value.GetTypeName() )
			{
				if( const ClassPtr class_type= type->GetClassType() )
				{
					// Expand mixins only from parent namespace.
					// Otherwise we can get loop, using type alias.
					if( class_type->members->GetParent() == &names_scope )
						ExpandClassMixins( class_type );
				}
			}
			else if( const auto mixins= value.GetMixins() )
				all_mixins.push_back( mixins );
			else if(
				value.GetFunctionsSet() != nullptr ||
				value.GetTypeTemplatesSet() != nullptr ||
				value.GetClassField() != nullptr ||
				value.GetVariable() != nullptr ||
				value.GetErrorValue() != nullptr ||
				value.GetStaticAssert() != nullptr ||
				value.GetTypeAlias() != nullptr ||
				value.GetIncompleteGlobalVariable() != nullptr )
			{}
			else U_ASSERT(false);
		} );

	// Expand mixins outside namespace iteration, because expansion requires namespace modification.
	for( const auto mixins : all_mixins )
	{
		// Avoid using iterator-based foreach, in order to handle container additions in recursive calls.
		for( size_t i= 0; i < mixins->syntax_elements.size(); ++i )
			ExpandNamespaceMixin( names_scope, *mixins->syntax_elements[i] );

		// Clear the container, since all mixins from it were already expanded.
		mixins->syntax_elements.clear();
	}
}

void CodeBuilder::ExpandClassMixins( const ClassPtr class_type )
{
	llvm::SmallVector<Mixins*, 1> all_mixins;

	class_type->members->ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( value.GetNamespace() != nullptr )
			{
				U_ASSERT(false); // Should not have namespaces inside classes.
			}
			else if( const Type* const type= value.GetTypeName() )
			{
				if( const ClassPtr inner_class_type= type->GetClassType() )
				{
					// Expand mixins only from parent namespace.
					// Otherwise we can get loop, using type alias.
					if( inner_class_type->members->GetParent() == class_type->members.get() )
						ExpandClassMixins( inner_class_type );
				}
			}
			else if( const auto mixins= value.GetMixins() )
				all_mixins.push_back( mixins );
			else if(
				value.GetFunctionsSet() != nullptr ||
				value.GetTypeTemplatesSet() != nullptr ||
				value.GetClassField() != nullptr ||
				value.GetVariable() != nullptr ||
				value.GetErrorValue() != nullptr ||
				value.GetStaticAssert() != nullptr ||
				value.GetTypeAlias() != nullptr ||
				value.GetIncompleteGlobalVariable() != nullptr )
			{}
			else U_ASSERT(false);
		} );

	// Expand mixins outside namespace iteration, because expansion requires namespace modification.
	for( const auto mixins : all_mixins )
	{
		// Avoid using iterator-based foreach, in order to handle container additions in recursive calls.
		for( size_t i= 0; i < mixins->syntax_elements.size(); ++i )
			ExpandClassMixin( class_type, *mixins->syntax_elements[i] );

		// Clear the container, since all mixins from it were already expanded.
		mixins->syntax_elements.clear();
	}
}

void CodeBuilder::ExpandNamespaceMixin( NamesScope& names_scope, const Synt::Mixin& mixin )
{
	const auto mixin_text= EvaluateMixinString( names_scope, mixin );
	if( mixin_text == std::nullopt )
		return;

	const MixinExpansionKey key{ mixin.src_loc, mixin_text->str() };
	auto it= namespace_mixin_expansions_.find(key);

	if( it == namespace_mixin_expansions_.end() )
	{
		const auto lexems= PrepareMixinLexems( names_scope, mixin, *mixin_text );
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
				REPORT_ERROR( MixinSyntaxError, names_scope.GetErrors(), mixin.src_loc, error.text );

			return;
		}

		// We need to preserve syntax result, because we store raw pointers to syntax elements.
		it= namespace_mixin_expansions_.emplace( std::make_pair( key, NamespaceMixinExpansionResult{ std::move(synt_result.namespace_elements) } ) ).first;
	}

	NamesScopeFill( names_scope, it->second.program_elements );
}

void CodeBuilder::ExpandClassMixin( const ClassPtr class_type, const Synt::Mixin& mixin )
{
	NamesScope& class_members= *class_type->members;

	const auto mixin_text= EvaluateMixinString( class_members, mixin );
	if( mixin_text == std::nullopt )
		return;

	const MixinExpansionKey key{ mixin.src_loc, mixin_text->str() };
	auto it= class_mixin_expansions_.find(key);

	if( it == class_mixin_expansions_.end() )
	{
		const auto lexems= PrepareMixinLexems( class_members, mixin, *mixin_text );
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
				REPORT_ERROR( MixinSyntaxError, class_members.GetErrors(), mixin.src_loc, error.text );

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

	// TODO - use visibility label of this mixin.
	ClassMemberVisibility visibility= ClassMemberVisibility::Public;

	FillClassNamesScope( class_type, class_name, class_kind, it->second.class_elements, visibility );
}

std::optional<llvm::StringRef> CodeBuilder::EvaluateMixinString( NamesScope& names_scope, const Synt::Mixin& mixin )
{
	VariablePtr variable;
	{
		const StackVariablesStorage dummy_stack_variables_storage( *global_function_context_ );
		variable= BuildExpressionCodeEnsureVariable( mixin.expression, names_scope, *global_function_context_ );
	}

	if( variable->constexpr_value == nullptr )
	{
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), mixin.src_loc );
		return std::nullopt;
	}

	const auto array_type= variable->type.GetArrayType();
	if( array_type == nullptr ||
		array_type->element_type != FundamentalType( U_FundamentalType::char8_, fundamental_llvm_types_.char8_ )  )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), mixin.src_loc, "char8 array", variable->type.ToString() );
		return std::nullopt;
	}

	const auto constant_data= llvm::dyn_cast<llvm::ConstantDataArray>( variable->constexpr_value );
	if( constant_data == nullptr )
	{
		REPORT_ERROR( NotImplemented, names_scope.GetErrors(), mixin.src_loc, "non-trivial mixin constants" );
		return std::nullopt;
	}

	const llvm::StringRef mixin_text= constant_data->getRawDataValues();
	// TODO - check UTF-8 is valid.

	return mixin_text;
}

std::optional<Lexems> CodeBuilder::PrepareMixinLexems( NamesScope& names_scope, const Synt::Mixin& mixin, std::string_view mixin_text )
{
	LexicalAnalysisResult lex_result= LexicalAnalysis( mixin_text );

	// Create new macro expansion context for mixin expansion.
	const uint32_t macro_expansion_index= uint32_t(macro_expansion_contexts_->size());

	{
		Synt::MacroExpansionContext mixin_context;
		mixin_context.macro_declaration_src_loc= mixin.src_loc;
		mixin_context.macro_name= Keyword( Keywords::mixin_ );
		mixin_context.src_loc= mixin.src_loc;

		macro_expansion_contexts_->push_back( std::move(mixin_context) );
	}

	// Use file index of mixin.
	const uint32_t file_index= mixin.src_loc.GetFileIndex();

	// Numerate lines in mixin lexems starting with line of mixin expansion point.
	const uint32_t line_shift= mixin.src_loc.GetLine() - 1;

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
