/*
 * Pilote de l'écran GDEW0154M09 (contrôleur Fitipower JD79653A) du CoreInk.
 * Voir eink_display.h et docs/coreink.md, section 2.
 *
 * PREMIER JET, ÉCRIT SANS LA CARTE : à vérifier (docs/verifications-carte.md).
 *
 * La séquence d'initialisation, les formes d'onde de mise à jour rapide et
 * l'ordre des commandes viennent de Panel_GDEW0154M09 dans M5GFX
 * (https://github.com/m5stack/M5GFX, licence MIT, LICENSES/M5GFX.txt), qui
 * reprend LovyanGFX (https://github.com/lovyan03/LovyanGFX, licence FreeBSD,
 * LICENSES/LovyanGFX.txt). Le reste du fichier est propre à Retimer.
 */
#include "eink_display.h"

#include <string.h>

#include "constants.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "eink";

/* Brochage (docs/coreink.md, section 1). */
#define PIN_CS 9
#define PIN_DC 15
#define PIN_SCK 18
#define PIN_MOSI 23
#define PIN_RST 0
#define PIN_BUSY 4

#define SPI_FREQUENCY_HZ (10 * 1000 * 1000)
#define BUSY_TIMEOUT_MS 4000

/* Commandes du JD79653A (famille UC8151). */
#define CMD_PANEL_SETTING 0x00
#define CMD_POWER_OFF 0x02
#define CMD_POWER_ON 0x04
#define CMD_DEEP_SLEEP 0x07
#define CMD_OLD_IMAGE 0x10
#define CMD_REFRESH 0x12
#define CMD_NEW_IMAGE 0x13
#define CMD_VCOM_INTERVAL 0x50
#define CMD_WINDOW 0x90
#define CMD_PARTIAL_IN 0x91
#define CMD_PARTIAL_OUT 0x92

/*
 * Liste d'initialisation : commande, nombre d'octets de données, données.
 * Un nombre de 0x80 | n ajoute une attente (octet suivant les données, en
 * ms). 0xFF termine la liste.
 */
#define WAIT_BUSY 0x80
static const uint8_t INIT_SEQUENCE[] = {
    0x00, 2, 0xdf, 0x0e, /* réglage du panneau */
    0x4D, 1, 0x55,       /* réglages internes Fitipower */
    0xaa, 1, 0x0f,
    0xe9, 1, 0x02,
    0xb6, 1, 0x11,
    0xf3, 1, 0x0a,
    0x61, 3, 0xc8, 0x00, 0xc8, /* résolution 200 × 200 */
    0x60, 1, 0x00,             /* TCON */
    0xe3, 1, 0x00,
    0x04, 0,                   /* allumage */
    /* Formes d'onde lues dans les registres. */
    0x00, WAIT_BUSY | 2, 0xff, 0x0e, 160,
    /* Formes d'onde de mise à jour rapide (0x20 VCOM, 0x21 à 0x24). */
    0x20, 56, 0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x21, 42, 0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x22, 56, 0x01, 0x84, 0x84, 0x83, 0x01, 0x01, 0x01,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x23, 56, 0x01, 0x44, 0x44, 0x43, 0x01, 0x01, 0x01,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x24, 56, 0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF,
};

static spi_device_handle_t spi;
static bool bus_ready = false;
static bool controller_ready = false;
/* Image actuellement à l'écran, au format Retimer (1 = noir). */
static uint8_t displayed[EINK_SIZE];
static bool displayed_known = false;
static uint32_t partials = 0;

static void send(bool data, const uint8_t *bytes, size_t n)
{
    if (n == 0) {
        return;
    }
    gpio_set_level(PIN_DC, data ? 1 : 0);
    spi_transaction_t t = {
        .length = n * 8,
        .tx_buffer = bytes,
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));
}

static void command(uint8_t c) { send(false, &c, 1); }

static void data_bytes(const uint8_t *d, size_t n)
{
    /* Le SPI de l'ESP32 envoie au plus 4092 octets par transaction (DMA). */
    while (n > 0) {
        size_t chunk = n > 4000 ? 4000 : n;
        send(true, d, chunk);
        d += chunk;
        n -= chunk;
    }
}

static void data(uint8_t d) { send(true, &d, 1); }

