#pragma once
#include <map>
#include <string>
#include <vector>

#include "code_builder_types.hpp"
#include "syntax_elements.hpp"
#include "vm.hpp"

namespace Interpreter
{

namespace CodeBuilderPrivate
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
		const NamesScope& names,
		FunctionContext& function_context,
		BlockStackContext stack_context );

	U_FundamentalType BuildFuncCall(
		const Function& func,
		const CallOperator& call_operator,
		const NamesScope& names,
		FunctionContext& function_context,
		BlockStackContext stack_context );

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

} // CodeBuilderPrivate

using CodeBuilderPrivate::CodeBuilder;

} //namespace Interpreter
