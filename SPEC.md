# Retimer : spécification des besoins

Version de travail du 22 novembre 2026. Ce document décrit le comportement
attendu de la nouvelle minuterie. La minuterie décompte le temps ; elle n'est pas
responsable de la cuisson. La cuisson n'apparaît ici que comme cas d'usage.
Le document est en deux parties :

- la **partie 1** rassemble ce qui est établi ;
- la **partie 2** rassemble ce qui reste à définir : le matériel, et les valeurs
  et comportements à fixer lors des essais d'utilisation.

Les **essais d'utilisation** se font sur une version préliminaire de la
minuterie : on développe une première version, on l'utilise quelques jours en
cuisine, puis on garde ce qui fonctionne et on améliore le reste. Les valeurs
(CST) et les comportements (OUV) de la partie 2 seront fixés à l'issue de ces
essais.

## Identifiants

Chaque exigence porte un identifiant stable, formé d'un préfixe de thème et d'un
numéro : `DEM-01`, `ACQ-03`, etc. Ces identifiants servent à relier la
spécification aux développements : on les cite dans les commits, les pull
requests, les tests et les commentaires du code (par exemple « Implémente
DEM-01 »).

Règles :

- un identifiant n'est **jamais réutilisé ni renuméroté**, même si l'on déplace ou
  réorganise les sections ;
- une nouvelle exigence prend le numéro suivant disponible dans son thème ;
- une exigence abandonnée reste dans le document, barrée, avec la raison ;
- quand un point de la partie 2 est tranché, il devient une exigence de la
  partie 1 avec un nouvel identifiant, et le point d'origine est barré avec un
  renvoi (par exemple « tranché, voir ACQ-09 »).

| Préfixe | Thème |
|---|---|
| GEN | Principes généraux |
| CMD | Commandes et signaux |
| PLG | Plages de réglage |
| REG | Réglage par les encodeurs |
| DEM | Démarrage |
| ECH | Échéances, sonnerie et alertes |
| MOD | Modification en cours de décompte |
| ACQ | Arrêt et acquittement |
| ETA | États et transitions |
| ERR | Comportement en cas d'erreur |
| HP | Hors périmètre |
| MAT | Matériel à choisir (partie 2) |
| CST | Constantes à fixer lors des essais d'utilisation (partie 2) |
| OUV | Comportements à valider lors des essais d'utilisation (partie 2) |

## Contexte

Le prototype de 2020 (CircuitPython sur Adafruit PyBadge) a montré l'utilité d'une
minuterie qui rappelle régulièrement de vérifier la cuisson. Il a aussi montré ses
limites :

- pas de mise en veille, donc une autonomie faible ;
- trois compteurs fixes au lieu d'une minuterie globale et d'une minuterie rappel ;
- commande sans les mains par le capteur de luminosité de la carte, peu fiable
  quand la lumière est faible ;
- aucune électronique sur mesure, et un code qui ne tourne plus sur les versions
  actuelles de CircuitPython.

## Vocabulaire

| Terme | Définition |
|---|---|
| **Minuterie** | L'ensemble formé par la minuterie globale et la minuterie rappel. C'est aussi le nom de l'appareil. |
| **Minuterie globale** | Décompte le temps total. |
| **Décompte** | Période pendant laquelle la minuterie globale, et éventuellement la minuterie rappel, comptent le temps. |
| **Minuterie rappel** | Se déclenche à intervalle régulier pendant le décompte pour signaler qu'il faut agir : retourner les légumes, arroser la viande, rabattre la pâte. Facultative. (Le « retimer » du prototype.) |
| **Échéance** | Moment où la minuterie globale ou la minuterie rappel arrive à zéro. |
| **Sonnerie** | Signal sonore émis une seule fois à chaque échéance. |
| **Alerte** | Signal visuel, non intrusif, qui persiste après la sonnerie tant que l'échéance n'a pas été acquittée. Elle permet de voir qu'on a manqué une sonnerie. Il y a deux alertes distinctes : l'**alerte de rappel** et l'**alerte de fin**. |
| **Acquitter** | Indiquer à la minuterie qu'on a pris connaissance d'une échéance. L'acquittement éteint l'alerte. |
| **Minuterie matérielle** | Périphérique du microcontrôleur qui le réveille pendant qu'il est éteint (GEN-05). À ne pas confondre avec la minuterie globale ou la minuterie rappel. |

