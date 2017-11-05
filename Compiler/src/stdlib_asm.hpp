// Implement here some utility functions, required by compiler for language features.

static const char g_std_lib_asm[]=
R"(

; ModuleID = 'Ü-std'

; Function Attrs: noreturn nounwind
declare void @llvm.trap() #0

; Function Attrs: noreturn nounwind
define void @__U_halt() #0
{
	call void @llvm.trap()
	ret void
}

attributes #0 = { noreturn nounwind }

)";
