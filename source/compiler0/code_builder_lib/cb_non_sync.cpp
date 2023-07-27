#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"

namespace U
{

bool CodeBuilder::GetTypeNonSync( const Type& type, NamesScope& names_scope, const SrcLoc& src_loc )
{
	size_t loop_start= non_sync_expression_stack_.size();
	for( const Type& prev_type : non_sync_expression_stack_ )
	{
		if( type == prev_type )
		{
			loop_start= size_t( &prev_type - non_sync_expression_stack_.data() );
			break;
		}
	}

	if( loop_start < non_sync_expression_stack_.size() )
	{
		std::string description;

		for( size_t i= loop_start; i < non_sync_expression_stack_.size(); ++i )
		{
			description += Keyword( Keywords::non_sync_ );
			description += "</";
			description += non_sync_expression_stack_[i].ToString();
			description += "/>";
			description += " -> ";
		}

		description += Keyword( Keywords::non_sync_ );
		description += "</";
		description += type.ToString();
		description += "/>";

		REPORT_ERROR( GlobalsLoopDetected, global_errors_, src_loc, description );
		return false;
	}

	non_sync_expression_stack_.push_back(type);

	llvm::SmallVector<Type, 4> types_stack;
	const bool result= GetTypeNonSyncImpl( types_stack, type, names_scope, src_loc );
	U_ASSERT( types_stack.empty() ); // Should be empty at end.

	non_sync_expression_stack_.pop_back();
	return result;
}

// TODO - cache results.
bool CodeBuilder::GetTypeNonSyncImpl( llvm::SmallVectorImpl<Type>& prev_types_stack, const Type& type, NamesScope& names_scope, const SrcLoc& src_loc )
{
	// Simple non-recursive types without "non_sync" tag.
	if( type.GetFundamentalType() != nullptr ||
		type.GetFunctionPointerType() != nullptr ||
		type.GetRawPointerType() != nullptr ||
		type.GetEnumType() != nullptr )
		return false;

	// Break recursive dependency. It is fine, since "x = y || x" is equivalent to " x = y || false".
	for( const Type& prev_type : prev_types_stack )
	{
		if( type == prev_type )
			return false;
	}

	// Do not forget to call "pop_back" before return!
	prev_types_stack.push_back( type );

	if( const auto array_type= type.GetArrayType() )
	{
		const bool is_non_sync= GetTypeNonSyncImpl( prev_types_stack, array_type->element_type, names_scope, src_loc );
		prev_types_stack.pop_back();
		return is_non_sync;
	}
	if( const auto tuple_type= type.GetTupleType() )
	{
		for( const Type& element_type : tuple_type->element_types )
		{
			if( GetTypeNonSyncImpl( prev_types_stack, element_type, names_scope, src_loc ) )
			{
				prev_types_stack.pop_back();
				return true;
			}
		}

		prev_types_stack.pop_back();
		return false;
	}
	if( const auto class_type= type.GetClassType() )
	{
		// Check "non_sync" tag existence first.
		if( class_type->syntax_element != nullptr )
		{
			if( std::get_if<Synt::NonSyncTagNone>( &class_type->syntax_element->non_sync_tag_ ) != nullptr )
			{}
			else if( std::get_if<Synt::NonSyncTagTrue>( &class_type->syntax_element->non_sync_tag_ ) != nullptr )
			{
				prev_types_stack.pop_back();
				return true;
			}
			else if( const auto expression_ptr= std::get_if<Synt::ExpressionPtr>( &class_type->syntax_element->non_sync_tag_ ) )
			{
				const Synt::Expression& expression= **expression_ptr;

				// Evaluate non_sync condition using initial class members parent scope.
				NamesScope& class_parent_scope= *class_type->members_initial->GetParent();
				if( const auto non_sync_expression= std::get_if<Synt::NonSyncExpression>( &expression ) )
				{
					// Process "non_sync</T/>" expression specially to handle cases with recursive dependencies.
					// TODO - handle also simple logical expressions with "non_sync" tag?
					const Type dependent_type= PrepareType( *non_sync_expression->type_, class_parent_scope, *global_function_context_ );
					if( GetTypeNonSyncImpl( prev_types_stack, dependent_type, names_scope, src_loc ) )
					{
						prev_types_stack.pop_back();
						return true;
					}
				}
				else
				{
					// Process general non_sync expression. This approach can't resolve circular dependency.
					if( EvaluateBoolConstantExpression( class_parent_scope, *global_function_context_, expression ) )
					{
						prev_types_stack.pop_back();
						return true;
					}
				}
			}
			else{ U_ASSERT(false); } // Unhandled non_sync tag kind.
		}

		// Check coroutines non_sync flag.
		if( class_type->coroutine_type_description != std::nullopt && class_type->coroutine_type_description->non_sync )
		{
			prev_types_stack.pop_back();
			return true;
		}

		// Check "non_sync" tag existence for parents.

		GlobalThingPrepareClassParentsList( class_type );

		for( const Class::Parent& parent : class_type->parents )
		{
			if( GetTypeNonSyncImpl( prev_types_stack, parent.class_, names_scope, src_loc ) )
			{
				prev_types_stack.pop_back();
				return true;
			}
		}

		// Check "non_sync" tag existence for fields. Type completion is required for this.

		if( !EnsureTypeComplete( type ) )
		{
			REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), src_loc, type );
			prev_types_stack.pop_back();
			return false;
		}

