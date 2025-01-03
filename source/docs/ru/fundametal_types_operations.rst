Фундаментальные типы и операции над ними
========================================

************
*Пустой тип*
************

Пустой тип ``void`` предназначен в основном для использования в качестве типа возвращаемого значения функций, которым нечего возвращать.
Имеет нулевой размер.
Может быть инициализирован инициализатором по умолчанию.
Имеет единственное значение, все значения типа ``void`` равны между собой.
Значения данного типа можно копировать и присваивать, передавать по значению или по ссылке в функцию и возвращать значения и ссылки данного типа из функций.

****************
*Логический тип*
****************

Логический тип ``bool`` может иметь два значения - ``true`` или ``false``. Над значениями данного типа существуют следующие операции:

* ``&`` - Бинарное логическое И.
* ``|`` - Бинарное логическое ИЛИ.
* ``^`` - Бинарное логическое исключающее ИЛИ.
* ``!`` - Унарное логическое НЕ.
* ``&&`` - Ленивое логическое И.
* ``||`` - Ленивое логическое ИЛИ.

Типом результата всех операций над типом ``bool`` тоже является ``bool``.

***************
*Числовые типы*
***************

В Ü существует ряд числовых типов, делящихся на вещественные и целочисленные. Целочисленные типы в свою очередь делятся на знаковые и беззнаковые.

Вещественные типы:

* ``f32``
* ``f64``

Знаковые целочисленные типы:

* ``i8``
* ``i16``
* ``i32``
* ``i64``
* ``i128``
* ``ssize_type``

Беззнаковые целочисленные типы:

* ``u8``
* ``u16``
* ``u32``
* ``u64``
* ``u128``
* ``size_type``

Числовой суффикс типа означает его размер, в битах.

К значениям числовых типов применимы следующие арифметические операции:

* ``+`` - Бинарное сложение.
* ``-`` - Бинарная разность.
* ``*`` - Бинарное умножение.
* ``/`` - Бинарное деление.
* ``%`` - Взятие остатка от деления.
* ``-`` - Унарный минус. Эквивалентен вычитанию значения из нуля.

Поведение арифметических операций несколько отличается от вида числового типа.
Результат операции для целочисленных типов переполняется, для вещественных - насыщается до бесконечности.

Результатом деления для вещественных чисел является ближайший вещественный результат, а для целых - ближайшее целое.
Для целых чисел не определён результат деления на 0, тогда как для вещественных чисел результат будет равен ±бесконечности или ``NaN``.

**********************************************
*Побитовые операции над целочисленными типами*
**********************************************

Для целочисленных типов применим ряд побитовых операций:

* ``&`` - Бинарное побитовое И.
* ``|`` - Бинарное побитовое ИЛИ.
* ``^`` - Бинарное побитовое исключающее ИЛИ.
* ``~`` - Унарная битовая инверсия.


********
*Сдвиги*
********

К значениям целочисленных типов применимы также операторы битового сдвига влево ``<<`` и вправо ``>>``.
Первым операндом такого оператора является сдвигаемое число, вторым - количество бит, на которое нужно осуществить сдвиг.

Оператор битового сдвига ``<<`` сдвигает число влево на указанное количество бит. Старшие разряды теряются, младшие заполняются нулями.

Оператор битового сдвига ``>>`` сдвигает число вправо на указанное количество бит. Младшие разряды теряются, старшие заполняются либо нулём (для беззнаковых чисел), либо битом знака (для знаковых чисел).

***********
*size_type*
***********

``size_type`` - это беззнаковый целочисленный тип, отличный от других беззнаковых целых.
Его размер равен размеру указателя и поэтому зависит от целевой архитектуры.
Его внутреннее представление аналогично таковому одного из беззнаковых целых типов того же размера.

``ssize_type`` - знаковый аналог ``size_type``.
Результат разницы сырых указателей имеет этот тип.

*****************
*Символьные типы*
*****************

В Ü существуют следующие типы для представления символов:

* ``char8``
* ``char16``
* ``char32``

Числовой суффикс типа означает его размер, в битах.

