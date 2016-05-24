#pragma once
#include <map>
#include <string>
#include <vector>

#include "syntax_elements.hpp"
#include "vm.hpp"

namespace Interpreter
{

class CodeBuilder final
{
public:
	CodeBuilder();
	~CodeBuilder();

	struct BuildResult
	{
		std::vector<std::string> error_messages;
		VmProgram program;
	};

	BuildResult BuildProgram( const ProgramElements& program_elements );

private:
	struct Function;

	struct Type
	{
		enum class Kind
		{
			Fundamental,
			Function,
		};

		Kind kind;
		U_FundamentalType fundamental = U_FundamentalType::InvalidType;
		std::unique_ptr<Function> function;

		Type();
		Type( const Type& other );
		Type( Type&& other );

		Type& operator=( const Type& other );
		Type& operator=( Type&& other );
	};

	struct Function
	{
		Type return_type;
		std::vector<Type> args;
	};

	struct Variable
	{
		enum class Location
		{
			FunctionArgument,
			Stack,
			Global,
		};

		// For function argumnet - minus offset from caller frame
		// For stack variable - offset from stack frame
		// For Global varianle - global offset
		unsigned int offset;

		Location location;
		Type type;
	};

	class NamesScope
	{
	public:
		typedef std::map< ProgramString, Variable > NamesMap;

		NamesScope( const NamesScope* prev= nullptr );

		const NamesMap::value_type* AddName( const ProgramString& name, Variable variable );
		const NamesMap::value_type* GetName( const ProgramString& name ) const;

	private:
		const NamesScope* const prev_;
		NamesMap names_map_;
	};

	class ProgramError : public std::exception
	{
	public:
		virtual ~ProgramError() override{}

		virtual const char* what() const noexcept override
		{
			return "ProgramError";
		}
	};

	// Class for locals variables offset calculation.
	// Also, it calclats max needed stack size for locals.
	class BlockStackContext final
	{
	public:
		BlockStackContext();
		BlockStackContext( BlockStackContext& parent_context );
		~BlockStackContext();

		void IncreaseStack( unsigned int size );

		unsigned int GetStackSize() const;
		unsigned int GetMaxReachedStackSize() const;

	private:
		BlockStackContext* const parent_context_;

		unsigned int stack_size_;
		unsigned int max_reached_stack_size_;
	};

	struct FunctionContext
	{
		unsigned int result_offset;
		U_FundamentalType result_type;

		struct WhileFrame
		{
			// For "continue".
			OpIndex first_while_op_index;
			// Stored "break" operations indeces.
			std::vector<OpIndex> break_operations_indeces;
		};

		std::vector<WhileFrame> while_frames;
	};

private:
	void BuildFuncCode(
		const Function& func,
		const std::vector<ProgramString> arg_names,
		const Block& block );

	void BuildBlockCode(
		const Block& block,
		const NamesScope& names,
		FunctionContext& function_context,
		BlockStackContext stack_context );

	U_FundamentalType BuildExpressionCode(
		const BinaryOperatorsChain& expression,
		const NamesScope& names );

	U_FundamentalType BuildFuncCall(
		const Function& func,
		unsigned int func_number,
		const CallOperator& call_operator,
		const NamesScope& names );

	void BuildIfOperator(
		const NamesScope& names,
		const IfOperator& if_operator,
		FunctionContext& function_context,
		BlockStackContext stack_context );

	void BuildWhileOperator(
		const NamesScope& names,
		const WhileOperator& while_operator,
		FunctionContext& function_context,
		BlockStackContext stack_context );

private:
	unsigned int error_count_= 0;
	std::vector<std::string> error_messages_;
	VmProgram result_;

	NamesScope global_names_;
	unsigned int next_func_number_= 0;

};

} //namespace Interpreter
