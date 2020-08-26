#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "mangling.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

void CodeBuilder::PrepareClassVirtualTable( Class& the_class, const Type& class_type, const std::vector<FunctionVariable*>& functions )
{
	U_ASSERT( the_class.completeness != TypeCompleteness::Complete );
	U_ASSERT( the_class.virtual_table.empty() );

	// First, borrow virtual table of parent with 0 offset.
	// Class reuses virtual table pointer of first parent, so, virtual table layout must be equal.
	for( const Class::Parent& parent : the_class.parents )
		if( parent.field_number == 0u )
		{
			the_class.virtual_table= parent.class_->class_->virtual_table;

			for( Class::VirtualTableEntry& entry : the_class.virtual_table )
			{
				entry.index_in_table= uint32_t( &entry - the_class.virtual_table.data() );
				entry.parent_virtual_table_index= uint32_t( &parent - the_class.parents.data() );
			}

			break;
		}

	// Then, add virtual functions from other parents.
	for( const Class::Parent& parent : the_class.parents )
	{
		if( parent.field_number == 0u )
			continue;

		for( const Class::VirtualTableEntry& parent_vtable_entry : parent.class_->class_->virtual_table )
		{
			bool already_exists_in_vtable= false;
			for( const Class::VirtualTableEntry& this_class_vtable_entry : the_class.virtual_table )
			{
				if( this_class_vtable_entry.name == parent_vtable_entry.name &&
					this_class_vtable_entry.function_variable.VirtuallyEquals( parent_vtable_entry.function_variable ) )
				{
					already_exists_in_vtable= true;
					break;
				}
			}

			if( !already_exists_in_vtable )
			{
				Class::VirtualTableEntry vtable_entry_copy= parent_vtable_entry;
				vtable_entry_copy.index_in_table= uint32_t( &parent_vtable_entry - parent.class_->class_->virtual_table.data() );
				vtable_entry_copy.parent_virtual_table_index= uint32_t( &parent - the_class.parents.data() );
				the_class.virtual_table.push_back( std::move(vtable_entry_copy) );
			}
		} // for parent virtual table
	}

	uint32_t own_virtual_table_index= 0;

	// Process functions
	for( FunctionVariable* const function_ptr : functions )
	{
		FunctionVariable& function= *function_ptr;

		const std::string& function_name= function.syntax_element->name_.back();
		const FilePos& file_pos= function.syntax_element->file_pos_;
		CodeBuilderErrorsContainer& errors_container= the_class.members.GetErrors();

		if( function.virtual_function_kind != Synt::VirtualFunctionKind::None &&
			the_class.GetMemberVisibility( function_name ) == ClassMemberVisibility::Private )
		{
			// Private members not visible in child classes. So, virtual private function is 100% error.
			REPORT_ERROR( VirtualForPrivateFunction, errors_container, file_pos, function_name );
		}

		if( !function.is_this_call )
			continue; // May be in case of error

		Class::VirtualTableEntry* virtual_table_entry= nullptr;
		for( Class::VirtualTableEntry& e : the_class.virtual_table )
		{
			if( e.name == function_name && e.function_variable.VirtuallyEquals( function ) )
			{
				virtual_table_entry= &e;
				break;
			}
		}
		unsigned int virtual_table_index= ~0u;
		if( virtual_table_entry != nullptr )
			virtual_table_index= static_cast<unsigned int>(virtual_table_entry - the_class.virtual_table.data());

		switch( function.virtual_function_kind )
		{
		case Synt::VirtualFunctionKind::None:
			if( function_name == Keywords::destructor_ )
			{
				// For destructors virtual specifiers are optional.
				// If destructor not marked as virtual, but it placed in polymorph class, make it virtual.
				if( virtual_table_entry != nullptr )
				{
					function.virtual_table_index= virtual_table_index;
					virtual_table_entry->function_variable= function;
				}
				else if( the_class.kind == Class::Kind::PolymorphFinal || the_class.kind == Class::Kind::PolymorphNonFinal ||
						 the_class.kind == Class::Kind::Interface || the_class.kind == Class::Kind::Abstract )
				{
					function.virtual_table_index= static_cast<unsigned int>(the_class.virtual_table.size());

					Class::VirtualTableEntry new_virtual_table_entry;
					new_virtual_table_entry.name= function_name;
					new_virtual_table_entry.function_variable= function;
					new_virtual_table_entry.is_pure= false;
					new_virtual_table_entry.is_final= false;
					new_virtual_table_entry.index_in_table= own_virtual_table_index;
					++own_virtual_table_index;
					the_class.virtual_table.push_back( std::move( new_virtual_table_entry ) );
				}
			}
			else if( virtual_table_entry != nullptr )
				REPORT_ERROR( VirtualRequired, errors_container, file_pos, function_name );
			break;

		case Synt::VirtualFunctionKind::DeclareVirtual:
			if( virtual_table_entry != nullptr )
				REPORT_ERROR( OverrideRequired, errors_container, file_pos, function_name );
			else
			{
				function.virtual_table_index= static_cast<unsigned int>(the_class.virtual_table.size());

				Class::VirtualTableEntry new_virtual_table_entry;
				new_virtual_table_entry.name= function_name;
				new_virtual_table_entry.function_variable= function;
				new_virtual_table_entry.is_pure= false;
				new_virtual_table_entry.is_final= false;
				new_virtual_table_entry.index_in_table= own_virtual_table_index;
				++own_virtual_table_index;
				the_class.virtual_table.push_back( std::move( new_virtual_table_entry ) );
			}
			break;

		case Synt::VirtualFunctionKind::VirtualOverride:
			if( virtual_table_entry == nullptr )
				REPORT_ERROR( FunctionDoesNotOverride, errors_container, file_pos, function_name );
			else if( virtual_table_entry->is_final )
				REPORT_ERROR( OverrideFinalFunction, errors_container, file_pos, function_name );
			else
			{
				function.virtual_table_index= virtual_table_index;
				virtual_table_entry->function_variable= function;
				virtual_table_entry->is_pure= false;
			}
			break;

		case Synt::VirtualFunctionKind::VirtualFinal:
			if( virtual_table_entry == nullptr )
				REPORT_ERROR( FinalForFirstVirtualFunction, errors_container, file_pos, function_name );
			else
			{
				if( virtual_table_entry->is_final )
					REPORT_ERROR( OverrideFinalFunction, errors_container, file_pos, function_name );
				else
				{
					function.virtual_table_index= virtual_table_index;
					virtual_table_entry->function_variable= function;
					virtual_table_entry->is_pure= false;
					virtual_table_entry->is_final= true;
				}
			}
			break;

		case Synt::VirtualFunctionKind::VirtualPure:
			if( virtual_table_entry != nullptr )
				REPORT_ERROR( OverrideRequired, errors_container, file_pos, function_name );
			else
			{
				if( function.syntax_element->block_ != nullptr )
					REPORT_ERROR( BodyForPureVirtualFunction, errors_container, file_pos, function_name );
				if( function_name == Keyword( Keywords::destructor_ ) )
					REPORT_ERROR( PureDestructor, errors_container, file_pos, the_class.members.GetThisNamespaceName() );
				function.have_body= true; // Mark pure function as "with body", because we needs to disable real body creation for pure function.

				function.virtual_table_index= static_cast<unsigned int>(the_class.virtual_table.size());

				Class::VirtualTableEntry new_virtual_table_entry;
				new_virtual_table_entry.name= function_name;
				new_virtual_table_entry.function_variable= function;
				new_virtual_table_entry.is_pure= true;
				new_virtual_table_entry.is_final= false;
				new_virtual_table_entry.index_in_table= own_virtual_table_index;
				++own_virtual_table_index;
				the_class.virtual_table.push_back( std::move( new_virtual_table_entry ) );
			}
			break;
		};
	}

	// Generate destructor prototype.
	{
		if( the_class.members.GetThisScopeValue( Keyword( Keywords::destructor_ ) ) != nullptr )
			return;

		// Generate destructor prototype.
		FunctionVariable destructor_function_variable= GenerateDestructorPrototype( the_class, class_type );
		destructor_function_variable.prototype_file_pos= destructor_function_variable.body_file_pos= FilePos(); // TODO - set correct file_pos

		// Add destructor to virtual table.
		Class::VirtualTableEntry* virtual_table_entry= nullptr;
		for( Class::VirtualTableEntry& e : the_class.virtual_table )
		{
			if( e.name == Keywords::destructor_ )
			{
				virtual_table_entry= &e;
				break;
			}
		}
		if( virtual_table_entry == nullptr )
		{
			destructor_function_variable.virtual_table_index= static_cast<unsigned int>(the_class.virtual_table.size());
			Class::VirtualTableEntry new_virtual_table_entry;
			new_virtual_table_entry.function_variable= destructor_function_variable;
			new_virtual_table_entry.name= Keyword( Keywords::destructor_ );
			new_virtual_table_entry.is_pure= false;
			new_virtual_table_entry.is_final= false;
			new_virtual_table_entry.index_in_table= own_virtual_table_index;
			++own_virtual_table_index;
			the_class.virtual_table.push_back( std::move( new_virtual_table_entry ) );
		}
		else
			virtual_table_entry->function_variable= destructor_function_variable;

		// Add destructor to names scope.
		OverloadedFunctionsSet destructors_set;
		destructors_set.functions.push_back(destructor_function_variable);
		the_class.members.AddName( Keyword( Keywords::destructor_ ), destructors_set );
	}
}