В отличии от числовых типов, над символьными типами нельзя производить арифметические или побитовые операции. Их можно только сравнивать.

*************************************
*Типы для представления сырых данных*
*************************************

В Ü существуют следующие типы для представления сырых данных:

* ``byte8``
* ``byte16``
* ``byte32``
* ``byte64``
* ``byte128``

Эти типы служат для того, чтобы представлять сырые байты и наборы байтов (2 байта, 4 байта и т. д.).
Из операций для этих типов существует только сравнение на равенство.
Значения `byte`-типов можно преобразовывать в численные и символьные типы соответствующих размеров и наоборот - можно преобразовывать значения `byte`-типов в численные и символьные.
При этом преобразование происходит путём интерпретации битового представления (в том числе для вещественных типов).

***********
*Сравнение*
***********

В Ü есть ряд операторов сравнения. Результат всех операторов сравнения - ``bool``.

Для всех фундаментальных типов определены операторы сравнения на равенство и неравенство - ``==`` и ``!=``.

Кроме этого для всех типов, исключая ``bool``, ``void`` и ``byte``-типы определены следующие операторы упорядочивающего сравнения:

* ``<`` - Меньше.
* ``<=`` - Меньше либо равно.
* ``>`` - Больше.
* ``>=`` - Больше либо равно.

Для числовых типов сравнение происходит по порядку чисел на числовой прямой.
Для символьных типов сравнение происходит по порядковому номеру символа.

В сравнении значений вещественных чисел есть ряд нюансов:

* ``+0`` и ``-0`` имеют различное битовое представление, но при сравнении они идентичны.
* Любое сравнение с ``NaN``, исключая ``!=``, возвращает ``false``.
  ``!=`` с ``NaN ``возвращает всегда ``true``, даже если оба аргумента равны ``NaN``. Также, ``==`` с обоими аргументами равными ``NaN`` возвращает ``false``.
  Из всего этого вытекает, что ``NaN`` не равен никакому другому числу, даже самому себе.

Также для всех типов, для которых определено упорядочивающее сравнение, существует оператор ``<=>``.
Он возвращает результат типа ``i32``, -1 если операнд слева меньше операнда справа, +1 если операнд слева больше оператора справа, 0 если операнды равны.
Также 0 вовзращается, если хотя бы один из операндов является ``NaN``.

*******************
*Троичный оператор*
*******************

В Ü есть оператор выбора одного из двух вариантов - троичный оператор. Он состоит из трёх выражений, разделённых ``?`` и ``:`` в ``()`` скобках.
Тело состоит из логического выражения, выражения для истинного варианта после ``?`` и выражения для ложного варианта после ``:``.
Смысл данного оператора следующий: вычисляется первое выражение, которое должно иметь тип ``bool``.
Если результат первого выражения - истина, вычисляется второе выражение, иначе - вычисляется третье выражение.

.. code-block:: u_spr

   fn Foo()
   {
       auto x= ( true ? 1 : 2 ); // "x" будет равен 1
       auto y= ( false ? 0.5f : 3.5f ); // "y" будет равен 3.5
       var i32 mut z= 0, mut w= 0;
       ( x == 1 ? z : w )= 666; // троичный оператор можно применять в том числе для изменяемых ссылок
   }

**********************
*Приоритет операторов*
**********************

В сложном выражении со множеством операторов вычисление производится с учётом приоритета операторов.
Унарные операторы имеют наивысший приоритет, вычисляются раньше всех остальных.
Бинарные операторы вычисляются в порядке приоритета, от сильного к слабому:

* ``/``, ``*``, ``%``
* ``+``, ``-``
* ``<<``, ``>>``
* ``<=>``
* ``<``, ``<=``, ``>``, ``>=``
* ``==``, ``!=``
* ``&``
* ``^``
* ``|``
* ``&&``
* ``||``

Вышеописанные приоритеты аналогичны таковым в языке C++.
Бинарные операторы с одинаковым приоритетом лево-ассоциативны (вычисляются слева направо).
Если нужно задать отличный от стандартного порядок вычисления, выражение или его часть можно заключить в ``()`` скобки.
