/*
 * Tests du composant rendu : CMD-05, DEM-02, MAT-04, ETA-07, ERR-07 et les
 * alertes à l'écran de CLAUDE.md.
 *
 * Tests d'instantané : chaque écran est écrit dans build/ecrans/ et comparé
 * à test/host/references/. Pour accepter un changement voulu :
 *   RETIMER_UPDATE_REFERENCES=1 ctest --test-dir test/host/build
 * puis python3 tools/screens.py pour régénérer les images PNG de docs/ecrans/.
 */
#include <string.h>

#include "helpers.h"
#include "constants.h"
#include "kitchen_timer.h"
#include "render.h"
#include "unity.h"

#define S(x) ((int64_t)(x) * SECOND_MS)
#define M(x) ((int64_t)(x) * MINUTE_MS)
#define H(x) ((int64_t)(x) * HOUR_MS)
#define T0 ((int64_t)1000000)

static ktimer_t m;
static ktimer_view_t view;
static render_image_t image;
static render_options_t options;

void setUp(void)
{
    ktimer_init(&m);
    memset(&options, 0, sizeof options);
}
void tearDown(void) {}

static void draw(int64_t t)
{
    ktimer_view(&m, t, &view);
    render_draw(&image, &view, &options);
}

static void snapshot(const char *name)
{
    long d = helpers_snapshot(name, image.bytes, RENDER_WIDTH, RENDER_HEIGHT);
    if (d < 0) {
        TEST_FAIL_MESSAGE("Référence absente : lancer avec "
                          "RETIMER_UPDATE_REFERENCES=1");
    }
    TEST_ASSERT_EQUAL_MESSAGE(0, d, "L'écran diffère de sa référence");
}

static int64_t start_countdown(int64_t total, int64_t reminder)
{
    ktimer_set_total(&m, T0, total);
    if (reminder > 0) {
        ktimer_set_reminder(&m, T0, reminder);
    }
    int64_t start_value = T0 + CST_START_DELAY_MS;
    ktimer_advance(&m, start_value);
    return start_value;
}

/* Proportion de points noirs dans une bande de lignes. */
static double darkness(int y0, int y1)
{
    long black_count = 0;
    for (int y = y0; y <= y1; y++) {
        for (int x = 0; x < RENDER_WIDTH; x++) {
            black_count += render_pixel(&image, x, y);
        }
    }
    return (double)black_count / ((double)(y1 - y0 + 1) * RENDER_WIDTH);
}

/* ---- Format des durées -------------------------------------------------- */

static void check_format(int64_t ms, bool reduced, const char *expected)
{
    char t[10];
    render_format_duration(ms, reduced, t);
    TEST_ASSERT_EQUAL_STRING(expected, t);
}

/* CMD-05 : temps restant arrondi à la seconde supérieure. */
static void test_CMD_05_duration_format(void)
{
    check_format(0, false, "00:00");
    check_format(1, false, "00:01");
    check_format(S(45), false, "00:45");
    check_format(M(12) + S(34), false, "12:34");
    check_format(H(1) + M(5), false, "1:05:00");
    check_format(H(24), false, "24:00:00");
}

/* MAT-04 : affichage réduit « 12:-- », minutes arrondies au-dessus. */
static void test_MAT_04_reduced_format(void)
{
    check_format(M(12), true, "12:--");
    check_format(M(11) + 1, true, "12:--");
    check_format(H(2) + M(30), true, "2:30:--");
}

/* ---- Écrans ------------------------------------------------------------- */

static void test_ETA_08_MAT_12_idle_screen(void)
{
    draw(T0);
    snapshot("01_idle");
}

static void test_CMD_05_setting_total_and_reminder(void)
{
    ktimer_set_reminder(&m, T0, M(5));
    ktimer_set_total(&m, T0, M(20));
    draw(T0);
    snapshot("02_setting");
}

/* DEM-02 : phase éteinte du clignotement : les valeurs disparaissent. */
static void test_DEM_02_blink_off_phase(void)
{
    ktimer_set_reminder(&m, T0, M(5));
    ktimer_set_total(&m, T0, M(20));
    draw(T0 + CST_BLINK_PERIOD_MS / 2);
    snapshot("03_starting_off");
    double off = darkness(0, RENDER_HEIGHT - 1);
    draw(T0);
    TEST_ASSERT_TRUE(darkness(0, RENDER_HEIGHT - 1) > off * 2);
}

/* MAT-04 : décompte long, affichage réduit. */
static void test_MAT_04_reduced_countdown(void)
{
    int64_t start_value = start_countdown(M(45), M(10));
    draw(start_value + M(2) + S(20));
    TEST_ASSERT_TRUE(view.reduced);
    snapshot("04_countdown_reduced");
}

