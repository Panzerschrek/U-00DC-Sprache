#include "inverse_polish_notation.hpp"
#include "keywords.hpp"

#include "code_builder.hpp"

namespace Interpreter
{

namespace CodeBuilderPrivate
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
};

const char* const g_fundamental_types_names[ size_t(U_FundamentalType::LastType) ]=
{
	[ size_t(U_FundamentalType::InvalidType) ]= "InvalidType",
	[ size_t(U_FundamentalType::Void) ]= KeywordAscii( Keywords::void_ ),
	[ size_t(U_FundamentalType::Bool) ]= KeywordAscii( Keywords::bool_ ),
	[ size_t(U_FundamentalType::i8 ) ]= KeywordAscii( Keywords::i8_  ),
	[ size_t(U_FundamentalType::u8 ) ]= KeywordAscii( Keywords::u8_  ),
	[ size_t(U_FundamentalType::i16) ]= KeywordAscii( Keywords::i16_ ),
	[ size_t(U_FundamentalType::u16) ]= KeywordAscii( Keywords::u16_ ),
	[ size_t(U_FundamentalType::i32) ]= KeywordAscii( Keywords::i32_ ),
	[ size_t(U_FundamentalType::u32) ]= KeywordAscii( Keywords::u32_ ),
	[ size_t(U_FundamentalType::i64) ]= KeywordAscii( Keywords::i64_ ),
	[ size_t(U_FundamentalType::u64) ]= KeywordAscii( Keywords::u64_ ),
};

// Returns 0 for 8bit, 1 for 16bit, 2 for 32bit, 3 for 64 bit, 4 - else
unsigned int GetOpIndexOffsetForFundamentalType( U_FundamentalType type )
{
	switch( Type( type ).SizeOf() )
	{
		case 1: return 0;
		case 2: return 1;
		case 4: return 2;
		case 8: return 3;

		default: return 4;
	};
}

static bool IsNumericType( U_FundamentalType type )
{
	return
		type >= U_FundamentalType::i8 &&
		type <= U_FundamentalType::u64;
}

static bool IsUnsignedInteger( U_FundamentalType type )
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

static U_FundamentalType GetNumericConstantType( const NumericConstant& number )
{
	if( number.type_suffix_.empty() )
	{
		if( number.has_fractional_point_ )
		{
			// TODO - floating point types
			return U_FundamentalType::InvalidType;
		}
		else
			return U_FundamentalType::i32;
	}

	auto it= g_types_map.find( number.type_suffix_ );
	if( it == g_types_map.end() )
		return U_FundamentalType::InvalidType;

	return it->second;
}

void ReportUnknownFuncReturnType(
	std::vector<std::string>& error_messages,
	const FunctionDeclaration& func )
{
	error_messages.push_back(
		"Unknown return type " + ToStdString( func.return_type_ ) +
		" for function " + ToStdString( func.name_ ) );
}

void ReportUnknownVariableType(
	std::vector<std::string>& error_messages,
	const TypeName& type_name )
{
	error_messages.push_back(
		"Variable has unknown type " + ToStdString( type_name.name ) );
}

void ReportNameNotFound(
	std::vector<std::string>& error_messages,
	const ProgramString& name )
{
	error_messages.push_back(
		ToStdString( name ) +
		" was not declarated in this scope" );
}

void ReportNotImplemented(
	std::vector<std::string>& error_messages,
	const char* what )
{
	error_messages.push_back(
		std::string("Sorry, ") +
		what +
		" not implemented" );
}

void ReportRedefinition(
	std::vector<std::string>& error_messages,
	const ProgramString& name )
{
	error_messages.push_back(
		ToStdString(name) +
		" redifinition" );
}

void ReportTypesMismatch(
	std::vector<std::string>& error_messages,
	U_FundamentalType type,
	U_FundamentalType expected_type )
{
	error_messages.push_back(
		std::string("Unexpected type: ") +
		g_fundamental_types_names[ size_t(type) ] +
		" expected " +
		g_fundamental_types_names[ size_t(expected_type) ]);
}

void ReportArgumentsCountMismatch(
	std::vector<std::string>& error_messages,
	unsigned int count,
	unsigned int expected )
{
	error_messages.push_back(
		"Arguments count mismatch. actual " +
		std::to_string(count) +
		" expected " +
		std::to_string(expected) );
}

void ReportArithmeticOperationWithUnsupportedType(
	std::vector<std::string>& error_messages,
	U_FundamentalType type )
{
	error_messages.push_back(
		"Expected numeric arguments for arithmetic operators. Supported 32 and 64 bit types. Got " +
		std::string( g_fundamental_types_names[ size_t(type) ] ) );
}

} // namespace

CodeBuilder::CodeBuilder()
{
}

CodeBuilder::~CodeBuilder()
{
}

