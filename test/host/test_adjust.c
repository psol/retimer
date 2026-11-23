/* Tests du composant reglage : REG-01 à REG-04, PLG-01, PLG-02, ERR-06. */
#include "constants.h"
#include "adjust.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

#define S(x) ((int64_t)(x) * SECOND_MS)
#define M(x) ((int64_t)(x) * MINUTE_MS)
#define H(x) ((int64_t)(x) * HOUR_MS)

/* REG-02 : partant de 0, on avance de 5 s. */
static void test_REG_02_from_zero_step_of_5_s(void)
{
    TEST_ASSERT_EQUAL_INT64(S(5), adjust_total_step(0, ADJUST_UP, 0));
    TEST_ASSERT_EQUAL_INT64(S(5), adjust_total(0, ADJUST_UP, 0));
    TEST_ASSERT_EQUAL_INT64(S(10), adjust_total(S(5), ADJUST_UP, 0));
}

/* REG-02 : partant de 2 h, on avance directement de 5 min. */
static void test_REG_02_from_2_h_step_of_5_min(void)
{
    TEST_ASSERT_EQUAL_INT64(M(5), adjust_total_step(H(2), ADJUST_UP, 0));
    TEST_ASSERT_EQUAL_INT64(H(2) + M(5), adjust_total(H(2), ADJUST_UP, 0));
}

/* REG-02, REG-04 : le pas suit la table CST-03, palier par palier. */
static void test_REG_04_step_from_table(void)
{
    for (size_t i = 0; i < CST_TOTAL_TIER_COUNT; i++) {
        TEST_ASSERT_EQUAL_INT64(
            CST_TOTAL_TIERS[i].step_ms,
            adjust_total_step(CST_TOTAL_TIERS[i].threshold_ms, ADJUST_UP,
                              0));
    }
}

/* En descendant, on prend le pas du palier inférieur : 1:00 → 0:55. */
static void test_REG_02_going_down_uses_lower_tier(void)
{
    TEST_ASSERT_EQUAL_INT64(S(55), adjust_total(M(1), ADJUST_DOWN, 0));
    TEST_ASSERT_EQUAL_INT64(H(2) - M(2), adjust_total(H(2), ADJUST_DOWN, 0));
}

/* REG-01 : on part de la valeur affichée, alignée sur le pas. */
static void test_REG_01_from_displayed_value(void)
{
    /* 30 min réglées, 10 min passées : on part de 20 min, pas de 30. */
    TEST_ASSERT_EQUAL_INT64(M(21), adjust_total(M(20), ADJUST_UP, 0));
    /* Temps restant en cours de décompte : 19:37,4 → 20:00 ou 19:30. */
    int64_t remaining = M(19) + S(37) + 400;
    TEST_ASSERT_EQUAL_INT64(M(20), adjust_total(remaining, ADJUST_UP, 0));
    TEST_ASSERT_EQUAL_INT64(M(19) + S(30),
                            adjust_total(remaining, ADJUST_DOWN, 0));
}

/* REG-03 : le pas grandit avec le maintien (CST-04 : ×1, ×2, ×6). */
static void test_REG_03_acceleration_while_held(void)
{
    TEST_ASSERT_EQUAL_INT64(S(5), adjust_total_step(0, ADJUST_UP, 0));
    TEST_ASSERT_EQUAL_INT64(S(10), adjust_total_step(0, ADJUST_UP, 6));
    TEST_ASSERT_EQUAL_INT64(S(30), adjust_total_step(0, ADJUST_UP, 14));
    TEST_ASSERT_EQUAL_INT64(M(10), adjust_total_step(H(3), ADJUST_UP, 6));
}

/*
 * REG-01, REG-03 : molette maintenue pendant le décompte. Le temps restant
 * passe sous la valeur réglée entre deux répétitions ; on repart de la valeur
 * affichée (00:30) et non de 29,85 s, sinon la valeur reste bloquée à 30 s.
 */
static void test_REG_03_held_while_counting_down(void)
{
    TEST_ASSERT_EQUAL_INT64(M(1), adjust_total(S(29) + 850, ADJUST_UP, 14));
    TEST_ASSERT_EQUAL_INT64(S(35), adjust_total(S(29) + 850, ADJUST_UP, 0));
    TEST_ASSERT_EQUAL_INT64(S(25), adjust_total(S(29) + 850, ADJUST_DOWN, 0));
}

