#include "inverse_polish_notation.hpp"

#include "code_builder.hpp"

namespace Interpreter
{

namespace
{

typedef std::map< ProgramString, U_FundamentalType > TypesMap;

const TypesMap g_types_map=
{
	{ ToProgramString( "void" ), U_FundamentalType::Void },
	{ ToProgramString( "i8"  ), U_FundamentalType::i8  },
	{ ToProgramString( "u8"  ), U_FundamentalType::u8  },
	{ ToProgramString( "i16" ), U_FundamentalType::i16 },
	{ ToProgramString( "u16" ), U_FundamentalType::u16 },
	{ ToProgramString( "i32" ), U_FundamentalType::i32 },
	{ ToProgramString( "u32" ), U_FundamentalType::u32 },
	{ ToProgramString( "i64" ), U_FundamentalType::i64 },
	{ ToProgramString( "u64" ), U_FundamentalType::u64 },
};

const size_t g_fundamental_types_size[ size_t(U_FundamentalType::LastType) ]=
{
	[ size_t(U_FundamentalType::InvalidType) ]= 0,
	[ size_t(U_FundamentalType::Void) ]= 0,
	[ size_t(U_FundamentalType::i8 ) ]= 1,
	[ size_t(U_FundamentalType::u8 ) ]= 1,
	[ size_t(U_FundamentalType::i16) ]= 2,
	[ size_t(U_FundamentalType::u16) ]= 2,
	[ size_t(U_FundamentalType::i32) ]= 4,
	[ size_t(U_FundamentalType::u32) ]= 4,
	[ size_t(U_FundamentalType::i64) ]= 8,
	[ size_t(U_FundamentalType::u64) ]= 8,
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

CodeBuilder::CodeBuilder()
{
}

CodeBuilder::~CodeBuilder()
{
}

CodeBuilder::BuildResult CodeBuilder::BuildProgram(
	const ProgramElements& program_elements )
{
	BuildResult result;

	for( const IProgramElementPtr& program_element : program_elements )
	{
		if( const FunctionDeclaration* func=
			dynamic_cast<const FunctionDeclaration*>( program_element.get() ) )
		{
			Variable func_info;

			func_info.location= Variable::Location::Global;
			func_info.offset= ++next_func_number_;

			func_info.type.kind= Type::Kind::Function;

			// Return type.
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
					ReportUnknownFuncReturnType( result.error_messages, *func );
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
					ReportUnknownVariableType( result.error_messages, arg );
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
			}
			else
			{
				// TODO - register error
			}
		}
		else
		{
			U_ASSERT(false);
		}
	} // for program elements

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
			arg_size= sizeof(unsigned int);

		args_offset+= arg_size;
		var.offset= args_offset;

		// TODO - check redefenition
		block_names.AddName(
			arg_names[ arg_n ],
			std::move(var) );
	}

	VmProgram::FuncCallInfo func_entry;
	func_entry.first_op_position= result_.code.size();

	// First instruction - stack increasing.
	result_.code.emplace_back( Vm_Op::Type::StackPointerAdd );

	unsigned int locals_offset= 0;
	unsigned int needed_stack_size;
	BuildBlockCode( block, block_names, locals_offset, needed_stack_size );

	// Stack extension instruction - move stack for expression evaluation above local variables.
	// TODO - add space for expression evaluation.
	result_.code[ func_entry.first_op_position ].param.stack_add_size= needed_stack_size;

