/*
 * Tests du composant controller : ce que `main` faisait seul, et qui se teste
 * sans la carte (bogues des soirées 1 et 2, docs/verifications-carte.md).
 */
#include <string.h>

#include "constants.h"
#include "adjust.h"
#include "controller.h"
#include "unity.h"

#define S(x) ((int64_t)(x) * SECOND_MS)
#define M(x) ((int64_t)(x) * MINUTE_MS)

#define T0 ((int64_t)1000000)

static controller_t c;
static ktimer_t m;
static ktimer_view_t view;

void setUp(void)
{
    controller_init(&c);
    ktimer_init(&m);
}
void tearDown(void) {}

static void look(int64_t t) { ktimer_view(&m, t, &view); }

static controller_result_t press(input_type_t type, uint32_t repeat,
                                 int64_t now)
{
    input_event_t ev = {type, repeat};
    return controller_apply(&c, &m, &ev, now);
}

/* Règle la globale, laisse démarrer le décompte, rend l'instant du départ. */
static int64_t start_countdown(int64_t total)
{
    press(EV_TOTAL_PLUS, 0, T0);
    ktimer_set_total(&m, T0, total);
    int64_t start = T0 + CST_START_DELAY_MS;
    ktimer_advance(&m, start);
    TEST_ASSERT_EQUAL(STATE_RUNNING, m.state);
    return start;
}

/* ---- Réglage ------------------------------------------------------------ */

/* REG-01 : la molette part de la valeur affichée (le temps restant). */
static void test_REG_01_adjust_from_displayed_value(void)
{
    int64_t start = start_countdown(M(10));
    int64_t t = start + S(30);
    controller_result_t r = press(EV_TOTAL_PLUS, 0, t);
    TEST_ASSERT_EQUAL(CONTROLLER_ADJUSTED, r.action);
    TEST_ASSERT_EQUAL_INT64(M(9) + S(30), r.before_ms);
    TEST_ASSERT_EQUAL_INT64(adjust_total(M(9) + S(30), ADJUST_UP, 0),
                            r.after_ms);
    look(t);
    TEST_ASSERT_EQUAL_INT64(r.after_ms, view.total_ms);
}

/* ETA-07 : en Terminé, la molette repart de zéro, pas de la valeur finie. */
static void test_ETA_07_adjust_after_end_starts_from_zero(void)
{
    int64_t start = start_countdown(S(30));
    int64_t t = start + S(31);
    ktimer_advance(&m, t);
    TEST_ASSERT_EQUAL(STATE_FINISHED, m.state);
    controller_result_t r = press(EV_TOTAL_PLUS, 0, t);
    TEST_ASSERT_EQUAL_INT64(0, r.before_ms);
    TEST_ASSERT_EQUAL_INT64(CST_TOTAL_MIN_MS, r.after_ms);
}

/* PLG-02 : le bouton du rappel passe à l'intervalle suivant de la liste. */
static void test_PLG_02_next_reminder(void)
{
    press(EV_TOTAL_PLUS, 0, T0);
    ktimer_set_total(&m, T0, M(10));
    controller_result_t r = press(EV_NEXT_REMINDER, 0, T0 + 1);
    TEST_ASSERT_EQUAL(CONTROLLER_ADJUSTED, r.action);
    TEST_ASSERT_EQUAL_INT64(0, r.before_ms);
    TEST_ASSERT_EQUAL_INT64(adjust_next_reminder(0, M(10)), r.after_ms);
    TEST_ASSERT_EQUAL_INT64(r.after_ms, m.interval_ms);
}

/* PLG-02, soirée 2 : avec 2:00, le rappel ne dépasse pas la globale. */
static void test_PLG_02_reminder_capped_by_displayed_total(void)
{
    press(EV_TOTAL_PLUS, 0, T0);
    ktimer_set_total(&m, T0, M(2));
    for (int i = 0; i < 20; i++) {
        controller_result_t r = press(EV_NEXT_REMINDER, 0, T0 + 1 + i);
        TEST_ASSERT_TRUE(r.after_ms < M(2));
    }
}

/* CMD-03 : l'appui sur la molette acquitte ; en Terminé, retour en Veille. */
static void test_ETA_19_acknowledge_end(void)
{
    int64_t start = start_countdown(S(30));
    int64_t t = start + S(31);
    ktimer_advance(&m, t);
    controller_result_t r = press(EV_ACKNOWLEDGE, 0, t + 1);
    TEST_ASSERT_EQUAL(CONTROLLER_ACKNOWLEDGED, r.action);
    TEST_ASSERT_EQUAL(STATE_IDLE, m.state);
}

