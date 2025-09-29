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
