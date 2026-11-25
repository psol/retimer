/*
 * Retimer sur M5Stack CoreInk : assemblage des composants (CLAUDE.md,
 * « Architecture logicielle »).
 *
 * Boucle : lire l'heure et les boutons, traduire en événements, faire avancer
 * la minuterie, sonner, dessiner, afficher, puis dormir jusqu'au prochain
 * réveil (GEN-05).
 *
 * Vérifié sur la carte les 23 et 24 novembre 2026
 * (docs/verifications-carte.md, soirées 1 et 2).
 */
#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "controller.h"
#include "eink_display.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "input.h"
#include "input_hw.h"
#include "kitchen_timer.h"
#include "power.h"
#include "render.h"
#include "signals.h"

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

/*
 * Saisie : une tâche lit les boutons toutes les INPUT_PERIOD_MS, y compris
 * pendant les mises à jour de l'écran (environ 330 ms, bloquantes), et met
 * les événements en file. Sans elle, un cran de molette donné pendant une
 * mise à jour était perdu : en Démarrage imminent, l'écran clignote et le
 * délai n'était pas relancé (DEM-03, essai du 23 novembre 2026).
 */
#define INPUT_PERIOD_MS 5
static input_t input;
static SemaphoreHandle_t input_lock;
static QueueHandle_t input_queue;
static controller_t controller;
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

/* Applique un événement de saisie à la minuterie (composant controller). */
static uint32_t apply(const input_event_t *ev, int64_t now)
{
    controller_result_t r = controller_apply(&controller, &rtc.timer, ev, now);
    switch (r.action) {
    case CONTROLLER_REVEALED:
        ESP_LOGI(TAG, "affichage complet, sans réglage");
        break;
    case CONTROLLER_ADJUSTED:
        ESP_LOGI(TAG, "%s %lld -> %lld ms",
                 ev->type == EV_NEXT_REMINDER ? "rappel" : "globale",
                 (long long)r.before_ms, (long long)r.after_ms);
        break;
    case CONTROLLER_ACKNOWLEDGED:
        ESP_LOGI(TAG, "acquitter");
        break;
    case CONTROLLER_POWER_OFF:
        turn_off();
        break;
    case CONTROLLER_NONE:
        break;
    }
    return r.signals;
}

static void ring(uint32_t signals)
{
    size_t count;
    const cst_note_t *notes = controller_ring(signals, &count);
    if (notes != NULL) {
        ESP_LOGI(TAG, "sonnerie de %s",
                 notes == CST_END_RING ? "fin" : "rappel");
        signals_ring(notes, count);
    }
}

static void measure_battery(int64_t now)
{
    if (now - last_battery_measure < BATTERY_PERIOD_MS) {
        return;
    }
    last_battery_measure = now;
    uint32_t mv = power_battery_mv();
    low_battery = controller_low_battery(mv); /* ERR-07 */
    ESP_LOGI(TAG, "batterie %lu mV", (unsigned long)mv);
}

static void show_screen(int64_t now)
{
    ktimer_view_t view;
    ktimer_view(&rtc.timer, now, &view);
    render_options_t options = {.low_battery = low_battery};
    render_draw(&image, &view, &options);
    /* Complet à l'entrée et à la sortie d'une alerte inversée (B5). */
    eink_show(image.bytes, controller_full_refresh(&controller, &view));
    rtc.displayed = image;

    /* CST-06 : éclat de la LED en complément d'une alerte. */
    if (view.led_flash) {
        signals_led(true);
        vTaskDelay(pdMS_TO_TICKS(CST_ALERT_LED_FLASH_MS));
        signals_led(false);
    }
}

static void input_task(void *arg)
{
    (void)arg;
    for (;;) {
        input_event_t evs[8];
        xSemaphoreTake(input_lock, portMAX_DELAY);
        size_t n = input_process(&input, power_now_ms(), input_hw_read(),
                                 evs, 8);
        xSemaphoreGive(input_lock);
        for (size_t i = 0; i < n; i++) {
            xQueueSend(input_queue, &evs[i], 0);
        }
        vTaskDelay(pdMS_TO_TICKS(INPUT_PERIOD_MS));
    }
}

/* Un bouton tenu, ou des événements pas encore traités. */
static bool input_pending(void)
{
    xSemaphoreTake(input_lock, portMAX_DELAY);
    bool busy = input_busy(&input);
    xSemaphoreGive(input_lock);
    return busy || uxQueueMessagesWaiting(input_queue) > 0;
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

    controller_init(&controller);
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

    input_lock = xSemaphoreCreateMutex();
    input_queue = xQueueCreate(16, sizeof(input_event_t));
    /* Priorité au-dessus de la boucle principale, qui attend l'écran. */
    xTaskCreate(input_task, "saisie", 3072, NULL, 5, NULL);

    for (;;) {
        int64_t now = power_now_ms();
        uint32_t signals = SIGNAL_NONE;

        while (n < 8 && xQueueReceive(input_queue, &events[n], 0)) {
            n++;
        }
        for (size_t i = 0; i < n; i++) {
            signals |= apply(&events[i], now);
        }
        n = 0;
        signals |= ktimer_advance(&rtc.timer, now);

        measure_battery(now);
        show_screen(now);
        ring(signals);

        /* Tant qu'un bouton est tenu, on reste éveillé et on scrute. */
        if (input_pending()) {
            vTaskDelay(pdMS_TO_TICKS(POLL_PERIOD_MS));
            continue;
        }

        now = power_now_ms();
        xSemaphoreTake(input_lock, portMAX_DELAY);
        int64_t input_wakeup = input_next_wakeup(&input);
        xSemaphoreGive(input_lock);
        int64_t wakeup = controller_next_wakeup(&rtc.timer, now, input_wakeup);

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
