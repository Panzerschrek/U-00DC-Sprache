#pragma once
#include <memory>
#include <vector>
#include <map>

#include "vm.hpp"

namespace Interpreter
{

namespace CodeBuilderPrivate
{

struct Function;

struct Type final
{
	enum class Kind
	{
		Fundamental,
		Function,
	};

	Kind kind;
	U_FundamentalType fundamental;
	std::unique_ptr<Function> function;

	explicit Type( U_FundamentalType in_fundamental= U_FundamentalType::InvalidType );
	Type( const Type& other );
	Type( Type&& other );

	Type& operator=( const Type& other );
	Type& operator=( Type&& other );

	size_t SizeOf() const;
};

bool operator==( const Type& r, const Type& l );
bool operator!=( const Type& r, const Type& l );

struct Function final
{
	Type return_type;
	std::vector<Type> args;
};

bool operator==( const Function& r, const Function& l );
bool operator!=( const Function& r, const Function& l );

struct Variable final
{
	enum class Location
	{
		FunctionArgument,
		Stack,
		Global,

		ValueAtExpessionStackTop,
		AddressExpessionStackTop,
	};

	// For function argumnet - minus offset from caller frame
	// For stack variable - offset from stack frame
	// For Global varianle - global offset
	unsigned int offset;

	Location location;
	Type type;
};

class NamesScope final
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

class ProgramError final : public std::exception
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

class ExpressionStackSizeCounter
{
public:
	void operator+=( unsigned int add_size );
	void operator-=( unsigned int sub_size );

	unsigned int GetMaxReachedStackSize() const;
	unsigned int GetCurrentStackSize() const;

private:
	unsigned int size_= 0;
	unsigned int max_reached_size_= 0;
};

struct FunctionContext final
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

	ExpressionStackSizeCounter expression_stack_size_counter;
};

} //namespace CodeBuilderPrivate

} // namespace Interpreter
