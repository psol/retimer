/*
 * Commande de la minuterie (controller.h). Sans dépendance au matériel.
 */
#include "controller.h"

#include "adjust.h"

void controller_init(controller_t *c)
{
    c->revealing = false;
    c->revealed_type = EV_TOTAL_PLUS;
    c->was_inverted = false;
}

static bool is_adjustment(input_type_t type)
{
    return type == EV_TOTAL_PLUS || type == EV_TOTAL_MINUS ||
           type == EV_NEXT_REMINDER;
}

controller_result_t controller_apply(controller_t *c, ktimer_t *m,
                                     const input_event_t *ev, int64_t now)
{
    controller_result_t r = {CONTROLLER_NONE, SIGNAL_NONE, 0, 0};
    ktimer_view_t view;
    ktimer_view(m, now, &view);

    /*
     * MAT-04, affichage réduit : le premier cran (molette ou bouton du
     * rappel) ne fait que montrer les secondes ; les crans suivants règlent
     * (soirée 2). Les répétitions de ce même appui, si la molette reste
     * tenue, ne règlent pas non plus (soirée 2, E18 : 44:02 devenait 44:00).
     */
    if (ev->repeat == 0) {
        c->revealing = false;
    } else if (c->revealing && ev->type == c->revealed_type) {
        return r;
    }
    if (view.reduced && ev->repeat == 0 && is_adjustment(ev->type)) {
        ktimer_reveal(m, now);
        c->revealing = true;
        c->revealed_type = ev->type;
        r.action = CONTROLLER_REVEALED;
        return r;
    }

    switch (ev->type) {
    case EV_TOTAL_PLUS:
    case EV_TOTAL_MINUS: {
        /* REG-01 : on part de la valeur affichée. */
        adjust_direction_t direction =
            ev->type == EV_TOTAL_PLUS ? ADJUST_UP : ADJUST_DOWN;
        r.before_ms = view.state == STATE_FINISHED ? 0 : view.total_ms;
        r.after_ms = adjust_total(r.before_ms, direction, ev->repeat);
        r.signals = ktimer_set_total(m, now, r.after_ms);
        r.action = CONTROLLER_ADJUSTED;
        break;
    }
    case EV_NEXT_REMINDER: {
        r.before_ms = view.state == STATE_FINISHED ? 0 : m->interval_ms;
        int64_t total = view.state == STATE_FINISHED ? 0 : view.total_ms;
        r.after_ms = adjust_next_reminder(r.before_ms, total);
        r.signals = ktimer_set_reminder(m, now, r.after_ms);
        r.action = CONTROLLER_ADJUSTED;
        break;
    }
    case EV_ACKNOWLEDGE:
        r.signals = ktimer_acknowledge(m, now);
        r.action = CONTROLLER_ACKNOWLEDGED;
        break;
    case EV_POWER_OFF:
        r.action = CONTROLLER_POWER_OFF;
        break;
    }
    return r;
}

bool controller_full_refresh(controller_t *c, const ktimer_view_t *view)
{
    bool inverted =
        view->alert_inverted && (view->end_alert || view->reminder_alert);
    bool full = inverted != c->was_inverted;
    c->was_inverted = inverted;
    return full;
}

const cst_note_t *controller_ring(uint32_t signals, size_t *count)
{
    if (signals & SIGNAL_END_RING) {
        *count = CST_END_RING_NOTE_COUNT;
        return CST_END_RING;
    }
    if (signals & SIGNAL_REMINDER_RING) {
        *count = CST_REMINDER_RING_NOTE_COUNT;
        return CST_REMINDER_RING;
    }
    *count = 0;
    return NULL;
}

bool controller_low_battery(uint32_t battery_mv)
{
    /* Sur USB, la mesure donne la tension de charge. */
    return battery_mv > 0 && battery_mv < CST_LOW_BATTERY_MV;
}

int64_t controller_next_wakeup(const ktimer_t *m, int64_t now,
                               int64_t input_wakeup)
{
    int64_t wakeup = ktimer_next_wakeup(m, now);
    return input_wakeup < wakeup ? input_wakeup : wakeup;
}
