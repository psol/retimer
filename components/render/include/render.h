/*
 * Construction de l'image à afficher (CMD-05) : temps restants, alertes
 * inversées, clignotement du démarrage imminent (DEM-02), affichage réduit
 * (MAT-04), indicateur de batterie faible (ERR-07).
 *
 * L'image est un tampon 1 bit, sans rien savoir de l'écran : c'est le pilote
 * qui le convertit au format de son contrôleur.
 *
 * Disposition (CLAUDE.md, « Interface réduite ») : la minuterie globale en
 * haut, à côté de la molette ; la minuterie rappel en bas, à côté du bouton.
 */
#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include "kitchen_timer.h"

#define RENDER_WIDTH 200
#define RENDER_HEIGHT 200
#define RENDER_BYTES_PER_ROW ((RENDER_WIDTH + 7) / 8)
#define RENDER_SIZE (RENDER_BYTES_PER_ROW * RENDER_HEIGHT)

/* Ligne de séparation entre la zone globale et la zone rappel. */
#define RENDER_SEPARATOR_Y 128

/*
 * Image 1 bit, ligne par ligne, bit de poids fort à gauche.
 * Bit à 1 = point noir.
 */
typedef struct {
    uint8_t bytes[RENDER_SIZE];
} render_image_t;

typedef struct {
    bool low_battery; /* ERR-07 */
} render_options_t;

/* Dessine l'écran correspondant à la vue. */
void render_draw(render_image_t *image, const ktimer_view_t *view,
                 const render_options_t *options);

/*
 * Écran de message : deux lignes centrées, en grand puis en moyen (écran
 * d'extinction, application de diagnostic). Une ligne peut être NULL.
 */
void render_message(render_image_t *image, const char *line1,
                    const char *line2);

/* Lecture d'un point (tests, pilotes). */
bool render_pixel(const render_image_t *image, int x, int y);

/*
 * Texte d'une durée tel qu'il est affiché, par exemple « 12:34 », « 1:05:00 »
 * ou, en affichage réduit, « 12:-- ». Arrondi à la seconde (ou à la minute)
 * supérieure : « 00:01 » reste affiché jusqu'à l'échéance.
 * Le tampon doit faire au moins 10 caractères.
 */
void render_format_duration(int64_t duration_ms, bool reduced, char *text);

#endif /* RENDER_H */
