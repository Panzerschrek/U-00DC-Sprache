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

CodeBuilder::NamesScope::NamesScope( const NamesScope* prev )
	: prev_(prev)
{
}

void CodeBuilder::NamesScope::AddName( const ProgramString& name, Variable variable )
{
	names_map_.emplace( name, std::move( variable ) );
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
			}

			func_table_.emplace( func->name_, std::move( func_info ) );
		}
		else
		{
			U_ASSERT(false);
		}
	} // for program elements

	return result;
}

void CodeBuilder::BuildBlockCode(
	const Block& block,
	const NamesScope& names )
{
	NamesScope block_names( &names );

	for( const IBlockElementPtr& block_element : block.elements_ )
	{
		if( const VariableDeclaration* variable_declaration=
			dynamic_cast<const VariableDeclaration*>( block_element.get() ) )
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

			// TODO - check redefinition
			block_names.AddName( variable_declaration->name, std::move(variable) );
		}
		else if( const Block* inner_block=
			dynamic_cast<const Block*>( block_element.get() ) )
		{
			BuildBlockCode( *inner_block, block_names );
		}
	}
}

} //namespace Interpreter
