### Tekusceje položenije vescej

Na dannyj moment asinhronnyje funkçii vo mnogom shoži s generatorami.
`yield` prosto priostanavlivajet ispolnenije, `return` že zaveršajet jego i poroždajet rezuljtat.

`await` rabotajet cerez `yield` - zapuskajetsä ispolnenije docernej asinhronooj funkçii, i jesli ona ne gotova, priostanavlivajetsä ispolnenije tekuscej funkçii.
Delajetsä eto v çikle - do teh por, poka docernäja funkçija ne zaveršilasj.

V kompilätore realizovan speçialjnyj podhod, kotoryj umejet vstraivatj `async` vyzovy, tem samym prevrascaja tocki ostanovki vyzyvajemoj funkçii v tocki ostanovki vyzyvajuscej funkçii.
Delajetsä eto glavnym obrazom dlä togo, ctoby sokratitj nakladnyje rashody na vozobnovlenije glubokih stekov asinhronnyh funkçij.
Biblioteka "LLVM" umejet v takih slucajah ubiratj vydelenije pamäti iz kuci pod sostojanije korutiny, no samo sostojanije ona ne možet ubratj.

Predpolagajetsä, cto asinhronnyje funkçii v tekuscem ih vide mogut bytj ispoljzovany v nekoj srede vypolnenija s ožidanijem na osnove `poll`.
Samaja glubokaja funkçija, ožidajuscaja sobytija iz soketa (ili cego-to podobnogo) možet pomestitj etot soket v nekuju globaljnuju peremennuju, posle cego priostanovitj svojo vypolnenije i po çepocke priostanovitj vypolnenije kornevoj funkçii.
Ispolnitelj asinhronnyh funkçij možet sobratj eti sokety, vyzvatj dlä nih `poll` i posle etogo vozobnovitj vypolnenije teh funkçij, sokety kotoryh gotovy k cteniju.

V takom podhode imejetsä odin suscestvennyj nedostatok - zapusk i ožidanije srazu neskoljkih asinhronnyh funkçij iz drugoj asinhronnoj funkçii ne možet bytj realizovan dostatocno effektivno.
Pri gotovnosti odnogo soketa, sootvetstvujuscego odnoj vložennoj asinhronnoj funkçii, ispolnitelj asinhronnyh funkçij dolžen zapustitj kornevuju funkçiju, a ta v svoju oceredj vozobnovitj vypolnenije docernej funkçii.
Toljko vot ona ne znajet, kakuju konkretno funkçiju ona dolžna vozobnovitj, i tem samym jej ostajotsä toljko perebiratj ih vseh, cto neeffektivno.


### Vozmožnyje ulucšenija

Bylo by neploho delatj tak, ctoby možno bylo vozobnovitj vypolnenije konkretnoj docernej asinhronnoj funkçii.
Logicnym bylo by sdelatj tak, ctoby sam ispolnitelj asinhronnyh funkçij mog ih naprämuju vozobnovlätj, a ne rabotatj isklüciteljno s kornevymi asinhronnymi funkçijami.

Dlä takogo podhoda sledujet peredelatj rabotu operatora `await`.
On dolžet rabotatj kak `yield`, s tem otlicijem, cto kodu, kotoryj zapustil asinhronnuju funkçiju, nekim sposobom peredajotsä deskriptor asinhronnoj funkçii, kotoruju dannaja funkçija ožidajet.
Boleje togo, operator `await` možno rasširitj do vozmožnosti raboty s neskoljkimi docernimi asinhronnymi funkçijami i peredavatj vyzyvajemomu kodu deskriptory vseh etih asinhronnyh funkçij.

