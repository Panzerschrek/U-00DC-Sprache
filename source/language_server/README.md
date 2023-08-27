### About

This is an implementation of the [Language Server Protocol](https://en.wikipedia.org/wiki/Language_Server_Protocol) for Ü.

_LanguageServer_ executable reads messages via _stdin_ and outputs messages via _stdout_.
For now this is the only way to perform communications with it (no things like sockets or pipes).

**Work in progress!**

The Language Server is still in the early stage of the development.
Thus there are a lot of unfinished code, incorrect behavior, crashes etc.


#### How to use it

Specify path to the Ü language server executable in settings of your IDE (or settings of its language server plugin).
Additionally it is possible to specify include directories and log file location via command-line options.


#### Supported features
* Go to definition
* Find references
* Replace
* Highlighting
* Symbols tree construction


#### Limitations
* There is only limited possibility to specify include directories - via LSP executable options. Thus there is no way to use different directories for different files.
* There is no way to specify target architecture (for now) and any other code generation option.
* References search and replace can't find symbols outside hierarchy of current document.
* References search, replace, highlighting doesn't work for templates and non-compiled code, like disabled `enable_if` functions or false `static_if` branches.


#### How it works

Basically it just launches lexical/syntax analysis and frontend code building for each opened document.
Such way allows to reuse existing compiler code and provides full language support basically for free.
Downsides of this approach are bad handling of template code, slow processing and high memory usage.
