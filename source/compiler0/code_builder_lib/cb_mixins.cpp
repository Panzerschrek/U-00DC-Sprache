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
						ExpandMixins_r( *class_type->members );
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
		ExpandMixins( names_scope, *mixins );
}

void CodeBuilder::ExpandMixins( NamesScope& names_scope, Mixins& mixins )
{
	// Avoid using iterator-based foreach, in order to handle container additions in recursive calls.
	for( size_t i= 0; i < mixins.syntax_elements.size(); ++i )
		ExpandMixin( names_scope, *mixins.syntax_elements[i] );

	// Clear the container, since all mixins from it were already expanded.
	mixins.syntax_elements.clear();
}

void CodeBuilder::ExpandMixin( NamesScope& names_scope, const Synt::Mixin& mixin )
{
	VariablePtr variable;
	{
		const StackVariablesStorage dummy_stack_variables_storage( *global_function_context_ );
		variable= BuildExpressionCodeEnsureVariable( mixin.expression, names_scope, *global_function_context_ );
	}

	if( variable->constexpr_value == nullptr )
	{
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), mixin.src_loc );
		return;
	}

	const auto array_type= variable->type.GetArrayType();
	if( array_type == nullptr ||
		array_type->element_type != FundamentalType( U_FundamentalType::char8_, fundamental_llvm_types_.char8_ )  )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), mixin.src_loc, "char8 array", variable->type.ToString() );
		return;
	}

	const auto constant_data= llvm::dyn_cast<llvm::ConstantDataArray>( variable->constexpr_value );
	if( constant_data == nullptr )
	{
		REPORT_ERROR( NotImplemented, names_scope.GetErrors(), mixin.src_loc, "non-trivial mixin constants" );
		return;
	}

	const llvm::StringRef mixin_text= constant_data->getRawDataValues();

	const MixinExpansionKey key{ mixin.src_loc, mixin_text.str() };
	auto it= mixin_expansions_.find(key);

	if( it == mixin_expansions_.end() )
	{
		// TODO - check if is valid UTF-8.

		const LexicalAnalysisResult lex_result= LexicalAnalysis( StringRefToStringView( mixin_text ) );
		// TODO - setup file index and macro expansion context.
		if( !lex_result.errors.empty() )
		{
			for( const LexSyntError& error : lex_result.errors )
				REPORT_ERROR( MixinLexicalError, names_scope.GetErrors(), mixin.src_loc, error.text );

			return;
		}

		// TODO - handle wrong file index.
		const SourceGraph::Node& source_graph_node= source_graph_->nodes_storage[ mixin.src_loc.GetFileIndex() ];

		// TODO - use different functions for mixins in different contexts.
		Synt::SyntaxAnalysisResult synt_result=
			Synt::ParseNamespaceElements(
				lex_result.lexems,
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
		it= mixin_expansions_.emplace( std::make_pair( key, MixinExpansionResult{ std::move(synt_result.program_elements) } ) ).first;
	}

	MixinExpansionResult& mixin_expansion_result= it->second;

	NamesScopeFill( names_scope, mixin_expansion_result.program_elements );
}

} // namespace U
