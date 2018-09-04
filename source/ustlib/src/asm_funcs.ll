; ModuleID = 'Ü-std'

attributes #0 = { noreturn nounwind }

declare void @llvm.trap() #0

$__U_halt = comdat any
define linkonce_odr void @__U_halt() unnamed_addr #0 comdat
{
	call void @llvm.trap()
	ret void
}
