#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

void CodeBuilder::PrepareClassVirtualTable( Class& the_class )
{
	U_ASSERT( !the_class.is_complete );
	U_ASSERT( the_class.virtual_table.empty() );

	// First, borrow virtual table of parent with 0 offset.
	// Class reuses virtual table pointer of first parent, so, virtual table layout must be equal.
	for( const Class::Parent& parent : the_class.parents )
		if( parent.field_number == 0u )
		{
			the_class.virtual_table= parent.class_->virtual_table;

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

		for( const Class::VirtualTableEntry& parent_vtable_entry : parent.class_->virtual_table )
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
				vtable_entry_copy.index_in_table= uint32_t( &parent_vtable_entry - parent.class_->virtual_table.data() );
				vtable_entry_copy.parent_virtual_table_index= uint32_t( &parent - the_class.parents.data() );
				the_class.virtual_table.push_back( std::move(vtable_entry_copy) );
			}
		} // for parent virtual table
	}

	struct ClassFunction
	{
		FunctionVariable* function;
		std::string name;
		bool operator<( const ClassFunction& other ) const{ return this->function->llvm_function->name_mangled < other.function->llvm_function->name_mangled; }
	};
	std::vector<ClassFunction> class_functions;

	the_class.members->ForEachInThisScope(
		[&]( const std::string_view name, NamesScopeValue& value )
		{
			if( const auto functions_set= value.value.GetFunctionsSet() )
				for( FunctionVariable& function : functions_set->functions )
					class_functions.emplace_back( ClassFunction{ &function, std::string(name) } );
		});

	// We needs strong order of functions in virtual table. So, sort them, using mangled name.
	std::sort( class_functions.begin(), class_functions.end() );

	uint32_t own_virtual_table_index= 0;

	// Process functions
	for( const ClassFunction& class_function : class_functions )
	{
		FunctionVariable& function= *class_function.function;

		const std::string& function_name= class_function.name;
		const SrcLoc& src_loc= function.syntax_element->src_loc;
		CodeBuilderErrorsContainer& errors_container= the_class.members->GetErrors();

		if( function.virtual_function_kind != Synt::VirtualFunctionKind::None &&
			the_class.GetMemberVisibility( function_name ) == ClassMemberVisibility::Private )
		{
			// Private members not visible in child classes. So, virtual private function is 100% error.
			REPORT_ERROR( VirtualForPrivateFunction, errors_container, src_loc, function_name );
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
		uint32_t virtual_table_index= ~0u;
		if( virtual_table_entry != nullptr )
			virtual_table_index= uint32_t(virtual_table_entry - the_class.virtual_table.data());

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
					function.virtual_table_index= uint32_t(the_class.virtual_table.size());

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
				REPORT_ERROR( VirtualRequired, errors_container, src_loc, function_name );
			break;

		case Synt::VirtualFunctionKind::DeclareVirtual:
			if( virtual_table_entry != nullptr )
				REPORT_ERROR( OverrideRequired, errors_container, src_loc, function_name );
			else
			{
				function.virtual_table_index= uint32_t(the_class.virtual_table.size());

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
				REPORT_ERROR( FunctionDoesNotOverride, errors_container, src_loc, function_name );
			else if( virtual_table_entry->is_final )
				REPORT_ERROR( OverrideFinalFunction, errors_container, src_loc, function_name );
			else
			{
				function.virtual_table_index= virtual_table_index;
				virtual_table_entry->function_variable= function;
				virtual_table_entry->is_pure= false;
			}
			break;

		case Synt::VirtualFunctionKind::VirtualFinal:
			if( virtual_table_entry == nullptr )
				REPORT_ERROR( FinalForFirstVirtualFunction, errors_container, src_loc, function_name );
			else
			{
				if( virtual_table_entry->is_final )
					REPORT_ERROR( OverrideFinalFunction, errors_container, src_loc, function_name );
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
				REPORT_ERROR( OverrideRequired, errors_container, src_loc, function_name );
			else
			{
				if( function.syntax_element->block != nullptr )
					REPORT_ERROR( BodyForPureVirtualFunction, errors_container, src_loc, function_name );
				if( function_name == Keyword( Keywords::destructor_ ) )
					REPORT_ERROR( PureDestructor, errors_container, src_loc, the_class.members->GetThisNamespaceName() );
				function.has_body= true; // Mark pure function as "with body", because we needs to disable real body creation for pure function.

				function.virtual_table_index= uint32_t(the_class.virtual_table.size());

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
}

void CodeBuilder::PrepareClassVirtualTableType( const ClassPtr class_type )
{
	/*
	Virtual table layout for polymorph class without parens:

		size_type base_offset;
		type_id_table_element* type_id; // pointer to first element of null-terminated table.
		[ fn(), N ] functions_table;

	Virtual table layout for polymorph class with parents:

		virtual_table_type0 paren0_table; // 100% table of parent with offset 0 (base class for non-interfaces)
		virtual_table_type1 paren1_table;
		...
		virtual_table_typeN parenN_table;
		[ fn(), N ] functions_table;


	Own class functions table contains only newly added virtaul functions.
	If class inherits or even overrides virtual function, it uses functions table of some parent for such function.
	*/

	Class& the_class= *class_type;
	U_ASSERT( !the_class.is_complete );
	U_ASSERT( the_class.virtual_table_llvm_type == nullptr );

	if( the_class.virtual_table.empty() )
		return; // Non-polymorph class.

	llvm::SmallVector<llvm::Type*, 16> virtual_table_struct_fields;

	// First, add first class virtual table, then, virtual tables of other parent classes.
	for( const Class::Parent& parent : the_class.parents )
		if( parent.field_number == 0u )
			virtual_table_struct_fields.push_back( parent.class_->virtual_table_llvm_type );

	for( const Class::Parent& parent : the_class.parents )
	{
		if( parent.field_number != 0u )
			virtual_table_struct_fields.push_back( parent.class_->virtual_table_llvm_type );
	}

	if( virtual_table_struct_fields.empty() )
	{
		// No parents - create special fields - base offset, type id, etc.
		virtual_table_struct_fields.push_back( fundamental_llvm_types_.size_type_ ); // Offset field. Always use "size_type" here.
		virtual_table_struct_fields.push_back( polymorph_type_id_table_element_type_->getPointerTo() ); // type_id field
	}

	uint32_t own_functions_count= 0u;
	for( const Class::VirtualTableEntry& entry : the_class.virtual_table )
		if( entry.parent_virtual_table_index == ~0u )
			++own_functions_count;

	const auto own_virtual_functions_table_type= llvm::ArrayType::get( virtual_function_pointer_type_, own_functions_count );

	virtual_table_struct_fields.push_back( own_virtual_functions_table_type );

	// TODO - maybe create unnamed struct (like a tuple)?
	the_class.virtual_table_llvm_type= llvm::StructType::create( virtual_table_struct_fields, "_vtable_type_" + mangler_->MangleType(class_type) );
}

void CodeBuilder::BuildPolymorphClassTypeId( const ClassPtr class_type )
{
	Class& the_class= *class_type;

	U_ASSERT( the_class.polymorph_type_id_table == nullptr );

	// Build polymorph type id only for polymorph classes.
	if( !(
		the_class.kind == Class::Kind::Interface ||
		the_class.kind == Class::Kind::Abstract ||
		the_class.kind == Class::Kind::PolymorphNonFinal ||
		the_class.kind == Class::Kind::PolymorphFinal ) )
		return;

	const auto class_data_layout= data_layout_.getStructLayout( the_class.llvm_type );
	llvm::SmallVector<llvm::Constant*, 5> table_initializers;

	for( const Class::Parent& parent : the_class.parents )
	{
		llvm::Value* const gep_indices[]{ GetZeroGEPIndex(), GetZeroGEPIndex() };
		const auto first_element_address=
			llvm::ConstantExpr::getGetElementPtr(
				parent.class_->polymorph_type_id_table->getInitializer()->getType(),
				parent.class_->polymorph_type_id_table,
				gep_indices );

		const auto initializer=
		llvm::ConstantStruct::get(
			polymorph_type_id_table_element_type_,
			{
				llvm::ConstantInt::get(
					fundamental_llvm_types_.size_type_,
					class_data_layout->getElementOffset( parent.field_number ) ),
				first_element_address,
			} );

		if( parent.field_number == 0 )
			table_initializers.insert( table_initializers.begin(), initializer ); // Put base class initializer first.
		else
			table_initializers.push_back( initializer );
	}

	// Put null terminator.
	// We need this to ensure non-zero result size and give a way to iterate over this table in run-time.
	table_initializers.push_back( llvm::Constant::getNullValue( polymorph_type_id_table_element_type_ ) );

	const auto type_id_table_type= llvm::ArrayType::get( polymorph_type_id_table_element_type_, table_initializers.size() );

	the_class.polymorph_type_id_table=
		new llvm::GlobalVariable(
			*module_,
			type_id_table_type,
			true, // is_constant
			llvm::GlobalValue::ExternalLinkage,
			llvm::ConstantArray::get( type_id_table_type, table_initializers ),
			"_type_id_for_" + mangler_->MangleType( class_type ) );

	if( !IsSrcLocFromMainFile( class_type->syntax_element->src_loc ) )
	{
		// This is a class, declared in imported file.
		// Create comdat in order to ensure uniquiness of the table across different modules, that import file with declaration of this class.
		llvm::Comdat* const type_id_comdat= module_->getOrInsertComdat( the_class.polymorph_type_id_table->getName() );
		type_id_comdat->setSelectionKind( llvm::Comdat::Any );
		the_class.polymorph_type_id_table->setComdat( type_id_comdat );
	}
	else
	{
		// This class is declared in main file.
		// It should not be externally available, thus there is no need to use external linkage or comdat.
		the_class.polymorph_type_id_table->setLinkage( llvm::GlobalValue::PrivateLinkage );
	}
}

llvm::Constant* CodeBuilder::BuildClassVirtualTable_r( const Class& ancestor_class, const Class& dst_class, const uint64_t offset )
{
	const auto class_data_layout= data_layout_.getStructLayout( ancestor_class.llvm_type );

	llvm::SmallVector<llvm::Constant*, 5> initializer_values;
	for( const Class::Parent& parent : ancestor_class.parents )
	{
		const uint64_t parent_offset= offset + class_data_layout->getElementOffset( parent.field_number );
		const auto initializer= BuildClassVirtualTable_r( *parent.class_, dst_class, parent_offset );
		if( parent.field_number == 0 )
			initializer_values.insert( initializer_values.begin(), initializer );
		else
			initializer_values.push_back( initializer );
	}

	if( initializer_values.empty() )
	{
		// offset
		initializer_values.push_back( llvm::ConstantInt::get( fundamental_llvm_types_.size_type_, offset ) );

		// Type id
		// Take address of first element of type id table.
		llvm::Value* const gep_indices[]{ GetZeroGEPIndex(), GetZeroGEPIndex() };
		const auto address=
			llvm::ConstantExpr::getGetElementPtr(
				dst_class.polymorph_type_id_table->getInitializer()->getType(),
				dst_class.polymorph_type_id_table,
				gep_indices );
		initializer_values.push_back( address );
	}

	const auto array_type= llvm::dyn_cast<llvm::ArrayType>( ancestor_class.virtual_table_llvm_type->getElementType( uint32_t(initializer_values.size() ) ) );

	llvm::SmallVector<llvm::Constant*, 16> function_pointers_initializer_values;
	for( const Class::VirtualTableEntry& ancestor_virtual_table_entry : ancestor_class.virtual_table )
	{
		if( ancestor_virtual_table_entry.parent_virtual_table_index != ~0u )
			continue;

		llvm::Function* func= EnsureLLVMFunctionCreated( ancestor_virtual_table_entry.function_variable );
		for( const Class::VirtualTableEntry& dst_virtual_table_entry : dst_class.virtual_table )
		{
			if( dst_virtual_table_entry.name == ancestor_virtual_table_entry.name &&
				dst_virtual_table_entry.function_variable.VirtuallyEquals( ancestor_virtual_table_entry.function_variable ) )
			{
				func= EnsureLLVMFunctionCreated( dst_virtual_table_entry.function_variable );
				break;
			}
		}

		function_pointers_initializer_values.push_back( func );
	}

	initializer_values.push_back( llvm::ConstantArray::get( array_type, function_pointers_initializer_values ) );

	return llvm::ConstantStruct::get( ancestor_class.virtual_table_llvm_type, initializer_values );
}

void CodeBuilder::BuildClassVirtualTable( const ClassPtr class_type )
{
	Class& the_class= *class_type;

	U_ASSERT( !the_class.is_complete );
	U_ASSERT( the_class.virtual_table_llvm_variable == nullptr );

	// Build virtual table only for polymorph non-abstract classes.
	if( !(
		the_class.kind == Class::Kind::PolymorphNonFinal ||
		the_class.kind == Class::Kind::PolymorphFinal ) )
		return;

	U_ASSERT( the_class.virtual_table_llvm_type != nullptr );

	auto virtual_table_initializer= BuildClassVirtualTable_r( the_class, the_class, 0u );

	the_class.virtual_table_llvm_variable=
		new llvm::GlobalVariable(
			*module_,
			the_class.virtual_table_llvm_type,
			true, // is constant
			llvm::GlobalValue::PrivateLinkage,
			virtual_table_initializer,
			mangler_->MangleVirtualTable(class_type) );
	the_class.virtual_table_llvm_variable->setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );
}

std::pair<VariablePtr, llvm::Value*> CodeBuilder::TryFetchVirtualFunction(
	const VariablePtr& this_,
	const FunctionVariable& function,
	FunctionContext& function_context,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	if( function.virtual_table_index == ~0u )
		return std::make_pair( this_, EnsureLLVMFunctionCreated( function ) ); // No need to perform virtual call.

	if( function_context.is_functionless_context )
		return std::make_pair( this_, nullptr );

	const Type& function_this_type= function.type.params.front().type;

	if( !ReferenceIsConvertible( this_->type, function_this_type, errors_container, src_loc ) )
	{
		// This normally should not happen, if reference compatibility is checked during overloaded function selecting.
		// If not - error will be generated during call itself
		return std::make_pair( this_, EnsureLLVMFunctionCreated( function ) );
	}

	// Cast "this" into type of class, where this virtual function is declared.
	// This is needed to perform (possible) pointer correction later.
	const VariableMutPtr this_casted=
		Variable::Create(
			function_this_type,
			this_->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut,
			Variable::Location::Pointer,
			"casted " + this_->name,
			CreateReferenceCast( this_->llvm_value, this_->type, function_this_type, function_context ) );
	function_context.variables_state.AddNode( this_casted );
	function_context.variables_state.TryAddLink( this_, this_casted, errors_container, src_loc );
	function_context.variables_state.TryAddInnerLinks( this_, this_casted, errors_container, src_loc );

	RegisterTemporaryVariable( function_context, this_casted );

	const Class& class_type= *this_casted->type.GetClassType();
	U_ASSERT( function.virtual_table_index < class_type.virtual_table.size() );

	// Fetch vtable pointer.
	// Virtual table pointer is always first field.
	llvm::LoadInst* const virtual_table_ptr=
		function_context.llvm_ir_builder.CreateLoad(
			class_type.virtual_table_llvm_type->getPointerTo(),
			this_casted->llvm_value );
	virtual_table_ptr->setMetadata( llvm::LLVMContext::MD_nonnull, llvm::MDNode::get( llvm_context_, llvm::None ) ); // Virtual table pointer is never null.
	if( generate_tbaa_metadata_ )
		virtual_table_ptr->setMetadata( llvm::LLVMContext::MD_tbaa, tbaa_metadata_builder_.CreateVirtualTablePointerAccessTag() );

	const uint32_t c_offset_field_number= 0u;
	[[maybe_unused]] const uint32_t c_type_id_field_number= 1u;
	const uint32_t c_funcs_table_field_number= 2u; // Only for class with no parents.

	// Select virtual subtable of class, where function declared first time.
	llvm::Value* function_virtual_table= virtual_table_ptr;
	const Class::VirtualTableEntry* virtual_table_entry= &class_type.virtual_table[ function.virtual_table_index ];
	const Class* function_class= &class_type;
	while( virtual_table_entry->parent_virtual_table_index != ~0u )
	{
		U_ASSERT( virtual_table_entry->parent_virtual_table_index < function_class->parents.size() );
		uint32_t field_index= ~0u;
		if( function_class->parents[ virtual_table_entry->parent_virtual_table_index ].field_number == 0u )
			field_index= 0u;
		else
		{
			field_index= 1u;
			for( size_t i= 0; i < virtual_table_entry->parent_virtual_table_index; ++i )
				if( function_class->parents[i].field_number != 0 )
					++field_index;
		}

		function_virtual_table=
			CreateCompositeElementGEP(
				function_context,
				function_class->virtual_table_llvm_type,
				function_virtual_table,
				GetFieldGEPIndex( field_index ) );

		const auto next_class= function_class->parents[ virtual_table_entry->parent_virtual_table_index ].class_;
		virtual_table_entry= &next_class->virtual_table[ virtual_table_entry->index_in_table ];
		function_class= next_class;
	}
	const uint32_t functions_table_field_number= function_class->parents.empty() ? c_funcs_table_field_number : uint32_t(class_type.parents.size());

	// Get functions table pointer from whole virtual table pointer, than get address of function in this table.
	llvm::Value* const ptr_to_function_ptr=
		function_context.llvm_ir_builder.CreateInBoundsGEP(
			function_class->virtual_table_llvm_type,
			function_virtual_table,
			{
				GetZeroGEPIndex(),
				GetFieldGEPIndex( functions_table_field_number ),
				GetFieldGEPIndex( virtual_table_entry->index_in_table )
			} );

	llvm::LoadInst* const function_ptr= function_context.llvm_ir_builder.CreateLoad( virtual_function_pointer_type_, ptr_to_function_ptr );
	function_ptr->setMetadata( llvm::LLVMContext::MD_nonnull, llvm::MDNode::get( llvm_context_, llvm::None ) ); // Function address in virtual table is never null.
	if( generate_tbaa_metadata_ )
		function_ptr->setMetadata( llvm::LLVMContext::MD_tbaa, tbaa_metadata_builder_.CreateVirtualTableFunctionPointerAccessTag() );

	// Correct "this" pointer.
	// Only interfaces may have non-zero offsets. So, make pointer adjustment only for call via interface.
	if( class_type.kind == Class::Kind::Interface )
	{
		// Fetch "this" pointer offset. It is located in deepest first virtual table (actual offset should always be zero).

		const Class* root_class= &class_type;
		while( root_class->base_class != nullptr )
			root_class= root_class->base_class;

		llvm::Value* const offset_ptr=
			CreateCompositeElementGEP(
				function_context,
				root_class->virtual_table_llvm_type,
				virtual_table_ptr, // Reinterpretate this pointer as pointer to root table.
				GetFieldGEPIndex(c_offset_field_number) );
		llvm::Value* const offset= CreateTypedLoad( function_context, size_type_, offset_ptr );

		llvm::Value* const this_ptr_as_int= function_context.llvm_ir_builder.CreatePtrToInt( this_casted->llvm_value, fundamental_llvm_types_.size_type_ );
		llvm::Value* this_sub_offset= function_context.llvm_ir_builder.CreateSub( this_ptr_as_int, offset );
		this_casted->llvm_value= function_context.llvm_ir_builder.CreateIntToPtr( this_sub_offset, this_casted->type.GetLLVMType()->getPointerTo() );
	}
	return std::make_pair( std::move(this_casted), function_ptr );
}

void CodeBuilder::SetupVirtualTablePointers_r(
	llvm::Value* const this_,
	llvm::Value* const ptr_to_vtable_ptr,
	const Class& the_class,
	FunctionContext& function_context )
{
	// Setup virtual table pointers for parents.
	uint32_t vtable_field_number= 1;
	for( const Class::Parent& parent : the_class.parents )
	{
		llvm::Value* const parent_ptr= CreateClassFieldGEP( function_context, the_class, this_, parent.field_number );
		llvm::Value* const vtable_ptr=
			CreateCompositeElementGEP(
				function_context,
				the_class.virtual_table_llvm_type,
				ptr_to_vtable_ptr,
				GetFieldGEPIndex( parent.field_number == 0 ? 0 : vtable_field_number ) );

		SetupVirtualTablePointers_r( parent_ptr, vtable_ptr, *parent.class_, function_context );

		if( parent.field_number != 0 )
			++vtable_field_number;
	}

	// Store virtual table pointer only if it allocated in current class.
	if( the_class.parents.empty() )
	{
		llvm::Value* const vtable_ptr=
			CreateCompositeElementGEP(
				function_context,
				the_class.llvm_type,
				this_,
				GetZeroGEPIndex() /* virtual table pointer is allways first field */ );
		llvm::StoreInst* const store= function_context.llvm_ir_builder.CreateStore( ptr_to_vtable_ptr, vtable_ptr );
		if( generate_tbaa_metadata_ )
			store->setMetadata( llvm::LLVMContext::MD_tbaa, tbaa_metadata_builder_.CreateVirtualTablePointerAccessTag() );
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
		U_ASSERT( the_class.virtual_table_llvm_variable == nullptr );
		return;
	}

	if( the_class.kind == Class::Kind::Interface || the_class.kind == Class::Kind::Abstract )
		return; // Such kinds of classes have no virtual tables. SPRACHE_TODO - maybe generate for such classes some virtual tables?

	if( the_class.virtual_table_llvm_variable == nullptr )
		return; // May be in case of errors.

	SetupVirtualTablePointers_r( this_, the_class.virtual_table_llvm_variable, the_class, function_context );
}

} // namespace U
