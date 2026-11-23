/* Logique de la minuterie. Voir kitchen_timer.h et SPEC.md, sections 5 à 10. */
#include "kitchen_timer.h"

#include "constants.h"

static int64_t min64(int64_t a, int64_t b) { return a < b ? a : b; }

/* ETA-01. */
static void to_idle(ktimer_t *m)
{
    ktimer_init(m);
}

/* Après une action de réglage hors décompte : ETA-10, ETA-11, ETA-13. */
static void after_adjust_action(ktimer_t *m, int64_t now)
{
    m->last_action_ms = now;
    if (m->total_ms > 0) {
        /* DEM-01, DEM-03 : le délai de démarrage (re)commence. */
        m->state = STATE_STARTING;
        m->limit_ms = now + CST_START_DELAY_MS;
    } else {
        /* ERR-01 : pas de démarrage ; veille après le délai d'inactivité. */
        m->state = STATE_SETTING;
        m->limit_ms = now + CST_INACTIVITY_DELAY_MS;
    }
}

/* DEM-04, ETA-14 : le décompte part de la fin du délai, pas du réveil. */
static void start_countdown(ktimer_t *m, int64_t start_value)
{
    m->state = STATE_RUNNING;
    m->end_ms = start_value + m->total_ms;
    m->reminder_ms = (m->interval_ms > 0) ? start_value + m->interval_ms
                                          : KTIMER_NEVER;
    m->full_until_ms = start_value + CST_FULL_DISPLAY_AT_START_MS;
    m->reminder_alert = false;
    m->end_alert = false;
    m->limit_ms = KTIMER_NEVER;
}

/* ECH-02, ECH-04, ETA-17. */
static void finish(ktimer_t *m)
{
    m->state = STATE_FINISHED;
    m->reminder_alert = false;
    m->reminder_ms = KTIMER_NEVER;
    m->end_alert = true;
    m->alert_start_ms = m->end_ms;
    /* CST-10, ERR-03, ETA-21. */
    m->limit_ms = m->end_ms + CST_END_GIVE_UP_DELAY_MS;
}

/* ECH-05 : la minuterie rappel repart de l'acquittement. */
static void restart_reminder(ktimer_t *m, int64_t now)
{
    m->reminder_alert = false;
    m->reminder_ms = (m->interval_ms > 0) ? now + m->interval_ms
                                          : KTIMER_NEVER;
}

void ktimer_init(ktimer_t *m)
{
    m->state = STATE_IDLE;
    m->total_ms = 0;
    m->end_ms = KTIMER_NEVER;
    m->interval_ms = 0;
    m->reminder_ms = KTIMER_NEVER;
    m->reminder_alert = false;
    m->end_alert = false;
    m->alert_start_ms = 0;
    m->limit_ms = KTIMER_NEVER;
    m->last_action_ms = 0;
    m->full_until_ms = 0;
}

uint32_t ktimer_advance(ktimer_t *m, int64_t now)
{
    uint32_t signals = SIGNAL_NONE;

    /* Une boucle, car un réveil tardif peut franchir plusieurs échéances. */
    for (;;) {
        switch (m->state) {
        case STATE_IDLE:
            return signals;

        case STATE_SETTING:
            if (now >= m->limit_ms) { /* ETA-12, ERR-01 */
                to_idle(m);
                continue;
            }
            return signals;

        case STATE_STARTING:
            if (now >= m->limit_ms) { /* DEM-04, ETA-14 */
                start_countdown(m, m->limit_ms);
                continue;
            }
            return signals;

        case STATE_RUNNING:
            /*
             * La globale d'abord : ECH-08 (échéances simultanées : seule la
             * globale sonne) et ECH-03 (la fin sonne même si une alerte de
             * rappel attend).
             */
            if (now >= m->end_ms) {
                finish(m);
                signals = (signals & ~SIGNAL_REMINDER_RING) |
                          SIGNAL_END_RING;
                continue;
            }
            /*
             * ECH-01. ECH-06 : pendant une alerte, reminder_ms vaut JAMAIS.
             * ECH-07 : une échéance de rappel après la fin ne sonne pas, ce
             * qu'assure le test précédent.
             */
            if (now >= m->reminder_ms) {
                m->reminder_alert = true;
                m->alert_start_ms = m->reminder_ms;
                m->reminder_ms = KTIMER_NEVER;
                signals |= SIGNAL_REMINDER_RING;
                continue;
            }
            return signals;

        case STATE_FINISHED:
            if (now >= m->limit_ms) { /* ETA-21, ERR-03 */
                to_idle(m);
                continue;
            }
            return signals;
        }
        return signals;
    }
}

