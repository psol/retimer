# Retimer : spécification des besoins

Version de travail du 22 novembre 2026. Ce document décrit le comportement
attendu de la nouvelle minuterie, indépendamment du matériel qui sera retenu.
Les points marqués **[à valider]** sont des propositions en attente de décision.

## 1. Contexte

Le prototype de 2020 (CircuitPython sur Adafruit PyBadge) a montré l'utilité d'une
minuterie qui rappelle régulièrement de vérifier la cuisson. Il a aussi montré ses
limites :

- pas de mise en veille, donc une autonomie faible ;
- trois compteurs fixes (un « retimer » et deux minuteries) au lieu d'un programme
  de cuisson simple ;
- commande sans les mains par le capteur de luminosité de la carte, peu fiable
  quand la lumière est faible ;
- aucune électronique sur mesure, et un code qui ne tourne plus sur les versions
  actuelles de CircuitPython.

## 2. Principes

- **Tout se fait sur l'appareil.** Pas d'application, pas de téléphone, pas de
  connexion nécessaire.
- **Une seule cuisson à la fois.**
- **Pas de modes.** Chaque commande a une seule fonction, toujours la même. Un
  bouton rotatif sur lequel on appuie pour changer de fonction est plus
  compliqué qu'un bouton séparé.
- **Rappels non intrusifs.** Quand le rappel sonne, on est déjà en train d'agir :
  l'appareil ne doit pas continuer à sonner. Il doit en revanche montrer
  clairement qu'un rappel a été manqué.
- **Autonomie la plus longue possible.** Le microcontrôleur dort entre deux
  événements et n'est réveillé que par une minuterie matérielle ou une action de
  l'utilisateur.

## 3. Programme de cuisson

Un programme de cuisson se compose de :

| Paramètre | Obligatoire | Description |
|---|---|---|
| Temps total | oui | Durée de la cuisson, au bout de laquelle l'alarme de fin se déclenche. |
| Intervalle de rappel | non | Période à laquelle un rappel se déclenche pendant la cuisson (retourner les légumes, arroser la viande, rabattre la pâte). Zéro ou absent : aucun rappel. |

### Plages et pas de réglage

Plages :

- temps total : de 10 s à 24 h (une cuisson de 45 s au micro-ondes doit être
  possible) ;
- intervalle de rappel : de 0 (désactivé) à 2 h ;
- l'intervalle de rappel ne peut pas être supérieur ou égal au temps total ; s'il
  l'est, aucun rappel n'est programmé.

Pas de réglage progressifs : plus la valeur est grande, plus chaque cran du
bouton rotatif avance loin. On ne doit pas tourner des centaines de fois pour
régler 2 h, et la précision à la seconde n'a de sens que pour les cuissons
courtes. Exemple de paliers, valable pour les deux réglages :

| Valeur actuelle | Pas par cran |
|---|---|
| jusqu'à 2 min | 10 s |
| de 2 à 30 min | 1 min |
| de 30 min à 2 h | 5 min |
| au-delà de 2 h | 10 min |

Les seuils et les pas exacts seront fixés par des tests à l'usage.

### Rappels

- Le premier rappel tombe un intervalle après le démarrage de la cuisson.
- Chaque rappel suivant tombe un intervalle après l'**acquittement** du rappel
  précédent, et non à une heure fixe depuis le démarrage.
- Tant qu'un rappel n'est pas acquitté, aucun nouveau rappel n'est programmé :
  l'indicateur visuel continue de signaler le rappel manqué, sans nouveau son.
- Les rappels s'arrêtent dès que le temps restant est inférieur à l'intervalle.

### Modification en cours de cuisson

Le programme reste modifiable pendant la cuisson, sans passer par un autre
écran : en fin de cuisson, on doit souvent allonger ou raccourcir les temps.

- Tourner le bouton « temps total » modifie le **temps restant**, avec les mêmes
  paliers qu'au réglage. Le ramener à zéro annule la cuisson. **[à valider]**
- Tourner le bouton « rappel » modifie l'intervalle ; le prochain rappel est
  recalculé depuis le dernier acquittement (ou le démarrage s'il n'y en a pas eu).
  Si ce moment est déjà passé, le rappel se déclenche immédiatement. **[à valider]**
- Ramener l'intervalle à 0 désactive les rappels pour la suite de la cuisson.

## 4. Entrées

| Entrée | Rôle | Priorité |
|---|---|---|
| Bouton rotatif « temps total » | Régler le temps total. | Essentiel |
| Bouton rotatif « rappel » | Régler l'intervalle de rappel. | Essentiel |
| Bouton séparé « OK » | Démarrer la cuisson, acquitter un rappel ou l'alarme de fin. **[à valider : un seul bouton pour ces deux usages]** | Essentiel |
| Capteur sans contact (geste ou proximité) | Acquitter un rappel les mains sales. Doit fonctionner quelle que soit la lumière ambiante, contrairement au capteur de luminosité du prototype. | Souhaitable |
| Minuterie matérielle (RTC ou équivalent) | Réveiller le microcontrôleur à la prochaine échéance. | Essentiel |
| Mesure de la batterie | Détecter une batterie faible. | Essentiel |

