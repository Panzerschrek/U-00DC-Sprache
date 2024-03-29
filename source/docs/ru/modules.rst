Модульность
===========

***************
*Модель сборки*
***************

Программа на Ü состоит из одного или нескольких исходных файлов. Каждый такой файл собирается компилятором в платформозависимый объектный файл.
Получившиеся объектные файлы компонуются в итоговый исполняемый файл, статическую или разделяемую библиотеку.

Технически возможна компоновка с объектными файлами, полученными компиляцией исходных файлов других языков, например C или C++.


***********
*Включение*
***********

Иногда бывает необходимо, чтобы одни и те же объявления (функций, классов и т. д.) были доступны в более чем одном исходном файле.
Для этого можно выделить общие объявления в отдельный файл и импортировать его, используя соответствующий механизм.

Для импортирования следует в начале файла исходного кода, до всех объявлений, указать дерективу ``import`` и путь к импортируемому файлу в ``""`` скобках.
Путь может быть абсолютным - начинающимся с ``/``, или относительным в противном случае.
Если путь абсолютный - файл для импорта будет искаться начиная с одной из корневых директорий, которыми можно управлять через соответствующие опции компилятора.
Если путь относительный - файл для импорта будет искаться относительно текущего файла.

.. code-block:: u_spr

   import "a.u" // Импортируем файл, находящийся рядом с текущим
   import "../b.u" // Импортируем файл, находящийся на уровень выше от текущего в файловой системе
   import "cc/c.u" // Импортируем файл, находящийся в директории "cc", находящейся в одной директории с текущим файлом
   import "/d.u" // Импортируем файл, находящийся в корневой директории компилятора

***************************
*Механизм работы включения*
***************************

Включаемый файл компилируется как обычно, возможно с включением других файлов. Объявления из каждого включённого файла объединяются и становятся видимыми в текущем файле.
Порядок включений не влияет на компиляцию включаемых файлов, как и не влияет на файл, в котором происходит включение. При любом порядке набор видимых объявлений будет одним и тем же.

Функции, определённые в импортируемых файлах, имеют ``private`` видимость, дабы не происходило конфликтов при компоновке нескольких объектных файлов, полученных путём соответствующей компиляции исходных файлов, импортирующих один и тот же общий файл.
Также ``private`` видимость имеют сгенерированные функции (специальные методы классов), шаблонные функции и все функции, прямо или косвенно объявленные внутри шаблонов классов.

Использование ``private`` видимости позволяет определять функции в общих файлах, не имея проблем компоновки.
Например, есть три файла, и содержимое одного из них включается в два других:

.. code-block:: u_spr
  :caption: a.u

   fn GetX() : i32 { return 42; }

.. code-block:: u_spr
  :caption: b.u

   import "a.u"

.. code-block:: u_spr
  :caption: c.u

   import "a.u"

При компоновке программы из файлов ``"b.u"`` и ``"c.u"`` конфликтов из-за функции ``GetX()`` не будет.

Также ``private`` видимость имеют функции, определённые в главном (компилируемом) файле, если они не имеют прототипа в одном из импортируемых файлов.
Исключение - функции объявленные как ``nomangle``.
Данная особенность позволяет объявлять функции, видимые только в файле с их объявлением и не создающие конфликтов при компоновке, если даже ``private`` функции с таким же имеем и сигнатурой существуют в других файлах.

.. code-block:: u_spr
  :caption: a.u

   fn SomeLocal(){}

.. code-block:: u_spr
  :caption: b.u

   fn SomeLocal(){}

При компоновке программы из файлов ``"b.u"`` и ``"c.u"`` конфликтов из-за функции ``SomeLocal()`` не будет.


************************************
*Правило единственности определения*
************************************

Каждая сущность итоговой программы на Ü должна быть объявлена только в одном месте.
Не допускается, чтобы существовало более одной сущности с одним и тем же именем (и сигнатурой, для функций).
Исключение составляют ``private`` одноимённые сущности, объявленные в разных исходных файлах.
При импорте компилятор проверяет, что сущности из разных файлов не конфликтуют друг с другом - имена не переопределяются, не существует одноимённых функций с одинаковой сигнатурой и т. д.

Но не во всех случаях компилятор может проверить корректность правила единственности определения.
Программист может объявить локально в двух исходных файлах сущности с не-``pirvate`` видимостью и одним и тем же именем, скомпилировать эти файлы раздельно и попытаться скомпоновать их.
Компилятор такой ошибки обнаружить не сможет, также не гарантируется, что компоновщик её найдёт.
Поэтому ответственность за то, чтобы правило единственности определения не нарушалось, лежит на программисте.
