idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES constants controller adjust kitchen_timer render input
                                     eink_display signals power esp_hw_support)