CodeBuilder::BuildResult CodeBuilder::BuildProgram(
	const ProgramElements& program_elements )
{
	result_.code.emplace_back( Vm_Op::Type::NoOp );

	for( const IProgramElementPtr& program_element : program_elements )
	{
		if( const FunctionDeclaration* func=
			dynamic_cast<const FunctionDeclaration*>( program_element.get() ) )
		{
			Variable func_info;

			func_info.location= Variable::Location::Global;
			func_info.offset= next_func_number_;
			next_func_number_++;

			func_info.type.kind= Type::Kind::Function;

			// Return type.
			func_info.type.function.reset( new Function() );
			func_info.type.function->return_type.kind= Type::Kind::Fundamental;
			if( func->return_type_.empty() )
			{
				func_info.type.function->return_type.fundamental= U_FundamentalType::Void;
			}
			else
			{
				auto it= g_types_map.find( func->return_type_ );
				if( it == g_types_map.end() )
				{
					++error_count_;
					ReportUnknownFuncReturnType( error_messages_, *func );
					func_info.type.function->return_type.fundamental= U_FundamentalType::Void;
				}
				else
					func_info.type.function->return_type.fundamental= it->second;
			}

			// Args.
			std::vector<ProgramString> arg_names;
			arg_names.reserve( func->arguments_.size() );

			func_info.type.function->args.reserve( func->arguments_.size() );
			for( const VariableDeclaration& arg : func->arguments_ )
			{
				func_info.type.function->args.push_back( PrepareType( arg.type ) );
				arg_names.push_back( arg.name );
			}

			const NamesScope::NamesMap::value_type* inserted_func =
				global_names_.AddName( func->name_, std::move( func_info ) );
			if( inserted_func )
			{
				BuildFuncCode(
					*inserted_func->second.type.function,
					arg_names,
					*func->block_ );

				// Push funcs to exports table
				// TODO - do it not for all functions
				FuncEntry func_entry;
				func_entry.name= func->name_;
				func_entry.func_number= inserted_func->second.offset;
				for( const Type& type : inserted_func->second.type.function->args )
				{
					if( type.kind != Type::Kind::Fundamental )
					{
						// TODO - register error
					}
					func_entry.params.emplace_back( type.fundamental );
				}

				if( inserted_func->second.type.function->return_type.kind != Type::Kind::Fundamental )
				{
					// TODO - register error
				}
				func_entry.return_type= inserted_func->second.type.function->return_type.fundamental;

				result_.export_funcs.emplace_back( std::move( func_entry ) );
			}
			else
			{
				error_count_++;
				ReportRedefinition( error_messages_, func->name_ );
				continue;
			}
		}
		else
		{
			U_ASSERT(false);
		}
	} // for program elements

	BuildResult result;
	result.error_messages= std::move( error_messages_ );
	if( error_count_ == 0 )
		result.program= std::move( result_ );

	return result;
}

Type CodeBuilder::PrepareType( const TypeName& type_name )
{
	Type result;
	Type* last_type= &result;

	for( const std::unique_ptr<NumericConstant>& num : type_name.array_sizes )
	{
		last_type->kind= Type::Kind::Array;
		last_type->array.reset( new Array() );

		U_FundamentalType size_type= GetNumericConstantType( *num );
		if( !( IsInteger(size_type) && num->value_ >= 0 ) )
			error_messages_.push_back( "Error, array size must be nonnegative integer" );

		last_type->array->size= size_t(num->value_);

		last_type= &last_type->array->type;
	}

	last_type->kind= Type::Kind::Fundamental;

	auto it= g_types_map.find( type_name.name );
	if( it == g_types_map.end() )
	{
		last_type->fundamental= U_FundamentalType::i32;
		ReportUnknownVariableType( error_messages_, type_name );
	}
	else
		last_type->fundamental= it->second;

	return result;
}

void CodeBuilder::BuildFuncCode(
	const Function& func,
	const std::vector<ProgramString> arg_names,
	const Block& block )
{
	U_ASSERT( arg_names.size() == func.args.size() );

	NamesScope block_names( &global_names_ );

	unsigned int args_offset= 0;
	args_offset+= VM::c_saved_caller_frame_size_;

	unsigned int arg_n= arg_names.size() - 1;
	for( auto it= func.args.rbegin(); it != func.args.rend(); it++, arg_n-- )
	{
		Variable var;
		var.type= *it;
		var.location= Variable::Location::FunctionArgument;

		unsigned int arg_size= var.type.SizeOf();

		args_offset+= arg_size;
		var.offset= args_offset;

		const NamesScope::NamesMap::value_type* inserted_arg=
			block_names.AddName(
				arg_names[ arg_n ],
				std::move(var) );
		if( !inserted_arg )
		{
			error_count_++;
			ReportRedefinition( error_messages_, arg_names[ arg_n ] );
			return;
		}
	}

	FunctionContext function_context;
	function_context.result_offset= args_offset + func.return_type.SizeOf();
	function_context.result_type= func.return_type.fundamental;

	VmProgram::FuncCallInfo func_entry;
	func_entry.first_op_position= result_.code.size();

	// First instruction - stack increasing.
	result_.code.emplace_back( Vm_Op::Type::StackPointerAdd );

	BlockStackContext block_stack_context;
	BuildBlockCode( block, block_names, function_context, block_stack_context );

	U_ASSERT( block_stack_context.GetStackSize() == 0 );

	U_ASSERT(
		error_count_ != 0 ||
		function_context.expression_stack_size_counter.GetCurrentStackSize() == 0 );

	// Stack extension instruction - move stack for expression evaluation above local variables.
	result_.code[ func_entry.first_op_position ].param.stack_add_size=
		block_stack_context.GetMaxReachedStackSize();

	result_.code.emplace_back( Vm_Op::Type::Ret );

	unsigned int needed_stack_size=
		block_stack_context.GetMaxReachedStackSize() +
		function_context.expression_stack_size_counter.GetMaxReachedStackSize();

	func_entry.stack_frame_size= needed_stack_size;
	result_.funcs_table.emplace_back( std::move( func_entry ) );
}

