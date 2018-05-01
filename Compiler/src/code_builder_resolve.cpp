#include "assert.hpp"

#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

void CodeBuilder::PushCacheFillResolveHandler( ResolvingCache& resolving_cache, NamesScope& start_namespace )
{
	const size_t prev_handler_index= resolving_funcs_stack_.size() - 1u;

	resolving_funcs_stack_.emplace_back( new PreResolveFunc(
		[this, &resolving_cache, &start_namespace, prev_handler_index](
			NamesScope& names_scope,
			const Synt::ComplexName::Component* components,
			size_t component_count,
			size_t& out_skip_components )
			-> std::pair<const NamesScope::InsertedName*, NamesScope*>
		{
			const std::pair<const NamesScope::InsertedName*, NamesScope*> resolve_start_point=
				(*resolving_funcs_stack_[prev_handler_index])( names_scope, components, component_count, out_skip_components );
			if( resolve_start_point.first == nullptr )
				return resolve_start_point;
			U_ASSERT( out_skip_components > 0u && out_skip_components <= component_count );

			// Do not push to cache names from child relative "start_namespace" spaces.
			// If resolve start point parent namespace is null, that, this value recieved from cache and may not be re-cached.
			if( !components[0].is_generated &&
				resolve_start_point.second != nullptr &&
				( resolve_start_point.second == &start_namespace || resolve_start_point.second->IsAncestorFor( start_namespace ) ) )
			{
				NameResolvingKey key;
				key.components= components;
				key.component_count= component_count;

				resolving_cache.emplace(
					std::move(key),
					ResolvingCacheValue{ *resolve_start_point.first, out_skip_components } );
			}

			return resolve_start_point;

		} ) );
}

void CodeBuilder::PushCacheGetResolveHandelr( const ResolvingCache& resolving_cache )
{
	const size_t prev_handler_index= resolving_funcs_stack_.size() - 1u;

	resolving_funcs_stack_.emplace_back( new PreResolveFunc(
		[this, &resolving_cache, prev_handler_index](
			NamesScope& names_scope,
			const Synt::ComplexName::Component* components,
			size_t component_count,
			size_t& out_skip_components )
			-> std::pair<const NamesScope::InsertedName*, NamesScope*>
		{
			NameResolvingKey key;
			key.components= components;
			key.component_count= component_count;
			const auto it= resolving_cache.find(key);
			if( it == resolving_cache.end() )
				return (*resolving_funcs_stack_[prev_handler_index])( names_scope, components, component_count, out_skip_components );

			const ResolvingCacheValue& cache_value= it->second;
			out_skip_components= cache_value.name_components_cut;
			return std::make_pair( &cache_value.name, nullptr );

		} ) );
}

void CodeBuilder::PopResolveHandler()
{
	U_ASSERT( resolving_funcs_stack_.size() >= 2u );
	resolving_funcs_stack_.pop_back();
}

const NamesScope::InsertedName* CodeBuilder::ResolveName( const FilePos& file_pos, NamesScope& names_scope, const Synt::ComplexName& complex_name )
{
	return ResolveName( file_pos, names_scope, complex_name.components.data(), complex_name.components.size() );
}

