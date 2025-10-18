## sm_async_net

This directory contains a library named *sm_async_net*.
This library is designed for basic networking using `async`/`await` mechanisms provided by the Ü language.
It's only basic, which means that it's pretty limited - it supports only async socket operations (UDP sockets, TCP listener and streams), but nothing else (like async file operations).


### Building

This library requires nothing special for building.
Just use the Ü build system to build it.
In your project you can access it as a versioned package or place it within your project directory and make it into a subproject.


### Supported systems

For now only GNU/Linux and FreeBSD are supported.
Windows support isn't implemented yet (since it a little bit more complex).

*sm_async_net* uses `poll` call to wait for sockets to be ready and dispatch control flow to async functions waiting on these sockets.
It's slightly less performant compared to mechanisms like `epoll` or `kqueue`, but still reasonably fast.


### Usage

The library provides a runner class, which can execute many async functions concurrently.
It does this by creating one or more runner threads for doing this.

```
fn Run()
{
	var ust::box</sm_async_net::runner_interface/> r= sm_async_net::create_runner();

	r.deref().add_task( Func() );
}

fn async Func() {}
```

Additionally it provides classes like `udp_socket`, `tcp_listener`, `tcp_stream`.
They work similar to networking classes from the Ü standard library, but their read/write operations are implemented via async functions.
These async functions should be called only within an async function added into a runner.

```
fn async Func( ust::socket_address_v4 server_address, ust::string8 message )
{
	var sm_async_net::udp_socket mut client_socket= sm_async_net::udp_socket::create_v4().try_take();
	assert( client_socket.send_to( server_address, message.range().to_byte8_range() ).await == message.size() );
}

```

The library also provides several functions like `join_subtasks`, which allow to execute two or more tasks concurrently, but on the same thread.

```
fn Run()
{
	var ust::shared_ptr_mt_final</ ust::semaphore /> finish_semaphore( ust::semaphore( 0u ) );
	var ust::box</sm_async_net::runner_interface/> r= sm_async_net::create_runner();
	r.deref().add_task( Func( finish_semaphore ) );
	finish_semaphore.deref().acquire();
}

fn async Func( ust::shared_ptr_mt_final</ ust::semaphore /> finish_semaphore )
{
	var tup [ i32, f32 ] res= sm_async_net::join_subtasks( FuncA(), FuncB() ).await;
	assert( res[0] == 5 );
	assert( res[1] == 7.3f );
	finish_semaphore.deref().release();
}

fn async FuncA() : i32
{
	return 5;
}

fn async FuncB() : f32
{
	return 7.3f;
}
```


### Usage limitations

Networking classes provided by this library should be only used within a running task executed by a runner.
It's possible to create some of them (where a factory method is non-async) in synchronous code, but async methods should be used only within a running task.

All async functions and methods provided by this library should be used only via `await` expression directly following the function/method call itself.
It's not supported to store an async function object somehow and manually calling `if_coro_advance` for it.
