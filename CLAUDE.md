# CLAUDE.md

Guide de travail pour le dépôt Retimer. À lire avant toute intervention, avec
[SPEC.md](SPEC.md).

## Le projet

Retimer est une minuterie de cuisine physique : une minuterie globale décompte le
temps total, une minuterie rappel facultative sonne à intervalle régulier pendant
le décompte (retourner les légumes, arroser la viande). Le prototype de 2020
(CircuitPython sur Adafruit PyBadge) est archivé sous le tag `v1.1`
(`git checkout v1.1`) ; il n'est plus maintenu. Le plan de réalisation est
dans [docs/PLAN.md](docs/PLAN.md).

On écrit en français : documentation, messages de commit, commentaires.

## Rôle de SPEC.md

[SPEC.md](SPEC.md) est la référence des besoins. Il décrit le comportement
attendu de la minuterie, indépendamment du matériel.

- **Partie 1** : ce qui est établi. **Partie 2** : ce qui reste à définir
  (matériel `MAT`, constantes `CST`, comportements à valider en essai `OUV`).
- Chaque exigence porte un **identifiant stable** (`DEM-01`, `ACQ-04`…), jamais
  réutilisé ni renuméroté. On les cite dans les commits, les pull requests, les
  tests et les commentaires du code (« Implémente DEM-01 »).
- Les `OUV-xx` sont réservés aux questions qu'on ne peut trancher qu'en essai
  d'utilisation. Une question qui se tranche sans essai se pose directement à
  Benoît.
- **On ne modifie pas SPEC.md sans l'accord de Benoît.** La commande
  `/spec` (`.claude/commands/spec.md`) sert à le faire évoluer.
- Les écarts imposés par le matériel du moment sont décrits ici, dans
  CLAUDE.md, et non dans SPEC.md (voir [EINK.md](EINK.md)).

## Choix du matériel (provisoire) : M5Stack CoreInk

### Pourquoi le CoreInk

Le choix est **tactique**. Le CoreInk n'est pas la solution qui colle le mieux
aux besoins documentés : c'est un appareil excédentaire, disponible tout de
suite, sans aucun surcoût.

La [comparaison du 22 novembre 2026](docs/comparaison-materiel.md) (critères, solutions retenues, sources)
recommandait plutôt :

- **à construire** : une carte Seeed XIAO nRF52840 avec un écran Sharp Memory
  LCD, deux encodeurs rotatifs et deux piles AA (~70 €), la plus sobre en veille ;
- **packagé** : un M5Stack PaperMono (e-paper tactile, NFC) ou un PaperS3.

Le CoreInk (ESP32-PICO-D4, cœur Xtensa LX6, e-ink 1,54" 200×200 GDEW0154M09,
molette trois positions, bouton, buzzer, horloge BM8563, batterie LiPo de
390 mAh, boîtier) remplit les critères obligatoires, mais avec des limites
connues : écran petit, buzzer faible, pas de piles du commerce, pas
d'encodeurs. L'écran GDEW0154M09 est arrêté (remplacé par le GDEY0154D67, même
brochage mais autres commandes) et fonctionne de 0 à 50 °C : une cuisine
convient, pas un four ouvert. Détails, durée de vie et consommation : voir
[docs/coreink.md](docs/coreink.md).

### La question ouverte : e-ink ou Sharp Memory LCD

L'adéquation de l'e-ink à une minuterie reste à démontrer :

- l'e-ink garde l'image sans consommer, mais un rafraîchissement partiel chaque
  seconde et un clignotement (DEM-02) risquent de laisser des traces et
  d'user l'écran ;
- le Sharp Memory LCD se rafraîchit instantanément pour quelques µA, mais perd
  l'image hors tension et n'existe pour l'instant qu'en solution à souder.

Le choix tactique rejoint cette question : le CoreInk permet de **tester l'e-ink
sur le terrain**, lors des essais d'utilisation. Si l'e-ink ne convient pas,
cela confirmera le besoin de la solution à souder avec écran Sharp.

Points à observer pendant les essais :

