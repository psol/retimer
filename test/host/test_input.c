/*
 * Tests de la logique de saisie (CLAUDE.md, « Interface réduite ») :
 * anti-rebond, répétition au maintien de la molette, correspondance des
 * commandes avec CMD-01, CMD-02 et CMD-03.
 */
#include "constants.h"
#include "input.h"
#include "unity.h"

static input_t s;
static input_event_t ev[8];

void setUp(void) { input_init(&s); }
void tearDown(void) {}

static size_t read_input(int64_t t, uint32_t pressed)
{
    return input_process(&s, t, pressed, ev, 8);
}

#define UP INPUT_MASK(BUTTON_UP)
#define DOWN INPUT_MASK(BUTTON_DOWN)
#define PUSH INPUT_MASK(BUTTON_PUSH)
#define REMINDER INPUT_MASK(BUTTON_REMINDER)

/* CMD-01 : molette haut et bas règlent la globale. */
static void test_CMD_01_wheel_up_and_down(void)
{
    read_input(0, UP);
    TEST_ASSERT_EQUAL(1, read_input(CST_DEBOUNCE_MS, UP));
    TEST_ASSERT_EQUAL(EV_TOTAL_PLUS, ev[0].type);
    TEST_ASSERT_EQUAL_UINT32(0, ev[0].repeat);
    read_input(100, 0);
    read_input(100 + CST_DEBOUNCE_MS, 0);
    read_input(200, DOWN);
    TEST_ASSERT_EQUAL(1, read_input(200 + CST_DEBOUNCE_MS, DOWN));
    TEST_ASSERT_EQUAL(EV_TOTAL_MINUS, ev[0].type);
}

/* CMD-02 : le bouton sous la molette passe au rappel suivant. */
static void test_CMD_02_reminder_button(void)
{
    read_input(0, REMINDER);
    TEST_ASSERT_EQUAL(1, read_input(CST_DEBOUNCE_MS, REMINDER));
    TEST_ASSERT_EQUAL(EV_NEXT_REMINDER, ev[0].type);
    /* Pas de répétition : un appui = un intervalle. */
    TEST_ASSERT_EQUAL(0, read_input(CST_DEBOUNCE_MS + POWER_OFF_HOLD_MS - 1,
                                    REMINDER));
}

/* Le bouton du rappel est aussi le bouton marche : maintenu, il éteint. */
static void test_reminder_button_held_powers_off(void)
{
    read_input(0, REMINDER);
    read_input(CST_DEBOUNCE_MS, REMINDER);
    TEST_ASSERT_EQUAL_INT64(CST_DEBOUNCE_MS + POWER_OFF_HOLD_MS,
                            input_next_wakeup(&s));
    TEST_ASSERT_EQUAL(1, read_input(CST_DEBOUNCE_MS + POWER_OFF_HOLD_MS,
                                    REMINDER));
    TEST_ASSERT_EQUAL(EV_POWER_OFF, ev[0].type);
    /* Une seule fois, même si l'appui continue. */
    TEST_ASSERT_EQUAL(0, read_input(10000, REMINDER));
}

/* CMD-03 : l'appui sur la molette acquitte. */
static void test_CMD_03_push_acknowledges(void)
{
    read_input(0, PUSH);
    TEST_ASSERT_EQUAL(1, read_input(CST_DEBOUNCE_MS, PUSH));
    TEST_ASSERT_EQUAL(EV_ACKNOWLEDGE, ev[0].type);
    TEST_ASSERT_EQUAL(0, read_input(5000, PUSH));
}

/* Anti-rebond : un rebond plus court que CST_DEBOUNCE_MS est ignoré. */
static void test_debounce(void)
{
    read_input(0, UP);
    read_input(5, 0);
    read_input(8, UP);
    TEST_ASSERT_EQUAL(0, read_input(8 + CST_DEBOUNCE_MS - 1, UP));
    TEST_ASSERT_EQUAL(1, read_input(8 + CST_DEBOUNCE_MS, UP));
    /* Un rebond au relâchement ne crée pas de deuxième appui. */
    read_input(300, 0);
    read_input(305, UP);
    read_input(310, 0);
    TEST_ASSERT_EQUAL(0, read_input(310 + CST_DEBOUNCE_MS, 0));
}

