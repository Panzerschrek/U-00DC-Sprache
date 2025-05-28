### Problema

V tekuscem variante klass heš-tabliçy ("unordered_map") ne ocenj udoben k ispoljzovaniju.
Problemu sostavläjut poljzovateljskije tipy, dlä kotoryh nado opredelätj svoj algoritm heširovanija.
Eto neskoljko neudobno, t. k. dlä etogo nužno objavlätj otdeljnyj klass dlä heširovanija.
Krome togo eto ne ocenj praviljno perekladyvatj na poljzovateljskij kod realizaçiju algoritma heširovanija.

Osobenno problematicna realizaçija heširovanija v poljzovateljskom kode v tom plane, cto strogo govorä etot algortim možet bytj razlicnym dlä razlicnyh realizaçij heš-tabliç.
Možet tak slucitjsä, cto poljzovateljskij heš neeffektiven v kakoj-to realizaçii heš-tabliçy, iz-za cego proishodit mnogo heš-kollizij i takim obrazom stradajet proizvoditeljnostj.


### Rešenije

Stoit otdelitj algoritm heširovanija ot polucenija dannyh dlä heširovanija.
Poljzovateljskije tipy dolžny nekim obrazom predostavlätj algoritmu heširovanija ishodnyje dannyje, iz kotoryh dolžen vycislätjsä heš.
Po suti vsö heširovanije svoditsä k manipuläçii çelocislennymi skalärami, imi to i dolžen operirovatj algoritm heširovanija.

V takom podhode budet imetjsä vozmožnostj ispoljzovatj razlicnyje algoritmy heširovanija dlä raznyh realizaçij heš-tabliç.
Krome togo budet vozmožna i otnositeljno prostaja zamena heš-funkçij konkretnoj realizaçii heš-tabliçy, cto možet bytj polezno dlä optimizaçii heš-tabliç pod speçificnyje dannyje.


### Primery realizaçii

V "Rust" rabota s heš-tabliçami organizavana primerno sledujuscim obrazom. (https://doc.rust-lang.org/std/collections/struct.HashMap.html)

Šablon klassa "HashMap" možet bytj parametrizovan tipom - fabrikoj funkçij heširovanija.
Funkçija heširovanija - eta realizaçija tipaža "Hasher", kotoryj trebujet metody dlä heširovanija bazovyh skalärov i polucenija rezuljtata.

Poljzovateljskije tipy, kotoryje ispoljzujutsä v kacestve klücej v "HashMap", dolžny realizovyvatj tipaž "Hash".
Etot tipaž trebujet nalicija šablonnogo metoda s imenem "hash" i parametrom - kakoj-to iz realizaçij tipaža "Hasher".
Jestj vozmožnostj po-prostomu realizovatj tipaž "Hash" dlä svojego tipa, ispoljzuja "derive".


### Realizaçija mehanizmov heširovanija v Ü

Poskoljku v otlicije ot "Rust" v Ü netu tipažej, realizovatj podhod iz "Rust" 1 v 1 ne vyjdet.
No cto-to ocenj shožeje sdelatj možno.

Fakticeski necto vrode tipaža "Hasher" možet suscestvovatj, no v kacestve nekogo slovesnogo opisanija, trebujuscego realizaçiju opredelönnyh metodov ot každogo klassa, ispolnäjuscego rolj "Hasher".
Primerom realizaçii dolžen bytj klass "Hasher", kotoryj ispoljzujetsä po umolcaniju.
Aljternativnyje realizaçii budut bratj s nego primer i kopirovatj jego interfejs.

Kak i prežde osnovnyje vstrojennyje tipy dolžny bytj heširujemy.

Klassy, kotoryje olanirujetsä ispoljzovatj v kacestve klücej v heš-tabliçe, dolžny imetj metod "hash".
Dannyj metod dolžen bytj šablonnym i imetj odin parametr - izmenäjemuju ssylku na etot šablonnyj tip, pod kotorym podrazumevajetsä nekotoraja realizaçija "Hasher".
Rabota etogo metoda dolžna zaklücatjsä v peredace ekzempläru "hasher" dannyh dlä heširovanija i/ili vyzova metoda "hash" dlä polej.

Dolžna suscestvovatj v kakom-to vide vozmožnostj vycislenija hešej dlä massivov i kortežej, tipy kotoryh heširujemy.
Poskoljku takaja realizaçija v obscem ne zavisit ot funkçii heša, kod, kotoryj kombinirujet heši dlä etih tipov možet bytj napisan odin raz i pomescön v standartnuju biblioteku.

Dolžna bytj vozmožnostj heširovatj klassy, ne imejuscije metoda "hash".
V takom slucaje heš dlä nih dolžen kombinirovatjsä na osnove hešej ih polej.
No takoje vozmožno toljko dlä klassov bez ssylocnyh polej i gde vidimostj vseh polej toljko "public".

Osobyje tipy klassov takže dolžny kak-to heširovatjsä avtomaticeski.
K nim otnosätsä korutiny i lämbdy.

Cto kasajetsä poljzovateljskih tipov bez metoda "hash" i bez vozmožnosti avtomaticeski vycislitj heš, suscestvujet vsö že sposob ispoljzovatj ih v kacestve klücej heš-tabliçy.
Dlä etogo nado napisatj klass-obörtku i realizovatj u nego metod "hash".
Eto budet rabotatj, kolj skoro publicnyj interfejs ishodnogo tipa predostavläjet dostatocno informaçii dlä heširovanija.

Klass heš-tabliçy dolžen bytj v buduscem rasširen - v nöm dolžen pojavitjsä tretij šablonnyj parametr - funkçija heširovanija.
Jejo umolcateljnoje znacenije dolžno bytj razvo standartnoj realizaçii "Hasher".


### Daljnejšije dorabotki heš-tabliçy

Posle peredelki heširovanija imejet smysl pereträhnutj klass heš-tabliçy - kak realizaçiju, tak i interfejs.

Realizaçija dolžna bytj prevedena v sootvetstvije s nekotorym širokoispoljzujemym podhodom.
Naprimer, možno realizovatj "Google’s SwissTable".
V lübom slucaje dolžny bytj likvidirovany problemy tekuscej realizaçii, vrode dolgoj iteraçii po heš-tabliçe, kotoraja ranjše byla boljšoj, no opustela.

Interfejs tože dolžen bytj dorabotan, daby uskoritj realizaçiju tipicnyh sposobov ispoljzovanija.
Dolžny pojavitjsä metody dlä vstavki ili izmenenija znacenija za odin poisk v tabiçe.
Dolžen pojavitjsä metod dlä poiska znacenija ili vstavki, jesli jego jescö netu.