Les boutons rotatifs ne servent qu'à régler ; ils ne s'enfoncent pas. La
disposition physique exacte sera fixée lors de la conception de l'appareil.

Le choix du capteur sans contact dépendra du matériel retenu ; il n'est pas
bloquant pour une première version.

## 5. Sorties

| Sortie | Rôle |
|---|---|
| Écran | Afficher le temps restant, l'intervalle de rappel et le temps avant le prochain rappel. |
| Haut-parleur ou buzzer | Émettre un son court et unique à chaque rappel et à la fin de cuisson. |
| Indicateur visuel | Signaler un rappel ou une fin de cuisson non acquittés, sans son. |

L'alarme de fin suit la même règle non intrusive que les rappels : un son
unique, puis l'indicateur visuel jusqu'à l'acquittement.

Ordre de préférence pour l'indicateur visuel, selon le matériel :

1. une LED qui clignote tant que l'alerte n'est pas acquittée ;
2. à défaut, l'écran qui clignote ;
3. à défaut seulement, le son rejoué à intervalle espacé **[à valider : intervalle]**,
   puisque le son est intrusif.

## 6. États

| État | Description | Consommation |
|---|---|---|
| Veille | Aucune cuisson en cours. Écran éteint ou affichage minimal. | Minimale : seul le réveil par action de l'utilisateur est actif. |
| Réglage | L'utilisateur tourne les boutons pour composer le programme. | Microcontrôleur éveillé. |
| Décompte | La cuisson est en cours. | Microcontrôleur endormi entre deux réveils programmés. |
| Rappel en attente | Un rappel s'est déclenché et n'a pas été acquitté. Le décompte continue. | Indicateur visuel actif. |
| Fin | Le temps total est écoulé. Un son unique a été émis, l'indicateur visuel reste actif. | Indicateur visuel actif. |

Transitions :

| Depuis | Événement | Vers |
|---|---|---|
| Veille | Rotation d'un bouton ou appui | Réglage |
| Réglage | Appui sur « OK » avec un temps total non nul | Décompte |
| Réglage | Aucune action pendant le délai d'inactivité | Veille |
| Décompte | Échéance d'un rappel | Rappel en attente (son court unique) |
| Rappel en attente | Acquittement | Décompte (prochain rappel dans un intervalle) |
| Décompte ou Rappel en attente | Rotation d'un bouton | Même état, programme modifié |
| Décompte ou Rappel en attente | Temps total écoulé | Fin (son court unique) |
| Décompte ou Rappel en attente | Temps restant ramené à zéro | Réglage |
| Fin | Acquittement | Veille |

Si un rappel tombe au même moment que la fin de cuisson, seule l'alarme de fin
se déclenche.

## 7. Temporisations

| Temporisation | Valeur | Statut |
|---|---|---|
| Délai d'inactivité en réglage avant la veille | 2 min | [à valider] |
| Durée du son de rappel | 1 à 2 s, une seule fois | [à valider] |
| Fréquence de clignotement de la LED en rappel en attente | flash bref toutes les 2 s | [à valider] |
| Réveil du microcontrôleur en décompte | à la prochaine échéance (rappel, fin) et au rafraîchissement de l'affichage | [à valider] |
| Rafraîchissement de l'affichage en décompte | toutes les minutes, puis chaque seconde dans la dernière minute | [à valider] |
| Calcul des rappels | un intervalle après le démarrage, puis un intervalle après chaque acquittement | validé |
| Pas de réglage des boutons rotatifs | progressifs, de 10 s à 10 min par cran selon la valeur | seuils à fixer par des tests |

## 8. Comportement en cas d'erreur

| Situation | Comportement attendu |
|---|---|
| Démarrage avec un temps total nul | Le démarrage est refusé, l'appareil reste en réglage. |
| Intervalle de rappel supérieur ou égal au temps total | Aucun rappel n'est programmé ; l'affichage le signale. |
| Batterie faible | Un indicateur s'affiche ; une cuisson en cours n'est jamais interrompue par l'appareil tant que la batterie le permet. [à valider : seuil] |
| Coupure d'alimentation pendant une cuisson | La cuisson en cours est perdue. [à valider : faut-il la reprendre au redémarrage ?] |
| Rappel jamais acquitté | Aucun nouveau rappel n'est programmé ; l'indicateur visuel continue jusqu'à la fin de cuisson. |
| Fin de cuisson jamais acquittée | L'indicateur visuel continue. [à valider : limite de durée pour préserver la batterie] |

## 9. Hors périmètre de cette version

- application mobile ou connexion à un téléphone ;
- plusieurs cuissons en parallèle ;
- thermomètre ou autre capteur de cuisson connecté.
