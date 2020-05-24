#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "mangling.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

namespace
{

void AddAncestorsAccessRights_r( Class& the_class, const ClassProxyPtr& ancestor_class )
{
	the_class.members.AddAccessRightsFor( ancestor_class, ClassMemberVisibility::Protected );
	for( const Class::Parent& parent : ancestor_class->class_->parents )
		AddAncestorsAccessRights_r( the_class, parent.class_ );
}

//
// Globals loops detecting
//

class GlobalsLoopsDetectorGuard final
{
public:
	GlobalsLoopsDetectorGuard( std::function<void()> function ) : function_(std::move(function)) {}
	~GlobalsLoopsDetectorGuard(){ function_(); }
private:
	std::function<void()> function_;
};
#define DETECT_GLOBALS_LOOP( in_thing_ptr, in_name, in_file_pos, in_completeness ) \
	{ \
		const FilePos file_pos__= in_file_pos; \
		const GlobalThing global_thing( static_cast<const void*>(in_thing_ptr), in_name, file_pos__, in_completeness ); \
		const size_t loop_pos= GlobalThingDetectloop( global_thing ); \
		if( loop_pos != ~0u ) \
		{ \
			GlobalThingReportAboutLoop( loop_pos, in_name, file_pos__ ); \
			return; \
		} \
		global_things_stack_.push_back( global_thing ); \
	} \
	GlobalsLoopsDetectorGuard glbals_loop_detector_guard( [this]{ global_things_stack_.pop_back(); } );

void SortClassFields( Class& class_, ClassFieldsVector<llvm::Type*>& fields_llvm_types, const llvm::DataLayout& data_layout )
{
	// Fields in original order
	using FieldsMap= std::map< unsigned int, ClassField* >;
	FieldsMap fields;

	bool fields_is_ok= true;
	class_.members.ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( ClassField* const field= value.GetClassField() )
			{
				fields[field->index]= field;
				if( !field->is_reference && !field->type.GetLLVMType()->isSized() )
					fields_is_ok= false;
			}
		} );

	if( fields.empty() || !fields_is_ok )
		return;

	unsigned int field_index= fields.begin()->first;

	fields_llvm_types.resize( field_index ); // Remove all fields ( parent classes and virtual table pointers are not in fields list ).

	// "getABITypeAlignment" "getTypeAllocSize" functions used, as in llvm/lib/IR/DataLayout.cpp:40.

	// Calculate start offset, include parents fields, virtual table pointer.
	unsigned int current_offst= 0u;
	for( llvm::Type* type : fields_llvm_types )
	{
		const unsigned int alignment= data_layout.getABITypeAlignment( type );
		const unsigned int padding= ( alignment - current_offst % alignment ) % alignment;
		current_offst+= padding + static_cast<unsigned int>( data_layout.getTypeAllocSize( type ) );
	}

	// Sort fields, minimize paddings and minimize fields reordering.
	while( !fields.empty() )
	{
		FieldsMap::iterator best_field_it= fields.begin();
		unsigned int best_field_padding= ~0u;
		for( auto it= fields.begin(); it != fields.end(); ++it )
		{
			const unsigned int alignment= data_layout.getABITypeAlignment( best_field_it->second->is_reference ? it->second->type.GetLLVMType()->getPointerTo() : it->second->type.GetLLVMType() );
			U_ASSERT( alignment != 0u );

			const unsigned int padding= ( alignment - current_offst % alignment ) % alignment;
			if( padding < best_field_padding )
			{
				best_field_padding= padding;
				best_field_it= it;
				if( padding == 0 )
					break;
			}
		}

		llvm::Type* best_field_llvm_type= best_field_it->second->type.GetLLVMType();
		if( best_field_it->second->is_reference )
			best_field_llvm_type= best_field_llvm_type->getPointerTo();

		best_field_it->second->index= field_index;
		++field_index;
		fields_llvm_types.push_back( best_field_llvm_type );
		current_offst+= best_field_padding + static_cast<unsigned int>(data_layout.getTypeAllocSize( best_field_llvm_type ));
		fields.erase( best_field_it );
	}
}

} // namespace

//
// CodeBuilder
//

bool CodeBuilder::IsTypeComplete( const Type& type ) const
{
	if( const auto fundamental_type= type.GetFundamentalType() )
		return fundamental_type->fundamental_type != U_FundamentalType::Void;
	else if( type.GetFunctionType() != nullptr || type.GetFunctionPointerType() != nullptr )
		return true;
	else if( const auto enum_type= type.GetEnumType() )
		return enum_type->syntax_element == nullptr;
	else if( const auto array_type= type.GetArrayType() )
		return IsTypeComplete( array_type->type );
	else if( const auto tuple_type= type.GetTupleType() )
	{
		bool all_complete= true;
		for( const Type& element_type : tuple_type->elements )
			all_complete= all_complete && IsTypeComplete( element_type );
		return all_complete;
	}
	else if( const auto class_type= type.GetClassTypeProxy() )
		return class_type->class_->completeness == TypeCompleteness::Complete;
	else
	{
		U_ASSERT(false);
		return false;
	}
}

bool CodeBuilder::EnsureTypeCompleteness( const Type& type, const TypeCompleteness completeness )
{
	if( completeness == TypeCompleteness::Incomplete )
		return true;

	if( const auto fundamental_type= type.GetFundamentalType() )
	{
		if( completeness > TypeCompleteness::Incomplete )
			return fundamental_type->fundamental_type != U_FundamentalType::Void;
		return true;
	}
	else if( type.GetFunctionType() != nullptr || type.GetFunctionPointerType() != nullptr )
		return true;
	else if( const auto enum_type= type.GetEnumType() )
	{
		GlobalThingBuildEnum( enum_type, completeness );
		return true;
	}
	else if( const auto array_type= type.GetArrayType() )
		return EnsureTypeCompleteness( array_type->type, completeness );
	else if( const auto tuple_type= type.GetTupleType() )
	{
		bool ok= true;
		for( const Type& element_type : tuple_type->elements )
			ok= EnsureTypeCompleteness( element_type, completeness ) || ok;
		return ok;
	}
	else if( const auto class_type= type.GetClassTypeProxy() )
	{
		GlobalThingBuildClass( class_type, completeness );
		return class_type->class_->completeness >= completeness; // Return true if we achived required completeness.
	}
	else U_ASSERT(false);

	return false;
}

