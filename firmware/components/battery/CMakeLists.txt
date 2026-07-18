idf_component_register(SRCS "battery.c" "battery_calc.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_gpio esp_adc board_cfg)
