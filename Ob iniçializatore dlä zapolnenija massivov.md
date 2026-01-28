### Motivaçija

Inogda suscestvujet potrebnostj iniçializirovatj massiv (so staticeskim razmerom).
Pri etom byvajet tak, cto "zero_init" ne podhodit ili ne dostupen.
Konstruktor po umolcaniju tože inogda ne možet bytj ispoljzovan.
Tak cto ostajotsä toljko perecislenije iniçializatorov dlä každogo elementa po otdeljnosti.
Eto terpimo dlä neboljših massivov (vrode 4 elementov), no dlä boljših kolicestv elementov eto sozajot sliškom mnogo kopipasty.
Ctoby eto delo ulucšitj, hotelosj by imetj iniçializator, kotoryj ukazyvajetsä odin raz dlä vsego massiva, no pri etom imejet vozmožnostj (po neobhodimosti) iniçializirovatj razlicnyje elementy razlicno.


### Iniçializator-funktor

Rešenijem vyšeizložennoj problemy možet bytj speçialjnyj iniçializator - funktor.
V nöm ukazyvajetsä vyraženije, kotoroje dolžno vycislätjsä vo cto-to vyzyvajemoje kak funkçija, budj eto funkçija, ukazatelj na funkçiju, funkçionaljnyj objekt (lämbda).
Kompilätor vyzyvajet etot funktor posledovateljno dlä každogo elementa iniçializirujemogo massiva i iniçializirujet každyj element rezuljtatom vyzova etogo funktora.

Dannyj iniçializator verojatno potrebujet osobogo sintaksisa.
Prosto ukazatj () ne dostatocno, ibo možet vozniknutj neodnoznacnostj, jesli, naprimer, element massiva sam javläjetsä funkçionaljnym.
Pri etom etot iniçializator primenäjetsä ko vsemu massivu çelikom, no ne k casti elementov ili hvostu massiva.

Funktor ne dolžen imetj parametrov krome "this".
Pri etom parametr "this" možet bytj izmenäjemym, jesli nado, naprimer, podderživatj vnutrennij scötcik.
Takže dolžen rabotatj vyzov peregružennyh funktorov i šablonnyh funktorov, gde eto ne neodnoznacno.

Dopolniteljno možno bylo by imetj otdeljnyj sintaksis dlä iniçializatora - funktora s odnim parametrom tipa "size_type".
Takoje polezno, jesli rezuljtirujusceje znacenije zavisit ot indeksa v massive.


### Na cto obratitj vnimanije pri realizaçii

Vyšeizložennyj iniçializator dolžen rabotatj s:

* Obycnymi funkçijami
* Peregružennymi funkçijami (dolžna vybiratjsä dolžnaja peregruzka)
* Ukazatelämi na funkçiju
* Staticeskimi metodami klassov
* Metodami klassov s prisojedinönnym "this"
* Šablonnymi funkçijami
* Šablonnymi staticeskimi metodami klassov
* Šablonnymi metodami klassov s prisojedinönnym "this"
* Lämbdami s "imut this" peredannymi po znaceniju, po ssylke, po izmenäjemoj ssylke
* Lämbdami s "mut this" peredannymi po znaceniju, po ssylke, po izmenäjemoj ssylke
* Lämbdami s "byval this" peredannymi po znaceniju, po ssylke, po izmenäjemoj ssylke
* Funktorami, vozvrascajuscimi znacenije tipa elementa massiva
* Funktorami, vozvrascajuscimi izmenäjemuju ssylku tipa elementa massiva
* Funktorami, vozvrascajuscimi neizmenäjemuju ssylku tipa elementa massiva
* Funktorami, vozvrascajuscimi znacenije, konvertirujemuje v tip elementa massiva
* Funktorami, vozvrascajuscimi izmenäjemuju ssylku, konvertirujemuje v tip elementa massiva
* Funktorami, vozvrascajuscimi neizmenäjemuju ssylku, konvertirujemuje v tip elementa massiva

Vyšeizložennyj iniçializator NE dolžen rabotatj s:

* Znacenijami, kotoryje neljzä vyzvatj
* Funktorami, kotoryje možno vyzvatj toljko odin raz (kotoryje imejut "byval this" i kotoryje ne kopirujemy)
* Funktorami s nepraviljnoj signaturoj
* Funktorami, kotoryje vozvrascajut tip, ne preobrazujemyj v tip elementa massiva
* Funktorami, dlä kotoryh neljzä vyvesti argumenty šablona
* "unsafe" funktorami vne "unsafe" konteksta
* "async" funktorami i znacenijami tipov korutin

Krome togo dolžno bytj provereno, cto sväzyvanije ssylok rabotajet - rezuljtirujuscij massiv sväzan so ssylkami, vozvrascönnymi peredannym funktorom.
Dolžno bytj provereno, cto "await" vnutri vyraženija-funktora dolžnym obrazom obrabatyvajutsä i casticno-sozdannyje elementy kompozitov razrušajutsä.
Vyšeopisannyj iniçializator dolžen takže rabotatj dlä polej struktur.
Rabota v "constexpr" kontekste tože neobhodima, no poka cto s ogranicenijem, cto "mut this" funktora ne podderživajetsä.
Neobhodimo udostoveritjsä, cto pri ispoljzovanii etogo iniçializatora osuscestvläjetsä peremescenije rezuljtata vyzova-znacenija (jesli tip sovpadajet), a ne kopirovanije.
Takže nado proverätj, cto ne osuscestvläjetsä sozdanije ekzemplärov abstraktnyh klassov.
