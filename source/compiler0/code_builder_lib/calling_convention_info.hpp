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

	struct ArgumentPassingByPointer{};
	struct ArguemntPassingInStack{};

	using ArgumentPassing= std::variant<ArgumentPassingDirect, ArgumentPassingByPointer, ArguemntPassingInStack>;

public:
	virtual ~ICallingConventionInfo()= default;

	virtual ArgumentPassing CalculareValueArgumentPassingInfo( const Type& type ) = 0;
};

using ICallingConventionInfoPtr= std::shared_ptr<ICallingConventionInfo>;

using CallingConventionInfos= std::array<ICallingConventionInfoPtr, g_num_calling_conventions>;

CallingConventionInfos CreateCallingConventionInfos( const llvm::Triple& target_triple, const llvm::DataLayout& data_layout );

} // namespace U
