This list contains features to be implemented and cases to be considered:

* Add a function like `async_sleep` - for effective delaying with possibility to swith to execution of another async functions.
* Stabilize naming of various classes, functions, types, etc.
* Write stress-tests for large quantities of concurrent async functions.
* Consider supporting execution of tasks with captured references.
* Add a method like `execute_task_blocking` to wait for a task to finish.
* Consider adding a method for adding a task, which returns a handle, which can be awaited to receive execution result.
* Consider a possibility for the `add_task` free function to return an error - in case if no active runner thread is present, in case of shutdown, if tasks queue is full, etc.
* Consider implementing async versions of synchronization primitives like "barrier" and "semaphore", which perform yielding instead of blocking.
* Consider adding a function for file descriptors limit increasing.
