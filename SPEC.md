# Retimer : spécification des besoins

Version de travail du 22 novembre 2026. Ce document décrit le comportement
attendu de la nouvelle minuterie. Il est en deux parties :

- la **partie 1** rassemble ce qui est établi ;
- la **partie 2** rassemble ce qui reste à définir : le matériel, et les valeurs
  et comportements à fixer lors des essais de manipulation.

## Contexte

Le prototype de 2020 (CircuitPython sur Adafruit PyBadge) a montré l'utilité d'une
minuterie qui rappelle régulièrement de vérifier la cuisson. Il a aussi montré ses
limites :

- pas de mise en veille, donc une autonomie faible ;
- trois compteurs fixes au lieu d'un programme de cuisson simple ;
- commande sans les mains par le capteur de luminosité de la carte, peu fiable
  quand la lumière est faible ;
- aucune électronique sur mesure, et un code qui ne tourne plus sur les versions
  actuelles de CircuitPython.

## Vocabulaire

| Terme | Définition |
|---|---|
| **Minuterie** | L'ensemble formé par la minuterie globale et la minuterie rappel. C'est aussi le nom de l'appareil. |
| **Minuterie globale** | Compte le temps total de la cuisson. |
| **Minuterie rappel** | Se déclenche à intervalle régulier pendant la cuisson pour signaler qu'il faut agir : retourner les légumes, arroser la viande, rabattre la pâte. Facultative. (Le « retimer » du prototype.) |
| **Échéance** | Moment où la minuterie globale ou la minuterie rappel arrive à zéro. |
| **Sonnerie** | Signal sonore émis une seule fois à chaque échéance. |
| **Alerte** | Signal visuel, non intrusif, qui persiste après la sonnerie tant que l'échéance n'a pas été acquittée. Il permet de voir qu'on a manqué une sonnerie. |
| **Acquitter** | Indiquer à la minuterie qu'on a pris connaissance d'une échéance. L'acquittement éteint l'alerte. |

---

# Partie 1 : ce qui est établi

## 1. Principes

- **Tout se fait sur l'appareil.** Pas d'application, pas de téléphone, pas de
  connexion nécessaire.
- **Une seule cuisson à la fois.**
- **Pas de modes.** Chaque commande a une seule fonction, toujours la même. Pas de
  bouton de démarrage : la minuterie démarre d'elle-même quand on cesse de la
  régler. Un bouton sert uniquement à acquitter.
- **Non intrusif.** À une échéance, on est déjà en train d'agir : la minuterie ne
  continue pas à sonner. Elle montre en revanche clairement qu'une échéance n'a
  pas été acquittée.
- **Autonomie la plus longue possible.** Le microcontrôleur est éteint entre deux
  événements. Il n'est réveillé que par une minuterie matérielle, dont la période
  dépend de l'état, ou par une action de l'utilisateur.
