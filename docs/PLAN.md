# Retimer : plan de réalisation

Version de travail du 23 novembre 2026. Ce plan décrit comment passer du
prototype de 2020 à une version préliminaire de la nouvelle minuterie, l'essayer
en cuisine, puis la corriger. Il s'appuie sur [SPEC.md](../SPEC.md) (les besoins)
et [CLAUDE.md](../CLAUDE.md) (matériel provisoire M5Stack CoreInk, interface
réduite, chaîne ESP-IDF, architecture modulaire). Il ne modifie pas SPEC.md.

Sommaire :

1. Archivage du prototype de 2020
2. README
3. Étapes de développement
4. Tests techniques
5. Tests d'usage (essais d'utilisation)
6. Rapport d'essai et phase de correctifs
7. Versions et branches

---

## 1. Archivage du prototype de 2020

### Les deux options

| Option | Principe | Avantages | Inconvénients |
|---|---|---|---|
| **A. Tag puis suppression** | On pose un tag sur le dernier état du prototype, puis on supprime ses fichiers de la branche de développement. | Pratique la plus courante. L'arborescence de la nouvelle version reste propre. Le prototype reste consultable, téléchargeable et restaurable en une commande (`git checkout v1.1`). Rien n'est perdu : tout l'historique reste dans Git. | Le prototype n'est plus visible en parcourant les fichiers : il faut passer par le tag (lien dans le README). |
| B. Dossier `archive/` | On déplace les fichiers dans un dossier. | Visible tout de suite. | Du code mort dans l'arborescence, qui encombre les recherches, les outils (CI, analyse) et les sessions d'IA, qui le lisent comme s'il était actuel. L'historique Git le conserve déjà : c'est un doublon. |

**Ce qui se fait le plus : l'option A.** Un dossier d'archive se rencontre surtout
dans les projets sans gestion de version, ou quand l'ancien code doit continuer à
être compilé à côté du nouveau, ce qui n'est pas le cas ici : le code de 2020
fonctionne sur la carte de l'époque, flashée avec CircuitPython 5.3, mais la
nouvelle version ne le réutilise pas.

### Solution retenue : option A

Constat sur le dépôt :

- un tag `v1.0` existe déjà, sur le commit `1b45b58` (« Minimal set of
  features », 23 mai 2020) ;
- `master` a un commit de plus, `de68c8c` (« Photo added », 26 mai 2020) : la
  photo, son lien dans le README, et une petite correction de `code.py`
  (couleur des libellés).

On **ne déplace pas** `v1.0` : modifier un tag déjà publié est une mauvaise
pratique (les copies existantes gardent l'ancien). On ajoute un tag pour le
dernier état :

1. **Tag annoté `v1.1`** sur `de68c8c` (le `master` actuel), avec le message
   « Prototype CircuitPython sur Adafruit PyBadge (2020), dernier état avant la
   nouvelle version ». Tag poussé sur GitHub.
2. **Release GitHub** sur ce tag (facultatif, recommandé) : une page visible
   avec une archive zip et un résumé du prototype.
3. **Suppression sur la branche de développement**, en un commit dédié :
   - `code.py` et `fonts/Noto-18.bdf` (code et police du prototype) ;
   - les entrées propres à CircuitPython dans `.gitignore` (`boot_out.txt`,
     `lib`, `.fseventsd`, `.metadata_never_index`, `.Trashes`), remplacées par
     celles d'ESP-IDF (`build/`, `sdkconfig.old`, `managed_components/`…).
4. **On garde** : `LICENSE`, `.gitattributes`, et la photo
   `img/retimer_1055.jpg`, déplacée dans `docs/img/` : elle illustre la première
   version dans le nouveau README.
5. **CLAUDE.md** : la phrase « le prototype reste dans le dépôt comme référence
   historique » devient « le prototype est archivé sous le tag `v1.1` ».
6. Le README renvoie au tag : `https://github.com/psol/retimer/tree/v1.1`.

Pour retrouver le prototype : `git checkout v1.1`, ou le lien du tag sur GitHub.

`master` ne change qu'à la fusion de la pull request de la branche de
développement (section 7).

**À valider par Benoît avant exécution** : la poussée du tag et la suppression
des fichiers.

## 2. README

Le README reste en anglais. Un projet sur GitHub est lu par des visiteurs de
passage : il présente le projet, il ne remplace ni SPEC.md ni CLAUDE.md.

Contenu (le brouillon est sur la branche) :

1. **Retimer** en une phrase : une minuterie de cuisine qui décompte le temps
   total et rappelle régulièrement d'agir (retourner les légumes, arroser la
   viande).
