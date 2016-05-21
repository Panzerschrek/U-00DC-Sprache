#include <cstring>

#include "assert.hpp"

#include "vm.hpp"

namespace Interpreter
{

namespace
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

} // namespace

inline size_t VM::SizeOfArgs()
{
	return 0u;
}

template< class T, class ... Args >
size_t VM::SizeOfArgs( const T& arg0, const Args&... args )
{
	return sizeof(arg0) + SizeOfArgs( args... );
}

template<class T, class ... Args>
bool VM::PushArgs(
	StackFrame::iterator stack,
	const std::vector<U_FundamentalType> params,
	unsigned int n,
	const T& arg0,
	const Args&... args )
{
	U_ASSERT( n <= params.size() );
	if( n == params.size() ) return false;

	bool ok= TypeLinker<T>::GetUFundamentalType() == params[n];
	if( ok )
	{
		std::memcpy(
			&*stack,
			&arg0,
			sizeof(T) );
	}

	ok&= PushArgs( stack + sizeof(T), params, n + 1, args... );

	return ok;
}

inline VM::CallResult VM::Call( const ProgramString& func_name )
{
	CallResult call_result;

	auto it= FindExportedFunc( func_name );
	if( it == program_.export_funcs.end() )
	{
		call_result.error_message= c_func_not_found_;
		return call_result;
	}

	const FuncEntry& func= *it;
	const VmProgram::FuncCallInfo& call_info= program_.funcs_table[ func.func_number ];

	// Caller stack. Size - for return address and saved previous caller.
	stack_frames_.emplace_back(
		sizeof( unsigned int) + sizeof(unsigned int));

	stack_pointer_= stack_frames_.back().begin();

	OpCallImpl( call_info, 0 );
	OpLoop( call_info.first_op_position );

	stack_frames_.pop_back();

	call_result.ok= true;
	return call_result;
}

template<class ... Args>
VM::CallResult VM::Call( const ProgramString& func_name, Args&&... args )
{
	CallResult call_result;

	auto it= FindExportedFunc( func_name );
	if( it == program_.export_funcs.end() )
	{
		call_result.error_message= c_func_not_found_;
		return call_result;
	}

	const FuncEntry& func= *it;
	const VmProgram::FuncCallInfo& call_info= program_.funcs_table[ func.func_number ];

	size_t args_size= SizeOfArgs(args...);

	stack_frames_.emplace_back(
		sizeof( unsigned int) + sizeof(unsigned int)+
		 + args_size );

	bool args_ok= PushArgs( stack_frames_.back().begin(), func.params, 0, args... );
	if( !args_ok )
	{
		call_result.error_message= c_invalid_arguments_type_;
		return call_result;
	}

	stack_pointer_= stack_frames_.back().begin() + args_size;

	OpCallImpl( call_info, 0 );
	OpLoop( call_info.first_op_position );

	stack_frames_.pop_back();

	call_result.ok= true;
	return call_result;
}

template<class Ret>
VM::CallResult VM::CallRet( const ProgramString& func_name, Ret& result )
{
	CallResult call_result;

	auto it= FindExportedFunc( func_name );
	if( it == program_.export_funcs.end() )
	{
		call_result.error_message= c_func_not_found_;
		return call_result;
	}

	const FuncEntry& func= *it;
	const VmProgram::FuncCallInfo& call_info= program_.funcs_table[ func.func_number ];

	if( TypeLinker<Ret>::GetUFundamentalType() != func.return_type )
	{
		call_result.error_message= c_invalid_return_type_;
		return call_result;
	}

	// Caller stack. Size - for return address and saved previous caller + result.
	stack_frames_.emplace_back(
		sizeof( unsigned int) + sizeof(unsigned int) + sizeof(Ret) );

	stack_pointer_= stack_frames_.back().begin();
	stack_pointer_+= sizeof(Ret); // Reserve result.

	OpCallImpl( call_info, 0 );
	OpLoop( call_info.first_op_position );

	std::memcpy(
		&result,
		&*stack_pointer_ - sizeof(Ret),
		sizeof( result ) );

	stack_frames_.pop_back();

	call_result.ok= true;
	return call_result;
}

template<class Ret, class ... Args>
VM::CallResult VM::CallRet(
	const ProgramString& func_name,
	Ret& result,
	Args&&... args)
{
	CallResult call_result;

	auto it= FindExportedFunc( func_name );
	if( it == program_.export_funcs.end() )
	{
		call_result.error_message= c_func_not_found_;
		return call_result;
	}

	const FuncEntry& func= *it;
	const VmProgram::FuncCallInfo& call_info= program_.funcs_table[ func.func_number ];

	if( TypeLinker<Ret>::GetUFundamentalType() != func.return_type )
	{
		call_result.error_message= c_invalid_return_type_;
		return call_result;
	}

	size_t args_and_result_size= sizeof(Ret) + SizeOfArgs(args...);

	stack_frames_.emplace_back(
		sizeof( unsigned int) + sizeof(unsigned int)+
		 + args_and_result_size );

	bool args_ok= PushArgs( stack_frames_.back().begin() + sizeof(Ret), func.params, 0, args... );
	if( !args_ok )
	{
		call_result.error_message= c_invalid_arguments_type_;
		return call_result;
	}

	stack_pointer_= stack_frames_.back().begin() + args_and_result_size;

	OpCallImpl( call_info, 0 );
	OpLoop( call_info.first_op_position );

	std::memcpy(
		&result,
		&*stack_pointer_ - args_and_result_size,
		sizeof( result ) );

	stack_frames_.pop_back();

	call_result.ok= true;
	return call_result;
}

} // namespace Interpreter
