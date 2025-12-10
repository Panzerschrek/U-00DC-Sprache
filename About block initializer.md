### Motivation

Sometimes initializing a variable may require several non-trivial steps.
This doesn't just include evaluating an expression, but may require defining other (intermediate) variables, using loops and in case if initialization isn't possible `return`/`break`/`continue` statements.

This all can be solved with usage of existing language and standard library means, but sometimes it looks somewhat ugly and requires too many typing.

Using intermediate variables and loops is possible if initialization code is moved into some other function or lambda.
But this adds a little bit of boilerplate for such function/lambda definition and this way doesn't allow to use control flow transfer statements within context of parent function (`return`/`break`/`continue`).

Sometimes it's possible to just write initialization code "as is" with result variable declaration as last step.
But this approach doesn't allow to limit lifetime of intermediate named variables created for purposes of initialization of the result variable.


### About blocks as expressions

Several programming languages (notably Rust) solve the problem described above by letting blocks to be expressions.
Each block may return a value, which is usually the value of the last expression within this block.
But in case of Ü such approach may be problematic.

Allowing using blocks with all possible block elements in every place where an expression is expected means allowing blocks in places like condition expression of `if` and `while` operators and in `return` statement.
This may create ambiguity if `return`/`break`/`continue` statements are used in such blocks or generally can lead to ugly-looking hardly-readable code.
Limiting usage of blocks for variable initialization only prevents such ambiguities.

Another problem of blocks as expressions used for variables initialization is that the name of a variable is specified before code initializing it.
An example Rust code:

```rust
    let x= {
        let y= 1;
        y * 2
    };
```

In such cases the name of a variable is specified before code initializing it.
This reduces code readability, since such code is actually executed in order reverse compared to seen one (first the initializer block is executed, than the result variable is defined).
It would be better to specify variable name after code block initializing it.


### Proposed solution in Ü

A possible solution to the problem of complex variable initialization is to introduce a new initializer kind for variables defined with `var` keyword and (maybe) with `auto` keyword.
This initializer should consist of a block in `{}` with code doing initialization inside it.
Sine blocks can't return values in Ü, a new control flow transfer operator should be introduced to pass a result value from variable initializer blocks - similar to `return`.

Such block initializer can allow defining variables with lifetime ending just after initializator value is calculated - with no leftover variables polluting current scope.
Also it can allow using `return`/`break`/`continue` statements, if calculating intializer value isn't possible and control flow should be transferred somewhere else.

A simple example of possible syntax:

```
    var i32 {
      auto y= 1;
      block_return y * 2;
    } x;
```

Note that the initializer block is defined before specifying the name of the variable being initialized.
`block_return` statement transfers control flow out of the initializer block and initializes the result variable using a value produced by this statement.

A more complex example:

```
    var i32 a= 0;
    var i32
        {
           var i32 triple_a= a * 3;
           block_return triple_a * triple_a;
        } x, // Define an immutable variable.
        {
            block_return a;
        } & a_ref, // Define a reference.
        {
            var i32 minus_a= -a;
            block_return minus_a * 3 + minus_a * minus_a;
        } mut y; // Define a mutable variable.
```

Using control transfer within an initializer block:

```
fn Foo( i32 x ) : i32
{
    var i32
        {
            if( x == 0 )
            {
                return 0;
            }

            block_return 1024 / x;
        } inv;

    return inv + 128;
}
```

### Typical use-cases

Simplifying success path code when using `ust::optional`:

```
fn Foo()
{
    var i32
        {
            var ust::optional</i32/> mut x_opt= Bar();

            if( x_opt.empty() )
            {
                LogWarning( "Shit happens" );
                return;
            }

            block_return x_opt.try_take();
        } x;

    // A lot of code working with "x".
    // Doing the same using "if_var" library macro is possible, but requires wrapping the whole success path code into a separate block.
    // ...
}

fn Bar() : ust::optional</i32/>;

fn LogWarning( ust::string_view8 s );
```

Hiding/destroying intermediate variables after they aren't needed anymore:

```
fn Foo( ust::array_view_imut</u32/> arr )
{
    var u32
        {
            var u32 mut sum= 0u;

            foreach( x : arr )
            {
                sum+= x;
            }

            block_return sum / u32( arr.size() );
        } average;

    // "sum" variable needed for intermediate claculations does no longer exist in current scope.
    // ...
}
```