2. **Version 1 (2020)** : prototype CircuitPython sur Adafruit PyBadge, un
   « retimer » qui se relance et deux minuteries simples, commande sans les mains
   par le capteur de luminosité. Ce qu'il a validé (l'utilité du rappel
   régulier) et ses limites (autonomie, capteur peu fiable en faible lumière,
   code lié à CircuitPython 5.3 : il fonctionne toujours sur la carte de
   l'époque, mais une carte neuve avec un CircuitPython récent demanderait un
   portage). Photo et lien vers le tag `v1.1`.
3. **Version 2 (en cours)** : objectifs : une minuterie globale et une minuterie
   rappel, pas de modes ni de bouton de démarrage, alertes non intrusives,
   autonomie maximale (microcontrôleur en veille), tout sur l'appareil. Matériel
   provisoire M5Stack CoreInk pour essayer l'e-ink en cuisine. Liens vers
   SPEC.md et CLAUDE.md (en français, signalé).
4. **Built with AI** : le choix de développer avec Claude Code et la méthode :
   spécification d'abord, identifiants d'exigences cités jusque dans les tests,
   règles de travail dans CLAUDE.md, décisions et relectures humaines, essais
   en cuisine par une personne.
5. **Project layout**, **Building** (ESP-IDF, Mac Apple Silicon natif) et
   **Running the tests** : ajoutés quand le code existera (étapes 1 et 2).
6. **License** : inchangée.

## 3. Étapes de développement

Chaque étape se termine par une pull request relue par Benoît. Les exigences
couvertes sont citées dans les commits et les tests.

| Étape | Contenu | Livrable | Fin d'étape |
|---|---|---|---|
| **0. Préparation** | Archivage (section 1), README (section 2), ce plan. Pas de code applicatif. | PR de documentation. | PR fusionnée dans `master`. |
| **1. Chaîne et squelette** | Installer ESP-IDF et esptool natifs arm64 sur le Mac, vérifier avec `file`. Projet ESP-IDF vide avec les composants de CLAUDE.md (en-têtes seulement). Projet de tests sur l'hôte (section 4). Intégration continue GitHub Actions. Un « bonjour » flashé sur le CoreInk et lu dans `idf.py monitor`. | Dépôt qui compile, CI au vert, procédure d'installation dans le README. | Le CoreInk affiche un texte flashé depuis le Mac, sans Rosetta. |
| **2. Logique métier, sur l'hôte** | `constants`, `adjust`, `kitchen_timer`, `render`, écrits avec leurs tests, sans carte. Ordre : constantes → reglage (REG, PLG) → minuterie (ETA, DEM, ECH, ACQ, MOD, ERR) → rendu (CMD-05, alertes inversées, clignotement, affichage réduit). | Composants testés ; images de référence du rendu. | Toutes les exigences de la partie 1 applicables au CoreInk ont au moins un test. |
| **3. Pilotes CoreInk** | `eink_display`, `input`, `signals`, `power`. Chacun avec une petite application de test sur la carte. | Pilotes et applications de test. | Chaque pilote passe sa vérification sur la carte (section 4.3). Consommation en veille mesurée. |
| **4. Intégration** | `main` : réveil, événements, logique, rendu, prochain réveil, mise en veille. Tests de bout en bout sur la carte. | Version préliminaire `v2.0`. | Recette technique passée (section 4.4). |
| **5. Essais d'utilisation, tour 1** | Quelques jours en cuisine (section 5). | Rapport d'essai. | Rapport rédigé. |
| **6. Correctifs** | Tri du rapport, corrections, ajustement des constantes, évolutions de SPEC.md avec l'accord de Benoît (section 6). | `v2.1`, puis nouveau tour d'essai si nécessaire. | Critères de sortie de la section 6. |
| **7. Bilan matériel** | Décision e-ink ou Sharp Memory LCD, à partir des points d'observation de CLAUDE.md. | Note de décision. | Choix du matériel suivant, ou CoreInk confirmé. |