---

# Partie 1 : ce qui est établi

## 1. Principes

- **GEN-01. Tout se fait sur l'appareil.** Pas d'application, pas de téléphone,
  pas de connexion nécessaire.
- **GEN-02. Un seul décompte à la fois.**
- **GEN-03. Pas de modes.** Chaque commande a une seule fonction, toujours la
  même. Pas de bouton de démarrage : la minuterie démarre d'elle-même quand on
  cesse de la régler. Un bouton sert uniquement à acquitter.
- **GEN-04. Non intrusif.** À une échéance, on est déjà en train d'agir : la
  minuterie ne continue pas à sonner. Elle montre en revanche clairement qu'une
  échéance n'a pas été acquittée.
- **GEN-05. Autonomie la plus longue possible.** Le microcontrôleur est éteint
  entre deux événements. Il n'est réveillé que par une minuterie matérielle, dont
  la période dépend de l'état, ou par une action de l'utilisateur.
- **GEN-06. Réglages ajustables.** Toutes les valeurs de réglage (délais, pas,
  seuils, motifs de sonnerie et d'alerte) sont regroupées dans des tables de
  constantes, pour pouvoir les faire évoluer rapidement lors des essais.
- **GEN-07. Développement sur Mac Apple Silicon sans Rosetta.** Toute la chaîne
  de développement du matériel retenu (compilateur, outil de flashage, débogueur)
  doit fonctionner nativement sur un Mac à processeur Apple. C'est un critère
  éliminatoire dans le choix du matériel.

## 2. Commandes et signaux

| Id | Élément | Rôle |
|---|---|---|
| CMD-01 | Encodeur rotatif « minuterie globale » | Régler et modifier la minuterie globale. |
| CMD-02 | Encodeur rotatif « minuterie rappel » | Régler et modifier la minuterie rappel. |
| CMD-03 | Bouton d'acquittement | Acquitter l'échéance en cours. Obligatoire. |
| CMD-04 | Capteur « mains sales » | Acquitter sans contact. Souhaitable, selon le matériel (voir ACQ-06 à ACQ-08 et MAT-06). |
| CMD-05 | Affichage | Montrer le temps restant de la minuterie globale et de la minuterie rappel, et signaler l'imminence du démarrage. Il porte aussi le retour visuel du capteur « mains sales » (ACQ-07) et l'indicateur de batterie faible (ERR-07). Un affichage réduit est permis pour les longs décomptes (voir MAT-04). |
| CMD-06 | Émetteur sonore | Émettre la sonnerie : plusieurs bips brefs sur 1 à 2 s, et non une tonalité continue. |
| CMD-07 | Indicateurs d'alerte | Signaux visuels persistants jusqu'à l'acquittement ; l'alerte de rappel et l'alerte de fin doivent être distinguables (moyen à définir, voir MAT-05). |

## 3. Plages de réglage

| Id | Minuterie | Minimum | Maximum |
|---|---|---|---|
| PLG-01 | Globale | 5 s (la valeur 0 reste atteignable : elle arrête le décompte, voir ACQ-01) | 24 h |
| PLG-02 | Rappel | 0 (désactivée) | 2 h |

- **PLG-03.** Un décompte court, par exemple 45 s pour le micro-ondes, doit être
  possible, sans minuterie rappel.

## 4. Réglage par les encodeurs

- **REG-01.** On tourne toujours **à partir de la valeur affichée**, c'est-à-dire
  du temps restant. Exemple : pour un décompte de 30 min dont 10 min sont passées,
  l'affichage indique 20 min, et tourner l'encodeur part de 20 min, pas de 30.
  Même principe pour la minuterie rappel.
- **REG-02.** Le pas d'un cran dépend de **la valeur de départ** : partant de 0,
  on avance de 5 s ; partant de 2 h, on avance directement de 5 min.
- **REG-03.** Le pas d'un cran dépend aussi de **la rotation continue** : plus on
  tourne longtemps, plus le pas grandit. Par exemple 5 s, puis 10 s, puis 30 s ;
  ou 5 min, puis 10 min.
- **REG-04.** Les seuils et les pas sont définis dans des tables de constantes
  (voir CST-03 et CST-04).

## 5. Démarrage

- **DEM-01.** Quand l'utilisateur cesse de tourner les encodeurs et que la
  minuterie globale n'est pas nulle, la minuterie attend un **délai de
  démarrage** (voir CST-01).
- **DEM-02.** Pendant ce délai, l'affichage clignote pour indiquer que le
  décompte va démarrer.
- **DEM-03.** Toute rotation pendant ce délai le recommence.
- **DEM-04.** À la fin du délai, le décompte démarre.

## 6. Échéances, sonnerie et alertes

- **ECH-01.** À l'échéance de la minuterie rappel, une sonnerie est émise une
  fois, puis l'alerte de rappel reste active jusqu'à l'acquittement.
- **ECH-02.** À l'échéance de la minuterie globale, même règle non intrusive :
  une sonnerie, puis l'alerte de fin jusqu'à l'acquittement.
- **ECH-03.** La fin du décompte est un **événement distinct** : elle déclenche
  sa sonnerie et l'alerte de fin même si une alerte de rappel attend d'être
  acquittée.
- **ECH-04.** À l'échéance de la minuterie globale, la minuterie rappel s'arrête
  et une éventuelle alerte de rappel s'éteint : quand la cuisson est finie, il
  n'est plus nécessaire de retourner les légumes. Seule l'alerte de fin reste
  active.
- **ECH-05.** La minuterie rappel **repart de l'acquittement** : la prochaine
  échéance tombe un intervalle après l'acquittement, pas à une heure fixe depuis
  le démarrage.
- **ECH-06.** Tant qu'une échéance de la minuterie rappel n'est pas acquittée, la
  minuterie rappel ne redémarre pas ; aucune nouvelle sonnerie n'est émise.
- **ECH-07.** Si la prochaine échéance de la minuterie rappel tombe après celle
  de la minuterie globale, il n'y a plus d'échéance de rappel ; la minuterie
  globale sonnera la fin.
- **ECH-08.** Si les deux échéances coïncident, seule l'échéance de la minuterie
  globale sonne.

## 7. Modification en cours de décompte

Les minuteries restent modifiables pendant le décompte : en fin de cuisson, on
doit souvent allonger ou raccourcir les temps.

- **MOD-01.** L'encodeur « minuterie globale » modifie le temps restant.
- **MOD-02.** L'encodeur « minuterie rappel » modifie le temps restant avant la
  prochaine échéance de rappel, et l'intervalle utilisé ensuite. Le comportement
  exact est à fixer lors des essais (voir OUV-02).
- **MOD-03.** Le décompte continue pendant qu'on tourne.
- **MOD-04.** Après ACQ-02, on peut réactiver la minuterie rappel en la
  remontant pendant le même décompte ; c'est un nouveau réglage. A priori, à
  confirmer lors des essais (OUV-08).

## 8. Arrêt et acquittement

Par les encodeurs :

- **ACQ-01.** **Ramener la minuterie globale à zéro** arrête le décompte et
  acquitte tout.
- **ACQ-02.** **Ramener la minuterie rappel à zéro** acquitte l'échéance de rappel
  en cours et désactive la minuterie rappel pour la suite du décompte.
- **ACQ-03.** Toute autre modification d'une minuterie dont l'échéance n'est pas
  acquittée vaut probablement acquittement (par exemple, ramener la minuterie
  rappel de 5 à 2 min). À confirmer lors des essais (voir OUV-01).

Par le bouton d'acquittement :

- **ACQ-04.** L'appareil a **au moins un bouton d'acquittement**. Un appui
  acquitte l'échéance en attente (rappel ou fin) et éteint l'alerte
  correspondante. Les deux alertes ne sont jamais actives en même temps
  (ECH-04).
- **ACQ-05.** Un appui sans échéance en attente n'a aucun effet.

Par le capteur « mains sales », s'il est présent :

- **ACQ-06.** Selon le matériel, un capteur « mains sales » permet aussi
  d'acquitter sans contact. Il ne remplace pas le bouton.
- **ACQ-07.** Le capteur **guide l'utilisateur à l'écran**, comme un curseur sur
  un écran tactile : dès que le début d'un geste reconnu est détecté, l'écran
  l'indique, puis le retour visuel progresse avec le geste jusqu'à
  l'acquittement. Exemple, si le geste retenu est un mouvement de la main de haut
  en bas : quand la main est repérée, l'écran affiche une petite main avec une
  flèche, et la flèche grandit à mesure que la main avance.
- **ACQ-08.** Un geste commencé puis abandonné n'acquitte rien ; le retour visuel
  disparaît.
- **ACQ-09.** Le capteur « mains sales » n'est actif que lorsqu'une alerte est en
  attente. Le reste du temps, il est éteint : cela économise les piles et évite
  les faux gestes.

## 9. États

| Id | État | Description | Microcontrôleur |
|---|---|---|---|
| ETA-01 | Veille | Aucun décompte en cours, aucune alerte. | Éteint ; réveillé par une rotation d'encodeur. |
| ETA-02 | Réglage | Aucun décompte en cours ; l'utilisateur tourne les encodeurs. | Éveillé. |
| ETA-03 | Démarrage imminent | Délai de démarrage en cours, l'affichage clignote. | Éveillé. |
| ETA-04 | Décompte | Les minuteries décomptent. L'alerte de rappel peut être active. | Éteint entre deux réveils programmés (échéance, rafraîchissement de l'affichage, alerte) ; réveillé aussi par une action de l'utilisateur (encodeur, bouton, capteur). |
| ETA-05 | Terminé | La minuterie globale est arrivée à zéro. Seule l'alerte de fin est active (ECH-04). | Réveillé selon le besoin des alertes, et par une action de l'utilisateur (encodeur, bouton, capteur). |