void CodeBuilder::PrepareClassVirtualTableType( const ClassProxyPtr& class_type )
{
	Class& the_class= *class_type->class_;
	U_ASSERT( the_class.completeness != TypeCompleteness::Complete );
	U_ASSERT( the_class.virtual_table_llvm_type == nullptr );

	if( the_class.virtual_table.empty() )
		return; // Non-polymorph class.

	std::vector<llvm::Type*> virtual_table_struct_fields;

	// First, add first class virtual table, then, virtual tables of other parent classes.
	for( const Class::Parent& parent : the_class.parents )
		if( parent.field_number == 0u )
			virtual_table_struct_fields.push_back( parent.class_->class_->virtual_table_llvm_type );

	for( const Class::Parent& parent : the_class.parents )
	{
		if( parent.field_number != 0u )
			virtual_table_struct_fields.push_back( parent.class_->class_->virtual_table_llvm_type );
	}

	if( virtual_table_struct_fields.empty() )
	{
		// No parents - create special fields - base offset, type id, etc.
		virtual_table_struct_fields.push_back( fundamental_llvm_types_.int_ptr ); // Offset field.
		virtual_table_struct_fields.push_back( fundamental_llvm_types_.int_ptr->getPointerTo() ); // type_id field
	}

	const auto fn_type= llvm::FunctionType::get( fundamental_llvm_types_.void_for_ret, true );
	const auto fn_ptr_type= llvm::PointerType::get( fn_type, 0u );

	uint32_t own_functions_count= 0u;
	for( const Class::VirtualTableEntry& entry : the_class.virtual_table )
		if( entry.parent_virtual_table_index == ~0u )
			++own_functions_count;

	const auto own_virtual_functions_table_type= llvm::ArrayType::get( fn_ptr_type, own_functions_count );

	virtual_table_struct_fields.push_back( own_virtual_functions_table_type );

	// TODO - maybe create unnamed struct (like a tuple)?
	the_class.virtual_table_llvm_type= llvm::StructType::create( virtual_table_struct_fields, "_vtable_type_" + MangleType(class_type) );
}