Ispolnitelj asinhronnyh funkçij možet vzätj eti deskriptory, nacatj ispolnenije etih asinhronnyh funkçij i peredatj upravlenije roditeljskoj funkçii, toljko jesli vse docernije funkçii zaveršilisj.
Dlä etogo etot ispolnitelj dolžen podderživatj vnutri sebä nekoje derevo, predstavläjusceje tekuscuju strukturu asinhronnyh vyzovov.
V proçesse svojej raboty on dolžen obhoditj eto derevo, vyzyvatj ispolnenije funkçij (cto menäjet derevo), v tom cisle udaläja uzly zaveršönnyh funkçij.
Listy dereva - samyje glubokije funkçii.
Listy mogut pomecatjsä kak ožidajuscije kakogo-to soketa.
Jesi vse listy ožidajut, možet zapuskatjsä `poll` dlä nih, posle cego vozobnovläjetsä vypolnenije toljko teh listov, sokety kotoryh gotovy, cto možet daljše vyzyvatj vypolnenije vyzvavših ih funkçij.


### Tehnicceskije podrobnosti i nerazrešönnyje voprosy

#### Množestvennyj "await"

`await` so množestvom znacenij v jazyke možno realizovatj dlä massivov asinhronnyh funkçij fiksirovannogo razmera i dlä kortežej, sostojascih toljko iz asinhronnyh funkçij.
Rezuljtat takogo `await` - massiv ili kortež s tipami elementov, sootvetstvujuscim tipu vozvrascajemogo znacenija asinhronnoj funkçii.
Poskoljku massivov ssylok i kortežej ssylok v jazyke ne suscestvujuet, takoj `await` vozmožen toljko dlä asinhronnyh funkçij, vozvrascajuscih znacenije, no ne ssylku.

Ne vpolne jasno, kak delatj `await` dlä massivov s razmerom, opredeläjemym vo vremeni ispolnenija - dlä cego-to vrode `vector` s elementami - asinhronnymi funkçijami.
Odin iz variantov - razrešitj `await` dlä pary ukazatelj + razmer i datj vozmožnostj vrucnuju izvlecj rezuljtaty iz `promise` asinhronnyh funkçij.
Drugoj variant - razbitj ishodnyj massiv na kuski fiksirovannogo razmera nekoj vspomogateljnoj rekursivnoj bibliotecnoj funkçijej.


#### Sposob peredaci deskriptorov docernih asinhronnyh funkçij vyzyvajuscemu kodu

Ne vpolne jasno, kak vernutj iz `await` vyzyvajuscemu kodu spisok deskriptorov docernih asinhronnyh funkçij.
Prämogo sposoba sdelatj eto netu, t. k. funkçija `llvm.coro.resume` znacenija ne vozvrascajet.
No jestj neprämyje sposoby:

* V samoj asinhronnoj funkçii možno v nacale `promise` vydelätj dva polä - razmer i ukazatelj. Ukazatelj ukazyvajet na stekovyj bufer s deskriptorami korutin. Iz vyzyvajuscego koda eti polä možno polucitj cerez vyzov `llvm.coro.promise`.
* Možno ispoljzovatj dlä peredaci globaljnuju `thread_local` peremennuju, s imenem, izvestnym kompilätoru. Kompilätor pri postrojenii `await` pišet v etu peremennuju deskriptory asinhronnyh funkçij. `yield` možet pisatj pustoj spisok.


#### Testirovanije

Zapusk bazovyh testov na asinhronnyje funkçii (napisannyh na "C++" i "Python") možet suscestvenno usložnitjsä.
Sejcas dlä etogo nicego osobogo ne nado, `if_coro_advance` provoracivajet vsü mašineriju asinhronnyh funkçij.
S peredelkoj že, opisannoj vyše, nužno v každom teste realizovyvatj nekij bazovyj vypolnitelj asinhronnyh funkçij.
Kak minimum nado kak-to, nevernäka s ispoljzovanijem `unsafe`, polucatj deskriptory docernih asinhronnyh funkçij i zapuskatj ih.

Sejcas nascityvajetsä okolo 120 testov s ispoljzovanijem `await`.
navernäka oni vse nuždajutsä v pererabotke.
Krome etogo mogut bytj napisany množestvo drugih testov.


#### Dizajn ispolniteljä asinhronnyh funkçij

Stoit produmatj, kak prinçipialjno možet bytj realizovan ispolnitelj asinhronnyh funkçij.
Lucše vsego voobsce, realizovatj bazovyj ispolnitelj v standartnoj biblioteke, ctoby udostoveritjsä, cto vesj podhod realizaçii asinhronnyh funkçij rabotajet kak zadumano.

