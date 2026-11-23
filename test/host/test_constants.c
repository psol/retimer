/*
 * Cohérence des tables de constantes (GEN-06) : une erreur de saisie dans
 * une table pendant les essais doit être vue avant de flasher.
 */
#include "constants.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

/* PLG-01, PLG-02 : bornes des plages. */
static void test_PLG_01_PLG_02_bounds(void)
{
    TEST_ASSERT_EQUAL_INT64(5 * SECOND_MS, CST_TOTAL_MIN_MS);
    TEST_ASSERT_EQUAL_INT64(24 * HOUR_MS, CST_TOTAL_MAX_MS);
    TEST_ASSERT_EQUAL_INT64(2 * HOUR_MS, CST_REMINDER_MAX_MS);
}

/* CST-01, CST-02, CST-10 : valeurs de départ de SPEC.md. */
static void test_CST_01_02_10_delays(void)
{
    TEST_ASSERT_EQUAL_INT64(3 * SECOND_MS, CST_START_DELAY_MS);
    TEST_ASSERT_EQUAL_INT64(2 * MINUTE_MS, CST_INACTIVITY_DELAY_MS);
    TEST_ASSERT_EQUAL_INT64(1 * HOUR_MS, CST_END_GIVE_UP_DELAY_MS);
}

/* REG-02, CST-03 : paliers triés, 5 s à partir de 0, 5 min à partir de 2 h. */
static void test_REG_02_CST_03_tiers(void)
{
    TEST_ASSERT_TRUE(CST_TOTAL_TIER_COUNT > 0);
    TEST_ASSERT_EQUAL_INT64(0, CST_TOTAL_TIERS[0].threshold_ms);
    TEST_ASSERT_EQUAL_INT64(5 * SECOND_MS, CST_TOTAL_TIERS[0].step_ms);
    for (size_t i = 1; i < CST_TOTAL_TIER_COUNT; i++) {
        TEST_ASSERT_TRUE(CST_TOTAL_TIERS[i].threshold_ms >
                         CST_TOTAL_TIERS[i - 1].threshold_ms);
        TEST_ASSERT_TRUE(CST_TOTAL_TIERS[i].step_ms >=
                         CST_TOTAL_TIERS[i - 1].step_ms);
    }
    const cst_tier_t *last =
        &CST_TOTAL_TIERS[CST_TOTAL_TIER_COUNT - 1];
    TEST_ASSERT_EQUAL_INT64(2 * HOUR_MS, last->threshold_ms);
    TEST_ASSERT_EQUAL_INT64(5 * MINUTE_MS, last->step_ms);
    /* Chaque seuil tombe sur un multiple de son pas : l'alignement marche. */
    for (size_t i = 0; i < CST_TOTAL_TIER_COUNT; i++) {
        TEST_ASSERT_EQUAL_INT64(0, CST_TOTAL_TIERS[i].threshold_ms %
                                       CST_TOTAL_TIERS[i].step_ms);
    }
}

/* REG-03, CST-04 : accélération croissante, à partir de ×1. */
static void test_REG_03_CST_04_acceleration(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, CST_ACCELERATION[0].from_repeat);
    TEST_ASSERT_EQUAL_UINT32(1, CST_ACCELERATION[0].multiplier);
    for (size_t i = 1; i < CST_ACCELERATION_COUNT; i++) {
        TEST_ASSERT_TRUE(CST_ACCELERATION[i].from_repeat >
                         CST_ACCELERATION[i - 1].from_repeat);
        TEST_ASSERT_TRUE(CST_ACCELERATION[i].multiplier >
                         CST_ACCELERATION[i - 1].multiplier);
    }
}

/* Interface réduite : arrêt d'abord, liste croissante, dans PLG-02. */
static void test_PLG_02_reminder_intervals(void)
{
    TEST_ASSERT_EQUAL_INT64(0, CST_REMINDER_INTERVALS_MS[0]);
    for (size_t i = 1; i < CST_REMINDER_INTERVAL_COUNT; i++) {
        TEST_ASSERT_TRUE(CST_REMINDER_INTERVALS_MS[i] >
                         CST_REMINDER_INTERVALS_MS[i - 1]);
        TEST_ASSERT_TRUE(CST_REMINDER_INTERVALS_MS[i] <= CST_REMINDER_MAX_MS);
    }
}

/* CMD-06, CST-05 : plusieurs bips brefs sur 1 à 2 s, pas de tonalité continue. */
static void check_ring(const cst_note_t *notes, size_t count)
{
    int64_t total = 0;
    int beeps = 0;
    for (size_t i = 0; i < count; i++) {
        total += notes[i].duration_ms;
        if (notes[i].frequency_hz > 0) {
            beeps++;
            TEST_ASSERT_TRUE(notes[i].duration_ms <= 300);
        }
    }
    TEST_ASSERT_TRUE(beeps >= 2);
    TEST_ASSERT_TRUE(total <= 2 * SECOND_MS);
}

static void test_CMD_06_CST_05_rings(void)
{
    check_ring(CST_REMINDER_RING, CST_REMINDER_RING_NOTE_COUNT);
    check_ring(CST_END_RING, CST_END_RING_NOTE_COUNT);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_PLG_01_PLG_02_bounds);
    RUN_TEST(test_CST_01_02_10_delays);
    RUN_TEST(test_REG_02_CST_03_tiers);
    RUN_TEST(test_REG_03_CST_04_acceleration);
    RUN_TEST(test_PLG_02_reminder_intervals);
    RUN_TEST(test_CMD_06_CST_05_rings);
    return UNITY_END();
}
