// Implement here some utility functions, required by compiler for language features.

static const char g_std_lib_asm[]=
R"(

; ModuleID = 'Ü-std'

declare void @llvm.trap() #0

$__U_halt = comdat any
define linkonce_odr void @__U_halt() unnamed_addr #0 comdat
{
	call void @llvm.trap()
	ret void
}

attributes #0 = { noreturn nounwind }

)";
