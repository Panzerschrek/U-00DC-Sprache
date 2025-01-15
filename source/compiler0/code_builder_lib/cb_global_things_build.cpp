#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

namespace
{

void AddAncestorsAccessRights_r( Class& the_class, const ClassPtr ancestor_class )
{
	the_class.members->AddAccessRightsFor( ancestor_class, ClassMemberVisibility::Protected );
	for( const Class::Parent& parent : ancestor_class->parents )
		AddAncestorsAccessRights_r( the_class, parent.class_ );
}

//
// Globals loops detecting
//

class GlobalsLoopsDetectorGuard
{
public:
	GlobalsLoopsDetectorGuard( std::function<void()> function ) : function_(std::move(function)) {}
	~GlobalsLoopsDetectorGuard(){ function_(); }
private:
	std::function<void()> function_;
};
#define DETECT_GLOBALS_LOOP( in_thing_ptr, in_name, in_src_loc ) \
	{ \
		const SrcLoc src_loc__= in_src_loc; \
		const GlobalThing global_thing( static_cast<const void*>(in_thing_ptr), in_name, src_loc__ ); \
		const size_t loop_pos= GlobalThingDetectloop( global_thing ); \
		if( loop_pos != ~0u ) \
		{ \
			GlobalThingReportAboutLoop( loop_pos, in_name, src_loc__ ); \
			return; \
		} \
		global_things_stack_.push_back( global_thing ); \
	} \
	GlobalsLoopsDetectorGuard globals_loop_detector_guard( [this]{ global_things_stack_.pop_back(); } );

void SortClassFields( Class& class_, ClassFieldsVector<llvm::Type*>& fields_llvm_types, const llvm::DataLayout& data_layout )
{
	// Fields in original order
	using FieldsMap= std::map< uint32_t, ClassFieldPtr >;
	FieldsMap fields;

	bool fields_are_ok= true;
	class_.members->ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( const ClassFieldPtr field= value.GetClassField() )
			{
				fields[field->index]= field;
				if( !field->is_reference && !field->type.GetLLVMType()->isSized() )
					fields_are_ok= false;
			}
		} );

	if( fields.empty() || !fields_are_ok )
		return;

	uint32_t field_index= fields.begin()->first;

	fields_llvm_types.resize( field_index ); // Remove all fields ( parent classes and virtual table pointers are not in fields list ).

	// Calculate start offset, include parents fields, virtual table pointer.
	uint64_t current_offset= 0u;
	for( llvm::Type* type : fields_llvm_types )
	{
		const uint64_t alignment= data_layout.getABITypeAlign( type ).value();
		const uint64_t padding= ( alignment - current_offset % alignment ) % alignment;
		current_offset+= padding + data_layout.getTypeAllocSize( type );
	}

	// Sort fields, minimize paddings and minimize fields reordering.
	while( !fields.empty() )
	{
		FieldsMap::iterator best_field_it= fields.begin();
		uint64_t best_field_padding= ~0u;
		for( auto it= fields.begin(); it != fields.end(); ++it )
		{
			const uint64_t alignment= data_layout.getABITypeAlign( best_field_it->second->is_reference ? it->second->type.GetLLVMType()->getPointerTo() : it->second->type.GetLLVMType() ).value();
			U_ASSERT( alignment != 0u );

			const uint64_t padding= ( alignment - current_offset % alignment ) % alignment;
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
		current_offset+= best_field_padding + data_layout.getTypeAllocSize( best_field_llvm_type );
		fields.erase( best_field_it );
	}
}

} // namespace

//
// CodeBuilder
//

bool CodeBuilder::IsTypeComplete( const Type& type ) const
{
	if( type.GetFundamentalType() != nullptr )
		return true;
	else if( type.GetFunctionPointerType() != nullptr )
		return true;
	else if( const auto enum_type= type.GetEnumType() )
		return enum_type->syntax_element == nullptr;
	else if( const auto array_type= type.GetArrayType() )
		return IsTypeComplete( array_type->element_type );
	else if( type.GetRawPointerType() != nullptr )
		return true; // Pointer is always complete.
	else if( const auto tuple_type= type.GetTupleType() )
	{
		bool all_complete= true;
		for( const Type& element_type : tuple_type->element_types )
			all_complete= all_complete && IsTypeComplete( element_type );
		return all_complete;
	}
	else if( const auto class_type= type.GetClassType() )
		return class_type->is_complete;
	else
	{
		U_ASSERT(false);
		return false;
	}
}

bool CodeBuilder::EnsureTypeComplete( const Type& type )
{
	if( type.GetFundamentalType() != nullptr )
		return true;
	else if( type.GetFunctionPointerType() != nullptr )
		return true;
	else if( const auto enum_type= type.GetEnumType() )
	{
		GlobalThingBuildEnum( enum_type );
		return true;
	}
	else if( const auto array_type= type.GetArrayType() )
		return EnsureTypeComplete( array_type->element_type );
	else if( type.GetRawPointerType() != nullptr )
		return true; // Pointer is always complete.
	else if( const auto tuple_type= type.GetTupleType() )
	{
		bool ok= true;
		for( const Type& element_type : tuple_type->element_types )
			ok&= EnsureTypeComplete( element_type );
		return ok;
	}
	else if( const auto class_type= type.GetClassType() )
	{
		GlobalThingBuildClass( class_type );
		return class_type->is_complete;
	}
	else U_ASSERT(false);

	return false;
}

bool CodeBuilder::ReferenceIsConvertible( const Type& from, const Type& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	if( from == to )
		return true;

	if( !EnsureTypeComplete( from ) )
		REPORT_ERROR( UsingIncompleteType, errors_container, src_loc, from );
	if( !EnsureTypeComplete(   to ) )
		REPORT_ERROR( UsingIncompleteType, errors_container, src_loc,   to );

	return from.ReferenceIsConvertibleTo(to);
}

void CodeBuilder::GlobalThingBuildNamespace( NamesScope& names_scope )
{
	names_scope.ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( const NamesScopePtr inner_namespace= value.GetNamespace() )
				GlobalThingBuildNamespace( *inner_namespace );
			else if( const OverloadedFunctionsSetPtr functions_set= value.GetFunctionsSet() )
				BuildFunctionsSetBodies( names_scope, *functions_set );
			else if( const Type* const type= value.GetTypeName() )
			{
				if( type->GetFundamentalType() != nullptr ||
					type->GetFunctionPointerType() != nullptr ||
					type->GetArrayType() != nullptr ||
					type->GetRawPointerType() != nullptr ||
					type->GetTupleType() != nullptr )
				{}
				else if( const ClassPtr class_type= type->GetClassType() )
				{
					// Build classes only from parent namespace.
					// Otherwise we can get loop, using type alias.
					if( class_type->members->GetParent() == &names_scope )
					{
						GlobalThingBuildClass( class_type );
						GlobalThingBuildNamespace( *class_type->members );
					}
				}
				else if( const EnumPtr enum_type= type->GetEnumType() )
					GlobalThingBuildEnum( enum_type );
				else U_ASSERT(false);
			}
			else if( const auto type_templates_set= value.GetTypeTemplatesSet() )
				GlobalThingBuildTypeTemplatesSet( names_scope, *type_templates_set );
			else if( value.GetClassField() != nullptr ) {} // Can be in classes.
			else if( value.GetVariable() != nullptr ){}
			else if( value.GetErrorValue() != nullptr ){}
			else if( const auto static_assert_= value.GetStaticAssert() )
			{
				BuildStaticAssert( *static_assert_, names_scope, *global_function_context_ );
				global_function_context_->args_preevaluation_cache.clear();
			}
			else if( value.GetTypeAlias() != nullptr )
				GlobalThingBuildTypeAlias( names_scope, value );
			else if( value.GetIncompleteGlobalVariable() != nullptr )
				GlobalThingBuildVariable( names_scope, value );
			else if( value.GetMixins() != nullptr ) {} // Nothing to do here. Mixins should be expanded earlier.
			else U_ASSERT(false);
		});
}

