/*
 * Tables de constantes de la minuterie (GEN-06).
 *
 * Toutes les valeurs de réglage sont regroupées ici pour être ajustées
 * rapidement pendant les essais d'utilisation. Chaque constante cite
 * l'exigence ou le point de la partie 2 de SPEC.md qu'elle concerne.
 * Les durées sont en millisecondes.
 *
 * Ce composant ne dépend d'aucun matériel.
 */
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SECOND_MS ((int64_t)1000)
#define MINUTE_MS (60 * SECOND_MS)
#define HOUR_MS (60 * MINUTE_MS)

/* ---- Plages de réglage (PLG) -------------------------------------------- */

/* PLG-01 : la globale va de 5 s à 24 h ; 0 reste atteignable (ACQ-01). */
#define CST_TOTAL_MIN_MS (5 * SECOND_MS)
#define CST_TOTAL_MAX_MS (24 * HOUR_MS)

/* PLG-02 : le rappel va de 0 (désactivé) à 2 h. */
#define CST_REMINDER_MAX_MS (2 * HOUR_MS)

/* ---- Délais (DEM, ETA, ERR) --------------------------------------------- */

/* CST-01 : délai de démarrage (DEM-01). */
#define CST_START_DELAY_MS (3 * SECOND_MS)

/* CST-02 : délai d'inactivité en réglage avant la veille (ETA-12, ERR-01). */
#define CST_INACTIVITY_DELAY_MS (2 * MINUTE_MS)

/* CST-10 : délai d'abandon de l'alerte de fin non acquittée (ERR-03, ETA-21). */
#define CST_END_GIVE_UP_DELAY_MS (1 * HOUR_MS)

/* ---- Pas de réglage (REG-02, REG-04, CST-03) ---------------------------- */

/*
 * Pas d'un cran selon la valeur de départ. La table est triée par seuil
 * croissant : le pas retenu est celui de la dernière ligne dont le seuil est
 * inférieur ou égal à la valeur de départ.
 */
typedef struct {
    int64_t threshold_ms;
    int64_t step_ms;
} cst_tier_t;

extern const cst_tier_t CST_TOTAL_TIERS[];
extern const size_t CST_TOTAL_TIER_COUNT;

/* ---- Accélération au maintien (REG-03, CST-04) -------------------------- */

/*
 * Sur le CoreInk, l'accélération dépend de la durée du maintien de la
 * molette (CLAUDE.md, « Interface réduite »). Chaque ligne donne, à partir
 * d'un nombre de répétitions, le multiplicateur du pas.
 * Exemple avec un pas de 5 s : 5 s, puis 10 s, puis 30 s.
 */
typedef struct {
    uint32_t from_repeat;
    uint32_t multiplier;
} cst_acceleration_t;

extern const cst_acceleration_t CST_ACCELERATION[];
extern const size_t CST_ACCELERATION_COUNT;

/* Répétition au maintien de la molette : premier délai, puis période. */
#define CST_REPEAT_DELAY_MS 500
#define CST_REPEAT_PERIOD_MS 150

/* Anti-rebond des boutons. */
#define CST_DEBOUNCE_MS 20

/*
 * Interface réduite du CoreInk (CLAUDE.md) : le bouton du rappel est aussi le
 * bouton marche. Un appui maintenu aussi longtemps éteint l'appareil.
 */
#define POWER_OFF_HOLD_MS 3000

/* ---- Intervalles de rappel (CLAUDE.md, interface réduite) --------------- */

/* Liste parcourue par le bouton sous la molette ; 0 = arrêt (ACQ-02). */
extern const int64_t CST_REMINDER_INTERVALS_MS[];
extern const size_t CST_REMINDER_INTERVAL_COUNT;

/* ---- Comportements à valider en essai (OUV) ----------------------------- */

/*
 * OUV-01 / ACQ-03 : modifier une minuterie dont l'échéance n'est pas
 * acquittée vaut acquittement. A priori oui.
 */
#define CST_OUV01_CHANGE_ACKNOWLEDGES true

/*
 * OUV-07 / ETA-20 : en Terminé, une action de réglage acquitte l'alerte de
 * fin et commence un nouveau réglage. A priori oui.
 */
#define CST_OUV07_ADJUST_WHEN_FINISHED true

/* ---- Affichage (CMD-05, MAT-04, CST-07, CST-08) ------------------------- */

/* MAT-04 : affichage réduit (« 12:-- ») pour les longs décomptes. */
#define CST_REDUCED_DISPLAY true

/* CST-08 : en dessous de ce temps restant, l'affichage redevient complet. */
#define CST_FULL_DISPLAY_THRESHOLD_MS (1 * MINUTE_MS)

/* DEM-02 : période du clignotement de démarrage imminent (allumé + éteint). */
#define CST_BLINK_PERIOD_MS (1 * SECOND_MS)

/*
 * Rafraîchissement complet de l'écran e-ink contre les traces : au plus tard
 * après ce nombre de mises à jour partielles.
 */
#define CST_DISPLAY_PARTIALS_BEFORE_FULL 300

/* ---- Sonnerie et alertes (CMD-06, CST-05, CST-06) ----------------------- */

typedef struct {
    uint16_t frequency_hz; /* 0 = silence */
    uint16_t duration_ms;
} cst_note_t;

/* CST-05 : plusieurs bips brefs sur 1 à 2 s. */
extern const cst_note_t CST_REMINDER_RING[];
extern const size_t CST_REMINDER_RING_NOTE_COUNT;
extern const cst_note_t CST_END_RING[];
extern const size_t CST_END_RING_NOTE_COUNT;

/*
 * CST-06 : motif des alertes à l'écran. 0 = inversion fixe ; sinon période
 * d'un clignotement lent de la zone inversée.
 */
#define CST_REMINDER_ALERT_PERIOD_MS 0
#define CST_END_ALERT_PERIOD_MS 0

/* CST-06 : la LED complète l'écran pendant une alerte (éclat bref). */
#define CST_ALERT_LED true
#define CST_ALERT_LED_PERIOD_MS (4 * SECOND_MS)
#define CST_ALERT_LED_FLASH_MS 30

/* ---- Batterie (ERR-07, CST-09) ------------------------------------------ */

#define CST_LOW_BATTERY_MV 3500

#endif /* CONSTANTS_H */
