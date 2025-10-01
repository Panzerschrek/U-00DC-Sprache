This list contains features to be implemented and cases to be considered:

* Add a function like `await_all` for effective awaiting for multiple async functions.
* Build, maintain and evaluate async functions tree.
* Implement async UDP socket class.
* Implement async TCP listener class.
* Implement async TCP stream class.
* Use `poll` function for sockets for waiting and dispatching of running async functions.
* Add a free function like `add_task`, which may be used from currently running async function and which adds a task into current async unner.
* Add a function like `async_sleep` - for effective delaying with possibility to swith to execution of another async functions.
* Support multiple runner threads.
* Consider a case where a registered child async function is transferred out of current runner context and executed there.
* Handle cases when an async function finishes prior to its children functions.
* Handle destruction of async functions (including children functions) during runner shutdown.
* Handle destruction of children async functions triggered within their parent async functions.
* Handle destruction of in-flight socket operations.
* Handle a case where an async runner is created insie an async function runned by another async runner instance.
* Handle a case where an async function is destroyed and it has some children functions, which already produced a result, but this result wasn't consumed yet and thus requires destruction.
* Move declarations of types/functions used only internally into private headers.
* Stabilize naming of various classes, functions, types, etc.
* Write stress-tests for large quantities of concurrent async functions.
* Implement a strategy for extracting tasks from the tasks queue, so that computation load is distributed among runner threads evenly.
* Limit tasks queue size.
* Limit per-thread number of running async functions.
* Optimize allocations usage, try perform as few allocations as possible.
