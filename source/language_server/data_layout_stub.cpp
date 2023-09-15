#include "data_layout_stub.hpp"

namespace U
{

namespace LangServer
{

llvm::DataLayout CreateStubDataLayout( const llvm::Triple& target_triple )
{
	// List some common data layouts.
	// TODO - add more.

	if( target_triple.getArch() == llvm::Triple::x86 )
		return llvm::DataLayout( "e-m:e-p:32:32-p270:32:32-p271:32:32-p272:64:64-f64:32:64-f80:32-n8:16:32-S128" );
	if( target_triple.getArch() == llvm::Triple::x86_64 )
		return llvm::DataLayout( "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128" );

	if( target_triple.getArch() == llvm::Triple::arm )
		return llvm::DataLayout( "e-m:e-p:32:32-Fi8-i64:64-v128:64:128-a:0:32-n32-S64" );
	if( target_triple.getArch() == llvm::Triple::armeb )
		return llvm::DataLayout( "E-m:e-p:32:32-Fi8-i64:64-v128:64:128-a:0:32-n32-S64" );

	if( target_triple.getArch() == llvm::Triple::wasm32 )
		return llvm::DataLayout( "e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20" );
	if( target_triple.getArch() == llvm::Triple::wasm64 )
		return llvm::DataLayout( "e-m:e-p:64:64-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20" );

	if( target_triple.getArch() == llvm::Triple::riscv32 )
		return llvm::DataLayout( "e-m:e-p:32:32-i64:64-n32-S128" );
	if( target_triple.getArch() == llvm::Triple::riscv64 )
		return llvm::DataLayout( "e-m:e-p:64:64-i64:64-i128:128-n64-S128" );

	// Return some stub for general case. This is better, than nothing.
	return llvm::DataLayout( "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128" );
}

} // namespace LangServer

} // namespace U
