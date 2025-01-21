; ModuleID = 'Ü-std'

attributes #0 = { cold noreturn nounwind }

; C library function
declare void @abort() #0

;
; halt
;

$__U_halt = comdat any
define linkonce_odr hidden void @__U_halt() unnamed_addr #0 comdat
{
	call void @abort()
	unreachable
}