bool CodeBuilder::ReferenceIsConvertible( const Type& from, const Type& to, CodeBuilderErrorsContainer& errors_container, const FilePos& file_pos )
{
	if( from == to )
		return true;

	if( from != void_type_ && to != void_type_ )
	{
		if( !EnsureTypeCompleteness( from, TypeCompleteness::Complete ) )
			REPORT_ERROR( UsingIncompleteType, errors_container, file_pos, from );
		if( !EnsureTypeCompleteness(   to, TypeCompleteness::Complete ) )
			REPORT_ERROR( UsingIncompleteType, errors_container, file_pos,   to );
	}

	return from.ReferenceIsConvertibleTo(to);
}

void CodeBuilder::GlobalThingBuildNamespace( NamesScope& names_scope )
{
	names_scope.ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( const NamesScopePtr inner_namespace= value.GetNamespace() )
				GlobalThingBuildNamespace( *inner_namespace );
			else if( OverloadedFunctionsSet* const functions_set= value.GetFunctionsSet() )
				GlobalThingBuildFunctionsSet( names_scope, *functions_set, true );
			else if( const Type* const type= value.GetTypeName() )
			{
				if( type->GetFundamentalType() != nullptr ||
					type->GetFunctionPointerType() != nullptr ||
					type->GetArrayType() != nullptr ||
					type->GetTupleType() != nullptr )
				{}
				else if( const ClassProxyPtr class_type= type->GetClassTypeProxy() )
				{
					// Build classes only from parent namespace.
					// Otherwise we can get loop, using typedef.
					if( class_type->class_->members.GetParent() == &names_scope )
					{
						GlobalThingBuildClass( class_type, TypeCompleteness::Complete );
						GlobalThingBuildNamespace( class_type->class_->members );
					}
				}
				else if( const EnumPtr enum_type= type->GetEnumType() )
					GlobalThingBuildEnum( enum_type, TypeCompleteness::Complete );
				else U_ASSERT(false);
			}
			else if( const auto type_templates_set= value.GetTypeTemplatesSet() )
				GlobalThingBuildTypeTemplatesSet( names_scope, *type_templates_set );
			else if( value.GetClassField() != nullptr ) {} // Can be in classes.
			else if( value.GetFunctionVariable() != nullptr ) {} // It is function, generating from template.
			else if( value.GetVariable() != nullptr ){}
			else if( value.GetErrorValue() != nullptr ){}
			else if( const auto static_assert_= value.GetStaticAssert() )
				BuildStaticAssert( *static_assert_, names_scope, *global_function_context_ );
			else if( value.GetTypedef() != nullptr )
				GlobalThingBuildTypedef( names_scope, value );
			else if( value.GetIncompleteGlobalVariable() != nullptr )
				GlobalThingBuildVariable( names_scope, value );
			else U_ASSERT(false);
		});
}

void CodeBuilder::GlobalThingBuildFunctionsSet( NamesScope& names_scope, OverloadedFunctionsSet& functions_set, const bool build_body )
{
	if( !functions_set.syntax_elements.empty() || !functions_set.out_of_line_syntax_elements.empty() || !functions_set.template_syntax_elements.empty() )
	{
		FilePos functions_set_file_pos;
		std::string functions_set_name;
		if( !functions_set.syntax_elements.empty() )
		{
			functions_set_file_pos= functions_set.syntax_elements.front()->file_pos_;
			functions_set_name= functions_set.syntax_elements.front()->name_.back();
		}
		else if( !functions_set.template_syntax_elements.empty() )
		{
			functions_set_file_pos= functions_set.template_syntax_elements.front()->file_pos_;
			functions_set_name= functions_set.template_syntax_elements.front()->function_->name_.back();
		}
		DETECT_GLOBALS_LOOP( &functions_set, functions_set_name, functions_set_file_pos, TypeCompleteness::Complete );

		for( const Synt::Function* const function : functions_set.syntax_elements )
		{
			const size_t function_index= PrepareFunction( names_scope, functions_set.base_class, functions_set, *function, false );

			if( function_index == ~0u )
				continue;
			FunctionVariable& function_variable= functions_set.functions[function_index];

			// Immediately build functions with auto return type.
			if( function_variable.return_type_is_auto && !function_variable.have_body )
			{
				// First, compile function only for return type deducing.
				const Type return_type=
					BuildFuncCode(
						function_variable,
						functions_set.base_class,
						names_scope,
						functions_set_name,
						function_variable.syntax_element->type_.arguments_,
						function_variable.syntax_element->block_.get(),
						function_variable.syntax_element->constructor_initialization_list_.get() );

				function_variable.have_body= false;
				function_variable.type.GetFunctionType()->return_type= return_type;
				function_variable.return_type_is_auto= false;

				function_variable.type.GetFunctionType()->llvm_function_type= nullptr;
				function_variable.llvm_function->eraseFromParent();
				function_variable.llvm_function= nullptr;

				// Then, compile function again, when type already known.
				BuildFuncCode(
					function_variable,
					functions_set.base_class,
					names_scope,
					functions_set_name,
					function_variable.syntax_element->type_.arguments_,
					function_variable.syntax_element->block_.get(),
					function_variable.syntax_element->constructor_initialization_list_.get() );
			}
		}

		for( const Synt::Function* const function : functions_set.out_of_line_syntax_elements )
			PrepareFunction( names_scope, functions_set.base_class, functions_set, *function, true );

		for( const Synt::FunctionTemplate* const function_template : functions_set.template_syntax_elements )
			PrepareFunctionTemplate( *function_template, functions_set, names_scope, functions_set.base_class );

		functions_set.syntax_elements.clear();
		functions_set.out_of_line_syntax_elements.clear();
		functions_set.template_syntax_elements.clear();
	}

	if( build_body )
	{
		for( FunctionVariable& function_variable : functions_set.functions )
		{
			if( function_variable.syntax_element != nullptr && function_variable.syntax_element->block_ != nullptr &&
				!function_variable.have_body && !function_variable.return_type_is_auto )
			{
				BuildFuncCode(
					function_variable,
					functions_set.base_class,
					names_scope,
					function_variable.syntax_element->name_.back(),
					function_variable.syntax_element->type_.arguments_,
					function_variable.syntax_element->block_.get(),
					function_variable.syntax_element->constructor_initialization_list_.get() );
			}
		}
	}
	else if( functions_set.base_class == nullptr )
	{
		// Immediately build constexpr functions.
		for( FunctionVariable& function_variable : functions_set.functions )
		{
			if( function_variable.syntax_element != nullptr && function_variable.syntax_element->block_ != nullptr &&
				!function_variable.have_body && function_variable.constexpr_kind != FunctionVariable::ConstexprKind::NonConstexpr )
			{
				BuildFuncCode(
					function_variable,
					functions_set.base_class,
					names_scope,
					function_variable.syntax_element->name_.back(),
					function_variable.syntax_element->type_.arguments_,
					function_variable.syntax_element->block_.get(),
					function_variable.syntax_element->constructor_initialization_list_.get() );
			}
		}
	}
}

