; ModuleID = 'Ü-std'

attributes #0 = { cold noreturn nounwind }

;
; llvm intrinsics
;

declare void @llvm.trap() #0

;
; halt
;

$_U_halt_handler = comdat any
@_U_halt_handler = global void()* @__U_default_halt_handler, comdat

$__U_default_halt_handler = comdat any
define linkonce_odr void @__U_default_halt_handler() unnamed_addr #0 comdat
{
	call void @llvm.trap()
	ret void
}

$__U_halt = comdat any
define linkonce_odr hidden void @__U_halt() unnamed_addr #0 comdat
{
	%1 = load void ()*, void ()** @_U_halt_handler
	call void %1()
	ret void
}