/* Bouton du rappel maintenu 3 s : main doit éteindre, sans toucher au reste. */
static void test_power_off_request(void)
{
    int64_t start = start_countdown(M(10));
    ktimer_t before = m;
    controller_result_t r = press(EV_POWER_OFF, 0, start + 1);
    TEST_ASSERT_EQUAL(CONTROLLER_POWER_OFF, r.action);
    TEST_ASSERT_EQUAL_MEMORY(&before, &m, sizeof m);
}

/* ---- Affichage réduit (MAT-04) ------------------------------------------ */

/* Soirée 2 : en affichage réduit, le premier cran montre les secondes. */
static void test_MAT_04_first_press_reveals(void)
{
    int64_t start = start_countdown(M(45));
    int64_t t = start + M(1);
    look(t);
    TEST_ASSERT_TRUE(view.reduced);
    int64_t end = m.end_ms;
    controller_result_t r = press(EV_TOTAL_MINUS, 0, t);
    TEST_ASSERT_EQUAL(CONTROLLER_REVEALED, r.action);
    TEST_ASSERT_EQUAL_INT64(end, m.end_ms);
    look(t);
    TEST_ASSERT_FALSE(view.reduced);
    /* Le cran suivant règle. */
    r = press(EV_TOTAL_MINUS, 0, t + S(1));
    TEST_ASSERT_EQUAL(CONTROLLER_ADJUSTED, r.action);
    TEST_ASSERT_TRUE(m.end_ms < end);
}

/* Soirée 2, E18 : molette tenue après le premier cran, rien ne change. */
static void test_MAT_04_E18_held_reveal_press_does_not_adjust(void)
{
    int64_t start = start_countdown(M(45));
    int64_t t = start + M(1);
    int64_t end = m.end_ms;
    TEST_ASSERT_EQUAL(CONTROLLER_REVEALED, press(EV_TOTAL_PLUS, 0, t).action);
    for (uint32_t i = 1; i <= 20; i++) {
        controller_result_t r = press(EV_TOTAL_PLUS, i, t + i * 100);
        TEST_ASSERT_EQUAL(CONTROLLER_NONE, r.action);
        TEST_ASSERT_EQUAL_INT64(end, m.end_ms);
    }
    /* Un nouvel appui règle de nouveau. */
    TEST_ASSERT_EQUAL(CONTROLLER_ADJUSTED,
                      press(EV_TOTAL_PLUS, 0, t + S(3)).action);
    TEST_ASSERT_TRUE(m.end_ms > end);
}

/* Le bouton du rappel montre aussi les secondes, sans changer le rappel. */
static void test_MAT_04_reminder_button_reveals(void)
{
    int64_t start = start_countdown(M(45));
    int64_t t = start + M(1);
    int64_t interval = m.interval_ms;
    controller_result_t r = press(EV_NEXT_REMINDER, 0, t);
    TEST_ASSERT_EQUAL(CONTROLLER_REVEALED, r.action);
    TEST_ASSERT_EQUAL_INT64(interval, m.interval_ms);
}

/* L'acquittement agit toujours, même en affichage réduit (GEN-03). */
static void test_MAT_04_acknowledge_not_swallowed(void)
{
    int64_t start = start_countdown(M(45));
    int64_t t = start + M(1);
    look(t);
    TEST_ASSERT_TRUE(view.reduced);
    TEST_ASSERT_EQUAL(CONTROLLER_ACKNOWLEDGED,
                      press(EV_ACKNOWLEDGE, 0, t).action);
}

/* En affichage complet, le premier cran règle tout de suite. */
static void test_MAT_04_full_display_adjusts_at_once(void)
{
    int64_t start = start_countdown(M(20));
    look(start + M(1));
    TEST_ASSERT_FALSE(view.reduced);
    TEST_ASSERT_EQUAL(CONTROLLER_ADJUSTED,
                      press(EV_TOTAL_PLUS, 0, start + M(1)).action);
}

/* ---- Écran -------------------------------------------------------------- */

static ktimer_view_t alert_view(bool reminder, bool end, bool inverted)
{
    ktimer_view_t v;
    memset(&v, 0, sizeof v);
    v.state = end ? STATE_FINISHED : STATE_RUNNING;
    v.reminder_alert = reminder;
    v.end_alert = end;
    v.alert_inverted = inverted;
    return v;
}

/* Soirée 1, B5 : complet à l'entrée et à la sortie d'une alerte inversée. */
static void test_B5_full_refresh_around_inverted_alert(void)
{
    ktimer_view_t normal = alert_view(false, false, false);
    ktimer_view_t reminder = alert_view(true, false, true);
    ktimer_view_t end = alert_view(false, true, true);

    TEST_ASSERT_FALSE(controller_full_refresh(&c, &normal));
    TEST_ASSERT_TRUE(controller_full_refresh(&c, &reminder));
    TEST_ASSERT_FALSE(controller_full_refresh(&c, &reminder));
    TEST_ASSERT_TRUE(controller_full_refresh(&c, &normal));
    TEST_ASSERT_FALSE(controller_full_refresh(&c, &normal));
    TEST_ASSERT_TRUE(controller_full_refresh(&c, &end));
    /* Du rappel à la fin, l'écran reste inversé : partiel suffit. */
    controller_init(&c);
    controller_full_refresh(&c, &reminder);
    TEST_ASSERT_FALSE(controller_full_refresh(&c, &end));
}

