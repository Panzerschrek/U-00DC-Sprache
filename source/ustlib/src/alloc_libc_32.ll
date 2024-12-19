;
; C standard library functions
;

declare i8* @malloc( i32 )
declare i8* @realloc( i8*, i32 )
declare void @free( i8* )

; Impl functions

$ust_memory_allocate_impl = comdat any
define linkonce_odr hidden i8* @ust_memory_allocate_impl( i32 %size ) unnamed_addr comdat
{
	%res = call i8* @malloc( i32 %size )
	ret i8* %res
}

$ust_memory_reallocate_impl = comdat any
define linkonce_odr hidden i8* @ust_memory_reallocate_impl( i8* %ptr, i32 %size ) unnamed_addr comdat
{
	%res = call i8* @realloc( i8* %ptr, i32 %size )
	ret i8* %res
}

$ust_memory_free_impl = comdat any
define linkonce_odr hidden void @ust_memory_free_impl( i8* %ptr ) unnamed_addr comdat
{
	call void @free( i8* %ptr )
	ret void
}

attributes #0 = { argmemonly nounwind }
