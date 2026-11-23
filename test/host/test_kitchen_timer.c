/*
 * Tests du composant minuterie : ETA, DEM, ECH, MOD, ACQ, ERR de SPEC.md.
 * Le temps est simulé : chaque test fixe lui-même « maintenant ».
 */
#include "constants.h"
#include "kitchen_timer.h"
#include "unity.h"

#define S(x) ((int64_t)(x) * SECOND_MS)
#define M(x) ((int64_t)(x) * MINUTE_MS)
#define H(x) ((int64_t)(x) * HOUR_MS)

/* Instant arbitraire de départ, pour ne pas tester qu'à partir de 0. */
#define T0 ((int64_t)1000000)

static ktimer_t m;
static ktimer_view_t view;

void setUp(void) { ktimer_init(&m); }
void tearDown(void) {}

static void look(int64_t t) { ktimer_view(&m, t, &view); }

/* Règle la globale (et le rappel) à T0, puis laisse démarrer. */
static int64_t start_countdown(int64_t total, int64_t reminder)
{
    ktimer_set_total(&m, T0, total);
    if (reminder > 0) {
        ktimer_set_reminder(&m, T0, reminder);
    }
    int64_t start_value = T0 + CST_START_DELAY_MS;
    ktimer_advance(&m, start_value);
    TEST_ASSERT_EQUAL(STATE_RUNNING, m.state);
    return start_value;
}

/* ---- États et transitions ----------------------------------------------- */

static void test_ETA_01_initial_state_idle(void)
{
    TEST_ASSERT_EQUAL(STATE_IDLE, m.state);
    TEST_ASSERT_EQUAL_INT64(KTIMER_NEVER, ktimer_next_wakeup(&m, T0));
}

static void test_ETA_10_ETA_11_setting_then_imminent_start(void)
{
    ktimer_set_total(&m, T0, M(10));
    TEST_ASSERT_EQUAL(STATE_STARTING, m.state);
    look(T0);
    TEST_ASSERT_EQUAL_INT64(M(10), view.total_ms);
}

static void test_ETA_10_reminder_alone_opens_setting(void)
{
    ktimer_set_reminder(&m, T0, M(5));
    TEST_ASSERT_EQUAL(STATE_SETTING, m.state);
    look(T0);
    TEST_ASSERT_EQUAL(REMINDER_SET, view.reminder);
    TEST_ASSERT_EQUAL_INT64(M(5), view.reminder_ms);
}

/* ETA-12, ERR-01 : globale nulle, pas de démarrage, veille après CST-02. */
static void test_ETA_12_ERR_01_zero_total_back_to_idle(void)
{
    ktimer_set_reminder(&m, T0, M(5));
    int64_t limit = T0 + CST_INACTIVITY_DELAY_MS;
    TEST_ASSERT_EQUAL_INT64(limit, ktimer_next_wakeup(&m, T0));
    ktimer_advance(&m, limit - 1);
    TEST_ASSERT_EQUAL(STATE_SETTING, m.state);
    ktimer_advance(&m, limit);
    TEST_ASSERT_EQUAL(STATE_IDLE, m.state);
}

/* Ramener la globale à 0 pendant le démarrage imminent : plus de démarrage. */
static void test_ETA_13_total_back_to_zero_before_start(void)
{
    ktimer_set_total(&m, T0, S(5));
    ktimer_set_total(&m, T0 + 500, 0);
    TEST_ASSERT_EQUAL(STATE_SETTING, m.state);
    ktimer_advance(&m, T0 + S(10));
    TEST_ASSERT_EQUAL(STATE_SETTING, m.state);
}

/* DEM-01, DEM-04, ETA-14 : démarrage CST-01 après la dernière action. */
static void test_DEM_01_DEM_04_ETA_14_start_after_delay(void)
{
    ktimer_set_total(&m, T0, M(1));
    ktimer_advance(&m, T0 + CST_START_DELAY_MS - 1);
    TEST_ASSERT_EQUAL(STATE_STARTING, m.state);
    ktimer_advance(&m, T0 + CST_START_DELAY_MS);
    TEST_ASSERT_EQUAL(STATE_RUNNING, m.state);
    TEST_ASSERT_EQUAL_INT64(T0 + CST_START_DELAY_MS + M(1), m.end_ms);
}

