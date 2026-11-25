/*
 * Commande de la minuterie : traduit les événements de saisie en appels à
 * `kitchen_timer` et `adjust`, et prend les petites décisions de `main` qui
 * ne dépendent pas du matériel (rafraîchissement complet de l'écran,
 * sonnerie, batterie faible, prochain réveil).
 *
 * Sorti de `main` pour être testé sur l'hôte (test/host/test_controller.c) :
 * `main` ne garde que le branchement des pilotes.
 */
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "constants.h"
#include "input.h"
#include "kitchen_timer.h"

/* Mémoire de la commande entre deux événements (RAM ordinaire). */
typedef struct {
    /*
     * MAT-04 : un appui en affichage réduit montre les secondes sans régler ;
     * ses répétitions, molette tenue, ne règlent pas non plus.
     */
    bool revealing;
    input_type_t revealed_type;
    /* Dernière image affichée en alerte inversée (rafraîchissement complet). */
    bool was_inverted;
} controller_t;

/* Ce que `main` doit faire après un événement. */
typedef enum {
    CONTROLLER_NONE = 0,
    CONTROLLER_REVEALED,  /* MAT-04 : secondes montrées, rien réglé */
    CONTROLLER_ADJUSTED,  /* globale ou rappel réglé */
    CONTROLLER_ACKNOWLEDGED,
    CONTROLLER_POWER_OFF, /* bouton du rappel maintenu : éteindre */
} controller_action_t;

typedef struct {
    controller_action_t action;
    uint32_t signals;   /* SIGNAL_* rendus par la minuterie */
    int64_t before_ms;  /* valeur réglée avant (CONTROLLER_ADJUSTED) */
    int64_t after_ms;   /* valeur réglée après (CONTROLLER_ADJUSTED) */
} controller_result_t;

void controller_init(controller_t *c);

/* Applique un événement de saisie à la minuterie, à l'instant `now`. */
controller_result_t controller_apply(controller_t *c, ktimer_t *m,
                                     const input_event_t *ev, int64_t now);

/*
 * Vrai si l'image `view` demande un rafraîchissement complet : à l'entrée et
 * à la sortie d'une alerte inversée, sinon l'image précédente reste visible
 * en blanc sur noir (vérification B5 du 23 novembre 2026). Retient l'état
 * pour l'appel suivant.
 */
bool controller_full_refresh(controller_t *c, const ktimer_view_t *view);

/*
 * Sonnerie à jouer pour les signaux reçus (la fin l'emporte sur le rappel),
 * ou NULL. `count` reçoit le nombre de notes.
 */
const cst_note_t *controller_ring(uint32_t signals, size_t *count);

/* ERR-07 : batterie faible. 0 mV veut dire « mesure impossible ». */
bool controller_low_battery(uint32_t battery_mv);

/* Prochain réveil : le plus proche de la minuterie et de la saisie. */
int64_t controller_next_wakeup(const ktimer_t *m, int64_t now,
                               int64_t input_wakeup);

#endif /* CONTROLLER_H */