/* Une alerte dans sa phase non inversée (motif clignotant) compte normale. */
static void test_B5_non_inverted_phase_counts_as_normal(void)
{
    ktimer_view_t off_phase = alert_view(true, false, false);
    TEST_ASSERT_FALSE(controller_full_refresh(&c, &off_phase));
}

/* Avec la vraie minuterie : entrée en alerte de fin, puis acquittement. */
static void test_B5_with_timer(void)
{
    int64_t start = start_countdown(S(30));
    look(start + S(1));
    TEST_ASSERT_FALSE(controller_full_refresh(&c, &view));
    int64_t t = start + S(31);
    ktimer_advance(&m, t);
    look(t);
    TEST_ASSERT_TRUE(view.end_alert && view.alert_inverted);
    TEST_ASSERT_TRUE(controller_full_refresh(&c, &view));
    press(EV_ACKNOWLEDGE, 0, t + 1);
    look(t + 1);
    TEST_ASSERT_TRUE(controller_full_refresh(&c, &view));
}

/* ---- Sonnerie, batterie, réveil ----------------------------------------- */

/* ECH-01, ECH-02 : la sonnerie de fin l'emporte sur celle du rappel. */
static void test_ECH_02_end_ring_wins(void)
{
    size_t count = 99;
    TEST_ASSERT_NULL(controller_ring(SIGNAL_NONE, &count));
    TEST_ASSERT_EQUAL_size_t(0, count);
    TEST_ASSERT_EQUAL_PTR(CST_REMINDER_RING,
                          controller_ring(SIGNAL_REMINDER_RING, &count));
    TEST_ASSERT_EQUAL_size_t(CST_REMINDER_RING_NOTE_COUNT, count);
    TEST_ASSERT_EQUAL_PTR(CST_END_RING,
                          controller_ring(SIGNAL_END_RING, &count));
    TEST_ASSERT_EQUAL_size_t(CST_END_RING_NOTE_COUNT, count);
    TEST_ASSERT_EQUAL_PTR(
        CST_END_RING,
        controller_ring(SIGNAL_END_RING | SIGNAL_REMINDER_RING, &count));
}

/* ERR-07 : seuil de batterie faible ; 0 = mesure impossible, pas d'alerte. */
static void test_ERR_07_low_battery_threshold(void)
{
    TEST_ASSERT_FALSE(controller_low_battery(0));
    TEST_ASSERT_TRUE(controller_low_battery(1));
    TEST_ASSERT_TRUE(controller_low_battery(CST_LOW_BATTERY_MV - 1));
    TEST_ASSERT_FALSE(controller_low_battery(CST_LOW_BATTERY_MV));
    TEST_ASSERT_FALSE(controller_low_battery(4300));
}

/* GEN-05 : le réveil le plus proche, de la minuterie ou de la saisie. */
static void test_GEN_05_next_wakeup_is_earliest(void)
{
    int64_t start = start_countdown(M(10));
    int64_t t = start + 10;
    int64_t timer_wakeup = ktimer_next_wakeup(&m, t);
    TEST_ASSERT_EQUAL_INT64(timer_wakeup,
                            controller_next_wakeup(&m, t, INT64_MAX));
    TEST_ASSERT_EQUAL_INT64(t + 5, controller_next_wakeup(&m, t, t + 5));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_REG_01_adjust_from_displayed_value);
    RUN_TEST(test_ETA_07_adjust_after_end_starts_from_zero);
    RUN_TEST(test_PLG_02_next_reminder);
    RUN_TEST(test_PLG_02_reminder_capped_by_displayed_total);
    RUN_TEST(test_ETA_19_acknowledge_end);
    RUN_TEST(test_power_off_request);
    RUN_TEST(test_MAT_04_first_press_reveals);
    RUN_TEST(test_MAT_04_E18_held_reveal_press_does_not_adjust);
    RUN_TEST(test_MAT_04_reminder_button_reveals);
    RUN_TEST(test_MAT_04_acknowledge_not_swallowed);
    RUN_TEST(test_MAT_04_full_display_adjusts_at_once);
    RUN_TEST(test_B5_full_refresh_around_inverted_alert);
    RUN_TEST(test_B5_non_inverted_phase_counts_as_normal);
    RUN_TEST(test_B5_with_timer);
    RUN_TEST(test_ECH_02_end_ring_wins);
    RUN_TEST(test_ERR_07_low_battery_threshold);
    RUN_TEST(test_GEN_05_next_wakeup_is_earliest);
    return UNITY_END();
}