/* DEM-03, ETA-13 : toute action pendant le délai le recommence. */
static void test_DEM_03_action_restarts_delay(void)
{
    ktimer_set_total(&m, T0, M(1));
    ktimer_set_total(&m, T0 + S(2), M(2));
    ktimer_advance(&m, T0 + CST_START_DELAY_MS);
    TEST_ASSERT_EQUAL(STATE_STARTING, m.state);
    ktimer_set_reminder(&m, T0 + S(4), M(1));
    ktimer_advance(&m, T0 + S(4) + CST_START_DELAY_MS - 1);
    TEST_ASSERT_EQUAL(STATE_STARTING, m.state);
    ktimer_advance(&m, T0 + S(4) + CST_START_DELAY_MS);
    TEST_ASSERT_EQUAL(STATE_RUNNING, m.state);
}

/* DEM-02 : l'affichage clignote pendant le délai, et réveille à chaque phase. */
static void test_DEM_02_blink(void)
{
    ktimer_set_total(&m, T0, M(1));
    look(T0);
    TEST_ASSERT_FALSE(view.blink_hidden);
    look(T0 + CST_BLINK_PERIOD_MS / 2);
    TEST_ASSERT_TRUE(view.blink_hidden);
    look(T0 + CST_BLINK_PERIOD_MS);
    TEST_ASSERT_FALSE(view.blink_hidden);
    TEST_ASSERT_EQUAL_INT64(T0 + CST_BLINK_PERIOD_MS / 2,
                            ktimer_next_wakeup(&m, T0));
    /* Plus de clignotement une fois le décompte parti. */
    look(T0 + CST_START_DELAY_MS + CST_BLINK_PERIOD_MS / 2);
    TEST_ASSERT_FALSE(view.blink_hidden);
}

/* Un réveil tardif ne décale pas le départ (DEM-04). */
static void test_DEM_04_late_wakeup(void)
{
    ktimer_set_total(&m, T0, M(1));
    ktimer_advance(&m, T0 + S(10));
    TEST_ASSERT_EQUAL(STATE_RUNNING, m.state);
    look(T0 + S(10));
    TEST_ASSERT_EQUAL_INT64(M(1) - S(10) + CST_START_DELAY_MS,
                            view.total_ms);
}

/* ETA-17, ECH-02 : sonnerie de fin, alerte de fin, Terminé, « 00:00 ». */
static void test_ETA_17_ECH_02_ETA_07_end_of_countdown(void)
{
    int64_t start_value = start_countdown(M(1), 0);
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_NONE,
                             ktimer_advance(&m, start_value + M(1) - 1));
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_END_RING,
                             ktimer_advance(&m, start_value + M(1)));
    TEST_ASSERT_EQUAL(STATE_FINISHED, m.state);
    look(start_value + M(1));
    TEST_ASSERT_TRUE(view.end_alert);
    TEST_ASSERT_EQUAL_INT64(0, view.total_ms);
    /* Une seule sonnerie (GEN-04). */
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_NONE,
                             ktimer_advance(&m, start_value + M(2)));
}

/* ETA-18, ACQ-01 : globale ramenée à zéro en décompte, tout est acquitté. */
static void test_ETA_18_ACQ_01_total_to_zero_stops_all(void)
{
    int64_t start_value = start_countdown(M(10), M(1));
    ktimer_advance(&m, start_value + M(1));
    TEST_ASSERT_TRUE(m.reminder_alert);
    ktimer_set_total(&m, start_value + M(1) + S(5), 0);
    TEST_ASSERT_EQUAL(STATE_IDLE, m.state);
    TEST_ASSERT_FALSE(m.reminder_alert);
    TEST_ASSERT_FALSE(m.end_alert);
}

/* ETA-19, ACQ-04 : le bouton acquitte la fin et renvoie en Veille. */
static void test_ETA_19_ACQ_04_acknowledge_end(void)
{
    int64_t start_value = start_countdown(M(1), 0);
    ktimer_advance(&m, start_value + M(1));
    ktimer_acknowledge(&m, start_value + M(1) + S(3));
    TEST_ASSERT_EQUAL(STATE_IDLE, m.state);
    TEST_ASSERT_FALSE(m.end_alert);
}

