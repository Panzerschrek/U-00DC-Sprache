#include "../../lex_synt_lib_common/assert.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"

namespace U
{

// TODO - process case with recursion.
// TODO - cache results.
bool CodeBuilder::GetTypeShared( const Type& type, NamesScope& names_scope, const SrcLoc& src_loc )
{
	if( type.GetFundamentalType() != nullptr ||
		type.GetFunctionType() != nullptr ||
		type.GetFunctionPointerType() != nullptr ||
		type.GetRawPointerType() != nullptr ||
		type.GetEnumType() != nullptr )
		return false;
	if( const auto array_type= type.GetArrayType() )
		return GetTypeShared( array_type->type, names_scope, src_loc );
	if( const auto tuple_type= type.GetTupleType() )
	{
		for( const Type& element_type : tuple_type->elements )
		{
			if( GetTypeShared( element_type, names_scope, src_loc ) )
				return true;
		}
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
				return true;
			else if( const auto expression_ptr = std::get_if<std::unique_ptr<Synt::Expression>>( &class_type->syntax_element->shared_tag_ ) )
			{
				// Evaluate shared condition using initial class members parent scope.
				NamesScope& class_parent_scope= *class_type->members_initial->GetParent();
				const Variable v= BuildExpressionCodeEnsureVariable( **expression_ptr, class_parent_scope, *global_function_context_ );
				if( v.type != bool_type_ )
				{
					REPORT_ERROR( TypesMismatch, class_parent_scope.GetErrors(), Synt::GetExpressionSrcLoc( **expression_ptr ), bool_type_, v.type );
					return false;
				}
				if( v.constexpr_value == nullptr )
				{
					REPORT_ERROR( ExpectedConstantExpression, class_parent_scope.GetErrors(), Synt::GetExpressionSrcLoc( **expression_ptr ) );
					return false;
				}

				if( !v.constexpr_value->isZeroValue() )
					return true;
			}
		}

		// Check "shared" tag existence for parents.

		GlobalThingPrepareClassParentsList( class_type );

		for( const Class::Parent& parent : class_type->parents )
		{
			if( GetTypeShared( parent.class_, names_scope, src_loc ) )
				return true;
		}

		// Check "shared" tag existence for fields. Type completion is required for this.

		if( !EnsureTypeComplete( type ) )
		{
			REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), src_loc, type );
			return false;
		}

		for( const std::string& field_name : class_type->fields_order )
		{
			if( const auto value= class_type->members->GetThisScopeValue( field_name ) )
			{
				if( const auto class_field = value->GetClassField() )
				{
					// Check shared tag for both reference and non-reference fields.
					if( GetTypeShared( class_field->type, names_scope, src_loc ) )
						return true;
				}
			}
		}

		return false;
	}

	// Unhandled type kind.
	U_ASSERT(false);
	return false;
}

} // namespace U
