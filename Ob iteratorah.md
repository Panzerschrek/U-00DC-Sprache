Bylo by neploho dobavitj v standartnuju biblioteku iteratory.

Iterator - eto kakoj-libo klass, kotoryj pozvoläjet posledovateljno obhoditj nekotoruju posledovateljnostj, budj to posledovateljnostj elementov v kontejnerah, ili že dinamiceski-generirujemaja posledovateljnostj.

Po suti klass, ctoby bytj iteratorom, dolžen soderžatj toljko metod _next_, kotryj by vozvrascal _optional_ ili _optional_ref_.
Kod, kotoryj ispoljzujet iterator, dolžen otrabatyvatj do teh por, poka rezuljtat _next_ ne pust.

Krome metoda _next_ iteratory dolužny imetj sledujuscije metody:

* _all_ - proveritj, cto vse elementy udovletvoräjut usloviju
* _any_ - proveritj, cto kak minimum odin element udovletvoräjet usloviju
* _chain_ - sozdatj iterator iz dvuh drugih posledovateljnyh iteratorov
* _count_ - poscitatj kolicestvo elementov (linejno)
* _filter_ - propustitj kakije-libo elementy
* _filter_map_ - propustitj kakije-libo elementy, a ostaljnyje preobrazovatj
* _first_ - vernutj pervyj element (jesli jestj)
* _fold_ - obojti vse znacenija i posledovateljno primenitj k nim nekuju binarnuju operaçiju
* _last_ - vernutj poslednij element (jesli jestj)
* _map_ - preobrazovatj element vo vremä iteraçii
* _nth_ - vernutj element pod zadannym nomerom (jesli jestj)
* _position_ - najti nomer pervogo elementa, udovletvoräjuscego zadannomu usloviju
* _reduce_ - kak _fold_, no vozvrascajet pustoj rezuljtat, jesli posledovateljnostj pustaja
* _skip_/_skip_while_ - propustitj pervyje neskoljko elementov
* _take_/_take_while_ - sozdatj iterator, kotoryj ogranicivajet razmer ishodnogo pervymi neskoljkimi elementami
* _zip_ - objedinitj s drugim iteratorom, proizvodä pary znacenij

Vyše opisano množestvo metodov iteratorov.
Na praktike realizovyvatj ih vse dlä vseh tipov iteratorov nakladno.
Poetomu iteratory imejet smysl realizovatj sledujuscim obrazom:
jestj množestvo klassov iteratorov, realizujuscih toljko metod _next_ i jestj klass-obörtka dlä nih s realizaçijej vseh vyšeizložennyh metodov.
Metody, sozdajuscije iteratory opredelönnogo tipa (vrode _map_), sozdajut nekij _raw_map_iterator_ klass, kotoryj imejet toljko sootvetstvujuscuju realizaçiju _next_, posle cego etot klass zavoracivajetsä v obörtku.

Metod _next_ dolžen suscestvovatj kak minimum v tröh vidah - s rezuljtatom tipov _optional_, _optional_ref_mut_, _optional_ref_imut_.
Dlä nekotoryh tipov operatorov etot tip opredeläjetsä cerez _ValueType_ rezuljtata peredannogo funktora.
No etot _ValueType_ iz koda programmy možno uznatj ne vsegda - toljko jesli peredannyj funktor imejet nešablonnyj operator `()` ( cerez inspekçiju `typeinfo</T/>.functions_list` ).
Etogo dostatocno dlä funktorov-ukazatelej na funkçiju i lämbd, no možet bytj nedostatocno dlä drugih funktorov.
Posemu nado kak-to obojti etu problemu.
V krajen slucaje možno datj programmistu vozmožnostj ukazatj tip rezuljtata metoda _next_ javno.

Metody klassa iteratora-obörtki, prinimajuscije funktor, dolžny sozdavatj klassy iteratorov, hranäscije vnutri sebä znacenije etogo funktora.
Eto potrebujet ukazanije ssylocnoj notaçii dlä polej a takže dlä metoda _next_.
Vozmožno daže eto potrebujet inspekçii ssylocnoj notaçii metoda `()` funktora.

Stoit rassmotetj vozmožnostj suscestvovanija obrascajemyh iteratorov - dlä iteraçii po posledovateljnostäm v obratnom porädke.

Stoit izbegatj ispoljzovanije `unsafe` v kode realizaçii iteratorov.
V etom ne dolžno bytj neobhodimosti.
Otsutstvije `unsafe` pozvolit udostoveritjsä cto kod iteratorov napisan praviljno s tocki zrenija proverki ssylok.

Mogut bytj problemy s realizaçijej iteratorov vrode _enumerate_ i _zip_.
Metod _next_ etih iteratorov dolžen vozvrascatj nekuju struktury, vozmožno so ssylkoj ili paroj ssylok vnutri.
Korteži vozvrascatj ne polucitjsä, ibo v kortežah ssylki ne vozmožny.

V sam jazyk nado budet vnesti nekotoryje izmenenija.
Nužno nalicije ssylocnoj notaçii v `typeinfo` funkçij.
Takže budet poleznym avtomaticeskoje preobrazovanija nabora funkçij s odnoj funkçijej v ukazatelj na funkçiju.
