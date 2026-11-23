/*
 * Retimer, étape 1 : « bonjour » (docs/PLAN.md, section 3).
 *
 * Maintient l'alimentation du CoreInk et écrit un message par seconde sur la
 * console série, pour vérifier la chaîne de flashage depuis le Mac
 * (idf.py flash monitor). L'assemblage complet viendra à l'étape 4.
 */
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PIN_HOLD 12 /* docs/coreink.md : alimentation maintenue à 1 */

void app_main(void)
{
    gpio_set_direction(PIN_HOLD, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_HOLD, 1);
    for (int i = 0;; i++) {
        ESP_LOGI("retimer", "Bonjour, Retimer ! (%d)", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
