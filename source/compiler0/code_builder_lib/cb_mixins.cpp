#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/ConvertUTF.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../../code_builder_lib_common/string_ref.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "../lex_synt_lib/lexical_analyzer.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"

namespace U
{

namespace
{

constexpr uint32_t g_max_mixins_depth= 4;

} // namespace

void CodeBuilder::ProcessMixins( NamesScope& names_scope )
{
	// Perform several iterations in order to process mixins within mixins.
	for( uint32_t i= 0; i < g_max_mixins_depth; ++i )
	{
		// First evaluate all expressions. Doing so we prevent symbols produced in expansion of one mixin visible in expression of another.
		const uint32_t num_expressions= EvaluateMixinsExpressions_r( names_scope );
		if( num_expressions == 0 )
			break;

		if( i == g_max_mixins_depth - 1 )
			REPORT_ERROR( MixinExpansionDepthReached, names_scope.GetErrors(), SrcLoc( 0, 1, 0 ) );

		// Populate name scopes using expressions evaluated on previous step.
		ExpandNamespaceMixins_r( names_scope );
	}
}

uint32_t CodeBuilder::EvaluateMixinsExpressions_r( NamesScope& names_scope )
{
	uint32_t result= 0;

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
				result+= uint32_t(mixins->size());
				for( Mixin& mixin : *mixins )
					EvaluateMixinExpressionInGlobalContext( names_scope, mixin );
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
	for( uint32_t i= 0; i < g_max_mixins_depth; ++i )
	{
		// First evaluate all expressions.
		const uint32_t num_expressions= EvaluateMixinsExpressions_r( *class_type->members );
		if( num_expressions == 0 )
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

	const std::string_view mixin_text= StringRefToStringView( mixin.string_constant->getRawDataValues() );
	mixin.string_constant= nullptr;

	MixinExpansionKey key{ mixin.src_loc, std::string(mixin_text) };
	auto it= namespace_mixin_expansions_.find(key);

	if( it == namespace_mixin_expansions_.end() )
	{
		const auto lexems= PrepareMixinLexems( names_scope, mixin.src_loc, mixin_text );
		if( lexems == std::nullopt )
			return;

		U_ASSERT( mixin.src_loc.GetFileIndex() < source_graph_->nodes_storage.size() );
		const SourceGraph::Node& source_graph_node= source_graph_->nodes_storage[ mixin.src_loc.GetFileIndex() ];

		Synt::NamespaceParsingResult synt_result=
			Synt::ParseNamespaceElements(
				*lexems,
				source_graph_node.ast.macros, // Macros should not be modified.
				source_graph_->macro_expansion_contexts, // Populate contexts, if necessary.
				source_graph_node.contents_hash );

		if( !synt_result.error_messages.empty() )
		{
			for( const LexSyntError& error : synt_result.error_messages )
				REPORT_ERROR( MixinSyntaxError, names_scope.GetErrors(), error.src_loc, error.text );

			return;
		}

		// We need to preserve syntax result, because we store raw pointers to syntax elements.
		it= namespace_mixin_expansions_.emplace( std::move(key), std::move(synt_result.namespace_elements) ).first;
	}

	NamesScopeFill( names_scope, it->second );
	NamesScopeFillOutOfLineElements( names_scope, it->second );
}

void CodeBuilder::ExpandClassMixin( const ClassPtr class_type, Mixin& mixin )
{
	NamesScope& class_members= *class_type->members;

	if( mixin.string_constant == nullptr )
		return;

	const std::string_view mixin_text= StringRefToStringView( mixin.string_constant->getRawDataValues() );
	mixin.string_constant= nullptr;

	MixinExpansionKey key{ mixin.src_loc, std::string(mixin_text) };
	auto it= class_mixin_expansions_.find(key);

	if( it == class_mixin_expansions_.end() )
	{
		const auto lexems= PrepareMixinLexems( class_members, mixin.src_loc, mixin_text );
		if( lexems == std::nullopt )
			return;

		U_ASSERT( mixin.src_loc.GetFileIndex() < source_graph_->nodes_storage.size() );
		const SourceGraph::Node& source_graph_node= source_graph_->nodes_storage[ mixin.src_loc.GetFileIndex() ];

		Synt::ClassElementsParsingResult synt_result=
			Synt::ParseClassElements(
				*lexems,
				source_graph_node.ast.macros, // Macros should not be modified.
				source_graph_->macro_expansion_contexts, // Populate contexts, if necessary.
				source_graph_node.contents_hash );

		if( !synt_result.error_messages.empty() )
		{
			for( const LexSyntError& error : synt_result.error_messages )
				REPORT_ERROR( MixinSyntaxError, class_members.GetErrors(), error.src_loc, error.text );

			return;
		}

		// We need to preserve syntax result, because we store raw pointers to syntax elements.
		it= class_mixin_expansions_.emplace( std::move(key), std::move(synt_result.class_elements) ).first;
	}

	Synt::ClassKindAttribute class_kind= Synt::ClassKindAttribute::Struct;
	std::string_view class_name;
	if( class_type->syntax_element != nullptr )
	{
		class_kind= class_type->syntax_element->kind_attribute;
		class_name= class_type->syntax_element->name;
	}
	else
		class_name= class_members.GetThisNamespaceName();

	FillClassNamesScope( class_type, class_name, class_kind, it->second, mixin.visibility );
}

const Synt::BlockElementsList* CodeBuilder::ExpandBlockMixin( NamesScope& names_scope, FunctionContext& function_context, const Synt::Mixin& mixin )
{
	Mixin temp_mixin;
	temp_mixin.syntax_element= &mixin;
	temp_mixin.src_loc= mixin.src_loc;

	EvaluateMixinExpression( names_scope, function_context, temp_mixin );

	if( temp_mixin.string_constant == nullptr )
		return nullptr;

	const std::string_view mixin_text= StringRefToStringView( temp_mixin.string_constant->getRawDataValues() );

	MixinExpansionKey key{ mixin.src_loc, std::string(mixin_text) };
	auto it= block_mixin_expansions_.find(key);

	if( it == block_mixin_expansions_.end() )
	{
		const auto lexems= PrepareMixinLexems( names_scope, mixin.src_loc, mixin_text );
		if( lexems == std::nullopt )
			return nullptr;

		U_ASSERT( mixin.src_loc.GetFileIndex() < source_graph_->nodes_storage.size() );
		const SourceGraph::Node& source_graph_node= source_graph_->nodes_storage[ mixin.src_loc.GetFileIndex() ];

		Synt::BlockElementsParsingResult synt_result=
			Synt::ParseBlockElements(
				*lexems,
				source_graph_node.ast.macros, // Macros should not be modified.
				source_graph_->macro_expansion_contexts, // Populate contexts, if necessary.
				source_graph_node.contents_hash );

		if( !synt_result.error_messages.empty() )
		{
			for( const LexSyntError& error : synt_result.error_messages )
				REPORT_ERROR( MixinSyntaxError, names_scope.GetErrors(), error.src_loc, error.text );

			return nullptr;
		}

		// We need to preserve syntax result, because we store raw pointers to syntax elements.
		it= block_mixin_expansions_.emplace( std::move(key), std::move(synt_result.block_elements) ).first;
	}

	return &it->second;
}

const Synt::TypeName* CodeBuilder::ExpandTypeNameMixin( NamesScope& names_scope, FunctionContext& function_context, const Synt::Mixin& mixin )
{
	Mixin temp_mixin;
	temp_mixin.syntax_element= &mixin;
	temp_mixin.src_loc= mixin.src_loc;

	EvaluateMixinExpression( names_scope, function_context, temp_mixin );

	const std::string_view mixin_text=
		temp_mixin.string_constant == nullptr
			? std::string_view()
			: StringRefToStringView( temp_mixin.string_constant->getRawDataValues() );

	MixinExpansionKey key{ mixin.src_loc, std::string(mixin_text) };
	auto it= type_name_mixin_expansions_.find(key);

	if( it == type_name_mixin_expansions_.end() )
	{
		const auto lexems= PrepareMixinLexems( names_scope, mixin.src_loc, mixin_text );
		if( lexems == std::nullopt )
			return nullptr;

		U_ASSERT( mixin.src_loc.GetFileIndex() < source_graph_->nodes_storage.size() );
		const SourceGraph::Node& source_graph_node= source_graph_->nodes_storage[ mixin.src_loc.GetFileIndex() ];

		Synt::TypeNameParsingResult synt_result=
			Synt::ParseTypeName(
				*lexems,
				source_graph_node.ast.macros, // Macros should not be modified.
				source_graph_->macro_expansion_contexts, // Populate contexts, if necessary.
				source_graph_node.contents_hash );

		if( !synt_result.error_messages.empty() )
		{
			for( const LexSyntError& error : synt_result.error_messages )
				REPORT_ERROR( MixinSyntaxError, names_scope.GetErrors(), error.src_loc, error.text );

			return nullptr;
		}

		// We need to preserve syntax result, because we store raw pointers to syntax elements.
		it= type_name_mixin_expansions_.emplace( std::move(key), std::move(synt_result.type_name) ).first;
	}

	return &it->second;
}

const Synt::Expression* CodeBuilder::ExpandExpressionMixin( NamesScope& names_scope, FunctionContext& function_context, const Synt::Mixin& mixin )
{
	Mixin temp_mixin;
	temp_mixin.syntax_element= &mixin;
	temp_mixin.src_loc= mixin.src_loc;

	EvaluateMixinExpression( names_scope, function_context, temp_mixin );

	const std::string_view mixin_text=
		temp_mixin.string_constant == nullptr
			? std::string_view()
			: StringRefToStringView( temp_mixin.string_constant->getRawDataValues() );

	MixinExpansionKey key{ mixin.src_loc, std::string(mixin_text) };
	auto it= expression_mixin_expansions_.find(key);

	if( it == expression_mixin_expansions_.end() )
	{
		const auto lexems= PrepareMixinLexems( names_scope, mixin.src_loc, mixin_text );
		if( lexems == std::nullopt )
			return nullptr;

		U_ASSERT( mixin.src_loc.GetFileIndex() < source_graph_->nodes_storage.size() );
		const SourceGraph::Node& source_graph_node= source_graph_->nodes_storage[ mixin.src_loc.GetFileIndex() ];

		Synt::ExpressionParsingResult synt_result=
			Synt::ParseExpression(
				*lexems,
				source_graph_node.ast.macros, // Macros should not be modified.
				source_graph_->macro_expansion_contexts, // Populate contexts, if necessary.
				source_graph_node.contents_hash );

		if( !synt_result.error_messages.empty() )
		{
			for( const LexSyntError& error : synt_result.error_messages )
				REPORT_ERROR( MixinSyntaxError, names_scope.GetErrors(), error.src_loc, error.text );

			return nullptr;
		}

		// We need to preserve syntax result, because we store raw pointers to syntax elements.
		it= expression_mixin_expansions_.emplace( std::move(key), std::move(synt_result.expression) ).first;
	}

	return &it->second;
}

void CodeBuilder::EvaluateMixinExpressionInGlobalContext( NamesScope& names_scope, Mixin& mixin )
{
	WithGlobalFunctionContext(
		[&]( FunctionContext& function_context )
		{
			EvaluateMixinExpression( names_scope, function_context, mixin );
		} );
}

void CodeBuilder::EvaluateMixinExpression( NamesScope& names_scope, FunctionContext& function_context, Mixin& mixin )
{
	if( mixin.syntax_element == nullptr || mixin.string_constant != nullptr )
		return;

	const Synt::Mixin& syntax_element= *mixin.syntax_element;

	const VariablePtr variable= BuildExpressionCodeEnsureVariable( syntax_element.expression, names_scope, function_context );

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

	if( const auto constant_data= llvm::dyn_cast<llvm::ConstantDataArray>( variable->constexpr_value ) )
	{
		const llvm::StringRef mixin_text= constant_data->getRawDataValues();
		auto ptr= reinterpret_cast<const llvm::UTF8*>(mixin_text.data());
		if( !llvm::isLegalUTF8String( &ptr, ptr + mixin_text.size() ) )
		{
			REPORT_ERROR( MixinInvalidUTF8, names_scope.GetErrors(), syntax_element.src_loc );
			return;
		}

		mixin.string_constant= constant_data;
	}
	else if( llvm::isa<llvm::ConstantAggregateZero>( variable->constexpr_value ) )
	{
		// Ignore mixins containing zeros. Their expansion leads to nothing.
	}
	else
		REPORT_ERROR( NotImplemented, names_scope.GetErrors(), syntax_element.src_loc, "non-trivial mixin constants" );
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

	// Use file index of the mixin for lexems in its expansion.
	const uint32_t file_index= src_loc.GetFileIndex();

	// Numerate lines in mixin lexems starting with line of the mixin expansion point.
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
