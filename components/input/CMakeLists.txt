# input.c est sans matériel (testé sur l'hôte) ; coreink/ lit les boutons.
idf_component_register(SRCS "input.c" "coreink/input_coreink.c"
                       INCLUDE_DIRS "include"
                       REQUIRES constants
                       PRIV_REQUIRES esp_driver_gpio)