void CodeBuilder::BuildBlockCode(
	const Block& block,
	const NamesScope& names,
	FunctionContext& function_context,
	BlockStackContext stack_context )
{
	NamesScope block_names( &names );

	for( const IBlockElementPtr& block_element : block.elements_ )
	{
		const IBlockElement* const block_element_ptr= block_element.get();

		try
		{
			if( const VariableDeclaration* variable_declaration=
				dynamic_cast<const VariableDeclaration*>( block_element_ptr ) )
			{
				Variable variable;
				variable.location= Variable::Location::Stack;

				variable.type= PrepareType( variable_declaration->type );

				variable.offset= stack_context.GetStackSize();
				stack_context.IncreaseStack( variable.type.SizeOf() );

				const NamesScope::NamesMap::value_type* inserted_variable=
					block_names.AddName( variable_declaration->name, std::move(variable) );

				if( !inserted_variable )
				{
					ReportRedefinition( error_messages_, variable_declaration->name );
					throw ProgramError();
				}

				if( inserted_variable->second.type.kind == Type::Kind::Fundamental )
				{ // Initialize
					if( variable_declaration->initial_value )
					{
						Variable init_type=
							BuildExpressionCode(
								*variable_declaration->initial_value,
								block_names,
								function_context,
								stack_context );

						BuildMoveToStackCode( init_type, function_context );

						if( init_type.type.fundamental != inserted_variable->second.type.fundamental )
						{
							ReportTypesMismatch( error_messages_, init_type.type.fundamental, inserted_variable->second.type.fundamental );
							throw ProgramError();
						}
					} // if variable initializer
					else
					{ // default initialization

						unsigned int op_index=
							GetOpIndexOffsetForFundamentalType( inserted_variable->second.type.fundamental );

						Vm_Op move_zero_op(
							Vm_Op::Type(
								size_t(Vm_Op::Type::PushC8) +
								op_index) );

						if( op_index == 0 )
							move_zero_op.param.push_c_8= 0;
						else if( op_index == 1 )
							move_zero_op.param.push_c_16= 0;
						else if( op_index == 2 )
							move_zero_op.param.push_c_32= 0;
						else if( op_index == 3 )
							move_zero_op.param.push_c_64= 0;
						else
						{
							U_ASSERT(false);
						}

						result_.code.push_back( move_zero_op );

						function_context.expression_stack_size_counter+= inserted_variable->second.type.SizeOf();
					}

					Vm_Op init_var_op(
						Vm_Op::Type(
							size_t(Vm_Op::Type::PopToLocalStack8) +
							GetOpIndexOffsetForFundamentalType( inserted_variable->second.type.fundamental ) ) );

					init_var_op.param.local_stack_operations_offset=
						inserted_variable->second.offset;

					result_.code.push_back( init_var_op );

					function_context.expression_stack_size_counter-=
						inserted_variable->second.type.SizeOf();
				}
			}
			else if(
				const SingleExpressionOperator* expression=
				dynamic_cast<const SingleExpressionOperator*>( block_element_ptr ) )
			{
				Variable result=
					BuildExpressionCode(
						*expression->expression_,
						block_names,
						function_context,
						stack_context );

				BuildMoveToStackCode( result, function_context );

				unsigned int size= result.type.SizeOf();
				if( size > 0 )
				{
					Vm_Op op( Vm_Op::Type::StackPointerAdd );
					op.param.stack_add_size= -int(size);
					result_.code.push_back( op );
				}

				function_context.expression_stack_size_counter-= size;
			}
			else if(
				const AssignmentOperator* assignment_operator=
				dynamic_cast<const AssignmentOperator*>( block_element_ptr ) )
			{
				const BinaryOperatorsChain& l_value= *assignment_operator->l_value_;
				const BinaryOperatorsChain& r_value= *assignment_operator->r_value_;

				U_ASSERT( l_value.components.size() >= 1 );

				Variable l_var=
					BuildExpressionCode( l_value, block_names, function_context, stack_context );

				Variable r_var=
					BuildExpressionCode( r_value, block_names, function_context, stack_context );

				BuildMoveToStackCode( r_var, function_context );

				if( l_var.type != r_var.type )
				{
					ReportTypesMismatch( error_messages_, l_var.type.fundamental, r_var.type.fundamental );
					throw ProgramError();
				}

				Vm_Op op;
				if( l_var.location == Variable::Location::FunctionArgument )
				{
					op.type=
						Vm_Op::Type(
							size_t(Vm_Op::Type::PopToCallerStack8) +
							GetOpIndexOffsetForFundamentalType( l_var.type.fundamental ) );
					op.param.caller_stack_operations_offset= -int( l_var.offset );
				}
				else if( l_var.location == Variable::Location::Stack )
				{
					op.type=
						Vm_Op::Type(
							size_t(Vm_Op::Type::PopToLocalStack8) +
							GetOpIndexOffsetForFundamentalType( l_var.type.fundamental ) );
					op.param.local_stack_operations_offset= l_var.offset;
				}
				else if( l_var.location == Variable::Location::AddressAtExpessionStackTop )
				{
					error_messages_.push_back( "Can not assign to address, not implemented" );
					throw ProgramError();
				}
				else if( l_var.location == Variable::Location::Global )
				{
					error_messages_.push_back( "Can not assign to global variable" );
					throw ProgramError();
				}
				else
				{
					U_ASSERT(false);
				}

				result_.code.push_back( op );

				function_context.expression_stack_size_counter-= l_var.type.SizeOf();
			}
			else if( const Block* inner_block=
				dynamic_cast<const Block*>( block_element_ptr ) )
			{
				BuildBlockCode(
					*inner_block,
					block_names,
					function_context,
					stack_context );
			}
			else if( const ReturnOperator* return_operator=
				dynamic_cast<const ReturnOperator*>( block_element_ptr ) )
			{
				if( return_operator->expression_ )
				{
					Variable expression_result=
						BuildExpressionCode(
							*return_operator->expression_,
							block_names,
							function_context,
							stack_context );

					BuildMoveToStackCode( expression_result, function_context );

					if( expression_result.type.fundamental != function_context.result_type )
					{
						ReportTypesMismatch( error_messages_, expression_result.type.fundamental, function_context.result_type );
						throw ProgramError();
					}

					Vm_Op op{
						Vm_Op::Type(
							size_t(Vm_Op::Type::PopToCallerStack8) +
							GetOpIndexOffsetForFundamentalType( expression_result.type.fundamental ) ) };

					op.param.caller_stack_operations_offset= -int( function_context.result_offset );

					result_.code.push_back( op );

					function_context.expression_stack_size_counter-=
						Type( function_context.result_type ).SizeOf();
				}

				Vm_Op ret_op;
				ret_op.type= Vm_Op::Type::Ret;
				result_.code.push_back( ret_op );
			}
			else if( const IfOperator* if_operator=
				dynamic_cast<const IfOperator*>( block_element_ptr ) )
			{
				BuildIfOperator(
					block_names,
					*if_operator,
					function_context,
					stack_context );
			}
			else if( const WhileOperator* while_operator=
				dynamic_cast<const WhileOperator*>( block_element_ptr ) )
			{
				BuildWhileOperator(
					block_names,
					*while_operator,
					function_context,
					stack_context );
			}
			else if( dynamic_cast<const BreakOperator*>( block_element_ptr ) )
			{
				if( function_context.while_frames.empty() )
				{
					error_messages_.push_back( "break outside while loop" );
					throw ProgramError();
				}

				function_context.while_frames.back().break_operations_indeces.push_back(
					result_.code.size() );

				result_.code.emplace_back( Vm_Op::Type::Jump );
			}
			else if( dynamic_cast<const ContinueOperator*>( block_element_ptr ) )
			{
				if( function_context.while_frames.empty() )
				{
					error_messages_.push_back( "continue outside while loop" );
					throw ProgramError();
				}

				Vm_Op op( Vm_Op::Type::Jump );
				op.param.jump_op_index=
					function_context.while_frames.back().first_while_op_index;

				result_.code.push_back( op );
			}
			else
			{
				U_ASSERT(false);
			}
		} // try
		catch( const ProgramError& )
		{
			error_count_++;
		}
	} // for block elements
}

