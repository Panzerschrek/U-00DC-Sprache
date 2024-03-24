Fonctions asynchrones
===============

Les fonction asynchrones sont des coroutines qui renvoient une seule valeur.
Elles sont déclarées en utilisant le mot-clé ``async``.

Les fonctions asynchrones renvoient des valeurs comme les fonctions ordinaires - via l'opérateur ``return``.
Ses règles d'usage sont les mêmes que pour les fonctions ordinaires.

Exemple simple de fonction asynchrone:

.. code-block:: u_spr

   fn async DoubleIt(u32 x) : u32
   {
       return x * 2u;
   }

Dans les fonctions asynchrones, il est possible d'utiliser l'opérateur ``yield`` sans valeur.
Son utilisation met en pause l'exécution de la fonction asynchrone, qui peut être reprise plus tard avec le code qui suit l'opérateur ``yield``.

*********************************
*Usage des fonctions asynchrones*
*********************************

L'appel d'une fonction asynchrone renvoie un object de fonction asynchrone.
Il est possible de démarrer/reprendre l'exécution de l'object de fonction asynchrone via l'opérateur :ref:`if-coro-advance`.

Chaque utilisation de cet opérateur reprend l'exécution de la fonction asynchrone, mais uniquement si elle n'était pas terminée.
La structure de contrôle ne peut être passée à un bloc de ``if_coro_advance`` une fois seulement - quand la fonction asynchrone a terminé et donc renvoyé un résultat.

.. code-block:: u_spr

   // Cette fonction asynchrone est mise en pause plusieurs fois avant de renvoyer un résultat.
   fn async SimpleFunc() : i32
   {
       yield;
       yield;
       yield;
       return 555444;
   }
   
   fn Foo()
   {
       auto mut f= SimpleFunc();
       auto mut result= 0;
       // Exécute l'opérateur "if_coro_advance" jusqu'à que la fonction asynchrone soit terminée.
       // 3 itérations complètes de la boucle vont être exécutées (égal au nombre d'opérateurs "yield" dans le corps de la fonction asynchrone), un break (arrêt) de la boucle se produira à la 4ème itération.
       loop
       {
           if_coro_advance( x : f )
           {
               result= x;
               break;
           }
       }
   }

Il est important de faire attention avec l'utilisation de ``if_coro_advance`` dans une boucle jusqu'à qu'un résultat soit obtenu.
Si une fonction asynchrone est déjà terminée, ``if_coro_advance`` ne renverra jamais de résultat and la boucle ne se terminera pas.
Pour éviter cela, il est nécessaire de vérifier par son objet si la fonction asynchrone est déjà terminée avant d'entrer dans la boucle avec ``if_coro_advance``.

*******************
*L'opérateur await*
*******************

Il existe un opérateur ``await`` qui simplifie les appels de fonctions asynchrones.
Cet operator est postfixe et consiste d'un point (``.``) et du mot-clé ``await``, et peut être utilisé sur un objet de fonction asynchrone à l'intérieur d'une autre fonction asynchrone.

L'opérateur ``await`` fonctionne de la manière suivante : il reprend l'exécution de la fonction asynchrone passée, si elle est terminée - en extrait le résultat, sinon la fonction asynchrone appelant met en pause son exécution et une fois reprise, la structure de contrôle sera passée au code, qui reprendra de nouveau l'exécution de la fonction asynchrone passée, etc. jusqu'à que l'exécution de la fonction asynchrone passée soit terminée.

Cet opérateur est un peu près équivalent au code suivant :

.. code-block:: u_spr

   loop
   {
       if_coro_advance( x : f )
       {
           // x - attend le résultat de l'opérateur.
           break;
       }
       else
       {
           yield;
       }
   }

L'opérateur ``await`` requiert que la valeur passée soit la valeur immédiate de type fonction asynchrone.
Il est également nécessaire que la fonction passée ne soit pas encore terminée, dans le cas contraire ``halt`` ne sera jamais exécutée.
Après avoir obtenu le résultat de l'exécution, l'objet de fonction asynchrone passée est détruit correctement.

Exemple d'utilisation de l'opérateur ``await`` :

.. code-block:: u_spr

   fn async Foo( i32 x ) : i32;

   fn async Bar( i32 x, i32 y ) : i32
   {
       auto foo_res= Foo( x * y ).await;
       return foo_res / 3;
   }

En réalité, l'opérateur ``await`` est juste une manière de simplifier l'appel de fonction asynchrone depuis une autre fonction asynchrone.
Là où l'opérateur d'appel ordinaire est utilisé pour les fonctions ordinaires, l'opérateur d'appel suivi par l'opérateur ``await`` est utilisé pour les fonctions asynchrones à la place.

*****************************
*Le type fonction asynchrone*
*****************************

Le type fonction asynchrone est le type de l'object d'une fonction asynchrone.
Les fonctions asynchrones renvoient des objets de type fonction asynchrone.

Ü a une syntaxe spéciale pour spécifier le type des fonctions asynchrones.
Elle consiste du mot-clé ``async``, de la notation optionnelle pour la spécification des références internes, d'une étiquette optionnelle ``non_sync``, du type de retour (avec/sans modificateur de référence).

.. code-block:: u_spr

   type IntAsyncFunc= async : i32; // Fonction asynchrone simple
   var [ [ char8, 2 ], 1 ] return_references[ "0a" ];
   type FloatRefAsyncFunc= async'imut' : f32 & @(return_references); // Une fonction asynchrone qui renvoie une référence et stocke les références à l'intérieur.
   type NonSyncRefAsyncFunc= async'mut' non_sync : u64 &mut @(return_references); // Fonction asynchrone non_sync qui renvoie une référence immuable et stocke les références muables à l'intérieur.

Comme il peut être constaté, le type fonction asynchrone n'est pas strictement affecté par les détails d'une fonction asynchrone spécifique (par laquelle il a été créé).
Cela permet d'utiliser la même variable pour stocker l'objet de fonction asynchrone produit par les appels de fonctions asynchrones différentes - avec des corps et paramètres différents. 

.. code-block:: u_spr

    // Fonctions asynchrones. Leur type de retour est (async : i32).
   fn async Foo(i32 x, i32 y) : i32;
   fn async Bar() : i32;
    // Une fonction qui renvoie un objet de fonction asynchrone mais qui n'est pas asynchrone.
   fn CreateFunc(bool cond) : (async : i32)
   {
       return select(cond ? Foo( 14, 56 ) : Bar() );
   }
