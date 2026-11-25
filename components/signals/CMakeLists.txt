idf_component_register(SRCS "signals.c"
                       INCLUDE_DIRS "include"
                       REQUIRES constants
                       PRIV_REQUIRES esp_driver_gpio esp_driver_ledc)