La logique métier (étape 2) est la plus grosse partie et ne demande pas la
carte : on peut avancer dessus pendant que la chaîne de flashage se met en
place.

### Travail sans le Mac ni le CoreInk

Benoît n'a accès au Mac et au CoreInk que le soir. Le travail est donc réparti
en deux files : la journée, Claude avance dans le cloud sur ce qui ne demande
ni la carte ni le Mac, et Benoît relit les pull requests depuis son téléphone ;
le soir, on fait sur le Mac seulement ce qui en a vraiment besoin.

| Sans le Mac (journée, dans le cloud) | Avec le Mac et le CoreInk (soir) |
|---|---|
| **Recherche documentaire** sur le CoreInk : brochage de la molette, du bouton, du buzzer et de la LED, contrôleur de l'écran et ses modes de mise à jour partielle, broches capables de réveiller l'ESP32 depuis la veille profonde, mesure de la tension de la batterie. Résultat : une fiche `docs/coreink.md` avec ses sources, qui sert d'entrée aux pilotes. | Installer ESP-IDF et esptool, vérifier qu'ils sont natifs (`file`). |
| **Étape 1, partie hôte** : squelette du dépôt, projet de tests `test/host/`, intégration continue GitHub Actions (tests sur un runner macOS arm64, compilation du firmware dans l'image Docker d'ESP-IDF). | Flasher le « bonjour » et lire la console : fin de l'étape 1. |
| **Étape 2 en entier** : `constants`, `adjust`, `kitchen_timer`, `render`, avec leurs tests. Les images du rendu sont jointes aux pull requests, pour relire les écrans sans carte. | — |
| **Étape 3, premier jet** : pilotes écrits d'après la fiche `docs/coreink.md` et compilés par la CI, mais pas encore vérifiés. | Applications de test des pilotes sur la carte, mesures de consommation, corrections. |
| **Étape 4** : assemblage dans `main`, compilé par la CI. | Recette technique (section 4.4), tag `v2.0`. |
| Modèles de journal et de rapport d'essai (`docs/essais/`). | — |

Limites :

- un pilote écrit sans la carte n'est qu'une hypothèse : il n'est réputé
  fonctionner qu'après sa vérification sur la carte, et il faut s'attendre à
  des corrections le soir ;
- la CI vérifie que le firmware compile, pas qu'il tourne ;
- le temps du soir passe surtout à flasher et observer : il vaut mieux
  préparer une liste précise de vérifications pour chaque soirée.

## 4. Tests techniques

Les tests techniques vérifient que le logiciel **fait ce que dit SPEC.md**. Ils
sont automatisés autant que possible et ne demandent pas de cuisiner.

### 4.1 Pourquoi pas seulement un firmware de test suivi sur la console