- **ETA-06.** L'alerte de rappel n'est pas un état à part : c'est un indicateur
  qui ne peut être actif qu'en Décompte.
- **ETA-07.** En Terminé, l'affichage indique « 00:00 ». L'alerte de fin est
  portée par les indicateurs d'alerte (CMD-07).
- **ETA-08.** En Veille, ce que montre l'affichage dépend de la technologie
  d'écran retenue (voir MAT-12).

Transitions :

| Id | Depuis | Événement | Vers |
|---|---|---|---|
| ETA-10 | Veille | Rotation d'un encodeur | Réglage |
| ETA-11 | Réglage | Aucune rotation, minuterie globale non nulle | Démarrage imminent |
| ETA-12 | Réglage | Aucune rotation pendant le délai d'inactivité, minuterie globale nulle | Veille |
| ETA-13 | Démarrage imminent | Rotation d'un encodeur | Réglage |
| ETA-14 | Démarrage imminent | Fin du délai de démarrage | Décompte |
| ETA-15 | Décompte | Échéance de la minuterie rappel | Décompte (sonnerie, alerte de rappel active) |
| ETA-16 | Décompte | Acquittement de l'alerte de rappel | Décompte (minuterie rappel relancée, sauf après ACQ-02 ou dans le cas d'ECH-07) |
| ETA-17 | Décompte | Échéance de la minuterie globale | Terminé (sonnerie, alerte de fin active) |
| ETA-18 | Décompte | Minuterie globale ramenée à zéro | Veille |
| ETA-19 | Terminé | Acquittement de l'alerte de fin | Veille |
| ETA-20 | Terminé | Rotation d'un encodeur (acquitte l'alerte de fin et commence un nouveau réglage ; a priori, à confirmer : OUV-07) | Réglage |
| ETA-21 | Terminé | Aucun acquittement pendant le délai d'abandon de l'alerte de fin (CST-10) | Veille (alerte de fin éteinte) |

