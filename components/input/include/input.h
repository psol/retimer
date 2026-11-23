/*
 * Traduction des commandes en événements abstraits (CLAUDE.md, « Interface
 * réduite ») : anti-rebond et répétition au maintien de la molette.
 *
 * Deux parties :
 * - la logique (input.c), sans matériel, testée sur l'hôte : elle reçoit
 *   l'état des boutons et l'instant présent ;
 * - la lecture des boutons du CoreInk (input_coreink.c), seule dépendance au
 *   matériel, déclarée dans input_hw.h.
 */
#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    BUTTON_UP = 0,   /* molette haut */
    BUTTON_PUSH,     /* appui sur la molette */
    BUTTON_DOWN,     /* molette bas */
    BUTTON_REMINDER, /* bouton sous la molette */
    BUTTON_COUNT,
} input_button_t;

#define INPUT_MASK(b) (1u << (b))

typedef enum {
    EV_TOTAL_PLUS = 0, /* remplace CMD-01 */
    EV_TOTAL_MINUS,    /* remplace CMD-01 */
    EV_NEXT_REMINDER,  /* remplace CMD-02 */
    EV_ACKNOWLEDGE,    /* CMD-03 */
} input_type_t;

typedef struct {
    input_type_t type;
    /* 0 au premier cran d'un appui, puis 1, 2… pendant le maintien (REG-03). */
    uint32_t repeat;
} input_event_t;

typedef struct {
    bool raw;          /* dernier état lu */
    bool stable;       /* état après anti-rebond */
    int64_t change_ms; /* instant du dernier changement brut */
    int64_t next_repeat_ms;
    uint32_t repeat;
} input_button_state_t;

typedef struct {
    input_button_state_t buttons[BUTTON_COUNT];
} input_t;

void input_init(input_t *s);

/*
 * Traite l'état des boutons (masque INPUT_MASK des boutons enfoncés) à
 * l'instant donné. Écrit au plus max événements dans sortie et rend leur
 * nombre.
 */
size_t input_process(input_t *s, int64_t now, uint32_t pressed,
                     input_event_t *out, size_t max);

/*
 * Signale un appui déjà terminé, par exemple celui qui a réveillé la carte
 * de la veille profonde : la carte démarre trop lentement pour le voir.
 */
size_t input_wake_press(input_t *s, input_button_t button,
                        input_event_t *out, size_t max);

/* Vrai tant qu'un bouton est enfoncé ou qu'un changement est en attente. */
bool input_busy(const input_t *s);

/* Instant de la prochaine répétition, ou INT64_MAX. */
int64_t input_next_wakeup(const input_t *s);

#endif /* INPUT_H */