void CodeBuilder::PrepareFunctionsSet( NamesScope& names_scope, OverloadedFunctionsSet& functions_set )
{
	if( functions_set.syntax_elements.empty() &&
		functions_set.out_of_line_syntax_elements.empty() &&
		functions_set.template_syntax_elements.empty() )
		return;

	SrcLoc functions_set_src_loc;
	std::string functions_set_name;
	if( !functions_set.syntax_elements.empty() )
	{
		functions_set_src_loc= functions_set.syntax_elements.front()->src_loc;
		functions_set_name= functions_set.syntax_elements.front()->name.back().name;
	}
	else if( !functions_set.template_syntax_elements.empty() )
	{
		functions_set_src_loc= functions_set.template_syntax_elements.front()->src_loc;
		functions_set_name= functions_set.template_syntax_elements.front()->function->name.back().name;
	}
	DETECT_GLOBALS_LOOP( &functions_set, functions_set_name, functions_set_src_loc );

	for( const Synt::Function* const function : functions_set.syntax_elements )
	{
		const size_t function_index= PrepareFunction( names_scope, functions_set.base_class, functions_set, *function, false );

		if( function_index == ~0u )
			continue;
		FunctionVariable& function_variable= functions_set.functions[function_index];

		// Immediately build functions with auto return type.
		if( function->type.IsAutoReturn() && !function_variable.has_body && function->block != nullptr )
		{
			// Preprocess function in order to deduce return type and reference notation.
			ReturnTypeDeductionContext return_type_deduction_context;
			ReferenceNotationDeductionContext reference_notation_deduction_context;

			if( function_variable.constexpr_kind == FunctionVariable::ConstexprKind::NonConstexpr )
				function_variable.constexpr_kind= FunctionVariable::ConstexprKind::ConstexprAuto; // We can deduce "constexpr" property for all auto-return functions.

			BuildFuncCode(
				function_variable,
				functions_set.base_class,
				names_scope,
				functions_set_name,
				&return_type_deduction_context,
				&reference_notation_deduction_context );

			// Update function type.
			function_variable.type.return_type= return_type_deduction_context.return_type.value_or( void_type_ );
			function_variable.type.return_references= std::move(reference_notation_deduction_context.return_references);
			function_variable.type.return_inner_references= std::move(reference_notation_deduction_context.return_inner_references);
			function_variable.type.references_pollution= std::move(reference_notation_deduction_context.references_pollution);

			function_variable.has_body= false;
			// Remove old LLVM function and create new one (with name based on exact deduced function type).
			function_variable.llvm_function->function->eraseFromParent();
			function_variable.llvm_function= std::make_shared<LazyLLVMFunction>( function_variable.no_mangle ? function->name.back().name : mangler_->MangleFunction( names_scope, function->name.back().name, function_variable.type ) );

			// Compile function when its type is exactly known.
			BuildFuncCode( function_variable, functions_set.base_class, names_scope, functions_set_name );
		}
	}
	functions_set.syntax_elements.clear();

	for( const Synt::Function* const function : functions_set.out_of_line_syntax_elements )
		PrepareFunction( names_scope, functions_set.base_class, functions_set, *function, true );
	functions_set.out_of_line_syntax_elements.clear();

	for( const Synt::FunctionTemplate* const function_template : functions_set.template_syntax_elements )
		PrepareFunctionTemplate( *function_template, functions_set, names_scope, functions_set.base_class );
	functions_set.template_syntax_elements.clear();

	functions_set.has_unbuilt_constexpr_functions= false;
	for( const FunctionVariable& function_variable : functions_set.functions )
		functions_set.has_unbuilt_constexpr_functions|= function_variable.constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete;
}

void CodeBuilder::BuildFunctionsSetBodies( NamesScope& names_scope, OverloadedFunctionsSet& functions_set )
{
	PrepareFunctionsSet( names_scope, functions_set );

	for( FunctionVariable& function_variable : functions_set.functions )
	{
		if(
			skip_building_generated_functions_ &&
			function_variable.constexpr_kind == FunctionVariable::ConstexprKind::NonConstexpr &&
			names_scope.IsInsideTemplate() )
		{
			// This is some non-constexpr function inside a template and we skip building such functions.
			continue;
		}

		if( function_variable.syntax_element != nullptr &&
			function_variable.syntax_element->block != nullptr &&
			!function_variable.has_body &&
			!function_variable.syntax_element->type.IsAutoReturn() &&
			!function_variable.is_inherited )
		{
			BuildFuncCode(
				function_variable,
				functions_set.base_class,
				names_scope,
				function_variable.syntax_element->name.back().name );
		}
	}
}

void CodeBuilder::PrepareFunctionsSetAndBuildConstexprBodies( NamesScope& names_scope, OverloadedFunctionsSet& functions_set )
{
	PrepareFunctionsSet( names_scope, functions_set );

	if( !functions_set.has_unbuilt_constexpr_functions )
		return; // Nothing to do.

	functions_set.has_unbuilt_constexpr_functions= false; // // Reset the flag in order to avoid recursion.

	for( FunctionVariable& function_variable : functions_set.functions )
		{
			if( function_variable.syntax_element != nullptr &&
				function_variable.syntax_element->block != nullptr &&
				!function_variable.has_body &&
				function_variable.constexpr_kind != FunctionVariable::ConstexprKind::NonConstexpr )
			{
				BuildFuncCode(
					function_variable,
					functions_set.base_class,
					names_scope,
					function_variable.syntax_element->name.back().name );
			}
		}
}

void CodeBuilder::GlobalThingPrepareClassParentsList( const ClassPtr class_type )
{
	if( class_type->parents_list_prepared || class_type->syntax_element == nullptr )
		return;

	DETECT_GLOBALS_LOOP( &class_type, class_type->members->ToString(), class_type->src_loc );

	const Synt::Class& class_declaration= *class_type->syntax_element;

	// Perform only steps necessary to build parents list.
	// Do not even require completeness of parent class.
	NamesScope& class_parent_namespace= *class_type->members->GetParent();
	for( const Synt::ComplexName& parent : class_declaration.parents )
	{
		const Value parent_value= ResolveValue( class_parent_namespace, *global_function_context_, parent );
		global_function_context_->args_preevaluation_cache.clear();

		const Type* const type_name= parent_value.GetTypeName();
		if( type_name == nullptr )
		{
			REPORT_ERROR( NameIsNotTypeName, class_parent_namespace.GetErrors(), Synt::GetSrcLoc(parent), parent_value.GetKindName() );
			continue;
		}

		const ClassPtr parent_class= type_name->GetClassType();
		if( parent_class == nullptr )
		{
			REPORT_ERROR( CanNotDeriveFromThisType, class_parent_namespace.GetErrors(), class_declaration.src_loc, type_name );
			continue;
		}

		bool duplicated= false;
		for( const Class::Parent& parent : class_type->parents )
			duplicated= duplicated || parent.class_ == parent_class;
		if( duplicated )
		{
			REPORT_ERROR( DuplicatedParentClass, class_parent_namespace.GetErrors(), class_declaration.src_loc, type_name );
			continue;
		}

		class_type->parents.emplace_back();
		class_type->parents.back().class_= parent_class;

		GlobalThingPrepareClassParentsList( parent_class );
		AddAncestorsAccessRights_r( *class_type, parent_class );

	} // for parents

	class_type->parents_list_prepared= true;
}

