#pragma once
#include <cstdint>

#include "program_string.hpp"

namespace Interpreter
{

struct Vm_Op
{
	enum class Type : unsigned char
	{
		FirstOp= 0,
		NoOp= 0,

		Call,
		Ret,

		Syscall,

		// Bit and
		And8,
		And16,
		And32,
		And64,

		// Bit or
		Or8,
		Or16,
		Or32,
		Or64,

		// Bit ixor
		Xor8,
		Xor16,
		Xor32,
		Xor64,

		// Bit inverse
		Not8,
		Not16,
		Not32,
		Not64,

		// Subtraction
		Subi32,
		Subu32,
		Subi64,
		Subu64,

		// Addition
		Addi32,
		Addu32,
		Addi64,
		Addu64,

		// Push constant
		PushC8 ,
		PushC16,
		PushC32,
		PushC64,

		LastOp
	};

	Type type;

	union
	{
		struct
		{
			unsigned int func_number;
		} call_param;

		std::uint8_t  push_c_8 ;
		std::uint16_t push_c_16;
		std::uint32_t push_c_32;
		std::uint64_t push_c_64;
	} param;
};

typedef void U_void;
typedef std::int8_t   U_i8 ;
typedef std::uint8_t  U_u8 ;
typedef std::int16_t  U_i16;
typedef std::uint16_t U_u16;
typedef std::int32_t  U_i32;
typedef std::uint32_t U_u32;
typedef std::int64_t  U_i64;
typedef std::uint64_t U_u64;

enum class U_FundamentalType
{
	InvalidType,
	Void,
	i8 ,
	u8 ,
	i16,
	u16,
	i32,
	u32,
	i64,
	u64,

	LastType,
};

struct FuncEntry
{
	ProgramString name;
	unsigned int func_number;
	std::vector<U_FundamentalType> params;
	U_FundamentalType return_type;

	bool operator<( const FuncEntry& other ) const
	{
		return name < other.name;
	}
};

typedef std::vector<FuncEntry> Funcs;

struct VmProgram
{
	std::vector<Vm_Op> code;

	// Sorted by name import and export funcs.
	Funcs import_funcs;
	Funcs export_funcs;


	struct FuncCallInfo
	{
		unsigned int first_op_position;
		unsigned int stack_frame_size;
	};

	std::vector<FuncCallInfo> funcs_table;
};

class VM final
{
public:
	VM( VmProgram program );

	void Call( const ProgramString& func_name );

	template<class ... Args>
	void Call( const ProgramString& func_name, Args&&... args );

	template<class Ret>
	void CallRet( const ProgramString& func_name, Ret& result );

	template<class Ret, class ... Args>
	void CallRet(
		const ProgramString& func_name,
		Ret& result,
		Args&&... args );

private:
	typedef std::vector<std::uint8_t> StackFrame;

private:
	Funcs::iterator FindExportedFunc( const ProgramString& name );

	bool PushArgs(
		StackFrame& stack,
		const std::vector<U_FundamentalType> params,
		unsigned int n );

	template<class T, class ... Args>
	bool PushArgs(
		StackFrame& stack,
		const std::vector<U_FundamentalType> params,
		unsigned int n,
		const T& arg0,
		const Args&... args );

private:
	void OpLoop( unsigned int start_op_index );
	void OpCallImpl( const VmProgram::FuncCallInfo& call_info, unsigned int return_index );

	// Returns index of next op.
	typedef unsigned (VM::* VMOpPoiter)( unsigned int op_index );

	static const VMOpPoiter operations_[size_t( Vm_Op::Type::LastOp ) ];

	unsigned int OpNoOp( unsigned int op_index );
	unsigned int OpCall( unsigned int op_index );
	unsigned int OpRet( unsigned int op_index );
	unsigned int OpSysCall( unsigned int op_index );

private:
	VmProgram program_;

	std::vector<StackFrame> stack_frames_;

	StackFrame::iterator caller_frame_pos_;
	StackFrame::iterator stack_pointer_;
};

}

#include "vm.inl"