llvm::Constant* CodeBuilder::BuildClassVirtualTable_r(
	const Class& ancestor_class,
	const Class& dst_class,
	llvm::Value* const dst_class_ptr_null_based )
{
	std::vector<llvm::Constant*> initializer_values;
	for( const Class::Parent& parent : ancestor_class.parents )
	{
		if( parent.field_number == 0u )
		{
			llvm::Value* index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex( 0 ) };
			llvm::Value* const offset_ptr= global_function_context_->llvm_ir_builder.CreateGEP( dst_class_ptr_null_based, index_list );
			initializer_values.push_back( BuildClassVirtualTable_r( *parent.class_->class_, dst_class, offset_ptr ) );
		}
	}

	for( const Class::Parent& parent : ancestor_class.parents )
	{
		if( parent.field_number != 0u )
		{
			llvm::Value* index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex( parent.field_number ) };
			llvm::Value* const offset_ptr= global_function_context_->llvm_ir_builder.CreateGEP( dst_class_ptr_null_based, index_list );
			initializer_values.push_back( BuildClassVirtualTable_r( *parent.class_->class_, dst_class, offset_ptr ) );
		}
	}

	if( initializer_values.empty() )
	{
		// offset
		initializer_values.push_back(
				llvm::dyn_cast<llvm::Constant>(
					global_function_context_->llvm_ir_builder.CreatePtrToInt( dst_class_ptr_null_based, fundamental_llvm_types_.int_ptr ) ) );

		// Type id
		initializer_values.push_back( dst_class.polymorph_type_id );
	}

	const auto array_type= llvm::dyn_cast<llvm::ArrayType>( ancestor_class.virtual_table_llvm_type->getElementType( uint32_t(initializer_values.size() ) ) );
	const auto fn_type_ptr= array_type->getElementType();

	std::vector<llvm::Constant*> function_pointers_initializer_values;
	for( const Class::VirtualTableEntry& ancestor_virtual_table_entry : ancestor_class.virtual_table )
	{
		if( ancestor_virtual_table_entry.parent_virtual_table_index != ~0u )
			continue;

		llvm::Function* func= ancestor_virtual_table_entry.function_variable.llvm_function;
		for( const Class::VirtualTableEntry& dst_virtual_table_entry : dst_class.virtual_table )
		{
			if( dst_virtual_table_entry.name == ancestor_virtual_table_entry.name &&
				dst_virtual_table_entry.function_variable.VirtuallyEquals( ancestor_virtual_table_entry.function_variable ) )
			{
				func= dst_virtual_table_entry.function_variable.llvm_function;
				break;
			}
		}

		llvm::Value* const function_pointer_casted=
			global_function_context_->llvm_ir_builder.CreateBitOrPointerCast( func, fn_type_ptr );

		function_pointers_initializer_values.push_back( llvm::dyn_cast<llvm::Constant>(function_pointer_casted) );
	}

	initializer_values.push_back( llvm::ConstantArray::get( array_type, function_pointers_initializer_values ) );

	return llvm::ConstantStruct::get( ancestor_class.virtual_table_llvm_type, initializer_values );
}

