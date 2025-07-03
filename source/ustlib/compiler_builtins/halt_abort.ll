; ModuleID = 'Ü-std'

attributes #0 = { cold noreturn nounwind }

; C library function
declare void @abort() #0

;
; halt
;

define linkonce_odr hidden void @__U_halt() unnamed_addr #0
{
	call void @abort()
	unreachable
}
