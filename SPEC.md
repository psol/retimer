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

Plages et pas de réglage **[à valider]** :

- temps total : de 1 min à 24 h, par pas de 1 min ;
- intervalle de rappel : de 0 (désactivé) à 2 h, par pas de 1 min ;
- l'intervalle de rappel ne peut pas être supérieur ou égal au temps total ; s'il
  l'est, aucun rappel n'est programmé.

## 4. Entrées

| Entrée | Rôle | Priorité |
|---|---|---|
| Bouton rotatif « temps total » | Régler le temps total. | Essentiel |
| Bouton rotatif « rappel » | Régler l'intervalle de rappel. | Essentiel |
| Appui (bouton poussoir intégré à un rotatif, ou bouton séparé) | Démarrer, acquitter un rappel ou l'alarme de fin, annuler. **[à valider]** | Essentiel |
| Capteur sans contact (geste ou proximité) | Acquitter un rappel les mains sales. Doit fonctionner quelle que soit la lumière ambiante, contrairement au capteur de luminosité du prototype. | Souhaitable |
| Minuterie matérielle (RTC ou équivalent) | Réveiller le microcontrôleur à la prochaine échéance. | Essentiel |
| Mesure de la batterie | Détecter une batterie faible. | Essentiel |

Le choix du capteur sans contact dépendra du matériel retenu ; il n'est pas
bloquant pour une première version.

## 5. Sorties

| Sortie | Rôle |
|---|---|
| Écran | Afficher le temps restant, l'intervalle de rappel et le temps avant le prochain rappel. |
| Haut-parleur ou buzzer | Émettre un son court et unique à chaque rappel, et l'alarme de fin. |
| Indicateur visuel de rappel manqué | Signaler un rappel non acquitté, sans son. |

Ordre de préférence pour l'indicateur de rappel manqué, selon le matériel :

1. une LED qui clignote tant que le rappel n'est pas acquitté ;
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
| Fin | Le temps total est écoulé, l'alarme de fin est active. | Selon l'alarme. |

Transitions :

| Depuis | Événement | Vers |
|---|---|---|
| Veille | Rotation d'un bouton ou appui | Réglage |
| Réglage | Appui « démarrer » avec un temps total non nul | Décompte |
| Réglage | Aucune action pendant le délai d'inactivité | Veille |
| Décompte | Échéance d'un rappel | Rappel en attente (son court unique) |
| Rappel en attente | Acquittement | Décompte |
| Rappel en attente | Échéance du rappel suivant | Rappel en attente (nouveau son court) |
| Décompte ou Rappel en attente | Temps total écoulé | Fin |
| Décompte ou Rappel en attente | Annulation | Réglage |
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
| Calcul des rappels | à intervalle fixe compté depuis le démarrage de la cuisson | [à valider] |

## 8. Comportement en cas d'erreur

| Situation | Comportement attendu |
|---|---|
| Démarrage avec un temps total nul | Le démarrage est refusé, l'appareil reste en réglage. |
| Intervalle de rappel supérieur ou égal au temps total | Aucun rappel n'est programmé ; l'affichage le signale. |
| Batterie faible | Un indicateur s'affiche ; une cuisson en cours n'est jamais interrompue par l'appareil tant que la batterie le permet. [à valider : seuil] |
| Coupure d'alimentation pendant une cuisson | La cuisson en cours est perdue. [à valider : faut-il la reprendre au redémarrage ?] |
| Rappel jamais acquitté | L'indicateur visuel continue jusqu'à la fin de cuisson. [à valider : limite de durée pour préserver la batterie] |

## 9. Hors périmètre de cette version

- application mobile ou connexion à un téléphone ;
- plusieurs cuissons en parallèle ;
- thermomètre ou autre capteur de cuisson connecté.
