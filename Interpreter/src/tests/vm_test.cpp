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

	vm.Call( func.name );
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
	func.return_type= U_FundamentalType::Void;
	program.export_funcs.push_back( func );

	VmProgram::FuncCallInfo call_info;
	call_info.first_op_position= 1;
	call_info.stack_frame_size= 16;
	program.funcs_table.push_back( call_info );

	std::sort( program.export_funcs.begin(), program.export_funcs.end() );

	VM vm{ program };

	U_u8 result;
	vm.CallRet( func.name, result );
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
	func.return_type= U_FundamentalType::Void;
	program.export_funcs.push_back( func );

	VmProgram::FuncCallInfo call_info;
	call_info.first_op_position= 1;
	call_info.stack_frame_size= 16;
	program.funcs_table.push_back( call_info );

	std::sort( program.export_funcs.begin(), program.export_funcs.end() );

	VM vm{ program };

	U_u16 result;
	vm.CallRet( func.name, result );
	U_ASSERT( result == c_result );
}


void RunVMTests()
{
	SimpleProgramTest();
	SimpleRetProgramTest();
	SimpleRetProgramTest2();
}

} // namespace Interpreter
