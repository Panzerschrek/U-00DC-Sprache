;
; llvm intrisics
;

declare i1 @llvm.coro.done(i8* %handle)
declare i8* @llvm.coro.promise(i8* %handle, i32 %alignemnt, i1 %from)
declare void @llvm.coro.resume(i8* %handle)

;
; Impl functions
;

$coro_done_impl = comdat any
define linkonce_odr hidden fastcc i1 @coro_done_impl( i8* %handle ) unnamed_addr comdat
{
	%res = call i1 @llvm.coro.done( i8* %handle )
	ret i1 %res
}

$coro_resume_impl = comdat any
define linkonce_odr hidden fastcc void @coro_resume_impl( i8* %handle ) unnamed_addr comdat
{
	call void @llvm.coro.resume( i8* %handle )
	ret void
}

$coro_return_value_address_impl = comdat any
define linkonce_odr hidden fastcc i18* @coro_return_value_address_impl( i8* %handle ) unnamed_addr comdat
{
	; TODO - maybe pass correct alignment?
	%res = call i8* @llvm.coro.promise( i8* %handle, i32 1, i1 0 )
	ret i8* %res
}
