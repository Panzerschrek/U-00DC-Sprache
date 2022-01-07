%__U_void = type {}

$ust_memory_copy_impl = comdat any
define linkonce_odr void @ust_memory_copy_impl( %__U_void* %dst, %__U_void* %src, i32 %size ) unnamed_addr comdat
{
	%dst_casted = bitcast %"__U_void"* %dst to i8*
	%src_casted = bitcast %"__U_void"* %src to i8*
	call void @llvm.memcpy.p0i8.p0i8.i32(i8* nonnull %dst_casted, i8* nonnull %src_casted, i32 %size, i1 false)
	ret void
}

declare void @llvm.memcpy.p0i8.p0i8.i32(i8* nocapture writeonly, i8* nocapture readonly, i32, i1 immarg) #1

attributes #0 = { argmemonly nounwind }
