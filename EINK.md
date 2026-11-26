# EINK.md

Comportement attendu de l'affichage sur l'écran e-ink du M5Stack CoreInk. À lire
avec [SPEC.md](SPEC.md) (besoins indépendants du matériel) et
[CLAUDE.md](CLAUDE.md) (guide de travail). La fiche matérielle de l'écran est
dans [docs/coreink.md](docs/coreink.md).

## Introduction

### Pourquoi ce document

L'écran e-ink garde l'image sans consommer, mais il est lent (0,33 s pour une
mise à jour partielle, 0,66 s pour une mise à jour complète, mesuré sur la
carte), il clignote mal et il s'use avec le nombre de rafraîchissements. Une
minuterie qui décompte demande des mises à jour fréquentes : l'affichage doit
donc être adapté à l'écran. SPEC.md reste indépendant du matériel et ne dit pas
comment ; ce document dit comment, pour cet écran, et définit les valeurs
retenues des constantes que SPEC.md laisse ouvertes (CST-07, CST-08).

Il traite : la précision de l'affichage selon le temps restant, les règles
de rafraîchissement, le démarrage imminent, les alertes à l'écran, les
recommandations du fabricant et nos écarts. Il ne traite ni la logique de la
minuterie (SPEC.md), ni l'électronique (`docs/coreink.md`).

### Comment le lire

- Chaque besoin porte un identifiant **`INK-xx`**, stable : jamais réutilisé ni
  renuméroté, comme dans SPEC.md. On le cite dans les commits, les tests et les
  commentaires du code (« Implémente INK-04 »). Un besoin retiré est barré, pas
  supprimé.
- Un besoin qui précise une exigence de SPEC.md la cite entre parenthèses.
- **État** de chaque besoin : *essayé* (validé en essai sur la carte),
  *implémenté* (dans le code, pas encore validé en essai), *à implémenter* (décidé,
  pas dans le code), *à définir*. La section 11 liste les écarts entre ce
  document et le code actuel.
- Ce document ne modifie pas SPEC.md. Les retouches de SPEC.md qu'il appelle
  sont dans la section 13.
- Ce document est provisoire comme le matériel : si l'e-ink est abandonné
  (CLAUDE.md, « La question ouverte »), il disparaît avec lui.

### Vocabulaire

| Terme | Sens |
|---|---|
| **Valeur précise** | Le temps restant d'une minuterie, arrondi à la seconde supérieure (9,3 s restantes s'affichent `00:10`). C'est la seule valeur arrondie ; les niveaux dizaine et minute ne font que masquer des chiffres de cette valeur (INK-03). |
| **Niveau** | Précis, dizaine ou minute : combien de chiffres de la valeur précise on affiche. |
| **Tiret** | Caractère `-` qui remplace un chiffre masqué. |
| **Rémanence du réglage** | Le temps pendant lequel l'affichage reste précis après un réglage (INK-12). À ne pas confondre avec la rémanence de l'image de l'écran (traces, INK-20). |
| **Révélation** | Le premier cran sur un affichage partiel : il montre le précis sans régler (INK-14). |
| **Partiel / complet** | Mise à jour partielle (sans clignotement, rapide) ou complète (inversion de tous les pixels, clignote, efface les traces). À ne pas confondre avec « niveau partiel » : un affichage en dizaine ou en minute. |

## 1. Niveaux de précision

- **INK-01. Trois niveaux.** Chaque minuterie affiche son temps restant à l'un
  des trois niveaux suivants. Le niveau dépend du temps restant (INK-05), pas du
  réglage ni de l'état de l'autre minuterie.

  | Niveau | Affichage | Exemples |
  |---|---|---|
  | précis | toutes les secondes | `01:43`, `00:12`, `1:32:07` |
  | dizaine | les dizaines de secondes, un tiret | `05:2-`, `12:5-` |
  | minute | les minutes, deux tirets | `32:--`, `59:--`, `1:32:--` |

  Il n'y a pas de niveau supplémentaire : le niveau minute s'écrit déjà
  `h:mm:--` au-delà d'une heure, et chaque niveau ajouté multiplie les
  transitions à documenter et à tester. *État : à implémenter (le code ne
  connaît que précis et minute).*

- **INK-02. Pourquoi.** Plus le temps restant est long, moins on a besoin de
  précision : en fin de cuisson on veut les secondes, en milieu de cuisson une
  précision grossière suffit. Un niveau grossier évite des rafraîchissements
  inutiles (INK-16). Le gain d'énergie propre à l'écran est modeste
  (`docs/coreink.md`, section 6) ; le gain principal vient du nombre de
  réveils du processeur.