/* ETA-20, OUV-07 : en Terminé, régler acquitte la fin et commence un réglage. */
static void test_ETA_20_OUV_07_adjust_when_finished(void)
{
    int64_t start_value = start_countdown(M(1), 0);
    ktimer_advance(&m, start_value + M(1));
    ktimer_set_total(&m, start_value + M(1) + S(2), S(5));
    TEST_ASSERT_EQUAL(STATE_STARTING, m.state);
    TEST_ASSERT_FALSE(m.end_alert);
    TEST_ASSERT_EQUAL_INT64(S(5), m.total_ms);
}

static void test_ETA_20_OUV_07_reminder_when_finished(void)
{
    int64_t start_value = start_countdown(M(1), 0);
    ktimer_advance(&m, start_value + M(1));
    ktimer_set_reminder(&m, start_value + M(1) + S(2), M(1));
    TEST_ASSERT_EQUAL(STATE_SETTING, m.state);
    TEST_ASSERT_FALSE(m.end_alert);
}

/* ETA-21, ERR-03 : alerte de fin abandonnée après CST-10. */
static void test_ETA_21_ERR_03_end_alert_gives_up(void)
{
    int64_t start_value = start_countdown(M(1), 0);
    int64_t end = start_value + M(1);
    ktimer_advance(&m, end);
    TEST_ASSERT_TRUE(ktimer_next_wakeup(&m, end + 1) <=
                     end + CST_END_GIVE_UP_DELAY_MS);
    ktimer_advance(&m, end + CST_END_GIVE_UP_DELAY_MS - 1);
    TEST_ASSERT_EQUAL(STATE_FINISHED, m.state);
    ktimer_advance(&m, end + CST_END_GIVE_UP_DELAY_MS);
    TEST_ASSERT_EQUAL(STATE_IDLE, m.state);
    TEST_ASSERT_FALSE(m.end_alert);
}

/* ---- Échéances ---------------------------------------------------------- */

/* ECH-01, ETA-15 : sonnerie de rappel une fois, puis alerte persistante. */
static void test_ECH_01_ETA_15_reminder_deadline(void)
{
    int64_t start_value = start_countdown(M(30), M(5));
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_NONE,
                             ktimer_advance(&m, start_value + M(5) - 1));
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_REMINDER_RING,
                             ktimer_advance(&m, start_value + M(5)));
    TEST_ASSERT_EQUAL(STATE_RUNNING, m.state);
    look(start_value + M(6));
    TEST_ASSERT_TRUE(view.reminder_alert);
    TEST_ASSERT_EQUAL(REMINDER_ALERT, view.reminder);
}

/* ECH-06, ERR-02 : pas de relance ni de nouvelle sonnerie sans acquittement. */
static void test_ECH_06_ERR_02_no_restart_without_ack(void)
{
    int64_t start_value = start_countdown(M(30), M(5));
    ktimer_advance(&m, start_value + M(5));
    for (int i = 6; i < 30; i++) {
        TEST_ASSERT_EQUAL_UINT32(SIGNAL_NONE,
                                 ktimer_advance(&m, start_value + M(i)));
    }
    TEST_ASSERT_TRUE(m.reminder_alert);
}

/* ECH-05, ETA-16 : le rappel repart de l'acquittement. */
static void test_ECH_05_ETA_16_reminder_restarts_from_ack(void)
{
    int64_t start_value = start_countdown(M(30), M(5));
    ktimer_advance(&m, start_value + M(5));
    int64_t ack = start_value + M(5) + S(47);
    ktimer_acknowledge(&m, ack);
    TEST_ASSERT_FALSE(m.reminder_alert);
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_NONE,
                             ktimer_advance(&m, ack + M(5) - 1));
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_REMINDER_RING,
                             ktimer_advance(&m, ack + M(5)));
}

/* ECH-03, ECH-04, ERR-02 : la fin sonne même avec une alerte de rappel ;
 * elle éteint l'alerte de rappel et arrête le rappel. */
