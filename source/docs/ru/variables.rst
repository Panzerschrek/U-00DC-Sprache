Переменные
==========

Переменную можно объявить так:

.. code-block:: u_spr

   var i32 x= 0;

Можно объявить сразу несколько переменных одного типа:

.. code-block:: u_spr

   var i32 x= 0, y= 1, z= 2;

Модификаторы ссылки и изменяемости задаются индивидуально для каждой переменной:

.. code-block:: u_spr

   var i32 mut a= 0, imut b= 0;
   var i32 &mut a_ref= a, &imut b_ref= b, & b_ref2= b, imut y= 66, mut z= -56;

********
*Ссылки*
********
Модификатор ссылочности ``&`` указывает, что переменная будет являться ссылкой.
Это значит, что ссылочная переменная будет ссылаться на значение какой-то другой переменной.
Изменение ссылочной переменной приведёт к изменению исходной переменной.

Объявление ссылки не создаёт копию значения, что может быть полезно, когда создание копии есть тяжёлая операция.

**************
*Изменяемость*
**************
Переменная может иметь один из трёх модификаторов изменяемости:

* ``mut`` - переменную можно будет изменять после объявления
* ``imut`` - переменную нельзя будет изменять после объявления
* ``constexpr`` - переменная должна быть константой времени сборки

Если для переменной не указан никакой из модификаторов изменяемости, к ней будет применён модификатор ``imut``.

***************
*Инициализация*
***************
В примерах выше значения всех переменных инициализирующий при объявлении через ``=``.
Это не единственный способ инициализации, в зависимости от типа переменной к ней применимы различные виды инициализаторов, для переменных некоторых типов вообще можно не указывать инициализатор.
Более подробно про инициализацию читайте в :doc:`соответствующей главе </initializers>`.

.. _auto-variables:

*****************
*auto переменные*
*****************

Существует возможность объявить переменную, тип которой будет выведен из типа инициализатора.
Для этого существует специальный синтаксис - объявление ``auto`` переменной. Объявление состоит из ключевого слова ``auto``, опциональных модификаторов ссылочности и изменяемости, имени переменной и инициализатора после ``=``.

.. code-block:: u_spr

   auto x = 0; // Неизменяемая auto переменная. Тип будет равен "i32".
   auto mut y = 0.5f; // Изменяемая auto переменная. Тип будет равен "f32".
   
   auto &mut y_ref= y; // Неизменяемая auto ссылка. Тип будет равен "f32".
   auto &imut x_ref0= x; // Неизменяемая auto ссылка. Тип будет равен "i32".
   auto & x_ref0= x; // Неизменяемая auto ссылка. Неизменяемость выбрана неявно. Тип будет равен "i32".
   
   var [ bool, 16 ] arr= zero_init;
   auto& arr_ref= arr;// Неизменяемая auto ссылка. Тип будет равен "[ bool, 16 ]".

***********************
*Глобальные переменные*
***********************

Переменные можно объявлять также вне тела функций - в глобальном пространстве, в пространствах имён, внутри структур и классов.
Но у таких переменных есть ограничение - они должны быть константами времени компиляции (``constexpr``).

.. code-block:: u_spr

   auto global_var = 55;
   var f32 global_f0= 0.25f, global_f1 = 555.1f;
   
   namespace NN
   {
       auto constexpr nn_var = global_var;
       var bool imut b = global_f0 < 66.0f;
   }
   
   struct S
   {
       var [ i32, 42 ] zeros = zero_init;
       auto constexpr zero24_plus2 = zeros[24] + 2;
   }

**********************************
*Глобальные изменяемые переменные*
**********************************

Глобальные изменяемые переменные во многом аналогичны неизменяемым глобальным переменным.
Для них так же действует требование на ``constexpr`` инициализатор, и они должны быть ``constexpr`` типа.

Доступ к глобальным изменяемым переменным возможен только в ``unsafe`` коде, включая и чтение и запись.
Это необходимо, т. к. для глобальных изменяемых переменных не работает контроль ссылок и отсутствуют механизмы синхронизации.
Программист сам должен реализовать гарантии количества ссылок и обеспечить необходимую синхронизацию доступа.

Объявляются глобальные изменяемые переменные так же, как и неимзеняемые, но с неизменяемые ``mut``.

.. code-block:: u_spr

   auto mut global_int = 66;
   var f32 mut global_float = 0.25f;

Единственное, что существенно отличает изменяемые глобальные переменные от неизменяемых, так это невозможность создания изменяемых ссылок.
Они запрещены ввиду потенциальных проблем с синхронизацией доступа.


thread_local переменные
-----------------------

``thread_local`` переменные - это по сути те же глобальные изменяемые переменные, которые отличаются лишь тем, что каждый поток имеет свою копию такой переменной.
Ограничения для них все те же, что и для других глобальных изменяемых переменных - доступ к ним возможен только из ``unsafe`` кода, возможны только переменные ``constexpr`` типов.
Синтаксис объявления такой переменной особый - требуется указать ключевое слово ``thread_local``, после чего следует имя типа и список переменных (с инициализаторами), перечисляемых через запятую. Модификаторы ссылочности и изменяемости при этом отсутствуют.

.. code-block:: u_spr

   thread_local i32 x= zero_init, y(1), z= 2;
