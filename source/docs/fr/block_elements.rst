Elements du corps de fonction
=============================

Le corps d'une fonction consiste en une séquence d'élements.
Les plus importants d'entre eux sont listés ci-dessous.

**************************
*Déclaration de variables*
**************************
Voir :doc:`variables`.

*******************************
*Déclaration de variables auto*
*******************************
Voir :ref:`auto-variables`.

*******************
*Expression simple*
*******************

En général, elles sont nécessaires pour appeler une fonction avec des effets de bord.

.. code-block:: u_spr

   Foo(); // Appelle une fonction, qui peut avoir des effets de bord
   Add( 5, 7 ); // Appelle une fonction avec ou sans effet de bord

*************
*Assignation*
*************

L'assignation consiste d'une partie gauche - la destination et une partie droite - la source.

.. code-block:: u_spr

   x = 2; // Assigne une valeur numérique à une variable
   x = Foo() + Bar() - 66; // Assigne le résultat d'une expression complexe à une variable
   Min( x, y )= 0; // Ceci est également possible, si le résultat de la fonction "Min" est une référence muable

*************************************
*Assignation combinée avec opérateur*
*************************************

Les opérations suivantes combinées avec assignation sont :

* ``+=`` - addition
* ``-=`` - soustraction
* ``*=`` - multiplication
* ``/=`` - division
* ``%=`` - reste
* ``&=`` - AND (ET) bit-à-bit
* ``|=`` - OR (OU) bit-à-bit
* ``^=`` - XOR (OU exclusif) bit-à-bit
* ``=<<`` - décalage vers la gauche bit-à-bit
* ``=>>`` - décalage vers la droite bit-à-bit

Une opération avec assignation est équivalent à un appel de l'opérateur binaire correspondant pour les parties gauche et droite de l'opération suivi de l'assignation du résultat à la partie gauche.

.. code-block:: u_spr

   x += 2; // Augmente la valeur de "x" de 2
   x /= Foo() + Bar() - 66; // Divise la varible "x" par le résultat de l'expression à droite et assigne le résultat à "x"
   Min( x, y ) &= 0xFF; // Fixe les bits supérieurs de la variable du résultat de l'appel de "Min" à zéro

**********************************
*Incrémentation et décrémentation*
**********************************

``++`` incrémente le résultat d'une expression numérique de un, ``--`` le diminue.

.. code-block:: u_spr

   ++ x;
   -- x;
   ++ Min( x, y );

***********************************
*Éléments de structure de contrôle*
***********************************
Voir :doc:`control_flow`.

************************************
*static_assert (assertion statique)*
************************************
Voir :doc:`static_assert`.

******
*halt (arrêt)*
******
See :doc:`halt`.

******
*Bloc*
******

Un bloc consiste en une séquence d'élements dans ``{}``.
Il peut inclure les éléments listés ci-dessus et d'autres blocs.

Un bloc est utilisé principalement pour la création de portée (scope) de nouvelles variables.
Une variable définie dans un bloc est visible à l'intérieur de ce bloc et des blocs imbriqués.
Les variables définies dans un bloc ont une durée de vie (lifetime) limitée à la fin de ce bloc.

Il est possible de définir une variable à l'intérieur d'un block avec un nom existant déjà dans un bloc extérieur.
La variable extérieure ne sera donc plus accessible.

.. code-block:: u_spr

   fn Foo()
   {
       var i32 mut x= 0;
       {
            ++x; // Modifie la valeur de la variable extérieure
            var f64 mut x= 3.14; // Définit une variable avec le même nom qu'une variable du bloc extérieur. Désormais, la variable extérieure "x" ne sera plus accessible jusqu'à la fin du bloc actuel.
            x= 0.0; // Modifie une variable de ce bloc.
            var i32 mut y= 0;
       }
       --y; // Erreur - nom "y" non trouvé
   }

Un bloc peut avoir un label.
Ce label peut être utilisé dans les opérateurs ``break`` à l'intérieur de ce bloc.
Dans ce cas, ``break`` ne fonctionne pour ce bloc que si un label est spécifié.
``break`` sans label se rapporte à une boucle en cours, mais pas au bloc marqué avec un label.
``continue`` pour un label d'un bloc n'est pas possible et provoquera la production d'une erreur par le compilateur.

.. code-block:: u_spr

   fn Foo(bool cond)
   {
      {
          if( cond )
          {
              break label block_end;
          }
          // d'autres lignes de code
      } label block_end
   }

Il existe aussi des blocs ``unsafe`` (non-sûrs).
Voir :ref:`unsafe-blocks`.

******************
*L'opérateur with*
******************

Cet opérateur permet d'effectuer une action avec le résultat d'une expression et, si nécessaire, d'étendre la durée de vie de variables temporaires à l'intérieur de cette expression.
Cet opérateur contient une référence optionnelle, des modificateurs de muabilité et un nom pour le résultat de l'expression.

``with`` est utile à utiliser comme alternative à un bloc, à l'intérieur duquel une variable est définie et certaines opérations sont effectuées sur elle, dans les cas où la durée de vie de la variable devrait être limitée.
Il est également utile d'utiliser ``with`` dans du code modèle où il est n'est pas clair si le résultat d'une expression est une variable ou une référence, car ``with`` (contrairement à ``var`` et ``auto``) permet de créer une référence à une variable temporaire.

Exemples d'usage :

.. code-block:: u_spr

   with( x : Foo() )
   {
       Bar(x);
       return x + 1;
   }

.. code-block:: u_spr

   with( &mut x : s.Get() )
   {
       ++x;
   }
