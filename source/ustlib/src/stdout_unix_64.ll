;
; Unix functions
;

declare i32 @write( i32, i8*, i64 )

; Impl functions

; STDIN - 0
; STDOUT - 1
; STDERR - 2

$ust_stdout_print_impl = comdat any
define linkonce_odr hidden void @ust_stdout_print_impl( i8* %start, i64 %size ) unnamed_addr comdat
{
	%write_res= call i32 @write( i32 1, i8* %start, i64 %size )
	; TODO - halt if write fails
	ret void
}

$ust_stderr_print_impl = comdat any
define linkonce_odr hidden void @ust_stderr_print_impl( i8* %start, i64 %size ) unnamed_addr comdat
{
	%write_res= call i32 @write( i32 2, i8* %start, i64 %size )
	; TODO - halt if write fails
	ret void
}