void CodeBuilder::GlobalThingBuildClass( const ClassPtr class_type )
{
	Class& the_class= *class_type;

	if( the_class.is_complete )
		return;

	if( const auto typeinfo_class_description= std::get_if<TypeinfoClassDescription>( &class_type->generated_class_data ) )
	{
		const Type& type= typeinfo_class_description->source_type;
		BuildFullTypeinfo( type, typeinfo_cache_[type].variable, *the_class.members->GetRoot() );
		return;
	}

	GlobalThingPrepareClassParentsList( class_type );

	const Synt::Class& class_declaration= *the_class.syntax_element;
	const std::string& class_name= class_declaration.name;

	DETECT_GLOBALS_LOOP( &the_class, the_class.members->ToString(), the_class.src_loc );

	NamesScope& class_parent_namespace= *the_class.members->GetParent();
	// Perform remaining check of parents.
	for( Class::Parent& parent : the_class.parents )
	{
		if( !EnsureTypeComplete( parent.class_ ) )
		{
			REPORT_ERROR( UsingIncompleteType, class_parent_namespace.GetErrors(), class_declaration.src_loc, parent.class_ );
			return;
		}

		const auto parent_kind= parent.class_->kind;
		if( !( parent_kind == Class::Kind::Abstract || parent_kind == Class::Kind::Interface || parent_kind == Class::Kind::PolymorphNonFinal ) )
		{
			REPORT_ERROR( CanNotDeriveFromThisType, class_parent_namespace.GetErrors(), class_declaration.src_loc, parent.class_ );
			return;
		}

		if( parent_kind != Class::Kind::Interface ) // not interface=base
		{
			if( the_class.base_class != nullptr )
			{
				REPORT_ERROR( DuplicatedBaseClass, class_parent_namespace.GetErrors(), class_declaration.src_loc, parent.class_ );
				return;
			}
			the_class.base_class= parent.class_;
		}
	}

	the_class.no_discard= class_declaration.no_discard;

	// Pre-mark class as polymorph. Later we know class kind exactly, now, we only needs to know, that is polymorph - for virtual functions preparation.
	if( class_declaration.kind_attribute == Synt::ClassKindAttribute::Polymorph ||
		class_declaration.kind_attribute == Synt::ClassKindAttribute::Interface ||
		class_declaration.kind_attribute == Synt::ClassKindAttribute::Abstract ||
		!class_declaration.parents.empty() )
		the_class.kind= Class::Kind::PolymorphNonFinal;
	else if( class_declaration.kind_attribute == Synt::ClassKindAttribute::Struct )
		the_class.kind= Class::Kind::Struct;
	else
		the_class.kind= Class::Kind::NonPolymorph;

	the_class.can_be_constexpr= the_class.kind == Class::Kind::Struct;

	the_class.members->ForEachValueInThisScope(
		[&]( Value& value )
		{
			ClassFieldPtr const class_field= value.GetClassField();
			if( class_field == nullptr )
				return;

			const Synt::ClassField& in_field= *class_field->syntax_element;

			class_field->class_= class_type;
			class_field->is_reference= in_field.reference_modifier == Synt::ReferenceModifier::Reference;
			class_field->type= PrepareType( class_field->syntax_element->type, *the_class.members, *global_function_context_ );
			global_function_context_->args_preevaluation_cache.clear();

			if( !class_field->is_reference || in_field.mutability_modifier == Synt::MutabilityModifier::Constexpr )
			{
				// Full type completeness required for value-fields and constexpr reference-fields.
				if( !EnsureTypeComplete( class_field->type ) )
				{
					REPORT_ERROR( UsingIncompleteType, class_parent_namespace.GetErrors(), in_field.src_loc, class_field->type );
					return;
				}
			}

			if( class_field->is_reference )
			{
				if( !EnsureTypeComplete( class_field->type ) )
				{
					REPORT_ERROR( UsingIncompleteType, class_parent_namespace.GetErrors(), in_field.src_loc, class_field->type );
					return;
				}

				const size_t reference_tag_count= class_field->type.ReferenceTagCount();
				if( reference_tag_count > 0 )
				{
					if( class_field->type.GetSecondOrderInnerReferenceKind(0) != SecondOrderInnerReferenceKind::None )
						REPORT_ERROR( ReferenceIndirectionDepthExceeded, class_parent_namespace.GetErrors(), in_field.src_loc, 2, in_field.name );

					if( reference_tag_count > 1 )
						REPORT_ERROR( MoreThanOneInnerReferenceTagForSecondOrderReferenceField, class_parent_namespace.GetErrors(), in_field.src_loc, in_field.name );
				}
			}
			else if( class_field->type.IsAbstract() )
				REPORT_ERROR( ConstructingAbstractClassOrInterface, class_parent_namespace.GetErrors(), in_field.src_loc, class_field->type );

			if( class_field->is_reference ) // Reference-fields are immutable by default
				class_field->is_mutable= in_field.mutability_modifier == Synt::MutabilityModifier::Mutable;
			else // But value-fields are mutable by default
				class_field->is_mutable= in_field.mutability_modifier != Synt::MutabilityModifier::Immutable;

			// Disable constexpr, if field can not be constexpr, or if field is mutable reference.
			if( !class_field->type.CanBeConstexpr() || ( class_field->is_reference && class_field->is_mutable ) )
				the_class.can_be_constexpr= false;
		} );

	// Determine inner references.
	{
		// Inherit inner references of parents.
		// Normally any reference may be inhereted only from base, but not interfaces, since interfaces have no fields.
		if( the_class.base_class != nullptr )
			the_class.inner_references= the_class.base_class->inner_references;

		const bool has_parents_with_references_inside= !the_class.inner_references.empty();

		// Collect fields for which reference notation is required.

		ClassFieldsVector<ClassFieldPtr> reference_fields;
		ClassFieldsVector<ClassFieldPtr> fields_with_references_inside;
		bool has_fields_with_reference_notation= false;

		the_class.members->ForEachValueInThisScope(
			[&]( const Value& value )
			{
				const ClassFieldPtr field= value.GetClassField();
				if( field == nullptr )
					return;

				if( field->is_reference )
				{
					reference_fields.push_back(field);

					if( !std::holds_alternative< Synt::EmptyVariant >( field->syntax_element->inner_reference_tags_expression ) )
					{
						// Reference tags of a struct are used only for reference fields or value fields with references inside.
						// They are not used for second order references.
						// So, specifying inner reference tags for a reference field is useless or even confusing.
						REPORT_ERROR( InnerReferenceTagsForReferenceField, the_class.members->GetErrors(), field->syntax_element->src_loc, field->syntax_element->name );
					}
				}
				else
				{
					if( field->type.ReferenceTagCount() > 0 )
						fields_with_references_inside.push_back(field);
					else
					{
						if( !std::holds_alternative< Synt::EmptyVariant >( field->syntax_element->inner_reference_tags_expression ) )
						{
							if( const auto reference_tags= EvaluateReferenceFieldInnerTags( *the_class.members, field->syntax_element->inner_reference_tags_expression ) )
							{
								if( reference_tags->size() != 0 )
									REPORT_ERROR( InnerReferenceTagCountMismatch, the_class.members->GetErrors(), field->syntax_element->src_loc, size_t(0), reference_tags->size() );
							}
						}
					}
				}

				has_fields_with_reference_notation |=
					!std::holds_alternative< Synt::EmptyVariant >( field->syntax_element->reference_tag_expression ) ||
					!std::holds_alternative< Synt::EmptyVariant >( field->syntax_element->inner_reference_tags_expression );
			});

		// Determine reference mapping where it is needed.

		if( reference_fields.size() == 1 && fields_with_references_inside.size() == 0 && !has_fields_with_reference_notation && !has_parents_with_references_inside )
		{
			// Special case - class contains single reference field.
			ClassField& field= *reference_fields.front();
			field.reference_tag= uint8_t(0u);

			the_class.inner_references.push_back( InnerReference( field.is_mutable ? InnerReferenceKind::Mut : InnerReferenceKind::Imut ) );
		}
		else if( reference_fields.size() == 0 && fields_with_references_inside.size() == 1 && !has_fields_with_reference_notation && !has_parents_with_references_inside )
		{
			// Special case - class contains single field with references inside. Map reference tags of this field to reference tags of the whole class.
			ClassField& field= *fields_with_references_inside.front();
			const auto reference_tag_count= field.type.ReferenceTagCount();

			field.inner_reference_tags.resize( reference_tag_count );
			the_class.inner_references.reserve( reference_tag_count );
			for( size_t i= 0; i < reference_tag_count; ++i )
			{
				field.inner_reference_tags[i]= uint8_t(i);
				the_class.inner_references.push_back( InnerReference( field.type.GetInnerReferenceKind(i) ) );
			}
		}
		else
		{
			// General case - require reference notation.

			for( const ClassFieldPtr& reference_field : reference_fields )
			{
				std::optional<uint8_t> reference_tag_opt;
				if( !std::holds_alternative< Synt::EmptyVariant >( reference_field->syntax_element->reference_tag_expression ) )
					reference_tag_opt= EvaluateReferenceFieldTag( *the_class.members, reference_field->syntax_element->reference_tag_expression );
				else
					REPORT_ERROR( ExpectedReferenceNotation, the_class.members->GetErrors(), reference_field->syntax_element->src_loc, reference_field->syntax_element->name );

				const InnerReferenceKind inner_reference_kind= reference_field->is_mutable ? InnerReferenceKind::Mut : InnerReferenceKind::Imut;

				if( reference_tag_opt != std::nullopt )
				{
					const auto reference_tag= *reference_tag_opt;
					reference_field->reference_tag= reference_tag;

					if( reference_tag >= the_class.inner_references.size() )
						the_class.inner_references.resize( size_t(reference_tag + 1), InnerReference( InnerReferenceKind::Imut ) );

					the_class.inner_references[ size_t(reference_tag) ].kind= inner_reference_kind;
				}
				else
				{
					// Fallback for error cases with no notation - link reference field with tag 0.
					reference_field->reference_tag= uint8_t(0u);

					if( the_class.inner_references.empty() )
						the_class.inner_references.push_back( InnerReference( inner_reference_kind ) );
					else
						the_class.inner_references.front().kind= inner_reference_kind;
				}
			}

			for( const ClassFieldPtr& field : fields_with_references_inside )
			{
				std::optional< llvm::SmallVector<uint8_t, 4> > reference_tags;
				if( !std::holds_alternative< Synt::EmptyVariant >( field->syntax_element->inner_reference_tags_expression ) )
					reference_tags= EvaluateReferenceFieldInnerTags( *the_class.members, field->syntax_element->inner_reference_tags_expression );
				else
					REPORT_ERROR( ExpectedReferenceNotation, the_class.members->GetErrors(), field->syntax_element->src_loc, field->syntax_element->name );

				const auto reference_tag_count= field->type.ReferenceTagCount();

				if( reference_tags != std::nullopt )
				{
					if( reference_tags->size() != reference_tag_count )
					{
						REPORT_ERROR( InnerReferenceTagCountMismatch, the_class.members->GetErrors(), field->syntax_element->src_loc, reference_tag_count, reference_tags->size() );
						reference_tags->resize( reference_tag_count, uint8_t(0) );
					}
					field->inner_reference_tags= std::move(*reference_tags);

					for( size_t i= 0; i < field->inner_reference_tags.size(); ++i )
					{
						const size_t tag= field->inner_reference_tags[i];
						if( tag >= the_class.inner_references.size() )
							the_class.inner_references.resize( tag + 1, InnerReference( InnerReferenceKind::Imut ) );
						the_class.inner_references[ tag ].kind= field->type.GetInnerReferenceKind(i);
					}
				}
				else
				{
					// Fallback for error cases with no notation - link all field tags with tag 0.

					field->inner_reference_tags.resize( reference_tag_count, uint8_t(0) );

					if( the_class.inner_references.empty() )
						the_class.inner_references.push_back( InnerReference( InnerReferenceKind::Imut ) );
					for( size_t i= 0; i < reference_tag_count; ++i )
						the_class.inner_references.front().kind= field->type.GetInnerReferenceKind(i);
				}
			}
		}

		// Setup second order inner references.

		for( const ClassFieldPtr& field : reference_fields )
		{
			if( field->type.ReferenceTagCount() > 0 )
			{
				const size_t i= field->reference_tag;
				InnerReference& inner_reference= the_class.inner_references[i];

				const SecondOrderInnerReferenceKind second_order_kind=
					field->type.GetInnerReferenceKind(0) == InnerReferenceKind::Imut
						? SecondOrderInnerReferenceKind::Imut
						: SecondOrderInnerReferenceKind::Mut;

				if( inner_reference.second_order_kind == SecondOrderInnerReferenceKind::None )
					inner_reference.second_order_kind= second_order_kind;
				else if( inner_reference.second_order_kind != second_order_kind )
				{
					std::string s;
					s.push_back( char( 'a' + i ) );
					REPORT_ERROR( MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag, the_class.members->GetErrors(), class_declaration.src_loc, s );
				}
			}
		}

		for( const ClassFieldPtr& field : fields_with_references_inside )
		{
			for( size_t j= 0; j < field->inner_reference_tags.size(); ++j )
			{
				const size_t i= field->inner_reference_tags[j];
				InnerReference& inner_reference= the_class.inner_references[i];

				const SecondOrderInnerReferenceKind second_order_kind= field->type.GetSecondOrderInnerReferenceKind(j);

				if( inner_reference.second_order_kind == SecondOrderInnerReferenceKind::None )
					inner_reference.second_order_kind= second_order_kind;
				else if( inner_reference.second_order_kind != second_order_kind )
				{
					std::string s;
					s.push_back( char( 'a' + i ) );
					REPORT_ERROR( MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag, the_class.members->GetErrors(), class_declaration.src_loc, s );
				}
			}
		}

		// Check consistency of result tags.
		llvm::SmallVector<bool, 32> reference_tags_usage_flags;
		reference_tags_usage_flags.resize( the_class.inner_references.size(), false );

		for( const Class::Parent& parent : the_class.parents )
		{
			const Class& parent_class= *parent.class_;

			// Forbid changing inner references in inheritance.
			// Otherwise it may be possible to break reference-checking rules by using virtual methods.
			// Forbidding changing inner references ensures that no control for inner references can be skipped by casting a reference to a parent with less inner references.
			if( the_class.inner_references.size() != parent_class.inner_references.size() )
				REPORT_ERROR(
					ChangingReferenceTagCountInInheritance,
					the_class.members->GetErrors(),
					class_declaration.src_loc,
					parent_class.inner_references.size(),
					the_class.inner_references.size() );
			else
			{
				for( size_t i= 0; i < parent_class.inner_references.size(); ++i )
				{
					reference_tags_usage_flags[i]= true;
					if( parent_class.inner_references[i].kind != the_class.inner_references[i].kind )
					{
						std::string s;
						s.push_back( char( 'a' + i ) );
						REPORT_ERROR( MixingMutableAndImmutableReferencesInSameReferenceTag, the_class.members->GetErrors(), class_declaration.src_loc, s );
					}
					if( parent_class.inner_references[i].second_order_kind != the_class.inner_references[i].second_order_kind )
					{
						std::string s;
						s.push_back( char( 'a' + i ) );
						REPORT_ERROR( MixingMutableAndImmutableSecondOrderReferencesInSameReferenceTag, the_class.members->GetErrors(), class_declaration.src_loc, s );
					}
				}
			}
		}

		the_class.members->ForEachValueInThisScope(
			[&]( const Value& value )
			{
				const ClassFieldPtr field= value.GetClassField();
				if( field == nullptr )
					return;

				if( field->is_reference )
				{
					U_ASSERT( field->reference_tag < the_class.inner_references.size() );
					reference_tags_usage_flags[ field->reference_tag ]= true;

					if(
						(  field->is_mutable && the_class.inner_references[ field->reference_tag ].kind == InnerReferenceKind::Imut ) ||
						( !field->is_mutable && the_class.inner_references[ field->reference_tag ].kind == InnerReferenceKind:: Mut ) )
					{
						std::string s;
						s.push_back( char( 'a' + field->reference_tag ) );
						REPORT_ERROR( MixingMutableAndImmutableReferencesInSameReferenceTag, the_class.members->GetErrors(), class_declaration.src_loc, s );
					}
				}
				else
				{
					U_ASSERT( field->inner_reference_tags.size() == field->type.ReferenceTagCount() );
					for( size_t i= 0; i < field->inner_reference_tags.size(); ++i )
					{
						const size_t tag= field->inner_reference_tags[i];
						U_ASSERT( tag < the_class.inner_references.size() );
						reference_tags_usage_flags[ tag ]= true;

						if( field->type.GetInnerReferenceKind(i) != the_class.inner_references[tag].kind )
						{
							std::string s;
							s.push_back( char( 'a' + tag ) );
							REPORT_ERROR( MixingMutableAndImmutableReferencesInSameReferenceTag, the_class.members->GetErrors(), class_declaration.src_loc, s );
						}
					}
				}
			} );

		for( size_t i= 0; i < the_class.inner_references.size(); ++i )
		{
			if( !reference_tags_usage_flags[i] )
			{
				std::string s;
				s.push_back( char( 'a' + i ) );
				REPORT_ERROR( UnusedReferenceTag, the_class.members->GetErrors(), class_declaration.src_loc, s );
			}
		}
	}

	// Fill llvm struct type fields
	ClassFieldsVector<llvm::Type*> fields_llvm_types;

	// Base must be always first field.
	if( the_class.base_class != nullptr )
		fields_llvm_types.push_back( the_class.base_class->llvm_type );
	// Add non-base (interface) fields.
	for( Class::Parent& parent : the_class.parents )
	{
		if( parent.class_ == the_class.base_class )
		{
			parent.field_number= 0u;
			continue;
		}

		parent.field_number= uint32_t(fields_llvm_types.size());
		fields_llvm_types.emplace_back( parent.class_->llvm_type );
	}

	// Allocate virtual table pointer, if class has no parents.
	// If class has at least one parent, reuse it's virtual table pointer.
	bool allocate_virtual_table_pointer= false;
	if( the_class.parents.empty() && (
		class_declaration.kind_attribute == Synt::ClassKindAttribute::Abstract ||
		class_declaration.kind_attribute == Synt::ClassKindAttribute::Polymorph ||
		class_declaration.kind_attribute == Synt::ClassKindAttribute::Interface ) )
	{
		U_ASSERT( fields_llvm_types.empty() );
		fields_llvm_types.emplace_back( fundamental_llvm_types_.void_->getPointerTo() ); // set exact type later.
		allocate_virtual_table_pointer= true;
	}

	{ // Create fields.
		ClassFieldsVector< ClassFieldPtr > class_fields_in_original_order;

		the_class.members->ForEachValueInThisScope(
			[&]( Value& value )
			{
				if( const ClassFieldPtr class_field= value.GetClassField() )
					class_fields_in_original_order.push_back( class_field );
			});

		std::sort(
			class_fields_in_original_order.begin(), class_fields_in_original_order.end(),
			[](const ClassFieldPtr& l, const ClassFieldPtr& r) { return l->original_index < r->original_index; } );

		for( const ClassFieldPtr& class_field : class_fields_in_original_order )
		{
			class_field->index= uint32_t(fields_llvm_types.size());
			if( class_field->is_reference )
				fields_llvm_types.emplace_back( class_field->type.GetLLVMType()->getPointerTo() );
			else
			{
				if( !class_field->type.GetLLVMType()->isSized() )
					fields_llvm_types.emplace_back( fundamental_llvm_types_.i8_ );// May be in case of error (such dependency loop )
				else
					fields_llvm_types.emplace_back( class_field->type.GetLLVMType() );
			}
		}

		if( !class_declaration.keep_fields_order )
			SortClassFields( the_class, fields_llvm_types, data_layout_ );
	}

	// Fill container with fields names.
	the_class.fields_order.resize( fields_llvm_types.size() );
	the_class.members->ForEachValueInThisScope(
		[&]( const Value& value )
		{
			if( const auto field= value.GetClassField() )
				the_class.fields_order[field->index]= field;
		} );

	// Build declarations for some necessary methods.
	the_class.members->ForEachInThisScope(
		[&]( const std::string_view name, NamesScopeValue& names_scope_value )
		{
			const auto functions_set= names_scope_value.value.GetFunctionsSet();
			if( functions_set == nullptr )
				return;

			// We need to prepare special methods.
			bool need_to_prepare=
				name == Keywords::constructor_ ||
				name == Keywords::destructor_ ||
				name == OverloadedOperatorToString( OverloadedOperator::Assign ) ||
				name == OverloadedOperatorToString( OverloadedOperator::CompareEqual );

			if( !need_to_prepare )
			{
				// Functions declared virtual requires preparation in order to build virtual table later.
				for( const FunctionVariable& function : functions_set->functions )
					need_to_prepare|= function.syntax_element != nullptr && function.syntax_element->virtual_function_kind != Synt::VirtualFunctionKind::None;
				for( const Synt::Function* const synt_function : functions_set->syntax_elements )
					need_to_prepare|= synt_function->virtual_function_kind != Synt::VirtualFunctionKind::None;
			}
			if( !need_to_prepare )
			{
				// Also we need to check parent for virtual functions.
				for( const Class::Parent& parent : the_class.parents )
				{
					if( const auto parent_member= parent.class_->members->GetThisScopeValue( name ) )
					{
						if( const auto parent_functions_set= parent_member->value.GetFunctionsSet() )
						{
							for( const FunctionVariable& parent_function : parent_functions_set->functions )
								need_to_prepare|= parent_function.virtual_table_index != ~0u;
						}
					}
				}
			}

			if( need_to_prepare )
				PrepareFunctionsSet( *the_class.members, *functions_set );
		});

	// Generate destructor prototype before perparing virtual table to mark it as virtual and setup virtual table index.
	if( the_class.members->GetThisScopeValue( Keyword( Keywords::destructor_ ) ) == nullptr )
	{
		OverloadedFunctionsSetPtr destructors_set= std::make_shared<OverloadedFunctionsSet>();
		destructors_set->functions.push_back( GenerateDestructorPrototype( class_type ) );
		destructors_set->base_class= class_type;
		the_class.members->AddName( Keyword( Keywords::destructor_ ), NamesScopeValue( std::move(destructors_set), the_class.src_loc ) );
	}

	if( the_class.kind == Class::Kind::Interface ||
		the_class.kind == Class::Kind::Abstract ||
		the_class.kind == Class::Kind::PolymorphNonFinal ||
		the_class.kind == Class::Kind::PolymorphFinal )
		PrepareClassVirtualTable( the_class );

	// Search for explicit noncopy constructors.
	if( const NamesScopeValue* const constructors_value=
		the_class.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) ) )
	{
		const OverloadedFunctionsSetConstPtr constructors= constructors_value->value.GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		for( const FunctionVariable& constructor : constructors->functions )
		{
			if( !IsCopyConstructor( constructor.type, class_type ) )
			{
				the_class.has_explicit_noncopy_constructors= true;
				break;
			}
		};
		the_class.has_explicit_noncopy_constructors |= !constructors->template_functions.empty();
	}

	// Disable constexpr possibility for structs with:
	// * explicit destructors
	// * non-default copy-assignment operators
	// * non-default copy constructors
	// * non-default equality compare operators
	if( const NamesScopeValue* const destructor_value=
		the_class.members->GetThisScopeValue( Keyword( Keywords::destructor_ ) ) )
	{
		const OverloadedFunctionsSetConstPtr destructors= destructor_value->value.GetFunctionsSet();
		// Destructors may be invalid in case of error.
		if( !destructors->functions.empty() && !destructors->functions[0].is_generated )
			the_class.can_be_constexpr= false;
	}
	if( const NamesScopeValue* const constructor_value=
		the_class.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) ) )
	{
		const OverloadedFunctionsSetConstPtr constructors= constructor_value->value.GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		for( const FunctionVariable& constructor : constructors->functions )
		{
			if( IsCopyConstructor( constructor.type, class_type ) && !constructor.is_generated )
				the_class.can_be_constexpr= false;
		}
	}
	if( const NamesScopeValue* const assignment_operator_value=
		the_class.members->GetThisScopeValue( OverloadedOperatorToString( OverloadedOperator::Assign ) ) )
	{
		const OverloadedFunctionsSetConstPtr operators= assignment_operator_value->value.GetFunctionsSet();
		U_ASSERT( operators != nullptr );
		for( const FunctionVariable& op : operators->functions )
		{
			if( IsCopyAssignmentOperator( op.type, class_type ) && !op.is_generated )
				the_class.can_be_constexpr= false;
		}
	}
	if( const NamesScopeValue* const compare_equal_value=
		the_class.members->GetThisScopeValue( OverloadedOperatorToString( OverloadedOperator::CompareEqual ) ) )
	{
		const OverloadedFunctionsSetConstPtr operators= compare_equal_value->value.GetFunctionsSet();
		U_ASSERT( operators != nullptr );
		for( const FunctionVariable& op : operators->functions )
		{
			if( IsEqualityCompareOperator( op.type, class_type ) && !op.is_generated )
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
	switch( class_declaration.kind_attribute )
	{
	case Synt::ClassKindAttribute::Struct:
		U_ASSERT( class_declaration.parents.empty() );
		the_class.kind= Class::Kind::Struct;
		break;

	case Synt::ClassKindAttribute::Class: // Class without parents and without kind attribute is non-polymorph.
		if( the_class.parents.empty() )
			the_class.kind= Class::Kind::NonPolymorph;
		else
			the_class.kind= Class::Kind::PolymorphNonFinal;
		if( class_contains_pure_virtual_functions )
		{
			REPORT_ERROR( ClassContainsPureVirtualFunctions, the_class.members->GetErrors(), class_declaration.src_loc, class_name );
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
			REPORT_ERROR( ClassContainsPureVirtualFunctions, the_class.members->GetErrors(), class_declaration.src_loc, class_name );
			the_class.kind= Class::Kind::Abstract;
		}
		break;

	case Synt::ClassKindAttribute::Polymorph:
		the_class.kind= Class::Kind::PolymorphNonFinal;
		if( class_contains_pure_virtual_functions )
		{
			REPORT_ERROR( ClassContainsPureVirtualFunctions, the_class.members->GetErrors(), class_declaration.src_loc, class_name );
			the_class.kind= Class::Kind::Abstract;
		}
		break;

	case Synt::ClassKindAttribute::Interface:
		if( the_class.field_count != 0u )
			REPORT_ERROR( FieldsForInterfacesNotAllowed, the_class.members->GetErrors(), class_declaration.src_loc );
		if( the_class.base_class != nullptr )
			REPORT_ERROR( BaseClassForInterface, the_class.members->GetErrors(), class_declaration.src_loc );
		if( the_class.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) ) != nullptr )
			REPORT_ERROR( ConstructorForInterface, the_class.members->GetErrors(), class_declaration.src_loc );
		for( const Class::VirtualTableEntry& virtual_table_entry : the_class.virtual_table )
		{
			if( !virtual_table_entry.is_pure && virtual_table_entry.name != Keywords::destructor_ )
			{
				REPORT_ERROR( NonPureVirtualFunctionInInterface, the_class.members->GetErrors(), class_declaration.src_loc, class_name );
				break;
			}
		}
		the_class.kind= Class::Kind::Interface;
		break;

	case Synt::ClassKindAttribute::Abstract:
		the_class.kind= Class::Kind::Abstract;
		break;
	};

	// Merge functions sets of parents into functions sets of this class.
	// Do the same for type templates sets.
	// Merge functions sets in order to have possibility to fetch functions sets, combined from sets of multiple parents.
	// Do not borrow other kinds of symbols (type aliases, variables, etc.) in order to avoid global things build inside wrong namespace.
	for( const Class::Parent& parent : the_class.parents )
	{
		const auto parent_class= parent.class_;
		parent_class->members->ForEachInThisScope(
			[&]( const std::string_view name, NamesScopeValue& value )
			{
				const auto parent_member_visibility= parent_class->GetMemberVisibility( name );
				if( parent_member_visibility == ClassMemberVisibility::Private )
					return; // Do not inherit private members.

				NamesScopeValue* const result_class_value= the_class.members->GetThisScopeValue(name);

				if( const auto functions= value.value.GetFunctionsSet() )
				{
					if( name == Keywords::constructor_ ||
						name == Keywords::destructor_ ||
						name == OverloadedOperatorToString( OverloadedOperator::Assign ) ||
						name == OverloadedOperatorToString( OverloadedOperator::CompareEqual ) ||
						name == OverloadedOperatorToString( OverloadedOperator::CompareOrder ) )
						return; // Do not inherit constructors, destructors, assignment operators, compare operators.

					// Prepare source functions set before merging.
					PrepareFunctionsSet( *parent_class->members, *functions );

					if( result_class_value != nullptr )
					{
						if( const auto result_class_functions= result_class_value->value.GetFunctionsSet() )
						{
							// Prepare destination functions set before merging.
							PrepareFunctionsSet( *the_class.members, *result_class_functions );

							if( the_class.GetMemberVisibility( name ) != parent_class->GetMemberVisibility( name ) )
							{
								const auto& src_loc= result_class_functions->functions.empty() ? result_class_functions->template_functions.front()->src_loc : result_class_functions->functions.front().prototype_src_loc;
								REPORT_ERROR( FunctionsVisibilityMismatch, the_class.members->GetErrors(), src_loc, name );
							}

							// Merge function sets, if result class has functions set with given name.
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
								{
									if( ApplyOverloadedFunction( *result_class_functions, parent_function, the_class.members->GetErrors(), class_declaration.src_loc ) )
										result_class_functions->functions.back().is_inherited= true;
								}
							} // for parent functions

							// TODO - merge function templates smarter.
							for( const FunctionTemplatePtr& function_template : functions->template_functions )
								result_class_functions->template_functions.push_back(function_template);
						}
					}
					else
					{
						// Take whole function set, but mark functions as inherited.
						OverloadedFunctionsSetPtr functions_set= std::make_shared<OverloadedFunctionsSet>(*functions);
						functions_set->base_class= class_type;
						for( FunctionVariable& function : functions_set->functions )
							function.is_inherited= true;

						the_class.members->AddName( name, NamesScopeValue( std::move(functions_set), SrcLoc() ) );
						the_class.SetMemberVisibility( name, parent_member_visibility );
					}
				}
				if( const auto type_templates_set= value.value.GetTypeTemplatesSet() )
				{
					// Build source type templates set.
					GlobalThingBuildTypeTemplatesSet( *parent_class->members, *type_templates_set );

					if( result_class_value != nullptr )
					{
						if( const auto result_type_templates_set= result_class_value->value.GetTypeTemplatesSet() )
						{
							// Build destination type templates set.
							GlobalThingBuildTypeTemplatesSet( *the_class.members, *result_type_templates_set );

							if( the_class.GetMemberVisibility( name ) != parent_class->GetMemberVisibility( name ) )
								REPORT_ERROR( TypeTemplatesVisibilityMismatch, the_class.members->GetErrors(), result_class_value->src_loc, name );

							for( const TypeTemplatePtr& parent_type_template : type_templates_set->type_templates )
							{
								bool overrides= false;
								for( const TypeTemplatePtr& result_type_template : result_type_templates_set->type_templates )
								{
									if( result_type_template->signature_params == parent_type_template->signature_params )
									{
										overrides= true;
										break;
									}
								}
								if( !overrides )
									result_type_templates_set->type_templates.push_back( parent_type_template );
							}
						}
					}
					else
					{
						// Just take type templates set itself. There is no problem as soon as type templates set is complete.
						the_class.members->AddName( name, value );
						the_class.SetMemberVisibility( name, parent_member_visibility );
					}
				}
			} );
	}

	PrepareClassVirtualTableType( class_type );
	if( allocate_virtual_table_pointer )
		fields_llvm_types[0]= the_class.virtual_table_llvm_type->getPointerTo();

	// Check opaque before set body for cases of errors (class body duplication).
	if( the_class.llvm_type->isOpaque() )
		the_class.llvm_type->setBody( fields_llvm_types );

	BuildPolymorphClassTypeId( class_type );
	BuildClassVirtualTable( class_type );

	the_class.is_complete= true;

	TryGenerateDefaultConstructor( class_type );
	TryGenerateDestructor( class_type );
	TryGenerateCopyConstructor( class_type );
	TryGenerateCopyAssignmentOperator( class_type );
	TryGenerateEqualityCompareOperator( class_type );

	CheckClassFieldsInitializers( class_type );
}

