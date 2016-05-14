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
		U_FundamentalType fundamental;
		std::unique_ptr<Function> function;
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

		void AddName( const ProgramString& name, Variable variable );
		const NamesMap::value_type* GetName( const ProgramString& name ) const;

	private:
		const NamesScope* const prev_;
		NamesMap names_map_;

	};

	typedef std::map< ProgramString, Variable > FuncsTable;

private:
	void BuildBlockCode(
		const Block& block,
		const NamesScope& names );

private:
	VmProgram result_;

	FuncsTable func_table_;
	unsigned int next_func_number_= 0;

};

} //namespace Interpreter