		for( const std::string& field_name : class_type->fields_order )
		{
			if( const auto value= class_type->members->GetThisScopeValue( field_name ) )
			{
				if( const auto class_field= value->GetClassField() )
				{
					// Check non_sync tag for both reference and non-reference fields.
					if( GetTypeNonSyncImpl( prev_types_stack, class_field->type, names_scope, src_loc ) )
					{
						prev_types_stack.pop_back();
						return true;
					}
				}
			}
		}

		prev_types_stack.pop_back();
		return false;
	}

	// Unhandled type kind.
	U_ASSERT(false);
	prev_types_stack.pop_back();
	return false;
}

bool CodeBuilder::ImmediateEvaluateNonSyncTag( NamesScope& names, FunctionContext& function_context, const Synt::NonSyncTag& non_sync_tag )
{
	if( std::get_if<Synt::NonSyncTagNone>( &non_sync_tag ) != nullptr )
		return false;
	if( std::get_if<Synt::NonSyncTagTrue>( &non_sync_tag ) != nullptr )
		return true;
	if( const auto expression_ptr= std::get_if<Synt::ExpressionPtr>( &non_sync_tag ) )
		return EvaluateBoolConstantExpression( names, function_context, **expression_ptr );
	U_ASSERT(false); // Unhandled non_sync tag kind.
	return false;
}

void CodeBuilder::CheckClassNonSyncTagExpression( const ClassPtr class_type )
{
	if( class_type->syntax_element != nullptr )
	{
		// Evaluate non_sync condition using initial class members parent scope.
		ImmediateEvaluateNonSyncTag(  *class_type->members_initial->GetParent(), *global_function_context_, class_type->syntax_element->non_sync_tag_ );
	}
}

void CodeBuilder::CheckClassNonSyncTagInheritance( const ClassPtr class_type )
{
	// Forbid changing "non_sync" tag in inheritance.
	// If class is "non_sync" all its parents should be "non_sync".
	// Do this in order to prevent "non_sync" tag disappearing when storing derived class in container (box, box_nullable) for base class.
	// "non_sync" tag presence is strongly necessary to statically (during compilation) prevent usage of "non_sync" classes in multithreaded context.

	if( class_type->parents.empty() )
		return;

	SrcLoc src_loc;
	if( class_type->syntax_element != nullptr )
		src_loc= class_type->syntax_element->src_loc_;

	if( !GetTypeNonSync( class_type, *class_type->members->GetParent(), src_loc ) )
		return;

	for( const Class::Parent& parent : class_type->parents )
	{
		const ClassPtr parent_class_type= parent.class_;

		SrcLoc parent_src_loc;
		if( parent_class_type->syntax_element != nullptr )
			parent_src_loc= parent_class_type->syntax_element->src_loc_;

		if( !GetTypeNonSync( parent_class_type, *parent_class_type->members->GetParent(), parent_src_loc ) )
		{
			REPORT_ERROR( NonSyncTagAdditionInInheritance, class_type->members->GetErrors(), src_loc, Type(class_type), Type(parent_class_type) );
		}
	}
}

} // namespace U