void CodeBuilder::GlobalThingBuildEnum( const EnumPtr enum_ )
{
	if( enum_->syntax_element == nullptr )
		return;

	DETECT_GLOBALS_LOOP( enum_, enum_->members.GetThisNamespaceName(), enum_->syntax_element->src_loc );

	const Synt::Enum& enum_decl= *enum_->syntax_element;
	NamesScope& names_scope= *enum_->members.GetParent();

	// Default underlying type is mostyl u8, but can be large for enums with a lot of values.
	if( enum_decl.members.size() <= (1 << 8) )
		enum_->underlying_type= FundamentalType( U_FundamentalType::u8_, fundamental_llvm_types_.u8_ );
	else if( enum_decl.members.size() <= (1 << 16) )
		enum_->underlying_type= FundamentalType( U_FundamentalType::u16_, fundamental_llvm_types_.u16_ );
	else
		enum_->underlying_type= FundamentalType( U_FundamentalType::u32_, fundamental_llvm_types_.u32_ );

	// Process custom underlying type.
	if( enum_decl.underlying_type_name != std::nullopt )
	{
		const Value type_value= ResolveValue( names_scope, *global_function_context_, *enum_decl.underlying_type_name );
		global_function_context_->args_preevaluation_cache.clear();
		const Type* const type= type_value.GetTypeName();
		if( type == nullptr )
			REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), Synt::GetSrcLoc(*enum_decl.underlying_type_name), type_value.GetKindName() );
		else
		{
			const FundamentalType* const fundamental_type= type->GetFundamentalType();
			if( fundamental_type == nullptr || !IsInteger( fundamental_type->fundamental_type ) )
			{
				// SPRACHE_TODO - maybe allow inheritance of enums?
				REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), enum_decl.src_loc, "any integer type", type );
			}
			else
				enum_->underlying_type= *fundamental_type;
		}
	}

	for( const Synt::Enum::Member& in_member : enum_decl.members )
	{
		if( IsKeyword( in_member.name ) )
			REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), in_member.src_loc );

		const VariableMutPtr var=
			Variable::Create(
				enum_,
				ValueType::ReferenceImut,
				Variable::Location::Pointer );

		var->constexpr_value=
			llvm::Constant::getIntegerValue(
				enum_->underlying_type.llvm_type,
				llvm::APInt( enum_->underlying_type.llvm_type->getIntegerBitWidth(), enum_->element_count ) );
		var->llvm_value=
			CreateGlobalConstantVariable(
				var->type,
				mangler_->MangleGlobalVariable( enum_->members, in_member.name, enum_, true ),
				var->constexpr_value );

		if( enum_->members.AddName( in_member.name, NamesScopeValue( var, in_member.src_loc ) ) == nullptr )
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), in_member.src_loc, in_member.name );

		++enum_->element_count;
	}

	{
		const auto bit_width= enum_->underlying_type.llvm_type->getIntegerBitWidth();
		if( bit_width < 32 ) // Assume that 64 bits are enough for all enums.
		{
			const uint64_t max_value_plus_one=
				uint64_t(1) << ( uint64_t(bit_width) - ( IsSignedInteger( enum_->underlying_type.fundamental_type ) ? 1u : 0u ) );
			const uint64_t max_value= max_value_plus_one - 1u;

			if( enum_->element_count > max_value )
				REPORT_ERROR( UnderlyingTypeForEnumIsTooSmall, names_scope.GetErrors(), enum_decl.src_loc, enum_->element_count - 1u, max_value );
		}
	}

	enum_->syntax_element= nullptr;
}

