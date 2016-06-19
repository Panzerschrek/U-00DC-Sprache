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
struct Array;
struct Class;

typedef std::shared_ptr<Class> ClassPtr;

struct Type final
{
	enum class Kind
	{
		Fundamental,
		Function,
		Array,
		Class,
	};

	Kind kind;
	U_FundamentalType fundamental;
	std::unique_ptr<Function> function;
	std::unique_ptr<Array> array;

	// We use one instance for Class, because class has one point of definition.
	// Therefore, class are aquals, if equals pointers.
	ClassPtr class_;

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

struct Array final
{
	Type type;
	size_t size;
};

struct Class final
{
	Class();
	~Class();

	Class( const Class& )= delete;
	Class( Class&& )= delete;

	Class& operator=( const Class& )= delete;
	Class& operator=( Class&& )= delete;

	struct Field
	{
		ProgramString name;
		Type type;
		unsigned int offset;
	};

	const Field* GetField( const ProgramString& name );

	ProgramString name;
	std::vector<Field> fields;
	unsigned int size;
};

struct Variable final
{
	enum class Location
	{
		FunctionArgument,
		Stack,
		Global,

		ValueAtExpessionStackTop,
		AddressAtExpessionStackTop,
	};

	// For function argumnet - minus offset from caller frame
	// For stack variable - offset from stack frame
	// For Global varianle - global offset
	unsigned int offset;

	Location location;
	Type type;
};

struct Name final
{
	// If ptr not null - name is calss, else - variable
	ClassPtr class_;
	Variable variable;
};

class NamesScope final
{
public:

	typedef std::map< ProgramString, Name > NamesMap;
	typedef NamesMap::value_type InsertedName;

	NamesScope( const NamesScope* prev= nullptr );

	const InsertedName* AddName( const ProgramString& name, Variable variable );
	const InsertedName* AddName( const ProgramString& name, const ClassPtr& class_ );
	const InsertedName* AddName( const ProgramString& name, const Name name_value );

	const InsertedName* GetName( const ProgramString& name ) const;

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
