/*
 * Petite police 5 × 7 points, avec deux lignes au-dessus pour les accents
 * des majuscules (É, È, Ê, À, Ç). Dessinée pour Retimer : seuls les
 * caractères utiles à l'écran sont présents.
 */
#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#define FONT_WIDTH 5
#define FONT_HEIGHT 9 /* 2 lignes d'accent + 7 lignes */

/*
 * Rend les 9 lignes du caractère (5 bits de poids faible, bit 4 à gauche),
 * ou NULL s'il n'existe pas. code : point de code Unicode.
 */
const uint8_t *font_glyph(uint32_t code);

#endif /* FONT_H */
