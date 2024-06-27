## Problema

V Ü poka cto jestj ogranicenije - ssylocnyje polä klassov i struktur ne mogut bytj tipov, kotoryje mogut soderžatj ssylki vnutri.
Eto takže otnositsä k peremennym, zahvacennym po ssylke v lämbdah.
Analogicno eto ogranicenije zatragivajet ssylocnyje parametry funkçij-korutin - generatorov, asinhronnyh funkçij.

V boljšinstve slucajev eto ogranicenije ne vyzyvajet problem.
Obycno klassy/struktury so sslykami vnutri eto prosto klassy dlä kosvennogo dostupa k dannym, vrode `optional_ref` ili `array_view`.
Takije klassy imejet malo smysla hranitj/peredavatj po ssylke, vmesto etogo ih možno prosto kopirovatj, ibo oni pocti vsegda legkovesny.

Odnako byvajut slucai, kogda dannoje ogranicenije vsö-že mešajet.
Naprimer, v lämbdah s zahvatom vseh peremennyh po ssylke neljzä zahvatitj `array_view`, prihoditsä javno zahvatyvatj vse peremennyje po ssylke, krome teh, cto soderžat ssylki vnutri.
V korutinah problema shoža - parametry, kotoryje soderžat ssylki vnutri, prihoditsä peredavatj po znaceniju, cto inogda ne vsegda udobno.


## Pricina problemy

Dannoje ogranicenije vozniklo ocenj davno - jescö do napisanija Kompilätora1.
Pricina etogo do konça ne jasna, sejcas viditsä, cto ona svodilasj k tomu, cto pri nalicii vsego odnogo vnutrennego ssylocnogo tega ne bylo vozmožnosti realizovatj boleje glubokuju kosvennostj ssylok.
Takže ne ponätno bylo, kak v togdašnej ssylocnoj notaçii oboznacatj glubokije ssylki.

V tekuscem podhode proverki ssylok ssylocnaja notaçija ne pozvoläjet obrascenija k glubokim ssylkam.


## Neobhodimostj rešenija

V çelom neobhodimostj ustranitj vyšeopisannoje ogranicenije jestj, pustj i ne ostraja.
Glavnaja pricina, pocemu jego hocetsä ustranitj - jego nalicije v jazyke pridajot jazyku nekij nezakoncennyj ottenok.
Ogranicennostj možet vosprinimatjsä negativno.


## Prinçipy rešenija

Ocenj važno najti takoje rešenije, kotoroje by ne usložnälo prostyje slucai, kogda (kak sejcas eto vozmožno) glubina ssylocnosti ogranicena odnim urovnem.
Pri etom složnyje slucai dolžny bytj obobscenijem prostyh.

Krome togo želateljno bylo by najti takoje rešenije, kotoroje by možno bylo polucitj ne ocenj boljšoj i postepennoj adaptaçijej togo mehanizma proverki ssylok, cto jestj sejcas.
Eto neobhodimo dlä prostoty razrabotki.
Lomajusceje rešenije potrebovalo by jedinovremennogo perepisyvanija znaciteljnoj casti koda kompilätora i vseh sootvetstvujuscih testov, cto bylo by vesjma složno sdelatj.

Stoit sohranitj rešenije s ispoljzovanijem `constexpr` struktur i kortežej dlä ssylocnoj notaçii.
Dannyj podhod zarekomendoval sebä kak dostatocno gibkij i v meru ponätnyj.


## Podhod A - vossozdanije vsei ijerarhii ssylok dlä každoj peremennoj

Jesli bytj tocneje, dlä každoj peremennoj/ssylki možno sozdavatj uzly grafa kontrolä ssylok dlä každoj vnutrennej ssylki, a dlä nih v svoju oceredj svoi uzly dlä ssylok i tak daleje.
Eto obobscajet tekuscij podhod, dobavläja k nemu glubiny.
Pri sozdanii ssylki na peremennuju, vnutrennije ssylocnyje uzly etoj ssylki sväzyvajutsä s takovymi ishodnoj peremennoj, a ih vnutrennije uzly s sootvetstvujuscimi vnutrennimi uzlamui teh uzlov i t. d.

Pri sozdanii uzlov dlä argumentov funkçij sozdajutsä uzly-peremennyje dlä vseh potençialjno-dostupnyh peremennyh (ih dolžno bytj konecnoje kolicestvo).
Dlä etogo ispoljzujetsä informaçija o tipe argumenta i tipah ssylok vnutri nego.

Ne vpolne jasno, kak eta sistema budet socitatjsä s tekuscim podhodom ssylocnyh tegov.
Dannyje tegi javläjutsä nekoj abstrakçijej.
Po ideje možno bylo by etu abstrakçiju ne ispoljzovatj i sozdavatj dlä každoj vnutrennej ssylki svoj docernij uzel.
Vozmožno takže vsö že ostavitj ispoljzovanije ssylocnyh tegov, kombiniruja ih s fakticeskoj ijerarhijej ssylok.

V ssylocnoj notaçii nado kak-to rabotatj s glubokoj ijerarhijej ssylok.
Glubokije ssylki nado kak-to oboznacatj.
Naprimer, možno rasširitj tekuscuju notaçiju i sipoljzovatj cto-to vrode "2ac", "0ba", "1aac".

Notaçiju vozvrascajemyh vnutrennih ssylok tože neobhodimo dorabotatj do boljšej glubiny - ctoby u tegov možno bylo ukazyvatj docernije tegi.

Analogicno so sväzyvanijem ssylok - možet ponadobitjsä ukazyvatj ne toljko sväzj tegov pervogo urovnä, no i sväzj ih vnutrennih tegov.