Variable CodeBuilder::BuildExpressionCode(
	const BinaryOperatorsChain& expression,
	const NamesScope& names,
	FunctionContext& function_context,
	BlockStackContext stack_context,
	bool build_code )
{
	const InversePolishNotation ipn = ConvertToInversePolishNotation( expression );

	return BuildExpressionCode_r(
		ipn, ipn.size() - 1,
		names,
		function_context, stack_context,
		build_code );
}

Variable CodeBuilder::BuildExpressionCode_r(
	const InversePolishNotation& ipn,
	const unsigned int ipn_index,
	const NamesScope& names,
	FunctionContext& function_context,
	BlockStackContext stack_context,
	bool build_code )
{
	U_ASSERT( ipn_index < ipn.size() );
	const InversePolishNotationComponent& comp= ipn[ ipn_index ];

	if( comp.operator_ != BinaryOperator::None )
	{
		Variable l_var=
			BuildExpressionCode_r(
				ipn, comp.l_index,
				names,
				function_context, stack_context,
				false );

		Variable r_var=
			BuildExpressionCode_r(
				ipn, comp.r_index,
				names,
				function_context, stack_context,
				false );

		if( r_var.type != l_var.type )
		{
			ReportTypesMismatch( error_messages_, r_var.type.fundamental, l_var.type.fundamental );
			throw ProgramError();
		}

		if( build_code )
		{
			BuildExpressionCode_r(
				ipn, comp.l_index,
				names,
				function_context, stack_context,
				true );

			BuildMoveToStackCode( l_var, function_context, true );

			BuildExpressionCode_r(
				ipn, comp.r_index,
				names,
				function_context, stack_context,
				true );

			BuildMoveToStackCode( r_var, function_context, true );
		}

		if( l_var.type.kind != Type::Kind::Fundamental ||
			r_var.type.kind != Type::Kind::Fundamental )
		{
			error_messages_.push_back(
				"Expected fundamental arguments for binary operator " );
			throw ProgramError();
		}

		U_FundamentalType type0= l_var.type.fundamental;
		//U_FundamentalType type1= r_var.type.fundamental;

		if( build_code )
			function_context.expression_stack_size_counter-=
				l_var.type.SizeOf() + r_var.type.SizeOf();

		Vm_Op::Type op_type= Vm_Op::Type::NoOp;

		Variable result;
		result.location= Variable::Location::ValueAtExpessionStackTop;

		switch( comp.operator_ )
		{
		case BinaryOperator::Add:
		case BinaryOperator::Sub:
		case BinaryOperator::Div:
		case BinaryOperator::Mul:
			{
				switch( comp.operator_ )
				{
				case BinaryOperator::Add: op_type= Vm_Op::Type::Addi32; break;
				case BinaryOperator::Sub: op_type= Vm_Op::Type::Subi32; break;
				case BinaryOperator::Div: op_type= Vm_Op::Type::Divi32; break;
				case BinaryOperator::Mul: op_type= Vm_Op::Type::Muli32; break;
				default: U_ASSERT(false); break;
				};

				if( type0 == U_FundamentalType::i32 )
				{}
				else if( type0 == U_FundamentalType::u32 )
					op_type= Vm_Op::Type( size_t(op_type) + 1 );
				else if( type0 == U_FundamentalType::i64 )
					op_type= Vm_Op::Type( size_t(op_type) + 2 );
				else if(type0 == U_FundamentalType::u64 )
					op_type= Vm_Op::Type( size_t(op_type) + 3 );
				else
				{
					ReportArithmeticOperationWithUnsupportedType( error_messages_, type0 );
					throw ProgramError();
				}

				// Result - same as operands
				result.type.fundamental= type0;

				if( build_code )
					function_context.expression_stack_size_counter+= result.type.SizeOf();
			}
			break;

		case BinaryOperator::Equal:
		case BinaryOperator::NotEqual:
		case BinaryOperator::Less:
		case BinaryOperator::LessEqual:
		case BinaryOperator::Greater:
		case BinaryOperator::GreaterEqual:
			{
				if( !IsNumericType( type0 ) )
				{
					error_messages_.push_back(
						"Expected numeric arguments for relation operators Got " +
						std::string( g_fundamental_types_names[ size_t( type0 ) ] ) );
					throw ProgramError();
				}

				bool op_needs_sign;
				switch( comp.operator_ )
				{
				case BinaryOperator::Less:
					op_needs_sign= true;
					op_type= Vm_Op::Type::Less8i;
					break;

				case BinaryOperator::LessEqual:
					op_needs_sign= true;
					op_type= Vm_Op::Type::LessEqual8i;
					break;

				case BinaryOperator::Greater:
					op_needs_sign= true;
					op_type= Vm_Op::Type::Greater8i;
					break;

				case BinaryOperator::GreaterEqual:
					op_needs_sign= true;
					op_type= Vm_Op::Type::GreaterEqual8i;
					break;

				case BinaryOperator::Equal:
					op_needs_sign= false;
					op_type= Vm_Op::Type::Equal8;
					break;

				case BinaryOperator::NotEqual:
					op_needs_sign= false;
					op_type= Vm_Op::Type::NotEqual8;
					break;

				default: U_ASSERT(false); break;
				}

				op_type=
					Vm_Op::Type(
						size_t(op_type) +
						GetOpIndexOffsetForFundamentalType( type0 ) );

				if( op_needs_sign && IsUnsignedInteger( type0 ) )
					op_type= Vm_Op::Type( size_t(op_type) + 4 );

				// Result - bool
				result.type.fundamental= U_FundamentalType::Bool;

				if( build_code )
					function_context.expression_stack_size_counter+= result.type.SizeOf();
			}
			break;

		case BinaryOperator::And:
		case BinaryOperator::Or:
		case BinaryOperator::Xor:
			{
				switch( comp.operator_ )
				{
				case BinaryOperator::And: op_type= Vm_Op::Type::And8; break;
				case BinaryOperator::Or: op_type= Vm_Op::Type::Or8; break;
				case BinaryOperator::Xor: op_type= Vm_Op::Type::Xor8; break;
				default: U_ASSERT(false); break;
				};

				op_type=
					Vm_Op::Type(
						size_t(op_type) +
						GetOpIndexOffsetForFundamentalType( type0 ) );

				// Result - same as type0
				result.type.fundamental= type0;

				if( build_code )
					function_context.expression_stack_size_counter+= result.type.SizeOf();
			}
			break;

		case BinaryOperator::LazyLogicalAnd:
		case BinaryOperator::LazyLogicalOr:
			{
				// TODO - lazy operators
				ReportNotImplemented(
					error_messages_,
					"Lazy logical operators" );
				throw ProgramError();
			}
			break;

		case BinaryOperator::None:
		case BinaryOperator::Last:
			U_ASSERT(false);
			break;
		};

		U_ASSERT( op_type != Vm_Op::Type::NoOp );
		if( build_code )
			result_.code.emplace_back( op_type );

		return result;
	}
	else
	{
		U_ASSERT( comp.operand );
		U_ASSERT( comp.r_index == InversePolishNotationComponent::c_no_parent );
		U_ASSERT( comp.l_index == InversePolishNotationComponent::c_no_parent );

		const IBinaryOperatorsChainComponent& operand= *comp.operand;

		Variable result;

		if( const NamedOperand* named_operand=
			dynamic_cast<const NamedOperand*>(&operand) )
		{
			const NamesScope::NamesMap::value_type* variable_entry=
				names.GetName( named_operand->name_ );
			if( !variable_entry )
			{
				ReportNameNotFound( error_messages_, named_operand->name_ );
				throw ProgramError();
			}
			result= variable_entry->second;
		}
		else if( const BooleanConstant* boolean_constant=
			dynamic_cast<const BooleanConstant*>(&operand) )
		{
			if( build_code )
			{
				Vm_Op op{ Vm_Op::Type::PushC8 };
				op.param.push_c_8= boolean_constant->value_;
				result_.code.push_back( op );

				function_context.expression_stack_size_counter+=
					Type( U_FundamentalType::Bool ).SizeOf();
			}

			result.location= Variable::Location::ValueAtExpessionStackTop;
			result.type.kind= Type::Kind::Fundamental;
			result.type.fundamental= U_FundamentalType::Bool;
		}
		else if( const NumericConstant* number=
			dynamic_cast<const NumericConstant*>(&operand) )
		{
			U_FundamentalType type= GetNumericConstantType( *number );
			if( type == U_FundamentalType::InvalidType )
			{
				error_messages_.push_back( "Unknown numeric constant type" );
				throw ProgramError();
			}

			if( build_code )
			{
				Vm_Op op;
				op.type=
					Vm_Op::Type(
						size_t(Vm_Op::Type::PushC8) +
						GetOpIndexOffsetForFundamentalType(type) );

				switch(type)
				{
				case U_FundamentalType::i8 :
					op.param.push_c_8 = static_cast<U_i8 >(number->value_);
					break;
				case U_FundamentalType::u8 :
					op.param.push_c_8 = static_cast<U_u8 >(number->value_);
					break;

				case U_FundamentalType::i16:
					op.param.push_c_16 = static_cast<U_i16>(number->value_);
					break;
				case U_FundamentalType::u16:
					op.param.push_c_16 = static_cast<U_u16>(number->value_);
					break;

				case U_FundamentalType::i32:
					op.param.push_c_32 = static_cast<U_i32>(number->value_);
					break;
				case U_FundamentalType::u32:
					op.param.push_c_32 = static_cast<U_u32>(number->value_);
					break;

				case U_FundamentalType::i64:
					op.param.push_c_64 = static_cast<U_i64>(number->value_);
					break;
				case U_FundamentalType::u64:
					op.param.push_c_64 = static_cast<U_u64>(number->value_);
					break;

				case U_FundamentalType::InvalidType:
				case U_FundamentalType::Void:
				case U_FundamentalType::Bool:
				case U_FundamentalType::LastType:
					U_ASSERT(false);
				};

				result_.code.push_back(op);

				function_context.expression_stack_size_counter+=
					Type( type ).SizeOf();
			}

			result.location= Variable::Location::ValueAtExpessionStackTop;
			result.type.kind= Type::Kind::Fundamental;
			result.type.fundamental= type;
		}
		else if( const BracketExpression* bracket_expression=
			dynamic_cast<const BracketExpression*>(&operand) )
		{
			result=
				BuildExpressionCode(
					*bracket_expression->expression_,
					names,
					function_context, stack_context, build_code );
		}
		else
		{
			U_ASSERT(false);
		}

		for( const IUnaryPostfixOperatorPtr& postfix_operator : comp.postfix_operand_operators )
		{
			if( const CallOperator* call_operator=
				dynamic_cast<const CallOperator*>(postfix_operator.get()) )
			{
				if( result.type.kind != Type::Kind::Function )
				{
					error_messages_.push_back(
						"Can not call not function" );
					throw ProgramError();
				}

				BuildMoveToStackCode( result, function_context, build_code );

				result=
					BuildFuncCall(
						*result.type.function,
						*call_operator,
						names,
						function_context,
						stack_context,
						build_code );
			}
			else if( const IndexationOperator* indexation_operator=
				dynamic_cast<const IndexationOperator*>(postfix_operator.get()) )
			{
				if( result.type.kind != Type::Kind::Array )
				{
					error_messages_.push_back( "Error, indexation for non array" );
					throw ProgramError();
				}

				BuildMoveToStackCode( result, function_context, build_code );

				Variable indexation_result=
					BuildExpressionCode(
						*indexation_operator->index_,
						names,
						function_context,
						stack_context,
						build_code );

				BuildMoveToStackCode( indexation_result, function_context, build_code );

				if( !IsUnsignedInteger( indexation_result.type.fundamental ) )
				{
					error_messages_.push_back( "Error, index must be integer" );
					throw ProgramError();
				}

				if( build_code )
				{
					// Convert index to integer of pointer size
					bool is_64bit= sizeof(void*) == 8;
					switch( indexation_result.type.fundamental )
					{
					case U_FundamentalType::u8:
						result_.code.emplace_back( is_64bit ? Vm_Op::Type::Conv8To64U : Vm_Op::Type::Conv8To32U );
						break;

					case U_FundamentalType::u16:
						result_.code.emplace_back( is_64bit ? Vm_Op::Type::Conv16To64U : Vm_Op::Type::Conv16To32U );
						break;

					case U_FundamentalType::u32:
						if( is_64bit ) result_.code.emplace_back( Vm_Op::Type::Conv32To64U );
						break;

					case U_FundamentalType::u64:
						if( !is_64bit ) result_.code.emplace_back( Vm_Op::Type::Conv64To32 );
						break;

					default:
						U_ASSERT(false);
					};
					function_context.expression_stack_size_counter-= indexation_result.type.SizeOf();
					function_context.expression_stack_size_counter+= sizeof(void*);

					unsigned int element_size= result.type.array->type.SizeOf();
					if( element_size != 1 )
					{
						Vm_Op push_c_op;
						Vm_Op::Type mul_op_type;
						if( is_64bit )
						{
							push_c_op.type= Vm_Op::Type::PushC64;
							push_c_op.param.push_c_64= element_size;
							mul_op_type= Vm_Op::Type::Mulu64;
						}
						else
						{
							push_c_op.type= Vm_Op::Type::PushC32;
							push_c_op.param.push_c_32= element_size;
							mul_op_type= Vm_Op::Type::Mulu32;
						}

						result_.code.push_back( push_c_op );
						result_.code.emplace_back( mul_op_type );

						function_context.expression_stack_size_counter+= sizeof(void*); // push size
						function_context.expression_stack_size_counter-= sizeof(void*); // mul size
					}

					// Add to index to pointer.
					// TODO - check range
					result_.code.emplace_back( is_64bit ? Vm_Op::Type::Addu64 : Vm_Op::Type::Addu32 );
					function_context.expression_stack_size_counter-= sizeof(void*);
				}
				result.type= result.type.array->type;
				result.location= Variable::Location::AddressAtExpessionStackTop;
			}
			else
			{
				// Unknown potfix operator
				U_ASSERT(false);
			}
		} // for postfix operators

		for( const IUnaryPrefixOperatorPtr& prefix_operator : comp.prefix_operand_operators )
		{
			const IUnaryPrefixOperator* const prefix_operator_ptr= prefix_operator.get();
			if( dynamic_cast<const UnaryPlus*>( prefix_operator_ptr ) )
			{}
			else if( dynamic_cast<const UnaryMinus*>( prefix_operator_ptr ) )
			{
				BuildMoveToStackCode( result, function_context, build_code );

				if( result.type.kind != Type::Kind::Fundamental )
				{
					error_messages_.push_back(
						"Can not negate function" );
					throw ProgramError();
				}

				if( build_code )
				{
					U_FundamentalType type= result.type.fundamental;

					Vm_Op::Type op_type= Vm_Op::Type::Negi32;

					if( type == U_FundamentalType::i32 )
					{}
					else if( type == U_FundamentalType::u32 )
						op_type= Vm_Op::Type( size_t(op_type) + 1 );
					else if( type == U_FundamentalType::i64 )
						op_type= Vm_Op::Type( size_t(op_type) + 2 );
					else if( type == U_FundamentalType::u64 )
						op_type= Vm_Op::Type( size_t(op_type) + 3 );
					else
					{
						ReportArithmeticOperationWithUnsupportedType( error_messages_, type );
						throw ProgramError();
					}

					result_.code.emplace_back( op_type );
				}

				result.location= Variable::Location::ValueAtExpessionStackTop;
			}
			else
			{
				// Unknown prefix operator
				U_ASSERT(false);
			}

		} // for prefix operators

		return result;
	} // if operand
}

