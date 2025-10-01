Bylo by vesjma horošo napisatj nekuju biblioteku, realizujuscuju bazovuj asinhronnostj.
Ona dolžna vklücatj v sebä ispolnitelj asinhronnyh funkçij i primitivy asinhronnoj raboty s setju.
Neobhodima eta biblioteka glavnym obrazom dlä togo, ctoby udostoveritjsä, cto podderžka asinhronnyh funkçij v jazyke dejstviteljno rabotajet i ne soderžit izjanov.

Jestj smysl sozdatj dlä etogo imenno otdeljnuju biblioteku, a ne delatj etot funkçional castju standartnoj biblioteki.
Dlä standartnoj biblioteki on byl by sliškom objomnym.
K tomu že on ne pretendujet na universaljnostj i gibkostj, kotoroj dolžen obladatj vunkçional standartnoj biblioteki, a prednoznacen skoreje dlä nekih konkretnyh slucajev, vrode serverom s neobhodimostju raboty so množestvom klijentov odnovremenno.

Predploagajetsä sledujuscij nabor funkçionala:
* Ispolnitelj asinhronnyh funkçij - s vozmožnostju peredaci asinhronnyh funkçij na ispolnenije.
* Funkçija vrode `await_all`, kotoraja možet vyzyvatjsä iz asinhronnyh funkçij i cerez kotoruju možno effektivno realizovatj parallelizm vypolnenija bez ožidanija odnoj podfunkçijej drugoj.
* Ispolnenije asinhronnyh funkçij v neskoljko potokov - s razdelenijem po potokam na urovne kornevyh asinhronnyh funkçij.
* Asinhronnyje varianty klassov setevyh primitivov - `udp_socket`, `tcp_listener`, `tcp_stream`, kotoryje tesno vzaimodejstvujut s isplnitelem asinhronnyh funkçij.
* Ožidanije gotovnosti asinhronnyh funkçij k daljnejšemu ispolneniju s pomoscju sistemnogo vyzova `poll` ili cego-to podobnogo, s effektivnoj peredacej upravlenija tem asinhronnym funkçijam, dlä kotoryh `poll` prosignaliziroval gotovnostj.
* Asinhronnyj `sleep_for`.

Cego ne budet v etoj biblioteke, po krajnej mere v pervoj rabocej versii:
* Asinhronnaja rabota s fajlami - ne na vseh sistemah jestj neobhodimyj "API" dlä etogo, kotoryj byl by dostatocno effektivnym.
* Rabota s proçessami.
* Obrabotka signalov.
* Gibkih/hitryh strategij balansirovki vremeni, davajemogo razlicnym asinhronnym funkçijam.

Sledujet sozdatj nekuju modelj, kak asinhronnyje funkçij dolžny ispoljzovatjsä s etoj bibliotekoj.
Eta modelj dolžna vklücatj takže nekotoryje ogranicenija.
Narušenije takih ogranicenij nado obäzateljno detektirovatj, ctoby oni ne privodili k polomke konsistentnosti programmy.
Samoje prostoje, cto možno delatj, jesli ogranicenije narušeno - vyzvatj `halt`.

Krome togo nado imetj nekije kolicestvennyje ogranicenija vremeni ispolnenija.
Naprimer, razmer oceredi asinhronnyh funkçij, ožidajuscih raspredelenija po potokam, dolžen bytj ogranicen.
Každyj potok tože dolžen imetj ne boljše opredelönnogo kolicestva asinhronnyh funkçij, kotoryje ispolnäjutsä.
Dannyje ogranicenija neobhodimy, daby sistema ne šla vraznos, jesli kolicestvo zaprosov boljše vozmožnosti eti zaprosy obrabatyvatj.

Polezno v kacestve vdohnovlenija uznatj, kak primerno realizovany shožije biblioteki v drugih jazykah.
Naprimer v "Rust" jestj biblioteka "tokio".
V jazyke "Go" podobnyj funkçional, naskoljko izvestno, vstrojen v sam jazyk i jego standartnuju biblioteku.

Planirujetsä sozdanije etoj biblioteki prämo v repozitorii Ü.
Sozdanije otdeljnogo repozitorija sliškom nakladno i hlopotno dlä etogo.

Imä biblioteki poka ne jasno.
Stoit vybratj takoje imä, kotoroje s odnoj storony pokazyvajet, cto eto ne castj standartnoj biblioteki jazyka, no s drugoj storony ne zvucit stranno.