const NamesScope::InsertedName* CodeBuilder::ResolveName(
	const FilePos& file_pos,
	NamesScope& names_scope,
	const Synt::ComplexName::Component* components,
	size_t component_count )
{
	U_ASSERT( !resolving_funcs_stack_.empty() );

	size_t skip_components= 0u;
	const NamesScope::InsertedName* const resolve_start_point=
		PreResolve( names_scope, components, component_count, skip_components );
	if( resolve_start_point == nullptr )
		return nullptr;
	U_ASSERT( skip_components > 0u && skip_components <= component_count );

	components+= skip_components - 1u;
	component_count-= skip_components - 1u;
	const NamesScope::InsertedName* name= resolve_start_point;
	do
	{
		if( name == nullptr )
			return nullptr;

		if( components[0].have_template_parameters && name->second.GetTypeTemplate() == nullptr && name->second.GetFunctionsSet() == nullptr )
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
				if( component_count >= 2u && class_->is_incomplete )
				{
					errors_.push_back( ReportUsingIncompleteType( file_pos, type->ToString() ) );
					return nullptr;
				}
				next_space= &class_->members;
				next_space_class= type->GetClassTypeProxy();
			}
			else if( Enum* const enum_= type->GetEnumType() )
			{
				next_space= &enum_->members;
			}
		}
		else if( const TypeTemplatePtr type_template = name->second.GetTypeTemplate() )
		{
			if( components[0].have_template_parameters )
			{
				const NamesScope::InsertedName* generated_type=
					GenTemplateType(
						file_pos,
						type_template,
						components[0].template_parameters,
						*type_template->parent_namespace,
						names_scope );
				if( generated_type == nullptr )
					return nullptr;
				if( generated_type->second.GetType() == NontypeStub::TemplateDependentValue )
					return generated_type;

				const Type* const type= generated_type->second.GetTypeName();
				if( type->GetTemplateDependentType() != nullptr )
				{
					if( component_count >= 2u )
						return generated_type; // If this name is last, we know, that this is type
					else
						return &type_template->parent_namespace->GetTemplateDependentValue(); // Else it is something really template-dependent
				}
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
				errors_.push_back( ReportTemplateInstantiationRequired( file_pos, type_template->syntax_element->name_ ) );
				return nullptr;
			}
		}
		else if( const OverloadedFunctionsSet* const functions_set= name->second.GetFunctionsSet() )
		{
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
						*functions_set->template_functions.front()->parent_namespace, // All template functions in one set have one parent namespace.
						names_scope );
			}
		}

		if( component_count == 1u )
			break;
		else if( next_space != nullptr )
		{
			name= next_space->GetThisScopeName( components[1].name );

			if( next_space_class != nullptr )
			{
				const Class& class_= *next_space_class->class_;
				const auto visibility_map_it= class_.members_visibility.find(components[1].name );
				if( visibility_map_it != class_.members_visibility.end() &&
					visibility_map_it->second != Synt::ClassMemberVisibility::Public &&
					!names_scope.HaveAccessTo( next_space_class ) )
					errors_.push_back( ReportAccessingNonpublicClassMember( file_pos, next_space_class->class_->members.GetThisNamespaceName(), components[1].name ) );
			}
		}
		else
			return nullptr;

		++components;
		--component_count;
	} while ( component_count > 0u );

	if( name != nullptr && name->second.GetType() == NontypeStub::YetNotDeducedTemplateArg )
		errors_.push_back( ReportTemplateArgumentIsNotDeducedYet( file_pos, name == nullptr ? ""_SpC : name->first ) );

	return name;
}

const NamesScope::InsertedName* CodeBuilder::PreResolve(
	NamesScope& names_scope,
	const Synt::ComplexName::Component* const components,
	const size_t component_count,
	size_t& out_skip_components )
{
	return (*resolving_funcs_stack_.back())( names_scope, components, component_count, out_skip_components ).first;
}

std::pair<const NamesScope::InsertedName*, NamesScope*> CodeBuilder::PreResolveDefault(
	NamesScope& names_scope,
	const Synt::ComplexName::Component* components,
	size_t component_count,
	size_t& out_skip_components )
{
	out_skip_components= 0u;

	NamesScope* resolve_start_point= nullptr;
	if( components[0].name.empty() )
	{
		U_ASSERT( component_count >= 2u );
		resolve_start_point= const_cast<NamesScope*>(names_scope.GetRoot());
		++components;
		++out_skip_components;
		--component_count;
	}
	else
	{
		const ProgramString& start= components[0].name;
		NamesScope::InsertedName* start_resolved= nullptr;
		NamesScope* space= &names_scope;
		while(true)
		{
			NamesScope::InsertedName* const find= space->GetThisScopeName( start );
			if( find != nullptr )
			{
				start_resolved= find;
				break;
			}
			space= const_cast<NamesScope*>(space->GetParent());
			if( space == nullptr )
				return std::make_pair( nullptr, nullptr );
		}

		resolve_start_point= space;
	}

	// Resolve to first non-namespace.
	while( component_count > 0u )
	{
		NamesScope::InsertedName* const name= resolve_start_point->GetThisScopeName( components[0].name );
		if( name == nullptr )
			return std::make_pair( nullptr, nullptr );

		// TODO - generate error, for :: look inside non-class or namespace.

		if( const NamesScopePtr child_namespace= name->second.GetNamespace() )
		{
			++components;
			++out_skip_components;
			--component_count;
			if( component_count == 0u )
				return std::make_pair( name, resolve_start_point );
			resolve_start_point= child_namespace.get();

		}
		else
		{
			++out_skip_components; // Found class, class template, variable, functions set, etc.
			return std::make_pair( name, resolve_start_point );
		}
	}

	return std::make_pair( nullptr, nullptr );
}

} // namespace CodeBuilderPrivate

} // namespace U