uint32_t ktimer_set_total(ktimer_t *m, int64_t now,
                          int64_t new_total_ms)
{
    uint32_t signals = ktimer_advance(m, now);
    if (new_total_ms < 0) {
        new_total_ms = 0;
    }
    if (new_total_ms > CST_TOTAL_MAX_MS) { /* ERR-06 */
        new_total_ms = CST_TOTAL_MAX_MS;
    }

    switch (m->state) {
    case STATE_FINISHED:
        if (!CST_OUV07_ADJUST_WHEN_FINISHED) {
            return signals;
        }
        /* ETA-20 : acquitte l'alerte de fin, nouveau réglage. */
        to_idle(m);
        /* fallthrough */
    case STATE_IDLE: /* ETA-10 */
    case STATE_SETTING:
    case STATE_STARTING: /* ETA-13, DEM-03 */
        m->total_ms = new_total_ms;
        after_adjust_action(m, now);
        return signals;

    case STATE_RUNNING:
        if (new_total_ms == 0) { /* ACQ-01, ETA-18 */
            to_idle(m);
            return signals;
        }
        /* MOD-01, MOD-03 : le décompte continue depuis la nouvelle valeur. */
        m->end_ms = now + new_total_ms;
        m->full_until_ms = now + CST_FULL_DISPLAY_AFTER_ADJUST_MS;
        return signals;
    }
    return signals;
}

uint32_t ktimer_set_reminder(ktimer_t *m, int64_t now,
                             int64_t new_interval_ms)
{
    uint32_t signals = ktimer_advance(m, now);
    if (new_interval_ms < 0) {
        new_interval_ms = 0;
    }
    if (new_interval_ms > CST_REMINDER_MAX_MS) { /* PLG-02, ERR-06 */
        new_interval_ms = CST_REMINDER_MAX_MS;
    }

    switch (m->state) {
    case STATE_FINISHED:
        if (!CST_OUV07_ADJUST_WHEN_FINISHED) {
            return signals;
        }
        /* ETA-20 : acquitte l'alerte de fin, nouveau réglage. */
        to_idle(m);
        /* fallthrough */
    case STATE_IDLE: /* ETA-10 */
    case STATE_SETTING:
    case STATE_STARTING: /* DEM-03 */
        m->interval_ms = new_interval_ms;
        after_adjust_action(m, now);
        return signals;

    case STATE_RUNNING:
        m->interval_ms = new_interval_ms;
        m->full_until_ms = now + CST_FULL_DISPLAY_AFTER_ADJUST_MS;
        if (new_interval_ms == 0) {
            /* ACQ-02 : acquitte et désactive pour la suite du décompte. */
            m->reminder_alert = false;
            m->reminder_ms = KTIMER_NEVER;
            return signals;
        }
        if (m->reminder_alert && !CST_OUV01_CHANGE_ACKNOWLEDGES) {
            /* L'alerte attend ; le nouvel intervalle servira à la relance. */
            return signals;
        }
        /*
         * MOD-02 (lecture retenue pour OUV-02) : le nouvel intervalle vaut
         * pour le temps restant et pour les échéances suivantes ; la
         * prochaine échéance tombe un intervalle après la modification.
         * ACQ-03 / OUV-01 : si une alerte de rappel attendait, elle est
         * acquittée. MOD-04 / OUV-08 : réactive le rappel après ACQ-02.
         */
        restart_reminder(m, now);
        return signals;
    }
    return signals;
}

