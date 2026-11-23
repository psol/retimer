idf_component_register(SRCS "eink_display.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES constants esp_driver_gpio esp_driver_spi
                                     esp_timer)
