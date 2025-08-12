Код-прелюдия, генерируемый компилятором
=======================================

Компилятор генерирует кое-какой код, который добавляется в каждый исходный файл.
Этот код содержит информацию о самом компиляторе и о том, с каким опциями он запущен.

Пример такого кода:

.. code-block:: u_spr

   namespace compiler
   {
   auto& version = "0.9";
   auto& git_revision = "0000000000000000000000000000000000000000";
   var size_type generation = 1s;
   namespace options
   {
   var char8 optimization_level = '0';
   var bool generate_debug_info = false;
   auto& cpu_name = "";
   var tup[  ] features[  ];
   }
   namespace target
   {
   auto& str = "x86_64-unknown-linux-gnu";
   auto& arch = "x86_64";
   auto& vendor = "unknown";
   auto& os = "linux";
   auto& environment = "gnu";
   auto& os_and_environment = "linux-gnu";
   var bool is_big_endian = false;
   }
   }

Данную информацию можно использовать на своё усмотрение - например, реализуя код различным образом в зависимости от архитектуры/операционной системы.
