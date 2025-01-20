#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/TargetParser/Triple.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "ustlib.hpp"

namespace U
{

bool LinkUstLibModules(
	llvm::Module& result_module,
	const HaltMode halt_mode,
	const bool no_system_alloc )
{
	// Generated by "bin2c.cmake" arrays.
	#include "bc_files_headers/alloc_libc_32.h"
	#include "bc_files_headers/alloc_libc_64.h"
	#include "bc_files_headers/alloc_winapi_32.h"
	#include "bc_files_headers/alloc_winapi_64.h"
	#include "bc_files_headers/alloc_dummy.h"
	#include "bc_files_headers/atomic.h"
	#include "bc_files_headers/coro.h"
	#include "bc_files_headers/checked_math.h"
	#include "bc_files_headers/halt_abort.h"
	#include "bc_files_headers/halt_configurable.h"
	#include "bc_files_headers/halt_trap.h"
	#include "bc_files_headers/halt_unreachable.h"
	#include "bc_files_headers/math.h"
	#include "bc_files_headers/memory_32.h"
	#include "bc_files_headers/memory_64.h"
	#include "bc_files_headers/volatile.h"

	// Prepare stdlib modules set.
	#define STRING_REF(x) llvm::StringRef( reinterpret_cast<const char*>(c_##x##_file_content), sizeof(c_##x##_file_content) )

	llvm::StringRef halt_module= STRING_REF(halt_trap);
	switch(halt_mode)
	{
	case HaltMode::Trap:
		halt_module= STRING_REF(halt_trap);
		break;
	case HaltMode::Abort:
		halt_module= STRING_REF(halt_abort);
		break;
	case HaltMode::ConfigurableHandler:
		halt_module= STRING_REF(halt_configurable);
		break;
	case HaltMode::Unreachable:
		halt_module= STRING_REF(halt_unreachable);
		break;
	};

	const bool is_32_bit= result_module.getDataLayout().getPointerSizeInBits() == 32u ;

	const llvm::Triple triple( result_module.getTargetTriple() );
	const bool is_windows= triple.isOSWindows();

	const llvm::StringRef asm_funcs_modules[]=
	{
		STRING_REF(atomic),
		STRING_REF(coro),
		STRING_REF(checked_math),
		halt_module,
		STRING_REF(math),
		no_system_alloc
			? STRING_REF(alloc_dummy)
			: ( is_windows
				? ( is_32_bit ? STRING_REF(alloc_winapi_32) : STRING_REF(alloc_winapi_64) )
				: ( is_32_bit ? STRING_REF(alloc_libc_32) : STRING_REF(alloc_libc_64) ) ),
		is_32_bit ? STRING_REF(memory_32) : STRING_REF(memory_64),
		STRING_REF(volatile),
	};
	#undef STRING_REF

	// Link stdlib with result module.
	for( const llvm::StringRef& asm_funcs_module : asm_funcs_modules )
	{
		llvm::Expected<std::unique_ptr<llvm::Module>> std_lib_module=
			llvm::parseBitcodeFile(
				llvm::MemoryBufferRef( asm_funcs_module, "ustlib asm file" ),
				result_module.getContext() );

		if( !std_lib_module )
		{
			std::cerr << "Internal compiler error - stdlib module parse error: " << std::endl;
			auto ignore= llvm::handleErrors(
				std_lib_module.takeError(),
				[]( const llvm::ErrorInfoBase& base )
				{
					std::cerr << base.message() << std::endl;
				} );
			return false;
		}

		std_lib_module.get()->setDataLayout( result_module.getDataLayout() );

		std::string err_stream_str;
		llvm::raw_string_ostream err_stream( err_stream_str );
		if( llvm::verifyModule( *std_lib_module.get(), &err_stream ) )
		{
			std::cerr << "Internal compiler error - stdlib module verify error:\n" << err_stream.str() << std::endl;
			return false;
		}

		llvm::Linker::linkModules( result_module, std::move(std_lib_module.get()) );
	}

	return true;
}

} // namespace U
