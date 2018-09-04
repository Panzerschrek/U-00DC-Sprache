#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "mangling.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

void CodeBuilder::ProcessClassParentsVirtualTables( Class& the_class )
{
	U_ASSERT( the_class.completeness != TypeCompleteness::Complete );
	U_ASSERT( the_class.virtual_table.empty() );

	// Copy virtual table of base class.
	if( the_class.base_class != nullptr )
		the_class.virtual_table= the_class.base_class->class_->virtual_table;

	// Later, process interfaces.
	for( const ClassProxyPtr& parent : the_class.parents )
	{
		if( parent == the_class.base_class )
			continue;

		for( const Class::VirtualTableEntry& parent_vtable_entry : parent->class_->virtual_table )
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
				the_class.virtual_table.push_back( parent_vtable_entry );
		} // for parent virtual table
	}
}

void CodeBuilder::TryGenerateDestructorPrototypeForPolymorphClass( Class& the_class, const Type& class_type )
{
	U_ASSERT( the_class.completeness != TypeCompleteness::Complete );
	U_ASSERT( the_class.virtual_table_llvm_type == nullptr );
	U_ASSERT( the_class.this_class_virtual_table == nullptr );

	const NamesScope::InsertedName* const destructor_name= the_class.members.GetThisScopeName( Keyword( Keywords::destructor_ ) );
	if( destructor_name != nullptr )
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
		the_class.virtual_table.push_back( std::move( new_virtual_table_entry ) );
	}
	else
		virtual_table_entry->function_variable= destructor_function_variable;

	// Add destructor to names scope.
	OverloadedFunctionsSet destructors_set;
	destructors_set.functions.push_back(destructor_function_variable);
	the_class.members.AddName( Keyword( Keywords::destructor_ ), destructors_set );
}

void CodeBuilder::ProcessClassVirtualFunction( Class& the_class, FunctionVariable& function )
{
	U_ASSERT( the_class.completeness != TypeCompleteness::Complete );

	const ProgramString& function_name= function.syntax_element->name_.components.back().name;
	const FilePos& file_pos= function.syntax_element->file_pos_;

	if( function.virtual_function_kind != Synt::VirtualFunctionKind::None &&
		the_class.GetMemberVisibility( function_name ) == ClassMemberVisibility::Private )
	{
		// Private members not visible in child classes. So, virtual private function is 100% error.
		errors_.push_back( ReportVirtualForPrivateFunction( file_pos, function_name ) );
	}

	if( !function.is_this_call )
		return; // May be in case of error

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
				the_class.virtual_table.push_back( std::move( new_virtual_table_entry ) );
			}
		}
		else if( virtual_table_entry != nullptr )
			errors_.push_back( ReportVirtualRequired( file_pos, function_name ) );
		break;

	case Synt::VirtualFunctionKind::DeclareVirtual:
		if( virtual_table_entry != nullptr )
			errors_.push_back( ReportOverrideRequired( file_pos, function_name ) );
		else
		{
			function.virtual_table_index= static_cast<unsigned int>(the_class.virtual_table.size());

			Class::VirtualTableEntry new_virtual_table_entry;
			new_virtual_table_entry.name= function_name;
			new_virtual_table_entry.function_variable= function;
			new_virtual_table_entry.is_pure= false;
			new_virtual_table_entry.is_final= false;
			the_class.virtual_table.push_back( std::move( new_virtual_table_entry ) );
		}
		break;

	case Synt::VirtualFunctionKind::VirtualOverride:
		if( virtual_table_entry == nullptr )
			errors_.push_back( ReportFunctionDoesNotOverride( file_pos, function_name ) );
		else if( virtual_table_entry->is_final )
			errors_.push_back( ReportOverrideFinalFunction( file_pos, function_name  ) );
		else
		{
			function.virtual_table_index= virtual_table_index;
			virtual_table_entry->function_variable= function;
			virtual_table_entry->is_pure= false;
		}
		break;

	case Synt::VirtualFunctionKind::VirtualFinal:
		if( virtual_table_entry == nullptr )
			errors_.push_back( ReportFinalForFirstVirtualFunction( file_pos, function_name ) );
		else
		{
			if( virtual_table_entry->is_final )
				errors_.push_back( ReportOverrideFinalFunction( file_pos, function_name ) );
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
			errors_.push_back( ReportOverrideRequired( file_pos, function_name ) );
		else
		{
			if( function.syntax_element->block_ != nullptr )
				errors_.push_back( ReportBodyForPureVirtualFunction( file_pos, function_name ) );
			if( function_name == Keyword( Keywords::destructor_ ) )
				errors_.push_back( ReportPureDestructor( file_pos, the_class.members.GetThisNamespaceName() ) );
			function.have_body= true; // Mark pure function as "with body", because we needs to disable real body creation for pure function.

			function.virtual_table_index= static_cast<unsigned int>(the_class.virtual_table.size());

			Class::VirtualTableEntry new_virtual_table_entry;
			new_virtual_table_entry.name= function_name;
			new_virtual_table_entry.function_variable= function;
			new_virtual_table_entry.is_pure= true;
			new_virtual_table_entry.is_final= false;
			the_class.virtual_table.push_back( std::move( new_virtual_table_entry ) );
		}
		break;
	};
}