- **INK-03. Règle des tirets.** Un tiret remplace un chiffre de la valeur
  précise ; il ne l'arrondit pas. Les chiffres affichés sont ceux de la valeur
  précise du moment, les autres sont masqués :
  - `32:--` : le temps restant est compris entre 32:00 et 32:59 ;
  - `04:1-` : entre 4:10 et 4:19 ;
  - `1:32:--` : entre 1:32:00 et 1:32:59.

  La valeur précise est le temps restant arrondi à la seconde supérieure
  (9,3 s restantes → `00:10`). Les tirets s'appliquent à cette valeur, donc
  `04:1-` ne devient `04:0-` que lorsque la valeur précise passe de `04:10` à
  `04:09`. Une version antérieure arrondissait l'affichage vers le haut ou vers
  le bas : cela dérouta à l'usage et c'est exclu. *État : implémenté pour le
  niveau minute ; à implémenter pour le niveau dizaine.*

- **INK-04. Les minuteries sont indépendantes.** Chaque minuterie suit sa règle
  de niveau selon son propre temps restant. Exemple : globale `01:59` (précis)
  avec rappel `00:5-` (dizaine). Les deux niveaux se combinent librement.
  *État : à implémenter (le code actuel pilote les deux par la globale).*

- **INK-05. Seuils de niveau.** Le niveau se détermine sur la valeur précise s
  (en secondes) : bornes incluses vers le niveau le plus fin.

  | Minuterie | précis | dizaine | minute |
  |---|---|---|---|
  | globale | s ≤ 2 min | 2 min < s ≤ 20 min | s > 20 min |
  | rappel | s ≤ 20 s | 20 s < s ≤ 10 min | s > 10 min |

  Exemple : la globale à 20:00 s'affiche `20:0-`, puis `19:5-` ; à 2:00 elle
  s'affiche `02:00`, donc en précis. Le seuil de 2 min de la globale est
  validé en essai ; les autres valeurs sont à valider.
  Le rappel est une tâche (retourner, arroser) qui n'a pas besoin de la même
  précision que la globale.
  Valeurs retenues pour CST-08 (`CST_FULL_DISPLAY_THRESHOLD_MS`, et deux
  constantes à créer pour les autres seuils). *État : seuil de 2 min implémenté
  pour la globale ; autres seuils à implémenter. Le code actuel passe en
  minute au-delà de 30 min.*

- **INK-06. Les niveaux partiels n'existent qu'en Décompte (ETA-04).** En
  Réglage, en Démarrage imminent et en Terminé, l'affichage est précis. Une
  minuterie sans temps restant à montrer (rappel à l'arrêt, rappel consommé
  par ECH-07, `00:00`) n'a pas de niveau partiel. *État : à implémenter pour
  le rappel.*

## 2. Précision temporaire

- **INK-10. Départ du décompte.** Les 15 premières secondes du décompte sont en
  précis sur les deux minuteries (par exemple 5:00, 4:59, 4:58…), puis chaque
  minuterie reprend son niveau (`CST_FULL_DISPLAY_AT_START_MS`). *État :
  essayé.*

- **INK-11. Pourquoi les 15 s.** Après avoir réglé, on s'attend à voir les
  secondes tourner un instant ; un passage immédiat à `05:--` surprend.

- **INK-12. Rémanence du réglage.** Après chaque réglage ou révélation (INK-14),
  les **deux** minuteries restent en précis pendant 5 s
  (`CST_FULL_DISPLAY_AFTER_ADJUST_MS`). Chaque nouvel appui relance les 5 s.
  Quand on règle, on voit la valeur précise : sans cela, certains changements
  du réglage seraient invisibles (un pas de 10 s sur `12:5-`). Les deux
  minuteries suivent la même logique pour que l'écran reste cohérent. *État :
  essayé pour la minuterie réglée ; l'extension aux deux minuteries est à
  implémenter.*

- **INK-13. Retour au niveau.** À la fin du délai, chaque minuterie reprend son
  niveau selon INK-05, sans autre action de l'utilisateur.

## 3. Premier cran : la révélation

- **INK-14. Révélation.** Un cran sur une minuterie dont l'affichage est
  **partiel** (dizaine ou minute) montre le précis sans changer la valeur
  (déclenche INK-12). Un cran sur une minuterie en précis règle tout de suite.
  La molette règle la globale ; le bouton sous la molette règle le rappel :
  c'est le niveau de la minuterie réglée qui décide si le cran est consommé
  par la révélation. *État : essayé en niveau minute ; la règle par minuterie
  est à implémenter.*

