#pragma once
#include <cstdint>

#include "program_string.hpp"

namespace Interpreter
{

typedef std::uint32_t OpIndex;
typedef std::uint32_t FuncNumber;

struct Vm_Op
{
	enum class Type : unsigned char
	{
		FirstOp= 0,
		NoOp= 0,

		Call,
		Ret,

		Syscall,

		Jump,
		JumpIfZero,
		JumpIfNotZero,

		StackPointerAdd,

		// Push constant
		PushC8 ,
		PushC16,
		PushC32,
		PushC64,

		// Push to stack relative current frame.
		PushFromLocalStack8 ,
		PushFromLocalStack16,
		PushFromLocalStack32,
		PushFromLocalStack64,

		// Move to local stack from stack top.
		PopToLocalStack8 ,
		PopToLocalStack16,
		PopToLocalStack32,
		PopToLocalStack64,

		// Push to caller relative current frame.
		PushFromCallerStack8 ,
		PushFromCallerStack16,
		PushFromCallerStack32,
		PushFromCallerStack64,

		// Move to caller stack from stack top.
		PopToCallerStack8 ,
		PopToCallerStack16,
		PopToCallerStack32,
		PopToCallerStack64,

		// Push to stack address + offset
		PushLocalStackAddress,
		PushCallerStackAddress,

		// Dereference pointer on stack top
		Deref8 ,
		Deref16,
		Deref32,
		Deref64,

		// Pop value, pop address, move value to address
		Mov8 ,
		Mov16,
		Mov32,
		Mov64,

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

		// Negation
		Negi32,
		Negu32,
		Negi64,
		Negu64,

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

		// Multiplication
		Muli32,
		Mulu32,
		Muli64,
		Mulu64,

		// Division
		Divi32,
		Divu32,
		Divi64,
		Divu64,

		// 8 bit signed expansion
		Conv8To16S,
		Conv8To32S,
		Conv8To64S,

		// 8 bit unsigned expansion
		Conv8To16U,
		Conv8To32U,
		Conv8To64U,

		// 16 bit signed expansion
		Conv16To32S,
		Conv16To64S,

		// 16 bit unsigned expansion
		Conv16To32U,
		Conv16To64U,

		// 32 bit signmed and unsigned expansion
		Conv32To64S,
		Conv32To64U,

		// 64 bit reduction
		Conv64To32,
		Conv64To16,
		Conv64To8 ,

		// 32 bit reduction
		Conv32To16,
		Conv32To8 ,

		// 16 bit reduction
		Conv16To8,

		// ==
		Equal8 ,
		Equal16,
		Equal32,
		Equal64,

		// !=
		NotEqual8 ,
		NotEqual16,
		NotEqual32,
		NotEqual64,

		// <
		Less8i ,
		Less16i,
		Less32i,
		Less64i,
		Less8u ,
		Less16u,
		Less32u,
		Less64u,

		// <=
		LessEqual8i ,
		LessEqual16i,
		LessEqual32i,
		LessEqual64i,
		LessEqual8u ,
		LessEqual16u,
		LessEqual32u,
		LessEqual64u,

		// >
		Greater8i ,
		Greater16i,
		Greater32i,
		Greater64i,
		Greater8u ,
		Greater16u,
		Greater32u,
		Greater64u,

		// >=
		GreaterEqual8i ,
		GreaterEqual16i,
		GreaterEqual32i,
		GreaterEqual64i,
		GreaterEqual8u ,
		GreaterEqual16u,
		GreaterEqual32u,
		GreaterEqual64u,

		LastOp
	};


	Vm_Op() {}
	explicit Vm_Op( Type in_type ) : type( in_type ) {}

	Type type;

	union
	{
		std::uint8_t  push_c_8 ;
		std::uint16_t push_c_16;
		std::uint32_t push_c_32;
		std::uint64_t push_c_64;

		// Offset for pish/pop local stack operations.
		unsigned int local_stack_operations_offset;

		// Offset for push/pop caller stack operations.
		// For arguments, return address, result offset are negative.
		int caller_stack_operations_offset;

