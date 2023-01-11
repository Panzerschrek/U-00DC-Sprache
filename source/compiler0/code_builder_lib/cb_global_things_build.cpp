#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

namespace
{

void AddAncestorsAccessRights_r( Class& the_class, const ClassPtr& ancestor_class )
{
	the_class.members->AddAccessRightsFor( ancestor_class, ClassMemberVisibility::Protected );
	for( const Class::Parent& parent : ancestor_class->parents )
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
	GlobalsLoopsDetectorGuard glbals_loop_detector_guard( [this]{ global_things_stack_.pop_back(); } );

void SortClassFields( Class& class_, ClassFieldsVector<llvm::Type*>& fields_llvm_types, const llvm::DataLayout& data_layout )
{
	// Fields in original order
	using FieldsMap= std::map< unsigned int, ClassField* >;
	FieldsMap fields;

	bool fields_is_ok= true;
	class_.members->ForEachValueInThisScope(
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
	if( type.GetFundamentalType() != nullptr )
		return true;
	else if( type.GetFunctionType() != nullptr )
		return false; // Function type is always incomplete.
	else if( type.GetFunctionPointerType() != nullptr )
		return true;
	else if( const auto enum_type= type.GetEnumType() )
		return enum_type->syntax_element == nullptr;
	else if( const auto array_type= type.GetArrayType() )
		return IsTypeComplete( array_type->type );
	else if( type.GetRawPointerType() != nullptr )
		return true; // Pointer is always complete.
	else if( const auto tuple_type= type.GetTupleType() )
	{
		bool all_complete= true;
		for( const Type& element_type : tuple_type->elements )
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
	else if( type.GetFunctionType() != nullptr )
		return false; // Function type is always incomplete.
	else if( type.GetFunctionPointerType() != nullptr )
		return true;
	else if( const auto enum_type= type.GetEnumType() )
	{
		GlobalThingBuildEnum( enum_type );
		return true;
	}
	else if( const auto array_type= type.GetArrayType() )
		return EnsureTypeComplete( array_type->type );
	else if( type.GetRawPointerType() != nullptr )
		return true; // Pointer is always complete.
	else if( const auto tuple_type= type.GetTupleType() )
	{
		bool ok= true;
		for( const Type& element_type : tuple_type->elements )
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
			else if( OverloadedFunctionsSet* const functions_set= value.GetFunctionsSet() )
				GlobalThingBuildFunctionsSet( names_scope, *functions_set, true );
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
					// Otherwise we can get loop, using typedef.
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
		SrcLoc functions_set_src_loc;
		std::string functions_set_name;
		if( !functions_set.syntax_elements.empty() )
		{
			functions_set_src_loc= functions_set.syntax_elements.front()->src_loc_;
			functions_set_name= functions_set.syntax_elements.front()->name_.back();
		}
		else if( !functions_set.template_syntax_elements.empty() )
		{
			functions_set_src_loc= functions_set.template_syntax_elements.front()->src_loc_;
			functions_set_name= functions_set.template_syntax_elements.front()->function_->name_.back();
		}
		DETECT_GLOBALS_LOOP( &functions_set, functions_set_name, functions_set_src_loc );

		for( const Synt::Function* const function : functions_set.syntax_elements )
		{
			const size_t function_index= PrepareFunction( names_scope, functions_set.base_class, functions_set, *function, false );

			if( function_index == ~0u )
				continue;
			FunctionVariable& function_variable= functions_set.functions[function_index];

			// Immediately build functions with auto return type.
			// TODO - this is too complicated. Maybe remove auto-return functions from language?
			if( function_variable.return_type_is_auto && !function_variable.have_body && function->block_ != nullptr )
			{
				// First, compile function only for return type deducing.
				const Type return_type=
					BuildFuncCode(
						function_variable,
						functions_set.base_class,
						names_scope,
						functions_set_name,
						function_variable.syntax_element->type_.params_,
						*function_variable.syntax_element->block_,
						function_variable.syntax_element->constructor_initialization_list_.get() );

				FunctionType function_type= *function_variable.type.GetFunctionType();
				function_type.return_type= return_type;
				function_type.llvm_type= GetLLVMFunctionType( function_type );

				function_variable.have_body= false;
				function_variable.return_type_is_auto= false;
				function_variable.llvm_function->eraseFromParent();
				function_variable.llvm_function=
					llvm::Function::Create(
						function_type.llvm_type,
						llvm::Function::LinkageTypes::ExternalLinkage, // External - for prototype.
						function_variable.no_mangle ? function->name_.back() : mangler_->MangleFunction( names_scope, function->name_.back(), function_type ),
						module_.get() );

				function_variable.type= std::move(function_type);

				SetupFunctionParamsAndRetAttributes( function_variable );

				// Then, compile function again, when type already known.
				BuildFuncCode(
					function_variable,
					functions_set.base_class,
					names_scope,
					functions_set_name,
					function_variable.syntax_element->type_.params_,
					*function_variable.syntax_element->block_,
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
				!function_variable.have_body && !function_variable.return_type_is_auto && !function_variable.is_inherited )
			{
				BuildFuncCode(
					function_variable,
					functions_set.base_class,
					names_scope,
					function_variable.syntax_element->name_.back(),
					function_variable.syntax_element->type_.params_,
					*function_variable.syntax_element->block_,
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
					function_variable.syntax_element->type_.params_,
					*function_variable.syntax_element->block_,
					function_variable.syntax_element->constructor_initialization_list_.get() );
			}
		}
	}
}

void CodeBuilder::GlobalThingPrepareClassParentsList( const ClassPtr class_type )
{
	if( class_type->parents_list_prepared || class_type->syntax_element == nullptr )
		return;

	DETECT_GLOBALS_LOOP( &class_type, class_type->members->GetThisNamespaceName(), class_type->body_src_loc );

	const Synt::Class& class_declaration= *class_type->syntax_element;

	// Perform only steps necessary to build parents list.
	// Do not even require completeness of parent class.
	NamesScope& class_parent_namespace= *class_type->members->GetParent();
	for( const Synt::ComplexName& parent : class_declaration.parents_ )
	{
		const Value parent_value= ResolveValue( class_parent_namespace, *global_function_context_, parent );

		const Type* const type_name= parent_value.GetTypeName();
		if( type_name == nullptr )
		{
			REPORT_ERROR( NameIsNotTypeName, class_parent_namespace.GetErrors(), class_declaration.src_loc_, parent );
			continue;
		}

		const ClassPtr parent_class= type_name->GetClassType();
		if( parent_class == nullptr )
		{
			REPORT_ERROR( CanNotDeriveFromThisType, class_parent_namespace.GetErrors(), class_declaration.src_loc_, type_name );
			continue;
		}

		bool duplicated= false;
		for( const Class::Parent& parent : class_type->parents )
			duplicated= duplicated || parent.class_ == parent_class;
		if( duplicated )
		{
			REPORT_ERROR( DuplicatedParentClass, class_parent_namespace.GetErrors(), class_declaration.src_loc_, type_name );
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

	if( the_class.typeinfo_type != std::nullopt )
	{
		const Type& type= *the_class.typeinfo_type;
		BuildFullTypeinfo( type, typeinfo_cache_[type], *the_class.members->GetRoot() );
		return;
	}

	GlobalThingPrepareClassParentsList( class_type );

	const Synt::Class& class_declaration= *the_class.syntax_element;
	const std::string& class_name= class_declaration.name_;

	DETECT_GLOBALS_LOOP( &the_class, the_class.members->GetThisNamespaceName(), the_class.body_src_loc );

	NamesScope& class_parent_namespace= *the_class.members->GetParent();
	// Perform remaining check of parents.
	for( Class::Parent& parent : the_class.parents )
	{
		if( !EnsureTypeComplete( parent.class_ ) )
		{
			REPORT_ERROR( UsingIncompleteType, class_parent_namespace.GetErrors(), class_declaration.src_loc_, parent.class_ );
			return;
		}

		const auto parent_kind= parent.class_->kind;
		if( !( parent_kind == Class::Kind::Abstract || parent_kind == Class::Kind::Interface || parent_kind == Class::Kind::PolymorphNonFinal ) )
		{
			REPORT_ERROR( CanNotDeriveFromThisType, class_parent_namespace.GetErrors(), class_declaration.src_loc_, parent.class_ );
			return;
		}

		if( parent_kind != Class::Kind::Interface ) // not interface=base
		{
			if( the_class.base_class != nullptr )
			{
				REPORT_ERROR( DuplicatedBaseClass, class_parent_namespace.GetErrors(), class_declaration.src_loc_, parent.class_ );
				return;
			}
			the_class.base_class= parent.class_;
		}
	}

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

	the_class.members->ForEachValueInThisScope(
		[&]( Value& value )
		{
			ClassField* const class_field= value.GetClassField();
			if( class_field == nullptr )
				return;

			const Synt::ClassField& in_field= *class_field->syntax_element;

			class_field->class_= class_type;
			class_field->is_reference= in_field.reference_modifier == Synt::ReferenceModifier::Reference;
			class_field->type= PrepareType( class_field->syntax_element->type, *the_class.members, *global_function_context_ );

			if( !class_field->is_reference || in_field.mutability_modifier == Synt::MutabilityModifier::Constexpr )
			{
				// Full type completeness required for value-fields and constexpr reference-fields.
				if( !EnsureTypeComplete( class_field->type ) )
				{
					REPORT_ERROR( UsingIncompleteType, class_parent_namespace.GetErrors(), in_field.src_loc_, class_field->type );
					return;
				}
			}

			if( class_field->is_reference )
			{
				if( !EnsureTypeComplete( class_field->type ) )
				{
					REPORT_ERROR( UsingIncompleteType, class_parent_namespace.GetErrors(), in_field.src_loc_, class_field->type );
					return;
				}
				if( class_field->type.ReferencesTagsCount() > 0u )
					REPORT_ERROR( ReferenceFieldOfTypeWithReferencesInside, class_parent_namespace.GetErrors(), in_field.src_loc_, in_field.name );
			}
			else if( class_field->type.IsAbstract() )
				REPORT_ERROR( ConstructingAbstractClassOrInterface, class_parent_namespace.GetErrors(), in_field.src_loc_, class_field->type );

			if( class_field->is_reference ) // Reference-fields are immutable by default
				class_field->is_mutable= in_field.mutability_modifier == Synt::MutabilityModifier::Mutable;
			else // But value-fields are mutable by default
				class_field->is_mutable= in_field.mutability_modifier != Synt::MutabilityModifier::Immutable;

			// Disable constexpr, if field can not be constexpr, or if field is mutable reference.
			if( !class_field->type.CanBeConstexpr() || ( class_field->is_reference && class_field->is_mutable ) )
				the_class.can_be_constexpr= false;

			++the_class.field_count;
		} );

	// Determine inner reference type.
	the_class.members->ForEachValueInThisScope(
		[&]( const Value& value )
		{
			const ClassField* const field= value.GetClassField();
			if( field == nullptr )
				return;

			if( field->is_reference )
				the_class.inner_reference_type= std::max( the_class.inner_reference_type, field->is_mutable ? InnerReferenceType::Mut : InnerReferenceType::Imut );
			else
			{
				if( !EnsureTypeComplete( field->type ) )
					REPORT_ERROR( UsingIncompleteType, class_parent_namespace.GetErrors(), field->syntax_element->src_loc_, field->type );
				the_class.inner_reference_type= std::max( the_class.inner_reference_type, field->type.GetInnerReferenceType() );
			}

		});

	for( const Class::Parent& parent : the_class.parents )
		the_class.inner_reference_type= std::max( the_class.inner_reference_type, parent.class_->inner_reference_type );

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

		parent.field_number= static_cast<unsigned int>(fields_llvm_types.size());
		fields_llvm_types.emplace_back( parent.class_->llvm_type );
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

		the_class.members->ForEachValueInThisScope(
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
					fields_llvm_types.emplace_back( fundamental_llvm_types_.i8_ );// May be in case of error (such dependency loop )
				else
					fields_llvm_types.emplace_back( class_field->type.GetLLVMType() );
			}
		}

		if( !class_declaration.keep_fields_order_ )
			SortClassFields( the_class, fields_llvm_types, data_layout_ );
	}

	// Fill container with fields names.
	the_class.fields_order.resize( fields_llvm_types.size() );
	the_class.members->ForEachInThisScope(
		[&]( const std::string& name, const Value& value )
		{
			if( const auto field= value.GetClassField() )
				the_class.fields_order[field->index]= name;
		} );

	// Complete another body elements.
	// For class completeness we needs only fields, functions. Constants, types are not needed.
	the_class.members->ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( const auto functions_set= value.GetFunctionsSet() )
				GlobalThingBuildFunctionsSet( *the_class.members, *functions_set, false );
			else if( value.GetClassField() != nullptr ) {} // Fields are already complete.
			else if( value.GetTypeName() != nullptr ) {}
			else if( value.GetVariable() != nullptr ){}
			else if( value.GetErrorValue() != nullptr ){}
			else if( value.GetStaticAssert() != nullptr ){}
			else if( value.GetTypedef() != nullptr ) {}
			else if( const auto type_templates_set= value.GetTypeTemplatesSet() )
				GlobalThingBuildTypeTemplatesSet( *the_class.members, *type_templates_set );
			else if( value.GetIncompleteGlobalVariable() != nullptr ) {}
			else if( value.GetNamespace() != nullptr ) {} // Can be in case of type template parameters namespace.
			else U_ASSERT(false);
		});

	// Generate destructor prototype before perparing virtual table to mark it as virtual and setup virtual table index.
	if( the_class.members->GetThisScopeValue( Keyword( Keywords::destructor_ ) ) == nullptr )
	{
		FunctionVariable destructor_function_variable= GenerateDestructorPrototype( class_type );
		OverloadedFunctionsSet destructors_set;
		destructors_set.functions.push_back( std::move(destructor_function_variable) );
		the_class.members->AddName( Keyword( Keywords::destructor_ ), std::move(destructors_set) );
	}

	if( the_class.kind == Class::Kind::Interface ||
		the_class.kind == Class::Kind::Abstract ||
		the_class.kind == Class::Kind::PolymorphNonFinal ||
		the_class.kind == Class::Kind::PolymorphFinal )
		PrepareClassVirtualTable( the_class );

	// Search for explicit noncopy constructors.
	if( const Value* const constructors_value=
		the_class.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) ) )
	{
		const OverloadedFunctionsSet* const constructors= constructors_value->GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		for( const FunctionVariable& constructor : constructors->functions )
		{
			if( !IsCopyConstructor( *constructor.type.GetFunctionType(), class_type ) )
			{
				the_class.have_explicit_noncopy_constructors= true;
				break;
			}
		};
		the_class.have_explicit_noncopy_constructors |= !constructors->template_functions.empty();
	}

	// Disable constexpr possibility for structs with:
	// * explicit destructors
	// * non-default copy-assignment operators
	// * non-default copy constructors
	// * non-default equality compare operators
	if( const Value* const destructor_value=
		the_class.members->GetThisScopeValue( Keyword( Keywords::destructor_ ) ) )
	{
		const OverloadedFunctionsSet* const destructors= destructor_value->GetFunctionsSet();
		// Destructors may be invalid in case of error.
		if( !destructors->functions.empty() && !destructors->functions[0].is_generated )
			the_class.can_be_constexpr= false;
	}
	if( const Value* const constructor_value=
		the_class.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) ) )
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
		the_class.members->GetThisScopeValue( OverloadedOperatorToString( OverloadedOperator::Assign ) ) )
	{
		const OverloadedFunctionsSet* const operators= assignment_operator_value->GetFunctionsSet();
		U_ASSERT( operators != nullptr );
		for( const FunctionVariable& op : operators->functions )
		{
			if( IsCopyAssignmentOperator( *op.type.GetFunctionType(), class_type ) && !op.is_generated )
				the_class.can_be_constexpr= false;
		}
	}
	if( const Value* const compare_equal_value=
		the_class.members->GetThisScopeValue( OverloadedOperatorToString( OverloadedOperator::CompareEqual ) ) )
	{
		const OverloadedFunctionsSet* const operators= compare_equal_value->GetFunctionsSet();
		U_ASSERT( operators != nullptr );
		for( const FunctionVariable& op : operators->functions )
		{
			if( IsEqualityCompareOperator( *op.type.GetFunctionType(), class_type ) && !op.is_generated )
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
			REPORT_ERROR( ClassContainsPureVirtualFunctions, the_class.members->GetErrors(), class_declaration.src_loc_, class_name );
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
			REPORT_ERROR( ClassContainsPureVirtualFunctions, the_class.members->GetErrors(), class_declaration.src_loc_, class_name );
			the_class.kind= Class::Kind::Abstract;
		}
		break;

	case Synt::ClassKindAttribute::Polymorph:
		the_class.kind= Class::Kind::PolymorphNonFinal;
		if( class_contains_pure_virtual_functions )
		{
			REPORT_ERROR( ClassContainsPureVirtualFunctions, the_class.members->GetErrors(), class_declaration.src_loc_, class_name );
			the_class.kind= Class::Kind::Abstract;
		}
		break;

	case Synt::ClassKindAttribute::Interface:
		if( the_class.field_count != 0u )
			REPORT_ERROR( FieldsForInterfacesNotAllowed, the_class.members->GetErrors(), class_declaration.src_loc_ );
		if( the_class.base_class != nullptr )
			REPORT_ERROR( BaseClassForInterface, the_class.members->GetErrors(), class_declaration.src_loc_ );
		if( the_class.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) ) != nullptr )
			REPORT_ERROR( ConstructorForInterface, the_class.members->GetErrors(), class_declaration.src_loc_ );
		for( const Class::VirtualTableEntry& virtual_table_entry : the_class.virtual_table )
		{
			if( !virtual_table_entry.is_pure && virtual_table_entry.name != Keywords::destructor_ )
			{
				REPORT_ERROR( NonPureVirtualFunctionInInterface, the_class.members->GetErrors(), class_declaration.src_loc_, class_name );
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
			[&]( const std::string& name, const Value& value )
			{
				const auto parent_member_visibility= parent_class->GetMemberVisibility( name );
				if( parent_member_visibility == ClassMemberVisibility::Private )
					return; // Do not inherit private members.

				Value* const result_class_value= the_class.members->GetThisScopeValue(name);

				if( const auto functions= value.GetFunctionsSet() )
				{
					if( name == Keyword( Keywords::constructor_ ) ||
						name == Keyword( Keywords::destructor_ ) ||
						name == OverloadedOperatorToString( OverloadedOperator::Assign ) ||
						name == OverloadedOperatorToString( OverloadedOperator::CompareEqual ) ||
						name == OverloadedOperatorToString( OverloadedOperator::CompareOrder ) )
						return; // Do not inherit constructors, destructors, assignment operators, compare operators.

					if( result_class_value != nullptr )
					{
						if( const auto result_class_functions= result_class_value->GetFunctionsSet() )
						{
							if( the_class.GetMemberVisibility( name ) != parent_class->GetMemberVisibility( name ) )
							{
								const auto& src_loc= result_class_functions->functions.empty() ? result_class_functions->template_functions.front()->src_loc : result_class_functions->functions.front().prototype_src_loc;
								REPORT_ERROR( FunctionsVisibilityMismatch, the_class.members->GetErrors(), src_loc, name );
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
								{
									if( ApplyOverloadedFunction( *result_class_functions, parent_function, the_class.members->GetErrors(), class_declaration.src_loc_ ) )
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
						OverloadedFunctionsSet functions_set= *functions;
						for( FunctionVariable& function : functions_set.functions )
							function.is_inherited= true;

						the_class.members->AddName( name, std::move(functions_set) );
						the_class.SetMemberVisibility( name, parent_member_visibility );
					}
				}
				if( const auto type_templates_set= value.GetTypeTemplatesSet() )
				{
					if( result_class_value != nullptr )
					{
						if( const auto result_type_templates_set= result_class_value->GetTypeTemplatesSet() )
						{
							if( the_class.GetMemberVisibility( name ) != parent_class->GetMemberVisibility( name ) )
								REPORT_ERROR( TypeTemplatesVisibilityMismatch, the_class.members->GetErrors(), result_class_value->GetSrcLoc(), name );

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

	BuildPolymorphClassTypeId( the_class, class_type );
	BuildClassVirtualTable( the_class, class_type );

	the_class.is_complete= true;

	TryGenerateDefaultConstructor( class_type );
	TryGenerateDestructor( class_type );
	TryGenerateCopyConstructor( class_type );
	TryGenerateCopyAssignmentOperator( class_type );
	TryGenerateEqualityCompareOperator( class_type );

	CheckClassFieldsInitializers( class_type );

	// Immediately build constexpr functions.
	the_class.members->ForEachInThisScope(
		[&]( const std::string& name, Value& value )
		{
			OverloadedFunctionsSet* const functions_set= value.GetFunctionsSet();
			if( functions_set == nullptr )
				return;

			for( FunctionVariable& function : functions_set->functions )
			{
				if( function.constexpr_kind != FunctionVariable::ConstexprKind::NonConstexpr &&
					!function.have_body && function.syntax_element != nullptr && function.syntax_element->block_ != nullptr )
					BuildFuncCode(
						function,
						class_type,
						*the_class.members,
						name,
						function.syntax_element->type_.params_,
						*function.syntax_element->block_,
						function.syntax_element->constructor_initialization_list_.get() );
			}
		}); // for functions
}

void CodeBuilder::GlobalThingBuildEnum( const EnumPtr enum_ )
{
	if( enum_->syntax_element == nullptr )
		return;

	DETECT_GLOBALS_LOOP( enum_, enum_->members.GetThisNamespaceName(), enum_->syntax_element->src_loc_ );

	// Default underlaying type is 32bit. TODO - maybe do it platform-dependent?
	enum_->underlaying_type= FundamentalType( U_FundamentalType::u32_, fundamental_llvm_types_.u32_ );

	const Synt::Enum& enum_decl= *enum_->syntax_element;
	NamesScope& names_scope= *enum_->members.GetParent();

	if( !( std::get_if<Synt::EmptyVariant>( &enum_decl.underlaying_type_name.start_value ) != nullptr && enum_decl.underlaying_type_name.tail == nullptr ) )
	{
		const Value type_value= ResolveValue( names_scope, *global_function_context_, enum_decl.underlaying_type_name );
		const Type* const type= type_value.GetTypeName();
		if( type == nullptr )
			REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), enum_decl.src_loc_, enum_decl.underlaying_type_name );
		else
		{
			const FundamentalType* const fundamental_type= type->GetFundamentalType();
			if( fundamental_type == nullptr || !IsInteger( fundamental_type->fundamental_type ) )
			{
				// SPRACHE_TODO - maybe allow inheritance of enums?
				REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), enum_decl.src_loc_, "any integer type", type );
			}
			else
				enum_->underlaying_type= *fundamental_type;
		}
	}

	for( const Synt::Enum::Member& in_member : enum_decl.members )
	{
		if( IsKeyword( in_member.name ) )
			REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), in_member.src_loc );

		Variable var;

		var.type= enum_;
		var.location= Variable::Location::Pointer;
		var.value_type= ValueType::ReferenceImut;
		var.constexpr_value=
			llvm::Constant::getIntegerValue(
				enum_->underlaying_type.llvm_type,
				llvm::APInt( enum_->underlaying_type.llvm_type->getIntegerBitWidth(), enum_->element_count ) );
		var.llvm_value=
			CreateGlobalConstantVariable(
				var.type,
				mangler_->MangleGlobalVariable( enum_->members, in_member.name, enum_, true ),
				var.constexpr_value );

		if( enum_->members.AddName( in_member.name, Value( var, in_member.src_loc ) ) == nullptr )
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), in_member.src_loc, in_member.name );

		++enum_->element_count;
	}

	{
		const auto bit_width= enum_->underlaying_type.llvm_type->getIntegerBitWidth();
		if( bit_width < 32 ) // Assume that 64 bits are enough for all enums.
		{
			const uint64_t max_value_plus_one=
				uint64_t(1) << ( uint64_t(bit_width) - ( IsSignedInteger( enum_->underlaying_type.fundamental_type ) ? 1u : 0u ) );
			const uint64_t max_value= max_value_plus_one - 1u;

			if( enum_->element_count > max_value )
				REPORT_ERROR( UnderlayingTypeForEnumIsTooSmall, names_scope.GetErrors(), enum_decl.src_loc_, enum_->element_count - 1u, max_value );
		}
	}

	enum_->syntax_element= nullptr;
}

