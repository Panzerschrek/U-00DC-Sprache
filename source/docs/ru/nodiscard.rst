nodiscard
=========

Структуры и классы могут быть помечены флагом ``nodiscard``.
Возвращаемое значение типа с этим флагом нельзя игнорировать - нужно его использовать, передав куда-то дальше или инициализировать им переменную.

.. code-block:: u_spr

   struct SomeStruct nodiscard { i32 x; }
   fn Bar() : SomeStruct;
   fn Foo()
   {
       Bar(); // Ошибка - игнорируется значение "nodiscard" типа.
   }

Для классов флаг ``nodiscard`` указывается после возможных вида класса и списка родительских классов:

.. code-block:: u_spr

   class SomeInterface interface {}
   class SomeClass final : SomeInterface nodiscard {}

Также как ``nodiscard`` могут быть помечены перечисления:

.. code-block:: u_spr

   enum Ampel nodiscard { Rot, Gelb, Gruen }
   enum Richtungen : u32 nodiscard { Nord, Sued, Ost, West }


nodiscard для функций
---------------------

Функции также могут быть помечены как ``nodiscard``.
Результат вызова таких функций (значение или ссылку) необходимо использовать.

.. code-block:: u_spr

   fn nodiscard Bar() : i32;
   fn Foo()
   {
       Bar(); // Ошибка - игнорируется "nodiscard" значение.
   }

Лямбды тоже могут быть помечены как ``nodiscard``

.. code-block:: u_spr

   fn Foo()
   {
       auto f= lambda nodiscard () : i32 { return 654; };
       f(); // Ошибка - игнорируется "nodiscard" значение.
   }


nodiscard для временных переменных
----------------------------------

Переменные, полученные через вызов конструктора в контексте выражения, помечаются как ``nodiscard``.

.. code-block:: u_spr

   fn Foo( i32 x )
   {
       u32( x ); // Ошибка - игнорируется "nodiscard" значение типа "u32".
   }
