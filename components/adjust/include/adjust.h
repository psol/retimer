/*
 * Réglage des minuteries : pas selon la valeur de départ et accélération
 * (REG-01 à REG-04, CST-03, CST-04), liste des intervalles de rappel
 * (CLAUDE.md, « Interface réduite »).
 *
 * Fonctions pures : elles calculent une nouvelle valeur à partir de la
 * valeur affichée, sans rien savoir de la minuterie ni du matériel.
 */
#ifndef ADJUST_H
#define ADJUST_H

#include <stdint.h>

typedef enum {
    ADJUST_DOWN = -1,
    ADJUST_UP = 1,
} adjust_direction_t;

/*
 * Pas d'un cran de la minuterie globale, partant de value_ms, dans le sens
 * donné, à la répétition donnée (0 = premier cran d'un appui, puis 1, 2…
 * pendant le maintien). Implémente REG-02 et REG-03.
 */
int64_t adjust_total_step(int64_t value_ms, adjust_direction_t direction,
                          uint32_t repeat);

/*
 * Nouvelle valeur de la minuterie globale après un cran (REG-01 : on part de
 * la valeur affichée). La valeur est alignée sur le pas, bornée à
 * [0, CST_TOTAL_MAX_MS] (ERR-06), et ne prend jamais une valeur entre 0 et
 * CST_TOTAL_MIN_MS exclus (PLG-01).
 */
int64_t adjust_total(int64_t value_ms, adjust_direction_t direction,
                     uint32_t repeat);

/*
 * Intervalle de rappel suivant dans CST_REMINDER_INTERVALS_MS : le premier
 * strictement supérieur à interval_ms, ou 0 (« arrêt ») après le dernier.
 * Si total_ms est positif, la liste s'arrête aux intervalles strictement
 * inférieurs à total_ms : un rappel qui ne sonnerait qu'après la fin ne sert
 * à rien (retour de Benoît, soirée 2). total_ms = 0 : pas de limite.
 */
int64_t adjust_next_reminder(int64_t interval_ms, int64_t total_ms);

#endif /* ADJUST_H */