void CodeBuilder::BuildClassVirtualTable( Class& the_class, const Type& class_type )
{
	U_ASSERT( the_class.completeness != TypeCompleteness::Complete );
	U_ASSERT( the_class.this_class_virtual_table == nullptr );

	if( the_class.virtual_table.empty() )
		return; // Non-polymorph class.

	U_ASSERT( the_class.virtual_table_llvm_type != nullptr );

	// Type id
	llvm::Type* const type_id_type= fundamental_llvm_types_.int_ptr;
	the_class.polymorph_type_id=
		new llvm::GlobalVariable(
			*module_,
			type_id_type,
			true, // is_constant
			llvm::GlobalValue::ExternalLinkage,
			llvm::Constant::getNullValue( type_id_type ),
			"_type_id_for_" + MangleType( class_type ) );
	llvm::Comdat* const type_id_comdat= module_->getOrInsertComdat( the_class.polymorph_type_id->getName() );
	type_id_comdat->setSelectionKind( llvm::Comdat::Any );
	the_class.polymorph_type_id->setComdat( type_id_comdat );

	llvm::Value* const this_nullptr= llvm::Constant::getNullValue( the_class.llvm_type->getPointerTo() );
	auto virtual_table_initializer= BuildClassVirtualTable_r( the_class, the_class, this_nullptr );

	the_class.this_class_virtual_table=
		new llvm::GlobalVariable(
			*module_,
			the_class.virtual_table_llvm_type,
			true, // is constant
			llvm::GlobalValue::InternalLinkage,
			virtual_table_initializer,
			"_vtable_main_" + MangleType(class_type) );
	the_class.this_class_virtual_table->setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );
}

