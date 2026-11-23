/*
 * Diagnostic du CoreInk : vérifie chaque pilote sur la carte, étape par
 * étape, avec des messages sur la console série (idf.py monitor).
 * Suivre docs/verifications-carte.md en parallèle.
 *
 * Étapes, chacune lancée par un appui sur la molette (GPIO 38) :
 *   1. alimentation et batterie
 *   2. écran : image complète, puis 20 mises à jour partielles
 *   3. buzzer : bips à plusieurs fréquences
 *   4. LED : niveau bas, puis niveau haut
 *   5. boutons : affiche chaque changement de niveau pendant 30 s
 *   6. veille légère : réveil par minuterie ou par n'importe quel bouton
 *   7. veille profonde : réveil par molette haut, bouton sous la molette ou
 *      après 30 s
 *
 * L'écran affiche l'étape en attente d'un appui.
 */
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "constants.h"
#include "driver/gpio.h"
#include "eink_display.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "input_hw.h"
#include "kitchen_timer.h"
#include "power.h"
#include "render.h"
#include "signals.h"

static const char *TAG = "diag";

/* Compte les veilles profondes, et l'heure avant la dernière. */
static RTC_DATA_ATTR uint32_t deep_sleep_count;
static RTC_DATA_ATTR int64_t before_sleep_ms;

static render_image_t image;

/* Affiche l'étape à l'écran et attend un appui sur la molette. */
static void wait_for_press(int number, const char *name, const char *label)
{
    char title[16];
    snprintf(title, sizeof title, "ÉTAPE %d", number);
    render_message(&image, title, label);
    eink_show(image.bytes, false);
    ESP_LOGI(TAG, "---- %d. %s : appuyer sur la molette pour lancer ----",
             number, name);
    while (gpio_get_level(38) == 1) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    while (gpio_get_level(38) == 0) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* Dessine un écran de réglage avec une valeur donnée. */
static void draw_value(int64_t total_ms, int64_t reminder_ms)
{
    ktimer_view_t view;
    memset(&view, 0, sizeof view);
    view.state = STATE_SETTING;
    view.total_ms = total_ms;
    view.reminder = reminder_ms > 0 ? REMINDER_SET : REMINDER_OFF;
    view.reminder_ms = reminder_ms;
    render_options_t options = {0};
    render_draw(&image, &view, &options);
}

static void step_power(void)
{
    ESP_LOGI(TAG, "cause du démarrage : %d (1 = allumage, 5 = veille profonde)",
             (int)esp_reset_reason());
    ESP_LOGI(TAG, "cause du réveil : %d (2 = ext0, 3 = ext1, 4 = minuterie)",
             (int)esp_sleep_get_wakeup_cause());
    for (int i = 0; i < 3; i++) {
        ESP_LOGI(TAG, "batterie : %lu mV (attendu 3300 à 4200 sur batterie)",
                 (unsigned long)power_battery_mv());
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void step_display(void)
{
    draw_value(12 * MINUTE_MS + 34 * SECOND_MS, 5 * MINUTE_MS);
    ESP_LOGI(TAG, "écran : rafraîchissement complet, attendu « 12:34 » et "
                  "« RAPPEL 05:00 »");
    eink_show(image.bytes, true);
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "écran : 20 mises à jour partielles, une par seconde");
    for (int i = 0; i < 20; i++) {
        int64_t start = esp_timer_get_time();
        draw_value((20 - i) * SECOND_MS, 5 * MINUTE_MS);
        eink_show(image.bytes, false);
        int64_t duration = (esp_timer_get_time() - start) / 1000;
        ESP_LOGI(TAG, "  partielle %d : %lld ms", i + 1, (long long)duration);
        if (duration < 1000) {
            vTaskDelay(pdMS_TO_TICKS(1000 - duration));
        }
    }
    ESP_LOGI(TAG, "écran : alerte de fin (écran inversé)");
    ktimer_view_t view;
    memset(&view, 0, sizeof view);
    view.state = STATE_FINISHED;
    view.end_alert = true;
    view.alert_inverted = true;
    render_draw(&image, &view, NULL);
    eink_show(image.bytes, false);
}

static void step_buzzer(void)
{
    const uint16_t frequencies[] = {1000, 2000, 2700, 3200, 4000};
    for (size_t i = 0; i < sizeof frequencies / sizeof frequencies[0]; i++) {
        ESP_LOGI(TAG, "buzzer : %u Hz", frequencies[i]);
        cst_note_t note = {frequencies[i], 400};
        signals_ring(&note, 1);
        vTaskDelay(pdMS_TO_TICKS(600));
    }
    ESP_LOGI(TAG, "buzzer : sonnerie de rappel (CST-05)");
    signals_ring(CST_REMINDER_RING, CST_REMINDER_RING_NOTE_COUNT);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "buzzer : sonnerie de fin (CST-05)");
    signals_ring(CST_END_RING, CST_END_RING_NOTE_COUNT);
}

static void step_led(void)
{
    gpio_set_direction(10, GPIO_MODE_OUTPUT);
    ESP_LOGI(TAG, "LED : GPIO 10 à 0 pendant 3 s. Allumée ?");
    gpio_set_level(10, 0);
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "LED : GPIO 10 à 1 pendant 3 s. Allumée ?");
    gpio_set_level(10, 1);
    vTaskDelay(pdMS_TO_TICKS(3000));
    signals_led(false);
}

