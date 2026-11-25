idf_component_register(SRCS "diagnostic.c"
                       PRIV_REQUIRES constants kitchen_timer render input eink_display
                                     signals power esp_driver_gpio esp_timer
                                     esp_hw_support)