void CodeBuilder::GlobalThingBuildClass( const ClassProxyPtr class_type, const TypeCompleteness completeness )
{
	Class& the_class= *class_type->class_;

	if( completeness <= the_class.completeness ||
		completeness == TypeCompleteness::Incomplete ||
		( the_class.syntax_element != nullptr && the_class.syntax_element->is_forward_declaration_ ) )
		return;

	if( the_class.typeinfo_type != std::nullopt )
	{
		if( completeness <= TypeCompleteness::ReferenceTagsComplete )
			return;

		for( auto& typeinfo_cache_entry : typeinfo_cache_ )
		{
			if( typeinfo_cache_entry.second.type == class_type )
			{
				BuildFullTypeinfo( typeinfo_cache_entry.first, typeinfo_cache_entry.second, *class_type->class_->members.GetRoot() );
				return;
			}
		}
		U_ASSERT(false);
	}

	const Synt::Class& class_declaration= *the_class.syntax_element;
	const std::string& class_name= class_declaration.name_;

	if( completeness >= TypeCompleteness::ReferenceTagsComplete && the_class.completeness < TypeCompleteness::ReferenceTagsComplete )
	{
		DETECT_GLOBALS_LOOP( &the_class, the_class.members.GetThisNamespaceName(), the_class.body_file_pos, TypeCompleteness::ReferenceTagsComplete );

		the_class.have_shared_state= class_declaration.have_shared_state_;

		NamesScope& class_parent_namespace= *the_class.members.GetParent();
		for( const Synt::ComplexName& parent : class_declaration.parents_ )
		{
			const Value parent_value= ResolveValue( class_declaration.file_pos_, class_parent_namespace, *global_function_context_, parent );

			const Type* const type_name= parent_value.GetTypeName();
			if( type_name == nullptr )
			{
				REPORT_ERROR( NameIsNotTypeName, class_parent_namespace.GetErrors(), class_declaration.file_pos_, parent );
				continue;
			}

			const ClassProxyPtr parent_class_proxy= type_name->GetClassTypeProxy();
			if( parent_class_proxy == nullptr )
			{
				REPORT_ERROR( CanNotDeriveFromThisType, class_parent_namespace.GetErrors(), class_declaration.file_pos_, type_name );
				continue;
			}
			if( !EnsureTypeCompleteness( *type_name, TypeCompleteness::Complete ) )
			{
				REPORT_ERROR( UsingIncompleteType, class_parent_namespace.GetErrors(), class_declaration.file_pos_, type_name );
				continue;
			}

			bool duplicated= false;
			for( const Class::Parent& parent : the_class.parents )
				duplicated= duplicated || parent.class_ == parent_class_proxy;
			if( duplicated )
			{
				REPORT_ERROR( DuplicatedParentClass, class_parent_namespace.GetErrors(), class_declaration.file_pos_, type_name );
				continue;
			}

			const auto parent_kind= parent_class_proxy->class_->kind;
			if( !( parent_kind == Class::Kind::Abstract || parent_kind == Class::Kind::Interface || parent_kind == Class::Kind::PolymorphNonFinal ) )
			{
				REPORT_ERROR( CanNotDeriveFromThisType, class_parent_namespace.GetErrors(), class_declaration.file_pos_, type_name );
				continue;
			}

			if( parent_kind != Class::Kind::Interface ) // not interface=base
			{
				if( the_class.base_class != nullptr )
				{
					REPORT_ERROR( DuplicatedBaseClass, class_parent_namespace.GetErrors(), class_declaration.file_pos_, type_name );
					continue;
				}
				the_class.base_class= parent_class_proxy;
			}

			the_class.parents.emplace_back();
			the_class.parents.back().class_= parent_class_proxy;
			AddAncestorsAccessRights_r( the_class, parent_class_proxy );
		} // for parents

		ProcessClassParentsVirtualTables( the_class );

		// Pre-mark class as polymorph. Later we know class kind exactly, now, we only needs to know, that is polymorph - for virtual functions preparation.
		if( class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Polymorph ||
			class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Interface ||
			class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Abstract ||
			!class_declaration.parents_.empty() )
			the_class.kind= Class::Kind::PolymorphNonFinal;
		else if( class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Struct )
			the_class.kind= Class::Kind::Struct;
		else
			the_class.kind= Class::Kind::NonPolymorph;

		the_class.can_be_constexpr= the_class.kind == Class::Kind::Struct;

		the_class.members.ForEachValueInThisScope(
			[&]( Value& value )
			{
				ClassField* const class_field= value.GetClassField();
				if( class_field == nullptr )
					return;

				const Synt::ClassField& in_field= *class_field->syntax_element;

				class_field->class_= class_type;
				class_field->is_reference= in_field.reference_modifier == Synt::ReferenceModifier::Reference;
				class_field->type= PrepareType( class_field->syntax_element->type, the_class.members, *global_function_context_ );

				if( !class_field->is_reference || in_field.mutability_modifier == Synt::MutabilityModifier::Constexpr )
				{
					// Full type completeness required for value-fields and constexpr reference-fields.
					if( !EnsureTypeCompleteness( class_field->type, TypeCompleteness::Complete ) )
					{
						REPORT_ERROR( UsingIncompleteType, class_parent_namespace.GetErrors(), in_field.file_pos_, class_field->type );
						return;
					}
				}

				if( class_field->is_reference )
				{
					if( class_field->type != void_type_ && !EnsureTypeCompleteness( class_field->type, TypeCompleteness::ReferenceTagsComplete ) )
					{
						REPORT_ERROR( UsingIncompleteType, class_parent_namespace.GetErrors(), in_field.file_pos_, class_field->type );
						return;
					}
					if( class_field->type.ReferencesTagsCount() > 0u )
						REPORT_ERROR( ReferenceFiledOfTypeWithReferencesInside, class_parent_namespace.GetErrors(), in_field.file_pos_, in_field.name );
				}
				else if( class_field->type.IsAbstract() )
					REPORT_ERROR( ConstructingAbstractClassOrInterface, class_parent_namespace.GetErrors(), in_field.file_pos_, class_field->type );

				if( class_field->is_reference ) // Reference-fields are immutable by default
					class_field->is_mutable= in_field.mutability_modifier == Synt::MutabilityModifier::Mutable;
				else // But value-fields are mutable by default
					class_field->is_mutable= in_field.mutability_modifier != Synt::MutabilityModifier::Immutable;

				// Disable constexpr, if field can not be constexpr, or if field is mutable reference.
				if( !class_field->type.CanBeConstexpr() || ( class_field->is_reference && class_field->is_mutable ) )
					the_class.can_be_constexpr= false;

				++the_class.field_count;
			} );

		// TODO - ensure fields/parents reference tag completeness before setting up own references tags/references type.

		// Count reference tags.
		// SPRACHE_TODO - allow user explicitly set tag count.
		the_class.members.ForEachValueInThisScope(
			[&]( const Value& value )
			{
				const ClassField* const field= value.GetClassField();
				if( field == nullptr )
					return;

				if( field->is_reference || field->type.ReferencesTagsCount() != 0u )
					the_class.references_tags_count= 1u;

				if( field->is_reference )
					the_class.inner_reference_type= std::max( the_class.inner_reference_type, field->is_mutable ? InnerReferenceType::Mut : InnerReferenceType::Imut );
				else
					the_class.inner_reference_type= std::max( the_class.inner_reference_type, field->type.GetInnerReferenceType() );

			});
		for( const Class::Parent& parent : the_class.parents )
		{
			if( parent.class_->class_->references_tags_count != 0u )
				the_class.references_tags_count= 1u;
			the_class.inner_reference_type= std::max( the_class.inner_reference_type, parent.class_->class_->inner_reference_type );
		}

		the_class.completeness= TypeCompleteness::ReferenceTagsComplete;
	} // if reference tags completeness required

	if( completeness == TypeCompleteness::Complete )
	{
		DETECT_GLOBALS_LOOP( &the_class, the_class.members.GetThisNamespaceName(), the_class.body_file_pos, TypeCompleteness::Complete );

		// Fill llvm struct type fields
		ClassFieldsVector<llvm::Type*> fields_llvm_types;

		// Base must be always first field.
		if( the_class.base_class != nullptr )
			fields_llvm_types.push_back( the_class.base_class->class_->llvm_type );
		// Add non-base (interface) fields.
		for( Class::Parent& parent : the_class.parents )
		{
			if( parent.class_ == the_class.base_class )
			{
				parent.field_number= 0u;
				continue;
			}

			parent.field_number= static_cast<unsigned int>(fields_llvm_types.size());
			fields_llvm_types.emplace_back( parent.class_->class_->llvm_type );
		}

		// Allocate virtual table pointer, if class have no parents.
		// If class have at least one parent, reuse it's virtual table pointer.
		bool allocate_virtual_table_pointer= false;
		if( the_class.parents.empty() && (
			class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Abstract ||
			class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Polymorph ||
			class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Interface ) )
		{
			U_ASSERT( fields_llvm_types.empty() );
			fields_llvm_types.emplace_back( fundamental_llvm_types_.void_->getPointerTo() ); // set exact type later.
			allocate_virtual_table_pointer= true;
		}

		{ // Create fields.
			std::map< unsigned int, ClassField* > class_fields_in_original_order;

			the_class.members.ForEachValueInThisScope(
				[&]( Value& value )
				{
					if( ClassField* const class_field= value.GetClassField() )
						class_fields_in_original_order[class_field->original_index]= class_field;
				});

			for( const auto& field_entry : class_fields_in_original_order )
			{
				ClassField* const class_field= field_entry.second;
				class_field->index= static_cast<unsigned int>(fields_llvm_types.size());
				if( class_field->is_reference )
					fields_llvm_types.emplace_back( class_field->type.GetLLVMType()->getPointerTo() );
				else
				{
					if( !class_field->type.GetLLVMType()->isSized() )
						fields_llvm_types.emplace_back( fundamental_llvm_types_.i8 );// May be in case of error (such dependency loop )
					else
						fields_llvm_types.emplace_back( class_field->type.GetLLVMType() );
				}
			}

			if( !class_declaration.keep_fields_order_ )
				SortClassFields( the_class, fields_llvm_types, data_layout_ );
		}

		// Fill container with fileds names.
		the_class.fields_order.resize( fields_llvm_types.size() );
		the_class.members.ForEachInThisScope(
			[&]( const std::string& name, const Value& value )
			{
				if( const auto field= value.GetClassField() )
					the_class.fields_order[field->index]= name;
			} );

		// Complete another body elements.
		// For class completeness we needs only fields, functions. Constants, types and type templates dones not needed.
		std::vector< FunctionVariable* > class_functions;
		the_class.members.ForEachValueInThisScope(
			[&]( Value& value )
			{
				if( const auto functions_set= value.GetFunctionsSet() )
				{
					GlobalThingBuildFunctionsSet( the_class.members, *functions_set, false );
					for( FunctionVariable& function : functions_set->functions )
						class_functions.emplace_back( &function );
				}
				else if( value.GetClassField() != nullptr ) {} // Fields are already complete.
				else if( value.GetTypeName() != nullptr ) {}
				else if( value.GetVariable() != nullptr ){}
				else if( value.GetErrorValue() != nullptr ){}
				else if( value.GetStaticAssert() != nullptr ){}
				else if( value.GetTypedef() != nullptr ) {}
				else if( value.GetTypeTemplatesSet() != nullptr ) {}
				else if( value.GetIncompleteGlobalVariable() != nullptr ) {}
				else if( value.GetNamespace() != nullptr ) {} // Can be in case of type template parameters namespace.
				else U_ASSERT(false);
			});

		// Wee needs strong order of functions in virtual table. So, sort them, using mangled name.
		std::sort(
			class_functions.begin(), class_functions.end(),
			[]( const FunctionVariable* const l, const FunctionVariable* const r )
			{
				return l->llvm_function->getName() < r->llvm_function->getName();
			} );
		for( FunctionVariable* func : class_functions )
			ProcessClassVirtualFunction( the_class, *func );

		// Search for explicit noncopy constructors.
		if( const Value* const constructors_value=
			the_class.members.GetThisScopeValue( Keyword( Keywords::constructor_ ) ) )
		{
			const OverloadedFunctionsSet* const constructors= constructors_value->GetFunctionsSet();
			U_ASSERT( constructors != nullptr );
			for( const FunctionVariable& constructor : constructors->functions )
			{
				const Function& constructor_type= *constructor.type.GetFunctionType();

				U_ASSERT( constructor_type.args.size() >= 1u && constructor_type.args.front().type == class_type );
				if( !( constructor_type.args.size() == 2u && constructor_type.args.back().type == class_type && !constructor_type.args.back().is_mutable ) )
				{
					the_class.have_explicit_noncopy_constructors= true;
					break;
				}
			};
		}

		// Disable constexpr possibility for structs with explicit destructors, non-default copy-assignment operators and non-default copy constructors.
		if( const Value* const destructor_value=
			the_class.members.GetThisScopeValue( Keyword( Keywords::destructor_ ) ) )
		{
			const OverloadedFunctionsSet* const destructors= destructor_value->GetFunctionsSet();
			// Destructors may be invalid in case of error.
			if( !destructors->functions.empty() && !destructors->functions[0].is_generated )
				the_class.can_be_constexpr= false;
		}
		if( const Value* const constructor_value=
			the_class.members.GetThisScopeValue( Keyword( Keywords::constructor_ ) ) )
		{
			const OverloadedFunctionsSet* const constructors= constructor_value->GetFunctionsSet();
			U_ASSERT( constructors != nullptr );
			for( const FunctionVariable& constructor : constructors->functions )
			{
				if( IsCopyConstructor( *constructor.type.GetFunctionType(), class_type ) && !constructor.is_generated )
					the_class.can_be_constexpr= false;
			}
		}
		if( const Value* const assignment_operator_value=
			the_class.members.GetThisScopeValue( OverloadedOperatorToString( OverloadedOperator::Assign ) ) )
		{
			const OverloadedFunctionsSet* const operators= assignment_operator_value->GetFunctionsSet();
			U_ASSERT( operators != nullptr );
			for( const FunctionVariable& op : operators->functions )
			{
				if( IsCopyAssignmentOperator( *op.type.GetFunctionType(), class_type ) && !op.is_generated )
					the_class.can_be_constexpr= false;
			}
		}

		bool class_contains_pure_virtual_functions= false;
		for( Class::VirtualTableEntry& virtual_table_entry : the_class.virtual_table )
		{
			if( virtual_table_entry.is_pure )
			{
				class_contains_pure_virtual_functions= true;
				break;
			}
		}

		// Check given kind attribute and actual class properties.
		switch( class_declaration.kind_attribute_ )
		{
		case Synt::ClassKindAttribute::Struct:
			U_ASSERT( class_declaration.parents_.empty() );
			the_class.kind= Class::Kind::Struct;
			break;

		case Synt::ClassKindAttribute::Class: // Class without parents and without kind attribute is non-polymorph.
			if( the_class.parents.empty() )
				the_class.kind= Class::Kind::NonPolymorph;
			else
				the_class.kind= Class::Kind::PolymorphNonFinal;
			if( class_contains_pure_virtual_functions )
			{
				REPORT_ERROR( ClassContainsPureVirtualFunctions, the_class.members.GetErrors(), class_declaration.file_pos_, class_name );
				the_class.kind= Class::Kind::Abstract;
			}
			break;

		case Synt::ClassKindAttribute::Final:
			if( the_class.parents.empty() )
				the_class.kind= Class::Kind::NonPolymorph;
			else
			{
				for( Class::VirtualTableEntry& virtual_table_entry : the_class.virtual_table )
					virtual_table_entry.is_final= true; // All virtual functions of final class is final.
				the_class.kind= Class::Kind::PolymorphFinal;
			}
			if( class_contains_pure_virtual_functions )
			{
				REPORT_ERROR( ClassContainsPureVirtualFunctions, the_class.members.GetErrors(), class_declaration.file_pos_, class_name );
				the_class.kind= Class::Kind::Abstract;
			}
			break;

		case Synt::ClassKindAttribute::Polymorph:
			the_class.kind= Class::Kind::PolymorphNonFinal;
			if( class_contains_pure_virtual_functions )
			{
				REPORT_ERROR( ClassContainsPureVirtualFunctions, the_class.members.GetErrors(), class_declaration.file_pos_, class_name );
				the_class.kind= Class::Kind::Abstract;
			}
			break;

		case Synt::ClassKindAttribute::Interface:
			if( the_class.field_count != 0u )
				REPORT_ERROR( FieldsForInterfacesNotAllowed, the_class.members.GetErrors(), class_declaration.file_pos_ );
			if( the_class.base_class != nullptr )
				REPORT_ERROR( BaseClassForInterface, the_class.members.GetErrors(), class_declaration.file_pos_ );
			if( the_class.members.GetThisScopeValue( Keyword( Keywords::constructor_ ) ) != nullptr )
				REPORT_ERROR( ConstructorForInterface, the_class.members.GetErrors(), class_declaration.file_pos_ );
			for( const Class::VirtualTableEntry& virtual_table_entry : the_class.virtual_table )
			{
				if( !virtual_table_entry.is_pure && virtual_table_entry.name != Keywords::destructor_ )
				{
					REPORT_ERROR( NonPureVirtualFunctionInInterface, the_class.members.GetErrors(), class_declaration.file_pos_, class_name );
					break;
				}
			}
			the_class.kind= Class::Kind::Interface;
			break;

		case Synt::ClassKindAttribute::Abstract:
			the_class.kind= Class::Kind::Abstract;
			break;
		};

		if( the_class.kind == Class::Kind::Interface ||
			the_class.kind == Class::Kind::Abstract ||
			the_class.kind == Class::Kind::PolymorphNonFinal ||
			the_class.kind == Class::Kind::PolymorphFinal )
			TryGenerateDestructorPrototypeForPolymorphClass( the_class, class_type );

		// Merge namespaces of parents into result class.
		for( const Class::Parent& parent : the_class.parents )
		{
			const Class* const parent_class= parent.class_->class_;
			parent_class->members.ForEachInThisScope(
				[&]( const std::string& name, const Value& value )
				{
					if( parent_class->GetMemberVisibility( name ) == ClassMemberVisibility::Private )
						return; // Do not inherit private members.

					Value* const result_class_value= the_class.members.GetThisScopeValue(name);

					if( const OverloadedFunctionsSet* const functions= value.GetFunctionsSet() )
					{
						// SPARCHE_TODO - maybe also skip additive-assignment operators?
						if( name == Keyword( Keywords::constructor_ ) ||
							name == Keyword( Keywords::destructor_ ) ||
							name == OverloadedOperatorToString( OverloadedOperator::Assign ) )
							return; // Did not inherit constructors, destructors, assignment operators.

						if( result_class_value != nullptr )
						{
							if( OverloadedFunctionsSet* const result_class_functions= result_class_value->GetFunctionsSet() )
							{
								if( the_class.GetMemberVisibility( name ) != parent_class->GetMemberVisibility( name ) )
								{
									const auto& file_pos= result_class_functions->functions.empty() ? result_class_functions->template_functions.front()->file_pos : result_class_functions->functions.front().prototype_file_pos;
									REPORT_ERROR( FunctionsVisibilityMismatch, the_class.members.GetErrors(), file_pos, name );
								}

								// Merge function sets, if result class have functions set with given name.
								for( const FunctionVariable& parent_function : functions->functions )
								{
									bool overrides= false;
									for( FunctionVariable& result_class_function : result_class_functions->functions )
									{
										if( parent_function.type == result_class_function.type )
										{
											overrides= true; // Ok, result class function overrides parent clas function.
											break;
										}
									}
									if( !overrides )
										ApplyOverloadedFunction( *result_class_functions, parent_function, the_class.members.GetErrors(), class_declaration.file_pos_ );
								} // for parent functions

								// TODO - merge function templates smarter.
								for( const FunctionTemplatePtr& function_template : functions->template_functions )
									result_class_functions->template_functions.push_back(function_template);
							}
						}
						else
						{
							// Result class have no functions with this name. Inherit all functions from parent calass.
							the_class.members.AddName( name, value );
						}
					}
					else
					{
						// Just override other kinds of symbols.
						if( result_class_value == nullptr )
							the_class.members.AddName( name, value );
					}
				});
		}

		PrepareClassVirtualTableType( class_type );
		if( allocate_virtual_table_pointer )
			fields_llvm_types[0]= the_class.virtual_table_llvm_type->getPointerTo();

		// Check opaque before set body for cases of errors (class body duplication).
		if( the_class.llvm_type->isOpaque() )
			the_class.llvm_type->setBody( fields_llvm_types );

		BuildClassVirtualTables( the_class, class_type );

		the_class.completeness= TypeCompleteness::Complete;

		TryGenerateDefaultConstructor( the_class, class_type );
		TryGenerateDestructor( the_class, class_type );
		TryGenerateCopyConstructor( the_class, class_type );
		TryGenerateCopyAssignmentOperator( the_class, class_type );

		CheckClassFieldsInitializers( class_type );

		// Immediately build constexpr functions.
		the_class.members.ForEachInThisScope(
			[&]( const std::string& name, Value& value )
			{
				OverloadedFunctionsSet* const functions_set= value.GetFunctionsSet();
				if( functions_set == nullptr )
					return;

				for( FunctionVariable& function : functions_set->functions )
				{
					if( function.constexpr_kind != FunctionVariable::ConstexprKind::NonConstexpr &&
						function.syntax_element != nullptr )
						BuildFuncCode(
							function,
							class_type,
							the_class.members,
							name,
							function.syntax_element->type_.arguments_,
							function.syntax_element->block_.get(),
							function.syntax_element->constructor_initialization_list_.get() );
				}
			}); // for functions
	} // if full comleteness required
}

