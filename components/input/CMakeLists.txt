# input.c est sans matériel (testé sur l'hôte). La lecture des boutons du
# CoreInk viendra avec les pilotes (étape 3).
idf_component_register(SRCS "input.c"
                       INCLUDE_DIRS "include"
                       REQUIRES constants)