1. lisibilité et traces après des heures de rafraîchissement partiel ;
2. rendu du clignotement de démarrage imminent ;
3. intérêt des niveaux d'affichage dizaine et minute (MAT-04, EINK.md) ;
4. réveil par la molette et le bouton depuis la veille (non documenté par
   M5Stack ; vérifié depuis le 24 novembre 2026) et réveil par l'alarme du
   BM8563 depuis l'extinction complète (à tester) ;
5. audibilité du buzzer en cuisine ;
6. autonomie réelle de la batterie : courant total sur batterie dans chaque
   mode (veille profonde, veille légère, extinction). Le pont de mesure de la
   batterie consomme à lui seul environ 170 µA tant que la carte est allumée
   (il est coupé avec Q4 à l'extinction), plus que l'ESP32 en veille profonde (calcul d'après le schéma, `docs/coreink.md`,
   section 6).

## Interface réduite et affichage e-ink

L'interface réduite du CoreInk (molette pour la globale, intervalles prédéfinis
pour le rappel, au lieu des deux encodeurs de CMD-01 et CMD-02), la logique
d'affichage de l'écran e-ink (niveaux précis, dizaine et minute, rémanence du
réglage, rafraîchissements, démarrage imminent, alertes à l'écran) et les
écarts assumés sont décrits dans [EINK.md](EINK.md), avec des besoins numérotés
`INK-xx`. Même règle que pour SPEC.md : on cite ces identifiants dans les
commits, les tests et le code, et on ne les renumérote pas.

Commandes, en résumé : molette haut / bas pour la globale, bouton sous la
molette (appui court) pour le rappel suivant, appui sur la molette pour
acquitter, bouton maintenu 3 s pour éteindre. La minuterie globale s'affiche
en haut, le rappel en dessous.

## Chaîne de développement

### Contrainte

Toute la chaîne (compilateur, flashage, débogage) doit tourner nativement sur un
Mac Apple Silicon, sans Rosetta (GEN-07). C'est d'autant plus important
qu'Apple a annoncé que macOS 27 est la dernière version à prendre pleinement en
charge les applications Intel : à partir de macOS 28 (vers septembre 2027), un
outil qui dépend de Rosetta doit être considéré comme cassé.

### Choix : ESP-IDF

- **ESP-IDF v5.x** avec `idf.py` et CMake. Ses outils sont publiés en
  `macos-arm64` : compilateur `xtensa-esp-elf`, gdb, `openocd-esp32`.
- **esptool v5** pour le flashage, natif arm64 (paquet Python, ou archive
  `macos-arm64`).
- **Langage : C (C11).** La logique métier n'utilise que la bibliothèque
  standard, pour rester portable vers une autre plateforme (Zephyr sur nRF52,
  Pico SDK, STM32).
- **Débogage** : journaux série (`idf.py monitor`). L'ESP32-PICO-D4 du CoreInk
  n'a pas de JTAG par USB ; un JTAG externe est possible mais ses broches ne
  sont pas accessibles dans le boîtier.

### À éviter, et pourquoi

| Outil | Problème |
|---|---|
| IDE Arduino 2, arduino-cli | Compilent les `.ino` avec `ctags`, encore x86 uniquement : Rosetta obligatoire. |
| M5Burner | Application Intel uniquement. Inutile : esptool suffit. |
| PlatformIO | Peut installer silencieusement des chaînes x86 (compilateur ARM, `mkspiffs`, `mklittlefs`). |

Pour vérifier qu'un outil est natif : `file $(which <outil>)` doit indiquer
`arm64`.

## Architecture logicielle

Le matériel de cette étape est provisoire. Le code est donc **modulaire** : chaque
composant doit pouvoir être extrait et réutilisé sur une autre architecture
(nRF52840 avec écran Sharp, par exemple) sans emporter les autres.

Règles :

- la logique métier ne dépend ni d'ESP-IDF ni du CoreInk ; le temps et les
  événements lui sont fournis de l'extérieur ;
- chaque dépendance au matériel passe par une interface courte (en-tête C) ;
  un portage ne réécrit que l'implémentation ;
- la logique métier se compile et se teste aussi sur le Mac (tests sur l'hôte),
  sans carte ;
- **tout bogue qui peut se tester sans la carte a un test automatique** sur
  l'hôte (`test/host`), ajouté avec sa correction. Si la logique en cause est
  dans `main` ou dans un pilote, on la sort d'abord dans un composant sans
  dépendance au matériel. Le test doit échouer sans la correction. Un bogue
  qui ne se teste que sur la carte (veille, broches, horloge, tâches
  FreeRTOS) a une vérification dans `docs/verifications-carte.md` ;
- le code est en anglais (noms de fichiers, de composants, de fonctions, de
  types, de variables, de constantes), les commentaires et la documentation
  restent en français. Les identifiants d'exigences restent tels quels, y
  compris dans les noms des tests (`test_ECH_05_ETA_16_reminder_restarts_from_ack`).

Découpage prévu (composants ESP-IDF sous `components/`) :

| Composant | Rôle | Dépend du matériel |
|---|---|---|
| `kitchen_timer` (minuterie) | États ETA, échéances ECH, acquittements ACQ, modifications MOD. Reçoit des événements et l'heure, rend l'état à afficher et la date du prochain réveil nécessaire. | Non |
| `adjust` (réglage) | Pas de réglage selon la valeur de départ et l'accélération (REG, CST-03, CST-04), liste des intervalles de rappel. | Non |
| `constants` | Tables de constantes (GEN-06, CST). | Non |
| `render` (rendu) | Construit l'image à afficher (temps restants, alertes, clignotement, affichage réduit) dans un tampon, sans connaître l'écran. | Non |
| `controller` (commande) | Applique les événements de saisie à la minuterie (réglage, affichage réduit, acquittement, extinction) et prend les petites décisions de `main` : rafraîchissement complet de l'écran, sonnerie, batterie faible, prochain réveil. | Non |
| `input` (saisie) | Traduit la molette et les boutons en événements abstraits (globale +/-, rappel suivant, acquitter) : anti-rebond, répétition accélérée. | Oui (CoreInk) |
| `eink_display` | Pilote de l'écran e-ink : envoi du tampon, mise à jour partielle, rafraîchissement complet périodique contre les traces. | Oui (CoreInk) |
| `signals` (signaux) | Motifs de sonnerie (buzzer) et d'alerte (LED, écran). | Oui (CoreInk) |
| `power` (veille, alimentation) | Mise en veille profonde de l'ESP32, réveil par minuterie et par broche, conservation de l'état en mémoire RTC. | Oui (ESP32) |
| `main` | Branche les pilotes : réveil, tâche de saisie, appel de `controller`, rendu, affichage, programmation du prochain réveil, mise en veille. | Oui |

Points de conception :

- en veille profonde, l'ESP32 perd sa RAM sauf la mémoire RTC : l'état de
  `kitchen_timer` doit tenir dans une structure simple, copiable en mémoire RTC ;
- le prochain réveil est calculé par `kitchen_timer` (échéance, rafraîchissement
  de l'affichage, motif d'alerte) et appliqué par `power` (GEN-05) ;
- on commence par la veille profonde de l'ESP32 (réveil par minuterie et par
  broche). L'extinction complète avec réveil par l'horloge BM8563 est faisable
  matériellement d'après le schéma lu par Benoît le 24 novembre 2026 : VDD du
  BM8563 sur SYS_BAT (l'horloge compte carte éteinte), INT reliée à PWR_TRIG,
  qui rallume la carte. Mais seuls le bouton marche et l'alarme rallument
  (la molette ne passe pas par PWR_TRIG), le drapeau d'alarme doit être
  effacé avant l'extinction (sinon la carte ne s'éteint pas), l'état de la
  minuterie doit être gardé ailleurs que dans la mémoire RTC de l'ESP32 (flash,
  ou date de fin dans l'alarme du BM8563, qui n'a pas de mémoire libre). Le gain d'énergie est important
  (le pont de batterie, ~170 µA, est coupé avec Q4 ; reste quelques µA) mais
  le démarrage à froid coûte cher pour des réveils fréquents : à tester et à
  mesurer avant de l'adopter. La broche CLKOUT de la BM8563 n'est pas connectée. Voir
  `docs/coreink.md`, section 4, « La BM8563 ».
