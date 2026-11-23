/*
 * Sonnerie sur le buzzer (GPIO 2, MLI) et LED (GPIO 10) du CoreInk.
 * Voir signals.h et docs/coreink.md, sections 1 et 5.
 *
 * PREMIER JET, ÉCRIT SANS LA CARTE : à vérifier (docs/verifications-carte.md).
 */
#include "signals.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define PIN_BUZZER 2
#define PIN_LED 10

#define PWM_MODE LEDC_LOW_SPEED_MODE
#define PWM_TIMER LEDC_TIMER_0
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_RESOLUTION LEDC_TIMER_10_BIT
#define PWM_HALF ((1u << 10) / 2) /* rapport cyclique de 50 % */

void signals_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = PWM_MODE,
        .timer_num = PWM_TIMER,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = 2700,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);
    ledc_channel_config_t chan = {
        .gpio_num = PIN_BUZZER,
        .speed_mode = PWM_MODE,
        .channel = PWM_CHANNEL,
        .timer_sel = PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&chan);

    gpio_config_t led = {
        .pin_bit_mask = 1ULL << PIN_LED,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&led);
    signals_led(false);
}

void signals_ring(const cst_note_t *notes, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (notes[i].frequency_hz > 0) {
            ledc_set_freq(PWM_MODE, PWM_TIMER, notes[i].frequency_hz);
            ledc_set_duty(PWM_MODE, PWM_CHANNEL, PWM_HALF);
        } else {
            ledc_set_duty(PWM_MODE, PWM_CHANNEL, 0);
        }
        ledc_update_duty(PWM_MODE, PWM_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(notes[i].duration_ms));
    }
    ledc_set_duty(PWM_MODE, PWM_CHANNEL, 0);
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}

void signals_led(bool on)
{
#if CONFIG_RETIMER_LED_ACTIVE_LOW
    gpio_set_level(PIN_LED, on ? 0 : 1);
#else
    gpio_set_level(PIN_LED, on ? 1 : 0);
#endif
}