uint32_t ktimer_acknowledge(ktimer_t *m, int64_t now)
{
    uint32_t signals = ktimer_advance(m, now);
    switch (m->state) {
    case STATE_FINISHED: /* ACQ-04, ETA-19 */
        to_idle(m);
        break;
    case STATE_RUNNING:
        if (m->reminder_alert) { /* ACQ-04, ETA-16, ECH-05 */
            restart_reminder(m, now);
        }
        /* ACQ-05, ERR-04 : sans échéance en attente, aucun effet. */
        break;
    default: /* ACQ-05, ERR-04 */
        break;
    }
    return signals;
}

/* MAT-04 : réduit tant que les deux minuteries actives sont loin de zéro. */
static bool reduced_display(int64_t total_remaining, bool reminder_active,
                            int64_t reminder_remaining)
{
    if (!CST_REDUCED_DISPLAY) {
        return false;
    }
    if (total_remaining <= CST_FULL_DISPLAY_THRESHOLD_MS) {
        return false;
    }
    if (reminder_active &&
        reminder_remaining <= CST_FULL_DISPLAY_THRESHOLD_MS) {
        return false;
    }
    return true;
}

/* Phase d'un motif clignotant : vrai pendant la seconde moitié. */
static bool off_phase(int64_t start, int64_t period, int64_t now)
{
    if (period <= 0) {
        return false;
    }
    int64_t elapsed = now - start;
    if (elapsed < 0) {
        elapsed = 0;
    }
    return (elapsed % period) >= period / 2;
}

/* CST-06 : la LED brille brièvement au début de chaque période. */
static bool is_led_flash(int64_t start, int64_t now)
{
    if (!CST_ALERT_LED || CST_ALERT_LED_PERIOD_MS <= 0) {
        return false;
    }
    int64_t elapsed = now - start;
    if (elapsed < 0) {
        return false;
    }
    return (elapsed % CST_ALERT_LED_PERIOD_MS) < CST_ALERT_LED_FLASH_MS;
}

void ktimer_view(const ktimer_t *m, int64_t now,
                 ktimer_view_t *view)
{
    ktimer_t copy = *m;
    ktimer_advance(&copy, now);
    m = &copy;

    view->state = m->state;
    view->total_ms = 0;
    view->reminder = (m->interval_ms > 0) ? REMINDER_SET : REMINDER_OFF;
    view->reminder_ms = m->interval_ms;
    view->reminder_alert = m->reminder_alert;
    view->end_alert = m->end_alert;
    view->alert_inverted = false;
    view->blink_hidden = false;
    view->reduced = false;
    view->led_flash = false;

    switch (m->state) {
    case STATE_IDLE:
        view->reminder = REMINDER_OFF;
        view->reminder_ms = 0;
        break;

    case STATE_SETTING:
        view->total_ms = m->total_ms;
        break;

    case STATE_STARTING: /* DEM-02 */
        view->total_ms = m->total_ms;
        view->blink_hidden = off_phase(
            m->last_action_ms, CST_BLINK_PERIOD_MS, now);
        break;

    case STATE_RUNNING: {
        view->total_ms = m->end_ms - now;
        bool reminder_active = false;
        int64_t reminder_remaining = 0;
        if (m->reminder_alert) {
            view->reminder = REMINDER_ALERT;
            view->reminder_ms = 0;
            view->alert_inverted = !off_phase(
                m->alert_start_ms, CST_REMINDER_ALERT_PERIOD_MS, now);
            view->led_flash = is_led_flash(m->alert_start_ms, now);
        } else if (m->interval_ms == 0) {
            view->reminder = REMINDER_OFF;
            view->reminder_ms = 0;
        } else if (m->reminder_ms >= m->end_ms) {
            view->reminder = REMINDER_SPENT; /* ECH-07 */
            view->reminder_ms = 0;
        } else {
            view->reminder = REMINDER_RUNNING;
            reminder_remaining = m->reminder_ms - now;
            view->reminder_ms = reminder_remaining;
            reminder_active = true;
        }
        view->reduced = now >= m->full_until_ms &&
                        reduced_display(view->total_ms, reminder_active,
                                        reminder_remaining);
        break;
    }

    case STATE_FINISHED: /* ETA-07 : « 00:00 » */
        view->led_flash = is_led_flash(m->alert_start_ms, now);
        view->reminder = REMINDER_OFF;
        view->reminder_ms = 0;
        view->alert_inverted = !off_phase(
            m->alert_start_ms, CST_END_ALERT_PERIOD_MS, now);
        break;
    }
}