static void test_ECH_03_ECH_04_end_during_reminder_alert(void)
{
    int64_t start_value = start_countdown(M(10), M(3));
    ktimer_advance(&m, start_value + M(3));
    TEST_ASSERT_TRUE(m.reminder_alert);
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_END_RING,
                             ktimer_advance(&m, start_value + M(10)));
    TEST_ASSERT_EQUAL(STATE_FINISHED, m.state);
    TEST_ASSERT_FALSE(m.reminder_alert);
    TEST_ASSERT_TRUE(m.end_alert);
    look(start_value + M(10));
    TEST_ASSERT_EQUAL(REMINDER_OFF, view.reminder);
}

/* ECH-07 : rappel suivant après la fin : plus d'échéance de rappel. */
static void test_ECH_07_reminder_after_end(void)
{
    int64_t start_value = start_countdown(M(10), M(4));
    ktimer_advance(&m, start_value + M(4));
    ktimer_acknowledge(&m, start_value + M(7)); /* prochain à 11 min > 10 min */
    look(start_value + M(8));
    TEST_ASSERT_EQUAL(REMINDER_SPENT, view.reminder);
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_END_RING,
                             ktimer_advance(&m, start_value + M(11)));
}

/* ECH-08 : échéances simultanées : seule la globale sonne. */
static void test_ECH_08_simultaneous_deadlines(void)
{
    int64_t start_value = start_countdown(M(10), M(5));
    ktimer_advance(&m, start_value + M(5));
    ktimer_acknowledge(&m, start_value + M(5)); /* prochain rappel à 10 min */
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_END_RING,
                             ktimer_advance(&m, start_value + M(10)));
}

/* Un réveil tardif au-delà des deux échéances ne sonne que la fin. */
static void test_ECH_08_late_wakeup_after_both_deadlines(void)
{
    int64_t start_value = start_countdown(M(10), M(4));
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_END_RING,
                             ktimer_advance(&m, start_value + M(12)));
}

/* ---- Modifications en cours de décompte --------------------------------- */

/* MOD-01, MOD-03 : la globale modifie le temps restant ; pas de redémarrage. */
static void test_MOD_01_MOD_03_change_total(void)
{
    int64_t start_value = start_countdown(M(30), 0);
    ktimer_set_total(&m, start_value + M(10), M(25));
    TEST_ASSERT_EQUAL(STATE_RUNNING, m.state);
    look(start_value + M(10));
    TEST_ASSERT_EQUAL_INT64(M(25), view.total_ms);
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_END_RING,
                             ktimer_advance(&m, start_value + M(35)));
}

/* Raccourcir la globale sous le prochain rappel : ECH-07 s'applique. */
static void test_MOD_01_ECH_07_shorten_below_reminder(void)
{
    int64_t start_value = start_countdown(M(30), M(10));
    ktimer_set_total(&m, start_value + M(2), M(5));
    look(start_value + M(3));
    TEST_ASSERT_EQUAL(REMINDER_SPENT, view.reminder);
    /* Rallonger rend le rappel à nouveau possible. */
    ktimer_set_total(&m, start_value + M(3), M(20));
    look(start_value + M(4));
    TEST_ASSERT_EQUAL(REMINDER_RUNNING, view.reminder);
    TEST_ASSERT_EQUAL_INT64(M(6), view.reminder_ms);
}

/* MOD-02 (lecture retenue pour OUV-02) : nouvel intervalle depuis maintenant. */
static void test_MOD_02_OUV_02_change_reminder(void)
{
    int64_t start_value = start_countdown(M(30), M(5));
    ktimer_set_reminder(&m, start_value + M(2), M(10));
    look(start_value + M(2));
    TEST_ASSERT_EQUAL_INT64(M(10), view.reminder_ms);
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_REMINDER_RING,
                             ktimer_advance(&m, start_value + M(12)));
}

/* ACQ-02 : rappel ramené à zéro : acquitte et désactive. */
static void test_ACQ_02_reminder_to_zero(void)
{
    int64_t start_value = start_countdown(M(30), M(5));
    ktimer_advance(&m, start_value + M(5));
    ktimer_set_reminder(&m, start_value + M(6), 0);
    TEST_ASSERT_FALSE(m.reminder_alert);
    look(start_value + M(6));
    TEST_ASSERT_EQUAL(REMINDER_OFF, view.reminder);
    for (int i = 7; i < 30; i++) {
        TEST_ASSERT_EQUAL_UINT32(SIGNAL_NONE,
                                 ktimer_advance(&m, start_value + M(i)));
    }
}