## 10. Comportement en cas d'erreur

| Id | Situation | Comportement attendu |
|---|---|---|
| ERR-01 | Minuterie globale nulle à la fin du réglage | Le décompte ne démarre pas ; retour en veille après le délai d'inactivité. |
| ERR-02 | Échéance de rappel jamais acquittée | Pas de nouvelle sonnerie de rappel ; l'alerte de rappel continue jusqu'à son acquittement ou jusqu'à la fin du décompte (ECH-04). |
| ERR-03 | Fin du décompte jamais acquittée | L'alerte de fin continue pendant le délai d'abandon (CST-10), puis l'alerte de fin s'éteint et la minuterie passe en Veille, pour préserver les piles. |
| ERR-04 | Appui sur le bouton d'acquittement sans échéance en attente | Aucun effet. |
| ERR-05 | Geste incomplet ou mal reconnu | Aucun acquittement ; le retour visuel disparaît. |
| ERR-06 | Rotation au-delà des plages | La valeur reste bloquée au minimum ou au maximum. |
| ERR-07 | Batterie faible | Un indicateur s'affiche ; la minuterie n'interrompt jamais un décompte en cours tant que la batterie le permet (seuil : voir CST-09). |
| ERR-08 | Coupure d'alimentation pendant un décompte | Aucune information n'est conservée : au retour du courant, la minuterie redémarre comme à l'allumage de l'appareil. |

