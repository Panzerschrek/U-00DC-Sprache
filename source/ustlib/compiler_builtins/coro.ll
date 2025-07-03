;
; llvm intrisics
;

declare i1 @llvm.coro.done(i8* %handle)

;
; Impl functions
;

define linkonce_odr hidden i1 @coro_done_impl( i8* %handle ) unnamed_addr
{
	%res = call i1@llvm.coro.done( i8* %handle )
	ret i1 %res
}
