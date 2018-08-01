#include "assert.hpp"

#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

NamesScope::InsertedName* CodeBuilder::ResolveName( const FilePos& file_pos, NamesScope& names_scope, const Synt::ComplexName& complex_name, const ResolveMode resolve_mode )
{
	return ResolveName( file_pos, names_scope, complex_name.components.data(), complex_name.components.size(), resolve_mode );
}

NamesScope::InsertedName* CodeBuilder::ResolveName(
	const FilePos& file_pos,
	NamesScope& names_scope,
	const Synt::ComplexName::Component* components,
	size_t component_count,
	const ResolveMode resolve_mode  )
{
	U_ASSERT( component_count > 0u );

	NamesScope* last_space= &names_scope;
	if( components[0].name.empty() )
	{
		U_ASSERT( component_count >= 2u );
		last_space= names_scope.GetRoot();
		++components;
		--component_count;
	}
	else
	{
		const ProgramString& start= components[0].name;
		NamesScope* space= &names_scope;
		while(true)
		{
			NamesScope::InsertedName* const find= space->GetThisScopeName( start );
			if( find != nullptr )
				break;
			space= space->GetParent();
			if( space == nullptr )
				return nullptr;
		}
		last_space= space;
	}

	NamesScope::InsertedName* name= nullptr;
	while( true )
	{
		name= last_space->GetThisScopeName( components[0].name );
		if( name == nullptr )
			return nullptr;

		if( components[0].have_template_parameters && name->second.GetTypeTemplatesSet() == nullptr && name->second.GetFunctionsSet() == nullptr )
		{
			errors_.push_back( ReportValueIsNotTemplate( file_pos ) );
			return nullptr;
		}

		NamesScope* next_space= nullptr;
		ClassProxyPtr next_space_class= nullptr;

		if( const NamesScopePtr inner_namespace= name->second.GetNamespace() )
			next_space= inner_namespace.get();
		else if( const Type* const type= name->second.GetTypeName() )
		{
			if( Class* const class_= type->GetClassType() )
			{
				if( component_count >= 2u )
				{
					if( class_->syntax_element != nullptr && class_->syntax_element->is_forward_declaration_ )
					{
						errors_.push_back( ReportUsingIncompleteType( file_pos, type->ToString() ) );
						return nullptr;
					}
					if( resolve_mode != ResolveMode::ForDeclaration )
						GlobalThingBuildClass( type->GetClassTypeProxy(), TypeCompleteness::Complete );
				}
				next_space= &class_->members;
				next_space_class= type->GetClassTypeProxy();
			}
			else if( EnumPtr const enum_= type->GetEnumTypePtr() )
			{
				GlobalThingBuildEnum( enum_, TypeCompleteness::Complete );
				next_space= &enum_->members;
			}
		}
		else if( TypeTemplatesSet* const type_templates_set = name->second.GetTypeTemplatesSet() )
		{
			GlobalThingBuildTypeTemplatesSet( *last_space, *type_templates_set );
			if( components[0].have_template_parameters && !( resolve_mode == ResolveMode::ForTemplateSignatureParameter && component_count == 1u ) )
			{
				NamesScope::InsertedName* generated_type=
					GenTemplateType(
						file_pos,
						*type_templates_set,
						components[0].template_parameters,
						names_scope );
				if( generated_type == nullptr )
					return nullptr;

				const Type* const type= generated_type->second.GetTypeName();
				U_ASSERT( type != nullptr );
				if( Class* const class_= type->GetClassType() )
				{
					next_space= &class_->members;
					next_space_class= type->GetClassTypeProxy();
				}
				name= generated_type;
			}
			else if( component_count >= 2u )
			{
				errors_.push_back( ReportTemplateInstantiationRequired( file_pos, type_templates_set->type_templates.front()->syntax_element->name_ ) );
				return nullptr;
			}
		}
		else if( OverloadedFunctionsSet* const functions_set= name->second.GetFunctionsSet() )
		{
			if( resolve_mode != ResolveMode::ForDeclaration )
				GlobalThingBuildFunctionsSet( *last_space, *functions_set, false );
			if( components[0].have_template_parameters )
			{
				if( functions_set->template_functions.empty() )
				{
					errors_.push_back( ReportValueIsNotTemplate( file_pos ) );
					return nullptr;
				}

				name=
					GenTemplateFunctionsUsingTemplateParameters(
						file_pos,
						functions_set->template_functions,
						components[0].template_parameters,
						names_scope );
			}
		}

		if( component_count == 1u )
			break;
		else if( next_space != nullptr )
		{
			name= next_space->GetThisScopeName( components[1].name );

			if( next_space_class != nullptr && resolve_mode != ResolveMode::ForDeclaration &&
				names_scope.GetAccessFor( next_space_class ) < next_space_class->class_->GetMemberVisibility( components[1].name ) )
				errors_.push_back( ReportAccessingNonpublicClassMember( file_pos, next_space_class->class_->members.GetThisNamespaceName(), components[1].name ) );
		}
		else
			return nullptr;

		++components;
		--component_count;
		last_space= next_space;
	}

	if( name != nullptr && name->second.GetType() == NontypeStub::YetNotDeducedTemplateArg )
		errors_.push_back( ReportTemplateArgumentIsNotDeducedYet( file_pos, name == nullptr ? ""_SpC : name->first ) );

	// Complete some things in resolve.
	if( name != nullptr && resolve_mode != ResolveMode::ForDeclaration )
	{
		if( OverloadedFunctionsSet* const functions_set= name->second.GetFunctionsSet() )
			GlobalThingBuildFunctionsSet( *last_space, *functions_set, false );
		else if( TypeTemplatesSet* const type_templates_set= name->second.GetTypeTemplatesSet() )
			GlobalThingBuildTypeTemplatesSet( *last_space, *type_templates_set );
		else if( name->second.GetTypedef() != nullptr )
			GlobalThingBuildTypedef( *last_space, name->second );
		else if( name->second.GetIncompleteGlobalVariable() != nullptr )
			GlobalThingBuildVariable( *last_space, name->second );
	}
	return name;
}

} // namespace CodeBuilderPrivate

} // namespace U