void CodeBuilder::GlobalThingBuildTypeTemplatesSet( NamesScope& names_scope, TypeTemplatesSet& type_templates_set )
{
	if( !type_templates_set.syntax_elements.empty() )
	{
		DETECT_GLOBALS_LOOP( &type_templates_set, type_templates_set.syntax_elements.front()->name, type_templates_set.syntax_elements.front()->src_loc );

		for( const auto syntax_element : type_templates_set.syntax_elements )
			PrepareTypeTemplate( *syntax_element, type_templates_set, names_scope );

		type_templates_set.syntax_elements.clear();
	}
}

void CodeBuilder::GlobalThingBuildTypeAlias( NamesScope& names_scope, Value& type_alias_value )
{
	U_ASSERT( type_alias_value.GetTypeAlias() != nullptr );
	const Synt::TypeAlias& syntax_element= *type_alias_value.GetTypeAlias()->syntax_element;

	DETECT_GLOBALS_LOOP( &type_alias_value, syntax_element.name, syntax_element.src_loc );

	// Replace value in names map, when type alias is comlete.
	type_alias_value= PrepareType( syntax_element.value, names_scope, *global_function_context_ );
	global_function_context_->args_preevaluation_cache.clear();
}

void CodeBuilder::GlobalThingBuildVariable( NamesScope& names_scope, Value& global_variable_value )
{
	U_ASSERT( global_variable_value.GetIncompleteGlobalVariable() != nullptr );
	const IncompleteGlobalVariable incomplete_global_variable= *global_variable_value.GetIncompleteGlobalVariable();

	SrcLoc src_loc;
	if( incomplete_global_variable.variables_declaration != nullptr )
		src_loc= incomplete_global_variable.variables_declaration->variables[ incomplete_global_variable.element_index ].src_loc;
	else if( incomplete_global_variable.auto_variable_declaration != nullptr )
		src_loc= incomplete_global_variable.auto_variable_declaration->src_loc;

	std::string_view name;
	if( incomplete_global_variable.variables_declaration != nullptr )
		name= incomplete_global_variable.variables_declaration->variables[ incomplete_global_variable.element_index ].name;
	if( incomplete_global_variable.auto_variable_declaration != nullptr )
		name= incomplete_global_variable.auto_variable_declaration->name;

	DETECT_GLOBALS_LOOP( &global_variable_value, std::string(name), src_loc );
	#define FAIL_RETURN { global_variable_value= ErrorValue(); return; }

	FunctionContext& function_context= *global_function_context_;
	const StackVariablesStorage dummy_stack( function_context );

	if( const auto variables_declaration= incomplete_global_variable.variables_declaration )
	{
		const Synt::VariablesDeclaration::VariableEntry& variable_declaration= variables_declaration->variables[ incomplete_global_variable.element_index ];

		const bool is_mutable = variable_declaration.mutability_modifier == MutabilityModifier::Mutable;
		// Disable global mutable references because of problems with initializers and references protection.
		if( is_mutable && variable_declaration.reference_modifier == ReferenceModifier::Reference )
		{
			REPORT_ERROR( MutableGlobalReferencesAreNotAllowed, names_scope.GetErrors(), variable_declaration.src_loc );
			FAIL_RETURN;
		}

		const Type type= PrepareType( variables_declaration->type, names_scope, function_context );
		if( !EnsureTypeComplete( type ) ) // Type completeness required for variable or reference declaration.
		{
			REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), variable_declaration.src_loc, type );
			FAIL_RETURN;
		}

		if( !type.CanBeConstexpr() )
		{
			REPORT_ERROR( InvalidTypeForConstantExpressionVariable, names_scope.GetErrors(), variable_declaration.src_loc );
			FAIL_RETURN;
		}

		// Destruction frame for temporary variables of initializer expression.
		const StackVariablesStorage temp_variables_storage( function_context );

		const VariableMutPtr variable_reference=
			Variable::Create(
				type,
				is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				Variable::Location::Pointer,
				variable_declaration.name );
		function_context.variables_state.AddNode( variable_reference );

		if( variable_declaration.reference_modifier == ReferenceModifier::None )
		{
			const std::string name_mangled = mangler_->MangleGlobalVariable( names_scope, variable_declaration.name, type, !is_mutable );

			llvm::GlobalVariable* const global_variable=
				is_mutable
					? CreateGlobalMutableVariable( type, name_mangled, src_loc )
					: CreateGlobalConstantVariable( type, name_mangled );

			const VariableMutPtr variable=
				Variable::Create(
					type,
					ValueType::Value,
					Variable::Location::Pointer,
					variable_declaration.name,
					global_variable );
			function_context.variables_state.AddNode( variable );

			{
				const VariablePtr variable_for_initialization=
					Variable::Create(
						type,
						ValueType::ReferenceMut,
						Variable::Location::Pointer,
						variable_declaration.name,
						variable->llvm_value );
				function_context.variables_state.AddNode( variable_for_initialization );
				function_context.variables_state.AddLink( variable, variable_for_initialization );
				function_context.variables_state.TryAddInnerLinks( variable, variable_for_initialization, names_scope.GetErrors(), variables_declaration->src_loc );

				if( variable_declaration.initializer != nullptr )
					variable->constexpr_value= ApplyInitializer( variable_for_initialization, names_scope, function_context, *variable_declaration.initializer );
				else
					variable->constexpr_value= ApplyEmptyInitializer( variable_declaration.name, variable_declaration.src_loc, variable_for_initialization, names_scope, function_context );

				function_context.variables_state.RemoveNode( variable_for_initialization );

			}
			function_context.variables_state.RemoveNode( variable );

			if( variable->constexpr_value != nullptr )
				global_variable->setInitializer( variable->constexpr_value );

			variable_reference->llvm_value= variable->llvm_value;
			variable_reference->constexpr_value= variable->constexpr_value;
		}
		else if( variable_declaration.reference_modifier == ReferenceModifier::Reference )
		{
			if( variable_declaration.initializer == nullptr )
			{
				REPORT_ERROR( ExpectedInitializer, names_scope.GetErrors(), variable_declaration.src_loc, variable_declaration.name );
				FAIL_RETURN;
			}

			const Synt::Expression* initializer_expression= nullptr;
			if( const auto expression_initializer= std::get_if<Synt::Expression>( variable_declaration.initializer.get() ) )
				initializer_expression= expression_initializer;
			else if( const auto constructor_initializer= std::get_if<Synt::ConstructorInitializer>( variable_declaration.initializer.get() ) )
			{
				if( constructor_initializer->arguments.size() != 1u )
				{
					REPORT_ERROR( ReferencesHaveConstructorsWithExactlyOneParameter, names_scope.GetErrors(), constructor_initializer->src_loc );
					FAIL_RETURN;
				}
				initializer_expression= &constructor_initializer->arguments.front();
			}
			else
			{
				REPORT_ERROR( UnsupportedInitializerForReference, names_scope.GetErrors(), variable_declaration.src_loc );
				FAIL_RETURN;
			}

			const VariablePtr expression_result= BuildExpressionCodeEnsureVariable( *initializer_expression, names_scope, function_context );

			if( !ReferenceIsConvertible( expression_result->type, variable_reference->type, names_scope.GetErrors(), variable_declaration.src_loc ) )
			{
				REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), variable_declaration.src_loc, variable_reference->type, expression_result->type );
				FAIL_RETURN;
			}
			if( expression_result->value_type == ValueType::Value )
			{
				REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), variable_declaration.src_loc );
				FAIL_RETURN;
			}

			llvm::Value* result_ref= expression_result->llvm_value;
			if( variable_reference->type != expression_result->type )
				result_ref= CreateReferenceCast( result_ref, expression_result->type, variable_reference->type, function_context );
			variable_reference->llvm_value= result_ref;
			variable_reference->constexpr_value= expression_result->constexpr_value;
		}
		else U_ASSERT(false);

		if( variable_reference->constexpr_value == nullptr )
		{
			REPORT_ERROR( VariableInitializerIsNotConstantExpression, names_scope.GetErrors(), variable_declaration.src_loc );
			FAIL_RETURN;
		}

		// Reset constexpr initial value for mutable variables.
		if( is_mutable )
			variable_reference->constexpr_value = nullptr;

		// Do not call destructors, because global variable initializer must be constexpr and any constexpr type has trivial destructor.

		global_variable_value= variable_reference;
	}
	else if( const auto auto_variable_declaration= incomplete_global_variable.auto_variable_declaration )
	{
		const bool is_mutable = auto_variable_declaration->mutability_modifier == MutabilityModifier::Mutable;

		// Disable global mutable references because of problems with initializers and references protection.
		if( is_mutable && auto_variable_declaration->reference_modifier == ReferenceModifier::Reference )
		{
			REPORT_ERROR( MutableGlobalReferencesAreNotAllowed, names_scope.GetErrors(), auto_variable_declaration->src_loc );
			FAIL_RETURN;
		}

		// Destruction frame for temporary variables of initializer expression.
		const StackVariablesStorage temp_variables_storage( function_context );

		const VariablePtr initializer_experrsion= BuildExpressionCodeEnsureVariable( auto_variable_declaration->initializer_expression, names_scope, function_context );
		if( initializer_experrsion->type == invalid_type_ )
		{
			FAIL_RETURN; // Some error was generated before.
		}

		const VariableMutPtr variable_reference=
			Variable::Create(
				initializer_experrsion->type,
				is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				Variable::Location::Pointer,
				auto_variable_declaration->name,
				nullptr,
				initializer_experrsion->constexpr_value );

		if( !EnsureTypeComplete( variable_reference->type ) ) // Type completeness required for variable or reference declaration.
		{
			REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), auto_variable_declaration->src_loc, variable_reference->type );
			FAIL_RETURN;
		}
		if( !variable_reference->type.CanBeConstexpr() )
		{
			REPORT_ERROR( InvalidTypeForConstantExpressionVariable, names_scope.GetErrors(), auto_variable_declaration->src_loc );
			FAIL_RETURN;
		}

		if( auto_variable_declaration->reference_modifier == ReferenceModifier::Reference )
		{
			if( initializer_experrsion->value_type == ValueType::Value )
			{
				REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), auto_variable_declaration->src_loc );
				FAIL_RETURN;
			}

			variable_reference->llvm_value= initializer_experrsion->llvm_value;
		}
		else if( auto_variable_declaration->reference_modifier == ReferenceModifier::None )
		{
			const std::string name_mangled = mangler_->MangleGlobalVariable( names_scope, auto_variable_declaration->name, variable_reference->type, !is_mutable );
			llvm::GlobalVariable* const global_variable=
				is_mutable
					? CreateGlobalMutableVariable( variable_reference->type, name_mangled, src_loc )
					: CreateGlobalConstantVariable( variable_reference->type, name_mangled );

			variable_reference->llvm_value= global_variable;

			if( variable_reference->constexpr_value != nullptr )
				global_variable->setInitializer( variable_reference->constexpr_value );
		}
		else U_ASSERT(false);

		if( variable_reference->constexpr_value == nullptr )
		{
			REPORT_ERROR( VariableInitializerIsNotConstantExpression, names_scope.GetErrors(), auto_variable_declaration->src_loc );
			FAIL_RETURN;
		}

		// Reset constexpr initial value for mutable variables.
		if( is_mutable )
			variable_reference->constexpr_value = nullptr;

		// Do not call destructors, because global variables can be only constexpr and any constexpr type has trivial destructor.

		global_variable_value= variable_reference;
	}
	else U_ASSERT(false);

	global_function_context_->args_preevaluation_cache.clear();

	#undef FAIL_RETURN
}

size_t CodeBuilder::GlobalThingDetectloop( const GlobalThing& global_thing )
{
	for( const GlobalThing& prev_thing : global_things_stack_ )
		if( prev_thing.thing_ptr == global_thing.thing_ptr )
			return size_t( &prev_thing - global_things_stack_.data() );

	return ~0u;
}

void CodeBuilder::GlobalThingReportAboutLoop( const size_t loop_start_stack_index, const std::string_view last_loop_element_name, const SrcLoc& last_loop_element_src_loc )
{
	std::string description;

	SrcLoc min_src_loc= last_loop_element_src_loc;
	for( size_t i= loop_start_stack_index; i < global_things_stack_.size(); ++i )
	{
		min_src_loc= std::min( min_src_loc, global_things_stack_[i].src_loc );
		description+= global_things_stack_[i].name + " -> ";
	}
	description+= last_loop_element_name;

	REPORT_ERROR( GlobalsLoopDetected, *global_errors_, min_src_loc, description );
}

} // namespace U