/*
 * Prochain changement de l'affichage. Complet : la valeur affichée est
 * s = ceil(restant / 1 s) ; elle change quand restant atteint (s - 1) s.
 * Réduit : les minutes de l'affichage complet, s / 60 ; elles changent quand
 * s passe à 60 × (s / 60) - 1 (render_format_duration).
 */
static int64_t next_change(int64_t deadline, int64_t now, bool reduced)
{
    int64_t remaining = deadline - now;
    if (remaining <= 0) {
        return KTIMER_NEVER;
    }
    int64_t seconds = (remaining + SECOND_MS - 1) / SECOND_MS;
    if (reduced) {
        return deadline - ((seconds / 60) * 60 - 1) * SECOND_MS;
    }
    return deadline - (seconds - 1) * SECOND_MS;
}

/* Prochain changement de phase d'un motif de période donnée. */
static int64_t next_phase(int64_t start, int64_t period,
                          int64_t now)
{
    if (period <= 0) {
        return KTIMER_NEVER;
    }
    int64_t half = period / 2;
    if (half <= 0) {
        half = 1;
    }
    int64_t elapsed = now - start;
    if (elapsed < 0) {
        return start;
    }
    return start + (elapsed / half + 1) * half;
}

/* Prochain instant debut + k × periode strictement après maintenant. */
static int64_t next_multiple(int64_t start, int64_t period,
                             int64_t now)
{
    if (period <= 0) {
        return KTIMER_NEVER;
    }
    int64_t elapsed = now - start;
    if (elapsed < 0) {
        return start;
    }
    return start + (elapsed / period + 1) * period;
}

/* Réveils d'affichage d'une minuterie qui décompte vers echeance. */
static int64_t display_wakeup(int64_t deadline, int64_t now,
                              bool reduced)
{
    int64_t t = next_change(deadline, now, reduced);
    if (reduced) {
        /* Passage à l'affichage complet (CST-08). */
        int64_t toggle = deadline - CST_FULL_DISPLAY_THRESHOLD_MS;
        if (toggle > now) {
            t = min64(t, toggle);
        }
    }
    return t;
}

int64_t ktimer_next_wakeup(const ktimer_t *m, int64_t now)
{
    ktimer_t copy = *m;
    ktimer_advance(&copy, now);
    m = &copy;

    ktimer_view_t view;
    ktimer_view(m, now, &view);

    int64_t t = KTIMER_NEVER;
    switch (m->state) {
    case STATE_IDLE:
        break;

    case STATE_SETTING:
        t = m->limit_ms;
        break;

    case STATE_STARTING:
        t = min64(m->limit_ms,
                  next_phase(m->last_action_ms,
                             CST_BLINK_PERIOD_MS, now));
        break;

    case STATE_RUNNING:
        t = m->end_ms;
        t = min64(t, display_wakeup(m->end_ms, now, view.reduced));
        if (m->full_until_ms > now) {
            t = min64(t, m->full_until_ms); /* retour à l'affichage réduit */
        }
        if (view.reminder == REMINDER_RUNNING) {
            t = min64(t, m->reminder_ms);
            t = min64(t, display_wakeup(m->reminder_ms, now,
                                        view.reduced));
        }
        if (m->reminder_alert) {
            t = min64(t, next_phase(m->alert_start_ms,
                                    CST_REMINDER_ALERT_PERIOD_MS,
                                    now));
            if (CST_ALERT_LED) {
                t = min64(t, next_multiple(m->alert_start_ms,
                                           CST_ALERT_LED_PERIOD_MS,
                                             now));
            }
        }
        break;

    case STATE_FINISHED:
        t = m->limit_ms;
        t = min64(t, next_phase(m->alert_start_ms,
                                CST_END_ALERT_PERIOD_MS, now));
        if (CST_ALERT_LED) {
            t = min64(t, next_multiple(m->alert_start_ms,
                                       CST_ALERT_LED_PERIOD_MS,
                                         now));
        }
        break;
    }
    return t;
}
