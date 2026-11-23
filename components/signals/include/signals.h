/*
 * Signaux sonores et lumineux (CMD-06, CMD-07) : sonnerie sur le buzzer,
 * éclats de la LED en complément des alertes à l'écran (CLAUDE.md, « Alertes
 * à l'écran »).
 */
#ifndef SIGNALS_H
#define SIGNALS_H

#include <stdbool.h>
#include <stddef.h>

#include "constants.h"

void signals_init(void);

/* Joue un motif de sonnerie (CST-05). Bloque pendant sa durée (1 à 2 s). */
void signals_ring(const cst_note_t *notes, size_t count);

/* Allume ou éteint la LED. */
void signals_led(bool on);

#endif /* SIGNALS_H */
