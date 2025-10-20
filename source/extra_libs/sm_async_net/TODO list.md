This list contains features to be implemented and cases to be considered:

* Support Windows (using *Auxillary Function Driver* mechanisms).
* Use `epoll` on Linux.
* Use `kqueue` on FreeBSD.
* Add a function like `async_sleep` - for effective delaying with possibility to swith to execution of another async functions.
* Support network operations with timeout.
* Consider supporting execution of root tasks with captured references.
* Add a runner method like `execute_task_blocking` to wait for a task to finish.
* Consider adding a method for adding a task, which returns some sort of handle/future, which can be awaited to receive execution result.
* Consider implementing async versions of synchronization primitives like barrier and semaphore, which perform yielding instead of blocking.
