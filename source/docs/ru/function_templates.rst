Шаблоны функций
===============

В Ü есть возможность создавать абстрактные функции и методы, параметрезуемые другими типами или значениями.
Такие функции называются шаблонными.

Для объявления шаблонной функции надо объявить её с использованием ключевого слова "template" и перечислить список параметров, так же, как в шаблонах типов.
Инстанцировать шаблонную функцию можно, вызвав её.

.. code-block:: u_spr

   template</ type T />
   fn min( T &imut a, T &imut b ) : T &imut
   {
       if( a < b ) { return a; }
       return b;
   }
   
   template</ type T, size_type S />
   fn FillWithZeros( [ T, S ] &mut arr )
   {
       foreach( &mut el : arr )
       {
           el= T(0);
       }
   }
   
   fn Foo()
   {
       var i32 x= min( 55, 9 );
       var [ f64, 4 ] mut arr[ 1.0, 1.0, 1.0, 1.0 ];
       FillWithZeros( arr );
   }

Параметры шаблона выводятся из переданных аргументов функции. Если вывести их невозможно, нужно указать явно все параметры или только сколько-то первых из них.

.. code-block:: u_spr

   template</ type T />
   fn GetPi() : T
   {
       return T(3.1415926535);
   }
   
   auto constexpr pi_f= GetPi</f32/>();
   auto constexpr pi_d= GetPi</f64/>();

*********************************************
*Специализация и перегрузка шаблонов функций*
*********************************************

Можно объявить несколько шаблонных и нешаблонных функций в одном пространстве имён с одним и тем же именем.
При вызове функции будет инстанцироваться наиболее специализированная шаблонная функция. Правила специализации такие же, как в специализации шаблонов типов.

.. code-block:: u_spr

   template</ type T />
   fn GetSequenceSize( T& t ) : size_type // Функция для произвольных типов
   {
       return 0s;
   }
   
   template</ type T, size_type S />
   fn GetSequenceSize( [T, S] &arr ) : size_type // Специализация для массивов
   {
       return S;
   }
   
   fn constexpr GetSequenceSize( tup[] &t ) : size_type // Нешаблонная функция, специализированная для пустых кортежей. Считается более специализированной, чем предыдущая шаблонная функция.
   {
       return 0s;
   }
   
   template</ type T />
   fn GetSequenceSize( tup[T] &t ) : size_type // Специализация для кортежей с размером 1
   {
       return 1s;
   }
   
   template</ type T, type U />
   fn GetSequenceSize( tup[T, U] &t ) : size_type // Специализация для кортежей с размером 2
   {
       return 2s;
   }
   
   template</ type T, type U, type V />
   fn GetSequenceSize( tup[T, U, V] &t ) : size_type // Специализация для кортежей с размером 3
   {
        return 3s;
   }
   
   var i32 constexpr i= 0;
   static_assert( GetSequenceSize(i) == 0s );
   
   var [ bool, 16 ] constexpr arr= zero_init;
   static_assert( GetSequenceSize(arr) == 16s );
   
   var tup[] constexpr t0= zero_init;
   static_assert( GetSequenceSize(t0) == 0s );
   
   var tup[ f32 ] constexpr t1= zero_init;
   static_assert( GetSequenceSize(t1) == 1s );
   
   var tup[ bool, i32 ] constexpr t2= zero_init;
   static_assert( GetSequenceSize(t2) == 2s );
   
   var tup[ f32, u64, i32 ] constexpr t3= zero_init;
   static_assert( GetSequenceSize(t3) == 3s );

*********************************
*constexpr для шаблонных функций*
*********************************

Так же, как и обычную функцию, шаблонную функцию можно объявить как "constexpr". В таком случае будет проверяться соблюдение "constexpr" требований к каждому инстанцированию функции.
Если же не объявлять шаблонную функцию "constexpr", каждое её инстанцирование будет автоматически помечаться как "constexpr", если соответствующие требования соблюдены, и не будет помечаться, если не соблюдены.
