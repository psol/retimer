/*
 * Pilote de l'écran e-ink 200 × 200 du CoreInk (docs/coreink.md, section 2).
 *
 * Reçoit une image 1 bit (bit à 1 = noir, bit de poids fort à gauche, 25
 * octets par ligne) et choisit entre mise à jour partielle et
 * rafraîchissement complet périodique contre les traces
 * (CST_DISPLAY_PARTIALS_BEFORE_FULL).
 *
 * Seul l'écran GDEW0154M09 (contrôleur JD79653A) des CoreInk fabriqués
 * avant fin 2024 est géré : c'est celui de la carte de Benoît.
 */
#ifndef EINK_DISPLAY_H
#define EINK_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#define EINK_WIDTH 200
#define EINK_HEIGHT 200
#define EINK_SIZE (EINK_WIDTH / 8 * EINK_HEIGHT)

/*
 * Initialise le bus et l'écran. known_image : vrai si l'écran montre déjà
 * l'image passée (réveil de veille profonde, l'image e-ink est restée) ;
 * sinon NULL, et le prochain affichage sera complet.
 */
bool eink_init(const uint8_t *known_image);

/*
 * Affiche l'image. Ne fait rien si elle est identique à la précédente.
 * complet : force un rafraîchissement complet.
 */
void eink_show(const uint8_t *image, bool full);

/* Met le contrôleur en veille ; l'image reste affichée. */
void eink_sleep(void);

#endif /* EINK_DISPLAY_H */