/* ACQ-03, OUV-01 : modifier le rappel pendant son alerte vaut acquittement. */
static void test_ACQ_03_OUV_01_change_acknowledges(void)
{
    int64_t start_value = start_countdown(M(30), M(5));
    ktimer_advance(&m, start_value + M(5));
    ktimer_set_reminder(&m, start_value + M(6), M(2));
    TEST_ASSERT_FALSE(m.reminder_alert);
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_REMINDER_RING,
                             ktimer_advance(&m, start_value + M(8)));
}

/* Modifier la globale n'acquitte pas l'alerte de rappel (autre minuterie). */
static void test_ACQ_03_total_does_not_acknowledge_reminder(void)
{
    int64_t start_value = start_countdown(M(30), M(5));
    ktimer_advance(&m, start_value + M(5));
    ktimer_set_total(&m, start_value + M(6), M(20));
    TEST_ASSERT_TRUE(m.reminder_alert);
}

/* MOD-04, OUV-08 : après ACQ-02, on peut réactiver le rappel. */
static void test_MOD_04_OUV_08_reactivate_reminder(void)
{
    int64_t start_value = start_countdown(M(30), M(5));
    ktimer_set_reminder(&m, start_value + M(1), 0);
    ktimer_set_reminder(&m, start_value + M(2), M(1));
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_REMINDER_RING,
                             ktimer_advance(&m, start_value + M(3)));
}

/* ---- Acquittement par le bouton ----------------------------------------- */

/* ACQ-05, ERR-04 : appui sans échéance en attente : aucun effet. */
static void test_ACQ_05_ERR_04_push_without_deadline(void)
{
    ktimer_acknowledge(&m, T0);
    TEST_ASSERT_EQUAL(STATE_IDLE, m.state);

    int64_t start_value = start_countdown(M(30), M(5));
    ktimer_t before = m;
    ktimer_acknowledge(&m, start_value + M(1));
    TEST_ASSERT_EQUAL_MEMORY(&before, &m, sizeof m);
}

/* ERR-06 : au-delà des plages, la valeur reste bornée. */
static void test_ERR_06_bounded_values(void)
{
    ktimer_set_total(&m, T0, H(30));
    TEST_ASSERT_EQUAL_INT64(CST_TOTAL_MAX_MS, m.total_ms);
    ktimer_set_reminder(&m, T0, H(3));
    TEST_ASSERT_EQUAL_INT64(CST_REMINDER_MAX_MS, m.interval_ms);
}

/* PLG-01 : 24 h décomptées à la milliseconde près. */
static void test_PLG_01_24_h_countdown(void)
{
    int64_t start_value = start_countdown(H(24), 0);
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_NONE,
                             ktimer_advance(&m, start_value + H(24) - 1));
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_END_RING,
                             ktimer_advance(&m, start_value + H(24)));
}

/* PLG-03 : 45 s sans rappel. */
static void test_PLG_03_short_countdown(void)
{
    int64_t start_value = start_countdown(S(45), 0);
    look(start_value);
    TEST_ASSERT_EQUAL(REMINDER_OFF, view.reminder);
    TEST_ASSERT_EQUAL_UINT32(SIGNAL_END_RING,
                             ktimer_advance(&m, start_value + S(45)));
}

/* ---- Affichage et réveils (GEN-05, MAT-04) ------------------------------ */

/* MAT-04, CST-08 : affichage réduit loin des échéances, complet ensuite. */
static void test_MAT_04_reduced_display(void)
{
    int64_t start_value = start_countdown(M(10), 0);
    /* Affichage complet pendant les premières secondes du décompte. */
    look(start_value);
    TEST_ASSERT_FALSE(view.reduced);
    look(start_value + CST_FULL_DISPLAY_AT_START_MS - 1);
    TEST_ASSERT_FALSE(view.reduced);
    look(start_value + CST_FULL_DISPLAY_AT_START_MS);
    TEST_ASSERT_TRUE(view.reduced);
    look(start_value + M(5));
    TEST_ASSERT_TRUE(view.reduced);
    look(start_value + M(10) - CST_FULL_DISPLAY_THRESHOLD_MS);
    TEST_ASSERT_FALSE(view.reduced);
}

