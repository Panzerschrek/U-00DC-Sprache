#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"

namespace U
{

bool CodeBuilder::GetTypeShared( const Type& type, NamesScope& names_scope, const SrcLoc& src_loc )
{
	size_t loop_start= shared_expression_stack_.size();
	for( const Type& prev_type : shared_expression_stack_ )
	{
		if( type == prev_type )
		{
			loop_start = size_t( &prev_type - shared_expression_stack_.data() );
			break;
		}
	}

	if( loop_start < shared_expression_stack_.size() )
	{
		std::string description;

		for( size_t i= loop_start; i < shared_expression_stack_.size(); ++i )
		{
			description += Keyword( Keywords::shared_ );
			description += "</";
			description += shared_expression_stack_[i].ToString();
			description += "/>";
			description += " -> ";
		}

		description += Keyword( Keywords::shared_ );
		description += "</";
		description += type.ToString();
		description += "/>";

		REPORT_ERROR( GlobalsLoopDetected, global_errors_, src_loc, description );
		return false;
	}

	shared_expression_stack_.push_back(type);

	std::vector<Type> types_stack;
	const bool result= GetTypeSharedImpl( types_stack, type, names_scope, src_loc );

	shared_expression_stack_.pop_back();
	return result;
}

// TODO - cache results.
bool CodeBuilder::GetTypeSharedImpl( std::vector<Type>& prev_types_stack, const Type& type, NamesScope& names_scope, const SrcLoc& src_loc )
{
	// Simple non-recursive types without "shared" tag.
	if( type.GetFundamentalType() != nullptr ||
		type.GetFunctionType() != nullptr ||
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
		const bool is_shared= GetTypeSharedImpl( prev_types_stack, array_type->type, names_scope, src_loc );
		prev_types_stack.pop_back();
		return is_shared;
	}
	if( const auto tuple_type= type.GetTupleType() )
	{
		for( const Type& element_type : tuple_type->elements )
		{
			if( GetTypeSharedImpl( prev_types_stack, element_type, names_scope, src_loc ) )
			{
				prev_types_stack.pop_back();
				return true;
			}
		}

		prev_types_stack.pop_back();
		return false;
	}
	if( const auto class_type = type.GetClassType() )
	{
		// Check shared tag existence first.
		if( class_type->syntax_element != nullptr )
		{
			if( std::get_if<Synt::SharedTagNone>( &class_type->syntax_element->shared_tag_ ) != nullptr )
			{}
			else if( std::get_if<Synt::SharedTagTrue>( &class_type->syntax_element->shared_tag_ ) != nullptr )
			{
				prev_types_stack.pop_back();
				return true;
			}
			else if( const auto expression_ptr = std::get_if<std::unique_ptr<Synt::Expression>>( &class_type->syntax_element->shared_tag_ ) )
			{
				// Evaluate shared condition using initial class members parent scope.
				NamesScope& class_parent_scope= *class_type->members_initial->GetParent();
				if( const auto shared_expression = std::get_if<Synt::SharedExpression>( &**expression_ptr ) )
				{
					// Process "shared</T/>" expression specially to handle cases with recursive dependencies.
					// TODO - handle also simple logical expressions with "shared" tag?
					const Type dependent_type= PrepareType( *shared_expression->type_, class_parent_scope, *global_function_context_ );
					if( GetTypeSharedImpl( prev_types_stack, dependent_type, names_scope, src_loc ) )
					{
						prev_types_stack.pop_back();
						return true;
					}
				}
				else
				{
					// Process general shared expression. This approach can'r resolve circular dependency.
					const Variable v= BuildExpressionCodeEnsureVariable( **expression_ptr, class_parent_scope, *global_function_context_ );
					if( v.type != bool_type_ )
					{
						REPORT_ERROR( TypesMismatch, class_parent_scope.GetErrors(), Synt::GetExpressionSrcLoc( **expression_ptr ), bool_type_, v.type );
					}
					else if( v.constexpr_value == nullptr )
					{
						REPORT_ERROR( ExpectedConstantExpression, class_parent_scope.GetErrors(), Synt::GetExpressionSrcLoc( **expression_ptr ) );
					}
					else if( !v.constexpr_value->isZeroValue() )
					{
						prev_types_stack.pop_back();
						return true;
					}
				}
			}
		}

		// Check "shared" tag existence for parents.

		GlobalThingPrepareClassParentsList( class_type );

		for( const Class::Parent& parent : class_type->parents )
		{
			if( GetTypeSharedImpl( prev_types_stack, parent.class_, names_scope, src_loc ) )
			{
				prev_types_stack.pop_back();
				return true;
			}
		}

		// Check "shared" tag existence for fields. Type completion is required for this.

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
				if( const auto class_field = value->GetClassField() )
				{
					// Check shared tag for both reference and non-reference fields.
					if( GetTypeSharedImpl( prev_types_stack, class_field->type, names_scope, src_loc ) )
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

void CodeBuilder::CheckClassSharedTagExpression( const ClassPtr class_type )
{
	if( class_type->syntax_element != nullptr )
	{
		if( const auto expression_ptr = std::get_if<std::unique_ptr<Synt::Expression>>( &class_type->syntax_element->shared_tag_ ) )
		{
			// Evaluate shared condition using initial class members parent scope.
			NamesScope& class_parent_scope= *class_type->members_initial->GetParent();

			const Variable v= BuildExpressionCodeEnsureVariable( **expression_ptr, class_parent_scope, *global_function_context_ );
			if( v.type != bool_type_ )
			{
				REPORT_ERROR( TypesMismatch, class_parent_scope.GetErrors(), Synt::GetExpressionSrcLoc( **expression_ptr ), bool_type_, v.type );
			}
			else if( v.constexpr_value == nullptr )
			{
				REPORT_ERROR( ExpectedConstantExpression, class_parent_scope.GetErrors(), Synt::GetExpressionSrcLoc( **expression_ptr ) );
			}
		}
	}
}

} // namespace U
