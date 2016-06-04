#include "inverse_polish_notation.hpp"
#include "keywords.hpp"

#include "code_builder.hpp"

namespace Interpreter
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

const size_t g_fundamental_types_size[ size_t(U_FundamentalType::LastType) ]=
{
	[ size_t(U_FundamentalType::InvalidType) ]= 0,
	[ size_t(U_FundamentalType::Void) ]= 0,
	[ size_t(U_FundamentalType::Bool) ]= sizeof(U_bool),
	[ size_t(U_FundamentalType::i8 ) ]= sizeof(U_i8 ),
	[ size_t(U_FundamentalType::u8 ) ]= sizeof(U_u8 ),
	[ size_t(U_FundamentalType::i16) ]= sizeof(U_i16),
	[ size_t(U_FundamentalType::u16) ]= sizeof(U_u16),
	[ size_t(U_FundamentalType::i32) ]= sizeof(U_i32),
	[ size_t(U_FundamentalType::u32) ]= sizeof(U_u32),
	[ size_t(U_FundamentalType::i64) ]= sizeof(U_i64),
	[ size_t(U_FundamentalType::u64) ]= sizeof(U_u64),
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
	switch( g_fundamental_types_size[ size_t(type) ] )
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
	const VariableDeclaration& variable_declaration )
{
	error_messages.push_back(
		"Variable " + ToStdString( variable_declaration.name ) +
		" has unknown type " + ToStdString( variable_declaration.type ) );
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

CodeBuilder::Type::Type()
{
}

CodeBuilder::Type::Type( const Type& other )
{
	*this= other;
}

CodeBuilder::Type::Type( Type&& other )
{
	*this= std::move(other);
}

CodeBuilder::Type& CodeBuilder::Type::operator=( const Type& other )
{
	kind= other.kind;

	if( kind == Kind::Fundamental )
		fundamental= other.fundamental;
	else
		function.reset( new Function( *other.function ) );

	return *this;
}

CodeBuilder::Type& CodeBuilder::Type::operator=( Type&& other )
{
	kind= other.kind;
	fundamental= other.fundamental;
	function= std::move( other.function );
	return *this;
}

CodeBuilder::NamesScope::NamesScope( const NamesScope* prev )
	: prev_(prev)
{
}

const CodeBuilder::NamesScope::NamesMap::value_type*
	CodeBuilder::NamesScope::AddName(
		const ProgramString& name, Variable variable )
{
	auto it_bool_pair = names_map_.emplace( name, std::move( variable ) );
	if( it_bool_pair.second )
		return &*it_bool_pair.first;

	return nullptr;
}

const CodeBuilder::NamesScope::NamesMap::value_type*
	CodeBuilder::NamesScope::GetName(
		const ProgramString& name ) const
{
	auto it= names_map_.find( name );
	if( it != names_map_.end() )
		return &*it;

	if( prev_ != nullptr )
		return prev_->GetName( name );

	return nullptr;
}

CodeBuilder::BlockStackContext::BlockStackContext()
	: parent_context_(nullptr)
	, stack_size_(0)
	, max_reached_stack_size_(0)
{}

CodeBuilder::BlockStackContext::BlockStackContext( BlockStackContext& parent_context )
	: parent_context_( &parent_context )
	, stack_size_( parent_context.stack_size_ )
	, max_reached_stack_size_( parent_context.stack_size_ )
{}

CodeBuilder::BlockStackContext::~BlockStackContext()
{
	if( parent_context_ )
	{
		parent_context_->max_reached_stack_size_=
			std::max(
				parent_context_->max_reached_stack_size_,
				std::max(
					max_reached_stack_size_,
					stack_size_ ) );
	}
}

void CodeBuilder::BlockStackContext::IncreaseStack( unsigned int size )
{
	stack_size_+= size;
}

unsigned int CodeBuilder::BlockStackContext::GetStackSize() const
{
	return stack_size_;
}

unsigned int CodeBuilder::BlockStackContext::GetMaxReachedStackSize() const
{
	return max_reached_stack_size_;
}

void CodeBuilder::ExpressionStackSizeCounter::operator+=( unsigned int add_size )
{
	size_+= add_size;
	max_reached_size_= std::max( size_, max_reached_size_ );
}

void CodeBuilder::ExpressionStackSizeCounter::operator-=( unsigned int sub_size )
{
	U_ASSERT( sub_size <= size_ );
	size_-= sub_size;
}

unsigned int CodeBuilder::ExpressionStackSizeCounter::GetMaxReachedStackSize() const
{
	return max_reached_size_;
}

unsigned int CodeBuilder::ExpressionStackSizeCounter::GetCurrentStackSize() const
{
	return size_;
}

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
				auto it= g_types_map.find( arg.type );
				if( it == g_types_map.end() )
				{
					error_count_++;
					ReportUnknownVariableType( error_messages_, arg );
					continue;
				}

				Type ret_type;
				ret_type.kind= Type::Kind::Fundamental;
				ret_type.fundamental= it->second;
				func_info.type.function->args.push_back( std::move( ret_type ) );

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

		unsigned int arg_size;
		if( var.type.kind == Type::Kind::Fundamental )
			arg_size= g_fundamental_types_size[ size_t( var.type.fundamental ) ];
		else
			arg_size= sizeof(FuncNumber);

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
	function_context.result_offset=
		args_offset +
		g_fundamental_types_size[ size_t(func.return_type.fundamental) ];
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

				auto it= g_types_map.find( variable_declaration->type );
				if( it == g_types_map.end() )
				{
					ReportUnknownVariableType( error_messages_, *variable_declaration );
					throw ProgramError();
				}
				variable.type.kind= Type::Kind::Fundamental;
				variable.type.fundamental= it->second;
				variable.offset= stack_context.GetStackSize();

				if( variable.type.kind == Type::Kind::Fundamental )
					stack_context.IncreaseStack( g_fundamental_types_size[ size_t( variable.type.fundamental ) ] );
				else
				{
					// TODO
				}

				const NamesScope::NamesMap::value_type* inserted_variable=
					block_names.AddName( variable_declaration->name, std::move(variable) );

				if( !inserted_variable )
				{
					ReportRedefinition( error_messages_, variable_declaration->name );
					throw ProgramError();
				}

				if( variable_declaration->initial_value )
				{
					U_FundamentalType init_type=
						BuildExpressionCode( *variable_declaration->initial_value, block_names, function_context );
					if( init_type != inserted_variable->second.type.fundamental )
					{
						ReportTypesMismatch( error_messages_, init_type, inserted_variable->second.type.fundamental );
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

					function_context.expression_stack_size_counter+=
						g_fundamental_types_size[ size_t(inserted_variable->second.type.fundamental) ];
				}

				Vm_Op init_var_op(
					Vm_Op::Type(
						size_t(Vm_Op::Type::PopToLocalStack8) +
						GetOpIndexOffsetForFundamentalType( inserted_variable->second.type.fundamental ) ) );

				init_var_op.param.local_stack_operations_offset=
					inserted_variable->second.offset;

				result_.code.push_back( init_var_op );

				function_context.expression_stack_size_counter-=
					g_fundamental_types_size[ size_t(inserted_variable->second.type.fundamental) ];
			}
			else if(
				const SingleExpressionOperator* expression=
				dynamic_cast<const SingleExpressionOperator*>( block_element_ptr ) )
			{
				U_FundamentalType result=
					BuildExpressionCode(
						*expression->expression_,
						block_names,
						function_context );

				unsigned int size= g_fundamental_types_size[ size_t(result) ];
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

				const NamedOperand* named_operand=
					dynamic_cast<const NamedOperand*>( l_value.components[0].component.get() );

				if( l_value.components.size() != 1 ||
					!named_operand ||
					!l_value.components[0].prefix_operators.empty() ||
					!l_value.components[0].postfix_operators.empty() )
				{
					error_messages_.push_back( "Can not assign to r_value" );
					throw ProgramError();
				}

				const NamesScope::NamesMap::value_type* variable_entry=
					block_names.GetName( named_operand->name_ );
				if( !variable_entry )
				{
					ReportNameNotFound( error_messages_, named_operand->name_ );
					throw ProgramError();
				}
				const Variable& variable= variable_entry->second;

				U_FundamentalType l_value_type=
					BuildExpressionCode( r_value, block_names, function_context );

				if( l_value_type != variable.type.fundamental )
				{
					ReportTypesMismatch( error_messages_, l_value_type, variable.type.fundamental );
					throw ProgramError();
				}

				Vm_Op op;
				if( variable.location == Variable::Location::FunctionArgument )
				{
					op.type=
						Vm_Op::Type(
							size_t(Vm_Op::Type::PopToCallerStack8) +
							GetOpIndexOffsetForFundamentalType( variable.type.fundamental ) );
					op.param.caller_stack_operations_offset= -int( variable.offset );
				}
				else if( variable.location == Variable::Location::Stack )
				{
					op.type=
						Vm_Op::Type(
							size_t(Vm_Op::Type::PopToLocalStack8) +
							GetOpIndexOffsetForFundamentalType( variable.type.fundamental ) );
					op.param.local_stack_operations_offset=variable.offset;
				}
				else if( variable.location == Variable::Location::Global )
				{
					error_messages_.push_back( "Can not assign to global variable" );
					throw ProgramError();
				}
				else
				{
					U_ASSERT(false);
				}

				result_.code.push_back( op );

				function_context.expression_stack_size_counter-=
					g_fundamental_types_size[ size_t(variable.type.fundamental) ];
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
					U_FundamentalType expression_type=
						BuildExpressionCode(
							*return_operator->expression_,
							block_names,
							function_context );

					if( expression_type != function_context.result_type )
					{
						ReportTypesMismatch( error_messages_, expression_type, function_context.result_type );
						throw ProgramError();
					}

					Vm_Op op{
						Vm_Op::Type(
							size_t(Vm_Op::Type::PopToCallerStack8) +
							GetOpIndexOffsetForFundamentalType( expression_type ) ) };

					op.param.caller_stack_operations_offset= -int( function_context.result_offset );

					result_.code.push_back( op );

					function_context.expression_stack_size_counter-=
						g_fundamental_types_size[ size_t(function_context.result_type) ];
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

U_FundamentalType CodeBuilder::BuildExpressionCode(
	const BinaryOperatorsChain& expression,
	const NamesScope& names,
	FunctionContext& function_context )
{
	std::vector< Type > types_stack;

	InversePolishNotation ipn = ConvertToInversePolishNotation( expression );

	for( const InversePolishNotationComponent& comp : ipn )
	{
		// Operand
		if( comp.operator_ == BinaryOperator::None )
		{
			// HACK. Pass function number not through stack,
			// because Vm operation 'call' takes function number from stack top.
			unsigned int function_number= 0;

			const IBinaryOperatorsChainComponent& operand= *comp.operand;
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
				const Variable& variable= variable_entry->second;

				if( variable.location == Variable::Location::Global &&
					variable.type.function )
					function_number= variable.offset;

				else
				{
					U_ASSERT(!variable.type.function );
					Vm_Op op;

					if( variable.location == Variable::Location::FunctionArgument )
					{
						op.type= Vm_Op::Type::PushFromCallerStack8;
						op.param.caller_stack_operations_offset= -variable.offset;
					}
					else if( variable.location == Variable::Location::Stack )
					{
						op.type= Vm_Op::Type::PushFromLocalStack8;
						op.param.local_stack_operations_offset= variable.offset;
					}
					else
					{
						// Global variables not supported
						U_ASSERT(false);
					}

					op.type= Vm_Op::Type(
						size_t(op.type) +
						GetOpIndexOffsetForFundamentalType(variable.type.fundamental) );
					result_.code.emplace_back(op);

					function_context.expression_stack_size_counter+=
						g_fundamental_types_size[ size_t(variable.type.fundamental) ];
				}

				types_stack.push_back( variable.type );

			}
			else if( const BooleanConstant* boolean_constant=
				dynamic_cast<const BooleanConstant*>(&operand) )
			{
				Vm_Op op{ Vm_Op::Type::PushC8 };
				op.param.push_c_8= boolean_constant->value_;

				result_.code.push_back( op );

				function_context.expression_stack_size_counter+=
					g_fundamental_types_size[ size_t(U_FundamentalType::Bool) ];

				Type type;
				type.kind= Type::Kind::Fundamental;
				type.fundamental= U_FundamentalType::Bool;
				types_stack.push_back( type );
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
					g_fundamental_types_size[ size_t(type) ];

				Type big_type;
				big_type.kind= Type::Kind::Fundamental;
				big_type.fundamental= type;
				types_stack.push_back( big_type );
			}
			else if( const BracketExpression* bracket_expression=
				dynamic_cast<const BracketExpression*>(&operand) )
			{
				Type type;
				type.kind= Type::Kind::Fundamental;
				type.fundamental=
					BuildExpressionCode( *bracket_expression->expression_, names, function_context );

				types_stack.push_back( type );
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
					if( types_stack.back().kind != Type::Kind::Function )
					{
						error_messages_.push_back(
							"Can not call not function" );
						throw ProgramError();
					}

					U_FundamentalType call_type=
						BuildFuncCall(
							*types_stack.back().function,
							function_number,
							*call_operator,
							names,
							function_context );

					// Pop function
					types_stack.pop_back();

					// Push result
					Type type;
					type.kind= Type::Kind::Fundamental;
					type.fundamental= call_type;
					types_stack.push_back( type );
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
					U_ASSERT( !types_stack.empty() );
					if( types_stack.back().kind != Type::Kind::Fundamental )
					{
						error_messages_.push_back(
							"Can not negate function" );
						throw ProgramError();
					}

					U_FundamentalType type= types_stack.back().fundamental;

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
				else
				{
					// Unknown prefix operator
					U_ASSERT(false);
				}

			} // for prefix operators
		}
		else // Operator
		{
			U_ASSERT( types_stack.size() >= 2 );
			U_FundamentalType type0= types_stack.back().fundamental;
			U_FundamentalType type1= types_stack[ types_stack.size() - 2 ].fundamental;

			if( type0 != type1 )
			{
				ReportTypesMismatch( error_messages_, type0, type1 );
				throw ProgramError();
			}

			// Pop operands
			types_stack.resize( types_stack.size() - 2 );

			function_context.expression_stack_size_counter-=
				2 * g_fundamental_types_size[ size_t(type0) ];

			Vm_Op::Type op_type= Vm_Op::Type::NoOp;

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
					else if( type0 == U_FundamentalType::u64 )
						op_type= Vm_Op::Type( size_t(op_type) + 3 );
					else
					{
						ReportArithmeticOperationWithUnsupportedType( error_messages_, type0 );
						throw ProgramError();
					}

					// Result - same as operands
					Type type;
					type.kind= Type::Kind::Fundamental;
					type.fundamental= type0;
					types_stack.push_back( type );

					function_context.expression_stack_size_counter+=
						g_fundamental_types_size[ size_t(type0) ];
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
							std::string( g_fundamental_types_names[ size_t(type0) ] ) );
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
					Type type;
					type.kind= Type::Kind::Fundamental;
					type.fundamental= U_FundamentalType::Bool;
					types_stack.push_back( type );

					function_context.expression_stack_size_counter+=
						g_fundamental_types_size[ size_t(U_FundamentalType::Bool) ];
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
					Type type;
					type.kind= Type::Kind::Fundamental;
					type.fundamental= type0;
					types_stack.push_back( type );

					function_context.expression_stack_size_counter+=
						g_fundamental_types_size[ size_t(type0) ];
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

			result_.code.emplace_back( op_type );

		} // if operator
	} // for inverse polish notation

	U_ASSERT( types_stack.size() == 1 );
	U_ASSERT( types_stack.back().kind == Type::Kind::Fundamental );
	return types_stack.back().fundamental;
}

