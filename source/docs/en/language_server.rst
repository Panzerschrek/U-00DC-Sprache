Language server
===============

Ü has an implementation of the `Language Server Protocol <https://en.wikipedia.org/wiki/Language_Server_Protocol>`_.
This article describes it.

*******
*Usage*
*******

Provide path to the language server executable in settings of your IDE/Text editor with LSP support.
Additionally you may provide some options (see below).


**************
*How it works*
**************

Ü Language server communicates with a client application via *stdin* and *stdout* - it receives JSON requests and sends responses and notifications.

Internally it's implemented atop of the Ü compiler libraries.
For each opened document its compilation (frontend part) is launched in order to collect necessary information.

Since such compilation may be slow, it's performed rarely - only after no document editions were done in last seconds and only if a document is syntactically correct.
Compilation is performed in a background thread, in order to continue processing requests during compilation.


**********
*Features*
**********

Following features are implemented:

* Symbols highlighting
* Symbols declarations search
* Symbols usage search (limited)
* Find and replace
* Symbols tree construction
* Autocompletion
* Signature help

Known limitations:

* Symbols usage search and replace are limited to current document and documents imported by it
* Symbols usage search can't sometimes find usages in templates, in ``static_if`` branches, functions disabled via ``enable_if``, etc.
* Signature help ignores already typed arguments and suggests all available overloaded functions

Know issues:

* Autocompletion doesn't respect private members
* Names within strings and comments are sometimes highlighted
* Workspace info files aren't reloaded on change, language server restart is required


**************************
*Build system interaction*
**************************

Language server tries to find default build directory (named *build*) for each opened document and a workspace description file within it.
Such file is produced on each build by the Ü build system.
Language server does this in order to find proper include directories for a document.
If non-default build directory is used, it may be specified via special command line option (see below).


************************
*Command-line interface*
************************

Many command-line options of the language server work like same options in Ü compiler.

``--include-dir`` option allows to add additional directory for imports search.
More than one directory may be specified.

``--build-dir`` specifies a build directory for searching for a workspace info file.
Multiple build directories may be specified.

``--log-file`` provides a path for language server logs.
Default is empty and means no logs at all.

``--error-log-file`` option allows to write some information into a separate log file, if language server crashes (but not always).

``--num-threads`` option allows to specify number of threads used for background compilation jobs.
Default value is 0, which means using all available CPU cores.

``-O`` and ``-g`` options are like same compiler options, but do a little - basically only affect compiler-generated prelude used within langauge server.
``--target-vendor``, ``--target-os``, ``--target-environment`` options are provided for the same reason.