Stoit podumatj o voprose mnogopotocnosti - kak ona možet rabotatj.
Ona neobhodima v slucajah, kogda nado vyžatj maksimum proizvoditeljnosti, zaispoljzovav boleje odnogo jadra proçessora.
Samyj prostoj variant, eto, navernoje, zapuskatj na každom potoke svoj ispolnitelj i priväzyvatj každuju kornevuju asinhronnuju funkçiju k odnomu iz nih.
No dlä boljšej gibkosti raspredelenija nagruzki etogo možet bytj ne dostatocno.


#### Optimizaçii

Vstajot vopros, kak delo obstoit s optimaljnostju vyšeizložennogo podhoda.

Cto kasajetsä udalenija izlišnih allokaçij pamäti iz kuci - eto skoreje vsego rešajemo.
Biblioteka "LLVM", naskoljko ja mogu suditj, umejet zamenätj allokaçiju iz kuci dlä struktury sostojanija asinhronnoj funkçii allokaçijej so steka, jesli asinhronnaja funkçija sozdajotsä i razrušajetsä v odnoj i toj že funkçii.

Vstraivanije asinhronnyh vyzovov, cto realizovano sejcas otdeljnym samopisnym prohodom, stanovitsä ne stolj važno.
Problema çepocki `switch` instrukçij pri vozobnovlenii asinhronnoj funkçii (po instrukçii na každuju funkçiju v steke vyzovov) ne stoit tak ostro - ibo vozobnovläjutsä naprämuju te asinhronnyje funkçii, kotoryje dolžny vozobnovitjsä, a ne ih roditeljskije funkçii.
No vsö že eta optimizaçija možet bytj polezna dlä slucaja peredaci upravlenija cerez `await` dlä jedinstvennoj docernej funkçii - ctoby sekonomitj nemnogo pamäti na strukturu sostojanija korutiny i sekonomitj na pereklücenii ot odnoj funkçii k drugoj.


#### Razrušenije neispoljzovannyh vozvrascajemyh znacenij

V vyšeizložennom podhode voznikajet problema, kogda castj asinhronnyh funkçij, zapuscenyh cerez `await`, otrabotala i soderžit vozvrascajemoje znacenija, a castj - jescö net.
Pri etom v etot moment roditeljskaja asinhronnaja fukçija možet bytj razrušena (eto ne zapresceno), i kak sledstvije, budut razrušeny docernije asinhronnyje funkçii.
No uže gotovyje vozvrascajemyje znacenija pri etom ne razrušajutsä - predpolagajetsä, cto oni vsegda izvlekajutsä do etogo vyzyvajemoj storonoj.
Poetomu nužno kak-to razrušatj gotovyje vozvrascajemyje znacenija iz vyzyvajuscego koda.

Odno iz rešenij - na `destroy` puti tocki ostanova operatora `await` proverätj status vseh zapuscennyh asinhronnyh funkçij, i jesli kakije-to uže otrabotali - vyzyvatj destruktory dlä ih vozvrascajemogo znacenija.
Po ideje, eto ne siljko dorogo i etot kod budet zapuskatjsä vesjma redko - toljko jesli asinhronnaja funkçija razrušilasj v proçesse.


#### Rucnoj "yield" i vyzov "if_coro_advance"

Nado predusmotretj slucai, kogda vnutri asinhronnoj funkçii vrucnuju vyzyvajetsä `yield`.
V etom slucaje spisok deskriptorov zavisimyh asinhronnyh funkçij, vozvrascajemyj vyzyvajemomu kodu, dolžen bytj pust.
On dolžet daže bytj zanulön na každom vyzove `yield`, v slucaje, jesli tam ostalsä musor ot kakoj-to drugoj asinhronnoj funkçii.

Asinhronnaja funkçija, kotoraja priostanovilasj cerez `yield`, možet bytj tut že snova zapuscena, ibo ona nicego ne ožidajet.
Isklücenije - jesli ona pered etim kuda-to zapisala identifikator soketa (soketov), kotoryje nado ožidatj.
V takom slucaje upravlenije peredastsä etoj asinhronnoj funkçii toljko posle vyzova `poll` (ili cego-to podobnogo).

