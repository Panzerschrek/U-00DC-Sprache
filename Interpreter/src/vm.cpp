#include <algorithm>

#include "vm.hpp"

namespace Interpreter
{

// Hack. There is no std::bit_inverse unary function.
template<class T>
struct BitInverse
{
	T operator()( const T& x )
	{
		return ~x;
	}
};

template<class T>
OpIndex VM::PushFromLocalStackOpBase( OpIndex op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	std::memcpy(
		&*stack_pointer_,
		&*stack_frames_.back().begin() + op.param.local_stack_operations_offset,
		sizeof(T) );
	stack_pointer_+= sizeof(T);

	return op_index + 1;
}

template<class T>
OpIndex VM::PopToLocalStackOpBase( OpIndex op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	stack_pointer_-= sizeof(T);
	std::memcpy(
		&*stack_frames_.back().begin() + op.param.caller_stack_operations_offset,
		&*stack_pointer_,
		sizeof(T) );

	return op_index + 1;
}

template<class T>
OpIndex VM::PushFromCallerStackOpBase( OpIndex op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	std::memcpy(
		&*stack_pointer_,
		(&*caller_frame_pos_) + op.param.caller_stack_operations_offset,
		sizeof(T) );
	stack_pointer_+= sizeof(T);

	return op_index + 1;
}

template<class T>
OpIndex VM::PopToCallerStackOpBase( OpIndex op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	stack_pointer_-= sizeof(T);
	std::memcpy(
		(&*caller_frame_pos_) + op.param.caller_stack_operations_offset,
		&*stack_pointer_,
		sizeof(T) );

	return op_index + 1;
}

template<class T, class Func>
OpIndex VM::BinaryOpBase( OpIndex op_index )
{
	T second;
	T first;

	stack_pointer_-= sizeof(T);
	std::memcpy(
		&second,
		&*stack_pointer_,
		sizeof(T) );

	stack_pointer_-= sizeof(T);
	std::memcpy(
		&first,
		&*stack_pointer_,
		sizeof(T) );

	Func func;
	T result= func( first, second );

	std::memcpy(
		&*stack_pointer_,
		&result,
		sizeof(T) );
	stack_pointer_+= sizeof(T);

	return op_index + 1;
}

template<class T, class Func>
OpIndex VM::ComparisonOpBase( OpIndex op_index )
{
	T second;
	T first;

	stack_pointer_-= sizeof(T);
	std::memcpy(
		&second,
		&*stack_pointer_,
		sizeof(T) );

	stack_pointer_-= sizeof(T);
	std::memcpy(
		&first,
		&*stack_pointer_,
		sizeof(T) );

	Func func;
	U_bool result= func( first, second );

	std::memcpy(
		&*stack_pointer_,
		&result,
		sizeof(U_bool) );
	stack_pointer_+= sizeof(U_bool);

	return op_index + 1;
}

template<class T, class Func>
OpIndex VM::UnaryOpBase( OpIndex op_index )
{
	T operand;

	std::memcpy(
		&operand,
		&*stack_pointer_ - sizeof(T),
		sizeof(T) );

	Func func;
	operand= func( operand );

	std::memcpy(
		&*stack_pointer_ - sizeof(T),
		&operand,
		sizeof(T) );

	return op_index + 1;
}

template<class From, class To>
OpIndex VM::ConvertionOpBase( OpIndex op_index )
{
	From from;
	To to;

	stack_pointer_-= sizeof(From);
	std::memcpy(
		&from,
		&*stack_pointer_,
		sizeof(From) );

	to= static_cast<To>(from);

	std::memcpy(
		&*stack_pointer_,
		&to,
		sizeof(To) );
	stack_pointer_+= sizeof(To);

	return op_index + 1;
}

const char* const VM::c_func_not_found_= "Function not found.";
const char* const VM::c_invalid_return_type_= "Invalid return type.";
const char* const VM::c_invalid_arguments_type_= "Invalid argument(s) type.";

const unsigned int VM::c_saved_caller_frame_size_=
	sizeof(VM::StackFrame::iterator) +
	sizeof(OpIndex);

const VM::VMOpPoiter VM::operations_[size_t( Vm_Op::Type::LastOp ) ]=
{
	[ size_t(Vm_Op::Type::NoOp) ]= &VM::OpNoOp,
	[ size_t(Vm_Op::Type::Call) ]= &VM::OpCall,
	[ size_t(Vm_Op::Type::Ret)]= &VM::OpRet,

	[ size_t(Vm_Op::Type::Syscall)]= nullptr,

	[ size_t(Vm_Op::Type::Jump)]= &VM::OpJump,
	[ size_t(Vm_Op::Type::JumpIfZero)]= &VM::OpJumpIfZero,
	[ size_t(Vm_Op::Type::JumpIfNotZero)]= &VM::OpJumpIfNotZero,

	[ size_t(Vm_Op::Type::StackPointerAdd) ]= &VM::OpStackPointerAdd,

	[ size_t(Vm_Op::Type::PushC8 )]= &VM::OpPushC8 ,
	[ size_t(Vm_Op::Type::PushC16)]= &VM::OpPushC16,
	[ size_t(Vm_Op::Type::PushC32)]= &VM::OpPushC32,
	[ size_t(Vm_Op::Type::PushC64)]= &VM::OpPushC64,

	[ size_t(Vm_Op::Type::PushFromLocalStack8 )]= &VM::PushFromLocalStackOpBase<U_u8 >,
	[ size_t(Vm_Op::Type::PushFromLocalStack16)]= &VM::PushFromLocalStackOpBase<U_u16>,
	[ size_t(Vm_Op::Type::PushFromLocalStack32)]= &VM::PushFromLocalStackOpBase<U_u32>,
	[ size_t(Vm_Op::Type::PushFromLocalStack64)]= &VM::PushFromLocalStackOpBase<U_u64>,

	[ size_t(Vm_Op::Type::PopToLocalStack8 )]= &VM::PopToLocalStackOpBase<U_u8 >,
	[ size_t(Vm_Op::Type::PopToLocalStack16)]= &VM::PopToLocalStackOpBase<U_u16>,
	[ size_t(Vm_Op::Type::PopToLocalStack32)]= &VM::PopToLocalStackOpBase<U_u32>,
	[ size_t(Vm_Op::Type::PopToLocalStack64)]= &VM::PopToLocalStackOpBase<U_u64>,

	[ size_t(Vm_Op::Type::PushFromCallerStack8 )]= &VM::PushFromCallerStackOpBase<U_u8 >,
	[ size_t(Vm_Op::Type::PushFromCallerStack16)]= &VM::PushFromCallerStackOpBase<U_u16>,
	[ size_t(Vm_Op::Type::PushFromCallerStack32)]= &VM::PushFromCallerStackOpBase<U_u32>,
	[ size_t(Vm_Op::Type::PushFromCallerStack64)]= &VM::PushFromCallerStackOpBase<U_u64>,

	[ size_t(Vm_Op::Type::PopToCallerStack8 )]= &VM::PopToCallerStackOpBase<U_u8 >,
	[ size_t(Vm_Op::Type::PopToCallerStack16)]= &VM::PopToCallerStackOpBase<U_u16>,
	[ size_t(Vm_Op::Type::PopToCallerStack32)]= &VM::PopToCallerStackOpBase<U_u32>,
	[ size_t(Vm_Op::Type::PopToCallerStack64)]= &VM::PopToCallerStackOpBase<U_u64>,

	[ size_t(Vm_Op::Type::And8 )]= &VM::BinaryOpBase<U_u8 , std::bit_and<U_u8 >>,
	[ size_t(Vm_Op::Type::And16)]= &VM::BinaryOpBase<U_u16, std::bit_and<U_u16>>,
	[ size_t(Vm_Op::Type::And32)]= &VM::BinaryOpBase<U_u32, std::bit_and<U_u32>>,
	[ size_t(Vm_Op::Type::And64)]= &VM::BinaryOpBase<U_u64, std::bit_and<U_u64>>,

	[ size_t(Vm_Op::Type::Or8 )]= &VM::BinaryOpBase<U_u8 , std::bit_or<U_u8 >>,
	[ size_t(Vm_Op::Type::Or16)]= &VM::BinaryOpBase<U_u16, std::bit_or<U_u16>>,
	[ size_t(Vm_Op::Type::Or32)]= &VM::BinaryOpBase<U_u32, std::bit_or<U_u32>>,
	[ size_t(Vm_Op::Type::Or64)]= &VM::BinaryOpBase<U_u64, std::bit_or<U_u64>>,

	[ size_t(Vm_Op::Type::Xor8 )]= &VM::BinaryOpBase<U_u8 , std::bit_xor<U_u8 >>,
	[ size_t(Vm_Op::Type::Xor16)]= &VM::BinaryOpBase<U_u16, std::bit_xor<U_u16>>,
	[ size_t(Vm_Op::Type::Xor32)]= &VM::BinaryOpBase<U_u32, std::bit_xor<U_u32>>,
	[ size_t(Vm_Op::Type::Xor64)]= &VM::BinaryOpBase<U_u64, std::bit_xor<U_u64>>,

	[ size_t(Vm_Op::Type::Not8 )]= &VM::UnaryOpBase<U_u8 , BitInverse<U_u8 >>,
	[ size_t(Vm_Op::Type::Not16)]= &VM::UnaryOpBase<U_u16, BitInverse<U_u16>>,
	[ size_t(Vm_Op::Type::Not32)]= &VM::UnaryOpBase<U_u32, BitInverse<U_u32>>,
	[ size_t(Vm_Op::Type::Not64)]= &VM::UnaryOpBase<U_u64, BitInverse<U_u64>>,

	[ size_t(Vm_Op::Type::Negi32)]= &VM::UnaryOpBase<U_i32, std::negate<U_i32>>,
	[ size_t(Vm_Op::Type::Negu32)]= &VM::UnaryOpBase<U_u32, std::negate<U_u32>>,
	[ size_t(Vm_Op::Type::Negi64)]= &VM::UnaryOpBase<U_i64, std::negate<U_i64>>,
	[ size_t(Vm_Op::Type::Negu64)]= &VM::UnaryOpBase<U_u64, std::negate<U_u64>>,

	[ size_t(Vm_Op::Type::Subi32)]= &VM::BinaryOpBase<U_i32, std::minus<U_i32>>,
	[ size_t(Vm_Op::Type::Subu32)]= &VM::BinaryOpBase<U_u32, std::minus<U_u32>>,
	[ size_t(Vm_Op::Type::Subi64)]= &VM::BinaryOpBase<U_i64, std::minus<U_i64>>,
	[ size_t(Vm_Op::Type::Subu64)]= &VM::BinaryOpBase<U_u64, std::minus<U_u64>>,

	[ size_t(Vm_Op::Type::Addi32)]= &VM::BinaryOpBase<U_i32, std::plus<U_i32>>,
	[ size_t(Vm_Op::Type::Addu32)]= &VM::BinaryOpBase<U_u32, std::plus<U_u32>>,
	[ size_t(Vm_Op::Type::Addi64)]= &VM::BinaryOpBase<U_i64, std::plus<U_i64>>,
	[ size_t(Vm_Op::Type::Addu64)]= &VM::BinaryOpBase<U_u64, std::plus<U_u64>>,

	[ size_t(Vm_Op::Type::Muli32)]= &VM::BinaryOpBase<U_i32, std::multiplies<U_i32>>,
	[ size_t(Vm_Op::Type::Mulu32)]= &VM::BinaryOpBase<U_u32, std::multiplies<U_u32>>,
	[ size_t(Vm_Op::Type::Muli64)]= &VM::BinaryOpBase<U_i64, std::multiplies<U_i64>>,
	[ size_t(Vm_Op::Type::Mulu64)]= &VM::BinaryOpBase<U_u64, std::multiplies<U_u64>>,

	// TODO - check division by zero
	[ size_t(Vm_Op::Type::Divi32)]= &VM::BinaryOpBase<U_i32, std::divides<U_i32>>,
	[ size_t(Vm_Op::Type::Divu32)]= &VM::BinaryOpBase<U_u32, std::divides<U_u32>>,
	[ size_t(Vm_Op::Type::Divi64)]= &VM::BinaryOpBase<U_i64, std::divides<U_i64>>,
	[ size_t(Vm_Op::Type::Divu64)]= &VM::BinaryOpBase<U_u64, std::divides<U_u64>>,

	[ size_t(Vm_Op::Type::Conv8To16S)]= &VM::ConvertionOpBase<U_i8, U_i16>,
	[ size_t(Vm_Op::Type::Conv8To32S)]= &VM::ConvertionOpBase<U_i8, U_i32>,
	[ size_t(Vm_Op::Type::Conv8To64S)]= &VM::ConvertionOpBase<U_i8, U_i64>,

	[ size_t(Vm_Op::Type::Conv8To16U)]= &VM::ConvertionOpBase<U_u8, U_u16>,
	[ size_t(Vm_Op::Type::Conv8To32U)]= &VM::ConvertionOpBase<U_u8, U_u32>,
	[ size_t(Vm_Op::Type::Conv8To64U)]= &VM::ConvertionOpBase<U_u8, U_u64>,

	[ size_t(Vm_Op::Type::Conv16To32S)]= &VM::ConvertionOpBase<U_i16, U_i32>,
	[ size_t(Vm_Op::Type::Conv16To64S)]= &VM::ConvertionOpBase<U_i16, U_i64>,

	[ size_t(Vm_Op::Type::Conv16To32U)]= &VM::ConvertionOpBase<U_u16, U_u32>,
	[ size_t(Vm_Op::Type::Conv16To64U)]= &VM::ConvertionOpBase<U_u16, U_u64>,

	[ size_t(Vm_Op::Type::Conv32To64S)]= &VM::ConvertionOpBase<U_i32, U_i64>,
	[ size_t(Vm_Op::Type::Conv32To64U)]= &VM::ConvertionOpBase<U_u32, U_u64>,

	[ size_t(Vm_Op::Type::Conv64To32)]= &VM::ConvertionOpBase<U_u64, U_u32>,
	[ size_t(Vm_Op::Type::Conv64To16)]= &VM::ConvertionOpBase<U_u64, U_u16>,
	[ size_t(Vm_Op::Type::Conv64To8 )]= &VM::ConvertionOpBase<U_u64, U_u8 >,

	[ size_t(Vm_Op::Type::Conv32To16)]= &VM::ConvertionOpBase<U_u32, U_u16>,
	[ size_t(Vm_Op::Type::Conv32To8 )]= &VM::ConvertionOpBase<U_u32, U_u8 >,

	[ size_t(Vm_Op::Type::Conv16To8 )]= &VM::ConvertionOpBase<U_u16, U_u8 >,

	[ size_t(Vm_Op::Type::Equal8  )]= &VM::ComparisonOpBase<U_i8 , std::equal_to<U_i8 >>,
	[ size_t(Vm_Op::Type::Equal16 )]= &VM::ComparisonOpBase<U_i16, std::equal_to<U_i16>>,
	[ size_t(Vm_Op::Type::Equal32 )]= &VM::ComparisonOpBase<U_i32, std::equal_to<U_i32>>,
	[ size_t(Vm_Op::Type::Equal64 )]= &VM::ComparisonOpBase<U_i64, std::equal_to<U_i64>>,

	[ size_t(Vm_Op::Type::NotEqual8  )]= &VM::ComparisonOpBase<U_i8 , std::not_equal_to<U_i8 >>,
	[ size_t(Vm_Op::Type::NotEqual16 )]= &VM::ComparisonOpBase<U_i16, std::not_equal_to<U_i16>>,
	[ size_t(Vm_Op::Type::NotEqual32 )]= &VM::ComparisonOpBase<U_i32, std::not_equal_to<U_i32>>,
	[ size_t(Vm_Op::Type::NotEqual64 )]= &VM::ComparisonOpBase<U_i64, std::not_equal_to<U_i64>>,

	[ size_t(Vm_Op::Type::Less8i  )]= &VM::ComparisonOpBase<U_i8 , std::less<U_i8 >>,
	[ size_t(Vm_Op::Type::Less16i )]= &VM::ComparisonOpBase<U_i16, std::less<U_i16>>,
	[ size_t(Vm_Op::Type::Less32i )]= &VM::ComparisonOpBase<U_i32, std::less<U_i32>>,
	[ size_t(Vm_Op::Type::Less64i )]= &VM::ComparisonOpBase<U_i64, std::less<U_i64>>,
	[ size_t(Vm_Op::Type::Less8u  )]= &VM::ComparisonOpBase<U_u8 , std::less<U_u8 >>,
	[ size_t(Vm_Op::Type::Less16u )]= &VM::ComparisonOpBase<U_u16, std::less<U_u16>>,
	[ size_t(Vm_Op::Type::Less32u )]= &VM::ComparisonOpBase<U_u32, std::less<U_u32>>,
	[ size_t(Vm_Op::Type::Less64u )]= &VM::ComparisonOpBase<U_u64, std::less<U_u64>>,

	[ size_t(Vm_Op::Type::LessEqual8i  )]= &VM::ComparisonOpBase<U_i8 , std::less_equal<U_i8 >>,
	[ size_t(Vm_Op::Type::LessEqual16i )]= &VM::ComparisonOpBase<U_i16, std::less_equal<U_i16>>,
	[ size_t(Vm_Op::Type::LessEqual32i )]= &VM::ComparisonOpBase<U_i32, std::less_equal<U_i32>>,
	[ size_t(Vm_Op::Type::LessEqual64i )]= &VM::ComparisonOpBase<U_i64, std::less_equal<U_i64>>,
	[ size_t(Vm_Op::Type::LessEqual8u  )]= &VM::ComparisonOpBase<U_u8 , std::less_equal<U_u8 >>,
	[ size_t(Vm_Op::Type::LessEqual16u )]= &VM::ComparisonOpBase<U_u16, std::less_equal<U_u16>>,
	[ size_t(Vm_Op::Type::LessEqual32u )]= &VM::ComparisonOpBase<U_u32, std::less_equal<U_u32>>,
	[ size_t(Vm_Op::Type::LessEqual64u )]= &VM::ComparisonOpBase<U_u64, std::less_equal<U_u64>>,

	[ size_t(Vm_Op::Type::Greater8i  )]= &VM::ComparisonOpBase<U_i8 , std::greater<U_i8 >>,
	[ size_t(Vm_Op::Type::Greater16i )]= &VM::ComparisonOpBase<U_i16, std::greater<U_i16>>,
	[ size_t(Vm_Op::Type::Greater32i )]= &VM::ComparisonOpBase<U_i32, std::greater<U_i32>>,
	[ size_t(Vm_Op::Type::Greater64i )]= &VM::ComparisonOpBase<U_i64, std::greater<U_i64>>,
	[ size_t(Vm_Op::Type::Greater8u  )]= &VM::ComparisonOpBase<U_u8 , std::greater<U_u8 >>,
	[ size_t(Vm_Op::Type::Greater16u )]= &VM::ComparisonOpBase<U_u16, std::greater<U_u16>>,
	[ size_t(Vm_Op::Type::Greater32u )]= &VM::ComparisonOpBase<U_u32, std::greater<U_u32>>,
	[ size_t(Vm_Op::Type::Greater64u )]= &VM::ComparisonOpBase<U_u64, std::greater<U_u64>>,

	[ size_t(Vm_Op::Type::GreaterEqual8i  )]= &VM::ComparisonOpBase<U_i8 , std::greater_equal<U_i8 >>,
	[ size_t(Vm_Op::Type::GreaterEqual16i )]= &VM::ComparisonOpBase<U_i16, std::greater_equal<U_i16>>,
	[ size_t(Vm_Op::Type::GreaterEqual32i )]= &VM::ComparisonOpBase<U_i32, std::greater_equal<U_i32>>,
	[ size_t(Vm_Op::Type::GreaterEqual64i )]= &VM::ComparisonOpBase<U_i64, std::greater_equal<U_i64>>,
	[ size_t(Vm_Op::Type::GreaterEqual8u  )]= &VM::ComparisonOpBase<U_u8 , std::greater_equal<U_u8 >>,
	[ size_t(Vm_Op::Type::GreaterEqual16u )]= &VM::ComparisonOpBase<U_u16, std::greater_equal<U_u16>>,
	[ size_t(Vm_Op::Type::GreaterEqual32u )]= &VM::ComparisonOpBase<U_u32, std::greater_equal<U_u32>>,
	[ size_t(Vm_Op::Type::GreaterEqual64u )]= &VM::ComparisonOpBase<U_u64, std::greater_equal<U_u64>>,
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
	StackFrame::iterator stack,
	const std::vector<U_FundamentalType> params,
	unsigned int n)
{
	U_UNUSED(stack);

	return n == params.size();
}

void VM::OpLoop( OpIndex start_op_index )
{
	OpIndex next_op_index= start_op_index;

	while( next_op_index != 0 )
	{
		const Vm_Op& op= program_.code[ next_op_index ];
		auto func= operations_[ size_t(op.type) ];
		next_op_index= (this->*func)( next_op_index );
	}
}

void VM::OpCallImpl( const VmProgram::FuncCallInfo& call_info, OpIndex return_index )
{
	/* Stack frame of caller is.
	Result.
	Arguments.
	Saved caller caller stack pointer.
	Return operation index.
	*/

	// Save caller stack pointer of this function.
	std::memcpy( &*stack_pointer_, &caller_frame_pos_, sizeof(caller_frame_pos_) );
	stack_pointer_+= sizeof(caller_frame_pos_);

	// Push return address to stack.
	std::memcpy( &*stack_pointer_, &return_index, sizeof(OpIndex) );
	stack_pointer_+= sizeof(OpIndex);

	// Now, caller stack pointer is stack pointer of this function.
	caller_frame_pos_= stack_pointer_;

	// Make stack for new function
	stack_frames_.emplace_back( call_info.stack_frame_size );
	stack_pointer_= stack_frames_.back().begin();
}

OpIndex VM::OpNoOp( OpIndex op_index )
{
	return op_index + 1;
}

OpIndex VM::OpCall( OpIndex op_index )
{
	FuncNumber func_number;

	stack_pointer_-= sizeof(FuncNumber);
	std::memcpy(
		&func_number,
		&*stack_pointer_,
		sizeof(FuncNumber) );

	U_ASSERT( func_number < program_.funcs_table.size() );

	const VmProgram::FuncCallInfo& call_info=
		program_.funcs_table[ func_number ];

	OpCallImpl( call_info, op_index + 1 );

	return call_info.first_op_position;
}

OpIndex VM::OpRet( OpIndex op_index )
{
	U_UNUSED(op_index);

	// Destroy stack.
	stack_frames_.pop_back();

	// Pop return address.
	OpIndex ret_op_index;
	caller_frame_pos_-= sizeof(OpIndex);
	std::memcpy(
		&ret_op_index,
		&*caller_frame_pos_,
		sizeof(OpIndex) );

	// Pop old caller frame pos.
	StackFrame::iterator old_caller_frame_pos;
	caller_frame_pos_-= sizeof(StackFrame::iterator);
	std::memcpy(
		&old_caller_frame_pos,
		&*caller_frame_pos_,
		sizeof(StackFrame::iterator) );

	// Restore all stack pointers.
	stack_pointer_= caller_frame_pos_;
	caller_frame_pos_= old_caller_frame_pos;

	return ret_op_index;
}

OpIndex VM::OpSysCall( OpIndex op_index )
{
	// TODO
	U_UNUSED(op_index);
	return 0;
}

OpIndex VM::OpJump( OpIndex op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	return op.param.jump_op_index;
}

OpIndex VM::OpJumpIfZero( OpIndex op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	U_bool condition;
	stack_pointer_-= sizeof(U_bool);
	std::memcpy(
		&condition,
		&*stack_pointer_,
		sizeof(U_bool) );

	if( !condition )
		return op.param.jump_op_index;
	return op_index + 1;
}

OpIndex VM::OpJumpIfNotZero( OpIndex op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	U_bool condition;
	stack_pointer_-= sizeof(U_bool);
	std::memcpy(
		&condition,
		&*stack_pointer_,
		sizeof(U_bool) );

	if( condition )
		return op.param.jump_op_index;
	return op_index + 1;
}

OpIndex VM::OpStackPointerAdd( OpIndex op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	stack_pointer_+= op.param.stack_add_size;

	return op_index + 1;
}

OpIndex VM::OpPushC8( OpIndex op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	std::memcpy(
		&*stack_pointer_,
		&op.param.push_c_8,
		sizeof(U_i8) );
	stack_pointer_+= sizeof(U_i8);

	return op_index + 1;
}

OpIndex VM::OpPushC16( OpIndex op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	std::memcpy(
		&*stack_pointer_,
		&op.param.push_c_16,
		sizeof(U_i16) );
	stack_pointer_+= sizeof(U_i16);

	return op_index + 1;
}

OpIndex VM::OpPushC32( OpIndex op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	std::memcpy(
		&*stack_pointer_,
		&op.param.push_c_32,
		sizeof(U_i32) );
	stack_pointer_+= sizeof(U_i32);

	return op_index + 1;
}

OpIndex VM::OpPushC64( OpIndex op_index )
{
	const Vm_Op& op= program_.code[ op_index ];

	std::memcpy(
		&*stack_pointer_,
		&op.param.push_c_64,
		sizeof(U_i32) );
	stack_pointer_+= sizeof(U_i64);

	return op_index + 1;
}

} // namespace Interpreter