/* BUSY est actif à l'état BAS sur ce contrôleur. */
static bool wait_ready(void)
{
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(PIN_BUSY) == 0) {
        if (esp_timer_get_time() - start > BUSY_TIMEOUT_MS * 1000) {
            ESP_LOGW(TAG, "BUSY toujours actif après %d ms", BUSY_TIMEOUT_MS);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    return true;
}

static void reset(void)
{
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    wait_ready();
}

static void run_sequence(const uint8_t *sequence)
{
    while (*sequence != 0xFF) {
        uint8_t cmd = *sequence++;
        uint8_t n = *sequence++;
        bool with_wait = (n & WAIT_BUSY) != 0;
        n &= (uint8_t)~WAIT_BUSY;
        wait_ready();
        command(cmd);
        data_bytes(sequence, n);
        sequence += n;
        if (with_wait) {
            vTaskDelay(pdMS_TO_TICKS(*sequence++));
        }
    }
}

static void init_bus(void)
{
    if (bus_ready) {
        return;
    }
    gpio_config_t outputs = {
        .pin_bit_mask = (1ULL << PIN_DC) | (1ULL << PIN_RST),
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&outputs));
    gpio_config_t entry = {
        .pin_bit_mask = 1ULL << PIN_BUSY,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&entry));
    gpio_set_level(PIN_RST, 1);

    spi_bus_config_t bus = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus, SPI_DMA_CH_AUTO));
    spi_device_interface_config_t dev = {
        .clock_speed_hz = SPI_FREQUENCY_HZ,
        .mode = 0,
        .spics_io_num = PIN_CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &dev, &spi));
    bus_ready = true;
}

static void init_controller(void)
{
    if (controller_ready) {
        return;
    }
    init_bus();
    reset();
    run_sequence(INIT_SEQUENCE);
    /* Polarité des données : 0xD7 d'après M5GFX (à vérifier). */
    wait_ready();
    command(CMD_VCOM_INTERVAL);
    data(0xD7);
    controller_ready = true;
}

/*
 * Envoie une image au format du contrôleur. Sur ce contrôleur, un bit à 1 est
 * un point blanc : on inverse l'image de Retimer (1 = noir).
 * Le paramètre inverser permet d'envoyer le négatif (rafraîchissement
 * complet).
 */
static void send_image(uint8_t cmd, const uint8_t *image, bool invert)
{
    static uint8_t row[EINK_WIDTH / 8];
    wait_ready();
    command(cmd);
    for (int y = 0; y < EINK_HEIGHT; y++) {
        const uint8_t *src = &image[y * (EINK_WIDTH / 8)];
        for (int i = 0; i < EINK_WIDTH / 8; i++) {
            row[i] = invert ? src[i] : (uint8_t)~src[i];
        }
        data_bytes(row, sizeof row);
    }
}

/* Mise à jour de tout l'écran par la fenêtre partielle. */
static void update(const uint8_t *previous, bool previous_inverted,
                   const uint8_t *updated, bool updated_inverted)
{
    wait_ready();
    command(CMD_PARTIAL_IN);
    command(CMD_WINDOW);
    const uint8_t window[] = {
        0x00, (EINK_WIDTH - 1) | 0x07, /* x de 0 à 199 */
        0x00, 0x00,                    /* y début */
        0x00, EINK_HEIGHT - 1,         /* y fin */
        0x01,                          /* balayage dans la fenêtre */
    };
    data_bytes(window, sizeof window);
    send_image(CMD_OLD_IMAGE, previous, previous_inverted);
    send_image(CMD_NEW_IMAGE, updated, updated_inverted);
    command(CMD_REFRESH);
    vTaskDelay(pdMS_TO_TICKS(10));
    wait_ready();
    command(CMD_PARTIAL_OUT);
}

bool eink_init(const uint8_t *known_image)
{
    init_controller();
    if (known_image != NULL) {
        memcpy(displayed, known_image, EINK_SIZE);
        displayed_known = true;
    } else {
        displayed_known = false;
    }
    ESP_LOGI(TAG, "écran GDEW0154M09 initialisé");
    return true;
}

void eink_show(const uint8_t *image, bool full)
{
    if (displayed_known && !full &&
        memcmp(image, displayed, EINK_SIZE) == 0) {
        return;
    }
    init_controller();
    if (!displayed_known) {
        full = true;
    }
    if (++partials >= CST_DISPLAY_PARTIALS_BEFORE_FULL) {
        full = true;
    }

    int64_t start = esp_timer_get_time();
    if (full) {
        /*
         * Contre les traces : on passe par le négatif de la nouvelle image,
         * puis on affiche l'image (comme le mode « qualité » de M5GFX).
         */
        const uint8_t *before = displayed_known ? displayed : image;
        update(before, false, image, true);
        update(image, true, image, false);
        partials = 0;
    } else {
        update(displayed, false, image, false);
    }
    memcpy(displayed, image, EINK_SIZE);
    displayed_known = true;
    ESP_LOGI(TAG, "mise à jour %s en %lld ms", full ? "complète" : "partielle",
             (long long)((esp_timer_get_time() - start) / 1000));
}

void eink_sleep(void)
{
    if (!controller_ready) {
        return;
    }
    wait_ready();
    command(CMD_VCOM_INTERVAL);
    data(0xF7); /* sans ancienne image, d'après M5GFX */
    command(CMD_POWER_OFF);
    wait_ready();
    command(CMD_DEEP_SLEEP);
    data(0xA5);
    controller_ready = false;
}
