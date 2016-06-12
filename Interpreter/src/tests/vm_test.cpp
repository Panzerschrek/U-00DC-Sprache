#include <algorithm>

#include "../assert.hpp"
#include "../vm.hpp"

namespace Interpreter
{

static Vm_Op MakeNoOp()
{
	Vm_Op result;
	result.type= Vm_Op::Type::NoOp;
	return result;
}

static Vm_Op MakeRet()
{
	Vm_Op result;
	result.type= Vm_Op::Type::Ret;
	return result;
}

static Vm_Op MakePushC8( U_u8 c )
{
	Vm_Op result;
	result.type= Vm_Op::Type::PushC8;
	result.param.push_c_8= c;
	return result;
}

static Vm_Op MakePushC16( U_u16 c )
{
	Vm_Op result;
	result.type= Vm_Op::Type::PushC16;
	result.param.push_c_16= c;
	return result;
}


static void SimpleProgramTest()
{
	VmProgram program;

	program.code.push_back( MakeNoOp() );
	program.code.push_back( MakeRet() );

	FuncEntry func;
	func.func_number= 0;
	func.name= ToProgramString( "foo" );
	func.return_type= U_FundamentalType::Void;
	program.export_funcs.push_back( func );

	VmProgram::FuncCallInfo call_info;
	call_info.first_op_position= 1;
	call_info.stack_frame_size= 16;
	program.funcs_table.push_back( call_info );

	std::sort( program.export_funcs.begin(), program.export_funcs.end() );

	VM vm{ program };

	VM::CallResult call_result= vm.Call( func.name );
	U_ASSERT( call_result.ok );
}

static void SimpleRetProgramTest()
{
	const U_u8 c_result= 42;

	VmProgram program;

	program.code.push_back( MakeNoOp() );
	program.code.push_back( MakePushC8( c_result ) );
	{
		Vm_Op op;
		op.type= Vm_Op::Type::PopToCallerStack8;
		op.param.caller_stack_operations_offset=
			-int(
				VM::c_saved_caller_frame_size_ +
				sizeof(U_u8) // result
			);

		program.code.push_back( op );
	}
	program.code.push_back( MakeRet() );

	FuncEntry func;
	func.func_number= 0;
	func.name= ToProgramString( "foo" );
	func.return_type= U_FundamentalType::u8;
	program.export_funcs.push_back( func );

	VmProgram::FuncCallInfo call_info;
	call_info.first_op_position= 1;
	call_info.stack_frame_size= 16;
	program.funcs_table.push_back( call_info );

	std::sort( program.export_funcs.begin(), program.export_funcs.end() );

	VM vm{ program };

	U_u8 result;
	VM::CallResult call_result= vm.CallRet( func.name, result );
	U_ASSERT( call_result.ok );
	U_ASSERT( result == c_result );
}

static void SimpleRetProgramTest2()
{
	const U_u16 c_result= 0x63FF;

	VmProgram program;

	program.code.push_back( MakeNoOp() );
	program.code.push_back( MakePushC16( c_result ) );
	{
		Vm_Op op;
		op.type= Vm_Op::Type::PopToCallerStack16;
		op.param.caller_stack_operations_offset=
			-int(
				VM::c_saved_caller_frame_size_ +
				sizeof(U_u16) // result
			);

		program.code.push_back( op );
	}
	program.code.push_back( MakeRet() );

	FuncEntry func;
	func.func_number= 0;
	func.name= ToProgramString( "foo" );
	func.return_type= U_FundamentalType::u16;
	program.export_funcs.push_back( func );

	VmProgram::FuncCallInfo call_info;
	call_info.first_op_position= 1;
	call_info.stack_frame_size= 16;
	program.funcs_table.push_back( call_info );

	std::sort( program.export_funcs.begin(), program.export_funcs.end() );

	VM vm{ program };

	U_u16 result;
	VM::CallResult call_result= vm.CallRet( func.name, result );
	U_ASSERT( call_result.ok );
	U_ASSERT( result == c_result );
}

static void RetProgramWithArgsTest()
{
	const U_u32 arg_a= 764;
	const U_u32 arg_b= 7584;

	VmProgram program;

	program.code.push_back( MakeNoOp() );

	// Move args to stack
	for( unsigned int i= 0; i < 2; i++ )
	{
		Vm_Op op;
		op.type= Vm_Op::Type::PushFromCallerStack32;
		op.param.caller_stack_operations_offset=
			-int(
				VM::c_saved_caller_frame_size_ +
				( 2 - i ) * sizeof(U_u32) // args
			);

		program.code.push_back( op );
	}
	{ // Add
		Vm_Op op;
		op.type= Vm_Op::Type::Subu32;
		program.code.push_back( op );
	}
	{ // Move result
		Vm_Op op;
		op.type= Vm_Op::Type::PopToCallerStack32;
		op.param.caller_stack_operations_offset=
			-int(
				VM::c_saved_caller_frame_size_ +
				sizeof(U_u32) * 2 + // args
				sizeof(U_u32) // result
			);

		program.code.push_back( op );
	}
	program.code.push_back( MakeRet() );

	FuncEntry func;
	func.func_number= 0;
	func.name= ToProgramString( "foo" );
	func.return_type= U_FundamentalType::u32;
	func.params.push_back(U_FundamentalType::u32);
	func.params.push_back(U_FundamentalType::u32);
	program.export_funcs.push_back( func );

	VmProgram::FuncCallInfo call_info;
	call_info.first_op_position= 1;
	call_info.stack_frame_size= 16;
	program.funcs_table.push_back( call_info );

	std::sort( program.export_funcs.begin(), program.export_funcs.end() );

	VM vm{ program };

	U_u32 result;
	VM::CallResult call_result= vm.CallRet( func.name, result, arg_a, arg_b );
	U_ASSERT( call_result.ok );
	U_ASSERT( result == arg_a - arg_b );
}

static void ConditionsTest()
{
	const U_u32 arg_a= 764;
	const U_u32 arg_b= 7584;

	VmProgram program;

	program.code.push_back( MakeNoOp() );

	// Move args to stack
	for( unsigned int i= 0; i < 2; i++ )
	{
		Vm_Op op;
		op.type= Vm_Op::Type::PushFromCallerStack32;
		op.param.caller_stack_operations_offset=
			-int(
				VM::c_saved_caller_frame_size_ +
				( 2 - i ) * sizeof(U_u32) // args
			);

		program.code.push_back( op );
	}

	// Compare
	program.code.emplace_back( Vm_Op::Type::Less32u );

	{ // Conditional jump
		Vm_Op op{ Vm_Op::Type::JumpIfNotZero };
		op.param.jump_op_index= program.code.size() + 1 + 3;

		program.code.push_back( op );
	}

	// Return agr0 or arg1
	for( unsigned int i= 0; i < 2; i++ )
	{
		Vm_Op push_op{ Vm_Op::Type::PushFromCallerStack32 };
		push_op.param.caller_stack_operations_offset=
			-int(
				VM::c_saved_caller_frame_size_ +
				( 2 - i ) * sizeof(U_u32) // args
			);

		program.code.push_back( push_op );

		Vm_Op pop_op{ Vm_Op::Type::PopToCallerStack32 };
		pop_op.param.caller_stack_operations_offset=
			-int(
				VM::c_saved_caller_frame_size_ +
				sizeof(U_u32) * 2 + // args
				sizeof(U_u32) // result
			);

		program.code.push_back( pop_op );
		program.code.push_back( MakeRet() );
	}

	FuncEntry func;
	func.func_number= 0;
	func.name= ToProgramString( "max" );
	func.return_type= U_FundamentalType::u32;
	func.params.push_back(U_FundamentalType::u32);
	func.params.push_back(U_FundamentalType::u32);
	program.export_funcs.push_back( func );

	VmProgram::FuncCallInfo call_info;
	call_info.first_op_position= 1;
	call_info.stack_frame_size= 16;
	program.funcs_table.push_back( call_info );

	std::sort( program.export_funcs.begin(), program.export_funcs.end() );

	VM vm{ program };

	U_u32 result;
	VM::CallResult call_result= vm.CallRet( func.name, result, arg_a, arg_b );
	U_ASSERT( call_result.ok );
	U_ASSERT( result == std::max( arg_a, arg_b ) );

	call_result= vm.CallRet( func.name, result, arg_b, arg_a );
	U_ASSERT( call_result.ok );
	U_ASSERT( result == std::max( arg_a, arg_b ) );
}

static void AddressOperatorsTest0()
{
	const U_u16 c_expected_result= 256 + 42;

	VmProgram program;
	program.code.push_back( MakeNoOp() );

	{
		Vm_Op op( Vm_Op::Type::PushCallerStackAddress );
		op.param.caller_stack_operations_offset=
			-int(
				VM::c_saved_caller_frame_size_ +
				sizeof(U_u16) // param
			);
		program.code.push_back( op );
	}
	program.code.emplace_back( Vm_Op::Type::Deref16 );
	{
		Vm_Op op( Vm_Op::Type::PopToCallerStack16 );
		op.param.caller_stack_operations_offset=
			-int(
				VM::c_saved_caller_frame_size_ +
				sizeof(U_u16) +// param
				sizeof(U_u16) // result
			);
		program.code.push_back( op );
	}
	program.code.push_back( MakeRet() );

	FuncEntry func;
	func.func_number= 0;
	func.name= ToProgramString( "foo" );
	func.return_type= U_FundamentalType::u16;
	func.params.push_back(U_FundamentalType::u16);
	program.export_funcs.push_back( func );

	VmProgram::FuncCallInfo call_info;
	call_info.first_op_position= 1;
	call_info.stack_frame_size= 16;
	program.funcs_table.push_back( call_info );

	std::sort( program.export_funcs.begin(), program.export_funcs.end() );

	VM vm{ program };

	U_u16 result;
	VM::CallResult call_result= vm.CallRet( func.name, result, c_expected_result );
	U_ASSERT( call_result.ok );
	U_ASSERT( result == c_expected_result );
}

static void AddressOperatorsTest1()
{
	const U_u16 c_expected_result= 256 + 1488;

	VmProgram program;
	program.code.push_back( MakeNoOp() );

	{
		Vm_Op op( Vm_Op::Type::PushFromCallerStack16 );
		op.param.caller_stack_operations_offset=
			-int(
				VM::c_saved_caller_frame_size_ +
				sizeof(U_u16) // param
			);
		program.code.push_back( op );
	}
	{
		Vm_Op op( Vm_Op::Type::PopToLocalStack16 );
		op.param.local_stack_operations_offset= 7;
		program.code.push_back( op );
	}
	{
		Vm_Op op( Vm_Op::Type::PushLocalStackAddress );
		op.param.local_stack_operations_offset= 7;
		program.code.push_back( op );
	}
	program.code.emplace_back( Vm_Op::Type::Deref16 );
	{
		Vm_Op op( Vm_Op::Type::PopToCallerStack16 );
		op.param.caller_stack_operations_offset=
			-int(
				VM::c_saved_caller_frame_size_ +
				sizeof(U_u16) +// param
				sizeof(U_u16) // result
			);
		program.code.push_back( op );
	}
	program.code.push_back( MakeRet() );

	FuncEntry func;
	func.func_number= 0;
	func.name= ToProgramString( "foo" );
	func.return_type= U_FundamentalType::u16;
	func.params.push_back(U_FundamentalType::u16);
	program.export_funcs.push_back( func );

	VmProgram::FuncCallInfo call_info;
	call_info.first_op_position= 1;
	call_info.stack_frame_size= 16;
	program.funcs_table.push_back( call_info );

	std::sort( program.export_funcs.begin(), program.export_funcs.end() );

	VM vm{ program };

	U_u16 result;
	VM::CallResult call_result= vm.CallRet( func.name, result, c_expected_result );
	U_ASSERT( call_result.ok );
	U_ASSERT( result == c_expected_result );
}

void RunVMTests()
{
	SimpleProgramTest();
	SimpleRetProgramTest();
	SimpleRetProgramTest2();
	RetProgramWithArgsTest();
	ConditionsTest();
	AddressOperatorsTest0();
	AddressOperatorsTest1();
}

} // namespace Interpreter
