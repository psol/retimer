/*
 * Retimer sur M5Stack CoreInk : assemblage des composants (CLAUDE.md,
 * « Architecture logicielle »).
 *
 * Boucle : lire l'heure et les boutons, traduire en événements, faire avancer
 * la minuterie, sonner, dessiner, afficher, puis dormir jusqu'au prochain
 * réveil (GEN-05).
 *
 * PREMIER JET, ÉCRIT SANS LA CARTE : compilé, pas encore vérifié
 * (docs/verifications-carte.md).
 */
#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "eink_display.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "kitchen_timer.h"
#include "adjust.h"
#include "render.h"
#include "input.h"
#include "input_hw.h"
#include "signals.h"
#include "power.h"

static const char *TAG = "retimer";

/* Pas de boucle pendant qu'un bouton est tenu (anti-rebond, répétition). */
#define POLL_PERIOD_MS 10
/* Sous ce délai, on attend éveillé plutôt que de dormir. */
#define MIN_SLEEP_WAIT_MS 20
/* La batterie se mesure au plus une fois par minute. */
#define BATTERY_PERIOD_MS (60 * 1000)

/*
 * Ce qui survit à la veille profonde (mémoire RTC). ERR-08 : une coupure
 * d'alimentation efface tout, et la signature permet de le savoir.
 */
#define SIGNATURE 0x52544D52u /* « RTMR » */
typedef struct {
    uint32_t signature;
    ktimer_t timer;
    render_image_t displayed;
} rtc_memory_t;

static RTC_DATA_ATTR rtc_memory_t rtc;

static input_t input;
static render_image_t image;
static bool low_battery = false;
static int64_t last_battery_measure = INT64_MIN / 2;

/*
 * Bouton du rappel maintenu (c'est aussi le bouton marche, CLAUDE.md) :
 * message à l'écran, puis extinction. Sur USB, la carte reste alimentée :
 * on repart alors de la Veille, comme après une coupure (ERR-08).
 */
static void turn_off(void)
{
    render_message(&image, "RETIMER", "ÉTEINT");
    eink_show(image.bytes, true);
    eink_sleep();
    power_off();
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "toujours alimentée (USB) : retour en veille");
    ktimer_init(&rtc.timer);
}

/* Applique un événement de saisie à la minuterie. */
static uint32_t apply(const input_event_t *ev, int64_t now)
{
    ktimer_t *m = &rtc.timer;
    ktimer_view_t view;
    ktimer_view(m, now, &view);

    switch (ev->type) {
    case EV_TOTAL_PLUS:
    case EV_TOTAL_MINUS: {
        /* REG-01 : on part de la valeur affichée. */
        adjust_direction_t direction =
            ev->type == EV_TOTAL_PLUS ? ADJUST_UP : ADJUST_DOWN;
        int64_t start_value = view.state == STATE_FINISHED ? 0 : view.total_ms;
        int64_t updated = adjust_total(start_value, direction, ev->repeat);
        ESP_LOGI(TAG, "globale %lld -> %lld ms", (long long)start_value,
                 (long long)updated);
        return ktimer_set_total(m, now, updated);
    }
    case EV_NEXT_REMINDER: {
        int64_t start_value = view.state == STATE_FINISHED ? 0 : m->interval_ms;
        int64_t new_interval = adjust_next_reminder(start_value);
        ESP_LOGI(TAG, "rappel %lld -> %lld ms", (long long)start_value,
                 (long long)new_interval);
        return ktimer_set_reminder(m, now, new_interval);
    }
    case EV_ACKNOWLEDGE:
        ESP_LOGI(TAG, "acquitter");
        return ktimer_acknowledge(m, now);
    case EV_POWER_OFF:
        turn_off();
        return SIGNAL_NONE;
    }
    return SIGNAL_NONE;
}

static void ring(uint32_t signals)
{
    if (signals & SIGNAL_END_RING) {
        ESP_LOGI(TAG, "sonnerie de fin");
        signals_ring(CST_END_RING, CST_END_RING_NOTE_COUNT);
    } else if (signals & SIGNAL_REMINDER_RING) {
        ESP_LOGI(TAG, "sonnerie de rappel");
        signals_ring(CST_REMINDER_RING, CST_REMINDER_RING_NOTE_COUNT);
    }
}