void CodeBuilder::GlobalThingBuildEnum( const EnumPtr enum_, TypeCompleteness completeness )
{
	if( completeness < TypeCompleteness::Complete )
		return;
	if( enum_->syntax_element == nullptr )
		return;

	DETECT_GLOBALS_LOOP( enum_, enum_->members.GetThisNamespaceName(), enum_->syntax_element->file_pos_, TypeCompleteness::Complete );

	// Default underlaying type is 32bit. TODO - maybe do it platform-dependent?
	enum_->underlaying_type= FundamentalType( U_FundamentalType::u32, fundamental_llvm_types_.u32 );

	const Synt::Enum& enum_decl= *enum_->syntax_element;
	NamesScope& names_scope= *enum_->members.GetParent();

	if( !( std::get_if<Synt::EmptyVariant>( &enum_decl.underlaying_type_name.start_value ) != nullptr && enum_decl.underlaying_type_name.tail == nullptr ) )
	{
		const Value type_value= ResolveValue( enum_decl.file_pos_, names_scope, *global_function_context_, enum_decl.underlaying_type_name );
		const Type* const type= type_value.GetTypeName();
		if( type == nullptr )
			REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), enum_decl.file_pos_, enum_decl.underlaying_type_name );
		else
		{
			const FundamentalType* const fundamental_type= type->GetFundamentalType();
			if( fundamental_type == nullptr || !IsInteger( fundamental_type->fundamental_type ) )
			{
				// SPRACHE_TODO - maybe allow inheritance of enums?
				REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), enum_decl.file_pos_, "any integer type", type );
			}
			else
				enum_->underlaying_type= *fundamental_type;
		}
	}

	for( const Synt::Enum::Member& in_member : enum_decl.members )
	{
		if( IsKeyword( in_member.name ) )
			REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), in_member.file_pos );
		if( NameShadowsTemplateArgument( in_member.name, names_scope ) )
			REPORT_ERROR( DeclarationShadowsTemplateArgument, names_scope.GetErrors(), in_member.file_pos, in_member.name );

		Variable var;

		var.type= enum_;
		var.location= Variable::Location::Pointer;
		var.value_type= ValueType::ConstReference;
		var.constexpr_value=
			llvm::Constant::getIntegerValue(
				enum_->underlaying_type.llvm_type,
				llvm::APInt( enum_->underlaying_type.llvm_type->getIntegerBitWidth(), enum_->element_count ) );
		var.llvm_value=
			CreateGlobalConstantVariable(
				var.type,
				MangleGlobalVariable( enum_->members, in_member.name ),
				var.constexpr_value );

		if( enum_->members.AddName( in_member.name, Value( var, in_member.file_pos ) ) == nullptr )
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), in_member.file_pos, in_member.name );

		++enum_->element_count;
	}

	{
		const uint64_t max_value_plus_one=
			uint64_t(1) << ( uint64_t(enum_->underlaying_type.llvm_type->getIntegerBitWidth()) - ( IsSignedInteger( enum_->underlaying_type.fundamental_type ) ? 1u : 0u ) );
		const uint64_t max_value= max_value_plus_one - 1u;

		if( enum_->element_count > max_value )
			REPORT_ERROR( UnderlayingTypeForEnumIsTooSmall, names_scope.GetErrors(), enum_decl.file_pos_, enum_->element_count - 1u, max_value );
	}

	enum_->syntax_element= nullptr;
}