void CodeBuilder::PrepareClassVirtualTableType( Class& the_class )
{
	U_ASSERT( the_class.completeness != TypeCompleteness::Complete );
	U_ASSERT( the_class.virtual_table_llvm_type == nullptr );

	if( the_class.virtual_table.empty() )
		return; // Non-polymorph class.


	the_class.virtual_table_llvm_type= llvm::StructType::create( llvm_context_ );
	std::vector<llvm::Type*> virtual_table_struct_fields;

	virtual_table_struct_fields.push_back( fundamental_llvm_types_.int_ptr ); // Offset field.

	for( const Class::VirtualTableEntry& virtual_table_entry : the_class.virtual_table )
	{
		const Function& function_type= *virtual_table_entry.function_variable.type.GetFunctionType();
		virtual_table_struct_fields.push_back( llvm::PointerType::get( function_type.llvm_function_type, 0u ) ); // Function pointer field.
	}

	the_class.virtual_table_llvm_type->setBody( virtual_table_struct_fields );
}

void CodeBuilder::BuildClassVirtualTables_r( Class& the_class, const Type& class_type, const std::vector< ClassProxyPtr >& dst_class_path, llvm::Value* dst_class_ptr_null_based )
{
	const Class& dst_class= *dst_class_path.back()->class_;
	std::vector<llvm::Constant*> initializer_values;

	// Calculate offset from this class pointer to ancestor class pointer.
	// In virtual call we must subtract this offset.
	llvm::Value* const offset_as_int= dummy_function_context_->llvm_ir_builder.CreatePtrToInt( dst_class_ptr_null_based, fundamental_llvm_types_.int_ptr );
	llvm::Constant* const offset_const= llvm::dyn_cast<llvm::Constant>( offset_as_int );
	initializer_values.push_back( offset_const );

	for( const Class::VirtualTableEntry& ancestor_virtual_table_entry : dst_class.virtual_table )
	{
		const Class::VirtualTableEntry* overriden_in_this_class= nullptr;
		for( const Class::VirtualTableEntry& this_class_virtual_table_entry : the_class.virtual_table )
		{
			if( this_class_virtual_table_entry.name == ancestor_virtual_table_entry.name &&
				this_class_virtual_table_entry.function_variable.VirtuallyEquals( ancestor_virtual_table_entry.function_variable ) )
			{
				overriden_in_this_class= &this_class_virtual_table_entry;
				break;
			}
		}

		U_ASSERT( overriden_in_this_class != nullptr ); // We must override, or inherit function.

		llvm::Value* const function_pointer_casted=
			dummy_function_context_->llvm_ir_builder.CreateBitOrPointerCast(
				overriden_in_this_class->function_variable.llvm_function,
				llvm::PointerType::get( ancestor_virtual_table_entry.function_variable.type.GetFunctionType()->llvm_function_type, 0 ) );

		initializer_values.push_back( llvm::dyn_cast<llvm::Constant>(function_pointer_casted) );

	} // for ancestor virtual table

	std::string vtable_name= "_vtable_of_" + MangleType( class_type ) + "_for";
	for( const ClassProxyPtr& path_component : dst_class_path )
		vtable_name+= "_" + MangleType( path_component );

	llvm::GlobalVariable* const ancestor_vtable=
		new llvm::GlobalVariable(
			*module_,
			dst_class.virtual_table_llvm_type,
			true, // is constant
			llvm::GlobalValue::InternalLinkage,
			 llvm::ConstantStruct::get( dst_class.virtual_table_llvm_type, initializer_values ),
			vtable_name);
	ancestor_vtable->setUnnamedAddr( true );

	U_ASSERT( the_class.ancestors_virtual_tables.find( dst_class_path ) == the_class.ancestors_virtual_tables.end() );
	the_class.ancestors_virtual_tables[dst_class_path]= ancestor_vtable;

	if( !dst_class.parents.empty() )
	{
		auto parent_path= dst_class_path;
		parent_path.emplace_back();
		for( size_t i= 0u; i < dst_class.parents.size(); ++i )
		{
			parent_path.back()= dst_class.parents[i];

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(dst_class.parents_fields_numbers[i]) ) );
			llvm::Value* const offset_ptr= dummy_function_context_->llvm_ir_builder.CreateGEP( dst_class_ptr_null_based, index_list );

			BuildClassVirtualTables_r( the_class, class_type, parent_path, offset_ptr );
		}
	}
}