U_FundamentalType CodeBuilder::BuildFuncCall(
	const Function& func,
	unsigned int func_number,
	const CallOperator& call_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
	U_ASSERT( func.return_type.kind == Type::Kind::Fundamental );

	if( func.args.size() != call_operator.arguments_.size() )
	{
		ReportArgumentsCountMismatch( error_messages_, call_operator.arguments_.size(), func.args.size() );
		throw ProgramError();
	}

	// Reserve place for result
	unsigned int result_size=
		g_fundamental_types_size[ size_t(func.return_type.fundamental) ];

	Vm_Op reserve_result_op( Vm_Op::Type::StackPointerAdd );
	reserve_result_op.param.stack_add_size= result_size;
	result_.code.emplace_back( reserve_result_op );

	function_context.expression_stack_size_counter+= result_size;

	// Push arguments
	unsigned int args_size= 0;
	for( unsigned int i= 0; i < func.args.size(); i++ )
	{
		U_ASSERT( func.args[i].kind == Type::Kind::Fundamental );

		U_FundamentalType expression_type=
			BuildExpressionCode(
				*call_operator.arguments_[i],
				names,
				function_context );

		if( expression_type != func.args[i].fundamental )
		{
			ReportTypesMismatch( error_messages_, func.args[i].fundamental, expression_type );
			throw ProgramError();
		}

		args_size+= g_fundamental_types_size[ size_t(func.args[i].fundamental) ];
	}

	// Push func number
	static_assert(
		32/8 == sizeof(FuncNumber),
		"You need push_c operation appropriate for FuncNumber type" );
	Vm_Op push_func_op( Vm_Op::Type::PushC32 );
	push_func_op.param.push_c_32= func_number;
	result_.code.emplace_back( push_func_op );

	// Call
	result_.code.emplace_back( Vm_Op::Type::Call );

	function_context.expression_stack_size_counter+= sizeof(FuncNumber) + VM::c_saved_caller_frame_size_;

	// Clear args
	Vm_Op clear_args_op( Vm_Op::Type::StackPointerAdd );
	clear_args_op.param.stack_add_size= -int(args_size);
	result_.code.emplace_back( clear_args_op );

	function_context.expression_stack_size_counter-= sizeof(FuncNumber) + VM::c_saved_caller_frame_size_ + args_size;

	return func.return_type.fundamental;
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
			U_FundamentalType condition_type=
				BuildExpressionCode(
					*branch.condition,
					names,
					function_context );

			if( condition_type != U_FundamentalType::Bool )
			{
				ReportTypesMismatch( error_messages_, condition_type, U_FundamentalType::Bool );
				throw ProgramError();
			}

			condition_jump_operations_indeces[i]= result_.code.size();
			result_.code.emplace_back( Vm_Op::Type::JumpIfZero );

			function_context.expression_stack_size_counter-=
				g_fundamental_types_size[ size_t(U_FundamentalType::Bool) ];
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

	U_FundamentalType condition_type=
		BuildExpressionCode(
			*while_operator.condition_,
			names,
			function_context );

	if( condition_type != U_FundamentalType::Bool )
	{
		ReportTypesMismatch( error_messages_, condition_type, U_FundamentalType::Bool );
		throw ProgramError();
	}

	// Exit from loop, if condition is false.
	unsigned int jump_if_condition_is_false_operation_index= result_.code.size();
	result_.code.emplace_back( Vm_Op::Type::JumpIfZero );

	function_context.expression_stack_size_counter-=
		g_fundamental_types_size[ size_t(U_FundamentalType::Bool) ];

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

} //namespace Interpreter
