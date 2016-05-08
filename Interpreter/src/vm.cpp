#include <algorithm>

#include "vm.hpp"

namespace Interpreter
{

const VM::VMOpPoiter VM::operations_[size_t( Vm_Op::Type::LastOp ) ]=
{
	[ size_t(Vm_Op::Type::NoOp) ]= &VM::OpNoOp,
	[ size_t(Vm_Op::Type::Call) ]= &VM::OpCall,
	[ size_t(Vm_Op::Type::Ret)]= &VM::OpRet,

	[ size_t(Vm_Op::Type::Syscall)]= nullptr,

	[ size_t(Vm_Op::Type::PushC8 )]= &VM::OpPushC8 ,
	[ size_t(Vm_Op::Type::PushC16)]= &VM::OpPushC16,
	[ size_t(Vm_Op::Type::PushC32)]= nullptr,
	[ size_t(Vm_Op::Type::PushC64)]= nullptr,

	[ size_t(Vm_Op::Type::PushFromLocalStack8 )]= nullptr,
	[ size_t(Vm_Op::Type::PushFromLocalStack16)]= nullptr,
	[ size_t(Vm_Op::Type::PushFromLocalStack32)]= nullptr,
	[ size_t(Vm_Op::Type::PushFromLocalStack64)]= nullptr,

	[ size_t(Vm_Op::Type::PopToLocalStack8 )]= nullptr,
	[ size_t(Vm_Op::Type::PopToLocalStack16)]= nullptr,
	[ size_t(Vm_Op::Type::PopToLocalStack32)]= nullptr,
	[ size_t(Vm_Op::Type::PopToLocalStack64)]= nullptr,

	[ size_t(Vm_Op::Type::PushFromCallerStack8 )]= nullptr,
	[ size_t(Vm_Op::Type::PushFromCallerStack16)]= nullptr,
	[ size_t(Vm_Op::Type::PushFromCallerStack32)]= nullptr,
	[ size_t(Vm_Op::Type::PushFromCallerStack64)]= nullptr,

	[ size_t(Vm_Op::Type::PopToCallerStack8 )]= &VM::OpPopToCallerStack8 ,
	[ size_t(Vm_Op::Type::PopToCallerStack16)]= &VM::OpPopToCallerStack16,
	[ size_t(Vm_Op::Type::PopToCallerStack32)]= nullptr,
	[ size_t(Vm_Op::Type::PopToCallerStack64)]= nullptr,

};

VM::VM( VmProgram program )
	: program_( std::move( program ) )
{
	U_ASSERT( std::is_sorted( program_.import_funcs.begin(), program_.import_funcs.end() ) );
	U_ASSERT( std::is_sorted( program_.export_funcs.begin(), program_.export_funcs.end() ) );

	U_ASSERT(
		program_.code.size() > 0 &&
		program_.code.front().type == Vm_Op::Type::NoOp );
}

Funcs::iterator VM::FindExportedFunc( const ProgramString& name )
{
	// Perform binary search.
	// Warning, program_.export_funcs must be sorted.

	auto it= std::lower_bound(
		program_.export_funcs.begin(),
		program_.export_funcs.end(),
		name,
		[]( const FuncEntry& func, const ProgramString& name ) -> bool
		{
			return func.name < name;
		});

	return it;
}

bool VM::PushArgs(
	StackFrame& stack,
	const std::vector<U_FundamentalType> params,
	unsigned int n)
{
	U_UNUSED(stack);

	return n == params.size();
}

void VM::OpLoop( unsigned int start_op_index )
{
	unsigned int next_op_index= start_op_index;

	while( next_op_index != 0 )
	{
		const Vm_Op& op= program_.code[ next_op_index ];
		auto func= operations_[ size_t(op.type) ];
		next_op_index= (this->*func)( next_op_index );
	}
}

void VM::OpCallImpl( const VmProgram::FuncCallInfo& call_info, unsigned int return_index )
{
	/* Stack frame of caller is.
	Arguments.
	Result.
	Saved caller caller stack pointer.
	Return operation index.
	*/

	// Save caller stack pointer of this function.
	std::memcpy( &*stack_pointer_, &caller_frame_pos_, sizeof(caller_frame_pos_) );
	stack_pointer_+= sizeof(unsigned int);

	// Push return address to stack.
	std::memcpy( &*stack_pointer_, &return_index, sizeof(unsigned int) );
	stack_pointer_+= sizeof(unsigned int);

	// Now, caller stack pointer is stack pointer of this function.
	caller_frame_pos_= stack_pointer_;

	// Make stack for ne function
	stack_frames_.emplace_back( call_info.stack_frame_size );
	stack_pointer_= stack_frames_.back().begin();
}

unsigned int VM::OpNoOp( unsigned int op_index )
{
	return op_index + 1;
}

unsigned int VM::OpCall( unsigned int op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	const VmProgram::FuncCallInfo& call_info=
		program_.funcs_table[ op.param.call_param.func_number ];

	OpCallImpl( call_info, op_index + 1 );

	return call_info.first_op_position;
}

unsigned int VM::OpRet( unsigned int op_index )
{
	U_UNUSED(op_index);

	// Destroy stack.
	stack_frames_.pop_back();

	// Pop return address.
	unsigned int ret_op_index;
	caller_frame_pos_-= sizeof(unsigned int);
	std::memcpy(
		&ret_op_index,
		&*caller_frame_pos_,
		sizeof(unsigned int) );

	// Pop old caller fram pos.
	StackFrame::iterator old_caller_frame_pos;
	caller_frame_pos_-= sizeof(unsigned int);
	std::memcpy(
		&old_caller_frame_pos,
		&*caller_frame_pos_,
		sizeof(stack_pointer_) );

	// Restore all stack pointers.
	stack_pointer_= caller_frame_pos_;
	caller_frame_pos_= old_caller_frame_pos;

	return ret_op_index;
}

unsigned int VM::OpSysCall( unsigned int op_index )
{
	// TODO
	return 0;
}

unsigned int VM::OpPushC8( unsigned int op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	std::memcpy(
		&*stack_pointer_,
		&op.param.push_c_8,
		sizeof(U_i8) );
	stack_pointer_+= sizeof(U_i8);

	return op_index + 1;
}

unsigned int VM::OpPushC16( unsigned int op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	std::memcpy(
		&*stack_pointer_,
		&op.param.push_c_16,
		sizeof(U_i16) );
	stack_pointer_+= sizeof(U_i16);

	return op_index + 1;
}

unsigned int VM::OpPopToCallerStack8 ( unsigned int op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	stack_pointer_-= sizeof(U_i8);
	std::memcpy(
		(&*caller_frame_pos_) + op.param.caller_stack_operations_offset,
		&*stack_pointer_,
		sizeof(U_i8) );

	return op_index + 1;
}

unsigned int VM::OpPopToCallerStack16( unsigned int op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	stack_pointer_-= sizeof(U_i16);
	std::memcpy(
		(&*caller_frame_pos_) + op.param.caller_stack_operations_offset,
		&*stack_pointer_,
		sizeof(U_i16) );

	return op_index + 1;
}

} // namespace Interpreter