static void step_buttons(void)
{
    const int pins[] = {37, 38, 39, 5, 27};
    const char *names[] = {"molette haut", "appui molette", "molette bas",
                          "bouton au-dessus de l'écran",
                          "bouton sous la molette (marche, rappel)"};
    int before[5];
    for (int i = 0; i < 5; i++) {
        gpio_set_direction(pins[i], GPIO_MODE_INPUT);
        before[i] = gpio_get_level(pins[i]);
        ESP_LOGI(TAG, "GPIO %d (%s) au repos : %d", pins[i], names[i],
                 before[i]);
    }
    ESP_LOGI(TAG, "boutons : appuyer sur chaque commande, pendant 30 s");
    int64_t end = esp_timer_get_time() + 30 * 1000 * 1000;
    while (esp_timer_get_time() < end) {
        for (int i = 0; i < 5; i++) {
            int n = gpio_get_level(pins[i]);
            if (n != before[i]) {
                ESP_LOGI(TAG, "  GPIO %d (%s) : %d", pins[i], names[i], n);
                before[i] = n;
            }
        }
        uint32_t m = input_hw_read();
        (void)m;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void step_light_sleep(void)
{
    ESP_LOGI(TAG, "veille légère : 10 s, ou appui sur n'importe quel bouton");
    vTaskDelay(pdMS_TO_TICKS(500)); /* laisser la console se vider */
    int64_t before = power_now_ms();
    power_sleep(before + 10 * 1000, POWER_LIGHT_SLEEP);
    ESP_LOGI(TAG, "réveil après %lld ms, cause %d (4 = minuterie, 7 = GPIO)",
             (long long)(power_now_ms() - before),
             (int)esp_sleep_get_wakeup_cause());
}

static void step_deep_sleep(void)
{
    ESP_LOGI(TAG, "veille profonde n° %lu : 30 s, ou molette haut, ou bouton "
                  "sous la molette. "
                  "Faire l'essai sur batterie, câble USB débranché, puis "
                  "rebrancher pour lire la suite.",
             (unsigned long)deep_sleep_count + 1);
    draw_value(30 * SECOND_MS, 0);
    eink_show(image.bytes, false);
    eink_sleep();
    vTaskDelay(pdMS_TO_TICKS(500));
    deep_sleep_count++;
    before_sleep_ms = power_now_ms();
    power_sleep(before_sleep_ms + 30 * 1000, POWER_DEEP_SLEEP);
}

void app_main(void)
{
    power_init();
    signals_init();
    input_hw_init();

    if (power_woke_from_deep_sleep()) {
        ESP_LOGI(TAG, "==== réveil de veille profonde n° %lu après %lld ms, "
                      "bouton %lu (1 = haut, 8 = sous la molette, 0 = minuterie) ====",
                 (unsigned long)deep_sleep_count,
                 (long long)(power_now_ms() - before_sleep_ms),
                 (unsigned long)power_wake_button());
    } else {
        deep_sleep_count = 0;
        ESP_LOGI(TAG, "==== diagnostic Retimer ====");
    }
    eink_init(NULL);

    wait_for_press(1, "alimentation", "ALIMENTATION");
    step_power();
    wait_for_press(2, "écran", "ÉCRAN");
    step_display();
    wait_for_press(3, "buzzer", "BUZZER");
    step_buzzer();
    wait_for_press(4, "LED", "LED");
    step_led();
    wait_for_press(5, "boutons", "BOUTONS");
    step_buttons();
    wait_for_press(6, "veille légère", "VEILLE LÉGÈRE");
    step_light_sleep();
    wait_for_press(7, "veille profonde", "VEILLE PROFONDE");
    step_deep_sleep();
}
