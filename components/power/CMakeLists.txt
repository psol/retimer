idf_component_register(SRCS "power.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES input esp_driver_gpio esp_driver_uart esp_adc esp_hw_support)
