#include <cstring>

#include "assert.hpp"

#include "vm.hpp"

namespace Interpreter
{

template<class T>
struct TypeLinker
{
	static U_FundamentalType GetUFundamentalType()
	{
		return U_FundamentalType::InvalidType;
	}
};

#define LINK_TYPES( type, type_in_enum )\
template<>\
struct TypeLinker<type>\
{\
	static U_FundamentalType GetUFundamentalType()\
	{\
		return U_FundamentalType::type_in_enum;\
	}\
};

LINK_TYPES( U_void , Void )
LINK_TYPES( U_i8 , i8  )
LINK_TYPES( U_u8 , u8  )
LINK_TYPES( U_i16, i16 )
LINK_TYPES( U_u16, u16 )
LINK_TYPES( U_i32, i32 )
LINK_TYPES( U_u32, u32 )
LINK_TYPES( U_i64, i64 )
LINK_TYPES( U_u64, u64 )

#undef LINK_TYPES

template<class T, class ... Args>
bool VM::PushArgs(
	StackFrame& stack,
	const std::vector<U_FundamentalType> params,
	unsigned int n,
	const T& arg0,
	const Args&... args )
{
	U_ASSERT( n > params.size() );
	if( n == params.size() ) return false;

	bool ok= TypeLinker<T>::GetUFundamentalType() == params[n];
	if( ok )
	{
		stack.resize( stack.size() + sizeof(T));
		std::memcpy(
			stack.data() + stack.size() - sizeof(T),
			&arg0,
			sizeof(T) );
	}

	ok|= PushArgs( stack, params, n + 1, args... );

	return ok;
}

inline void VM::Call( const ProgramString& func_name )
{
	auto it= FindExportedFunc( func_name );
	if( it == program_.export_funcs.end() )
	{
		return;
	}

	const FuncEntry& func= *it;
	const VmProgram::FuncCallInfo& call_info= program_.funcs_table[ func.func_number ];

	// Caller stack. Size - for return address and saved previous caller.
	stack_frames_.emplace_back(
		sizeof( unsigned int) + sizeof(unsigned int));

	stack_pointer_= stack_frames_.back().begin();

	OpCallImpl( call_info, 0 );
	OpLoop( call_info.first_op_position );
}

template<class ... Args>
void VM::Call( const ProgramString& func_name, Args&&... args )
{
	auto it= FindExportedFunc( func_name );
	if( it == program_.export_funcs.end() )
	{
		return;
	}

	const FuncEntry& func= *it;

	StackFrame stack;
	PushArgs( stack, func.params, 0, args... );
}

template<class Ret>
void VM::CallRet( const ProgramString& func_name, Ret& result )
{
	auto it= FindExportedFunc( func_name );
	if( it == program_.export_funcs.end() )
	{
		return;
	}

	// TODO - check result type.

	const FuncEntry& func= *it;
	const VmProgram::FuncCallInfo& call_info= program_.funcs_table[ func.func_number ];

	// Caller stack. Size - for return address and saved previous caller + result.
	stack_frames_.emplace_back(
		sizeof( unsigned int) + sizeof(unsigned int) + sizeof(result) );

	stack_pointer_= stack_frames_.back().begin();
	stack_pointer_+= sizeof(result); // Reserve result.

	OpCallImpl( call_info, 0 );
	OpLoop( call_info.first_op_position );

	std::memcpy(
		&result,
		&*stack_pointer_,
		sizeof( result ) );
}

template<class Ret, class ... Args>
void VM::CallRet(
	const ProgramString& func_name,
	Ret& result,
	Args&&... args)
{
	auto it= FindExportedFunc( func_name );
	if( it == program_.export_funcs.end() )
	{
		return;
	}

	const FuncEntry& func= *it;

	StackFrame stack;
	PushArgs( stack, func.params, 0, args... );
}

} // namespace Interpreter