void CodeBuilder::BuildClassVirtualTables( Class& the_class, const Type& class_type )
{
	U_ASSERT( the_class.completeness != TypeCompleteness::Complete );
	U_ASSERT( the_class.this_class_virtual_table == nullptr );
	U_ASSERT( the_class.ancestors_virtual_tables.empty() );

	if( the_class.virtual_table.empty() )
		return; // Non-polymorph class.

	U_ASSERT( the_class.virtual_table_llvm_type != nullptr );

	std::vector<llvm::Constant*> initializer_values;
	initializer_values.push_back(
		llvm::Constant::getIntegerValue(
			fundamental_llvm_types_.int_ptr,
			llvm::APInt( fundamental_llvm_types_.int_ptr->getIntegerBitWidth(), 0u ) ) ); // For this class virtual table we have zero offset to real this.

	for( const Class::VirtualTableEntry& virtual_table_entry : the_class.virtual_table )
	{
		if( virtual_table_entry.is_pure )
			return;  // Class is interface or abstract.

		initializer_values.push_back( virtual_table_entry.function_variable.llvm_function );
	}

	the_class.this_class_virtual_table=
		new llvm::GlobalVariable(
			*module_,
			the_class.virtual_table_llvm_type,
			true, // is constant
			llvm::GlobalValue::InternalLinkage,
			 llvm::ConstantStruct::get( the_class.virtual_table_llvm_type, initializer_values ),
			"_vtable_main_" + MangleType(class_type) );
	the_class.this_class_virtual_table->setUnnamedAddr( true );

	// Recursive build virtual tables for all instances of all ancestors.
	llvm::Value* const this_nullptr= llvm::Constant::getNullValue( llvm::PointerType::get( the_class.llvm_type, 0u ) );
	for( size_t i= 0u; i < the_class.parents.size(); ++i )
	{
		llvm::Value* index_list[2];
		index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(the_class.parents_fields_numbers[i]) ) );
		llvm::Value* const offset_ptr= dummy_function_context_->llvm_ir_builder.CreateGEP( this_nullptr, index_list );
		BuildClassVirtualTables_r( the_class, class_type, {the_class.parents[i]}, offset_ptr );
	}
}


