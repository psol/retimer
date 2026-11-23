/*
 * Alimentation et veille de l'ESP32 du CoreInk. Voir power.h et
 * docs/coreink.md, sections 3 et 4.
 *
 * PREMIER JET, ÉCRIT SANS LA CARTE : à vérifier (docs/verifications-carte.md).
 */
#include "power.h"

#include <sys/time.h>

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "input.h"

static const char *TAG = "power";

#define PIN_HOLD 12 /* HOLD : alimentation maintenue tant qu'à 1 */
#define PIN_UP 37
#define PIN_PUSH 38
#define PIN_DOWN 39
#define PIN_REMINDER 27 /* bouton sous la molette, aussi bouton marche */
#define BATTERY_CHANNEL ADC_CHANNEL_7 /* GPIO 35 */

void power_init(void)
{
    /* Après une veille profonde, la broche est figée : on la libère. */
    gpio_hold_dis(PIN_HOLD);
    gpio_set_direction(PIN_HOLD, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_HOLD, 1);
    /* On la fige à 1 pour que la veille profonde ne coupe pas l'alimentation. */
    gpio_hold_en(PIN_HOLD);
    gpio_deep_sleep_hold_en();
}

bool power_woke_from_deep_sleep(void)
{
    return esp_reset_reason() == ESP_RST_DEEPSLEEP;
}

uint32_t power_wake_button(void)
{
    switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT0:
        return INPUT_MASK(BUTTON_UP);
    case ESP_SLEEP_WAKEUP_EXT1:
        if (esp_sleep_get_ext1_wakeup_status() & (1ULL << PIN_REMINDER)) {
            return INPUT_MASK(BUTTON_REMINDER);
        }
        return 0;
    default:
        return 0;
    }
}

int64_t power_now_ms(void)
{
    /*
     * L'heure système de l'ESP32 continue pendant les veilles, comptée par
     * l'horloge lente (8 MHz interne divisé par 256, sdkconfig.defaults :
     * dérive à mesurer, docs/coreink.md, section 4).
     */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void power_sleep(int64_t wakeup_ms, power_mode_t mode)
{
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    if (wakeup_ms != INT64_MAX) {
        int64_t delay = wakeup_ms - power_now_ms();
        if (delay < 1) {
            delay = 1;
        }
        esp_sleep_enable_timer_wakeup((uint64_t)delay * 1000);
    }

    if (mode == POWER_LIGHT_SLEEP) {
        /* Toutes les commandes réveillent, au niveau bas. */
        const gpio_num_t pins[] = {PIN_UP, PIN_PUSH, PIN_DOWN,
                                      PIN_REMINDER};
        for (size_t i = 0; i < sizeof pins / sizeof pins[0]; i++) {
            gpio_wakeup_enable(pins[i], GPIO_INTR_LOW_LEVEL);
        }
        esp_sleep_enable_gpio_wakeup();
        esp_light_sleep_start();
        for (size_t i = 0; i < sizeof pins / sizeof pins[0]; i++) {
            gpio_wakeup_disable(pins[i]);
        }
        return;
    }

    /*
     * Veille profonde : deux commandes seulement (docs/coreink.md,
     * section 4) : molette haut par ext0, bouton du rappel par ext1
     * (ETA-10). La molette bas ne réveille pas : en Veille, la globale est
     * à zéro et ne peut pas descendre.
     */
    esp_sleep_enable_ext0_wakeup(PIN_UP, 0);
    esp_sleep_enable_ext1_wakeup(1ULL << PIN_REMINDER,
                                 ESP_EXT1_WAKEUP_ALL_LOW);
    ESP_LOGI(TAG, "veille profonde");
    esp_deep_sleep_start();
}

uint32_t power_battery_mv(void)
{
    static adc_oneshot_unit_handle_t adc = NULL;
    static adc_cali_handle_t cali = NULL;
    if (adc == NULL) {
        adc_oneshot_unit_init_cfg_t unit = {.unit_id = ADC_UNIT_1};
        if (adc_oneshot_new_unit(&unit, &adc) != ESP_OK) {
            return 0;
        }
        adc_oneshot_chan_cfg_t chan = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        adc_oneshot_config_channel(adc, BATTERY_CHANNEL, &chan);
        adc_cali_line_fitting_config_t c = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        if (adc_cali_create_scheme_line_fitting(&c, &cali) != ESP_OK) {
            cali = NULL;
        }
    }
    int raw = 0;
    if (adc_oneshot_read(adc, BATTERY_CHANNEL, &raw) != ESP_OK) {
        return 0;
    }
    int mv = 0;
    if (cali == NULL || adc_cali_raw_to_voltage(cali, raw, &mv) != ESP_OK) {
        mv = raw * 3300 / 4095; /* approximation sans étalonnage */
    }
    /* Pont diviseur 20 kΩ / 5,1 kΩ (R41, R42). */
    return (uint32_t)((int64_t)mv * 251 / 51);
}

void power_off(void)
{
    ESP_LOGI(TAG, "extinction");
    gpio_hold_dis(PIN_HOLD);
    gpio_set_level(PIN_HOLD, 0);
}
