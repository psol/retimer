/*
 * Lecture des boutons : seule partie de saisie qui dépend du matériel.
 * Un portage ne réécrit que ce fichier d'implémentation.
 */
#ifndef INPUT_HW_H
#define INPUT_HW_H

#include <stdint.h>

/* Configure les broches des boutons. */
void input_hw_init(void);

/* Masque INPUT_MASK() des boutons enfoncés à l'instant présent. */
uint32_t input_hw_read(void);

#endif /* INPUT_HW_H */