/* REG-03 (maintien) : répétition après un délai, rang croissant. */
static void test_REG_03_repeat_while_held(void)
{
    int64_t t0 = CST_DEBOUNCE_MS;
    read_input(0, UP);
    read_input(t0, UP);
    TEST_ASSERT_EQUAL(0, read_input(t0 + CST_REPEAT_DELAY_MS - 1, UP));
    TEST_ASSERT_EQUAL(1, read_input(t0 + CST_REPEAT_DELAY_MS, UP));
    TEST_ASSERT_EQUAL_UINT32(1, ev[0].repeat);
    int64_t t = t0 + CST_REPEAT_DELAY_MS + CST_REPEAT_PERIOD_MS;
    TEST_ASSERT_EQUAL_INT64(t, input_next_wakeup(&s));
    TEST_ASSERT_EQUAL(1, read_input(t, UP));
    TEST_ASSERT_EQUAL_UINT32(2, ev[0].repeat);
    /* Le relâchement arrête la répétition. */
    read_input(t + 10, 0);
    read_input(t + 10 + CST_DEBOUNCE_MS, 0);
    TEST_ASSERT_EQUAL_INT64(INT64_MAX, input_next_wakeup(&s));
    TEST_ASSERT_FALSE(input_busy(&s));
}

/* Un nouvel appui repart du premier rang (pas d'accélération). */
static void test_REG_03_new_press_starts_over(void)
{
    read_input(0, UP);
    read_input(CST_DEBOUNCE_MS, UP);
    read_input(2000, UP);
    read_input(2100, 0);
    read_input(2100 + CST_DEBOUNCE_MS, 0);
    read_input(3000, UP);
    TEST_ASSERT_EQUAL(1, read_input(3000 + CST_DEBOUNCE_MS, UP));
    TEST_ASSERT_EQUAL_UINT32(0, ev[0].repeat);
}

/* Réveil par un bouton : l'appui compte une fois, même s'il dure. */
static void test_wake_press_counts_once(void)
{
    TEST_ASSERT_EQUAL(1, input_wake_press(&s, BUTTON_UP, ev, 8));
    TEST_ASSERT_EQUAL(EV_TOTAL_PLUS, ev[0].type);
    TEST_ASSERT_EQUAL(0, read_input(0, UP));
    TEST_ASSERT_EQUAL(0, read_input(100, UP));
    read_input(200, 0);
    TEST_ASSERT_EQUAL(0, read_input(200 + CST_DEBOUNCE_MS, 0));
    TEST_ASSERT_FALSE(input_busy(&s));
}

/* À l'allumage, le bouton marche encore tenu ne compte pas et n'éteint pas. */
static void test_power_on_press_is_ignored(void)
{
    input_ignore_until_release(&s, BUTTON_REMINDER);
    TEST_ASSERT_EQUAL(0, read_input(0, REMINDER));
    TEST_ASSERT_EQUAL(0, read_input(10000, REMINDER));
    read_input(10100, 0);
    read_input(10100 + CST_DEBOUNCE_MS, 0);
    /* L'appui suivant compte normalement. */
    read_input(11000, REMINDER);
    TEST_ASSERT_EQUAL(1, read_input(11000 + CST_DEBOUNCE_MS, REMINDER));
    TEST_ASSERT_EQUAL(EV_NEXT_REMINDER, ev[0].type);
}

/* Deux boutons en même temps : deux événements. */
static void test_two_buttons(void)
{
    read_input(0, UP | REMINDER);
    TEST_ASSERT_EQUAL(2, read_input(CST_DEBOUNCE_MS, UP | REMINDER));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_CMD_01_wheel_up_and_down);
    RUN_TEST(test_CMD_02_reminder_button);
    RUN_TEST(test_CMD_03_push_acknowledges);
    RUN_TEST(test_reminder_button_held_powers_off);
    RUN_TEST(test_debounce);
    RUN_TEST(test_REG_03_repeat_while_held);
    RUN_TEST(test_REG_03_new_press_starts_over);
    RUN_TEST(test_wake_press_counts_once);
    RUN_TEST(test_power_on_press_is_ignored);
    RUN_TEST(test_two_buttons);
    return UNITY_END();
}