/* MAT-04 : la minuterie la plus proche de son échéance impose le rythme. */
static void test_MAT_04_close_reminder_forces_full_display(void)
{
    int64_t start_value = start_countdown(M(30), M(5));
    look(start_value + S(30));
    TEST_ASSERT_TRUE(view.reduced);
    look(start_value + M(3) + S(10));
    TEST_ASSERT_FALSE(view.reduced);
}

/* MAT-04 : après un réglage pendant le décompte, on voit les secondes. */
static void test_MAT_04_full_display_after_adjust(void)
{
    int64_t start_value = start_countdown(M(10), 0);
    int64_t t = start_value + M(1);
    look(t);
    TEST_ASSERT_TRUE(view.reduced);
    ktimer_set_total(&m, t, M(12));
    look(t);
    TEST_ASSERT_FALSE(view.reduced);
    /* Réveil chaque seconde, puis retour à l'affichage réduit. */
    TEST_ASSERT_EQUAL_INT64(t + S(1), ktimer_next_wakeup(&m, t + 1));
    TEST_ASSERT_EQUAL_INT64(t + CST_FULL_DISPLAY_AFTER_ADJUST_MS,
                            ktimer_next_wakeup(
                                &m, t + CST_FULL_DISPLAY_AFTER_ADJUST_MS - 1));
    look(t + CST_FULL_DISPLAY_AFTER_ADJUST_MS);
    TEST_ASSERT_TRUE(view.reduced);
    /* Pareil pour le rappel. */
    ktimer_set_reminder(&m, t + S(20), M(5));
    look(t + S(20));
    TEST_ASSERT_FALSE(view.reduced);
}

/* GEN-05 : en affichage réduit, un réveil par minute au plus. */
static void test_GEN_05_wakeup_each_minute_in_reduced_display(void)
{
    /* 9:40 s'affiche « 09:-- » ; « 08:-- » à 8:59. */
    int64_t start_value = start_countdown(M(10), 0);
    int64_t t = start_value + S(20);
    TEST_ASSERT_EQUAL_INT64(start_value + M(1) + S(1),
                            ktimer_next_wakeup(&m, t));
    /* Fin de l'affichage complet du départ : réveil pour passer en réduit. */
    TEST_ASSERT_EQUAL_INT64(start_value + CST_FULL_DISPLAY_AT_START_MS,
                            ktimer_next_wakeup(
                                &m, start_value + CST_FULL_DISPLAY_AT_START_MS -
                                        1));
}

/* GEN-05 : en affichage complet, un réveil par seconde, sur le changement. */
static void test_GEN_05_wakeup_each_second_in_full_display(void)
{
    int64_t start_value = start_countdown(S(45), 0);
    TEST_ASSERT_EQUAL_INT64(start_value + S(1),
                            ktimer_next_wakeup(&m, start_value + 250));
    TEST_ASSERT_EQUAL_INT64(start_value + S(45),
                            ktimer_next_wakeup(&m, start_value + S(44) + 1));
}

/* GEN-05 : passage de l'affichage réduit au complet au bon instant. */
static void test_GEN_05_wakeup_when_switching_to_full_display(void)
{
    int64_t start_value = start_countdown(M(3) + S(30), 0);
    int64_t t = start_value + M(1); /* reste 2:30, « 02:-- » */
    TEST_ASSERT_EQUAL_INT64(start_value + M(3) + S(30) -
                                CST_FULL_DISPLAY_THRESHOLD_MS,
                            ktimer_next_wakeup(&m, t));
}

