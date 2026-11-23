/* Réglage des minuteries. Voir adjust.h. */
#include "adjust.h"

#include "constants.h"

/* REG-02 : pas selon la valeur de départ (CST-03). */
static int64_t step_for_value(int64_t value_ms)
{
    int64_t step_size = CST_TOTAL_TIERS[0].step_ms;
    for (size_t i = 0; i < CST_TOTAL_TIER_COUNT; i++) {
        if (value_ms >= CST_TOTAL_TIERS[i].threshold_ms) {
            step_size = CST_TOTAL_TIERS[i].step_ms;
        }
    }
    return step_size;
}

/* REG-03 : multiplicateur selon la durée du maintien (CST-04). */
static int64_t multiplier(uint32_t repeat)
{
    uint32_t mult = 1;
    for (size_t i = 0; i < CST_ACCELERATION_COUNT; i++) {
        if (repeat >= CST_ACCELERATION[i].from_repeat) {
            mult = CST_ACCELERATION[i].multiplier;
        }
    }
    return (int64_t)mult;
}

int64_t adjust_total_step(int64_t value_ms, adjust_direction_t direction,
                          uint32_t repeat)
{
    /*
     * En descendant, on prend le pas du palier situé sous la valeur : de
     * 1:00, on descend à 0:55 et non à 0:45.
     */
    int64_t start_value = (direction == ADJUST_DOWN) ? value_ms - 1 : value_ms;
    if (start_value < 0) {
        start_value = 0;
    }
    return step_for_value(start_value) * multiplier(repeat);
}

int64_t adjust_total(int64_t value_ms, adjust_direction_t direction,
                     uint32_t repeat)
{
    if (value_ms < 0) {
        value_ms = 0;
    }
    /*
     * REG-01 : on part de la valeur affichée, arrondie à la seconde au-dessus
     * comme à l'écran. Sans cela, en cours de décompte, 29,85 s remontait à
     * 30 s au lieu de passer au cran suivant (soirée 1, C3).
     */
    value_ms = (value_ms + SECOND_MS - 1) / SECOND_MS * SECOND_MS;
    int64_t step_size = adjust_total_step(value_ms, direction, repeat);
    int64_t updated;
    if (direction == ADJUST_UP) {
        /* Multiple du pas strictement supérieur (REG-01 : depuis l'affiché). */
        updated = (value_ms / step_size + 1) * step_size;
    } else {
        /* Multiple du pas strictement inférieur. */
        updated = ((value_ms + step_size - 1) / step_size - 1) * step_size;
    }

    /* ERR-06 : bornes. */
    if (updated > CST_TOTAL_MAX_MS) {
        updated = CST_TOTAL_MAX_MS;
    }
    if (updated < 0) {
        updated = 0;
    }
    /* PLG-01 : entre 0 et le minimum, on va au minimum ou à 0. */
    if (updated > 0 && updated < CST_TOTAL_MIN_MS) {
        updated = (direction == ADJUST_UP) ? CST_TOTAL_MIN_MS : 0;
    }
    return updated;
}

int64_t adjust_next_reminder(int64_t interval_ms, int64_t total_ms)
{
    for (size_t i = 0; i < CST_REMINDER_INTERVAL_COUNT; i++) {
        int64_t candidate = CST_REMINDER_INTERVALS_MS[i];
        if (total_ms > 0 && candidate >= total_ms) {
            break;
        }
        if (candidate > interval_ms) {
            return candidate;
        }
    }
    return 0; /* après le dernier : arrêt (ACQ-02) */
}