void CodeBuilder::BuildMoveToStackCode(
	Variable& variable,
	FunctionContext& function_context,
	bool build_code )
{
	if( variable.location == Variable::Location::ValueAtExpessionStackTop )
		return;

	if( build_code )
	{
		if( variable.type.kind == Type::Kind::Array )
		{
			function_context.expression_stack_size_counter+= sizeof(void*);

			Vm_Op op;

			switch( variable.location )
			{
			case Variable::Location::FunctionArgument:
				op.type= Vm_Op::Type::PushCallerStackAddress;
				op.param.caller_stack_operations_offset= -variable.offset;
				break;

			case Variable::Location::Stack:
				op.type= Vm_Op::Type::PushLocalStackAddress;
				op.param.caller_stack_operations_offset= variable.offset;
				break;

			case Variable::Location::Global:
				U_ASSERT( false );
				break;

			case Variable::Location::ValueAtExpessionStackTop:
			case Variable::Location::AddressAtExpessionStackTop:
				U_ASSERT(false);
			};

			result_.code.push_back( op );
		}
		else
		{
			function_context.expression_stack_size_counter+= variable.type.SizeOf();

			Vm_Op op;
			switch( variable.location )
			{
			case Variable::Location::FunctionArgument:
				op.type=
					Vm_Op::Type(
						static_cast<unsigned int>(Vm_Op::Type::PushFromCallerStack8) +
						GetOpIndexOffsetForFundamentalType( variable.type.fundamental ));
				op.param.caller_stack_operations_offset= -variable.offset;
				break;

			case Variable::Location::Stack:
				op.type=
					Vm_Op::Type(
						static_cast<unsigned int>(Vm_Op::Type::PushFromLocalStack8) +
						GetOpIndexOffsetForFundamentalType( variable.type.fundamental ));
				op.param.caller_stack_operations_offset= variable.offset;
				break;

			case Variable::Location::Global:
				U_ASSERT( variable.type.kind == Type::Kind::Function );
				op.type= Vm_Op::Type::PushC32;
				op.param.push_c_32= variable.offset;
				break;

			case Variable::Location::AddressAtExpessionStackTop:
				op.type=
					Vm_Op::Type(
						static_cast<unsigned int>(Vm_Op::Type::Deref8) +
						GetOpIndexOffsetForFundamentalType( variable.type.fundamental ));
				function_context.expression_stack_size_counter-= sizeof(void*);
				break;

			case Variable::Location::ValueAtExpessionStackTop:
				U_ASSERT(false);
			};

			result_.code.push_back( op );
		}
	}

	if( variable.type.kind == Type::Kind::Array )
		variable.location= Variable::Location::AddressAtExpessionStackTop;
	else
		variable.location= Variable::Location::ValueAtExpessionStackTop;
	return;
}