std::pair<Variable, llvm::Value*> CodeBuilder::TryFetchVirtualFunction( const Variable& this_, const FunctionVariable& function, FunctionContext& function_context )
{
	const Function& function_type= *function.type.GetFunctionType();

	if( !ReferenceIsConvertible( this_.type, function_type.args.front().type, FilePos() ) )
		return std::make_pair( this_, function.llvm_function );

	Variable this_casted;
	this_casted= this_;
	if( this_.type != function_type.args.front().type )
	{
		this_casted.type= function_type.args.front().type;
		this_casted.llvm_value= CreateReferenceCast( this_.llvm_value, this_.type, this_casted.type, function_context );
	}

	llvm::Value* llvm_function_ptr= function.llvm_function;
	if( function.virtual_table_index != ~0u )
	{
		const Class* const class_type= this_casted.type.GetClassType();
		U_ASSERT( class_type != nullptr );
		U_ASSERT( function.virtual_table_index < class_type->virtual_table.size() );

		const unsigned int func_ptr_field_number= function.virtual_table_index + 1u;
		const unsigned int offset_field_number= 0u;

		// Fetch vtable pointer.
		llvm::Value* index_list[2];
		index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(class_type->virtual_table_field_number) ) );
		llvm::Value* const ptr_to_virtual_table_ptr= function_context.llvm_ir_builder.CreateGEP( this_casted.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );
		llvm::Value* const virtual_table_ptr= function_context.llvm_ir_builder.CreateLoad( ptr_to_virtual_table_ptr );
		// Fetch function.
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(func_ptr_field_number) ) );
		llvm::Value* const ptr_to_function_ptr= function_context.llvm_ir_builder.CreateGEP( virtual_table_ptr, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );
		llvm_function_ptr= function_context.llvm_ir_builder.CreateLoad( ptr_to_function_ptr );
		// Fetch "this" pointer offset.
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(offset_field_number) ) );
		llvm::Value* const offset_ptr= function_context.llvm_ir_builder.CreateGEP( virtual_table_ptr, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );
		llvm::Value* const offset= function_context.llvm_ir_builder.CreateLoad( offset_ptr );
		// Correct "this" pointer.
		llvm::Value* const this_ptr_as_int= function_context.llvm_ir_builder.CreatePtrToInt( this_casted.llvm_value, fundamental_llvm_types_.int_ptr );
		llvm::Value* this_sub_offset= function_context.llvm_ir_builder.CreateSub( this_ptr_as_int, offset );
		this_casted.llvm_value= function_context.llvm_ir_builder.CreateIntToPtr( this_sub_offset, llvm::PointerType::get( this_casted.type.GetLLVMType(), 0u ) );
	}

	return std::make_pair( std::move(this_casted), llvm_function_ptr );
}


void CodeBuilder::SetupVirtualTablePointers_r(
	llvm::Value* this_,
	const std::vector< ClassProxyPtr >& class_path,
	const std::map< std::vector< ClassProxyPtr >, llvm::GlobalVariable* > virtual_tables,
	FunctionContext& function_context )
{
	const Class& the_class= *class_path.back()->class_;
	U_ASSERT( virtual_tables.find( class_path ) != virtual_tables.end() );

	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

	index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(the_class.virtual_table_field_number) ) );
	llvm::Value* const ptr_to_vtable_ptr= function_context.llvm_ir_builder.CreateGEP( this_, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );
	function_context.llvm_ir_builder.CreateStore( virtual_tables.find(class_path)->second, ptr_to_vtable_ptr );

	if( !the_class.parents.empty() )
	{
		auto parent_path= class_path;
		parent_path.emplace_back();
		for( size_t i= 0u; i < the_class.parents.size(); ++i )
		{
			parent_path.back()= the_class.parents[i];

			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(the_class.parents_fields_numbers[i]) ) );
			llvm::Value* const parent_ptr= function_context.llvm_ir_builder.CreateGEP( this_, index_list );
			SetupVirtualTablePointers_r( parent_ptr, parent_path, virtual_tables, function_context );
		}
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

	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

	index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(the_class.virtual_table_field_number) ) );
	llvm::Value* const ptr_to_vtable_ptr= function_context.llvm_ir_builder.CreateGEP( this_, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );
	function_context.llvm_ir_builder.CreateStore( the_class.this_class_virtual_table, ptr_to_vtable_ptr );

	for( size_t i= 0u; i < the_class.parents.size(); ++i )
	{
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(the_class.parents_fields_numbers[i]) ) );
		llvm::Value* const parent_ptr= function_context.llvm_ir_builder.CreateGEP( this_, index_list );
		SetupVirtualTablePointers_r( parent_ptr, { the_class.parents[i] }, the_class.ancestors_virtual_tables, function_context );
	}
}

} // namespace CodeBuilderPrivate

} // namespace U
