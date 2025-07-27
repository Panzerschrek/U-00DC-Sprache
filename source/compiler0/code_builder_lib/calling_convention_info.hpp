#pragma once
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/DataLayout.h>
#include <llvm/TargetParser/Triple.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "type.hpp"

namespace U
{

class ICallingConventionInfo
{
public:
	struct ArgumentPassingDirect
	{
		llvm::Type* llvm_type= nullptr;
		uint16_t load_store_alignment= 0;
		bool sext= false;
		bool zext= false;
	};

	// Pass pointer to a value allocated on caller stack.
	struct ArgumentPassingByPointer{};
	// Almost identical to passing by pointer,
	// but "byval" attribute is used and thus LLVM library pushes temporary copy direct onto the stack.
	// No actual pointer should be passed, value should be accessed by stack offset.
	struct ArgumentPassingInStack{};

	using ArgumentPassing= std::variant<ArgumentPassingDirect, ArgumentPassingByPointer, ArgumentPassingInStack>;

	struct ReturnValuePassingDirect
	{
		llvm::Type* llvm_type= nullptr;
		uint16_t load_store_alignment= 0;
	};

	// Pass as argument #0 a pointer, where returned value should be constructed.
	struct ReturnValuePassingByPointer{};

	using ReturnValuePassing= std::variant<ReturnValuePassingDirect, ReturnValuePassingByPointer>;

public:
	virtual ~ICallingConventionInfo()= default;

	virtual ArgumentPassing CalculateValueArgumentPassingInfo( const Type& type ) = 0;
	virtual ReturnValuePassing CalculateReturnValuePassingInfo( const Type& type ) = 0;
};

using ICallingConventionInfoPtr= std::shared_ptr<ICallingConventionInfo>;

using CallingConventionInfos= std::array<ICallingConventionInfoPtr, g_num_calling_conventions>;

CallingConventionInfos CreateCallingConventionInfos( const llvm::Triple& target_triple, const llvm::DataLayout& data_layout );

} // namespace U
