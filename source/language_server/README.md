### About

This is an implementation of the [Language Server Protocol](https://en.wikipedia.org/wiki/Language_Server_Protocol) for Ü.

_LanguageServer_ executable reads messages via _stdin_ and outputs messages via _stdout_.
For now this is the only way to perform communications with it (no things like sockets or pipes).


#### How to use it

Specify path to the Ü language server executable in settings of your IDE (or settings of its language server plugin).
Additionally it is possible to specify include directories and log file location via command-line options.


#### Supported features
* Go to definition
* Find references
* Replace
* Highlighting
* Symbols tree construction
* Completion
* Signature help


#### Limitations
* There is only limited possibility to specify include directories - via LSP executable options. Thus there is no way to use different directories for different files.
* Target arhitecture specification is pretty limited.
* References search and replace can't find symbols outside hierarchy of current document.
* References search, replace, highlighting, doesn't work sometimes for templates and non-compiled code, like disabled `enable_if` functions or false `static_if` branches.


#### Known issues
* Document symbols ranges are wrong - there is no proper range calculation for namespaces, classes, functions, etc.
* Single identifier-like char sequences inside strings and comments are highlighted like proper names.
* Go to defenition, search, replace may rarely return wrong result if documents are rapidly changed and are syntactically-incorrect in moment of the request.
* Completion for "." may return internal class types, which is not allowed by the language itself.


#### How it works

Basically it just launches lexical/syntax analysis and frontend code building for each opened document.
Such way allows to reuse existing compiler code and provides full language support basically for free.
Downsides of this approach are bad handling of template code, slow processing and high memory usage.