Variable CodeBuilder::BuildFuncCall(
	const Function& func,
	const CallOperator& call_operator,
	const NamesScope& names,
	FunctionContext& function_context,
	BlockStackContext stack_context,
	bool build_code )
{
	U_ASSERT( func.return_type.kind == Type::Kind::Fundamental );

	if( build_code )
	{
		if( func.args.size() != call_operator.arguments_.size() )
		{
			ReportArgumentsCountMismatch( error_messages_, call_operator.arguments_.size(), func.args.size() );
			throw ProgramError();
		}

		unsigned int result_size= func.return_type.SizeOf();

		// Pop function number and move it to temp variable.
		// Do it only if function have any args or returns non void.
		bool need_tmp_var_for_func_number= func.args.size() > 0 || result_size > 0;
		unsigned int tmp_var_func_number_offset= stack_context.GetStackSize();
		if( need_tmp_var_for_func_number )
		{
			stack_context.IncreaseStack( sizeof(FuncNumber) );

			Vm_Op op{ Vm_Op::Type::PopToLocalStack32 };
			op.param.local_stack_operations_offset= tmp_var_func_number_offset;
			result_.code.push_back( op );

			function_context.expression_stack_size_counter-= sizeof(FuncNumber);
		}

		// Reserve place for result
		if( result_size > 0 )
		{
			Vm_Op reserve_result_op( Vm_Op::Type::StackPointerAdd );
			reserve_result_op.param.stack_add_size= result_size;
			result_.code.emplace_back( reserve_result_op );
		}

		function_context.expression_stack_size_counter+= result_size;

		// Push arguments
		unsigned int args_size= 0;
		for( unsigned int i= 0;  i < func.args.size(); i++ )
		{
			U_ASSERT( func.args[i].kind == Type::Kind::Fundamental );

			Variable expression_result=
				BuildExpressionCode(
					*call_operator.arguments_[i],
					names,
					function_context,
					stack_context );

			BuildMoveToStackCode( expression_result, function_context );

			if( expression_result.type.fundamental != func.args[i].fundamental )
			{
				ReportTypesMismatch( error_messages_, func.args[i].fundamental, expression_result.type.fundamental );
				throw ProgramError();
			}

			args_size+= func.args[i].SizeOf();
		}

		// Push func number.
		if( need_tmp_var_for_func_number )
		{
			static_assert(
				32/8 == sizeof(FuncNumber),
				"You need push_c operation appropriate for FuncNumber type" );
			Vm_Op op( Vm_Op::Type::PushFromLocalStack32 );
			op.param.local_stack_operations_offset= tmp_var_func_number_offset;
			result_.code.emplace_back( op );

			function_context.expression_stack_size_counter+= sizeof(FuncNumber);
		}

		// Call
		result_.code.emplace_back( Vm_Op::Type::Call );

		function_context.expression_stack_size_counter+= VM::c_saved_caller_frame_size_;

		// Clear args
		if( args_size > 0 )
		{
			Vm_Op clear_args_op( Vm_Op::Type::StackPointerAdd );
			clear_args_op.param.stack_add_size= -int(args_size);
			result_.code.emplace_back( clear_args_op );
		}

		function_context.expression_stack_size_counter-= sizeof(FuncNumber) + VM::c_saved_caller_frame_size_ + args_size;
	}

	Variable result;
	result.location= Variable::Location::ValueAtExpessionStackTop;
	result.type= func.return_type;
	return result;
}