std::pair<Variable, llvm::Value*> CodeBuilder::TryFetchVirtualFunction(
	const Variable& this_,
	const FunctionVariable& function,
	FunctionContext& function_context,
	CodeBuilderErrorsContainer& errors_container,
	const FilePos& file_pos )
{
	const Function& function_type= *function.type.GetFunctionType();

	if( !ReferenceIsConvertible( this_.type, function_type.args.front().type, errors_container, file_pos ) )
		return std::make_pair( this_, function.llvm_function );

	Variable this_casted;
	this_casted= this_;
	if( this_.type != function_type.args.front().type )
	{
		this_casted.type= function_type.args.front().type;
		this_casted.llvm_value= CreateReferenceCast( this_.llvm_value, this_.type, this_casted.type, function_context );
	}

	llvm::Value* llvm_function_ptr= function.llvm_function;
	if( function.virtual_table_index == ~0u )
		return std::make_pair( std::move(this_casted), llvm_function_ptr );

	const Class& class_type= *this_casted.type.GetClassType();
	U_ASSERT( function.virtual_table_index < class_type.virtual_table.size() );

	// Fetch vtable pointer.
	// Virtual table pointer is always first field.
	llvm::Value* const ptr_to_virtual_table_ptr= function_context.llvm_ir_builder.CreatePointerCast( this_casted.llvm_value, class_type.virtual_table_llvm_type->getPointerTo()->getPointerTo() );
	llvm::Value* const virtual_table_ptr= function_context.llvm_ir_builder.CreateLoad( ptr_to_virtual_table_ptr, "virtual_table_ptr" );

	// Fetch "this" pointer offset. It is located in deepest first virtual table (actual offset should always be zero).
	llvm::Value* first_root_virtual_table= virtual_table_ptr;
	for( const Class* current_class= &class_type; !current_class->parents.empty(); )
	{
		llvm::Value* const index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex( 0 ) };
		first_root_virtual_table= function_context.llvm_ir_builder.CreateGEP( first_root_virtual_table, index_list );

		for( const Class::Parent& parent : current_class->parents )
			if( parent.field_number == 0 )
			{
				current_class= parent.class_->class_;
				break;
			}
	}

	const unsigned int c_offset_field_number= 0u;
	const unsigned int c_type_id_field_number= 1u;
	const unsigned int c_funcs_table_field_number= 2u; // Only for class with no parents.

	llvm::Value* const offset_index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex( c_offset_field_number ) };
	llvm::Value* const offset_ptr= function_context.llvm_ir_builder.CreateGEP( first_root_virtual_table, offset_index_list );
	llvm::Value* const offset= function_context.llvm_ir_builder.CreateLoad( offset_ptr, "offset" );

	// Select virtual subtable of class, where function declared first time.
	llvm::Value* function_virtual_table= virtual_table_ptr;
	uint32_t functions_table_field_number= ~0u;
	const Class::VirtualTableEntry* virtual_table_entry= &class_type.virtual_table[ function.virtual_table_index ];
	{
		const Class* current_class= &class_type;
		while( virtual_table_entry->parent_virtual_table_index != ~0u )
		{
			uint32_t field_index= ~0u;
			if( current_class->parents[ virtual_table_entry->parent_virtual_table_index ].field_number == 0u )
				field_index= 0u;
			else
			{
				field_index= 1u;
				for( size_t i= 0; i < virtual_table_entry->parent_virtual_table_index; ++i )
				{
					if( current_class->parents[ virtual_table_entry->parent_virtual_table_index ].field_number != 0 )
						++field_index;
				}
			}

			llvm::Value* const index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex( field_index ) };
			function_virtual_table= function_context.llvm_ir_builder.CreateGEP( function_virtual_table, index_list );

			auto next_class= current_class->parents[ virtual_table_entry->parent_virtual_table_index ].class_->class_;
			virtual_table_entry= &current_class->parents[ virtual_table_entry->parent_virtual_table_index ].class_->class_->virtual_table[ virtual_table_entry->index_in_table ];
			current_class= next_class;
		}
		functions_table_field_number= current_class->parents.empty() ? c_funcs_table_field_number : uint32_t(class_type.parents.size());
	}

	llvm::Value* const function_table_index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex( functions_table_field_number ) };
	llvm::Value* const functions_table_ptr= function_context.llvm_ir_builder.CreateGEP( function_virtual_table, function_table_index_list, "function_table_ptr" );

	// Fetch function.
	llvm::Value* const function_pointer_index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex( virtual_table_entry->index_in_table ) };
	llvm::Value* const ptr_to_function_ptr= function_context.llvm_ir_builder.CreateGEP( functions_table_ptr, function_pointer_index_list, "ptr_to_function_ptr" );
	llvm_function_ptr= function_context.llvm_ir_builder.CreateLoad( ptr_to_function_ptr );
	llvm_function_ptr= function_context.llvm_ir_builder.CreateBitOrPointerCast( llvm_function_ptr, function_type.llvm_function_type->getPointerTo(), "function_ptr_casted" );

	// Correct "this" pointer.
	llvm::Value* const this_ptr_as_int= function_context.llvm_ir_builder.CreatePtrToInt( this_casted.llvm_value, fundamental_llvm_types_.int_ptr, "this_ptr_as_int" );
	llvm::Value* this_sub_offset= function_context.llvm_ir_builder.CreateSub( this_ptr_as_int, offset, "this_sub_offset" );
	this_casted.llvm_value= function_context.llvm_ir_builder.CreateIntToPtr( this_sub_offset, this_casted.type.GetLLVMType()->getPointerTo(), "this_casted" );

	return std::make_pair( std::move(this_casted), llvm_function_ptr );
}

