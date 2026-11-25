/*
 * Logique de la minuterie : états (ETA), démarrage (DEM), échéances (ECH),
 * modifications en cours de décompte (MOD), acquittements (ACQ) et erreurs
 * (ERR) de SPEC.md.
 *
 * Le module ne lit ni horloge ni bouton : chaque fonction reçoit l'instant
 * présent (en millisecondes, horloge monotone) et rend les signaux à émettre.
 * Toutes les fonctions qui reçoivent « maintenant » traitent d'abord les
 * échéances passées. L'état tient dans une structure simple, copiable en
 * mémoire RTC pendant la veille profonde.
 */
#ifndef KITCHEN_TIMER_H
#define KITCHEN_TIMER_H

#include <stdbool.h>
#include <stdint.h>

#define KTIMER_NEVER INT64_MAX

/* ETA-01 à ETA-05. */
typedef enum {
    STATE_IDLE = 0,
    STATE_SETTING,
    STATE_STARTING,
    STATE_RUNNING,
    STATE_FINISHED,
} ktimer_state_t;

/* Signaux à émettre, combinables. */
enum {
    SIGNAL_NONE = 0,
    SIGNAL_REMINDER_RING = 1u << 0, /* ECH-01 */
    SIGNAL_END_RING = 1u << 1,      /* ECH-02 */
};

typedef struct {
    ktimer_state_t state;
    /* Réglage, Démarrage : valeur réglée de la globale. */
    int64_t total_ms;
    /* Décompte : instant de l'échéance de la globale. */
    int64_t end_ms;
    /* Intervalle de la minuterie rappel ; 0 = arrêt. */
    int64_t interval_ms;
    /* Décompte : instant de la prochaine échéance de rappel. */
    int64_t reminder_ms;
    bool reminder_alert;
    bool end_alert;
    /* Instant de début de l'alerte en cours (motifs d'alerte). */
    int64_t alert_start_ms;
    /*
     * Réglage : fin du délai d'inactivité (CST-02).
     * Démarrage : fin du délai de démarrage (CST-01).
     * Terminé : fin du délai d'abandon de l'alerte de fin (CST-10).
     */
    int64_t limit_ms;
    /* Démarrage : instant de la dernière action (phase du clignotement). */
    int64_t last_action_ms;
    /*
     * Décompte : affichage complet jusqu'à cet instant après un réglage
     * (CST_FULL_DISPLAY_AFTER_ADJUST_MS), pour voir la valeur réglée.
     */
    int64_t full_until_ms;
} ktimer_t;

/* Ce que la minuterie rappel montre à l'écran. */
typedef enum {
    REMINDER_OFF = 0, /* intervalle nul */
    REMINDER_SET,     /* hors décompte : intervalle réglé */
    REMINDER_RUNNING, /* décompte : temps restant avant l'échéance */
    REMINDER_ALERT,   /* échéance non acquittée (ECH-01, ECH-06) */
    REMINDER_SPENT,   /* prochaine échéance après la fin (ECH-07) */
} ktimer_reminder_t;

/* État à afficher, calculé pour un instant donné. */
typedef struct {
    ktimer_state_t state;
    /* Réglage, Démarrage : valeur réglée ; Décompte : temps restant ; 0 sinon. */
    int64_t total_ms;
    ktimer_reminder_t reminder;
    /* REMINDER_SET : intervalle ; REMINDER_RUNNING : temps restant. */
    int64_t reminder_ms;
    bool reminder_alert;
    bool end_alert;
    /* Phase d'un motif d'alerte clignotant (CST-06) : zone inversée ou non. */
    bool alert_inverted;
    /* DEM-02 : phase éteinte du clignotement de démarrage imminent. */
    bool blink_hidden;
    /* MAT-04 : affichage réduit, sans les secondes. */
    bool reduced;
    /* CST-06 : instant d'un éclat de la LED, en complément d'une alerte. */
    bool led_flash;
} ktimer_view_t;

/* État initial : Veille. ERR-08 : c'est aussi l'état après une coupure. */
void ktimer_init(ktimer_t *m);

/* Traite les échéances jusqu'à maintenant. Rend les signaux à émettre. */
uint32_t ktimer_advance(ktimer_t *m, int64_t now);

/*
 * Nouvelle valeur de la minuterie globale, calculée par l'appelant à partir
 * de ktimer_view().total_ms (REG-01). 0 arrête tout (ACQ-01).
 */
uint32_t ktimer_set_total(ktimer_t *m, int64_t now,
                          int64_t new_total_ms);

/* Nouvel intervalle de la minuterie rappel ; 0 la désactive (ACQ-02). */
/*
 * MAT-04 : pendant le décompte, repasse en affichage complet pour
 * CST_FULL_DISPLAY_AFTER_ADJUST_MS, sans rien régler. Sur le CoreInk, le
 * premier cran donné en affichage réduit ne fait que montrer les secondes.
 */
void ktimer_reveal(ktimer_t *m, int64_t now);

uint32_t ktimer_set_reminder(ktimer_t *m, int64_t now,
                             int64_t new_interval_ms);

/* Bouton d'acquittement (ACQ-04, ACQ-05). */
uint32_t ktimer_acknowledge(ktimer_t *m, int64_t now);

/* État à afficher à l'instant donné (ne modifie pas la minuterie). */
void ktimer_view(const ktimer_t *m, int64_t now,
                 ktimer_view_t *view);

/*
 * Instant du prochain réveil nécessaire (GEN-05) : échéance, délai,
 * changement de l'affichage ou phase d'un motif. KTIMER_NEVER en Veille.
 */
int64_t ktimer_next_wakeup(const ktimer_t *m, int64_t now);

#endif /* KITCHEN_TIMER_H */