void CodeBuilder::BuildIfOperator(
	const NamesScope& names,
	const IfOperator& if_operator,
	FunctionContext& function_context,
	BlockStackContext stack_context )
{
	std::vector<unsigned int> condition_jump_operations_indeces( if_operator.branches_.size() );
	std::vector<unsigned int> jump_from_branch_operations_indeces( if_operator.branches_.size() - 1 );

	unsigned int i= 0;
	for( const IfOperator::Branch& branch : if_operator.branches_ )
	{
		U_ASSERT( branch.condition || &branch == &if_operator.branches_.back() );

		// Set jump point for previous condition jump
		if( i != 0 )
			result_.code[ condition_jump_operations_indeces[ i - 1 ] ].param.jump_op_index=
				result_.code.size();

		if( branch.condition )
		{
			Variable condition_type=
				BuildExpressionCode(
					*branch.condition,
					names,
					function_context,
					stack_context );

			BuildMoveToStackCode( condition_type, function_context );

			if( condition_type.type.fundamental != U_FundamentalType::Bool )
			{
				ReportTypesMismatch( error_messages_, condition_type.type.fundamental, U_FundamentalType::Bool );
				throw ProgramError();
			}

			condition_jump_operations_indeces[i]= result_.code.size();
			result_.code.emplace_back( Vm_Op::Type::JumpIfZero );

			function_context.expression_stack_size_counter-=
				Type( U_FundamentalType::Bool ).SizeOf();
		}

		BuildBlockCode(
			*branch.block,
			names,
			function_context,
			stack_context );

		// Jump from block to if-else end. Do not need for last blocks
		if( &branch != &if_operator.branches_.back() )
		{
			jump_from_branch_operations_indeces[i]= result_.code.size();
			result_.code.emplace_back( Vm_Op::Type::JumpIfZero );
		}
		i++;
	}

	unsigned int next_op_index= result_.code.size();

	// Last if "else if" or "if"
	if( if_operator.branches_.back().condition )
	{
		result_.code[ condition_jump_operations_indeces.back() ].param.jump_op_index=
			next_op_index;
	}

	// Set jump point for blocks exit jumps
	for( unsigned int op_index : jump_from_branch_operations_indeces )
		result_.code[ op_index ].param.jump_op_index= next_op_index;
}