/* GEN-05 : pendant une alerte, réveil pour l'éclat de la LED (CST-06). */
static void test_GEN_05_CST_06_wakeup_for_led_flashes(void)
{
    if (!CST_ALERT_LED) {
        TEST_IGNORE_MESSAGE("LED désactivée dans les constantes");
    }
    int64_t start_value = start_countdown(M(1), 0);
    ktimer_advance(&m, start_value + M(1));
    TEST_ASSERT_EQUAL_INT64(start_value + M(1) + CST_ALERT_LED_PERIOD_MS,
                            ktimer_next_wakeup(&m, start_value + M(1) + 1));
    look(start_value + M(1) + CST_ALERT_LED_PERIOD_MS);
    TEST_ASSERT_TRUE(view.led_flash);
    look(start_value + M(1) + CST_ALERT_LED_PERIOD_MS + CST_ALERT_LED_FLASH_MS);
    TEST_ASSERT_FALSE(view.led_flash);
}

/* ERR-08 : l'état initial est la Veille, rien n'est conservé. */
static void test_ERR_08_state_after_power_loss(void)
{
    start_countdown(M(30), M(5));
    ktimer_init(&m);
    TEST_ASSERT_EQUAL(STATE_IDLE, m.state);
    TEST_ASSERT_EQUAL_INT64(0, m.interval_ms);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ETA_01_initial_state_idle);
    RUN_TEST(test_ETA_10_ETA_11_setting_then_imminent_start);
    RUN_TEST(test_ETA_10_reminder_alone_opens_setting);
    RUN_TEST(test_ETA_12_ERR_01_zero_total_back_to_idle);
    RUN_TEST(test_ETA_13_total_back_to_zero_before_start);
    RUN_TEST(test_DEM_01_DEM_04_ETA_14_start_after_delay);
    RUN_TEST(test_DEM_03_action_restarts_delay);
    RUN_TEST(test_DEM_02_blink);
    RUN_TEST(test_DEM_04_late_wakeup);
    RUN_TEST(test_ETA_17_ECH_02_ETA_07_end_of_countdown);
    RUN_TEST(test_ETA_18_ACQ_01_total_to_zero_stops_all);
    RUN_TEST(test_ETA_19_ACQ_04_acknowledge_end);
    RUN_TEST(test_ETA_20_OUV_07_adjust_when_finished);
    RUN_TEST(test_ETA_20_OUV_07_reminder_when_finished);
    RUN_TEST(test_ETA_21_ERR_03_end_alert_gives_up);
    RUN_TEST(test_ECH_01_ETA_15_reminder_deadline);
    RUN_TEST(test_ECH_06_ERR_02_no_restart_without_ack);
    RUN_TEST(test_ECH_05_ETA_16_reminder_restarts_from_ack);
    RUN_TEST(test_ECH_03_ECH_04_end_during_reminder_alert);
    RUN_TEST(test_ECH_07_reminder_after_end);
    RUN_TEST(test_ECH_08_simultaneous_deadlines);
    RUN_TEST(test_ECH_08_late_wakeup_after_both_deadlines);
    RUN_TEST(test_MOD_01_MOD_03_change_total);
    RUN_TEST(test_MOD_01_ECH_07_shorten_below_reminder);
    RUN_TEST(test_MOD_02_OUV_02_change_reminder);
    RUN_TEST(test_ACQ_02_reminder_to_zero);
    RUN_TEST(test_ACQ_03_OUV_01_change_acknowledges);
    RUN_TEST(test_ACQ_03_total_does_not_acknowledge_reminder);
    RUN_TEST(test_MOD_04_OUV_08_reactivate_reminder);
    RUN_TEST(test_ACQ_05_ERR_04_push_without_deadline);
    RUN_TEST(test_ERR_06_bounded_values);
    RUN_TEST(test_PLG_01_24_h_countdown);
    RUN_TEST(test_PLG_03_short_countdown);
    RUN_TEST(test_MAT_04_reduced_display);
    RUN_TEST(test_MAT_04_close_reminder_forces_full_display);
    RUN_TEST(test_MAT_04_full_display_after_adjust);
    RUN_TEST(test_GEN_05_wakeup_each_minute_in_reduced_display);
    RUN_TEST(test_GEN_05_wakeup_each_second_in_full_display);
    RUN_TEST(test_GEN_05_wakeup_when_switching_to_full_display);
    RUN_TEST(test_GEN_05_CST_06_wakeup_for_led_flashes);
    RUN_TEST(test_ERR_08_state_after_power_loss);
    return UNITY_END();
}
