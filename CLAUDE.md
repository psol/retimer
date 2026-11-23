# CLAUDE.md

Guide de travail pour le dépôt Retimer. À lire avant toute intervention, avec
[SPEC.md](SPEC.md).

## Le projet

Retimer est une minuterie de cuisine physique : une minuterie globale décompte le
temps total, une minuterie rappel facultative sonne à intervalle régulier pendant
le décompte (retourner les légumes, arroser la viande). Le prototype de 2020
(`code.py`, CircuitPython sur Adafruit PyBadge) reste dans le dépôt comme
référence historique ; il n'est plus maintenu.

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
  CLAUDE.md, et non dans SPEC.md (voir « Interface réduite »).

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

Le CoreInk (ESP32-PICO-D4, e-ink 1,54" 200×200, molette trois positions,
bouton, buzzer, horloge BM8563, batterie LiPo de 390 mAh, boîtier) remplit les
critères obligatoires, mais avec des limites connues : écran petit, buzzer
faible, pas de piles du commerce, pas d'encodeurs.

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
3. intérêt de l'affichage réduit « 12:-- » (MAT-04) ;
4. réveil par la molette et le bouton depuis la veille (non documenté par
   M5Stack) ;
5. audibilité du buzzer en cuisine ;
6. autonomie réelle de la batterie.

## Interface réduite (CoreInk, option 3)

Le CoreInk n'a pas les deux encodeurs prévus par CMD-01 et CMD-02. Ses
commandes sont une molette à trois positions (haut, appui, bas), un bouton
situé physiquement sous la molette et un bouton marche. L'interface retenue pour cette étape est
« molette pour la globale, intervalles prédéfinis pour le rappel ».

La disposition physique guide l'écran : la molette est au-dessus du bouton, donc
la minuterie globale s'affiche en haut de l'écran et la minuterie rappel en
dessous. Chaque commande se trouve ainsi à côté de la valeur qu'elle règle.

| Commande CoreInk | Fonction | Remplace |
|---|---|---|
| Molette haut / bas | Augmente / diminue la minuterie globale, à partir de la valeur affichée. | CMD-01 |
| Molette maintenue en haut ou en bas | Répétition dont le pas grandit avec la durée du maintien. | REG-03 (rotation continue) |
| Bouton sous la molette | Chaque appui passe à l'intervalle de rappel suivant dans une liste prédéfinie, puis revient à « arrêt » après le dernier. | CMD-02 |
| Appui sur la molette | Acquitte l'échéance en attente. | CMD-03 |
| Bouton marche | Allumage et extinction de l'appareil. | — |

La liste des intervalles de rappel est une table de constantes (GEN-06), par
exemple : arrêt, 1, 2, 3, 5, 10, 15, 20, 30, 45, 60, 90, 120 min.

Ce qui reste conforme à SPEC.md :

- pas de modes (GEN-03) : chaque commande garde toujours la même fonction ;
- démarrage automatique après le délai de démarrage, relancé par toute action
  de réglage (DEM-01 à DEM-04) ;
- ramener la globale à zéro arrête le décompte et acquitte tout (ACQ-01) ;
  choisir « arrêt » dans la liste du rappel vaut ACQ-02 ;
- modification pendant le décompte (MOD-01 à MOD-03), les règles d'échéance
  (ECH), les états (ETA) et les erreurs (ERR) sont inchangés.

Écarts assumés :

- le rappel se règle par valeurs discrètes et non en continu (PLG-02, REG-01 à
  REG-03 pour le rappel) ; pour diminuer, il faut faire le tour de la liste ;
- l'accélération du réglage dépend de la durée du maintien de la molette, pas
  d'une rotation continue ;
- les alertes ne reposent pas sur la seule LED (voir « Alertes à l'écran » ci-dessous) ;
- pas de capteur « mains sales » dans cette étape (CMD-04 est souhaitable) ; un
  capteur pourra être branché plus tard sur le port Grove.

### Alertes à l'écran

La fiche technique du CoreInk mentionne une LED, mais son emplacement sur le
boîtier n'est pas évident : on peut l'utiliser en complément, mais on ne compte
pas que sur elle pour les alertes (CMD-07, MAT-05). Le signal principal passe
par l'écran, en inversant le noir et le blanc, par exemple :

- **alerte de rappel** : seule la zone du rappel (en bas) est inversée ;
- **alerte de fin** : tout l'écran est inversé.

Avantage de l'e-ink : l'image inversée reste affichée sans consommer, ce qui
convient à une alerte persistante et non intrusive (GEN-04). Le motif exact
(fixe ou clignotant lent) et l'usage de la LED en complément sont à régler en
essai (CST-06). À vérifier en essai :
que l'inversion reste visible de loin et ne laisse pas de traces.

Cette interface permet de commencer les essais d'utilisation tout de suite, en
une main, avec des doigts sales : en cuisine, le rappel se règle presque
toujours sur quelques valeurs rondes.

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
  sans carte.

Découpage prévu (composants ESP-IDF sous `components/`) :

| Composant | Rôle | Dépend du matériel |
|---|---|---|
| `minuterie` | États ETA, échéances ECH, acquittements ACQ, modifications MOD. Reçoit des événements et l'heure, rend l'état à afficher et la date du prochain réveil nécessaire. | Non |
| `reglage` | Pas de réglage selon la valeur de départ et l'accélération (REG, CST-03, CST-04), liste des intervalles de rappel. | Non |
| `constantes` | Tables de constantes (GEN-06, CST). | Non |
| `rendu` | Construit l'image à afficher (temps restants, alertes, clignotement, affichage réduit) dans un tampon, sans connaître l'écran. | Non |
| `saisie` | Traduit la molette et les boutons en événements abstraits (globale +/-, rappel suivant, acquitter) : anti-rebond, répétition accélérée. | Oui (CoreInk) |
| `ecran_eink` | Pilote de l'écran e-ink : envoi du tampon, mise à jour partielle, rafraîchissement complet périodique contre les traces. | Oui (CoreInk) |
| `signaux` | Motifs de sonnerie (buzzer) et d'alerte (LED, écran). | Oui (CoreInk) |
| `veille` | Mise en veille profonde de l'ESP32, réveil par minuterie et par broche, conservation de l'état en mémoire RTC. | Oui (ESP32) |
| `main` | Assemble le tout : réveil, lecture des événements, logique, rendu, programmation du prochain réveil, mise en veille. | Oui |

Points de conception :

- en veille profonde, l'ESP32 perd sa RAM sauf la mémoire RTC : l'état de
  `minuterie` doit tenir dans une structure simple, copiable en mémoire RTC ;
- le prochain réveil est calculé par `minuterie` (échéance, rafraîchissement
  de l'affichage, motif d'alerte) et appliqué par `veille` (GEN-05) ;
- on commence par la veille profonde de l'ESP32 (réveil par minuterie et par
  broche). L'extinction complète avec réveil par l'horloge BM8563 consomme
  moins, mais les boutons ne réveilleraient peut-être plus l'appareil : à
  mesurer avant de l'adopter.
