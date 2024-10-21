### Tekusceje položenije

Ü sejcas ne podderživajet ssylocnyje polä v strukturah na tipy, soderžascije ssylki vnutri.
Drugimi slovami, glubina ssylocnoj kosvennosti ne možet bytj boljše 1.
Eto otnositsä takuže k lämbdam - neljzä po ssylke zahvatyvatj tipy so ssylkami vnutri.
Analogicno s korutinami - ssylocnyje parametry na tipy so ssylkami vnutri nevozmožny.

Dannoje ogranicenije uproscajet nekotoryje konçepçii jazyka i vnutrennüju realizaçiju kompilätora.
Naprimer, ne nužno sozdavatj vnutrennih ssylocnyh uzlov dlä vnutrennih ssylocnyh uzlov.
Ili, naprimer, ssylocnaja notaçija funkçij i polej klassov otnositeljno prosta - vsego lišj dvuhurovnevaja.

Eto ogranicenije v çelom slabo skazyvajetsä na vyraziteljnosti jazyka Ü.
No inogda vsö že ona javläjetsä ogranicenijem.

Naprimer, zahvat vsego po ssylke dlä lämbd ne rabotajet dlä zahvata tipov so ssylkami vnutri.
Prihoditsä vrucnuju ukazyvatj dlä nekotoryh peremennyh zahvat po kopii.

Analogicno s korutinami.
Prihoditsä argumenty, soderžascije ssylki vnutri, peredavatj po znaceniju.
Eto osobenno možet bytj neudobno v šablonnom kode, gde zaraneje ne izvestno, jestj li ssylki vnutri tipa.

Problemno ispoljzovatj nekotoryje kontejnery standartnoj biblioteki.
V `ust::vector` možno skladyvatj tipy so ssylkami vnutri, vrode `ust::string_view8`.
No iterirovatjsä po takomu kontejneru uže nevozmožno, t. k. iterator sam soderžit ssylku vnutri - na `ust::vector`, cto sozdajot uže vtoroj urovenj kosvennosti.


### Popytki snätj ogranicenije na odin urovenj kosvennosti

Polnoje snätije vyšeopisannyh ogranicenij verojatno možno bylo by osuscestvitj.
No eto privelo by k suscestvennomu usložneniju jazyka i kompilätora.
Ssylocnaja notaçija stala by mnogourovnevoj, i kak sledstvije siljno zaputannoj.
Sam kompilätor by suscestvenno usložnilsä, cto privelo by k povyšeniju kolicestva ošibok v nöm.

Poetomu polnoje snätije ogranicenij glubiny kosvennosti ssylok sejcas scitajetsä neçelesoobraznym.
Ono sozdajot boljše problem, cem problem rešajet.


### Casticnoje snätije ogranicenij

Jesli ogranicenije snätj polnostju neçelesoobrazno, možno popytatjsä toljko lišj oslabitj jego.
Naprimer, možno razrešitj kosvennostj s urovnem ne boleje 2.
Pri etom budet razrešeno sozdavatj ssylocnyje polä v strukturah na tipy, soderžascije ne boleje odnogo vnutrennego ssylocnogo tega.

Vyšeopisannyj podhod pozvolit v kompilätore prostavlätj vnutrennije ssylocnyje tegi rezuljtatam ctenija ssylocnyh polej, ne pribegaja k neobhodimosti sozdavatj vnutrennije ssylocnyje uzly dlä vnutrennih ssylocnyh uzlov.

Analogicno budet vozmožno sozdanije vnutrennih ssylocnyh uzlov dlä rezuljtatov vyzova funkçij-ssylok, ukazuvajuscih na vnutrennije ssylocnyje uzly argumentov.


### Vozmožnosti casticnogo razrešenija vtorogo urovnä kosvennosti

Pri takom podhode stanet vozmožna rabota kontejnerov vrode `ust::array_view_imut</ ust::string_view8 />`.
Iz nih možno budet vozvrascatj ssylki na hranimyje elementy so ssylkami vnutri.

V lämbdy stanet vozmožnym zahvatyvatj po ssylke i tipy so ssylkami vnutri, cto uprostit ih ispoljzovanije.

Ogranicenija korutin takže mogut bytj oslableny.


### Nevozmožnosti casticnogo razrešenija vtorogo urovnä kosvennosti

V vyšeizložennom podhode vsö jescö budet räd suscestvennyh ogranicenij.

Ssylocnyje polä na tipy, s boleje cem odnim ssylocnym tegom vnutri ne vozmožny, t. k. nalicije boleje cem odnogo tega potrebovalo by vybora kakogo-to konkretnogo vnutrennego ssylocnogo tega ukazyvaimoj peremennoj, a ne jedinstvenno-vozmožnogo, kogda osuscestvläjetsä ctenije ssylocnogo polä.

Bez osoboj ssylocnoj notaçii ne vozmožen vozvrat iz funkçij ssylok vtorogo urovnä kosvennosti.
Ne vozmožno takže i sväzyvanije ssylok dlä nih.


### Rasširenije ssylocnoj notaçii funkçij

Opçionaljnym dopolnenijem k vyšeopisannomu ulucšeniju možet poslužitj rasširenije ssylocnoj notaçii funkçij - daby v nej možno bylo ukazyvatj vnutrennije ssylki vtorogo porädka.
Naprimer, možno bylo by dlä etogo ispoljzovatj bukvy "A-Z".

Nedostatkom takogo rasširenija javläjetsä jego negibkostj.
Ono budet rabotatj toljko dlä urovnä kosvennosti 2 i toljko dlä struktur s odnim ssylocnym tegom vnutri.

Krome togo vvedenije takoj notaçii usložnit mestami ssylocnuju notaçiju, gde ukazano vozvrascenije ssylki, ukazyvajuscej na vnutrennij ssylocnyj teg argumenta.
Pri nalicii notaçii dlä ssylok vtorogo porädka v takom slucaje trebovalosj by i ukazanije vnutrennego ssylocnogo tega dlä tipa vozvrascajemoj ssylki.