## 11. Hors périmètre de cette version

- **HP-01.** Application mobile ou connexion à un téléphone.
- **HP-02.** Plusieurs décomptes en parallèle.
- **HP-03.** Thermomètre ou autre capteur de cuisson connecté.

---

# Partie 2 : ce qui reste à définir

## 12. Matériel

| Id | Sujet | Options et contraintes |
|---|---|---|
| MAT-01 | Microcontrôleur et langage | Doit pouvoir s'éteindre en ne laissant qu'une minuterie matérielle active, qui le réveille (GEN-05). Python n'est pas le meilleur choix pour cela. |
| MAT-02 | Chaîne de développement | Compilateur, outil de flashage et débogueur natifs sur Mac Apple Silicon, sans Rosetta (GEN-07). Certains outils de flashage en avaient encore besoin lors des derniers essais : à vérifier pour chaque option. |
| MAT-03 | Affichage | Type d'écran à choisir ; il doit pouvoir clignoter (démarrage imminent, éventuellement alertes). |
| MAT-04 | Affichage réduit | Flexibilité ouverte, pas un besoin : si l'analyse montre que cela économise l'énergie sur le matériel retenu, un long décompte peut s'afficher sans les secondes (par exemple « 12:-- ») pour éviter de rafraîchir l'écran chaque seconde. L'affichage complet (« 00:45 ») ne revient que sur les derniers instants. Les deux minuteries étant affichées ensemble, la plus proche de son échéance peut imposer le rythme de rafraîchissement. |
| MAT-05 | Moyen des alertes | LED ou écran, selon le matériel retenu, avec un signal distinct pour l'alerte de rappel et l'alerte de fin. En dernier recours seulement, la sonnerie rejouée à intervalle espacé, puisqu'elle est intrusive. |
| MAT-06 | Capteur « mains sales » | Type à choisir en regardant les capteurs disponibles : un capteur de gestes semble le plus plausible ; l'audio est risqué mais pas exclu ; d'autres idées sont bienvenues. Il doit fonctionner quelle que soit la lumière, contrairement au capteur de luminosité du prototype, et permettre le retour visuel progressif à l'écran (ACQ-07). Souhaitable, non essentiel. |
| MAT-07 | Bouton d'acquittement | Modèle, emplacement, facilité d'appui avec les mains sales. |
| MAT-08 | Encodeurs | Modèle, nombre de crans par tour, disposition physique. |
| MAT-09 | Émetteur sonore | Haut-parleur ou buzzer, volume. |
| MAT-10 | Alimentation | De préférence des piles du commerce plutôt qu'une batterie rechargeable (souhaitable, non obligatoire). Type et nombre de piles, mesure du niveau de charge. |
| MAT-11 | Boîtier | Emplacement dans la cuisine, résistance à la chaleur et aux éclaboussures. |
| MAT-12 | Affichage en Veille | Dépend de la consommation de l'écran : un écran e-ink peut garder un message « pause » sans consommer ; une autre technologie devra sans doute s'éteindre après un temps sans activité. |

