%__U_void = type {}

;
; C standard library functions
;

declare i8* @malloc( i32 )
declare i8* @realloc( i8*, i32 )
declare void @free( i8* )

; Impl functions

$ust_memory_allocate_impl = comdat any
define linkonce_odr %__U_void* @ust_memory_allocate_impl( i32 %size ) unnamed_addr comdat
{
	%res = call i8* @malloc( i32 %size )
	%res_casted = bitcast i8* %res to %"__U_void"*
	ret %"__U_void"* %res_casted
}

$ust_memory_reallocate_impl = comdat any
define linkonce_odr %__U_void* @ust_memory_reallocate_impl( %"__U_void"* %ptr, i32 %size ) unnamed_addr comdat
{
	%ptr_casted = bitcast %"__U_void"* %ptr to i8*
	%res = call i8* @realloc( i8* %ptr_casted, i32 %size )
	%res_casted = bitcast i8* %res to %"__U_void"*
	ret %"__U_void"* %res_casted
}

$ust_memory_free_impl = comdat any
define linkonce_odr void @ust_memory_free_impl( %"__U_void"* %ptr) unnamed_addr comdat
{
	%ptr_casted = bitcast %"__U_void"* %ptr to i8*
	call void @free( i8* %ptr_casted )
	ret void
}

attributes #0 = { argmemonly nounwind }