		// Number of bytes, added to stack pointer
		int stack_add_size;

		OpIndex jump_op_index;
	} param;
};

typedef void U_void;
typedef bool U_bool;
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
	Bool,
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
	FuncNumber func_number;
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
		OpIndex first_op_position;
		unsigned int stack_frame_size;
	};

	std::vector<FuncCallInfo> funcs_table;
};

class VM final
{
public:
	VM( VmProgram program );

public:
	struct CallResult
	{
		bool ok= false;
		std::string error_message;
	};

	CallResult Call( const ProgramString& func_name );

	template<class ... Args>
	CallResult Call( const ProgramString& func_name, Args&&... args );

	template<class Ret>
	CallResult CallRet( const ProgramString& func_name, Ret& result );

	template<class Ret, class ... Args>
	CallResult CallRet(
		const ProgramString& func_name,
		Ret& result,
		Args&&... args );

private:
	typedef std::vector<std::uint8_t> StackFrame;

private:
	Funcs::iterator FindExportedFunc( const ProgramString& name );

	size_t SizeOfArgs();

	template< class T, class ... Args >
	size_t SizeOfArgs( const T& arg0, const Args&... args );

	bool PushArgs(
		StackFrame::iterator stack,
		const std::vector<U_FundamentalType> params,
		unsigned int n );

	template<class T, class ... Args>
	bool PushArgs(
		StackFrame::iterator stack,
		const std::vector<U_FundamentalType> params,
		unsigned int n,
		const T& arg0,
		const Args&... args );

private:
	void OpLoop( unsigned int start_op_index );
	void OpCallImpl( const VmProgram::FuncCallInfo& call_info, OpIndex return_index );

	// Returns index of next op.
	typedef OpIndex (VM::* VMOpPoiter)( OpIndex op_index );

	static const VMOpPoiter operations_[size_t( Vm_Op::Type::LastOp ) ];

	OpIndex OpNoOp( OpIndex op_index );
	OpIndex OpCall( OpIndex op_index );
	OpIndex OpRet( OpIndex op_index );
	OpIndex OpSysCall( OpIndex op_index );

	OpIndex OpJump( OpIndex op_index );
	OpIndex OpJumpIfZero( OpIndex op_index );
	OpIndex OpJumpIfNotZero( OpIndex op_index );

	OpIndex OpStackPointerAdd( OpIndex op_index );

	OpIndex OpPushC8 ( OpIndex op_index );
	OpIndex OpPushC16( OpIndex op_index );
	OpIndex OpPushC32( OpIndex op_index );
	OpIndex OpPushC64( OpIndex op_index );

	OpIndex OpPushLocalStackAddress( OpIndex op_index );
	OpIndex OpPushCallerStackAddress( OpIndex op_index );

	template<class T>
	unsigned int PushFromLocalStackOpBase( unsigned int op_index );

	template<class T>
	unsigned int PopToLocalStackOpBase( unsigned int op_index );

	template<class T>
	unsigned int PushFromCallerStackOpBase( unsigned int op_index );

	template<class T>
	unsigned int PopToCallerStackOpBase( unsigned int op_index );

	template<class T>
	unsigned int DerefOpBase( unsigned int op_index );

	template<class T>
	unsigned int MovOpBase( unsigned int op_index );

	template<class T, class Func>
	unsigned int BinaryOpBase( unsigned int op_index );

	template<class T, class Func>
	unsigned int ComparisonOpBase( unsigned int op_index );

	template<class T, class Func>
	unsigned int UnaryOpBase( unsigned int op_index );

	template<class From, class To>
	unsigned int ConvertionOpBase( unsigned int op_index );

private:
	static const char* const c_func_not_found_;
	static const char* const c_invalid_return_type_;
	static const char* const c_invalid_arguments_type_;

private:
	VmProgram program_;

	std::vector<StackFrame> stack_frames_;

	StackFrame::iterator caller_frame_pos_;
	StackFrame::iterator stack_pointer_;

public:
	static const unsigned int c_saved_caller_frame_size_;
};

}

#include "vm.inl"