- **INK-15. Pendant la rémanence, tout règle.** Puisque les deux minuteries sont
  alors en précis, les crans suivants règlent directement, quelle que soit la
  commande. Exemple : globale `35:--`, rappel `05:2-`. Un cran de molette
  montre `35:12` et `05:23` sans régler ; un cran de bouton dans la foulée
  règle le rappel.

  **Molette tenue.** Les répétitions du même appui, tant qu'il est tenu, ne
  règlent pas non plus après une révélation (essai E18 : `44:02` devenait
  `44:00`). *État : essayé.*

  **Rappel à l'arrêt.** Le premier cran de bouton sur un rappel à l'arrêt
  (INK-06) règle tout de suite, même si la globale est en niveau partiel.
  *État : à implémenter ; le code actuel révèle dans ce cas.*

## 4. Cadence de rafraîchissement

- **INK-16. Mise à jour sur changement.** L'écran n'est mis à jour que lorsque
  le texte à afficher change, ou lors d'un événement (réglage, échéance,
  acquittement, alerte). Une minuterie en niveau minute change au plus une
  fois par minute, en niveau dizaine au plus toutes les 10 s, en niveau précis
  toutes les secondes. Le changement se calcule sur l'échéance, pas sur
  l'horloge murale (INK-03). *État : implémenté pour précis et minute.*

- **INK-17. Prochain réveil.** Le matériel se réveille à la prochaine date de
  changement d'affichage parmi les deux minuteries : changement de la valeur
  affichée, changement de niveau (INK-05), fin de la rémanence du réglage
  (INK-12), échéance, motif d'alerte (GEN-05, ETA-04). Chaque minuterie
  contribue indépendamment : une seule période unique de rafraîchissement
  n'existe pas.

- **INK-18. Mise à jour partielle.** La mise à jour de décompte est partielle
  (sans clignotement). *Point à vérifier à la relecture : seule la zone qui
  change est mise à jour, ou toute l'image est envoyée ?*

- **INK-19. Écart assumé avec la recommandation du fabricant.** Les niveaux de
  l'INK-05 provoquent davantage de mises à jour que ce que recommande le
  fabricant (section 6). L'écran s'usera plus vite ; c'est un coût accepté vu
  l'usage (précision utile en fin de cuisson). La fiche du GDEY0154D67 annonce
  « 1 000 000 de cycles ou 5 ans » (conditions non précisées) ; cette limite
  n'a pas été mesurée sur notre écran et on ne donne pas de durée de vie chiffrée.
  Une mesure (compteur de rafraîchissements en mémoire RTC) reste prévue dans
  les essais. Si l'écran de remplacement (GDEY0154D67) montre des traces, cette
  règle sera réexaminée.

## 5. Rafraîchissement complet et rémanence de l'image

