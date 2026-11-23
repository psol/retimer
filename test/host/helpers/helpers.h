/* Outils communs aux tests sur l'hôte. */
#ifndef HELPERS_H
#define HELPERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Image 1 bit (bit à 1 = noir, bit de poids fort à gauche) écrite au format
 * PBM binaire (P4), lisible par la plupart des visionneuses.
 */
bool helpers_write_pbm(const char *path, const uint8_t *bytes, int width,
                       int height);

/*
 * Lit une image PBM P4 de la taille attendue. Rend faux si le fichier
 * n'existe pas ou n'a pas la bonne taille.
 */
bool helpers_read_pbm(const char *path, uint8_t *bytes, int width,
                      int height);

/*
 * Test d'instantané : écrit l'image dans RETIMER_OUTPUTS/<nom>.pbm et la
 * compare à RETIMER_REFERENCES/<nom>.pbm. Avec la variable d'environnement
 * RETIMER_UPDATE_REFERENCES=1, la référence est remplacée par l'image.
 * Rend le nombre de points différents (0 si identique), ou -1 si la
 * référence manque.
 */
long helpers_snapshot(const char *name, const uint8_t *bytes, int width,
                      int height);

#endif /* HELPERS_H */
