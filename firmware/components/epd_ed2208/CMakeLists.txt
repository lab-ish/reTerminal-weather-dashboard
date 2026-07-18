idf_component_register(SRCS "epd_ed2208.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_gpio esp_driver_spi board_cfg)
