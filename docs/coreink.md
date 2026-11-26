# Fiche technique : M5Stack CoreInk

Fiche rédigée le 23 novembre 2026, sans la carte, à partir de la documentation
de M5Stack et du code de ses bibliothèques, puis corrigée d'après les
vérifications sur la carte des 23 et 24 novembre 2026 (soirées 1 et 2). Elle sert d'entrée aux pilotes
(`input`, `eink_display`, `signals`, `power`). Chaque information porte son
degré de certitude :

- **doc** : écrit dans la documentation officielle ;
- **code** : déduit du code des bibliothèques de M5Stack, qui fonctionne sur
  la carte ;
- **schéma** : lu sur le schéma. Le texte seul ne donne pas les liaisons : le
  24 novembre 2026, Benoît a lu les dessins des pages « RTC » et « PWR_SW &
  ADC » (captures) ; ce qui en est tiré porte « schéma (Benoît) » ;
- **lecture Claude** : interprétation du dessin ou calcul à partir du schéma,
  jamais testé sur la carte ;
- **fiche** : tiré d'une fiche technique ou d'une page de revendeur lue le
  24 novembre 2026 ;
- **général** : connaissance générale des composants, non relue dans une
  source (à confirmer) ;
- **carte** : mesuré ou observé sur la carte de Benoît (soirées 1 et 2) ;
- **à vérifier** : hypothèse, à confirmer sur la carte (voir
  [verifications-carte.md](verifications-carte.md)).

Sources en fin de fiche.

## 1. Brochage

