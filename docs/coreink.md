# Fiche technique : M5Stack CoreInk

Fiche rédigée le 23 novembre 2026, sans la carte, à partir de la documentation
de M5Stack et du code de ses bibliothèques. Elle sert d'entrée aux pilotes
(`input`, `eink_display`, `signals`, `power`). Chaque information porte son
degré de certitude :

- **doc** : écrit dans la documentation officielle ;
- **code** : déduit du code des bibliothèques de M5Stack, qui fonctionne sur
  la carte ;
- **schéma** : lu sur le schéma, dont seul le texte a pu être exploité (les
  liaisons sont des dessins) ;
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
| 12 | Maintien de l'alimentation (« HOLD ») | sortie | doc, code | À mettre à 1 dès le démarrage, sinon la carte s'éteint quand on relâche le bouton marche. À 0 : extinction complète (sur batterie). Broche de configuration (MTDI) de l'ESP32 : à vérifier qu'elle ne gêne pas le réveil. |
| 2 | Buzzer | sortie MLI | doc, code | Piloté par un transistor S8050 (Q1, schéma). M5Stack le commande par une fréquence (LEDC), donc buzzer **passif** : à vérifier. |
| 10 | LED verte | sortie | doc, code | Polarité non documentée : à vérifier. Emplacement sur le boîtier peu visible (CLAUDE.md). |
| 35 | Mesure de la batterie | entrée analogique (ADC1, canal 7) | code, schéma | Pont diviseur 20 kΩ / 5,1 kΩ (R41, R42) : V_bat = V_adc × 25,1 / 5,1. |
| 19 | Interruption de l'horloge BM8563 | entrée, actif bas | code | M5Unified l'utilise pour le réveil par l'alarme. Pas un GPIO RTC. Le schéma suggère que l'interruption agit aussi sur l'allumage (réseau `PWR_TRIG`) : à vérifier. |
| 21, 22 | I²C interne (SDA, SCL) : horloge BM8563, adresse 0x51 | — | doc, code | |
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

Le bus est en SPI 4 fils sans lecture (pas de broche de données en retour sur
le connecteur, schéma) : on ne peut pas identifier le contrôleur par lecture.

**La carte de Benoît a été livrée en 2023 : c'est a priori le GDEW0154M09.**
Le pilote `eink_display` ne gère que celui-là (décision du 23 novembre 2026).
Si un jour une carte plus récente est utilisée, il faudra ajouter le
GDEY0154D67 en s'inspirant de M5GFX. Première vérification du soir : que
l'image s'affiche bien (sinon, c'est l'autre écran).

Le code de M5Stack (M5GFX, qui reprend LovyanGFX) contient les séquences
d'initialisation des deux écrans ; le pilote s'en inspire.

### Rafraîchissement partiel et traces

- Durée annoncée : 2 s pour un rafraîchissement complet, 0,26 s pour un partiel
  (doc, GDEY0154D67).
- La documentation recommande d'**éviter les rafraîchissements fréquents
  prolongés** et d'espacer d'environ 15 s. Une minuterie qui affiche les
  secondes rafraîchit chaque seconde : c'est exactement le point 1 des essais
  de CLAUDE.md. L'affichage réduit « 12:-- » (MAT-04) ramène le rythme à une
  fois par minute pendant l'essentiel du décompte.
- Un rafraîchissement complet périodique efface les traces : sa période est une
  constante (`CST_ECRAN_RAFRAICHISSEMENT_COMPLET`), à régler en essai.
- Éviter l'exposition prolongée aux UV (doc).
- L'image reste affichée hors tension (doc) : utile pour l'écran de veille
  (MAT-12) et pour les alertes persistantes.

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
  3,5 V.

## 4. Veille et réveil

C'est le point le plus délicat, et la documentation n'en dit presque rien.

### Ce que permet l'ESP32

| Mode | Consommation de l'ESP32 seul (fiche Espressif) | Réveil possible par | RAM |
|---|---|---|---|
| Veille légère | ~0,8 mA | minuterie, **n'importe quel GPIO** (niveau) | conservée |
| Veille profonde | ~10 µA | minuterie, GPIO RTC seulement (`ext0` : une broche ; `ext1` : plusieurs broches, réveil si **toutes** à 0 ou si une à 1) | perdue, sauf la mémoire RTC (8 Ko) |
| Extinction (GPIO 12 à 0) | 0 (seule l'horloge BM8563 reste alimentée) | bouton marche ; alarme de la BM8563 (à vérifier) | perdue |

La consommation du reste de la carte (régulateur, pont diviseur de la batterie,
tirages) s'ajoute et n'est pas documentée : c'est la mesure du point 6 des
essais.

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
  (CLAUDE.md).

Si la veille légère consomme trop pendant les décomptes, deux pistes : lire le
bouton du rappel uniquement aux réveils programmés (appui long nécessaire), ou
un programme ULP qui surveille la molette pendant la veille profonde.

### Maintien de l'alimentation pendant la veille

Le GPIO 12 doit rester à 1 pendant la veille profonde, sinon la carte s'éteint
(sur batterie). Il faut donc le figer avec `gpio_hold_en()` et
`gpio_deep_sleep_hold_en()`. À vérifier le soir, sur batterie et non sur USB.

### Exactitude du temps

En veille profonde, l'ESP32 compte le temps avec son oscillateur RC interne
(150 kHz), étalonné au démarrage. Sa dérive peut atteindre plusieurs pour cent
selon la température, ce qui est trop pour un décompte de plusieurs heures. Le
CoreInk n'a pas de quartz 32 kHz sur l'ESP32 (d'après le schéma, le seul quartz
32,768 kHz est celui de la BM8563). Pistes, par ordre de simplicité :

1. mesurer la dérive (vérification du soir) : si elle reste sous 0,1 %, la
   garder ;
2. sinon, recaler l'heure sur la BM8563 à chaque réveil (précision à la
   seconde) ;
3. ou utiliser la sortie `CLKOUT` de la BM8563 comme horloge lente, si elle est
   reliée à l'ESP32 (non lisible sur le schéma).

La veille légère utilise la même horloge lente : le quartz de 40 MHz est coupé
pendant la veille. Mesure de la soirée 1 (C8) : environ 3 s d'avance sur
10 min (0,5 %), départ du chronomètre imprécis. Depuis, l'horloge lente est le
8 MHz interne divisé par 256 (`CONFIG_RTC_CLK_SRC_INT_8MD256`), plus stable
selon Espressif, pour environ 5 µA de plus en veille profonde. Si la dérive
reste au-dessus de 0,1 %, on passe à la piste 2.

## 5. Buzzer

Buzzer passif sur GPIO 2, derrière un transistor : on le commande par une
fréquence (périphérique LEDC de l'ESP32). M5Stack utilise 1 kHz par défaut ;
un buzzer de ce type est en général plus fort vers sa fréquence de résonance
(souvent 2 à 4 kHz). La fréquence des bips est donc une constante, à régler à
l'oreille (point 5 des essais).

## 6. Sources

- Documentation M5Stack CoreInk : <https://docs.m5stack.com/en/core/coreink>
  (brochage, changement d'écran du 25 octobre 2024, batterie, conseils de
  rafraîchissement).
- Schéma : <https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/schematic/Core/coreink/coreink_sch.pdf>
  (texte seulement : composants et noms de réseaux).
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
