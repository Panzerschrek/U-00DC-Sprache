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

void RunVMTests()
{
	SimpleProgramTest();
}

} // namespace Interpreter
