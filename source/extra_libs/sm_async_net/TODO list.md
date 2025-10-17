This list contains features to be implemented and cases to be considered:

* Add a function like `async_sleep` - for effective delaying with possibility to swith to execution of another async functions.
* Support multiple runner threads.
* Consider a case where a registered child async function is transferred out of current runner context and executed there.
* Handle cases when an async function finishes prior to its children functions.
* Handle destruction of async functions (including children functions) during runner shutdown.
* Handle destruction of children async functions triggered within their parent async functions.
* Handle a case where an async runner is created inside an async function runned by another async runner instance.
* Handle a case where an async function is destroyed and it has some children functions, which already produced a result, but this result wasn't consumed yet and thus requires destruction.
* Stabilize naming of various classes, functions, types, etc.
* Write stress-tests for large quantities of concurrent async functions.
* Limit tasks queue size.
* Limit per-thread number of running tasks.
* Optimize allocations usage, try perform as few allocations as possible, but without using fixed-sized buffers for everything.
* Consider supporting execution of tasks with captured references.
* Add a method like `execute_task_blocking` to wait for a task to finish.
* Consider adding a method for adding a task, which returns a handle, which can be awaited to receive execution result.
* Consider a possibility for the `add_task` free function to return an error - in case if no active runner thread is present, in case of shutdown, if tasks queue is full, etc.
* Consider implementing async versions of synchronization primitives like "barrier" and "semaphore", which perform yielding instead of blocking.
* Handle cases where a subtask is cancelled via actions of its parents task, so that there is no concurrent mutable access to the tasks map.
* Handle proper runner cancellation, including cancellation of registered socket operations and subtasks, so that there is no concurrent mutable access to the tasks map.
* Consider adding a function for file descriptors limit increasing.
* Investigate a strange bug when `poll` call on GNU/Linux blocks for a while (around 0.5 seconds) when using with large number of descriptors (around 225), which happens even with zero timeout.