void CodeBuilder::BuildWhileOperator(
	const NamesScope& names,
	const WhileOperator& while_operator,
	FunctionContext& function_context,
	BlockStackContext stack_context )
{
	function_context.while_frames.emplace_back();

	function_context.while_frames.back().first_while_op_index= result_.code.size();

	Variable condition_type=
		BuildExpressionCode(
			*while_operator.condition_,
			names,
			function_context,
			stack_context );

	BuildMoveToStackCode( condition_type, function_context );

	if( condition_type.type.fundamental != U_FundamentalType::Bool )
	{
		ReportTypesMismatch( error_messages_, condition_type.type.fundamental, U_FundamentalType::Bool );
		throw ProgramError();
	}

	// Exit from loop, if condition is false.
	unsigned int jump_if_condition_is_false_operation_index= result_.code.size();
	result_.code.emplace_back( Vm_Op::Type::JumpIfZero );

	function_context.expression_stack_size_counter-=
		Type( U_FundamentalType::Bool ).SizeOf();

	BuildBlockCode(
		*while_operator.block_,
		names,
		function_context,
		stack_context );

	// Jump to condition calculation.
	Vm_Op jump_op( Vm_Op::Type::Jump );
	jump_op.param.jump_op_index= function_context.while_frames.back().first_while_op_index;
	result_.code.push_back( jump_op );

	// Setup jump point for conditional jump.
	result_.code[ jump_if_condition_is_false_operation_index ].param.jump_op_index=
		result_.code.size();

	// Setup jump point for all "break".
	for( OpIndex op_index : function_context.while_frames.back().break_operations_indeces )
	{
		Vm_Op& op= result_.code[ op_index ];
		U_ASSERT( op.type == Vm_Op::Type::Jump );
		op.param.jump_op_index= result_.code.size();
	}

	function_context.while_frames.pop_back();
}

} // namespace CodeBuilderPrivate

} //namespace Interpreter