/* PLG-01 : sous 5 s, on va à 5 s en montant et à 0 en descendant. */
static void test_PLG_01_minimum_5_s_and_zero(void)
{
    TEST_ASSERT_EQUAL_INT64(0, adjust_total(S(5), ADJUST_DOWN, 0));
    TEST_ASSERT_EQUAL_INT64(0, adjust_total(S(3), ADJUST_DOWN, 0));
    TEST_ASSERT_EQUAL_INT64(S(5), adjust_total(S(3), ADJUST_UP, 0));
    TEST_ASSERT_EQUAL_INT64(S(5), adjust_total(1, ADJUST_UP, 0));
}

/* ERR-06 : la valeur reste bloquée au minimum ou au maximum. */
static void test_ERR_06_bounds(void)
{
    TEST_ASSERT_EQUAL_INT64(0, adjust_total(0, ADJUST_DOWN, 0));
    TEST_ASSERT_EQUAL_INT64(0, adjust_total(0, ADJUST_DOWN, 20));
    TEST_ASSERT_EQUAL_INT64(H(24), adjust_total(H(24), ADJUST_UP, 0));
    TEST_ASSERT_EQUAL_INT64(H(24), adjust_total(H(24) - M(1), ADJUST_UP, 20));
}

/* PLG-03 : 45 s pour le micro-ondes, en quelques crans. */
static void test_PLG_03_short_countdown(void)
{
    int64_t v = 0;
    int steps = 0;
    while (v < S(45)) {
        v = adjust_total(v, ADJUST_UP, 0);
        steps++;
    }
    TEST_ASSERT_EQUAL_INT64(S(45), v);
    TEST_ASSERT_EQUAL_INT(9, steps);
}

/* Monter puis descendre ramène à la même valeur (hors changement de palier). */
static void test_REG_01_round_trip(void)
{
    int64_t values[] = {S(10), M(3), M(12), M(45), H(1) + M(30), H(5)};
    for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
        int64_t top = adjust_total(values[i], ADJUST_UP, 0);
        TEST_ASSERT_EQUAL_INT64(values[i],
                                adjust_total(top, ADJUST_DOWN, 0));
    }
}

/* Interface réduite : la liste des rappels, puis retour à l'arrêt (ACQ-02). */
static void test_PLG_02_reminder_walks_the_list(void)
{
    int64_t v = 0;
    for (size_t i = 1; i < CST_REMINDER_INTERVAL_COUNT; i++) {
        v = adjust_next_reminder(v, 0);
        TEST_ASSERT_EQUAL_INT64(CST_REMINDER_INTERVALS_MS[i], v);
    }
    TEST_ASSERT_EQUAL_INT64(H(2), v);
    TEST_ASSERT_EQUAL_INT64(0, adjust_next_reminder(v, 0));
}

/* Une valeur hors liste passe à la suivante de la liste. */
static void test_PLG_02_reminder_not_in_list(void)
{
    TEST_ASSERT_EQUAL_INT64(M(5), adjust_next_reminder(M(4), 0));
    TEST_ASSERT_EQUAL_INT64(0, adjust_next_reminder(H(3), 0));
}

/* Le rappel ne dépasse pas la globale : avec 2:00, seulement 1 min. */
static void test_PLG_02_reminder_below_total(void)
{
    TEST_ASSERT_EQUAL_INT64(M(1), adjust_next_reminder(0, M(2)));
    TEST_ASSERT_EQUAL_INT64(0, adjust_next_reminder(M(1), M(2)));
    TEST_ASSERT_EQUAL_INT64(M(3), adjust_next_reminder(M(2), M(3) + 1));
    TEST_ASSERT_EQUAL_INT64(0, adjust_next_reminder(M(2), M(3)));
    /* Globale sous la première valeur : le rappel reste à l'arrêt. */
    TEST_ASSERT_EQUAL_INT64(0, adjust_next_reminder(0, S(45)));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_REG_02_from_zero_step_of_5_s);
    RUN_TEST(test_REG_02_from_2_h_step_of_5_min);
    RUN_TEST(test_REG_04_step_from_table);
    RUN_TEST(test_REG_02_going_down_uses_lower_tier);
    RUN_TEST(test_REG_01_from_displayed_value);
    RUN_TEST(test_REG_03_acceleration_while_held);
    RUN_TEST(test_PLG_01_minimum_5_s_and_zero);
    RUN_TEST(test_ERR_06_bounds);
    RUN_TEST(test_PLG_03_short_countdown);
    RUN_TEST(test_REG_01_round_trip);
    RUN_TEST(test_REG_03_held_while_counting_down);
    RUN_TEST(test_PLG_02_reminder_below_total);
    RUN_TEST(test_PLG_02_reminder_walks_the_list);
    RUN_TEST(test_PLG_02_reminder_not_in_list);
    return UNITY_END();
}