La pratique Arduino (flasher un programme de test et lire la console) marche,
mais elle est lente : flasher prend du temps, il faut la carte, et on ne peut pas
attendre 24 h pour tester une échéance. La pratique courante en embarqué est de
**séparer la logique du matériel** et de tester la logique **sur l'ordinateur**
(tests sur l'hôte), en quelques secondes. L'architecture de CLAUDE.md est
conçue pour cela. Le test sur la carte reste utile, mais seulement pour ce qui
touche au matériel.

| Niveau | Quoi | Où | Quand |
|---|---|---|---|
| Tests unitaires sur l'hôte | `constants`, `adjust`, `kitchen_timer`, `render` | Mac et CI | À chaque modification ; avant chaque commit. |
| Tests de scénario sur l'hôte | Séquences d'événements complètes (régler, démarrer, rappels, fin) avec une horloge simulée | Mac et CI | Idem. |
| Compilation du firmware | Projet ESP-IDF complet | CI | À chaque poussée. |
| Tests sur la carte | Pilotes `eink_display`, `input`, `signals`, `power` | CoreInk, console série | À chaque modification d'un pilote. |
| Recette technique | Version complète, liste de contrôle | CoreInk | Avant chaque version d'essai. |

### 4.2 Tests sur l'hôte

- **Outil** : [Unity](https://github.com/ThrowTheSwitch/Unity), framework de
  test en C très répandu en embarqué, celui qu'utilise ESP-IDF. Le même code de
  test pourra tourner sur la carte si besoin.
- **Projet** : `test/host/`, un projet CMake indépendant d'ESP-IDF, compilé
  avec le compilateur du Mac (clang natif arm64) et lancé avec `ctest`. Il ne
  compile que les composants sans dépendance matérielle : c'est aussi la
  preuve qu'ils restent portables.
- **Temps simulé** : `kitchen_timer` reçoit l'heure de l'extérieur (CLAUDE.md). Un
  test peut donc faire avancer le temps de 24 h en une microseconde et vérifier
  chaque échéance au milliseconde près (PLG-01, ECH-05, ECH-07, ERR-03).
- **Traçabilité** : chaque test porte dans son nom l'exigence qu'il vérifie,
  par exemple `test_ECH_05_rappel_repart_de_l_acquittement`. Un petit script
  liste les identifiants de la partie 1 sans test.
- **Rendu** : `render` produit un tampon 200×200. Les tests l'écrivent en image
  PBM et le comparent à une image de référence enregistrée dans le dépôt (tests
  d'instantané). On relit ainsi les écrans sans flasher, et on voit toute
  modification involontaire de l'affichage.
- **Qualité** : avertissements traités comme des erreurs (`-Wall -Wextra
  -Werror`), et AddressSanitizer et UndefinedBehaviorSanitizer activés sur
  l'hôte pour détecter les débordements et calculs indéfinis.
- **Intégration continue** : GitHub Actions lance les tests sur un runner
  macOS arm64 (ce qui vérifie aussi GEN-07 pour cette partie) et compile le
  firmware dans l'image Docker officielle d'ESP-IDF.

### 4.3 Tests sur la carte

Pour les pilotes, on garde la pratique connue, en la structurant : une
**application de test par pilote** (sous `test/board/`), avec des tests Unity
qui s'affichent dans `idf.py monitor`, et des vérifications à l'œil ou à
l'oreille quand l'automatisme n'est pas possible :

- `eink_display` : mise à jour partielle chaque seconde pendant une heure, puis
  observation des traces ; rafraîchissement complet ; inversion d'une zone ;
- `input` : anti-rebond, répétition accélérée au maintien de la molette ;
- `signals` : motifs de sonnerie (CST-05) et LED ;
- `power` : réveil par minuterie et par chacune des commandes, conservation
  de l'état en mémoire RTC, et **mesure de la consommation** en veille et
  éveillé (multimètre ou wattmètre USB), pour estimer l'autonomie.

Plus tard, l'outil `pytest-embedded` d'Espressif peut lire la console
automatiquement ; il ne vaut l'investissement que si les tests sur la carte se
multiplient.

### 4.4 Recette technique

Liste de contrôle passée sur le CoreInk avant de remettre une version à
l'essai : chaque transition du tableau ETA (ETA-10 à ETA-21), les cas d'erreur
(ERR-01 à ERR-08), un décompte court (PLG-03), un décompte long avec rappels,
et une nuit en veille. Le résultat est noté dans la pull request de la version.

## 5. Tests d'usage (essais d'utilisation)

Les tests d'usage vérifient que **ce que dit SPEC.md convient en cuisine**. Ils
ne cherchent pas des bogues (c'est le rôle des tests techniques), mais des
réponses : les valeurs CST, les comportements OUV, et les points à observer sur
le CoreInk listés dans CLAUDE.md.

Les deux sortes de tests sont séparées :

| | Tests techniques | Tests d'usage |
|---|---|---|
| Question | Le logiciel est-il conforme à la spécification ? | La spécification est-elle la bonne ? |
| Qui | Automatique, et Claude ou Benoît sur la carte | Benoît, en cuisinant |
| Quand | En continu | Sur une version d'essai, après la recette technique |
| Référence | Partie 1 de SPEC.md | Partie 2 de SPEC.md et points d'observation de CLAUDE.md |
| Résultat | Vert ou rouge | Rapport d'essai |
| Où c'est rangé | `test/` | `docs/essais/` |

Déroulement d'un tour d'essai :

1. une version taguée (`v2.0`, `v2.1`…), avec ses constantes ;
2. quelques jours d'utilisation normale, sans forcer les situations ;
3. un journal court rempli après chaque utilisation (5 minutes) ;
4. le rapport d'essai rédigé à la fin du tour, à partir du journal.

Un modèle de journal et de rapport sera créé dans `docs/essais/` avant le
premier tour (étape 4).

## 6. Rapport d'essai et phase de correctifs

### 6.1 Ce qu'on attend du rapport d'essai

Un fichier par tour, `docs/essais/essai-N.md` :

1. **Contexte** : dates, version et commit essayés, valeurs des constantes.
2. **Journal résumé** : une ligne par utilisation (date, plat, programme
   réglé, durée, nombre de rappels, incident éventuel).
3. **Réponses aux questions ouvertes** : pour chaque OUV et CST concerné,
   ce qui a été observé, la valeur ou le comportement proposé, et le degré de
   certitude (net, à confirmer, pas d'avis).
4. **Points d'observation du CoreInk** (CLAUDE.md) : lisibilité et traces,
   clignotement, affichage réduit, réveil, audibilité du buzzer, autonomie
   mesurée.
5. **Observations** : tout le reste, chacune classée :
   - **défaut** : la minuterie ne fait pas ce que dit SPEC.md ;
   - **ajustement** : une constante à changer ;
   - **évolution** : SPEC.md devrait dire autre chose ;
   - **matériel** : limite du CoreInk, à garder pour le bilan de l'étape 7.

   Avec une gravité : **bloquant** (empêche d'utiliser la minuterie),
   **gênant**, **mineur**.
6. **Avis global** : la minuterie a-t-elle servi ? qu'est-ce qui a manqué ?

Claude peut préparer le rapport à partir du journal ; Benoît le valide.

### 6.2 Phase de correctifs

Le rapport est trié en lots, chaque lot dans sa propre pull request :

| Catégorie | Traitement |
|---|---|
| Défaut | D'abord un test qui reproduit le défaut et échoue, puis la correction. Le test reste. |
| Ajustement | Changement de la table de constantes, avec l'observation qui le justifie dans le commit. |
| Évolution | Mise à jour de SPEC.md avec `/spec`, **avec l'accord de Benoît** : un OUV tranché devient une exigence de la partie 1 avec un nouvel identifiant. Puis développement et tests. |
| Matériel | Noté pour le bilan de l'étape 7 ; contournement logiciel si c'est simple. |

Ordre : les défauts bloquants, puis les évolutions de SPEC.md (elles changent ce
qu'on teste), puis le reste.

Après les correctifs, recette technique (section 4.4), version suivante
(`v2.1`, `v2.2`…), et nouveau tour d'essai si des questions restent ouvertes.

### 6.3 Évolutions et ajustements déjà notés

Notés pendant les vérifications sur la carte (soirée 1, 23 novembre 2026), à
traiter dans une étape suivante, après les essais :

- **Relancer le dernier programme** (évolution, proposée par Benoît) : un
  bouton relance toute la minuterie avec la globale et le rappel réglés la
  dernière fois, pour refaire la recette précédente sans tout régler de
  nouveau. Sur le CoreInk, le bouton au-dessus de l'écran (GPIO 5) est libre.
  Il ne réveille pas de la veille profonde : en Veille, il faudrait d'abord
  réveiller la carte, ou choisir un autre geste (appui long sur la molette,
  par exemple). À rédiger dans SPEC.md avec `/spec` : quel programme est
  gardé, s'il survit à l'extinction (ERR-08 dit aujourd'hui que rien n'est
  conservé après une coupure), et le retour à l'écran.
- **Accélération de la molette** (ajustement, CST-04) : elle accélère trop vite
  sur les petites valeurs. Pistes : faire dépendre l'accélération de la valeur
  (petits pas sous 1 à 2 min), ou allonger les paliers de maintien.
- **Campagne d'essais du réglage à la molette** (demande de Benoît, soirée 2) :
  molette maintenue, « ça saute ultra vite ». Préparer un firmware d'essai où
  les constantes du réglage (CST-03 paliers, CST-04 accélération, délai et
  période de répétition) se changent sans recompiler, par exemple par des
  jeux de valeurs choisis au bouton au-dessus de l'écran ou par la console
  série, avec un journal de chaque essai. Puis une soirée dédiée pour choisir
  les valeurs.
- **Consommation** (demande de Benoît, 24 novembre 2026), par ordre de gain
  estimé, non mesuré :
  1. veille légère de l'ESP32 pendant que l'écran se met à jour (environ
     330 ms par mise à jour, aujourd'hui passées éveillé à 160 MHz à
     attendre la broche BUSY) ;
  2. éteindre le contrôleur de l'écran après chaque mise à jour pendant le
     décompte (il n'est éteint qu'avant la veille profonde) ;
  3. journaux série coupés dans la version d'essai (y compris ceux du
     démarrage à chaque réveil de veille profonde), gardés dans une version
     de développement ;
  4. fréquence du CPU à 80 MHz au lieu de 160 MHz ;
  5. ~~couper la sortie 32 kHz de l'horloge BM8563 par I2C au démarrage~~ :
     abandonné le 24 novembre 2026, la broche CLKOUT n'est pas connectée
     (schéma lu par Benoît) : rien à gagner côté carte.
  Éléments chiffrés trouvés depuis (calculs d'après le schéma, non mesurés,
  détail dans `docs/coreink.md`, section 6) : le pont de mesure de la batterie
  (R41 + R42, 25,1 kΩ) est branché en permanence et consomme ~170 µA, plus de
  quinze fois l'ESP32 en veille profonde ; R32 ajoute ~42 µA tant que le
  maintien GPIO 12 est actif ; le plancher théorique en veille profonde est
  d'environ 230 µA avant les convertisseurs (de l'ordre de 70 jours sur la
  batterie de 390 mAh). La lecture de l'ADC, elle, est négligeable. Le courant
  de repos des convertisseurs est inconnu. Priorité de la mesure d'autonomie :
  courant total sur batterie en veille profonde, avant toute optimisation du
  logiciel.
  Pour le point 1 : l'attente est dans `wait_ready()` (eink_display.c), une
  boucle sur BUSY avec `vTaskDelay(2 ms)` ; sans gestion d'énergie
  (`CONFIG_PM_ENABLE=n`), le cœur s'arrête entre deux tics mais la PLL et les
  périphériques restent alimentés. Remplacer par un réveil sur BUSY à 1 et
  `esp_light_sleep_start()`.
  **Implémentation obligatoire** (décision de Benoît, 24 novembre 2026) :
  toute mise en veille passe par `power`, qui reste le seul module à appeler
  les fonctions de veille d'ESP-IDF. `power` offre un service unique
  `power_wait_until(broche, niveau, délai max)` qui met en veille légère
  jusqu'au changement de la broche ou au délai. Le pilote d'écran ne connaît
  pas `power` : il reçoit une fonction d'attente à l'initialisation
  (`eink_config_t`), qu'il appelle à la place de sa boucle `vTaskDelay`.
  `main` fait le branchement. Même mécanisme pour les notes du buzzer
  (`signals`).
  Puis mesurer l'autonomie sur batterie pour chiffrer le gain. Déjà en place :
  WiFi et Bluetooth jamais démarrés, veille légère entre deux changements
  d'affichage avec réveil par les boutons, veille profonde en Veille, écran
  jamais redessiné à l'identique.
- **Pistes d'utilisation de l'horloge BM8563** (discussion du 24 novembre 2026,
  aucune mise en œuvre décidée ; détail et réserves dans `docs/coreink.md`,
  section 4, « La BM8563 »). Schéma lu par Benoît : VDD sur SYS_BAT (l'horloge
  compte carte éteinte), CLKOUT non connectée, INT en parallèle sur GPIO 19 et
  sur PWR_TRIG. Trois options à comparer après mesure :
  - **A** : veille profonde actuelle (horloge lente 8MD256, broches RTC) ;
  - **B** : veille légère pendant les décomptes, réveil par INT sur GPIO 19
    (alarme ou minuterie programmée par I²C dans le BM8563) ;
  - **C** : extinction complète, l'alarme du BM8563 rallume la carte par
    PWR_TRIG. Réserves : effacer le drapeau d'alarme avant d'éteindre (sinon la
    carte ne s'éteint pas) ; seuls le bouton marche et l'alarme rallument (pas
    la molette) ; état de la minuterie à garder dans le BM8563 ; démarrage à
    froid plus long ; gain limité tant que le pont de batterie reste branché
    (~60 µA sur ~230 µA).
  Mesures proposées (`docs/verifications-carte.md`, soirée F) : alarme à
  1 minute, effacement du drapeau, réveil de la veille légère par GPIO 19,
  courant dans chaque mode, dérive de l'horloge lente sur 30 min à deux
  températures.
- **Rafraîchissement de l'écran** (fiche du GDEY0154D67, valable par hypothèse
  pour le GDEW0154M09) : rafraîchir au moins toutes les 24 h, et un complet
  après 5 partiels consécutifs. À comparer à `CST_DISPLAY_PARTIALS_BEFORE_FULL`
  et à l'écran de veille laissé affiché une nuit. À poser à Benoît (pas à
  inscrire dans SPEC.md sans son accord). Ajouter aux essais un compteur de
  rafraîchissements (complets et partiels) en mémoire RTC, affiché au
  diagnostic, et des photos du contraste avant et après plusieurs heures.
- **Format de l'image** (remarque de Benoît, 24 novembre 2026) : `render`
  dessine en « 1 = noir » et le pilote recopie chaque ligne en l'inversant
  pour le contrôleur. Passer à `render` une option de rendu (polarité des
  points, négatif) pour que le pilote envoie le tampon tel quel ; le négatif
  du rafraîchissement complet serait un second rendu. `render` reste
  indépendant du contrôleur : c'est le pilote qui déclare sa polarité.
- **Délai de démarrage** (ajustement, CST) : 3 s, jugé court pour régler
  plusieurs minutes.
- **Premier appui perdu au réveil** (défaut) : depuis la Veille, il faut
  appuyer, attendre, puis appuyer de nouveau.

**Critères de sortie** des essais : aucun défaut bloquant ou gênant ouvert, les
OUV et CST concernés tranchés, les points d'observation du CoreInk renseignés.
On passe alors au bilan matériel (étape 7).

## 7. Versions et branches

- `master` : la version stable. On y arrive par pull request, après relecture.
- Branches de travail : une par étape ou par lot de correctifs.
- Tags, dans la continuité de `v1.0` et `v1.1` (le prototype) :
  - `v2.0` : la première version préliminaire, remise en essai ;
  - `v2.1`, `v2.2`… : une version par tour de correctifs, chacune remise en
    essai ;
  - `v2.0.1`, `v2.1.1`… : une correction urgente pendant un tour d'essai (un
    défaut bloquant), sans attendre la fin du tour.
- Le rapport d'essai cite la version essayée. La fin des essais n'a pas de
  numéro à part : c'est la dernière version `v2.x` qui remplit les critères de
  sortie, notée comme telle dans sa release.
- Les tags sont annotés et ne sont jamais déplacés une fois poussés.

## Points à valider par Benoît

1. Archivage par tag `v1.1` puis suppression (section 1), avec la release GitHub.
2. Le contenu du README (brouillon sur la branche).
3. Le découpage en étapes et les deux sortes de tests.

Remarque sans modification : dans la section 13 de SPEC.md, la phrase « Seules
les questions à valider pendant les essais restent ici… » est placée entre
l'en-tête du tableau des OUV et ses lignes, ce qui casse l'affichage du tableau
sur GitHub. À corriger avec ton accord.