Takže nado predusmotretj, ctoby `if_coro_advacne` vnutri asinhronnoj funkçii, vyzvannoj dlä drugoj asinhronnoj funkçii, nicego ne lomal.
On dolžen rabotatj, pustj jego ispoljzovanije vmesto `await` i neeffektivno.


#### Peredaca soketov dlä ožidanija

Nizkourovnevyje primitivy asinhronnyh operaçij (sokety i proceje) dolžny kak-to soobscatj vypolnitelü asinhronnyh funkçij, cego oni ožidajut.
Naprimer, oni ožidajut ctenija/zapisi na ukazannom sokete.
Eta informaçija kak-to dolžna peredavatjsä ispolnitelü asinhronnyh funkçij, daby on mog vyzyvatj `poll` ili cto-to shožeje.

Sejcas viditsä, cto kanal peredaci možno realizovatj cerez globaljnyje `thread_local` peremennyje.
Važno pri etom, ctoby ispolnitelj asinhronnyh funkçij ociscal takuju `thread_local` peremennuju, kogda on izvlekajet iz nejo identifikator soketa.

Po-nacalu, dumaju, možno realizovatj podderžku ne boleje odnogo soketa na asinhronnuju funkçiju.
jesli nado boljše - mogut bytj sozdany boljše odnoj asinhronnoj funkçii, kotoryje zapuskajutsä cerez odin `await`.
Boleje togo, navernäka tak budet lucše, cem podderžka množestva soketov dlä odnoj asinhronnoj funkçii - ibo vozobnovlenije koda tak budet bystreje rabotatj (budet vyzvano ctenije/zapisj na nužnom sokete).


#### Zascita ot vozobnovlenija roditeljskoj asinhronnoj funkçii do zaveršenija docernih funkçij

Vozmožno, imejet smysl dobavitj v `await` zascitu ot duraka.
Daby daže jesli asinhronnaja funkçija byla vozobnovlena na tocke s `await`, kogda ne vse zapuscennyje cerez etot `await` docernije asinhronnyje funkçii zaveršilisj, nicego ne lomalosj.
V takom slucaje možno, naprimer, zanovo vyzvatj `await` s deskriptorami teh docernih asinhronnyh funkçij, cto jescö ne zaveršilisj.


### Aljtarnativnyj podhod

Možno uže v tekuscem variante realizaçii asinhronnyh funkçij realizovatj funkçiju, analogicnuju množestvennomu `await` - kak bibliotecnuju funkçiju, vzaimodejstvujuscuju s tekuscim ispolnitelem asinhronnyh funkçij.
Eta funkçija možet stavitj na pauzu vypolnenije zapuskajuscej funkçii i effektivno zapuskatj/vozobnovlätj docernije funkçii.
Interfejs etoj funkçii možet vyglädetj kak-to tak:

```
template</type A, type B/>
fn async multiple_await( (async : A) &mut a, (async : B) &mut b ) : tup[ A, B ];

template</type T, size_type S/>
fn async multiple_await( [ (async : T), S ] &mut f ) : [ T, S ];

template</type />
fn async multiple_await( array_view_mut</ (async : T) /> f ) : vector</T/>;
```

Imä možet bytj drugim - vrode `async_parallel` ili kak-to tak.

Vnutri eti funkçii mogut bytj realizovany dostatocno prosto.
Po suti tam dolžna bytj zapisj peredannyh deskriptorov kuda-to s posledujuscim `yield`.
Posle `yield` idöt bezuslovnoje vytaskivanije rezuljtatov iz `promise` peredannyh asinhronnyh funkçij i ih vozvrat.

Postrojenije i obhod grafa vyzovov možet bytj takim že, kak i v podhode so množestvennym `await` kak osobennosti jazyka.

Ostojotsä voprosom, kak razrušatj rezuljtaty teh funkçij, kotoryje uže zaveršilisj, no rezuljtat kotoryh ne byl izvlecön.
Možno dlä etogo vmeste s deskriptorom asinhronnoj funkçii sohranätj ukazatelj na funkçiju - razrušitelj tipa reazuljtata peredannoj asinhronnoj funkçii.