static void measure_battery(int64_t now)
{
    if (now - last_battery_measure < BATTERY_PERIOD_MS) {
        return;
    }
    last_battery_measure = now;
    uint32_t mv = power_battery_mv();
    /* ERR-07. Sur USB, la mesure donne la tension de charge. */
    low_battery = mv > 0 && mv < CST_LOW_BATTERY_MV;
    ESP_LOGI(TAG, "batterie %lu mV", (unsigned long)mv);
}

static void show_screen(int64_t now)
{
    ktimer_view_t view;
    ktimer_view(&rtc.timer, now, &view);
    render_options_t options = {.low_battery = low_battery};
    render_draw(&image, &view, &options);
    /*
     * Rafraîchissement complet à l'entrée et à la sortie d'une alerte
     * inversée : en partiel, l'image précédente reste visible en blanc sur
     * noir (vérification B5 du 23 novembre 2026).
     */
    static bool was_inverted = false;
    bool inverted =
        view.alert_inverted && (view.end_alert || view.reminder_alert);
    eink_show(image.bytes, inverted != was_inverted);
    was_inverted = inverted;
    rtc.displayed = image;

    /* CST-06 : éclat de la LED en complément d'une alerte. */
    if (view.led_flash) {
        signals_led(true);
        vTaskDelay(pdMS_TO_TICKS(CST_ALERT_LED_FLASH_MS));
        signals_led(false);
    }
}

void app_main(void)
{
    power_init(); /* maintenir l'alimentation avant tout */

    bool resumed = power_woke_from_deep_sleep() &&
                   rtc.signature == SIGNATURE;
    if (!resumed) {
        /* ERR-08 : allumage ou coupure, on repart de la Veille. */
        memset(&rtc, 0, sizeof rtc);
        rtc.signature = SIGNATURE;
        ktimer_init(&rtc.timer);
        ESP_LOGI(TAG, "Retimer : allumage");
    } else {
        ESP_LOGI(TAG, "Retimer : réveil de veille profonde");
    }

    signals_init();
    input_hw_init();
    input_init(&input);
    if (!resumed) {
        /* Le bouton marche, qui sert aussi au rappel, est encore tenu. */
        input_ignore_until_release(&input, BUTTON_REMINDER);
    }
    eink_init(resumed ? rtc.displayed.bytes : NULL);

    input_event_t events[8];
    size_t n = 0;
    uint32_t button = resumed ? power_wake_button() : 0;
    for (int b = 0; b < BUTTON_COUNT && n == 0; b++) {
        if (button & INPUT_MASK(b)) {
            n = input_wake_press(&input, (input_button_t)b, events,
                                 8);
        }
    }

    for (;;) {
        int64_t now = power_now_ms();
        uint32_t signals = SIGNAL_NONE;

        n += input_process(&input, now, input_hw_read(),
                           events + n, 8 - n);
        for (size_t i = 0; i < n; i++) {
            signals |= apply(&events[i], now);
        }
        n = 0;
        signals |= ktimer_advance(&rtc.timer, now);

        measure_battery(now);
        show_screen(now);
        ring(signals);

        /* Tant qu'un bouton est tenu, on reste éveillé et on scrute. */
        if (input_busy(&input)) {
            vTaskDelay(pdMS_TO_TICKS(POLL_PERIOD_MS));
            continue;
        }

        now = power_now_ms();
        int64_t wakeup = ktimer_next_wakeup(&rtc.timer, now);
        int64_t r = input_next_wakeup(&input);
        if (r < wakeup) {
            wakeup = r;
        }

        if (rtc.timer.state == STATE_IDLE) {
            /* MAT-12 : l'écran garde le message de veille sans consommer. */
            eink_sleep();
            power_sleep(INT64_MAX, POWER_DEEP_SLEEP); /* ne revient pas */
        }
        if (wakeup - now < MIN_SLEEP_WAIT_MS) {
            int64_t delay = wakeup - now;
            vTaskDelay(pdMS_TO_TICKS(delay > 0 ? delay : 1));
            continue;
        }
        /*
         * Décompte, Terminé, Réglage : veille légère, pour que toutes les
         * commandes réveillent, molette bas et appui compris
         * (docs/coreink.md, section 4).
         */
        power_sleep(wakeup, POWER_LIGHT_SLEEP);
    }
}
