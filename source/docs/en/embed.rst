Embed
=====

``embed`` is a special language operator, which is used to embed contents of a given file into a program as ``constexpr`` array.

.. code-block:: u_spr

   auto& arr= embed( "some_file.bin" );

This operator accepts a ``constexpr`` string containing file name.
It returns a reference to a ``constexpr`` array of ``byte8`` elements.

It's possible to specify element type other than ``byte8``.
``char8``, ``i8``, ``u8`` types are allowed.

.. code-block:: u_spr

   auto& extension= ".txt";
   // Embed contents of a file as ``char8`` array.
   // Note that an "embed" operator argument may be not only a string literal, but arbitrary expression.
   auto& contents_str= embed</char8/>( "some_file" + extension );

File search is performed like search for an ``import``.
It's possible to add a directory for ``embed`` files search via the same compiler option as for ``import``.
