Getting started
===============

*********
*About Ü*
*********

Ü is a compiled statically-typed general purpose programming language with C-like syntax.
Ü supports object-oriented paradigm, allows to write generic code, has some functional programming elements and many other useful features.
Ü is created with error prevention in mind, has safe/unsafe code separation and thus is memory-safe and thread-safe.


*******************
*Getting Ü package*
*******************

It's recommended to download one of the latest Ü builds for your system from Ü project Github page.
There are also downloads on the Ü website, but they are usually slightly outdated.

Ü distribution contains various components, including Ü compiler, build system, standard library, language server and C++ header converter.
But having Ü package isn't enough for building Ü programs.
System-specific thridparty components may be also necessary, like MSVC on Windows, libc and GCC libraries on GNU/Linux, etc.
So, if you get some build error indicating that such components are missing, install them.
For cross-compiling you may also require additional SDKs for the target platform.


*********************
*Hello world program*
*********************

*Hello world* in Ü looks like this:

.. code-block:: u_spr
  :caption: hello_world.u

   import "/main_wrapper.u"
   import "/stdout.u"

   pretty_main
   {
       ust::stdout_print( "Hello, world!\n" );
       return 0;
   }

You can compile it, using Ü build system executable via the following command:

.. code-block:: sh

   u.._build_system build_single hello_world.u

Alternatively you can create an empty Ü project using the command below:

.. code-block:: sh

   u.._build_system init

This command creates a project consisting of two files - ``build.u`` file, containing build instructions and ``main.u`` source file.
After project creation you can build it:

.. code-block:: sh

   u.._build_system build

Having a ``build.u`` file is necessary for building complex programs - with multiple source files, with dependencies, non-trivial properties, etc.
See build system documentation for more details.


**********
*Examples*
**********

There are many examples of basic Ü language usage available `here <https://github.com/Panzerschrek/U-00DC-Sprache/tree/master/source/examples>`_.
Each Ü file in the directory above may be compiled just like a *hello world* program shown previously.
It's recommended to see these examples and read documentation, describing features used in each example.