- **Réglages ajustables.** Toutes les valeurs de réglage (délais, pas, seuils,
  motifs de sonnerie et d'alerte) sont regroupées dans des tables de constantes,
  pour pouvoir les faire évoluer rapidement lors des essais.
- **Développement sur Mac Apple Silicon sans Rosetta.** Toute la chaîne de
  développement du matériel retenu (compilateur, outil de flashage, débogueur)
  doit fonctionner nativement sur un Mac à processeur Apple. C'est un critère
  éliminatoire dans le choix du matériel.

## 2. Commandes et signaux

| Élément | Rôle |
|---|---|
| Encodeur rotatif « minuterie globale » | Régler et modifier la minuterie globale. |
| Encodeur rotatif « minuterie rappel » | Régler et modifier la minuterie rappel. |
| Bouton d'acquittement | Acquitter l'échéance en cours. Obligatoire. |
| Capteur « mains sales » | Acquitter sans contact. Souhaitable, selon le matériel (voir « Acquittement » en section 8 et partie 2). |
| Affichage | Montrer le temps restant de la minuterie globale et de la minuterie rappel, et signaler l'imminence du démarrage. |
| Sonnerie | Plusieurs bips brefs sur 1 à 2 s, et non une tonalité continue. |
| Alerte | Signal visuel persistant jusqu'à l'acquittement (moyen à définir, voir partie 2). |

## 3. Plages de réglage

| Minuterie | Minimum | Maximum |
|---|---|---|
| Globale | 5 s | 24 h |
| Rappel | 0 (désactivée) | 2 h |

Une cuisson courte, par exemple 45 s au micro-ondes, doit être possible, sans
minuterie rappel.

## 4. Réglage par les encodeurs

- On tourne toujours **à partir de la valeur affichée**, c'est-à-dire du temps
  restant. Exemple : pour une cuisson de 30 min dont 10 min sont passées,
  l'affichage indique 20 min, et tourner l'encodeur part de 20 min, pas de 30.
  Même principe pour la minuterie rappel.
- Le **pas** d'un cran dépend de deux paramètres :
  1. **la valeur de départ** : partant de 0, on avance de 5 s ; partant de 2 h,
     on avance directement de 5 min ;
  2. **la rotation continue** : plus on tourne longtemps, plus le pas grandit.
     Par exemple 5 s, puis 10 s, puis 30 s ; ou 5 min, puis 10 min.
- Les seuils et les pas sont définis dans des tables de constantes (voir
  partie 2).

## 5. Démarrage

- Quand l'utilisateur cesse de tourner les encodeurs et que la minuterie globale
  n'est pas nulle, la minuterie attend un **délai de démarrage** (3 s, à valider).
- Pendant ce délai, l'affichage clignote pour indiquer que la cuisson va
  démarrer.
- Toute rotation pendant ce délai le recommence.
- À la fin du délai, la cuisson démarre : les deux minuteries décomptent.

## 6. Échéances, sonnerie et alerte

- **Échéance de la minuterie rappel :** une sonnerie est émise une fois, puis
  l'alerte reste active jusqu'à l'acquittement.
- **Échéance de la minuterie globale :** même règle non intrusive, une sonnerie
  puis l'alerte jusqu'à l'acquittement.
- La minuterie rappel **repart de l'acquittement** : la prochaine échéance tombe
  un intervalle après l'acquittement, pas à une heure fixe depuis le démarrage.
- Tant qu'une échéance de la minuterie rappel n'est pas acquittée, la minuterie
  rappel ne redémarre pas ; aucune nouvelle sonnerie n'est émise.
- Si l'intervalle de la minuterie rappel dépasse le temps restant de la minuterie
  globale, il n'y a plus d'échéance de rappel ; la minuterie globale sonnera la
  fin.
- Si les deux échéances coïncident, seule celle de la minuterie globale compte.

## 7. Modification en cours de cuisson

Le programme reste modifiable pendant la cuisson : en fin de cuisson, on doit
souvent allonger ou raccourcir les temps.

- L'encodeur « minuterie globale » modifie le temps restant de la cuisson.
- L'encodeur « minuterie rappel » modifie le temps restant avant la prochaine
  échéance de rappel, et l'intervalle utilisé ensuite. **[comportement exact à
  fixer lors des essais, voir partie 2]**
- Le décompte continue pendant qu'on tourne.

## 8. Arrêt et acquittement par les encodeurs

- **Ramener la minuterie globale à zéro** arrête la cuisson et acquitte tout.
- **Ramener la minuterie rappel à zéro** acquitte l'échéance de rappel en cours et
  désactive la minuterie rappel pour la suite de la cuisson.
- Toute autre modification d'une minuterie dont l'échéance n'est pas acquittée
  vaut probablement acquittement (par exemple, ramener la minuterie rappel de 5 à
  2 min). **[à confirmer lors des essais]**

### Acquittement

- L'appareil a **au moins un bouton d'acquittement**. Un appui acquitte
  l'échéance en cours (rappel ou fin de cuisson) et éteint l'alerte.
- Un appui sans échéance en attente n'a aucun effet.
- Selon le matériel, un **capteur « mains sales »** permet aussi d'acquitter sans
  contact. Il ne remplace pas le bouton.
- Le capteur doit **guider l'utilisateur à l'écran**, comme un curseur sur un
  écran tactile :
  - dès que le début d'un geste reconnu est détecté, l'écran l'indique ;
  - le retour visuel progresse avec le geste, jusqu'à l'acquittement.
  - Exemple, si le geste retenu est un mouvement de la main de haut en bas :
    quand la main est repérée, l'écran affiche une petite main avec une flèche,
    et la flèche grandit à mesure que la main avance.
- Un geste commencé puis abandonné n'acquitte rien ; le retour visuel disparaît.

## 9. États