void CodeBuilder::SetupVirtualTablePointers_r(
	llvm::Value* this_,
	llvm::Value* ptr_to_vtable_ptr,
	const Class& the_class,
	FunctionContext& function_context )
{
	// Store virtual table pointer for current class (including base).
	function_context.llvm_ir_builder.CreateStore(
		ptr_to_vtable_ptr,
		function_context.llvm_ir_builder.CreatePointerCast( this_, the_class.virtual_table_llvm_type->getPointerTo()->getPointerTo() ) );

	if( the_class.parents.size() <= 1u )
		return;

	// Setup virtual table pointers for non-base parent classes.
	unsigned int vtable_field_number= 1;
	for( const Class::Parent& parent : the_class.parents )
	{
		if( parent.field_number == 0 )
			continue;

		llvm::Value* const this_index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex( parent.field_number ) };
		llvm::Value* const parent_ptr= function_context.llvm_ir_builder.CreateGEP( this_, this_index_list );

		llvm::Value* const virtual_table_index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex( vtable_field_number ) };
		llvm::Value* const vtable_ptr= function_context.llvm_ir_builder.CreateGEP( ptr_to_vtable_ptr, virtual_table_index_list );

		SetupVirtualTablePointers_r( parent_ptr, vtable_ptr, *parent.class_->class_, function_context );

		++vtable_field_number;
	}
}

void CodeBuilder::SetupVirtualTablePointers(
	llvm::Value* this_,
	const Class& the_class,
	FunctionContext& function_context )
{
	if( the_class.virtual_table.empty() )
	{
		U_ASSERT( the_class.virtual_table_llvm_type == nullptr );
		U_ASSERT( the_class.this_class_virtual_table == nullptr );
		return;
	}

	if( the_class.kind == Class::Kind::Interface || the_class.kind == Class::Kind::Abstract )
		return; // Such kinds of classes have no virtual tables. SPRACHE_TODO - maybe generate for such classes some virtual tables?

	if( the_class.this_class_virtual_table == nullptr )
		return; // May be in case of errors.

	SetupVirtualTablePointers_r( this_, the_class.this_class_virtual_table, the_class, function_context );
}

} // namespace CodeBuilderPrivate

} // namespace U
