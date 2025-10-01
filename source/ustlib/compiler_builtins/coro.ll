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

$coro_return_value_address_impl_align1 = comdat any
define linkonce_odr hidden fastcc i18* @coro_return_value_address_impl_align1( i8* %handle ) unnamed_addr comdat
{
	%res = call i8* @llvm.coro.promise( i8* %handle, i32 1, i1 0 )
	ret i8* %res
}

$coro_return_value_address_impl_align2 = comdat any
define linkonce_odr hidden fastcc i18* @coro_return_value_address_impl_align2( i8* %handle ) unnamed_addr comdat
{
	%res = call i8* @llvm.coro.promise( i8* %handle, i32 2, i1 0 )
	ret i8* %res
}

$coro_return_value_address_impl_align4 = comdat any
define linkonce_odr hidden fastcc i18* @coro_return_value_address_impl_align4( i8* %handle ) unnamed_addr comdat
{
	%res = call i8* @llvm.coro.promise( i8* %handle, i32 4, i1 0 )
	ret i8* %res
}

$coro_return_value_address_impl_align8 = comdat any
define linkonce_odr hidden fastcc i18* @coro_return_value_address_impl_align8( i8* %handle ) unnamed_addr comdat
{
	%res = call i8* @llvm.coro.promise( i8* %handle, i32 8, i1 0 )
	ret i8* %res
}

$coro_return_value_address_impl_align16 = comdat any
define linkonce_odr hidden fastcc i18* @coro_return_value_address_impl_align16( i8* %handle ) unnamed_addr comdat
{
	%res = call i8* @llvm.coro.promise( i8* %handle, i32 16, i1 0 )
	ret i8* %res
}
