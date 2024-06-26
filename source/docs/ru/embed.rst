Embed
=====

``embed`` - специальный оператор в языке, предназначенный для встраивания содержимого файла в программу в качестве ``constexpr`` массива.

.. code-block:: u_spr

   auto& arr= embed( "some_file.bin" );

Данный оператор принимает ``constexpr`` строку, содержащую имя файла.
Возвращает он ссылку на ``constexpr`` массив элементов типа ``byte8``.

Возможно указать иной тип элемента, отличный от ``byte8``.
Дозволены типы ``char8``, ``i8``, ``u8``.

.. code-block:: u_spr

   auto& extension= ".txt";
   // Встраиваем содержимое файла как массив элементов ``char8``.
   // Обратите внимание, что аргумент оператора "embed" может быть не только строковым литералом, но и более сложным выражением.
   auto& contents_str= embed</char8/>( "some_file" + extension );

Поиск файла для встраивания осуществляется точно так же, как поиск файла в ``import``.
Можно добавить директорию для поиска ``embed`` файлов через ту же опцию компилятора, что и для ``import`` файлов.
