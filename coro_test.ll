; ModuleID = 'coro_test.bc'
source_filename = "coro_test.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare i8* @malloc(i64)

declare i8* @realloc(i8*, i64)

declare void @free(i8*)

declare void @print(i32)

; Function Attrs: nounwind
declare i8* @llvm.coro.begin(token, i8* writeonly) #0

; Function Attrs: nounwind
declare i1 @llvm.coro.end(i8*, i1) #0

; Function Attrs: argmemonly nounwind readonly
declare i8* @llvm.coro.free(token, i8* nocapture readonly) #1

; Function Attrs: argmemonly nounwind readonly
declare token @llvm.coro.id(i32, i8* readnone, i8* nocapture readonly, i8*) #1

; Function Attrs: nounwind
declare i8 @llvm.coro.suspend(token, i1) #0

; Function Attrs: nounwind readnone
declare i32 @llvm.coro.size.i32() #2

; Function Attrs: nounwind readnone
declare i64 @llvm.coro.size.i64() #2

define i8* @f(i32 %n) {
entry:
  %id = call token @llvm.coro.id(i32 0, i8* null, i8* null, i8* null)
  %size = call i64 @llvm.coro.size.i64()
  %alloc = call i8* @malloc(i64 %size)
  %hdl = call noalias i8* @llvm.coro.begin(token %id, i8* %alloc)
  br label %loop

loop:                                             ; preds = %loop, %entry
  %n.val = phi i32 [ %n, %entry ], [ %inc, %loop ]
  %inc = add nsw i32 %n.val, 1
  call void @print(i32 %n.val)
  %0 = call i8 @llvm.coro.suspend(token none, i1 false)
  switch i8 %0, label %suspend [
    i8 0, label %loop
    i8 1, label %cleanup
  ]

cleanup:                                          ; preds = %loop
  %mem = call i8* @llvm.coro.free(token %id, i8* %hdl)
  call void @free(i8* %mem)
  br label %suspend

suspend:                                          ; preds = %cleanup, %loop
  %unused = call i1 @llvm.coro.end(i8* %hdl, i1 false)
  ret i8* %hdl
}

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind readonly }
attributes #2 = { nounwind readnone }