## 13. À fixer lors des essais d'utilisation

Valeurs à placer dans les tables de constantes (GEN-06) :

| Id | Constante | Valeur de départ |
|---|---|---|
| CST-01 | Délai de démarrage | 3 s |
| CST-02 | Délai d'inactivité en réglage avant la veille | 2 min |
| CST-03 | Pas initial selon la valeur de départ | 5 s à partir de 0 ; 5 min à partir de 2 h ; paliers intermédiaires à définir |
| CST-04 | Accélération selon la rotation continue | par exemple 5 s, 10 s, 30 s ; 5 min, 10 min |
| CST-05 | Motif de la sonnerie | plusieurs bips brefs sur 1 à 2 s |
| CST-06 | Motifs des alertes | un motif pour le rappel, un autre pour la fin ; période à définir |
| CST-07 | Rafraîchissement de l'affichage en décompte | à définir selon l'écran et la consommation |
| CST-08 | Seuil de passage à l'affichage complet (si affichage réduit) | par exemple la dernière minute |
| CST-09 | Seuil de batterie faible | à définir selon l'alimentation retenue |
| CST-10 | Délai d'abandon de l'alerte de fin non acquittée | 1 h |

Comportements à trancher :

| Id | Question |
|---|---|
Seules les questions à valider pendant les essais restent ici. Les questions de
conception sont tranchées et passent en partie 1.

| OUV-01 | Modifier une minuterie dont l'échéance n'est pas acquittée vaut-il acquittement ? A priori oui (ACQ-03). |
| OUV-02 | Comment la modification de la minuterie rappel en cours de décompte se répartit-elle entre le temps restant avant la prochaine échéance et l'intervalle des échéances suivantes (MOD-02) ? |
| OUV-03 | Quel geste retient-on pour le capteur « mains sales », et avec quels seuils de reconnaissance (distance, vitesse, durée) ? |
| ~~OUV-04~~ | ~~Faut-il limiter la durée de l'alerte de fin non acquittée ?~~ Tranché, voir ERR-03, ETA-21 et CST-10. |
| ~~OUV-05~~ | ~~Faut-il reprendre un décompte interrompu par une coupure d'alimentation ?~~ Tranché, voir ERR-08. |
| ~~OUV-06~~ | ~~Que montre l'affichage en Veille et en Terminé ?~~ Tranché, voir ETA-07, ETA-08 et MAT-12. |
| OUV-07 | En Terminé, tourner un encodeur acquitte-t-il et commence-t-il un nouveau réglage (ETA-20) ? A priori oui. |
| OUV-08 | Après ACQ-02, peut-on réactiver la minuterie rappel pendant le même décompte (MOD-04) ? A priori oui. |
| ~~OUV-09~~ | ~~Le capteur « mains sales » est-il actif hors alerte ?~~ Tranché, voir ACQ-09. |