/* CMD-05 : affichage complet sur les derniers instants (CST-08). */
static void test_CMD_05_full_countdown(void)
{
    int64_t start_value = start_countdown(M(10), M(3));
    draw(start_value + M(2) + S(15));
    TEST_ASSERT_FALSE(view.reduced);
    snapshot("05_countdown_full");
}

static void test_CMD_05_countdown_over_one_hour(void)
{
    int64_t start_value = start_countdown(H(2) + M(30), M(20));
    draw(start_value + M(14) + S(30));
    snapshot("06_countdown_hours");
}

/* Alerte de rappel : seule la zone du rappel est inversée. */
static void test_ECH_01_reminder_alert_inverted_zone(void)
{
    int64_t start_value = start_countdown(M(30), M(5));
    ktimer_advance(&m, start_value + M(5));
    draw(start_value + M(5) + S(20));
    snapshot("07_reminder_alert");
    TEST_ASSERT_TRUE(darkness(RENDER_SEPARATOR_Y + 2, RENDER_HEIGHT - 1) > 0.6);
    TEST_ASSERT_TRUE(darkness(0, RENDER_SEPARATOR_Y - 1) < 0.4);
}

/* ECH-07 : plus de rappel avant la fin. */
static void test_ECH_07_reminder_after_end(void)
{
    int64_t start_value = start_countdown(M(10), M(4));
    ktimer_advance(&m, start_value + M(4));
    ktimer_acknowledge(&m, start_value + M(7));
    draw(start_value + M(7) + S(30));
    snapshot("08_reminder_after_end");
}

/* ETA-07, ECH-02 : alerte de fin, tout l'écran inversé, « 00:00 ». */
static void test_ETA_07_end_alert_inverted_screen(void)
{
    int64_t start_value = start_countdown(M(1), 0);
    ktimer_advance(&m, start_value + M(1));
    draw(start_value + M(1) + S(5));
    snapshot("09_end_alert");
    TEST_ASSERT_TRUE(darkness(0, RENDER_HEIGHT - 1) > 0.6);
}

/* PLG-03 : décompte court sans rappel. */
static void test_PLG_03_short_countdown_without_reminder(void)
{
    int64_t start_value = start_countdown(S(45), 0);
    draw(start_value + S(12) + 300);
    snapshot("10_short_countdown");
}

/* ERR-07 : indicateur de batterie faible. */
static void test_ERR_07_low_battery(void)
{
    int64_t start_value = start_countdown(M(30), M(5));
    options.low_battery = true;
    draw(start_value + M(3));
    snapshot("11_low_battery");
    /* L'indicateur est dans le coin haut droit. */
    bool found = false;
    for (int y = 0; y < 20; y++) {
        for (int x = RENDER_WIDTH - 40; x < RENDER_WIDTH; x++) {
            found = found || render_pixel(&image, x, y);
        }
    }
    TEST_ASSERT_TRUE(found);
}

/* Le plus long texte possible tient dans l'écran (24:00:00). */
static void test_PLG_01_24_h_fit_on_screen(void)
{
    ktimer_set_total(&m, T0, H(24));
    ktimer_set_reminder(&m, T0, H(2));
    draw(T0);
    snapshot("12_setting_maximum");
    /* Rien ne touche les bords, hormis la ligne de séparation. */
    for (int y = 0; y < RENDER_HEIGHT; y++) {
        if (y == RENDER_SEPARATOR_Y || y == RENDER_SEPARATOR_Y + 1) {
            continue;
        }
        TEST_ASSERT_FALSE(render_pixel(&image, 0, y));
        TEST_ASSERT_FALSE(render_pixel(&image, RENDER_WIDTH - 1, y));
    }
}

/* Écran d'extinction (bouton du rappel maintenu, CLAUDE.md). */
static void test_power_off_screen(void)
{
    render_message(&image, "RETIMER", "ÉTEINT");
    snapshot("13_power_off");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_CMD_05_duration_format);
    RUN_TEST(test_MAT_04_reduced_format);
    RUN_TEST(test_ETA_08_MAT_12_idle_screen);
    RUN_TEST(test_CMD_05_setting_total_and_reminder);
    RUN_TEST(test_DEM_02_blink_off_phase);
    RUN_TEST(test_MAT_04_reduced_countdown);
    RUN_TEST(test_CMD_05_full_countdown);
    RUN_TEST(test_CMD_05_countdown_over_one_hour);
    RUN_TEST(test_ECH_01_reminder_alert_inverted_zone);
    RUN_TEST(test_ECH_07_reminder_after_end);
    RUN_TEST(test_ETA_07_end_alert_inverted_screen);
    RUN_TEST(test_PLG_03_short_countdown_without_reminder);
    RUN_TEST(test_ERR_07_low_battery);
    RUN_TEST(test_PLG_01_24_h_fit_on_screen);
    RUN_TEST(test_power_off_screen);
    return UNITY_END();
}
