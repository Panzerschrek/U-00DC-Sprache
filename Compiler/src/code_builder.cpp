#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include "pop_llvm_warnings.hpp"

#include "assert.hpp"
#include "keywords.hpp"
#include "lang_types.hpp"

#include "code_builder.hpp"

namespace U
{

namespace
{

typedef std::map< ProgramString, U_FundamentalType > TypesMap;

const TypesMap g_types_map=
{
	{ Keyword( Keywords::void_ ), U_FundamentalType::Void },
	{ Keyword( Keywords::bool_ ), U_FundamentalType::Bool },
	{ Keyword( Keywords::i8_  ), U_FundamentalType::i8  },
	{ Keyword( Keywords::u8_  ), U_FundamentalType::u8  },
	{ Keyword( Keywords::i16_ ), U_FundamentalType::i16 },
	{ Keyword( Keywords::u16_ ), U_FundamentalType::u16 },
	{ Keyword( Keywords::i32_ ), U_FundamentalType::i32 },
	{ Keyword( Keywords::u32_ ), U_FundamentalType::u32 },
	{ Keyword( Keywords::i64_ ), U_FundamentalType::i64 },
	{ Keyword( Keywords::u64_ ), U_FundamentalType::u64 },
	{ Keyword( Keywords::f32_ ), U_FundamentalType::f32 },
	{ Keyword( Keywords::f64_ ), U_FundamentalType::f64 },
};

bool IsNumericType( U_FundamentalType type )
{
	return
		( type >= U_FundamentalType::i8 && type <= U_FundamentalType::u64 ) ||
		type == U_FundamentalType::f32 ||
		type == U_FundamentalType::f64;
}

bool IsUnsignedInteger( U_FundamentalType type )
{
	return
		type == U_FundamentalType::u8  ||
		type == U_FundamentalType::u16 ||
		type == U_FundamentalType::u32 ||
		type == U_FundamentalType::u64;
}

static bool IsSignedInteger( U_FundamentalType type )
{
	return
		type == U_FundamentalType::i8  ||
		type == U_FundamentalType::i16 ||
		type == U_FundamentalType::i32 ||
		type == U_FundamentalType::i64;
}

static bool IsInteger( U_FundamentalType type )
{
	return IsSignedInteger( type ) || IsUnsignedInteger( type );
}

bool IsFloatingPoint( U_FundamentalType type )
{
	return
		type == U_FundamentalType::f32 ||
		type == U_FundamentalType::f64;
}

U_FundamentalType GetNumericConstantType( const NumericConstant& number )
{
	if( number.type_suffix_.empty() )
	{
		if( number.has_fractional_point_ )
			return U_FundamentalType::f64;
		else
			return U_FundamentalType::i32;
	}

	// Allow simple "u" suffix for unsigned 32bit values.
	// SPRACHE_TODO - maybe add "i" suffix for i32 type?
	if( number.type_suffix_ == "u"_SpC )
		return U_FundamentalType::u32;

	auto it= g_types_map.find( number.type_suffix_ );
	if( it == g_types_map.end() )
		return U_FundamentalType::InvalidType;

	return it->second;
}

} // namespace

namespace CodeBuilderPrivate
{

CodeBuilder::FunctionContext::FunctionContext(
	const Type in_return_type,
	const bool in_return_value_is_mutable,
	const bool in_return_value_is_reference,
	llvm::LLVMContext& llvm_context,
	llvm::Function* in_function )
	: return_type(in_return_type)
	, return_value_is_mutable(in_return_value_is_mutable)
	, return_value_is_reference(in_return_value_is_reference)
	, function(in_function)
	, function_basic_block( llvm::BasicBlock::Create( llvm_context, "", function ) )
	, llvm_ir_builder( function_basic_block )
	, block_for_break( nullptr )
	, block_for_continue( nullptr )
{
}

CodeBuilder::CodeBuilder()
	: llvm_context_( llvm::getGlobalContext() )
{
	fundamental_llvm_types_. i8= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_. u8= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.i16= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.u16= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.i32= llvm::Type::getInt32Ty( llvm_context_ );
	fundamental_llvm_types_.u32= llvm::Type::getInt32Ty( llvm_context_ );
	fundamental_llvm_types_.i64= llvm::Type::getInt64Ty( llvm_context_ );
	fundamental_llvm_types_.u64= llvm::Type::getInt64Ty( llvm_context_ );

	fundamental_llvm_types_.f32= llvm::Type::getFloatTy( llvm_context_ );
	fundamental_llvm_types_.f64= llvm::Type::getDoubleTy( llvm_context_ );

	fundamental_llvm_types_.invalid_type_= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.void_= llvm::Type::getVoidTy( llvm_context_ );
	fundamental_llvm_types_.bool_= llvm::Type::getInt1Ty( llvm_context_ );
}

CodeBuilder::~CodeBuilder()
{
}

CodeBuilder::BuildResult CodeBuilder::BuildProgram( const ProgramElements& program_elements )
{
	module_= std::unique_ptr<llvm::Module>( new llvm::Module( "U-Module", llvm_context_ ) );
	errors_.clear();
	error_count_= 0u;

	for( const IProgramElementPtr& program_element : program_elements )
	{
		if( const FunctionDeclaration* func=
			dynamic_cast<const FunctionDeclaration*>( program_element.get() ) )
		{
			if( IsKeyword( func->name_ ) )
				errors_.push_back( ReportUsingKeywordAsName( func->file_pos_ ) );

			Variable func_info;

			func_info.location= Variable::Location::Pointer;
			func_info.value_type= ValueType::ConstReference;

			std::unique_ptr<Function> function_type_storage( new Function() );
			Function& function_type= *function_type_storage;
			func_info.type.one_of_type_kind= std::move(function_type_storage);

			if( func->return_type_.empty() )
			{
				function_type.return_type.one_of_type_kind=
					FundamentalType( U_FundamentalType::Void, fundamental_llvm_types_.void_ );
			}
			else
			{
				// TODO - support nonfundamental return types.
				auto it= g_types_map.find( func->return_type_ );
				if( it == g_types_map.end() )
				{
					errors_.push_back( ReportNameNotFound( func->file_pos_, func->return_type_ ) );
					function_type.return_type.one_of_type_kind= FundamentalType( U_FundamentalType::InvalidType, fundamental_llvm_types_.invalid_type_ );
				}
				else
					function_type.return_type.one_of_type_kind= FundamentalType( it->second, GetFundamentalLLVMType( it->second ) );
			}

			// TODO - make variables without explicit mutability modifiers immutable.
			if( func->return_value_mutability_modifier_ == MutabilityModifier::Immutable )
				function_type.return_value_is_mutable= false;
			else
				function_type.return_value_is_mutable= true;
			function_type.return_value_is_reference= func->return_value_reference_modifier_ == ReferenceModifier::Reference;

			// Args.
			function_type.args.reserve( func->arguments_.size() );
			for( const FunctionArgumentDeclarationPtr& arg : func->arguments_ )
			{
				if( IsKeyword( arg->name_ ) )
					errors_.push_back( ReportUsingKeywordAsName( arg->file_pos_ ) );

				function_type.args.emplace_back();
				Function::Arg& out_arg= function_type.args.back();

				out_arg.type= PrepareType( arg->file_pos_, arg->type_ );

				// TODO - make variables without explicit mutability modifiers immutable.
				if( arg->mutability_modifier_ == MutabilityModifier::Immutable )
					out_arg.is_mutable= false;
				else
					out_arg.is_mutable= true;
				out_arg.is_reference= arg->reference_modifier_ == ReferenceModifier::Reference;
			}

			NamesScope::InsertedName* const func_name=
				global_names_.GetThisScopeName( func->name_ );
			if( func_name == nullptr )
			{
				OverloadedFunctionsSet functions_set;
				functions_set.push_back( std::move( func_info ) );

				// New name in this scope - insert it.
				NamesScope::InsertedName* const inserted_func=
					global_names_.AddName( func->name_, std::move( functions_set ) );
				U_ASSERT( inserted_func != nullptr );

				BuildFuncCode(
					boost::get<OverloadedFunctionsSet>( inserted_func->second ).front(),
					func->name_,
					func->arguments_,
					*func->block_ );
			}
			else
			{
				NamedSomething& named_something= func_name->second;
				if( OverloadedFunctionsSet* const functions_set=
					boost::get<OverloadedFunctionsSet>( &named_something ) )
				{
					try
					{
						ApplyOverloadedFunction( *functions_set, func_info, func->file_pos_ );
					} catch( const ProgramError& )
					{
						continue;
					}

					BuildFuncCode(
						functions_set->back(),
						func->name_,
						func->arguments_,
						*func->block_ );
				}
				else
					errors_.push_back( ReportRedefinition( func->file_pos_, func_name->first ) );
			}
		}
		else if(
			const ClassDeclaration* class_=
			dynamic_cast<const ClassDeclaration*>( program_element.get() ) )
		{
			const NamesScope::InsertedName* inserted_name=
				global_names_.AddName( class_->name_, PrepareClass( *class_ ) );
			if( inserted_name == nullptr )
			{
				errors_.push_back( ReportRedefinition( class_->file_pos_, class_->name_ ) );
			}
		}
		else
		{
			U_ASSERT(false);
		}
	} // for program elements

	if( error_count_ > 0u )
		errors_.push_back( ReportBuildFailed() );

	BuildResult result;
	result.errors= errors_;
	errors_.clear();
	result.module= std::move( module_ );
	return result;
}

Type CodeBuilder::PrepareType( const FilePos& file_pos, const TypeName& type_name )
{
	Type result;
	Type* last_type= &result;

	constexpr unsigned int c_max_array_dimensions= 16u;
	Type* arrays_stack[ c_max_array_dimensions ];
	unsigned int arrays_count= 0u;

	// Fill arrays hierarchy.
	for( auto rit= type_name.array_sizes.rbegin(); rit != type_name.array_sizes.rend(); ++rit )
	{
		if( arrays_count >= c_max_array_dimensions )
		{
			U_ASSERT( false && "WTF?" );
		}

		arrays_stack[ arrays_count ]= last_type;
		arrays_count++;

		const NumericConstant& num= * *rit;

		std::unique_ptr<Array> array_type_storage( new Array() );
		Array& array_type= *array_type_storage;
		last_type->one_of_type_kind= std::move(array_type_storage);

		U_FundamentalType size_type= GetNumericConstantType( num );
		if( !IsInteger(size_type) )
			errors_.push_back( ReportArraySizeIsNotInteger( num.file_pos_ ) );
		if( num.value_ < 0 )
			errors_.push_back( ReportArraySizeIsNegative( num.file_pos_ ) );

		array_type.size= size_t( std::max( num.value_, static_cast<NumericConstant::LongFloat>(0.0) ) );

		last_type= &array_type.type;
	}

	last_type->one_of_type_kind= FundamentalType( U_FundamentalType::InvalidType, fundamental_llvm_types_.invalid_type_ );

	auto it= g_types_map.find( type_name.name );
	if( it == g_types_map.end() )
	{
		const NamesScope::InsertedName* custom_type_name=
			global_names_.GetName( type_name.name );
		if( custom_type_name != nullptr )
		{
			const ClassPtr* const class_= boost::get<ClassPtr>( &custom_type_name->second );
			if( class_ != nullptr )
			{
				U_ASSERT( (*class_) != nullptr );
				last_type->one_of_type_kind= *class_;
			}
			else
				errors_.push_back( ReportNameIsNotTypeName( file_pos, type_name.name ) );
		}
		else
			errors_.push_back( ReportNameNotFound( file_pos, type_name.name ) );
	}
	else
	{
		last_type->one_of_type_kind= FundamentalType( it->second, GetFundamentalLLVMType( it->second ) );
	}

	// Setup arrays llvm types.
	if( arrays_count > 0u )
	{
		{
			const ArrayPtr& array_type= boost::get<ArrayPtr>( arrays_stack[ arrays_count - 1u ]->one_of_type_kind );
			U_ASSERT( array_type != nullptr );

				array_type->llvm_type=
				llvm::ArrayType::get(
					last_type->GetLLVMType(),
					array_type->size );
		}

		for( unsigned int i= arrays_count - 1u; i > 0u; i-- )
		{
			const ArrayPtr& array_type= boost::get<ArrayPtr>( arrays_stack[ i - 1u ]->one_of_type_kind );
			U_ASSERT( array_type != nullptr );

			array_type->llvm_type=
				llvm::ArrayType::get(
					boost::get<ArrayPtr>( arrays_stack[i]->one_of_type_kind )->llvm_type,
					array_type->size );
		}
	}

	return result;
}

ClassPtr CodeBuilder::PrepareClass( const ClassDeclaration& class_declaration )
{
	if( IsKeyword( class_declaration.name_ ) )
		errors_.push_back( ReportUsingKeywordAsName( class_declaration.file_pos_ ) );

	ClassPtr result= std::make_shared<Class>();

	result->name= class_declaration.name_;

	std::vector<llvm::Type*> members_llvm_types;

	members_llvm_types.reserve( class_declaration.fields_.size() );
	result->fields.reserve( class_declaration.fields_.size() );
	for( const ClassDeclaration::Field& in_field : class_declaration.fields_ )
	{
		if( result->GetField( in_field.name ) != nullptr )
			errors_.push_back( ReportRedefinition( in_field.file_pos, in_field.name ) );

		Class::Field out_field;
		out_field.name= in_field.name;
		out_field.type= PrepareType( in_field.file_pos, in_field.type );
		out_field.index= result->fields.size();

		members_llvm_types.emplace_back( out_field.type.GetLLVMType() );
		result->fields.emplace_back( std::move( out_field ) );
	}

	result->llvm_type=
		llvm::StructType::create(
			llvm_context_,
			members_llvm_types,
			ToStdString(class_declaration.name_) );

	return result;
}

void CodeBuilder::BuildFuncCode(
	Variable& func_variable,
	const ProgramString& func_name,
	const FunctionArgumentsDeclaration& args,
	const Block& block ) noexcept
{
	std::vector<llvm::Type*> args_llvm_types;
	const FunctionPtr& function_type_ptr= boost::get<FunctionPtr>( func_variable.type.one_of_type_kind );

	for( const Function::Arg& arg : function_type_ptr->args )
	{
		llvm::Type* type= arg.type.GetLLVMType();
		if( arg.is_reference )
			type= llvm::PointerType::get( type, 0u );
		args_llvm_types.push_back( type );
	}

	llvm::Type* llvm_function_return_type= function_type_ptr->return_type.GetLLVMType();
	if( function_type_ptr->return_value_is_reference )
		llvm_function_return_type= llvm::PointerType::get( llvm_function_return_type, 0u );

	function_type_ptr->llvm_function_type=
		llvm::FunctionType::get(
			llvm_function_return_type,
			llvm::ArrayRef<llvm::Type*>( args_llvm_types.data(), args_llvm_types.size() ),
			false );

	llvm::Function* llvm_function=
		llvm::Function::Create(
			function_type_ptr->llvm_function_type,
			llvm::Function::LinkageTypes::ExternalLinkage, // TODO - select linkage
			ToStdString( func_name ),
			module_.get() );

	NamesScope function_names( &global_names_ );
	FunctionContext function_context(
		function_type_ptr->return_type,
		function_type_ptr->return_value_is_mutable,
		function_type_ptr->return_value_is_reference,
		llvm_context_,
		llvm_function );

	unsigned int arg_number= 0u;
	for( llvm::Argument& llvm_arg : llvm_function->args() )
	{
		const Function::Arg& arg= function_type_ptr->args[ arg_number ];

		Variable var;
		var.location= Variable::Location::LLVMRegister;
		var.value_type= ValueType::Reference;
		var.type= arg.type;
		var.llvm_value= &llvm_arg;

		// TODO - make variables without explicit mutability modifiers immutable.
		if( args[ arg_number ]->mutability_modifier_ == MutabilityModifier::Immutable )
			var.value_type= ValueType::ConstReference;

		if( arg.is_reference )
			var.location= Variable::Location::Pointer;
		else
		{
			// Move parameters to stack for assignment possibility.
			// TODO - do it, only if parameters are not constant.

			llvm::Value* address= function_context.llvm_ir_builder.CreateAlloca( var.type.GetLLVMType() );
			function_context.llvm_ir_builder.CreateStore( var.llvm_value, address );

			var.llvm_value= address;
			var.location= Variable::Location::Pointer;
		}

		const ProgramString& arg_name= args[ arg_number ]->name_;

		const NamesScope::InsertedName* inserted_arg=
			function_names.AddName(
				arg_name,
				std::move(var) );
		if( !inserted_arg )
		{
			errors_.push_back( ReportRedefinition( args[ arg_number ]->file_pos_, arg_name ) );
			return;
		}

		llvm_arg.setName( ToStdString( arg_name ) );
		++arg_number;
	}

	func_variable.llvm_value= llvm_function;

	const BlockBuildInfo block_build_info=
		BuildBlockCode( block, function_names, function_context );

	Type void_type;
	void_type.one_of_type_kind= FundamentalType( U_FundamentalType::Void, fundamental_llvm_types_.void_ );

	if(  function_type_ptr->return_type == void_type )
	{
		// Manually generate "return" for void-return functions.
		if( !block_build_info.have_unconditional_return_inside )
			function_context.llvm_ir_builder.CreateRetVoid();
	}
	else
	{
		if( !block_build_info.have_unconditional_return_inside )
		{
			errors_.push_back( ReportNoReturnInFunctionReturningNonVoid( block.file_pos_ ) );
			return;
		}
	}

	llvm::Function::BasicBlockListType& bb_list = llvm_function->getBasicBlockList();

	// Remove duplicated terminator instructions at end of all function blocks.
	// This needs, for example, when "return" is last operator inside "if" or "while" blocks.
	for (llvm::BasicBlock& block : bb_list)
	{
		llvm::BasicBlock::InstListType& instr_list = block.getInstList();
		while (instr_list.size() >= 2u && instr_list.back().isTerminator() &&
			(std::next(instr_list.rbegin()))->isTerminator())
		{
			instr_list.pop_back();
		}
	}

	// Remove basic blocks without instructions. Such block can exist after if/else with return in all branches, for example.
	auto it= bb_list.begin();
	while(it != bb_list.end())
	{
		if( it->empty() )
			it= bb_list.erase(it);
		else
			++it;
	}
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockCode(
	const Block& block,
	const NamesScope& names,
	FunctionContext& function_context ) noexcept
{
	NamesScope block_names( &names );
	BlockBuildInfo block_build_info;

	for( const IBlockElementPtr& block_element : block.elements_ )
	{
		const IBlockElement* const block_element_ptr= block_element.get();

		const auto try_report_unreachable_code=
		[&]
		{
			const unsigned int block_element_index= &block_element - block.elements_.data();
			if( block_element_index + 1u < block.elements_.size() )
				errors_.push_back( ReportUnreachableCode( block.elements_[ block_element_index + 1u ]->file_pos_ ) );
		};

		try
		{
			if( const VariablesDeclaration* variables_declaration=
				dynamic_cast<const VariablesDeclaration*>( block_element_ptr ) )
			{
				BuildVariablesDeclarationCode( *variables_declaration, block_names, function_context );
			}
			else if(
				const SingleExpressionOperator* expression=
				dynamic_cast<const SingleExpressionOperator*>( block_element_ptr ) )
			{
				BuildExpressionCode(
					*expression->expression_,
					block_names,
					function_context );
			}
			else if(
				const AssignmentOperator* assignment_operator=
				dynamic_cast<const AssignmentOperator*>( block_element_ptr ) )
			{
				BuildAssignmentOperatorCode( *assignment_operator, block_names, function_context );
			}
			else if(
				const ReturnOperator* return_operator=
				dynamic_cast<const ReturnOperator*>( block_element_ptr ) )
			{
				BuildReturnOperatorCode(
					*return_operator,
					block_names,
					function_context );

				block_build_info.have_unconditional_return_inside= true;
				try_report_unreachable_code();
			}
			else if(
				const WhileOperator* while_operator=
				dynamic_cast<const WhileOperator*>( block_element_ptr ) )
			{
				BuildWhileOperatorCode(
					*while_operator,
					block_names,
					function_context );
			}
			else if(
				const BreakOperator* break_operator=
				dynamic_cast<const BreakOperator*>( block_element_ptr ) )
			{
				BuildBreakOperatorCode(
					*break_operator,
					function_context );

				block_build_info.have_uncodnitional_break_or_continue= true;
				try_report_unreachable_code();
			}
			else if(
				const ContinueOperator* continue_operator=
				dynamic_cast<const ContinueOperator*>( block_element_ptr ) )
			{
				BuildContinueOperatorCode(
					*continue_operator,
					function_context );

				block_build_info.have_uncodnitional_break_or_continue= true;
				try_report_unreachable_code();
			}
			else if(
				const IfOperator* if_operator=
				dynamic_cast<const IfOperator*>( block_element_ptr ) )
			{
				const CodeBuilder::BlockBuildInfo if_block_info=
					BuildIfOperatorCode(
						*if_operator,
						block_names,
						function_context );

				block_build_info.have_unconditional_return_inside=
					block_build_info.have_unconditional_return_inside || if_block_info.have_unconditional_return_inside;
				block_build_info.have_uncodnitional_break_or_continue=
					block_build_info.have_uncodnitional_break_or_continue || if_block_info.have_uncodnitional_break_or_continue;

				if( if_block_info.have_unconditional_return_inside ||
					block_build_info.have_uncodnitional_break_or_continue )
					try_report_unreachable_code();
			}
			else if(
				const Block* block=
				dynamic_cast<const Block*>( block_element_ptr ) )
			{
				const BlockBuildInfo inner_block_build_info=
					BuildBlockCode( *block, block_names, function_context );

				block_build_info.have_unconditional_return_inside=
					block_build_info.have_unconditional_return_inside || inner_block_build_info.have_unconditional_return_inside;
				block_build_info.have_uncodnitional_break_or_continue=
					block_build_info.have_uncodnitional_break_or_continue || inner_block_build_info.have_uncodnitional_break_or_continue;

				if( inner_block_build_info.have_unconditional_return_inside ||
					block_build_info.have_uncodnitional_break_or_continue )
					try_report_unreachable_code();
			}
			else
			{
				U_ASSERT(false);
			}
		}
		catch( const ProgramError& )
		{
			error_count_++;
		}
	}

	return block_build_info;
}

Variable CodeBuilder::BuildExpressionCode(
	const BinaryOperatorsChain& expression,
	const NamesScope& names,
	FunctionContext& function_context )
{
	const InversePolishNotation ipn= ConvertToInversePolishNotation( expression );

	return
		BuildExpressionCode_r(
			ipn,
			ipn.size() - 1,
			names,
			function_context );
}

Variable CodeBuilder::BuildExpressionCode_r(
	const InversePolishNotation& ipn,
	unsigned int ipn_index,
	const NamesScope& names,
	FunctionContext& function_context )
{
	U_ASSERT( ipn_index < ipn.size() );
	const InversePolishNotationComponent& comp= ipn[ ipn_index ];

	const FilePos file_pos = ipn.front().operand->file_pos_;

	if( comp.operator_ != BinaryOperator::None )
	{
		Variable l_var=
			BuildExpressionCode_r(
				ipn, comp.l_index,
				names,
				function_context );

		Variable r_var=
			BuildExpressionCode_r(
				ipn, comp.r_index,
				names,
				function_context );

		Variable result;

		// TODO - add cast for some integers here.
		if( r_var.type != l_var.type )
		{
			// TODO - report types mismatch.
			throw ProgramError();
		}

		const Type& result_type= r_var.type;
		const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &result_type.one_of_type_kind );

		switch( comp.operator_ )
		{
		case BinaryOperator::Add:
		case BinaryOperator::Sub:
		case BinaryOperator::Div:
		case BinaryOperator::Mul:

			if( fundamental_type == nullptr )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}
			else
			{
				if( result_type.SizeOf() < 4u )
				{
					// Operation supported only for 32 and 64bit operands
					errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
					throw ProgramError();
				}
				const bool is_float= IsFloatingPoint( fundamental_type->fundamental_type );
				if( !( IsInteger( fundamental_type->fundamental_type ) || is_float ) )
				{
					// this operations allowed only for integer and floating point operands.
					errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
					throw ProgramError();
				}

				const bool is_signed= IsSignedInteger( fundamental_type->fundamental_type );

				llvm::Value* l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
				llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
				llvm::Value* result_value;

				switch( comp.operator_ )
				{
				case BinaryOperator::Add:
					if( is_float )
						result_value=
							function_context.llvm_ir_builder.CreateFAdd( l_value_for_op, r_value_for_op );
					else
						result_value=
							function_context.llvm_ir_builder.CreateAdd( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperator::Sub:
					if( is_float )
						result_value=
							function_context.llvm_ir_builder.CreateFSub( l_value_for_op, r_value_for_op );
					else
						result_value=
							function_context.llvm_ir_builder.CreateSub( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperator::Div:
					if( is_float )
						result_value=
							function_context.llvm_ir_builder.CreateFDiv( l_value_for_op, r_value_for_op );
					else if( is_signed )
						result_value=
							function_context.llvm_ir_builder.CreateSDiv( l_value_for_op, r_value_for_op );
					else
						result_value=
							function_context.llvm_ir_builder.CreateUDiv( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperator::Mul:
					if( is_float )
						result_value=
							function_context.llvm_ir_builder.CreateFMul( l_value_for_op, r_value_for_op );
					else
						result_value=
							function_context.llvm_ir_builder.CreateMul( l_value_for_op, r_value_for_op );
					break;

				default: U_ASSERT( false ); break;
				};

				result.location= Variable::Location::LLVMRegister;
				result.value_type= ValueType::Value;
				result.type= r_var.type;
				result.llvm_value= result_value;
			}
			break;


		case BinaryOperator::Equal:
		case BinaryOperator::NotEqual:

		if( fundamental_type == nullptr )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
			throw ProgramError();
		}
		else
		{
			const bool if_float= IsFloatingPoint( fundamental_type->fundamental_type );
			if( !( IsInteger( fundamental_type->fundamental_type ) || if_float || fundamental_type->fundamental_type == U_FundamentalType::Bool ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}

			llvm::Value* l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
			llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			llvm::Value* result_value;

			switch( comp.operator_ )
			{
			// TODO - select ordered/unordered comparision flags for floats.
			case BinaryOperator::Equal:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUEQ( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpEQ( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperator::NotEqual:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUNE( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpNE( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT( false ); break;
			};

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type.one_of_type_kind= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.bool_ );
			result.llvm_value= result_value;
		}
			break;

		case BinaryOperator::Less:
		case BinaryOperator::LessEqual:
		case BinaryOperator::Greater:
		case BinaryOperator::GreaterEqual:
		if( fundamental_type == nullptr )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
			throw ProgramError();
		}
		else
		{
			const bool if_float= IsFloatingPoint( fundamental_type->fundamental_type );
			const bool is_signed= IsSignedInteger( fundamental_type->fundamental_type );
			if( !( IsInteger( fundamental_type->fundamental_type ) || if_float ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}

			llvm::Value* l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
			llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			llvm::Value* result_value;

			switch( comp.operator_ )
			{
			// TODO - select ordered/unordered comparision flags for floats.
			case BinaryOperator::Less:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpULT( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSLT( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpULT( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperator::LessEqual:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpULE( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSLE( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpULE( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperator::Greater:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUGT( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSGT( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpUGT( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperator::GreaterEqual:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUGE( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSGE( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpUGE( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT( false ); break;
			};

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type.one_of_type_kind= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.bool_ );
			result.llvm_value= result_value;
		}
			break;

		case BinaryOperator::And:
		case BinaryOperator::Or:
		case BinaryOperator::Xor:
		if( fundamental_type == nullptr )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
			throw ProgramError();
		}
		else
		{
			if( !( IsInteger( fundamental_type->fundamental_type ) || fundamental_type->fundamental_type == U_FundamentalType::Bool ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}

			llvm::Value* l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
			llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			llvm::Value* result_value;

			switch( comp.operator_ )
			{
			case BinaryOperator::And:
				result_value=
					function_context.llvm_ir_builder.CreateAnd( l_value_for_op, r_value_for_op );
				break;
			case BinaryOperator::Or:
				result_value=
					function_context.llvm_ir_builder.CreateOr( l_value_for_op, r_value_for_op );
				break;
			case BinaryOperator::Xor:
				result_value=
					function_context.llvm_ir_builder.CreateXor( l_value_for_op, r_value_for_op );
				break;
			default: U_ASSERT( false ); break;
			};

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type= result_type;
			result.llvm_value= result_value;
		}
			break;

		case BinaryOperator::LazyLogicalAnd:
		case BinaryOperator::LazyLogicalOr:
		case BinaryOperator::None:
		case BinaryOperator::Last:
			U_ASSERT(false);
			break;
		};

		return result;
	}
	else
	{
		U_ASSERT( comp.operand );
		U_ASSERT( comp.r_index == InversePolishNotationComponent::c_no_parent );
		U_ASSERT( comp.l_index == InversePolishNotationComponent::c_no_parent );

		const IBinaryOperatorsChainComponent& operand= *comp.operand;

		Variable result;

		if( const NamedOperand* const named_operand=
			dynamic_cast<const NamedOperand*>(&operand) )
		{
			const NamesScope::InsertedName* name_entry=
				names.GetName( named_operand->name_ );
			if( !name_entry )
			{
				errors_.push_back( ReportNameNotFound( named_operand->file_pos_, named_operand->name_ ) );
				throw ProgramError();
			}

			if( const Variable* const variable= boost::get<Variable>( &name_entry->second ) )
			{
				result= *variable;
			}
			else if( const OverloadedFunctionsSet* const functins_set=
				boost::get<OverloadedFunctionsSet>( &name_entry->second ) )
			{
				result.type.one_of_type_kind= NontypeStub::OverloadedFunctionsSet;
				result.functions_set= *functins_set;
			}
			else
			{
				// TODO - support typenames, etc.
				errors_.push_back(
					ReportNotImplemented( named_operand->file_pos_, "non variable or functions set name usage" ) );
				throw ProgramError();
			}
		}
		else if( const NumericConstant* numeric_constant=
			dynamic_cast<const NumericConstant*>(&operand) )
		{
			U_FundamentalType type= GetNumericConstantType( *numeric_constant );
			if( type == U_FundamentalType::InvalidType )
			{
				// TODO - report unknown numeric constant type.
				throw ProgramError();
			}
			llvm::Type* const llvm_type= GetFundamentalLLVMType( type );

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type.one_of_type_kind= FundamentalType( type, llvm_type );

			if( IsInteger( type ) )
				result.llvm_value=
					llvm::Constant::getIntegerValue( llvm_type, llvm::APInt( result.type.SizeOf() * 8u, uint64_t(numeric_constant->value_) ) );
			else if( IsFloatingPoint( type ) )
				result.llvm_value=
					llvm::ConstantFP::get( llvm_type, static_cast<double>( numeric_constant->value_) );
			else
			{
				U_ASSERT(false);
			}
		}
		else if( const BooleanConstant* boolean_constant=
			dynamic_cast<const BooleanConstant*>(&operand) )
		{
			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type.one_of_type_kind= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.bool_ );

			result.llvm_value=
				llvm::Constant::getIntegerValue(
					fundamental_llvm_types_.bool_ ,
					llvm::APInt( 1u, uint64_t(boolean_constant->value_) ) );
		}
		else if( const BracketExpression* bracket_expression=
			dynamic_cast<const BracketExpression*>(&operand) )
		{
			result= BuildExpressionCode( *bracket_expression->expression_, names, function_context );
		}
		else
		{
			// TODO
			U_ASSERT(false);
		}

		for( const IUnaryPostfixOperatorPtr& postfix_operator : comp.postfix_operand_operators )
		{
			if( const IndexationOperator* const indexation_operator=
				dynamic_cast<const IndexationOperator*>( postfix_operator.get() ) )
			{
				const ArrayPtr* const array_type= boost::get<ArrayPtr>( &result.type.one_of_type_kind );
				if( array_type == nullptr )
				{
					errors_.push_back( ReportOperationNotSupportedForThisType( indexation_operator->file_pos_, result.type.ToString() ) );
					throw ProgramError();
				}
				U_ASSERT( *array_type != nullptr );

				Variable index=
					BuildExpressionCode(
						*indexation_operator->index_,
						names,
						function_context );

				const FundamentalType* const index_fundamental_type= boost::get<FundamentalType>( &index.type.one_of_type_kind );
				if( index_fundamental_type == nullptr ||
					!IsUnsignedInteger( index_fundamental_type->fundamental_type ) )
				{
					errors_.push_back( ReportTypesMismatch( indexation_operator->file_pos_, "any unsigned integer"_SpC, index.type.ToString() ) );
					throw ProgramError();
				}

				if( result.location != Variable::Location::Pointer )
				{
					// TODO - Strange variable location.
					throw ProgramError();
				}

				result.location= Variable::Location::Pointer;
				result.value_type= ValueType::Reference;
				result.type= (*array_type)->type;

				// Make first index = 0 for array to pointer conversion.
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= CreateMoveToLLVMRegisterInstruction( index, function_context );

				result.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( result.llvm_value, llvm::ArrayRef< llvm::Value*> ( index_list, 2u ) );
			}
			else if( const MemberAccessOperator* const member_access_operator=
				dynamic_cast<const MemberAccessOperator*>( postfix_operator.get() ) )
			{
				const ClassPtr* const class_type= boost::get<ClassPtr>( &result.type.one_of_type_kind );
				if( class_type == nullptr )
				{
					errors_.push_back( ReportOperationNotSupportedForThisType( member_access_operator->file_pos_, result.type.ToString() ) );
					throw ProgramError();
				}
				U_ASSERT( *class_type != nullptr );

				const Class::Field* field= (*class_type)->GetField( member_access_operator->member_name_ );
				if( field == nullptr )
				{
					errors_.push_back( ReportNameNotFound( member_access_operator->file_pos_, member_access_operator->member_name_ ) );
					throw ProgramError();
				}

				U_ASSERT( result.location == Variable::Location::Pointer );

				// Make first index = 0 for array to pointer conversion.
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );

				result.location= Variable::Location::Pointer;
				result.value_type= ValueType::Reference;
				result.type= field->type;
				result.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( result.llvm_value, index_list );
			}
			else if( const CallOperator* const call_operator=
				dynamic_cast<const CallOperator*>( postfix_operator.get() ) )
			{
				const NontypeStub* nontype_stub= boost::get<NontypeStub>( &result.type.one_of_type_kind );
				if( nontype_stub == nullptr || *nontype_stub != NontypeStub::OverloadedFunctionsSet )
				{
					// TODO - Call of non-function.
					throw ProgramError();
				}
				const OverloadedFunctionsSet& functions_set= result.functions_set;

				std::vector<Function::Arg> actual_args( call_operator->arguments_.size() );
				std::vector<Variable> actual_args_variables( call_operator->arguments_.size() );
				for( unsigned int i= 0u; i < actual_args.size(); i++ )
				{

					Variable expr= BuildExpressionCode( *call_operator->arguments_[i], names, function_context );
					actual_args[i].type= expr.type;
					actual_args[i].is_reference= expr.value_type != ValueType::Value;
					actual_args[i].is_mutable= expr.value_type == ValueType::Reference;

					actual_args_variables[i]= std::move(expr);
				}

				const Variable function= GetOverloadedFunction( functions_set, actual_args, call_operator->file_pos_ );
				const Function& function_type= *boost::get<FunctionPtr>( function.type.one_of_type_kind );

				if( function_type.args.size() != actual_args.size( ))
				{
					errors_.push_back( ReportFunctionSignatureMismatch( call_operator->file_pos_ ) );
					throw ProgramError();
				}

				std::vector<llvm::Value*> llvm_args( actual_args.size() );
				for( unsigned int i= 0u; i < actual_args.size(); i++ )
				{
					const Function::Arg& arg= function_type.args[i];
					const Variable& expr= actual_args_variables[i];

					if( expr.type != arg.type )
					{
						errors_.push_back( ReportFunctionSignatureMismatch( call_operator->arguments_[i]->file_pos_ ) );
						throw ProgramError();
					}

					if( arg.is_reference )
					{
						if( arg.is_mutable )
						{
							if( expr.value_type == ValueType::Value )
							{
								errors_.push_back( ReportExpectedReferenceValue( call_operator->arguments_[i]->file_pos_ ) );
								throw ProgramError();
							}
							if( expr.value_type == ValueType::ConstReference )
							{
								errors_.push_back( ReportBindingConstReferenceToNonconstReference( call_operator->arguments_[i]->file_pos_ ) );
								throw ProgramError();
							}

							llvm_args[i]= expr.llvm_value;
						}
						else
						{
							if( expr.value_type == ValueType::Value )
							{
								// Bind value to const reference.
								// TODO - support nonfundamental values.
								llvm::Value* temp_storage= function_context.llvm_ir_builder.CreateAlloca( expr.type.GetLLVMType() );
								function_context.llvm_ir_builder.CreateStore( expr.llvm_value, temp_storage );
								llvm_args[i]= temp_storage;
							}
							else
							{
								llvm_args[i]= expr.llvm_value;
							}
						}
					}
					else
					{
						// TODO - support nonfundamental value-parameters.
						llvm_args[i]= CreateMoveToLLVMRegisterInstruction( expr, function_context );
					}
				}

				llvm::Value* call_result=
					function_context.llvm_ir_builder.CreateCall(
						llvm::dyn_cast<llvm::Function>(function.llvm_value),
						llvm_args );

				if( function_type.return_value_is_reference )
				{
					result.location= Variable::Location::Pointer;
					if( function_type.return_value_is_mutable )
						result.value_type= ValueType::Reference;
					else
						result.value_type= ValueType::ConstReference;
				}
				else
				{
					result.location= Variable::Location::LLVMRegister;
					result.value_type= ValueType::Value;
				}
				result.type= function_type.return_type;
				result.llvm_value= call_result;
			}
			else
			{
				//TODO
				U_ASSERT(false);
			}

		} // for unary postfix operators

		for( const IUnaryPrefixOperatorPtr& prefix_operator : comp.prefix_operand_operators )
		{
			if( const UnaryMinus* const unary_minus=
				dynamic_cast<const UnaryMinus*>( prefix_operator.get() ) )
			{
				(void)unary_minus;

				const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &result.type.one_of_type_kind );
				if( fundamental_type == nullptr )
				{
					errors_.push_back( ReportOperationNotSupportedForThisType( unary_minus->file_pos_, result.type.ToString() ) );
					throw ProgramError();
				}
				const bool is_float= IsFloatingPoint( fundamental_type->fundamental_type );
				if( !( IsInteger( fundamental_type->fundamental_type ) || is_float ) )
				{
					errors_.push_back( ReportOperationNotSupportedForThisType( unary_minus->file_pos_, result.type.ToString() ) );
					throw ProgramError();
				}
				// TODO - maybe not support unary minus for 8 and 16 bot integer types?

				llvm::Value* value_for_neg= CreateMoveToLLVMRegisterInstruction( result, function_context );
				if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFNeg( value_for_neg );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateNeg( value_for_neg );

				result.location= Variable::Location::LLVMRegister;
				result.value_type= ValueType::Value;
			}
			else if( const UnaryPlus* const unary_plus=
				dynamic_cast<const UnaryPlus*>( prefix_operator.get() ) )
			{
				(void)unary_plus;
				// DO NOTHING
			}
			// TODO
		} // for unary prefix operators

		return result;
	}
}

void CodeBuilder::BuildVariablesDeclarationCode(
	const VariablesDeclaration& variables_declaration,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	const Type type= PrepareType( variables_declaration.file_pos_, variables_declaration.type );
	for( const VariablesDeclaration::VariableEntry& variable_declaration : variables_declaration.variables )
	{
		if( IsKeyword( variable_declaration.name ) )
			errors_.push_back( ReportUsingKeywordAsName( variables_declaration.file_pos_ ) );

		Variable variable;
		variable.location= Variable::Location::Pointer;

		// TODO - make variables without explicit mutability modifiers immutable.
		if( variable_declaration.mutability_modifier == MutabilityModifier::Immutable )
			variable.value_type= ValueType::ConstReference;
		else
			variable.value_type= ValueType::Reference;

		variable.type= type;

		if( variable_declaration.reference_modifier == ReferenceModifier::None )
		{
			variable.llvm_value= function_context.llvm_ir_builder.CreateAlloca( variable.type.GetLLVMType() );

			const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &type.one_of_type_kind );
			if( fundamental_type != nullptr )
			{
				if( variable_declaration.initial_value == nullptr )
				{
					errors_.push_back( ReportExpectedInitializer( variables_declaration.file_pos_ ) );
					throw ProgramError();
				}

				const Variable initialzier_expression=
					BuildExpressionCode( *variable_declaration.initial_value, block_names, function_context );

				if( initialzier_expression.type !=variable.type )
				{
					errors_.push_back( ReportTypesMismatch( variables_declaration.file_pos_, variable.type.ToString(), initialzier_expression.type.ToString() ) );
					throw ProgramError();
				}

				llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( initialzier_expression, function_context );
				function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );
			}
			else
			{
				// TODO - support nonfundamental types initialization.
			}
		}
		else if( variable_declaration.reference_modifier == ReferenceModifier::Reference )
		{
			if( variable_declaration.initial_value == nullptr )
			{
				// TODO - report "expected initializer for reference"
				errors_.push_back( ReportExpectedInitializer( variables_declaration.file_pos_ ) );
				throw ProgramError();
			}

			const Variable initialzier_expression=
				BuildExpressionCode( *variable_declaration.initial_value, block_names, function_context );

			if( initialzier_expression.type != variable.type )
			{
				errors_.push_back( ReportTypesMismatch( variables_declaration.file_pos_, variable.type.ToString(), initialzier_expression.type.ToString() ) );
				throw ProgramError();
			}
			if( initialzier_expression.value_type == ValueType::Value )
			{
				errors_.push_back( ReportExpectedReferenceValue( variables_declaration.file_pos_ ) );
				throw ProgramError();
			}
			if( initialzier_expression.value_type == ValueType::ConstReference &&
				variable.value_type == ValueType::Reference )
			{
				errors_.push_back( ReportBindingConstReferenceToNonconstReference( variables_declaration.file_pos_ ) );
				throw ProgramError();
			}

			// TODO - maybe make copy of varaible address in new llvm register?
			variable.llvm_value= initialzier_expression.llvm_value;
		}
		else
		{
			U_ASSERT(false);
		}

		const NamesScope::InsertedName* inserted_name=
			block_names.AddName( variable_declaration.name, std::move(variable) );

		if( !inserted_name )
		{
			errors_.push_back( ReportRedefinition( variables_declaration.file_pos_, variable_declaration.name ) );
			throw ProgramError();
		}
	}
}

void CodeBuilder::BuildAssignmentOperatorCode(
	const AssignmentOperator& assignment_operator,
	const NamesScope& block_names,
	FunctionContext& function_context )
{
	const BinaryOperatorsChain& l_value= *assignment_operator.l_value_;
	const BinaryOperatorsChain& r_value= *assignment_operator.r_value_;

	const Variable l_var= BuildExpressionCode( l_value, block_names, function_context );
	const Variable r_var= BuildExpressionCode( r_value, block_names, function_context );

	if( l_var.value_type != ValueType::Reference )
	{
		errors_.push_back( ReportExpectedReferenceValue( assignment_operator.file_pos_ ) );
		throw ProgramError();
	}
	if( l_var.type != r_var.type )
	{
		errors_.push_back( ReportTypesMismatch( assignment_operator.file_pos_, l_var.type.ToString(), r_var.type.ToString() ) );
		throw ProgramError();
	}

	const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &l_var.type.one_of_type_kind );
	if( fundamental_type != nullptr )
	{
		if( l_var.location != Variable::Location::Pointer )
		{
			// TODO - write correct lvalue/rvalue flag into variable.
			throw ProgramError();
		}
		llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
		function_context.llvm_ir_builder.CreateStore( value_for_assignment, l_var.llvm_value );
	}
	else
	{
		// TODO - functions is not copyable.
		// TODO - arrays not copyable.
		// TODO - make classes copyable.
		errors_.push_back( ReportNotImplemented( assignment_operator.file_pos_, "nonfundamental types assignment." ) );
		throw ProgramError();
	}
}

void CodeBuilder::BuildReturnOperatorCode(
	const ReturnOperator& return_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
	if( return_operator.expression_ == nullptr )
	{
		Type void_type;
		void_type.one_of_type_kind= FundamentalType( U_FundamentalType::Void, fundamental_llvm_types_.void_ );

		if( function_context.return_type != void_type )
		{
			errors_.push_back( ReportTypesMismatch( return_operator.file_pos_, void_type.ToString(), function_context.return_type.ToString() ) );
			return;
		}

		// Add only return instruction for void return operators.
		function_context.llvm_ir_builder.CreateRetVoid();
		return;
	}

	const Variable expression_result=
		BuildExpressionCode(
			*return_operator.expression_,
			names,
			function_context );

	if( expression_result.type != function_context.return_type )
	{
		errors_.push_back( ReportTypesMismatch( return_operator.file_pos_, function_context.return_type.ToString(), expression_result.type.ToString() ) );
		return;
	}
	if( function_context.return_value_is_reference )
	{
		if( expression_result.value_type == ValueType::Value )
		{
			errors_.push_back( ReportExpectedReferenceValue( return_operator.file_pos_ ) );
			throw ProgramError();
		}
		if( expression_result.value_type == ValueType::ConstReference && function_context.return_value_is_mutable )
		{
			errors_.push_back( ReportBindingConstReferenceToNonconstReference( return_operator.file_pos_ ) );
			throw ProgramError();
		}

		function_context.llvm_ir_builder.CreateRet( expression_result.llvm_value );
	}
	else
	{
		llvm::Value* value_for_return= CreateMoveToLLVMRegisterInstruction( expression_result, function_context );
		function_context.llvm_ir_builder.CreateRet( value_for_return );
	}
}

void CodeBuilder::BuildWhileOperatorCode(
	const WhileOperator& while_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
	llvm::BasicBlock* test_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* while_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* block_after_while= llvm::BasicBlock::Create( llvm_context_ );

	// Break to test block. We must push terminal instruction at and of current block.
	function_context.llvm_ir_builder.CreateBr( test_block );

	// Test block code.
	function_context.function->getBasicBlockList().push_back( test_block );
	function_context.llvm_ir_builder.SetInsertPoint( test_block );

	Variable condition_expression= BuildExpressionCode( *while_operator.condition_, names, function_context );

	Type bool_type;
	bool_type.one_of_type_kind= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.void_ );

	if( condition_expression.type != bool_type )
	{
		errors_.push_back(
			ReportTypesMismatch(
				while_operator.condition_->file_pos_,
				bool_type.ToString(),
				condition_expression.type.ToString() ) );
		throw ProgramError();
	}

	llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );

	function_context.llvm_ir_builder.CreateCondBr( condition_in_register, while_block, block_after_while );

	// While block code.

	// Save previous while block break/continue labels.
	// WARNING - between save/restore nobody should throw exceptions!!!
	llvm::BasicBlock* const prev_block_for_break   = function_context.block_for_break   ;
	llvm::BasicBlock* const prev_block_for_continue= function_context.block_for_continue;
	// Set current while block labels.
	function_context.block_for_break   = block_after_while;
	function_context.block_for_continue= test_block;

	function_context.function->getBasicBlockList().push_back( while_block );
	function_context.llvm_ir_builder.SetInsertPoint( while_block );

	BuildBlockCode( *while_operator.block_, names, function_context );
	function_context.llvm_ir_builder.CreateBr( test_block );

	// Restore previous while block break/continue labels.
	function_context.block_for_break   = prev_block_for_break   ;
	function_context.block_for_continue= prev_block_for_continue;

	// Block after while code.
	function_context.function->getBasicBlockList().push_back( block_after_while );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_while );
}

void CodeBuilder::BuildBreakOperatorCode(
	const BreakOperator& break_operator,
	FunctionContext& function_context ) noexcept
{
	if( function_context.block_for_break == nullptr )
	{
		errors_.push_back( ReportBreakOutsideLoop( break_operator.file_pos_ ) );
		return;
	}

	function_context.llvm_ir_builder.CreateBr( function_context.block_for_break );
}

void CodeBuilder::BuildContinueOperatorCode(
	const ContinueOperator& continue_operator,
	FunctionContext& function_context ) noexcept
{
	if( function_context.block_for_continue == nullptr )
	{
		errors_.push_back( ReportContinueOutsideLoop( continue_operator.file_pos_ ) );
		return;
	}

	function_context.llvm_ir_builder.CreateBr( function_context.block_for_continue );
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildIfOperatorCode(
	const IfOperator& if_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
	U_ASSERT( !if_operator.branches_.empty() );

	BlockBuildInfo if_operator_blocks_build_info;
	bool have_return_in_all_branches= true;
	bool have_break_or_continue_in_all_branches= true;

	Type bool_type;
	bool_type.one_of_type_kind= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.void_ );

	// TODO - optimize this method. Make less basic blocks.
	//

	llvm::BasicBlock* block_after_if= llvm::BasicBlock::Create( llvm_context_ );

	llvm::BasicBlock* next_condition_block= llvm::BasicBlock::Create( llvm_context_ );
	// Break to first condition. We must push terminal instruction at end of current block.
	function_context.llvm_ir_builder.CreateBr( next_condition_block );

	for( unsigned int i= 0u; i < if_operator.branches_.size(); i++ )
	{
		const IfOperator::Branch& branch= if_operator.branches_[i];

		llvm::BasicBlock* body_block= llvm::BasicBlock::Create( llvm_context_ );
		llvm::BasicBlock* current_condition_block= next_condition_block;

		if( i + 1u < if_operator.branches_.size() )
			next_condition_block= llvm::BasicBlock::Create( llvm_context_ );
		else
			next_condition_block= block_after_if;

		// Build condition block.
		function_context.function->getBasicBlockList().push_back( current_condition_block );
		function_context.llvm_ir_builder.SetInsertPoint( current_condition_block );

		if( branch.condition == nullptr )
		{
			U_ASSERT( i + 1u == if_operator.branches_.size() );

			// Make empty condition block - move to it unconditional break to body.
			function_context.llvm_ir_builder.CreateBr( body_block );
		}
		else
		{
			Variable condition_expression= BuildExpressionCode( *branch.condition, names, function_context );

			if( condition_expression.type != bool_type )
			{
				errors_.push_back(
					ReportTypesMismatch(
						branch.condition->file_pos_,
						bool_type.ToString(),
						condition_expression.type.ToString() ) );
				throw ProgramError();
			}

			llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );
			function_context.llvm_ir_builder.CreateCondBr( condition_in_register, body_block, next_condition_block );
		}

		// Make body block code.
		function_context.function->getBasicBlockList().push_back( body_block );
		function_context.llvm_ir_builder.SetInsertPoint( body_block );

		const BlockBuildInfo block_build_info=
			BuildBlockCode( *branch.block, names, function_context );

		have_return_in_all_branches= have_return_in_all_branches && block_build_info.have_unconditional_return_inside;
		have_break_or_continue_in_all_branches= have_break_or_continue_in_all_branches && block_build_info.have_uncodnitional_break_or_continue;

		function_context.llvm_ir_builder.CreateBr( block_after_if );
	}

	U_ASSERT( next_condition_block == block_after_if );

	if( if_operator.branches_.back().condition != nullptr )
	{
		have_return_in_all_branches= false;
		have_break_or_continue_in_all_branches= false;
	}

	// Block after if code.
	function_context.function->getBasicBlockList().push_back( block_after_if );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_if );

	if_operator_blocks_build_info.have_unconditional_return_inside= have_return_in_all_branches;
	if_operator_blocks_build_info.have_uncodnitional_break_or_continue= have_break_or_continue_in_all_branches;
	return if_operator_blocks_build_info;
}

void CodeBuilder::ApplyOverloadedFunction(
	OverloadedFunctionsSet& functions_set,
	const Variable& function,
	const FilePos& file_pos )
{
	if( functions_set.empty() )
	{
		functions_set.push_back(function);
		return;
	}

	const FunctionPtr& function_type= boost::get<FunctionPtr>( function.type.one_of_type_kind );
	U_ASSERT(function_type);

	/*
	Algorithm for overloading applying:
	If parameter count differs - overload function.
	If "ArgOverloadingClass" of one or more arguments differs - overload function.
	*/
	for( const Variable& set_function : functions_set )
	{
		const FunctionPtr& set_function_type= boost::get<FunctionPtr>(set_function.type.one_of_type_kind); // Must be function type 100 %
		U_ASSERT(set_function_type);

		// If argument count differs - allow overloading.
		// SPRACHE_TODO - handle default arguments.
		if( function_type->args.size() != set_function_type->args.size() )
			continue;

		unsigned int arg_is_same_count= 0u;
		for( size_t i= 0u; i < function_type->args.size(); i++ )
		{
			const Function::Arg& arg= function_type->args[i];
			const Function::Arg& set_arg= set_function_type->args[i];

			if( arg.type != set_arg.type )
				continue;

			if( GetArgOverloadingClass( arg ) == GetArgOverloadingClass( set_arg ) )
				arg_is_same_count++;
		} // For args.

		if( arg_is_same_count == function_type->args.size() )
		{
			errors_.push_back( ReportCouldNotOverloadFunction(file_pos) );
			throw ProgramError();
		}
	} // For functions in set.

	// No error - add function to set.
	functions_set.push_back(function);
}

const Variable& CodeBuilder::GetOverloadedFunction(
	const OverloadedFunctionsSet& functions_set,
	const std::vector<Function::Arg>& actual_args,
	const FilePos& file_pos )
{
	U_ASSERT( !functions_set.empty() );

	// If we have only one function - return it.
	// Caller can generate error, if arguments does not match.
	if( functions_set.size() == 1u )
		return functions_set.front();


	const Variable* match_function= nullptr;
	for( const Variable& function : functions_set )
	{
		const Function& function_type= *boost::get<FunctionPtr>( function.type.one_of_type_kind );

		// SPRACHE_TODO - handle functions with default arguments.
		if( function_type.args.size() != actual_args.size() )
			continue;

		bool match= true;
		for( unsigned int i= 0u; i < actual_args.size(); i++ )
		{
			// SPRACHE_TODO - support type-casting for function call.
			// SPRACHE_TODO - support references-casting.
			// Now - only exactly compare types.
			if( actual_args[i].type != function_type.args[i].type )
			{
				match= false;
				break;
			}

			if( GetArgOverloadingClass( function_type.args[i] ) == ArgOverloadingClass::MutalbeReference &&
				GetArgOverloadingClass( actual_args[i] ) != ArgOverloadingClass::MutalbeReference )
			{
				// We can only bind nonconst-reference arg to nonconst-reference parameter.
				match= false;
				break;
			}
			else
			{
				// All ok - value or const-reference parameter accept values, const and nonconst references.
			}
		}

		if( match )
		{
			if( match_function == nullptr )
				match_function= &function;
			else
			{
				errors_.push_back( ReportTooManySuitableOverloadedFunctions( file_pos ) );
				throw ProgramError();
			}
		}
	} // for functions

	// Not found any function.
	if( match_function == nullptr )
	{
		errors_.push_back( ReportCouldNotSelectOverloadedFunction( file_pos ) );
		throw ProgramError();
	}
	else
	{
		return *match_function;
	}
}

llvm::Type* CodeBuilder::GetFundamentalLLVMType( const U_FundamentalType fundmantal_type )
{
	switch( fundmantal_type )
	{
	case U_FundamentalType::InvalidType:
		return fundamental_llvm_types_.invalid_type_;
	case U_FundamentalType::LastType:
		break;

	case U_FundamentalType::Void:
		return fundamental_llvm_types_.void_;
	case U_FundamentalType::Bool:
		return fundamental_llvm_types_.bool_;
	case U_FundamentalType::i8 :
		return fundamental_llvm_types_.i8 ;
	case U_FundamentalType::u8 :
		return fundamental_llvm_types_.u8 ;
	case U_FundamentalType::i16:
		return fundamental_llvm_types_.i16;
	case U_FundamentalType::u16:
		return fundamental_llvm_types_.u16;
	case U_FundamentalType::i32:
		return fundamental_llvm_types_.i32;
	case U_FundamentalType::u32:
		return fundamental_llvm_types_.u32;
	case U_FundamentalType::i64:
		return fundamental_llvm_types_.i64;
	case U_FundamentalType::u64:
		return fundamental_llvm_types_.u64;
	case U_FundamentalType::f32:
		return fundamental_llvm_types_.f32;
	case U_FundamentalType::f64:
		return fundamental_llvm_types_.f64;
	};

	U_ASSERT(false);
	return nullptr;
}

llvm::Value*CodeBuilder::CreateMoveToLLVMRegisterInstruction(
	const Variable& variable, FunctionContext& function_context )
{
	switch( variable.location )
	{
	case Variable::Location::LLVMRegister:
		return variable.llvm_value;
	case Variable::Location::Pointer:
		return function_context.llvm_ir_builder.CreateLoad( variable.llvm_value );
	};

	U_ASSERT(false);
	return nullptr;
}

} // namespace CodeBuilderLLVMPrivate

} // namespace U