void CodeBuilder::GlobalThingBuildTypeTemplatesSet( NamesScope& names_scope, TypeTemplatesSet& type_templates_set )
{
	if( !type_templates_set.syntax_elements.empty() )
	{
		DETECT_GLOBALS_LOOP( &type_templates_set, type_templates_set.syntax_elements.front()->name_, type_templates_set.syntax_elements.front()->file_pos_, TypeCompleteness::Complete );

		for( const auto syntax_element : type_templates_set.syntax_elements )
			PrepareTypeTemplate( *syntax_element, type_templates_set, names_scope );

		type_templates_set.syntax_elements.clear();
	}
}

void CodeBuilder::GlobalThingBuildTypedef( NamesScope& names_scope, Value& typedef_value )
{
	U_ASSERT( typedef_value.GetTypedef() != nullptr );
	const Synt::Typedef& syntax_element= *typedef_value.GetTypedef()->syntax_element;

	DETECT_GLOBALS_LOOP( &typedef_value, syntax_element.name, syntax_element.file_pos_, TypeCompleteness::Complete );

	// Replace value in names map, when typedef is comlete.
	typedef_value= Value( PrepareType( syntax_element.value, names_scope, *global_function_context_ ), syntax_element.file_pos_ );
}

void CodeBuilder::GlobalThingBuildVariable( NamesScope& names_scope, Value& global_variable_value )
{
	U_ASSERT( global_variable_value.GetIncompleteGlobalVariable() != nullptr );
	const IncompleteGlobalVariable incomplete_global_variable= *global_variable_value.GetIncompleteGlobalVariable();

	FilePos file_pos;
	if( incomplete_global_variable.variables_declaration != nullptr )
		file_pos= incomplete_global_variable.variables_declaration->variables[ incomplete_global_variable.element_index ].file_pos;
	else if( incomplete_global_variable.auto_variable_declaration != nullptr )
		file_pos= incomplete_global_variable.auto_variable_declaration->file_pos_;

	DETECT_GLOBALS_LOOP( &global_variable_value, incomplete_global_variable.name, file_pos, TypeCompleteness::Complete );
	#define FAIL_RETURN { global_variable_value= ErrorValue(); return; }

	FunctionContext& function_context= *global_function_context_;
	const StackVariablesStorage dummy_stack( function_context );

	if( const auto variables_declaration= incomplete_global_variable.variables_declaration )
	{
		const Synt::VariablesDeclaration::VariableEntry& variable_declaration= variables_declaration->variables[ incomplete_global_variable.element_index ];

		if( variable_declaration.mutability_modifier == Synt::MutabilityModifier::Mutable )
		{
			REPORT_ERROR( GlobalVariableMustBeConstexpr, names_scope.GetErrors(), variable_declaration.file_pos, variable_declaration.name );
			FAIL_RETURN;
		}

		const Type type= PrepareType( variables_declaration->type, names_scope, *global_function_context_ );
		if( !EnsureTypeCompleteness( type, TypeCompleteness::Complete ) ) // Global variables are all constexpr. Full completeness required for constexpr.
		{
			REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), variable_declaration.file_pos, type );
			FAIL_RETURN;
		}

		if( !type.CanBeConstexpr() )
		{
			REPORT_ERROR( InvalidTypeForConstantExpressionVariable, names_scope.GetErrors(), variable_declaration.file_pos );
			FAIL_RETURN;
		}

		// Destruction frame for temporary variables of initializer expression.
		const StackVariablesStorage temp_variables_storage( function_context );

		Variable variable;
		variable.type= type;
		variable.location= Variable::Location::Pointer;
		variable.value_type= ValueType::Reference;

		if( variable_declaration.reference_modifier == ReferenceModifier::None )
		{
			llvm::GlobalVariable* global_variable= nullptr;
			variable.llvm_value= global_variable= CreateGlobalConstantVariable( type, MangleGlobalVariable( names_scope, variable_declaration.name ) );

			if( variable_declaration.initializer != nullptr )
				variable.constexpr_value= ApplyInitializer( *variable_declaration.initializer, variable, names_scope, function_context );
			else
				ApplyEmptyInitializer( variable_declaration.name, variable_declaration.file_pos, variable, names_scope, function_context );

			// Make immutable, if needed, only after initialization, because in initialization we need call constructors, which is mutable methods.
			variable.value_type= ValueType::ConstReference;

			if( global_variable != nullptr && variable.constexpr_value != nullptr )
				global_variable->setInitializer( variable.constexpr_value );
		}
		else if( variable_declaration.reference_modifier == ReferenceModifier::Reference )
		{
			if( variable_declaration.initializer == nullptr )
			{
				REPORT_ERROR( ExpectedInitializer, names_scope.GetErrors(), variable_declaration.file_pos, variable_declaration.name );
				FAIL_RETURN;
			}

			variable.value_type= ValueType::ConstReference;

			const Synt::Expression* initializer_expression= nullptr;
			if( const auto expression_initializer= std::get_if<Synt::ExpressionInitializer>( variable_declaration.initializer.get() ) )
				initializer_expression= &expression_initializer->expression;
			else if( const auto constructor_initializer= std::get_if<Synt::ConstructorInitializer>( variable_declaration.initializer.get() ) )
			{
				if( constructor_initializer->call_operator.arguments_.size() != 1u )
				{
					REPORT_ERROR( ReferencesHaveConstructorsWithExactlyOneParameter, names_scope.GetErrors(), constructor_initializer->file_pos_ );
					FAIL_RETURN;
				}
				initializer_expression= &constructor_initializer->call_operator.arguments_.front();
			}
			else
			{
				REPORT_ERROR( UnsupportedInitializerForReference, names_scope.GetErrors(), variable_declaration.file_pos );
				FAIL_RETURN;
			}

			const Variable expression_result= BuildExpressionCodeEnsureVariable( *initializer_expression, names_scope, function_context );

			if( !ReferenceIsConvertible( expression_result.type, variable.type, names_scope.GetErrors(), variable_declaration.file_pos ) )
			{
				REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), variable_declaration.file_pos, variable.type, expression_result.type );
				FAIL_RETURN;
			}
			if( expression_result.value_type == ValueType::Value )
			{
				REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), variable_declaration.file_pos );
				FAIL_RETURN;
			}
			if( expression_result.value_type == ValueType::ConstReference && variable.value_type == ValueType::Reference )
			{
				REPORT_ERROR( BindingConstReferenceToNonconstReference, names_scope.GetErrors(), variable_declaration.file_pos );
				FAIL_RETURN;
			}

			// TODO - maybe make copy of varaible address in new llvm register?
			llvm::Value* result_ref= expression_result.llvm_value;
			if( variable.type != expression_result.type )
				result_ref= CreateReferenceCast( result_ref, expression_result.type, variable.type, function_context );
			variable.llvm_value= result_ref;
			variable.constexpr_value= expression_result.constexpr_value;
		}
		else U_ASSERT(false);

		if( variable.constexpr_value == nullptr )
		{
			REPORT_ERROR( VariableInitializerIsNotConstantExpression, names_scope.GetErrors(), variable_declaration.file_pos );
			FAIL_RETURN;
		}

		// Do not call destructors, because global variables can be only constexpr and any constexpr type have trivial destructor.

		global_variable_value= Value( variable, variable_declaration.file_pos );
	}
	else if( const auto auto_variable_declaration= incomplete_global_variable.auto_variable_declaration )
	{
		if( auto_variable_declaration->mutability_modifier == MutabilityModifier::Mutable )
		{
			REPORT_ERROR( GlobalVariableMustBeConstexpr, names_scope.GetErrors(), auto_variable_declaration->file_pos_, auto_variable_declaration->name );
			FAIL_RETURN;
		}

		// Destruction frame for temporary variables of initializer expression.
		const StackVariablesStorage temp_variables_storage( function_context );

		const Variable initializer_experrsion= BuildExpressionCodeEnsureVariable( auto_variable_declaration->initializer_expression, names_scope, function_context );

		{ // Check expression type. Expression can have exotic types, such "Overloading functions set", "class name", etc.
			const bool type_is_ok=
				initializer_experrsion.type.GetFundamentalType() != nullptr ||
				initializer_experrsion.type.GetArrayType() != nullptr ||
				initializer_experrsion.type.GetTupleType() != nullptr ||
				initializer_experrsion.type.GetClassType() != nullptr ||
				initializer_experrsion.type.GetEnumType() != nullptr ||
				initializer_experrsion.type.GetFunctionPointerType() != nullptr;
			if( !type_is_ok || initializer_experrsion.type == invalid_type_ )
			{
				REPORT_ERROR( InvalidTypeForAutoVariable, names_scope.GetErrors(), auto_variable_declaration->file_pos_, initializer_experrsion.type );
				FAIL_RETURN;
			}
		}

		Variable variable;
		variable.type= initializer_experrsion.type;
		variable.value_type= ValueType::ConstReference;
		variable.location= Variable::Location::Pointer;

		if( !EnsureTypeCompleteness( variable.type, TypeCompleteness::Complete ) ) // Global variables are all constexpr. Full completeness required for constexpr.
		{
			REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), auto_variable_declaration->file_pos_, variable.type );
			FAIL_RETURN;
		}
		if( !variable.type.CanBeConstexpr() )
		{
			REPORT_ERROR( InvalidTypeForConstantExpressionVariable, names_scope.GetErrors(), auto_variable_declaration->file_pos_ );
			FAIL_RETURN;
		}

		if( auto_variable_declaration->reference_modifier == ReferenceModifier::Reference )
		{
			if( initializer_experrsion.value_type == ValueType::Value )
			{
				REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), auto_variable_declaration->file_pos_ );
				FAIL_RETURN;
			}
			if( initializer_experrsion.value_type == ValueType::ConstReference && variable.value_type != ValueType::ConstReference )
			{
				REPORT_ERROR( BindingConstReferenceToNonconstReference, names_scope.GetErrors(), auto_variable_declaration->file_pos_ );
				FAIL_RETURN;
			}

			variable.llvm_value= initializer_experrsion.llvm_value;
			variable.constexpr_value= initializer_experrsion.constexpr_value;
		}
		else if( auto_variable_declaration->reference_modifier == ReferenceModifier::None )
		{
			llvm::GlobalVariable* const global_variable= CreateGlobalConstantVariable( variable.type, MangleGlobalVariable( names_scope, auto_variable_declaration->name ) );
			variable.llvm_value= global_variable;
			// Copy constructor for constexpr type is trivial, so, we can just take constexpr value of source.
			variable.constexpr_value= initializer_experrsion.constexpr_value;

			if( initializer_experrsion.value_type == ValueType::Value )
			{
				CopyBytes( initializer_experrsion.llvm_value, variable.llvm_value, variable.type, function_context );
				variable.constexpr_value= initializer_experrsion.constexpr_value; // Move can preserve constexpr.
			}
			else
				BuildCopyConstructorPart(
					variable.llvm_value, initializer_experrsion.llvm_value,
					variable.type,
					function_context );

			if( global_variable != nullptr && variable.constexpr_value != nullptr )
				global_variable->setInitializer( variable.constexpr_value );
		}
		else U_ASSERT(false);

		if( variable.constexpr_value == nullptr )
		{
			REPORT_ERROR( VariableInitializerIsNotConstantExpression, names_scope.GetErrors(), auto_variable_declaration->file_pos_ );
			FAIL_RETURN;
		}

		// Do not call destructors, because global variables can be only constexpr and any constexpr type have trivial destructor.

		global_variable_value= Value( variable, auto_variable_declaration->file_pos_ );
	}
	else U_ASSERT(false);

	#undef FAIL_RETURN
}

size_t CodeBuilder::GlobalThingDetectloop( const GlobalThing& global_thing )
{
	for( const GlobalThing& prev_thing : global_things_stack_ )
		if( prev_thing.thing_ptr == global_thing.thing_ptr && prev_thing.completeness == global_thing.completeness )
			return size_t( &prev_thing - global_things_stack_.data() );

	return ~0u;
}

void CodeBuilder::GlobalThingReportAboutLoop( const size_t loop_start_stack_index, const std::string& last_loop_element_name, const FilePos& last_loop_element_file_pos )
{
	std::string description;

	FilePos min_file_pos= last_loop_element_file_pos;
	for( size_t i= loop_start_stack_index; i < global_things_stack_.size(); ++i )
	{
		min_file_pos= std::min( min_file_pos, global_things_stack_[i].file_pos );
		description+= global_things_stack_[i].name + " -> ";
	}
	description+= last_loop_element_name;

	REPORT_ERROR( GlobalsLoopDetected, global_errors_, min_file_pos, description );
}

} // namespace CodeBuilderPrivate

} // namespace U
