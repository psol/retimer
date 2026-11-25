/*
 * Lecture des boutons du CoreInk (docs/coreink.md, section 1).
 *
 * Vérifié sur la carte les 23 et 24 novembre 2026
 * (docs/verifications-carte.md, soirées 1 et 2).
 * Tous les boutons sont actifs à l'état bas, avec tirage externe.
 */
#include "driver/gpio.h"
#include "input.h"
#include "input_hw.h"

#define PIN_UP 37      /* molette haut */
#define PIN_PUSH 38    /* appui sur la molette */
#define PIN_DOWN 39    /* molette bas */
#define PIN_REMINDER 27 /* bouton sous la molette, aussi bouton marche */

void input_hw_init(void)
{
    gpio_config_t c = {
        .pin_bit_mask = (1ULL << PIN_UP) | (1ULL << PIN_PUSH) |
                        (1ULL << PIN_DOWN) | (1ULL << PIN_REMINDER),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE, /* tirage externe */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&c);
}

uint32_t input_hw_read(void)
{
    uint32_t m = 0;
    if (gpio_get_level(PIN_UP) == 0) {
        m |= INPUT_MASK(BUTTON_UP);
    }
    if (gpio_get_level(PIN_PUSH) == 0) {
        m |= INPUT_MASK(BUTTON_PUSH);
    }
    if (gpio_get_level(PIN_DOWN) == 0) {
        m |= INPUT_MASK(BUTTON_DOWN);
    }
    if (gpio_get_level(PIN_REMINDER) == 0) {
        m |= INPUT_MASK(BUTTON_REMINDER);
    }
    return m;
}
