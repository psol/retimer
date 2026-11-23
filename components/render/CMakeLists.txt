idf_component_register(SRCS "render.c" "font.c"
                       INCLUDE_DIRS "include"
                       REQUIRES kitchen_timer
                       PRIV_REQUIRES constants)