- **INK-20. Complet périodique.** Un rafraîchissement complet est fait au plus
  tard après `CST_DISPLAY_PARTIALS_BEFORE_FULL` mises à jour partielles
  (300 aujourd'hui). Valeur validée en essai : à ce rythme, aucune rémanence
  de l'image n'est apparue. *État : essayé.*

- **INK-21. Complet aux alertes.** Un rafraîchissement complet est fait à
  l'entrée et à la sortie d'une alerte inversée (noir et blanc inversés), pour
  éviter la rémanence vue à l'essai B5
  (`controller_full_refresh`). *État : essayé.*

- **INK-22. Complet toutes les 24 h.** Besoin nouveau. L'écran doit être
  rafraîchi (complet) au moins toutes les 24 h, sinon rémanence
  (« ghosting » ou « image collée », fiche du GDEY0154D67, section 16). Cela
  concerne surtout l'écran de Veille laissé affiché une nuit (ETA-08, MAT-12).
  Comment le garantir (alarme de réveil ? au prochain réveil ? seulement
  quand l'appareil est allumé ?) est à définir. *État : à définir.*

## 6. Recommandations du fabricant et écarts

Sources lues le 26 novembre 2026 :
[guide d'usage Good Display](https://buyepaper.com/pages/epaper-display-usage-guidelines-1)
(mars 2025) et
[fiche du GDEY0154D67](https://evelta.com/content/datasheets/496-GDEY0154D67.pdf)
(rév. 1.2, avril 2024). Elles concernent le GDEY0154D67 (modèle de
remplacement) ; elles valent pour notre GDEW0154M09 (arrêté, fiche non
obtenue) par hypothèse : même fabricant, même génération.

| Recommandation | Source | Notre position |
|---|---|---|
| Un complet après 5 partiels consécutifs, contre l'accumulation de rémanence | guide (« Partial Update ») ; fiche, section 7-2 | **Écart assumé** : 300 partiels (INK-20), validé en essai. |
| Au moins 180 s d'intervalle « après une série de mises à jour » | guide (« Update Intervals ») ; absent de la fiche | **Écart assumé** : en précis on met à jour chaque seconde (INK-19). Contradiction avec l'ancienne valeur de 15 s de `docs/coreink.md` : recherche en cours. |
| Rafraîchir au moins toutes les 24 h | fiche, section 16, point 5 ; le guide la limite aux écrans couleur | À suivre : INK-22. |
| Veille profonde de l'écran entre deux mises à jour espacées | guide (« Deep Sleep Mode ») | Appliqué en pratique entre les mises à jour. |
| 0 à 50 °C en fonctionnement | fiche, section 7-1 | Une cuisine convient, pas un four ouvert. |
| Durée de vie : 1 000 000 de cycles ou 5 ans | fiche, section 8.1 | Non mesurée chez nous (INK-19). |
| Éviter chaleur, humidité, soleil, néons prolongés | fiche, section 16 | Aucune mesure prévue. |

## 7. Démarrage imminent

- **INK-30. Pas de clignotement.** Le clignotement (DEM-02) n'est pas retenu sur
  l'e-ink : trop lent pour se voir (soirée 2), et chaque rafraîchissement
  consomme (`CST_BLINK_PERIOD_MS` à 0).
- **INK-31. Libellé fixe.** Le démarrage imminent est signalé par le libellé
  « AVANT DÉPART » à la place de « TOTAL » pendant le délai de démarrage
  (CST-01). *État : essayé.*
- **INK-32. Autre piste.** Benoît a une autre piste que le clignotement, à
  documenter ici. *État : à définir.* (Une idée voisine, notée le 24/11 :
  3 ou 5 barres en haut d'écran qui apparaissent pendant le délai.)

## 8. Alertes à l'écran

- **INK-40. Pas de LED.** La LED du CoreInk est trop petite pour être vue
  (vérifié sur la carte) : elle n'est pas utilisée (`CST_ALERT_LED` à `false`,
  CMD-07, MAT-05).
- **INK-41. Inversion.** Le signal passe par l'écran, en inversant le noir et le
  blanc :
  - alerte de rappel : seule la zone du rappel (en bas) est inversée ;
  - alerte de fin : tout l'écran est inversé.
- **INK-42. Persistance.** L'image inversée reste affichée sans consommer, ce
  qui convient à une alerte persistante et non intrusive (GEN-04). Le motif est
  fixe (`CST_REMINDER_ALERT_PERIOD_MS` et `CST_END_ALERT_PERIOD_MS` à 0) ; un
  motif clignotant lent est à décider en essai (CST-06).
- **INK-43. À vérifier en essai.** L'inversion est visible de loin et ne
  laisse pas de traces (INK-21).

## 9. Interface réduite (CoreInk, option 3)

*Section déplacée de CLAUDE.md. Le texte est inchangé, sauf l'affichage
réduit, remplacé par les sections 1 à 3.*

Le CoreInk n'a pas les deux encodeurs prévus par CMD-01 et CMD-02. Ses
commandes sont une molette à trois positions (haut, appui, bas), le bouton
marche situé sous la molette (GPIO 27), un bouton au-dessus de l'écran (GPIO 5,
inutilisé : il ne réveille pas de la veille profonde) et un bouton de
réinitialisation (vérifié sur la carte le 23 novembre 2026). L'interface
retenue pour cette étape est « molette pour la globale, intervalles prédéfinis
pour le rappel ».

La disposition physique guide l'écran : la molette est au-dessus du bouton, donc
la minuterie globale s'affiche en haut de l'écran et la minuterie rappel en
dessous. Chaque commande se trouve ainsi à côté de la valeur qu'elle règle.

| Commande CoreInk | Fonction | Remplace |
|---|---|---|
| Molette haut / bas | Augmente / diminue la minuterie globale, à partir de la valeur affichée. | CMD-01 |
| Molette maintenue en haut ou en bas | Répétition dont le pas grandit avec la durée du maintien. | REG-03 (rotation continue) |
| Bouton sous la molette (bouton marche), appui court | Chaque appui passe à l'intervalle de rappel suivant dans une liste prédéfinie, puis revient à « arrêt » après le dernier. | CMD-02 |
| Appui sur la molette | Acquitte l'échéance en attente. | CMD-03 |
| Bouton sous la molette, maintenu 3 s | Extinction de l'appareil ; le même bouton l'allume (circuit d'alimentation). L'appui d'allumage ne compte pas comme un réglage du rappel. | — |

La liste des intervalles de rappel est une table de constantes (GEN-06), par
exemple : arrêt, 1, 2, 3, 5, 10, 15, 20, 30, 45, 60, 90, 120 min. La liste
s'arrête aux intervalles inférieurs à la minuterie globale affichée : avec
2:00, seul 1 min est proposé (soirée 2).

Sur un affichage partiel, le premier cran de molette ou de bouton ne règle rien :
il montre les secondes (INK-14, INK-15).

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
- pas de clignotement pendant le délai de démarrage (DEM-02) : voir INK-30 ;
- les alertes ne reposent pas sur la seule LED : voir INK-40 ;
- pas de capteur « mains sales » dans cette étape (CMD-04 est souhaitable) ; un
  capteur pourra être branché plus tard sur le port Grove.

Cette interface permet de commencer les essais d'utilisation tout de suite, en
une main, avec des doigts sales : en cuisine, le rappel se règle presque
toujours sur quelques valeurs rondes.

## 10. Constantes

Valeurs retenues. Elles vivent dans `components/constants` (GEN-06).

| Constante | Valeur | Besoin | État |
|---|---|---|---|
| `CST_FULL_DISPLAY_THRESHOLD_MS` | 2 min (précis ↔ dizaine, globale) | INK-05, CST-08 | existe ; rôle à redéfinir |
| seuil dizaine ↔ minute, globale | 20 min | INK-05 | à créer (le code a 30 min : `CST_REDUCED_ABOVE_MS`) |
| seuils du rappel (précis, dizaine) | 20 s, 10 min | INK-05 | à créer |
| `CST_FULL_DISPLAY_AT_START_MS` | 15 s | INK-10 | existe |
| `CST_FULL_DISPLAY_AFTER_ADJUST_MS` | 5 s | INK-12 | existe |
| `CST_DISPLAY_PARTIALS_BEFORE_FULL` | 300 | INK-20 | existe |
| `CST_BLINK_PERIOD_MS` | 0 | INK-30 | existe |
| `CST_ALERT_LED` | `false` | INK-40 | existe |
| période du complet de 24 h | 24 h | INK-22 | à créer |

## 11. Écarts avec l'implémentation actuelle

Décidés, non implémentés (aucun développement dans ce fil) :

1. trois niveaux au lieu de deux (INK-01) : le niveau dizaine est nouveau ;
2. un niveau par minuterie, et non un seul indicateur « réduit » piloté par la
   globale (INK-04) ; le rappel suit désormais ses propres seuils (INK-05) ;
3. seuil minute à 20 min au lieu de 30 min (INK-05) ;
4. rémanence du réglage sur les deux minuteries (INK-12) ;
5. révélation décidée par le niveau de la minuterie réglée ; pas de révélation
   sur un rappel à l'arrêt (INK-14, INK-15) ;
6. rafraîchissement complet toutes les 24 h (INK-22).

## 12. Points ouverts et à observer

1. Autre piste pour signaler le démarrage imminent (INK-32).
2. Recherche complémentaire sur l'intervalle minimal entre mises à jour : 180 s
   (guide Good Display) contre 15 s (`docs/coreink.md`).
3. Mécanisme du complet de 24 h (INK-22).
4. Mise à jour partielle limitée à une zone ou à toute l'image (INK-18).
5. À observer en essai : traces après des heures de mises à jour fréquentes
   (INK-19), lisibilité des niveaux dizaine et minute, rendu de l'inversion
   (INK-43).
6. Voir aussi CLAUDE.md, « La question ouverte : e-ink ou Sharp Memory LCD ».

## 13. Retouches de SPEC.md appelées par ce document

Faites avec l'accord de Benoît (26 novembre 2026) :

- **DEM-02** : le signal de démarrage imminent n'est plus forcément un
  clignotement, selon l'écran (INK-30 à INK-32) ;
- **ETA-03** : même retouche, par cohérence ;
- **ETA-04** : les réveils programmés suivent le prochain changement de valeur
  affichée de chacune des deux minuteries (INK-17) ;
- **MAT-04** : trois niveaux, minuteries indépendantes (INK-01 à INK-05) ;
- **CST-08** : seuil aligné sur INK-05. CST-07 reste tel quel : il indique que
  le choix dépend du matériel, la valeur retenue est dans ce document.

À revoir plus tard : **CMD-05** (affichage) et **MAT-03** (écran qui doit
pouvoir clignoter).
