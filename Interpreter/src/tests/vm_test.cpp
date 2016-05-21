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
			-(
			sizeof(unsigned int) + // saved previous caller pointer.
			sizeof(unsigned int) + // return address.
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
			-(
			sizeof(unsigned int) + // saved previous caller pointer.
			sizeof(unsigned int) + // return address.
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
			-(
			sizeof(unsigned int) + // saved previous caller pointer.
			sizeof(unsigned int) + // return address.
			sizeof(U_u32) + // result
			sizeof(U_u32) + ( 1 - i ) * sizeof(U_u32) // args
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
			-(
			sizeof(unsigned int) + // saved previous caller pointer.
			sizeof(unsigned int) + // return address.
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

void RunVMTests()
{
	SimpleProgramTest();
	SimpleRetProgramTest();
	SimpleRetProgramTest2();
	RetProgramWithArgsTest();
}

} // namespace Interpreter