void CodeBuilder::GlobalThingBuildTypeTemplatesSet( NamesScope& names_scope, TypeTemplatesSet& type_templates_set )
{
	if( !type_templates_set.syntax_elements.empty() )
	{
		DETECT_GLOBALS_LOOP( &type_templates_set, type_templates_set.syntax_elements.front()->name_, type_templates_set.syntax_elements.front()->src_loc_ );

		for( const auto syntax_element : type_templates_set.syntax_elements )
			PrepareTypeTemplate( *syntax_element, type_templates_set, names_scope );

		type_templates_set.syntax_elements.clear();
	}
}

void CodeBuilder::GlobalThingBuildTypedef( NamesScope& names_scope, Value& type_alias_value )
{
	U_ASSERT( type_alias_value.GetTypedef() != nullptr );
	const Synt::TypeAlias& syntax_element= *type_alias_value.GetTypedef()->syntax_element;

	DETECT_GLOBALS_LOOP( &type_alias_value, syntax_element.name, syntax_element.src_loc_ );

	// Replace value in names map, when typedef is comlete.
	type_alias_value= Value( PrepareType( syntax_element.value, names_scope, *global_function_context_ ), syntax_element.src_loc_ );
}

void CodeBuilder::GlobalThingBuildVariable( NamesScope& names_scope, Value& global_variable_value )
{
	U_ASSERT( global_variable_value.GetIncompleteGlobalVariable() != nullptr );
	const IncompleteGlobalVariable incomplete_global_variable= *global_variable_value.GetIncompleteGlobalVariable();

	SrcLoc src_loc;
	if( incomplete_global_variable.variables_declaration != nullptr )
		src_loc= incomplete_global_variable.variables_declaration->variables[ incomplete_global_variable.element_index ].src_loc;
	else if( incomplete_global_variable.auto_variable_declaration != nullptr )
		src_loc= incomplete_global_variable.auto_variable_declaration->src_loc_;

	DETECT_GLOBALS_LOOP( &global_variable_value, incomplete_global_variable.name, src_loc );
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

		const Type type= PrepareType( variables_declaration->type, names_scope, *global_function_context_ );
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

		Variable variable;
		variable.type= type;
		variable.location= Variable::Location::Pointer;
		variable.value_type= ValueType::ReferenceMut;

		if( variable_declaration.reference_modifier == ReferenceModifier::None )
		{
			const std::string name_mangled = mangler_->MangleGlobalVariable( names_scope, variable_declaration.name, type, !is_mutable );

			llvm::GlobalVariable* global_variable= nullptr;
			variable.llvm_value= global_variable=
				is_mutable
					? CreateGlobalMutableVariable( type, name_mangled )
					: CreateGlobalConstantVariable( type, name_mangled );

			if( variable_declaration.initializer != nullptr )
				variable.constexpr_value= ApplyInitializer( variable, names_scope, function_context, *variable_declaration.initializer );
			else
				variable.constexpr_value= ApplyEmptyInitializer( variable_declaration.name, variable_declaration.src_loc, variable, names_scope, function_context );

			// Make immutable, if needed, only after initialization, because in initialization we need call constructors, which is mutable methods.
			if( !is_mutable )
				variable.value_type= ValueType::ReferenceImut;

			if( global_variable != nullptr && variable.constexpr_value != nullptr )
				global_variable->setInitializer( variable.constexpr_value );
		}
		else if( variable_declaration.reference_modifier == ReferenceModifier::Reference )
		{
			if( variable_declaration.initializer == nullptr )
			{
				REPORT_ERROR( ExpectedInitializer, names_scope.GetErrors(), variable_declaration.src_loc, variable_declaration.name );
				FAIL_RETURN;
			}

			variable.value_type= is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;

			const Synt::Expression* initializer_expression= nullptr;
			if( const auto expression_initializer= std::get_if<Synt::Expression>( variable_declaration.initializer.get() ) )
				initializer_expression= expression_initializer;
			else if( const auto constructor_initializer= std::get_if<Synt::ConstructorInitializer>( variable_declaration.initializer.get() ) )
			{
				if( constructor_initializer->arguments.size() != 1u )
				{
					REPORT_ERROR( ReferencesHaveConstructorsWithExactlyOneParameter, names_scope.GetErrors(), constructor_initializer->src_loc_ );
					FAIL_RETURN;
				}
				initializer_expression= &constructor_initializer->arguments.front();
			}
			else
			{
				REPORT_ERROR( UnsupportedInitializerForReference, names_scope.GetErrors(), variable_declaration.src_loc );
				FAIL_RETURN;
			}

			const Variable expression_result= BuildExpressionCodeEnsureVariable( *initializer_expression, names_scope, function_context );

			if( !ReferenceIsConvertible( expression_result.type, variable.type, names_scope.GetErrors(), variable_declaration.src_loc ) )
			{
				REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), variable_declaration.src_loc, variable.type, expression_result.type );
				FAIL_RETURN;
			}
			if( expression_result.value_type == ValueType::Value )
			{
				REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), variable_declaration.src_loc );
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
			REPORT_ERROR( VariableInitializerIsNotConstantExpression, names_scope.GetErrors(), variable_declaration.src_loc );
			FAIL_RETURN;
		}

		// Reset constexpr initial value for mutable variables.
		if( is_mutable )
			variable.constexpr_value = nullptr;

		// Do not call destructors, because global variable initializer must be constexpr and any constexpr type have trivial destructor.

		global_variable_value= Value( variable, variable_declaration.src_loc );
	}
	else if( const auto auto_variable_declaration= incomplete_global_variable.auto_variable_declaration )
	{
		const bool is_mutable = auto_variable_declaration->mutability_modifier == MutabilityModifier::Mutable;

		// Disable global mutable references because of problems with initializers and references protection.
		if( is_mutable && auto_variable_declaration->reference_modifier == ReferenceModifier::Reference )
		{
			REPORT_ERROR( MutableGlobalReferencesAreNotAllowed, names_scope.GetErrors(), auto_variable_declaration->src_loc_ );
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
				initializer_experrsion.type.GetRawPointerType() != nullptr ||
				initializer_experrsion.type.GetFunctionPointerType() != nullptr;
			if( !type_is_ok || initializer_experrsion.type == invalid_type_ )
			{
				REPORT_ERROR( InvalidTypeForAutoVariable, names_scope.GetErrors(), auto_variable_declaration->src_loc_, initializer_experrsion.type );
				FAIL_RETURN;
			}
		}

		Variable variable;
		variable.type= initializer_experrsion.type;
		variable.value_type= is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;
		variable.location= Variable::Location::Pointer;

		if( !EnsureTypeComplete( variable.type ) ) // Type completeness required for variable or reference declaration.
		{
			REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), auto_variable_declaration->src_loc_, variable.type );
			FAIL_RETURN;
		}
		if( !variable.type.CanBeConstexpr() )
		{
			REPORT_ERROR( InvalidTypeForConstantExpressionVariable, names_scope.GetErrors(), auto_variable_declaration->src_loc_ );
			FAIL_RETURN;
		}

		if( auto_variable_declaration->reference_modifier == ReferenceModifier::Reference )
		{
			if( initializer_experrsion.value_type == ValueType::Value )
			{
				REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), auto_variable_declaration->src_loc_ );
				FAIL_RETURN;
			}

			variable.llvm_value= initializer_experrsion.llvm_value;
			variable.constexpr_value= initializer_experrsion.constexpr_value;
		}
		else if( auto_variable_declaration->reference_modifier == ReferenceModifier::None )
		{
			const std::string name_mangled = mangler_->MangleGlobalVariable( names_scope, auto_variable_declaration->name, variable.type, !is_mutable );
			llvm::GlobalVariable* const global_variable=
				is_mutable
					? CreateGlobalMutableVariable( variable.type, name_mangled )
					: CreateGlobalConstantVariable( variable.type, name_mangled );

			variable.llvm_value= global_variable;
			// Copy constructor for constexpr type is trivial, so, we can just take constexpr value of source.
			variable.constexpr_value= initializer_experrsion.constexpr_value;

			if( variable.constexpr_value != nullptr )
				global_variable->setInitializer( variable.constexpr_value );
		}
		else U_ASSERT(false);

		if( variable.constexpr_value == nullptr )
		{
			REPORT_ERROR( VariableInitializerIsNotConstantExpression, names_scope.GetErrors(), auto_variable_declaration->src_loc_ );
			FAIL_RETURN;
		}

		// Reset constexpr initial value for mutable variables.
		if( is_mutable )
			variable.constexpr_value = nullptr;

		// Do not call destructors, because global variables can be only constexpr and any constexpr type have trivial destructor.

		global_variable_value= Value( variable, auto_variable_declaration->src_loc_ );
	}
	else U_ASSERT(false);

	#undef FAIL_RETURN
}

size_t CodeBuilder::GlobalThingDetectloop( const GlobalThing& global_thing )
{
	for( const GlobalThing& prev_thing : global_things_stack_ )
		if( prev_thing.thing_ptr == global_thing.thing_ptr )
			return size_t( &prev_thing - global_things_stack_.data() );

	return ~0u;
}

void CodeBuilder::GlobalThingReportAboutLoop( const size_t loop_start_stack_index, const std::string& last_loop_element_name, const SrcLoc& last_loop_element_src_loc )
{
	std::string description;

	SrcLoc min_src_loc= last_loop_element_src_loc;
	for( size_t i= loop_start_stack_index; i < global_things_stack_.size(); ++i )
	{
		min_src_loc= std::min( min_src_loc, global_things_stack_[i].src_loc );
		description+= global_things_stack_[i].name + " -> ";
	}
	description+= last_loop_element_name;

	REPORT_ERROR( GlobalsLoopDetected, global_errors_, min_src_loc, description );
}

} // namespace U