| État | Description | Microcontrôleur |
|---|---|---|
| Veille | Aucune cuisson en cours. | Éteint ; réveillé par une rotation d'encodeur. |
| Réglage | L'utilisateur tourne les encodeurs. | Éveillé. |
| Démarrage imminent | Délai de démarrage en cours, l'affichage clignote. | Éveillé. |
| Cuisson | Les minuteries décomptent. | Éteint entre deux réveils programmés (échéance ou rafraîchissement de l'affichage). |
| Cuisson avec alerte | Une échéance de rappel n'a pas été acquittée. La minuterie globale continue. | Réveillé selon le besoin de l'alerte. |
| Fin | La minuterie globale est arrivée à zéro, l'alerte est active. | Réveillé selon le besoin de l'alerte. |

| Depuis | Événement | Vers |
|---|---|---|
| Veille | Rotation d'un encodeur | Réglage |
| Réglage | Aucune rotation, minuterie globale non nulle | Démarrage imminent |
| Réglage | Aucune rotation pendant le délai d'inactivité, minuterie globale nulle | Veille |
| Démarrage imminent | Rotation d'un encodeur | Réglage |
| Démarrage imminent | Fin du délai de démarrage | Cuisson |
| Cuisson | Échéance de la minuterie rappel | Cuisson avec alerte (sonnerie) |
| Cuisson avec alerte | Acquittement | Cuisson (minuterie rappel relancée) |
| Cuisson ou Cuisson avec alerte | Échéance de la minuterie globale | Fin (sonnerie) |
| Cuisson ou Cuisson avec alerte | Minuterie globale ramenée à zéro | Veille |
| Fin | Acquittement | Veille |

## 10. Comportement en cas d'erreur

| Situation | Comportement attendu |
|---|---|
| Minuterie globale nulle à la fin du réglage | La cuisson ne démarre pas ; retour en veille après le délai d'inactivité. |
| Échéance de rappel jamais acquittée | Pas de nouvelle sonnerie ; l'alerte continue jusqu'à la fin de cuisson. |
| Fin de cuisson jamais acquittée | L'alerte continue. **[limite de durée à définir pour préserver la batterie]** |
| Appui sur le bouton d'acquittement sans échéance en attente | Aucun effet. |
| Geste incomplet ou mal reconnu | Aucun acquittement ; le retour visuel disparaît. |
| Rotation au-delà des plages | La valeur reste bloquée au minimum ou au maximum. |
| Batterie faible | Un indicateur s'affiche ; la minuterie n'interrompt jamais une cuisson en cours tant que la batterie le permet. **[seuil à définir]** |
| Coupure d'alimentation pendant une cuisson | La cuisson en cours est perdue. **[à confirmer : faut-il la reprendre au redémarrage ?]** |

## 11. Hors périmètre de cette version

- application mobile ou connexion à un téléphone ;
- plusieurs cuissons en parallèle ;
- thermomètre ou autre capteur de cuisson connecté.

---

# Partie 2 : ce qui reste à définir

## 12. Matériel

| Sujet | Options et contraintes |
|---|---|
| Microcontrôleur et langage | Doit pouvoir s'éteindre en ne laissant qu'une minuterie matérielle active, qui le réveille. Python n'est pas le meilleur choix pour cela. |
| Chaîne de développement | Compilateur, outil de flashage et débogueur natifs sur Mac Apple Silicon, sans Rosetta. Certains outils de flashage en avaient encore besoin lors des derniers essais : à vérifier pour chaque option. |
| Affichage | Type d'écran à choisir ; il doit pouvoir clignoter (démarrage imminent, éventuellement alerte). |
| Moyen de l'alerte | LED ou écran, selon le matériel retenu. En dernier recours seulement, la sonnerie rejouée à intervalle espacé, puisqu'elle est intrusive. |
| Capteur « mains sales » | Type à choisir en regardant les capteurs disponibles : un capteur de gestes semble le plus plausible ; l'audio est risqué mais pas exclu ; d'autres idées sont bienvenues. Il doit fonctionner quelle que soit la lumière, contrairement au capteur de luminosité du prototype, et permettre le retour visuel progressif à l'écran. Souhaitable, non essentiel. |
| Bouton d'acquittement | Modèle, emplacement, facilité d'appui avec les mains sales. |
| Encodeurs | Modèle, nombre de crans par tour, disposition physique. |
| Sonnerie | Haut-parleur ou buzzer, volume. |
| Alimentation | Batterie, recharge, mesure du niveau de charge. |
| Boîtier | Emplacement dans la cuisine, résistance à la chaleur et aux éclaboussures. |

## 13. À fixer lors des essais de manipulation

Valeurs à placer dans les tables de constantes :

| Constante | Valeur de départ |
|---|---|
| Délai de démarrage | 3 s |
| Délai d'inactivité en réglage avant la veille | 2 min |
| Pas initial selon la valeur de départ | 5 s à partir de 0 ; 5 min à partir de 2 h ; paliers intermédiaires à définir |
| Accélération selon la rotation continue | par exemple 5 s, 10 s, 30 s ; 5 min, 10 min |
| Motif de la sonnerie | plusieurs bips brefs sur 1 à 2 s |
| Motif de l'alerte | clignotement bref, période à définir |
| Rafraîchissement de l'affichage en cuisson | à définir selon l'écran et la consommation |

Comportements à trancher :

1. Modifier une minuterie dont l'échéance n'est pas acquittée vaut-il
   acquittement ? A priori oui.
2. Comment la modification de la minuterie rappel en cours de cuisson se
   répartit-elle entre le temps restant avant la prochaine échéance et
   l'intervalle des échéances suivantes ?
3. Quel geste retient-on pour le capteur « mains sales », et avec quels seuils
   de reconnaissance (distance, vitesse, durée) ?