	func_entry.stack_frame_size= needed_stack_size;
	result_.funcs_table.emplace_back( std::move( func_entry ) );
}

void CodeBuilder::BuildBlockCode(
	const Block& block,
	const NamesScope& names,
	unsigned int locals_stack_offset,
	unsigned int& out_locals_stack_offset )
{
	NamesScope block_names( &names );

	unsigned int max_inner_block_stack_offset= 0;

	for( const IBlockElementPtr& block_element : block.elements_ )
	{
		const IBlockElement* const block_element_ptr= block_element.get();

		if( const VariableDeclaration* variable_declaration=
			dynamic_cast<const VariableDeclaration*>( block_element_ptr ) )
		{
			Variable variable;
			variable.location= Variable::Location::Stack;

			auto it= g_types_map.find( variable_declaration->type );
			if( it == g_types_map.end() )
			{
				// TODO - register error
				return;
			}
			variable.type.kind= Type::Kind::Fundamental;
			variable.type.fundamental= it->second;
			variable.offset= locals_stack_offset;

			if( variable.type.kind == Type::Kind::Fundamental )
				locals_stack_offset+= g_fundamental_types_size[ size_t( variable.type.fundamental ) ];
			else
			{
				// TODO
			}

			// TODO - check redefinition
			block_names.AddName( variable_declaration->name, std::move(variable) );
		}
		else if( const Block* inner_block=
			dynamic_cast<const Block*>( block_element_ptr ) )
		{
			unsigned int inner_block_stack_offset;
			BuildBlockCode(
				*inner_block,
				block_names,
				locals_stack_offset,
				inner_block_stack_offset);

			max_inner_block_stack_offset=
				std::max(
					max_inner_block_stack_offset,
					inner_block_stack_offset );
		}
		else if( const ReturnOperator* return_operator=
			dynamic_cast<const ReturnOperator*>( block_element_ptr ) )
		{
			if( return_operator->expression_ )
			{
				U_FundamentalType expression_type=
					BuildExpressionCode(
						*return_operator->expression_,
						block_names );

				unsigned int result_size=
					g_fundamental_types_size[ size_t(expression_type) ];

				unsigned int op_shift= ~0u;
				switch( result_size )
				{
					case 0: goto push_ret;

					case 1: op_shift= 0; break;
					case 2: op_shift= 1; break;
					case 4: op_shift= 2; break;
					case 8: op_shift= 3; break;
				};

				U_ASSERT( op_shift != ~0u );

				Vm_Op op;
				op.type= Vm_Op::Type( size_t(Vm_Op::Type::PopToCallerStack8) + op_shift );
				op.param.caller_stack_operations_offset=
					-int( sizeof(unsigned int) + sizeof(unsigned int) + result_size );

				result_.code.push_back( op );
			}

		push_ret:
			Vm_Op ret_op;
			ret_op.type= Vm_Op::Type::Ret;
			result_.code.push_back( ret_op );
		}
	}

	out_locals_stack_offset=
		std::max(
			locals_stack_offset,
			max_inner_block_stack_offset );
}

U_FundamentalType CodeBuilder::BuildExpressionCode(
	const BinaryOperatorsChain& expression,
	const NamesScope& names )
{
	std::vector< Type > types_stack;

	InversePolishNotation ipn = ConvertToInversePolishNotation( expression );

	for( const InversePolishNotationComponent& comp : ipn )
	{
		// Operand
		if( comp.operator_ != BinaryOperator::None )
		{
			const IBinaryOperatorsChainComponent& operand= *comp.operand;
			if( const NamedOperand* named_operand=
				dynamic_cast<const NamedOperand*>(&operand) )
			{
				const NamesScope::NamesMap::value_type* variable_entry=
					names.GetName( named_operand->name_ );
				if( !variable_entry )
				{
					// TODO - register error
				}
				const Variable& variable= variable_entry->second;

				Vm_Op op;

				if( variable.location == Variable::Location::Global &&
					variable.type.function )
				{
					op.type= Vm_Op::Type::PushC32;
					op.param.push_c_32= variable.offset;
				}
				else
				{
					U_ASSERT(! variable.type.function );

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
				}

				types_stack.push_back( variable.type );
				result_.code.emplace_back(op);
			}
			else if( const NumericConstant* number=
				dynamic_cast<const NumericConstant*>(&operand) )
			{
				// Convert str to number
				// push number
			}
			else if( const BracketExpression* bracket_expression=
				dynamic_cast<const BracketExpression*>(&operand) )
			{
				BuildExpressionCode( *bracket_expression->expression_, names );
			}
			else
			{
				U_ASSERT(false);
			}

			for( const IUnaryPostfixOperatorPtr& postfix_operator : comp.postfix_operand_operators )
			{
			}

			for( const IUnaryPrefixOperatorPtr& prefix_operator : comp.prefix_operand_operators )
			{
			}
		}
		else // Operator
		{
			U_ASSERT( types_stack.size() >= 2 );
			U_FundamentalType type0= types_stack.back().fundamental;
			U_FundamentalType type1= types_stack[ types_stack.size() - 2 ].fundamental;

			if( type0 != type1 )
			{
				// TODO -register error
			}

			Vm_Op::Type op_type;

			switch( comp.operator_ )
			{
			case BinaryOperator::Add:
			case BinaryOperator::Sub:
				{
					op_type= comp.operator_ == BinaryOperator::Add ? Vm_Op::Type::Addi32 : Vm_Op::Type::Subi32;

					if( type0 == U_FundamentalType::i32 )
					{}
					else if( type0 == U_FundamentalType::u32 )
						op_type= Vm_Op::Type( size_t(op_type) + 1 );
					else if( type0 == U_FundamentalType::i64 )
						op_type= Vm_Op::Type( size_t(op_type) + 2 );
					else if( type0 == U_FundamentalType::i64 )
						op_type= Vm_Op::Type( size_t(op_type) + 3 );
				}
				break;

			case BinaryOperator::Div:
			case BinaryOperator::Mul:
				op_type= Vm_Op::Type::NoOp;
				// TODO
				break;

			default:
				U_ASSERT(false);
				break;
			};

		} // if operator
	} // for inverse polish notation

	return U_FundamentalType::Void;
}

} //namespace Interpreter
