/* Valeurs des tables de constantes (GEN-06). Voir constants.h. */
#include "constants.h"

#define COUNT_OF(t) (sizeof(t) / sizeof((t)[0]))

/*
 * CST-03 : 5 s à partir de 0, 5 min à partir de 2 h (REG-02).
 * Paliers intermédiaires : valeurs de départ, à régler en essai.
 */
const cst_tier_t CST_TOTAL_TIERS[] = {
    {0, 5 * SECOND_MS},
    {1 * MINUTE_MS, 15 * SECOND_MS},
    {5 * MINUTE_MS, 30 * SECOND_MS},
    {20 * MINUTE_MS, 1 * MINUTE_MS},
    {1 * HOUR_MS, 2 * MINUTE_MS},
    {2 * HOUR_MS, 5 * MINUTE_MS},
};
const size_t CST_TOTAL_TIER_COUNT = COUNT_OF(CST_TOTAL_TIERS);

/* CST-04 : ×1 au début du maintien, puis ×2, puis ×6. */
const cst_acceleration_t CST_ACCELERATION[] = {
    {0, 1},
    {6, 2},
    {14, 6},
};
const size_t CST_ACCELERATION_COUNT = COUNT_OF(CST_ACCELERATION);

/* CLAUDE.md : arrêt, 1, 2, 3, 5, 10, 15, 20, 30, 45, 60, 90, 120 min. */
const int64_t CST_REMINDER_INTERVALS_MS[] = {
    0,
    1 * MINUTE_MS,
    2 * MINUTE_MS,
    3 * MINUTE_MS,
    5 * MINUTE_MS,
    10 * MINUTE_MS,
    15 * MINUTE_MS,
    20 * MINUTE_MS,
    30 * MINUTE_MS,
    45 * MINUTE_MS,
    60 * MINUTE_MS,
    90 * MINUTE_MS,
    120 * MINUTE_MS,
};
const size_t CST_REMINDER_INTERVAL_COUNT = COUNT_OF(CST_REMINDER_INTERVALS_MS);

/* CST-05 : rappel, trois bips (0,75 s). */
const cst_note_t CST_REMINDER_RING[] = {
    {2700, 120}, {0, 120}, {2700, 120}, {0, 120}, {2700, 120},
};
const size_t CST_REMINDER_RING_NOTE_COUNT = COUNT_OF(CST_REMINDER_RING);

/* CST-05 : fin, deux groupes de trois bips (1,8 s), pour la distinguer. */
const cst_note_t CST_END_RING[] = {
    {2700, 120}, {0, 80}, {2700, 120}, {0, 80}, {2700, 120}, {0, 400},
    {2700, 120}, {0, 80}, {2700, 120}, {0, 80}, {2700, 120},
};
const size_t CST_END_RING_NOTE_COUNT = COUNT_OF(CST_END_RING);
