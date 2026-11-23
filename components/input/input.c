/*
 * Logique de saisie, sans matériel. Voir input.h.
 *
 * Correspondance des commandes (CLAUDE.md, « Interface réduite ») :
 * molette haut / bas → globale plus / moins, avec répétition au maintien ;
 * bouton sous la molette → rappel suivant, et extinction s'il reste enfoncé
 * POWER_OFF_HOLD_MS (c'est aussi le bouton marche) ; appui sur la molette →
 * acquitter.
 */
#include "input.h"

#include "constants.h"

static bool repeated(input_button_t b)
{
    return b == BUTTON_UP || b == BUTTON_DOWN;
}

static input_type_t type_of(input_button_t b)
{
    switch (b) {
    case BUTTON_UP:
        return EV_TOTAL_PLUS;
    case BUTTON_DOWN:
        return EV_TOTAL_MINUS;
    case BUTTON_REMINDER:
        return EV_NEXT_REMINDER;
    case BUTTON_PUSH:
    default:
        return EV_ACKNOWLEDGE;
    }
}

void input_init(input_t *s)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        s->buttons[i].raw = false;
        s->buttons[i].stable = false;
        s->buttons[i].change_ms = 0;
        s->buttons[i].next_repeat_ms = INT64_MAX;
        s->buttons[i].repeat = 0;
    }
}

static size_t emit(input_type_t type, uint32_t repeat,
                   input_event_t *out, size_t n, size_t max)
{
    if (n < max) {
        out[n].type = type;
        out[n].repeat = repeat;
        return n + 1;
    }
    return n;
}

size_t input_process(input_t *s, int64_t now, uint32_t pressed,
                     input_event_t *out, size_t max)
{
    size_t n = 0;
    for (int i = 0; i < BUTTON_COUNT; i++) {
        input_button_state_t *b = &s->buttons[i];
        bool raw = (pressed & INPUT_MASK(i)) != 0;

        if (raw != b->raw) {
            b->raw = raw;
            b->change_ms = now;
        }

        /* Anti-rebond : l'état brut doit rester stable. */
        if (b->raw != b->stable &&
            now - b->change_ms >= CST_DEBOUNCE_MS) {
            b->stable = b->raw;
            if (b->stable) {
                /* Appui : un événement tout de suite. */
                b->repeat = 0;
                n = emit(type_of((input_button_t)i), 0, out, n, max);
                if (repeated((input_button_t)i)) {
                    b->next_repeat_ms = now + CST_REPEAT_DELAY_MS;
                } else if (i == BUTTON_REMINDER) {
                    b->next_repeat_ms = now + POWER_OFF_HOLD_MS;
                } else {
                    b->next_repeat_ms = INT64_MAX;
                }
            } else {
                b->next_repeat_ms = INT64_MAX;
            }
        }

        /* Bouton du rappel maintenu : extinction, une seule fois. */
        if (i == BUTTON_REMINDER && b->stable && now >= b->next_repeat_ms) {
            n = emit(EV_POWER_OFF, 0, out, n, max);
            b->next_repeat_ms = INT64_MAX;
        }

        /* Répétition au maintien (remplace REG-03, rotation continue). */
        if (b->stable && now >= b->next_repeat_ms) {
            b->repeat++;
            n = emit(type_of((input_button_t)i), b->repeat, out, n,
                     max);
            b->next_repeat_ms += CST_REPEAT_PERIOD_MS;
            if (b->next_repeat_ms <= now) {
                /* Réveil tardif : on ne rattrape pas les répétitions. */
                b->next_repeat_ms =
                    now + CST_REPEAT_PERIOD_MS;
            }
        }
    }
    return n;
}

size_t input_wake_press(input_t *s, input_button_t button,
                        input_event_t *out, size_t max)
{
    if (button >= BUTTON_COUNT) {
        return 0;
    }
    /*
     * Le bouton est peut-être encore enfoncé : on le note comme tel pour ne
     * pas compter l'appui deux fois. Pas de répétition pour cet appui.
     */
    input_button_state_t *b = &s->buttons[button];
    b->raw = true;
    b->stable = true;
    b->repeat = 0;
    b->next_repeat_ms = INT64_MAX;
    return emit(type_of(button), 0, out, 0, max);
}

void input_ignore_until_release(input_t *s, input_button_t button)
{
    if (button >= BUTTON_COUNT) {
        return;
    }
    input_button_state_t *b = &s->buttons[button];
    b->raw = true;
    b->stable = true;
    b->repeat = 0;
    b->next_repeat_ms = INT64_MAX;
}

bool input_busy(const input_t *s)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        const input_button_state_t *b = &s->buttons[i];
        if (b->raw || b->stable) {
            return true;
        }
    }
    return false;
}

int64_t input_next_wakeup(const input_t *s)
{
    int64_t t = INT64_MAX;
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (s->buttons[i].next_repeat_ms < t) {
            t = s->buttons[i].next_repeat_ms;
        }
    }
    return t;
}
