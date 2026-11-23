/*
 * Alimentation et mise en veille de l'ESP32 du CoreInk (GEN-05,
 * docs/coreink.md, sections 3 et 4) : maintien de l'alimentation, horloge
 * monotone conservée en veille profonde, veille légère ou profonde jusqu'au
 * prochain réveil, mesure de la batterie.
 */
#ifndef POWER_H
#define POWER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    /* Toutes les commandes réveillent ; RAM conservée. */
    POWER_LIGHT_SLEEP,
    /* Seules la molette haut et bas réveillent ; seule la mémoire RTC reste. */
    POWER_DEEP_SLEEP,
} power_mode_t;

/* Maintient l'alimentation (GPIO 12). À appeler en tout premier. */
void power_init(void);

/* Vrai si la carte sort de veille profonde (et non d'un allumage). */
bool power_woke_from_deep_sleep(void);

/*
 * Masque INPUT_MASK() du bouton qui a réveillé la carte de la veille
 * profonde, ou 0.
 */
uint32_t power_wake_button(void);

/* Horloge monotone en millisecondes, qui continue pendant la veille. */
int64_t power_now_ms(void);

/*
 * Dort jusqu'à wakeup_ms (INT64_MAX : pas de réveil programmé) ou jusqu'à
 * une action de l'utilisateur. En veille profonde, ne rend pas la main : la
 * carte redémarre.
 */
void power_sleep(int64_t wakeup_ms, power_mode_t mode);

/* Tension de la batterie en millivolts (ERR-07). */
uint32_t power_battery_mv(void);

/* Extinction complète (GPIO 12 à 0). Sur USB, la carte reste alimentée. */
void power_off(void);

#endif /* POWER_H */