| GPIO | Fonction | Sens | Certitude | Remarques |
|---|---|---|---|---|
| 37 | Molette haut (bouton A) | entrée, actif bas | doc, code | GPIO d'entrée seule, résistance de tirage externe 10 kΩ (R23 à R25, schéma). GPIO RTC : peut réveiller de la veille profonde. |
| 38 | Molette appui (bouton B) | entrée, actif bas | doc, code | Idem. |
| 39 | Molette bas (bouton C) | entrée, actif bas | doc, code | Idem. |
| 5 | Bouton utilisateur (« EXT », « top button » dans M5Unified), **au-dessus de l'écran** | entrée, actif bas | carte (23/11/2026) | **Pas un GPIO RTC** : ne peut pas réveiller de la veille profonde (voir 4). Inutilisé par Retimer. |
| 27 | Bouton marche (« PWR »), **sous la molette** | entrée, actif bas | carte (23/11/2026) | GPIO RTC. Le même bouton allume la carte par le circuit d'alimentation (schéma : S1, D5, D6). Retimer s'en sert aussi pour le rappel (CLAUDE.md). |
| 12 | Maintien de l'alimentation (« HOLD ») | sortie | doc, code, schéma (Benoît) | À mettre à 1 dès le démarrage, sinon la carte s'éteint quand on relâche le bouton marche. À 0 : extinction complète (sur batterie). Commande la grille de Q5 par la diode D14 (voir 3, « Circuit d'alimentation »). Broche de configuration (MTDI) de l'ESP32 : la diode D14 l'isole du circuit (lecture Claude) ; le réveil marche depuis la correction du maintien (carte, B13 résolu). |
| 2 | Buzzer | sortie MLI | doc, code | Piloté par un transistor S8050 (Q1, schéma). M5Stack le commande par une fréquence (LEDC), donc buzzer **passif** : à vérifier. |
| 10 | LED verte | sortie | doc, code | Active à l'état bas (vérifié, B7). Emplacement sur le boîtier peu visible (CLAUDE.md). |
| 35 | Mesure de la batterie | entrée analogique (ADC1, canal 7) | code, schéma (Benoît) | Pont diviseur 20 kΩ / 5,1 kΩ (R41, R42) : V_bat = V_adc × 25,1 / 5,1. Le pont est alimenté **derrière Q4** (schéma, Benoît) : il consomme environ 170 µA seulement carte allumée, veille profonde comprise, et rien carte éteinte (voir 6). |
| 19 | Interruption de l'horloge BM8563 (INT) | entrée, actif bas | code, schéma (Benoît) | M5Unified l'utilise pour le réveil par l'alarme. Pas un GPIO RTC : réveille la veille légère, pas la profonde. Rappelée vers MCU_VDD par R37 (10 kΩ), derrière la diode D12. La même ligne INT agit sur l'allumage (`PWR_TRIG`, diode D13) : voir 4, « La BM8563 ». |
| 21, 22 | I²C interne (SDA, SCL) : horloge BM8563, adresse 0x51 | — | doc, code, schéma (Benoît) | |
| 9 | Écran : CS | sortie | doc, code | |
| 15 | Écran : D/C | sortie | doc, code | |
| 18 | Écran : SCK | sortie | doc, code | |
| 23 | Écran : MOSI | sortie | doc, code | |
| 0 | Écran : RST | sortie | doc, code | Aussi broche de démarrage (BOOT) de l'ESP32. |
| 4 | Écran : BUSY | entrée | doc, code | Polarité selon le contrôleur (voir 2). |
| 1, 3 | Console série (puce WCH CH9102 sur la carte de Benoît, CP2104 selon la doc) | — | carte | `idf.py monitor`. Pas de pilote à installer sur macOS. |
| 32, 33 | Port Grove (PORT.CUSTOM) | libres | doc | Pour un futur capteur « mains sales » (CMD-04). |

Toutes les commandes sont actives à l'état bas avec tirage externe : pas besoin
des résistances internes (les GPIO 34 à 39 n'en ont pas de toute façon).

## 2. Écran e-ink

### Deux modèles selon la date de fabrication

La documentation signale un changement le 25 octobre 2024 : l'écran
**GDEW0154M09** a été remplacé par le **GDEY0154D67**. Les deux font 1,54",
200 × 200 points, noir et blanc, mais **leurs contrôleurs n'ont pas le même jeu
de commandes**. Le CoreInk excédentaire peut avoir l'un ou l'autre.

| | GDEW0154M09 (avant fin 2024) | GDEY0154D67 (depuis fin 2024) |
|---|---|---|
| Contrôleur | Fitipower JD79653A (famille UC8151) | Solomon SSD1681 (d'après le jeu de commandes) |
| Écriture de l'image | `0x10` (ancienne image), `0x13` (nouvelle) | `0x24` (noir et blanc), `0x26` (image de référence) |
| Fenêtre partielle | `0x91` (entrée), `0x90` (fenêtre), `0x92` (sortie) | `0x44`, `0x45` (fenêtre), `0x4E`, `0x4F` (curseur) |
| Rafraîchissement | `0x12` | `0x22` (séquence) puis `0x20` |
| Mode partiel | formes d'onde chargées par `0x20` à `0x24` | séquence `0x22` = `0xFF` (mode 2, OTP) |
| BUSY | actif **bas** | actif **haut** |
| Veille de l'écran | `0x02` puis `0x07` `0xA5` | `0x10` `0x01` |

Qui fabrique quoi (général, non relu dans une source) : le **film
électrophorétique** vient d'un seul fournisseur, **E Ink** (E Ink Holdings) ;
**Good Display** (références GDEW et GDEY) assemble les dalles et les modules ;
les **contrôleurs** viennent d'autres entreprises (Fitipower pour le JD79653A).
Le nom « SSD1681 » du contrôleur du GDEY0154D67 vient du jeu de commandes de
M5GFX : la fiche Good Display ne nomme pas le circuit (fiche du 24/11/2026, non
confirmé). Le Sharp Memory LCD n'est pas de l'e-ink : autre technologie, autre
chaîne d'approvisionnement.

Le bus est en SPI 4 fils sans lecture (pas de broche de données en retour sur
le connecteur, schéma) : on ne peut pas identifier le contrôleur par lecture.

**La carte de Benoît a été livrée en 2023 : c'est a priori le GDEW0154M09.**
Le pilote `eink_display` ne gère que celui-là (décision du 23 novembre 2026).
Si un jour une carte plus récente est utilisée, il faudra ajouter le
GDEY0154D67 en s'inspirant de M5GFX. Première vérification du soir : que
l'image s'affiche bien (sinon, c'est l'autre écran).

Le code de M5Stack (M5GFX, qui reprend LovyanGFX) contient les séquences
d'initialisation des deux écrans ; le pilote s'en inspire.

**Compatibilité des deux modèles.** Côté matériel, rien ne change (SPI 4 fils,
même brochage sur le CoreInk, 200 × 200, noir et blanc). Côté logiciel, le
jeu de commandes (écriture de l'image, fenêtre partielle, veille de l'écran) et
la polarité de BUSY diffèrent : seule la partie de `eink_display` propre au
contrôleur est à réécrire. La fiche du GDEY0154D67 confirme BUSY actif **haut**
(broche 9) ; elle ne permet pas de confirmer les codes de commande (tableau
illisible à l'extraction). Le GDEW0154M09 est **arrêté** (page du revendeur,
24/11/2026) : une carte de remplacement aura très probablement le GDEY0154D67.

### Forme d'onde

Une **forme d'onde** est la séquence de tensions appliquée à chaque pixel pour
déplacer les particules noires et blanches de sa microcapsule. Les particules
sont lentes (quelques centaines de millisecondes) et restent en place sans
tension : c'est pourquoi l'image tient hors alimentation.

- Le rafraîchissement **complet** (0,66 s mesuré sur la carte) inverse d'abord
  tous les pixels, puis écrit l'image : il clignote et efface la rémanence.
- Le rafraîchissement **partiel** (0,33 s mesuré) ne touche que les pixels qui
  changent entre l'ancienne et la nouvelle image : pas de clignotement, mais
  des erreurs s'accumulent (traces, contraste en baisse).
- Le contrôleur garde deux images (ancienne et nouvelle) pour savoir quels
  pixels changent. Sur le GDEW0154M09, les tables de formes d'onde du mode
  partiel sont chargées par les commandes `0x20` à `0x24` ; sur le
  GDEY0154D67, elles sont en mémoire OTP et on choisit un mode.
- Le blanc sur noir (alerte) est la transition la plus difficile : rémanence
  vue en B5, d'où le rafraîchissement complet à l'entrée et à la sortie d'une
  alerte (E7).

Certitude : principe général (général) ; durées (carte).

### Rafraîchissement partiel et traces

- Durée annoncée : 2 s pour un rafraîchissement complet, 0,26 s pour un partiel
  (doc, GDEY0154D67).
- La documentation recommande d'**éviter les rafraîchissements fréquents
  prolongés** et d'espacer (page M5Stack CoreInk, Tutorial > Notes :
  « The recommended refresh interval is (15s / time) », formule ambiguë que
  nous lisons « toutes les 15 s »). Le guide d'usage Good Display et la FAQ
  Waveshare donnent 180 s, pour le rafraîchissement complet (voir EINK.md,
  section 6). Une minuterie qui affiche les
  secondes rafraîchit chaque seconde : c'est exactement le point 1 des essais
  de CLAUDE.md. Les niveaux dizaine et minute de l'affichage (« 05:2- », « 12:-- », MAT-04, EINK.md) ramènent le rythme à une
  fois par minute pendant l'essentiel du décompte.
- Un rafraîchissement complet périodique efface les traces : sa période est une
  constante (`CST_ECRAN_RAFRAICHISSEMENT_COMPLET`), à régler en essai.
- Éviter l'exposition prolongée aux UV (doc).
- L'image reste affichée hors tension (doc) : utile pour l'écran de veille
  (MAT-12) et pour les alertes persistantes.

### Caractéristiques, durée de vie, usure

Valeurs du **GDEW0154M09** (page du revendeur buyepaper.com, lue le
24/11/2026 ; la fiche PDF du M09 n'a pas été obtenue) :

- fonctionnement **0 à 50 °C**, stockage −25 à 70 °C : une cuisine convient,
  pas un four ouvert ni un congélateur ;
- rafraîchissement complet 0,82 s, partiel 0,3 s (mesuré sur la carte : 0,66 s
  et 0,33 s) ;
- 6,6 mW pendant un rafraîchissement, 0,0165 mW en veille ;
- produit arrêté, échantillons seulement ;
- **aucune durée de vie ni recommandation de rafraîchissement** sur cette page.

Recommandations de la fiche du **GDEY0154D67** (Good Display, rév. 1.2, avril
2024), valables pour le M09 par hypothèse (même fabricant, même génération) :

- durée de vie **« 1 000 000 de cycles ou 5 ans »** (conditions non précisées) ;
- **rafraîchir au moins toutes les 24 h**, sinon rémanence ou « image collée » :
  concerne l'écran de veille laissé affiché une nuit ;
- **un rafraîchissement complet après 5 partiels consécutifs** : bien plus
  strict que la constante actuelle `CST_DISPLAY_PARTIALS_BEFORE_FULL`, à
  comparer et à trancher en essai ;
- chaleur, humidité, soleil et lumière fluorescente dégradent la dalle.

Estimation (lecture Claude, non mesurée) : à un partiel par seconde, une heure
de cuisson fait 3 600 rafraîchissements ; avec l'affichage réduit « :-- » (un
par minute), 60. Si la limite d'un million de cycles est exacte, l'usure par
nombre de cycles n'est pas le premier risque : la rémanence, les UV et la
température passent avant. Une mesure directe (compteur de rafraîchissements
en mémoire RTC, affiché au diagnostic) est prévue dans les essais.

## 3. Alimentation

- Batterie LiPo 390 mAh, 3,7 V (doc). Chargeur TP4057 (schéma, `TP40P57`).
- Régulateur 3,3 V à découpage SY8089 ; convertisseur élévateur SY7088 pour le
  5 V du port et du HAT (schéma).
- Allumage : appui long sur le bouton marche latéral (doc). Le logiciel doit
  alors mettre le GPIO 12 à 1 pour garder l'alimentation.
- Extinction : bouton de réinitialisation à l'arrière, ou par logiciel en
  mettant le GPIO 12 à 0 (doc). Sur USB, la carte reste alimentée. Retimer
  s'éteint quand on maintient le bouton marche (`POWER_OFF_HOLD_MS`, 3 s).
- Sur USB, la mesure de la batterie donne 4,29 à 4,33 V (carte, 23/11/2026).
- **Mesure de la batterie** : ADC1 canal 7 (GPIO 35), atténuation 11 dB,
  V_bat = V_adc × 25,1 / 5,1 (code M5Unified, cohérent avec R41 et R42 du
  schéma). Seuil de batterie faible (CST-09) : à fixer, valeur de départ
  3,5 V. Schéma lu par Benoît le 24/11/2026 : **R41 (20 kΩ)** de la sortie de Q4 (donc carte allumée seulement) à
  GPIO 35, **R42 (5,1 kΩ)** de GPIO 35 à la masse, **C30 (1 µF)** en
  parallèle. Le pont est derrière Q4 : sous tension seulement carte allumée (voir 6 pour son coût).

### Circuit d'alimentation (page « PWR_SW & ADC » du schéma)

Lu par Benoît sur le schéma le 24/11/2026 (composants et liaisons) ; le
fonctionnement décrit est une **lecture Claude**, non testée.

- **Q4 (SI2301, MOSFET canal P)** : interrupteur côté haut. Source sur SYS_BAT,
  grille rappelée à SYS_BAT par **R32 (100 kΩ)** (donc ouvert au repos) et
  reliée à la ligne **PWR_TRIG** par **R36 (47 Ω)**. Quand PWR_TRIG est tiré
  vers le bas, Q4 conduit. Son drain alimente, par **R50 (10 kΩ)**, le réseau
  **PS_EN**, l'entrée d'activation des convertisseurs 3,3 V et 5 V.
- **Q5 (SI2302, MOSFET canal N)** : grille commandée par **GPIO 12 à travers la
  diode D14**, rappelée à la masse par **R33 (10 kΩ)** ; drain sur PWR_TRIG,
  source à la masse.
- **Verrou** : GPIO 12 à 1 → Q5 conduit → PWR_TRIG bas → Q4 conduit → la
  carte est alimentée. GPIO 12 à 0, ou en l'air, → Q5 ouvert → R32 remonte la
  grille de Q4 → extinction, sauf si une autre source tire PWR_TRIG vers le
  bas (le bouton marche, ou l'interruption de la BM8563, voir 4).
- Explique B13 : en veille profonde sans `gpio_hold`, GPIO 12 retombe, R33
  ouvre Q5 et la carte s'éteint. Cohérent avec la correction du maintien
  (carte, B13 résolu).
- **Branche USB non comprise** : une branche **USB_5V → D19 → R51 (5,1 kΩ)**
  figure près de la grille de Q5. Son rôle probable est de garder ou d'allumer
  la carte quand l'USB est branché ; **à vérifier**.

## 4. Veille et réveil

C'est le point le plus délicat, et la documentation n'en dit presque rien.

### Ce que permet l'ESP32

| Mode | Consommation de l'ESP32 seul (fiche Espressif) | Réveil possible par | RAM |
|---|---|---|---|
| Veille légère | ~0,8 mA | minuterie, **n'importe quel GPIO** (niveau) | conservée |
| Veille profonde | ~10 µA | minuterie, GPIO RTC seulement (`ext0` : une broche ; `ext1` : plusieurs broches, réveil si **toutes** à 0 ou si une à 1) | perdue, sauf la mémoire RTC (8 Ko) |
| Extinction (GPIO 12 à 0) | 0 pour l'ESP32 (la BM8563 reste alimentée par SYS_BAT) | bouton marche ; alarme de la BM8563 par `PWR_TRIG` (schéma, **à tester**) | perdue |

La consommation du reste de la carte (régulateur, pont diviseur de la batterie,
tirages) s'ajoute et pèse plus que celle de l'ESP32 en veille profonde : voir 6.
C'est la mesure du point 6 des essais.

### Conséquences pour les commandes du CoreInk

- Les trois positions de la molette (GPIO 37, 38, 39) et le bouton marche
  (27) sont des GPIO RTC. Mais les boutons sont actifs à l'état bas, et sur
  l'ESP32 d'origine `ext1` ne sait réveiller « dès qu'une broche passe à 0 »
  que pour une seule broche. En veille profonde, on peut donc surveiller **deux
  commandes** : une par `ext0`, une par `ext1`. Choix : molette haut (`ext0`)
  et bouton sous la molette, celui du rappel (`ext1`, ETA-10). La molette bas
  ne sert à rien en Veille : la globale y est à zéro.
- Le **bouton au-dessus de l'écran (GPIO 5) ne peut pas réveiller de la veille
  profonde.** Le coprocesseur ULP ne peut pas non plus le lire (ce n'est pas un
  GPIO RTC). Retimer ne l'utilise pas.
- Vérifié sur la carte le 23 novembre 2026 : en veille légère, le GPIO 5 et la
  molette réveillent (cause 7) ; en veille profonde, la molette haut réveille
  (`ext0`).
- En **veille légère**, toutes les commandes réveillent la carte.

Stratégie retenue pour le premier jet (`power`) :

- en **Décompte** et en **Terminé** (alertes possibles, l'appui sur le bouton du
  rappel doit marcher) : veille légère entre deux réveils programmés ;
- en **Veille** (aucun décompte) : veille profonde, réveil par la molette haut
  ou le bouton du rappel ; l'état tient dans la mémoire RTC ;
- l'extinction complète, avec réveil par la BM8563, viendra après mesure
  (CLAUDE.md ; options et réserves ci-dessous, « La BM8563 »).

Si la veille légère consomme trop pendant les décomptes, deux pistes : lire le
bouton du rappel uniquement aux réveils programmés (appui long nécessaire), ou
un programme ULP qui surveille la molette pendant la veille profonde.

### Maintien de l'alimentation pendant la veille

Le GPIO 12 doit rester à 1 pendant la veille profonde, sinon la carte s'éteint
(sur batterie). Il faut donc le figer avec `gpio_hold_en()` et
`gpio_deep_sleep_hold_en()`. Au réveil, il faut configurer la broche en sortie
à 1 **avant** de la libérer avec `gpio_hold_dis()` : ses registres sont revenus
à l'état de démarrage, et la libérer d'abord coupe l'alimentation sur
batterie (soirée 2 : au réveil par le bouton sous la molette, la carte
redémarrait à froid ; sur USB, rien ne se voyait).

### Exactitude du temps

En veille profonde, l'ESP32 compte le temps avec son horloge lente interne
(« RTC_SLOW_CLK »), étalonnée au démarrage contre le quartz de 40 MHz. Le
CoreInk n'a pas de quartz 32 kHz sur l'ESP32 (le seul quartz 32,768 kHz est
celui de la BM8563, schéma). Trois sources de temps existent sur la carte :

1. le **quartz de 40 MHz** de l'ESP32 : précis, mais coupé en veille ;
2. l'**horloge lente interne** de l'ESP32, un oscillateur RC qui tourne pendant
   les veilles ; variantes d'ESP-IDF : environ 150 kHz (la plus grossière), ou
   le 8 MHz interne divisé par 256 (« 8MD256 », environ 33 kHz) ;
3. la **BM8563**, avec son propre quartz de 32,768 kHz (voir ci-dessous).

La veille légère utilise la même horloge lente : le quartz de 40 MHz est coupé
pendant la veille. Mesure de la soirée 1 (C8) avec l'ancien réglage : environ
3 s d'avance sur 10 min (0,5 %), départ du chronomètre imprécis. Depuis,
l'horloge lente est le 8 MHz interne divisé par 256
(`CONFIG_RTC_CLK_SRC_INT_8MD256`), plus stable selon Espressif, pour environ
5 µA de plus en veille profonde. **Mesure de la soirée 2 (E12, commit
37b2390) : 10:00,06 pour 10 minutes**, soit 0,06 s d'écart (0,01 %). C'est une
seule mesure, à température ambiante : la dérive en température, et en veille
profonde sur une longue durée, reste à mesurer (essais). La BM8563 n'est pas
lue par le code. Si la dérive dépasse 0,1 % sur de longues cuissons, on recale
sur la BM8563 à chaque réveil (précision à la seconde).

Pour un développeur C++ : l'horloge lente interne joue le rôle d'une
`steady_clock` (une durée écoulée qui repart de zéro à chaque démarrage à
froid), la BM8563 celui d'une `system_clock` (une date réelle qui survit à
l'extinction). La minuterie compte des durées : la première suffit au principe.

### La BM8563 (horloge temps réel)

Page « RTC » du schéma, lue par Benoît le 24/11/2026 (schéma) :

- **U4 BM8563**, quartz **X1 de 32,768 kHz** avec deux condensateurs de 6 pF
  sur OSCI et OSCO, découplage C25 (100 nF) ;
- **VDD (broche 8) sur SYS_BAT** : la puce reste alimentée carte éteinte, tant
  que la batterie ou l'USB est présent, et continue de compter ;
- **SDA sur GPIO 21, SCL sur GPIO 22** (adresse 0x51) ;
- **CLKOUT (broche 7) n'est pas connectée.** Pas de signal périodique (1 Hz,
  32 Hz, 1 kHz, 32 kHz) vers l'ESP32 : l'ancienne piste « CLKOUT comme horloge
  lente » est supprimée, et couper la sortie 32 kHz ne fait rien gagner à la
  carte ;
- **INT (broche 3)**, à collecteur ouvert, rappelée vers SYS_BAT par **R38
  (10 kΩ)**, est reliée **en parallèle** à deux endroits : **GPIO 19** par la
  diode **D12** (rappelé vers MCU_VDD par **R37, 10 kΩ**), et **PWR_TRIG** par
  la diode **D13**. Dans les deux cas l'anode est du côté d'INT : INT à 0 tire
  GPIO 19 et PWR_TRIG vers le bas ; elle ne peut pas les relever. Les diodes
  isolent le domaine 3,3 V de l'ESP32 et le domaine batterie (lecture Claude :
  ce n'est pas un OU câblé entre sources différentes).

**Trois usages possibles** (fonctionnement de la famille PCF8563, dont la
BM8563 est un équivalent : général, **à confirmer dans la fiche du BM8563, non
lue**) :

1. **Lecture par I²C** : le processeur lit l'heure quand il est éveillé. Rien
   ne réveille.
2. **Minuterie à rebours ou alarme programmée par I²C, puis INT sur GPIO 19** :
   la puce décompte seule et tire INT vers le bas à l'échéance. Réveille la
   **veille légère** (tout GPIO le peut), **pas la veille profonde** (GPIO 19
   n'est pas un GPIO RTC). Alarme à la **minute** près ; minuterie sur 8 bits
   (4096 Hz à 1/60 Hz, jusqu'à 255 minutes en mode lent), éventuellement avec
   impulsions répétées. INT reste bas tant que le drapeau n'est pas effacé par
   I²C.
3. **Réveil depuis l'extinction complète** : INT à 0 → D13 → PWR_TRIG bas →
   Q4 conduit → les convertisseurs démarrent (voir 3). **Faisable sur le
   papier ; jamais testé.**

Le mode « CLKOUT + interruption » n'existe pas sur cette carte.

**Réserves pour l'extinction complète** (lecture Claude, à tester) :

- **effacer le drapeau d'alarme avant d'éteindre** : sinon INT maintient
  PWR_TRIG bas et la carte **ne s'éteint pas** ;
- **seuls le bouton marche (GPIO 27) et l'alarme rallument** : la molette
  (GPIO 37, 38, 39) ne passe pas par PWR_TRIG ;
- **l'état de la minuterie est perdu** (mémoire RTC de l'ESP32 coupée) : il
  faut le garder ailleurs. La BM8563 n'est **pas** une mémoire : dans la
  famille PCF8563, ses 16 registres ne contiennent que l'heure, l'alarme, la
  minuterie et le contrôle, sans octet libre (à confirmer dans la fiche, non
  lue) ; ses registres sont volatils (conservés tant que SYS_BAT est présent).
  Options : la **mémoire flash de l'ESP32** (NVS, non volatile, quelques
  écritures par cuisson, usure sans souci), ou la date de fin placée dans
  l'alarme de la BM8563 (résolution à la minute) ;
- **démarrage à froid** plus long (initialisation de l'écran), à mesurer ;
- le gain d'énergie est important : le pont de batterie est coupé avec Q4, il
  ne reste que la BM8563 et les fuites du chargeur (voir 6).

**Coût de la BM8563** : de l'ordre de 0,25 µA (général), déjà payé puisque la
puce est toujours alimentée. Chaque échange I²C (quelques octets) dure de
0,2 à 1 ms (estimation) : négligeable.

**Options à comparer** (aucune mise en œuvre décidée) : (A) veille profonde
actuelle, réveil par l'horloge lente et les broches RTC ; (B) veille légère
pendant les décomptes, réveil par INT sur GPIO 19 (alarme ou minuterie de la
BM8563) ; (C) extinction complète avec alarme de la BM8563 (réserves
ci-dessus). Voir PLAN.md, section 6.3.

## 5. Buzzer

Buzzer passif sur GPIO 2, derrière un transistor : on le commande par une
fréquence (périphérique LEDC de l'ESP32). M5Stack utilise 1 kHz par défaut ;
un buzzer de ce type est en général plus fort vers sa fréquence de résonance
(souvent 2 à 4 kHz). La fréquence des bips est donc une constante, à régler à
l'oreille (point 5 des essais).

## 6. Consommation : ce qu'on sait

Calculs d'après le schéma lu le 24/11/2026 (**lecture Claude**, non mesurés sur
la carte) et chiffres typiques de fiches (**général**). Seule une mesure du
courant sur batterie, mode par mode, tranchera (CLAUDE.md, point 6 des essais).

| Poste | Courant ou puissance | Quand | Certitude |
|---|---|---|---|
| ESP32, veille profonde | ~10 µA, plus ~5 µA avec l'horloge 8MD256 | en Veille | fiche Espressif ; mesure à faire |
| ESP32, veille légère | ~0,8 mA | pendant les décomptes | fiche Espressif ; mesure à faire |
| ESP32, CPU actif à 160 MHz | ~40 à 50 mA (~25 à 30 mA à 80 MHz) | à chaque réveil, attentes d'écran | fiche Espressif ; non mesuré |
| **Pont de mesure de la batterie** (R41 + R42 = 25,1 kΩ) | **~170 µA à 4,2 V**, ~150 µA à 3,7 V, ~130 µA à 3,3 V | **carte allumée seulement** (derrière Q4), veille profonde comprise ; rien carte éteinte | schéma (Benoît) + calcul |
| R32 (100 kΩ) | ~42 µA à 4,2 V | tant que GPIO 12 est à 1 (maintien), donc aussi en veille profonde | schéma + calcul |
| R38 puis R37 (10 kΩ chacune) | jusqu'à ~420 µA (R38, 4,2 V) et ~270 µA (R37, via D12) | **seulement tant qu'INT est à 0**, c'est-à-dire tant que le drapeau d'alarme n'est pas effacé | schéma + calcul |
| BM8563 | ~0,25 µA | en permanence | général |
| Écran GDEW0154M09 | 6,6 mW pendant un rafraîchissement (0,3 s), 0,0165 mW en veille | à chaque mise à jour | page du revendeur ; périmètre de la valeur non précisé |
| Convertisseurs 3,3 V et 5 V, chargeur, buzzer | courant de repos inconnu | | à mesurer |

Ce que l'on en tire :

- **Le coût de la mesure de la batterie, c'est le pont diviseur, pas la
  lecture.** Lire l'ADC est négligeable ; mais le pont de 25,1 kΩ consomme
  ~170 µA, soit **plus de quinze fois** l'ESP32 en veille profonde. Il est
  alimenté par la sortie de Q4 (lu par Benoît le 24/11/2026) : il ne coûte
  donc que **carte allumée**, y compris pendant toute la veille profonde, et
  rien carte éteinte. Pas d'interrupteur dédié pour le couper par logiciel
  carte allumée. Les valeurs basses (20 kΩ et 5,1 kΩ, avec C30 de 1 µF) sont
  typiques d'un choix pour l'ADC de l'ESP32, qui supporte mal une source à
  haute impédance (général, supposition de conception).
- **Plancher en veille profonde (hors convertisseurs)** : 10 + 5 + 170 + 42 ≈
  230 µA. À 390 mAh, cela donne de l'ordre de 70 jours de veille profonde en
  théorie, avant courant de repos des convertisseurs et auto-décharge.
- **Plancher en extinction complète** : seulement la BM8563 (~0,25 µA) et les
  fuites carte éteinte (chargeur TP4057 notamment, non chiffrées), soit de
  l'ordre de quelques µA, **à mesurer**. Le gain par rapport à la veille
  profonde est donc d'environ **deux ordres de grandeur** (~230 µA contre
  quelques µA), ce qui rend l'extinction complète intéressante pour les
  réveils rares (toutes les dix minutes ou plus), mais pas pour les réveils
  chaque minute, dont le coût est celui du démarrage à froid (~0,7 mA de
  moyenne, estimation).
- Pendant un décompte en veille légère, le total est de l'ordre de 1 mA
  (0,8 mA + 0,2 mA), soit une autonomie de l'ordre de 16 jours de décompte
  continu. Le pont pèse environ un cinquième de ce total.
- **L'écran pèse peu.** Un partiel (0,33 s à 6,6 mW) coûte environ 2 mJ. À un
  par seconde, la moyenne reste de l'ordre de 2 mW, à comparer à ~150 mW pour
  le processeur éveillé à 160 MHz. Le gain d'énergie de l'affichage réduit
  « :-- » est donc modeste (remarque 8 de la soirée 2) ; ce qui coûte, c'est
  de **garder le processeur éveillé** pendant les 330 ms d'attente de l'écran,
  d'où la priorité donnée à la veille légère pendant l'attente (PLAN.md,
  section 6.3).
- **Une alarme non acquittée coûte cher** : tant que le drapeau d'alarme du
  BM8563 n'est pas effacé, INT reste à 0 et les rappels R38 et R37 tirent
  plusieurs centaines de µA. Effacer le drapeau tout de suite après le réveil.
- Pistes déjà identifiées (audit du 24/11) : veille légère pendant l'attente de
  l'écran et entre deux événements, éteindre le contrôleur d'écran entre deux
  mises à jour, couper les journaux série hors développement, CPU à 80 MHz.
  « Couper la sortie 32 kHz de la BM8563 » ne fait rien gagner : CLKOUT n'est
  pas connectée.

## 7. Processeur et fréquence

- L'ESP32-PICO-D4 est un ESP32 d'origine : cœurs **Xtensa LX6**, deux cœurs
  (général). Ni ARM ni RISC-V. Les ESP32-C3, C6 et H2 sont en RISC-V ; les S2
  et S3 en Xtensa LX7. ESP-IDF et son API C sont communs aux familles : la
  logique métier reste portable.
- Fréquence du CPU : 80, 160 ou 240 MHz (aussi 40, 20 ou 10 MHz avec le quartz
  de 40 MHz). Elle se fixe au démarrage (`CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ`)
  ou peut changer en cours d'exécution : à la main par `esp_pm_configure()`, ou
  automatiquement par la gestion d'énergie d'ESP-IDF (PM), qui monte la
  fréquence quand une tâche en a besoin et entre en veille légère pendant les
  attentes (général, documentation ESP-IDF). État actuel (audit du 24/11) :
  160 MHz fixe, PM désactivée.
- Baisser la fréquence réduit grossièrement la part « calcul » de la
  consommation ; le gain de ne pas calculer du tout (veille légère) est bien
  supérieur. À 80 MHz, SPI et MLI du buzzer sont à revérifier.

## 8. Sources

- Page revendeur du GDEW0154M09 (buyepaper.com) et fiche Good Display du
  GDEY0154D67, rév. 1.2 (evelta.com), lues le 24/11/2026 ; schéma du CoreInk,
  pages « RTC » et « PWR_SW & ADC » lues par Benoît le 24/11/2026.

- Documentation M5Stack CoreInk : <https://docs.m5stack.com/en/core/coreink>
  (brochage, changement d'écran du 25 octobre 2024, batterie, conseils de
  rafraîchissement).
- Schéma : <https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/schematic/Core/coreink/coreink_sch.pdf>
  (le texte donne les composants et les noms de réseaux ; les liaisons ont
  été lues par Benoît sur les dessins le 24/11/2026).
- Bibliothèque Arduino M5Core-Ink, `src/utility/config.h` :
  <https://github.com/m5stack/M5Core-Ink> (brochage, GPIO 12 de maintien).
- M5Unified, `src/utility/Power_Class.inl` :
  <https://github.com/m5stack/M5Unified> (batterie sur GPIO 35, rapport
  25,1 / 5,1 ; interruption de l'horloge sur GPIO 19 ; bouton marche sur 27).
- M5GFX, `src/lgfx/v1/panel/Panel_GDEW0154M09.*` et `Panel_GDEW0154D67.*`, et
  `extras/board_spec/boards/m5stack_coreink.json` :
  <https://github.com/m5stack/M5GFX> (séquences des deux écrans, polarité de
  BUSY, tirages externes des boutons). Licence MIT, code dérivé de LovyanGFX
  (licence FreeBSD).
- ESP-IDF, guide des modes de veille de l'ESP32 :
  <https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/system/sleep_modes.html>
  (sources de réveil, limites d'`ext1`).
- Fiche technique de l'ESP32 (consommations typiques) :
  <https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf>.
